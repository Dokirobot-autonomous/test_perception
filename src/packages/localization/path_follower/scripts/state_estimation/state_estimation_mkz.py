#!/usr/bin/env python

import rospy
from numpy import zeros, array, append, eye, diag
from math import sqrt
from tf import transformations
import tf2_ros
from ekf import ekf
from std_msgs.msg import Header
from sensor_msgs.msg import Imu
from geometry_msgs.msg import Pose2D, Quaternion, TwistStamped, PoseStamped, TransformStamped
from nav_msgs.msg import Odometry
from path_follower.msg import state_Dynamic
from dbw_mkz_msgs.msg import SteeringReport
from system_models import f_BicycleModel, h_BicycleModel_withGPS, h_BicycleModel_withoutGPS

#setting parameters
a = 1.20;                          #m
b = 1.65;                          #m
ftire_stiffness = 140000.0;        #N/rad
rtire_stiffness = 120000.0;        #N/rad
mass = 1800.0;                     #kg
iz = 3270.0;                       #kg*m^2

vhMdl = (a,b,mass,iz)
trMdl = (ftire_stiffness,rtire_stiffness)

# measurement
Vx_meas       = 0
Yaw_meas      = 0
IMU_time      = 0
Yaw_meas_prev = 0
IMU_time_prev = 0
Yawrate_meas  = 0
X_meas        = 0
Y_meas        = 0
ax_meas       = 0
ay_meas       = 0
delta_meas    = 0

relative_quaternion = zeros((1,4))
quaternion          = (0,0,0,0)
quaternion_prev     = (0,0,0,0)
GPS_read            = 0
Yaw_initialize      = 0
GPS_initialize      = 0
IMU_start           = 0

header=Header()

def RelativeOrientationCallback(data):
	global relative_quaternion
	global Yaw_initialize
	relative_quaternion = (data.x,data.y,data.z,data.w)
	Yaw_initialize = 1

# def SteeringReportCallback(data):
# 	global Vx_meas
# 	global delta_meas
# 	Vx_meas = data.speed
# 	delta_meas = data.steering_wheel_angle / 14.8

def TwistCallback(data):
	global Vx_meas
	global delta_meas
	Vx_meas = data.twist.linear.x
	delta_meas = data.twist.angular.z

def CurrentPose2DCallback(data):
	global Yaw_meas, X_meas, Y_meas
	global GPS_read, GPS_initialize
	Yaw_meas = data.theta
	X_meas = data.x
	Y_meas = data.y
	GPS_read = 1
	GPS_initialize = 1

def CurrentPoseStampedCallback(data):
	global Yaw_meas, X_meas, Y_meas
	global GPS_read, GPS_initialize
	global header
	# Yaw_meas = data.theta
	X_meas = data.pose.position.x
	Y_meas = data.pose.position.y
	(roll,pitch,Yaw_meas)=transformations.euler_from_quaternion((data.pose.orientation.x,data.pose.orientation.y,data.pose.orientation.z,data.pose.orientation.w))
	header=data.header
	GPS_read = 1
	GPS_initialize = 1

def IMUCallback(data):
	global Yaw_meas, Yaw_meas_prev, Yawrate_meas
	global IMU_time, IMU_time_prev
	global Yaw_initialize, IMU_start
	global ax_meas, ay_meas
	global quaternion_prev, quaternion

	ori        = data.orientation
	quaternion = (ori.x, ori.y, ori.z, ori.w)

	if Yaw_initialize == 0:
		(roll, pitch, Yaw_meas)      = transformations.euler_from_quaternion(quaternion)
	else:
		actual_quaternion            = transformations.quaternion_multiply(quaternion,relative_quaternion)
		(roll, pitch, Yaw_meas)      = transformations.euler_from_quaternion(actual_quaternion)
		(roll, pitch, Yaw_meas_prev) = transformations.euler_from_quaternion(transformations.quaternion_multiply(quaternion_prev, relative_quaternion))

	IMU_time = float(str(data.header.stamp))

	if IMU_start == 0:
		IMU_start    = 1
	else:
		Yawrate_meas = (Yaw_meas-Yaw_meas_prev)/(IMU_time-IMU_time_prev)

	Yaw_meas_prev    = Yaw_meas
	quaternion_prev  = quaternion
	IMU_time_prev    = IMU_time

	ax_meas          = data.linear_acceleration.x
	ay_meas          = data.linear_acceleration.y

