//
// Created by ohashi on 19/10/24.
//

#ifndef SRC_PHONE_LOCALIZER_H
#define SRC_PHONE_LOCALIZER_H

#include <sys/stat.h>
#include <sys/types.h>
#include <math.h>
#include<omp.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <limits>
#include <mutex>

#include <Eigen/Core>
#include <Eigen/LU>
#include <Eigen/Geometry>

#include <ros/ros.h>
#include <ros/console.h>

#include <sensor_msgs/Imu.h>
#include <sensor_msgs/MagneticField.h>
#include <sensor_msgs/NavSatFix.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/PoseArray.h>
#include <nav_msgs/Odometry.h>
#include <geometry_msgs/AccelWithCovarianceStamped.h>
#include <tf/transform_broadcaster.h>
#include <tf2_ros/static_transform_broadcaster.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include "../../../../../devel/include/autoware_msgs/DetectedObject.h"


#include <gnss/geo_pos_conv.hpp>

#include <ParticleFilter.h>
#include <State.h>

#define USE_IMU false
#define USE_MAGNETIC false
#define USE_GPS false
#define USE_GNSSRAW_FIX false
#define USE_GNSSRAW_ACCELERATION false
#define USE_VEHICLE_PERCEPTION false

#define MODE_INITIAL_STATE_SETTING "phone_sensors"

#define PHONE_NAME "android"

//#define PHONE_INITIAL_FIX "37.9180604745,-122.331348873,0.0"
//#define PHONE_INITIAL_DERECTION "-0.580381653" // radian from north, opposite clock direction
//#define PHONE_INITIAL_QUATERNION "0.05702500453,0.253443002701,0.902796983719,0.342742472887"

#define ACCELERATION_GROUND_BIAS "-0.03,0.01,0.0"
#define ACCELERATION_X_BIAS -0.03
#define ACCELERATION_Y_BIAS 0.01

#define GYRO_Z_BIAS -0.00
//#define PARTICLEFILTER_INITIAL_XY_RADIUS 2.0      // [m]
//#define PARTICLEFILTER_INITIAL_R_RADIUS 180.0      // [deg]
//#define PARTICLEFITLER_INITIAL_V_RADIUS 0.01    // [m/s]

#define PARTICLEFILTER_PARTICLE_SIZE 200
#define PARTICLEFILTER_SYSTEM_NOISE_STANDARD_DEVIATION {0.05,0.05,0.087263889,0.1,0.00001}  // x,y,r,vx,vy

#define PARTICLEFILTER_ACCELERATION_STANDARD_DEVIATION 0.17    //
#define PARTICLEFILTER_GYRO_STANDARD_DEVIATION 0.01            // [rad/s]
#define PARTICLEFILTER_TRANSITION_V_STANDARD_DEVIATION 0.1            // [m/s]
#define PARTICLEFILTER_IMU_YAW_STANDARD_DEVIATION 0.01*M_PI/180             // [rad]
#define PARTICLEFILTER_MAGNETIC_YAW_STANDARD_DEVIATION 5*M_PI/180 //

#define PARTICLEFILTER_GPS_XY_BIAS 5.0                      //
#define PARTICLEFILTER_GPS_V_BIAS 0.0                      //
#define PARTICLEFILTER_GPS_XY_STANDARD_DEVIATION 0.5                      //
#define PARTICLEFILTER_GPS_V_STANDARD_DEVIATION 0.2                      //
#define PARTICLEFILTER_GPS_R_STANDARD_DEVIATION 5*M_PI/180                      //

#define PARTICLEFILTER_GNSSRAW_XY_BIAS 3.0                      //
#define PARTICLEFILTER_GNSSRAW_XY_STANDARD_DEVIATION 0.5                      //
#define PARTICLEFILTER_GNSSRAW_V_STANDARD_DEVIATION 0.1                      //
#define PARTICLEFILTER_GNSSRAW_R_STANDARD_DEVIATION 5*M_PI/180                      //


#define PARTICLEFILTER_VEHICLE_PERCEPTION_XY_STANDARD_DEVIATION 0.3                      //
#define PARTICLEFILTER_VEHICLE_PERCEPTION_XY_NIS_TH 0.7                      //
#define PARTICLEFILTER_VEHICLE_PERCEPTION_V_STANDARD_DEVIATION 0.01                      //
#define PARTICLEFILTER_VEHICLE_PERCEPTION_R_STANDARD_DEVIATION 2.0*M_PI/180                      //

#define TOPIC_NAME_INIT_FIX "odom"
#define TOPIC_NAME_SUB_IMU "imu"
#define TOPIC_NAME_SUB_MAGNETIC "magnetic_field"
#define TOPIC_NAME_SUB_GPS "fix"
#define TOPIC_NAME_SUB_GNSSRAW_FIX "gnss/fixENU"
#define TOPIC_NAME_SUB_GNSSRAW_ACCELERATION "gnss/accelENU"
#define TOPIC_NAME_SUB_OBJECTS "pedestrian2phone"
#define TOPIC_NAME_PUB_ODOM "odom"
#define TOPIC_NAME_PUB_POSEARRAY "particles"


