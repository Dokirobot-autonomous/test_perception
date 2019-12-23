import subprocess
import os
import time

# dir = "/home/msc-map/spinnaker_compressed_ws/devel/lib/pkgconfig"
# os.environ['PKG_CONFIG_PATH'] = '%s:%s' % (os.environ['PKG_CONFIG_PATH'], dir) 
# print(os.environ['PKG_CONFIG_PATH'])
subprocess.Popen("gnome-terminal -e 'bash -c \"export ROS_NAMESPACE=/prius; roslaunch ~/catkin_ws/src/spinnaker_camera_driver/launch/acquisition0.launch; exec bash\"'", stdin=subprocess.PIPE, shell=True)
time.sleep(1.0)

subprocess.Popen("gnome-terminal -e 'bash -c \"export ROS_NAMESPACE=/prius; roslaunch ~/catkin_ws/src/spinnaker_camera_driver/launch/acquisition1.launch; exec bash\"'", stdin=subprocess.PIPE, shell=True)
time.sleep(1.0)

subprocess.Popen("gnome-terminal -e 'bash -c \"export ROS_NAMESPACE=/prius; roslaunch ~/catkin_ws/src/spinnaker_camera_driver/launch/acquisition2.launch; exec bash\"'", stdin=subprocess.PIPE, shell=True)
time.sleep(1.0)

subprocess.Popen("gnome-terminal -e 'bash -c \"export ROS_NAMESPACE=/prius; roslaunch ~/catkin_ws/src/spinnaker_camera_driver/launch/acquisition3.launch; exec bash\"'", stdin=subprocess.PIPE, shell=True)
time.sleep(1.0)

subprocess.Popen("gnome-terminal -e 'bash -c \"export ROS_NAMESPACE=/prius; roslaunch ~/catkin_ws/src/spinnaker_camera_driver/launch/acquisition4.launch; exec bash\"'", stdin=subprocess.PIPE, shell=True)
time.sleep(1.0)

subprocess.Popen("gnome-terminal -e 'bash -c \"export ROS_NAMESPACE=/prius; roslaunch ~/catkin_ws/src/spinnaker_camera_driver/launch/acquisition5.launch; exec bash\"'", stdin=subprocess.PIPE, shell=True)
time.sleep(2.0)

subprocess.Popen("gnome-terminal -e 'bash -c \"export ROS_NAMESPACE=/prius; roslaunch ~/catkin_ws/src/ros_rslidar/rslidar_pointcloud/launch/rs_lidar_32.launch; exec bash\"'", stdin=subprocess.PIPE, shell=True)
time.sleep(2.0)

subprocess.Popen("gnome-terminal -e 'bash -c \"export ROS_NAMESPACE=/prius; cd ~/16_novatel_span_driver-master; source devel/setup.bash; roslaunch novatel_span_driver span_cpt.launch; exec bash\"'", stdin=subprocess.PIPE, shell=True)
time.sleep(2.0)

subprocess.Popen("gnome-terminal -e 'bash -c \"export ROS_NAMESPACE=/prius; cd ~/catkin_ws/src/ublox; source devel/setup.bash; roslaunch src/ublox/ublox_gps/launch/ublox_device.launch; exec bash\"'", stdin=subprocess.PIPE, shell=True)
time.sleep(2.0)

subprocess.Popen("gnome-terminal -e 'bash -c \"ROS_NAMESPACE=/prius; cd ~/Autoware/ros; ./run; exec bash\"'", stdin=subprocess.PIPE, shell=True)
time.sleep(2.0)