def state_estimation():
	global Vx_meas, Yaw_meas, X_meas, Y_meas, delta_meas
	global Yaw_initialize, GPS_initialize, GPS_read
	global start_X, start_Y, pub_flag
	global header

	# initialize node
	rospy.init_node('state_estimation', anonymous=True)

	# topic subscriptions / publications
	rospy.Subscriber('xsens/imu/data', Imu, IMUCallback)
	# rospy.Subscriber('vehicle/steering_report', SteeringReport, SteeringReportCallback)
	rospy.Subscriber('vehicle/twist', TwistStamped, TwistCallback)
	# rospy.Subscriber('current_pose_2D', Pose2D, CurrentPose2DCallback)
	rospy.Subscriber('current_pose', PoseStamped, CurrentPoseStampedCallback)
	rospy.Subscriber('relative_quaternion', Quaternion , RelativeOrientationCallback)
	state_pub = rospy.Publisher('state_estimate', state_Dynamic, queue_size = 10)
	odom_pub = rospy.Publisher('odom', Odometry, queue_size = 10)

	# set node rate
	loop_rate = 50
	dt        = 1.0 / loop_rate
	rate      = rospy.Rate(loop_rate)

	# estimation variables for Luemberger observer
	z_EKF            = array([0.,0.,0.,0.,0.,0.])
	z_EKF_prev       = array([0.,0.,0.,0.,0.,0.])
	states_est       = array([0.,0.,0.,0.,0.,0.])
	state_initialize = 0
	state_est_obj    = state_Dynamic()
	odom_est_obj     = Odometry()
	odom_est_obj.child_frame_id=rospy.get_param('~base_link')
	br				 = tf2_ros.TransformBroadcaster()
	tf				 = TransformStamped()
	tf.child_frame_id= rospy.get_param('~base_link');
	tf_publish		 = rospy.get_param('~tf_publish')

	# estimation variables for EKF
	var_gps   = 1.0e-05
	var_v     = 1.0e-04
	var_psi   = 1.0e-06

	var_ax    = 1.0e-04
	var_ay    = 1.0e-04
	var_delta = 1.0e-04
	var_noise = 1.0e-04

	P         = eye(6)    # initial dynamics coveriance matrix
	Q         = diag(array([var_ax, var_delta, var_noise, var_noise, var_noise, var_noise]))     # process noise coveriance matrix
	R2         = diag(array([var_v, var_gps, var_gps, var_psi]))     # measurement noise coveriance matrix
	R1         = diag(array([var_v, var_psi, var_ay]))

	while (rospy.is_shutdown() != 1):

		#print((Yaw_initialize, GPS_initialize))

		if Yaw_initialize == 0 or GPS_initialize == 0:
			print("not initialized")
			print(Yaw_initialize)
			print(GPS_initialize)
			rate.sleep()
			continue

		elif state_initialize == 0:
			z_EKF            = array([Vx_meas, 0, X_meas, Y_meas, Yaw_meas, Yawrate_meas])
			state_est        = z_EKF
			state_initialize = 1

		else:
			u_ekf      = array([ax_meas, delta_meas])
			w_ekf      = array([0., 0., 0., 0., 0., 0.])
			args       = (u_ekf, vhMdl, trMdl, dt)
			z_EKF_prev = z_EKF

			if GPS_read == 0:
				y_ekf      = array([Vx_meas, Yaw_meas, ay_meas])
				v_ekf      = array([0.,0.,0.])
				(z_EKF, P) = ekf(f_BicycleModel, z_EKF, w_ekf, v_ekf, P, h_BicycleModel_withoutGPS, y_ekf, Q, R1, args)
			else:
				y_ekf      = array([Vx_meas, X_meas, Y_meas, Yaw_meas])
				v_ekf      = array([0.,0.,0.,0.])

				(z_EKF, P) = ekf(f_BicycleModel, z_EKF, w_ekf, v_ekf, P, h_BicycleModel_withGPS, y_ekf, Q, R2, args)
				GPS_read   = 0

			state_est = z_EKF

		rospy.loginfo(P)
		state_est_obj.vx = state_est[0]
		state_est_obj.vy = state_est[1]
		state_est_obj.X  = state_est[2]
		state_est_obj.Y  = state_est[3]
		state_est_obj.psi= state_est[4]
		state_est_obj.wz = state_est[5]
		state_pub.publish(state_est_obj)
		odom_est_obj.header=header
		odom_est_obj.twist.twist.linear.x = state_est[0]
		odom_est_obj.twist.twist.linear.y = state_est[1]
		odom_est_obj.pose.pose.position.x  = state_est[2]
		odom_est_obj.pose.pose.position.y  = state_est[3]
		(odom_est_obj.pose.pose.orientation.x,odom_est_obj.pose.pose.orientation.y,odom_est_obj.pose.pose.orientation.z,odom_est_obj.pose.pose.orientation.w)=transformations.quaternion_from_euler(0.0,0.0,state_est[4])
		odom_est_obj.twist.twist.angular.z = state_est[5]
		odom_pub.publish(odom_est_obj)
		if tf_publish:
			tf.header=header
			tf.transform.translation.x = odom_est_obj.pose.pose.position.x
			tf.transform.translation.y = odom_est_obj.pose.pose.position.y
			tf.transform.translation.z = odom_est_obj.pose.pose.position.z
			tf.transform.rotation.x = odom_est_obj.pose.pose.orientation.x
			tf.transform.rotation.y = odom_est_obj.pose.pose.orientation.y
			tf.transform.rotation.z = odom_est_obj.pose.pose.orientation.z
			tf.transform.rotation.w = odom_est_obj.pose.pose.orientation.w
			br.sendTransform(tf)

		rate.sleep()

if __name__ == '__main__':
	try:
		state_estimation()
	except rospy.ROSInterruptException:
		pass