class PhoneLocalizer : public ParticleFilter<State<>> {

public:

    PhoneLocalizer(ros::NodeHandle *nh, ros::NodeHandle *private_nh);

    ~PhoneLocalizer();

    void callback_imu(const sensor_msgs::Imu::ConstPtr &msg);

    void callback_magnetic(const sensor_msgs::MagneticField::ConstPtr &msg);

    void callback_gps(const sensor_msgs::NavSatFix::ConstPtr &msg);

    void callback_gnssraw_fix(const sensor_msgs::NavSatFix::ConstPtr &msg);

    void callback_gnssraw_acceleration(const geometry_msgs::AccelWithCovarianceStamped::ConstPtr &msg);

    void callback_vehicle_perception(const geometry_msgs::PoseWithCovarianceStamped::ConstPtr &msg);

    void publish_odom(const ros::Time &time);

    void publish_poseaary(const ros::Time &time);

    void run();

    bool particleInitialization();

    void particleTransition();

    void particleLikelihoodIMU();

    void particleLikelihoodMagnetic();

    void particleLikelihoodGpsXY();

    void particleLikelihoodGpsV();

    void particleLikelihoodGpsR();

    void particleLikelihoodGNSSRawXY();

    void particleLikelihoodGNSSRawV();

    void particleLikelihoodGNSSRawR();

    void particleLikelihoodVehiclePerceptionXY();

    void particleLikelihoodVehiclePerceptionV();

    void particleLikelihoodVehiclePerceptionR();

    void imu2rpy(const sensor_msgs::Imu &imu, double *rpy);

    void magnetic2yaw(const sensor_msgs::MagneticField &magnetic, double *yaw);

    void rpy2matrix(double roll, double pitch, double yaw, Eigen::Matrix3f *rotation_matrix);

    void quaternion2matrix(double qx, double qy, double qz, double qw, Eigen::Matrix3f *rotation_matrix);

    void object2state(const autoware_msgs::DetectedObject &object, State<> *state);

    bool isOrientationActive(const geometry_msgs::Quaternion& q);

    /** Thread lock **/
    std::mutex lock;

private:

    std::string phone_name;

    /** ROS **/
    ros::NodeHandle &nh;
    ros::NodeHandle &private_nh;
    ros::Publisher pub_odom, pub_particles;
    ros::Subscriber sub_imu, sub_magnetic, sub_gps, sub_gnssraw_fix, sub_gnssraw_acceleration, sub_vehicle_perception;

    /** Topic **/
    //// msgs
    sensor_msgs::Imu imu;
    sensor_msgs::MagneticField magnetic;
    sensor_msgs::NavSatFix gps;
    sensor_msgs::NavSatFix gnssraw_fix;
    geometry_msgs::AccelWithCovarianceStamped gnssraw_acceleration;

    geometry_msgs::PoseWithCovarianceStamped vehicle_perception;

    //// topic names
    std::string topic_name_sub_imu, topic_name_sub_magnetic, topic_name_sub_gps, topic_name_sub_gnssraw_fix, topic_name_sub_gnssraw_acceleration, topic_name_sub_vehicle_perception;
    std::string topic_name_pub_odom, topic_name_pub_particles;

    //// Frame ID
    std::string origin_frame_id, phone_frame_id, phone_origin_frame_id;

    //// Others
    ros::Time time;
    bool use_imu, use_mag, use_gps, use_gps_xy, use_gps_v, use_gps_r, use_gnssraw_fix,
         use_gnssraw_acceleration, use_gnssraw_acceleration_v, use_gnssraw_acceleration_r,
         use_vehicle_perception, use_vehicle_perception_xy, use_vehicle_perception_vr;
    bool update_imu, update_magnetic, update_gps, update_gnssraw_fix, update_gnssraw_acceleration,
         update_vehicle_perception, update_vehicle_perception_xy, update_vehicle_perception_vr;

    /** TF Listener **/
    tf2_ros::Buffer tf_buffer;

    /** Current Status **/
    nav_msgs::Odometry odom;
    geometry_msgs::PoseArray posearray;
    Eigen::Matrix3f phone2horizontal, phone2ground;

    /** GPS fix -> pose **/
    geo_pos_conv geo;
    std::vector<double> origin_fix;
//    double origin_lat, origin_lon, origin_ele;


    /** Particle Filter Tracker **/
    State<> phone_initial_state;
    int particle_size;
    State<> now_phone_state;
    State<> vehicle_perception_state;

    /** Random Seed **/
    std::random_device rnd;     // 非決定的な乱数生成器を生成
    std::mt19937_64 mt;     //  メルセンヌ・ツイスタの64ビット版、引数は初期シード値

    /** MUTEX **/
    std::mutex mtx;

    /** File Output **/
    std::ofstream ofs_state,ofs_particle,ofs_gps,ofs_imu,ofs_gnssraw_accel,ofs_weight,ofs_resampling;

};


#endif //SRC_PHONE_LOCALIZER_H
