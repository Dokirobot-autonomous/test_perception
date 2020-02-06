//
// Created by ohashi on 19/09/13.
//

#include <math.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <random>

#include <Eigen/Core>
#include <Eigen/LU>
#include <Eigen/Geometry>

#include <ros/ros.h>
#include <ros/console.h>

#include <sensor_msgs/Imu.h>
#include <sensor_msgs/MagneticField.h>
#include <sensor_msgs/NavSatFix.h>
#include <geometry_msgs/Vector3Stamped.h>
//#include <geometry_msgs/PoseStamped.h>
#include <nav_msgs/Odometry.h>
#include <visualization_msgs/Marker.h>
#include <tf/transform_listener.h>

#include <gnss/geo_pos_conv.hpp>
#include <geometry_msgs/AccelWithCovarianceStamped.h>

#define PHONE_NAME "android"

#define USE_IMU true
#define USE_MAGNETIC false
#define USE_GPS true
#define USE_GNSSRAW_FIX true
#define USE_GNSSRAW_ACCELERATION true

#define TOPIC_NAME_SUB_IMU "imu"
#define TOPIC_NAME_SUB_MAGNETIC "magnetic_field"
#define TOPIC_NAME_SUB_GPS "fix"
#define TOPIC_NAME_SUB_GNSSRAW_FIX "gnss/fix"
#define TOPIC_NAME_SUB_GNSSRAW_ACCELERATION "gnss/accelENU"
#define TOPIC_NAME_PUB_GPS_ODOM "gps_odom"
#define TOPIC_NAME_PUB_GNSSRAW_ODOM "gnssraw_odom"

/*
#define ORIGIN_FIX "37.9181551873,-122.331354939,0.0"
#define PERSON_INITIAL_DERECTION "-0.580381653" // radian from north, opposite clock direction
*/

#define ORIGIN_FRAME_ID "odom"
#define GPS_FRAME_ID "gps"
#define GNSSRAE_FRAME_ID "gnssraw"



class Localizer{

public:

    Localizer(ros::NodeHandle *nh, ros::NodeHandle *private_nh);

    ~Localizer();

    void callback_imu(const sensor_msgs::Imu::ConstPtr &msg);
    void callback_magnetic(const sensor_msgs::MagneticField::ConstPtr &msg);
    void callback_gps(const sensor_msgs::NavSatFix::ConstPtr &msg);
    void callback_gnssraw_fix(const sensor_msgs::NavSatFix::ConstPtr &msg);
    void callback_gnssraw_acceleration(const geometry_msgs::AccelWithCovarianceStamped::ConstPtr &msg);

    void run();

    void imu2rpy(const sensor_msgs::Imu &imu, double *rpy);
    void magnetic2yaw(const sensor_msgs::MagneticField& magnetic, double* rpy);
    void rpy2matrix(double roll, double pitch, double yaw, Eigen::Matrix3f *rotation_matrix);

private:

    std::string phone_name;

    /** ROS **/
    ros::NodeHandle &nh;
    ros::NodeHandle &private_nh;
    ros::Publisher pub_gps_odom,pub_gnssraw_odom;
//    ros::Publisher pub_accel;
    ros::Subscriber sub_imu, sub_magnetic, sub_gps, sub_gnssraw_fix, sub_gnssraw_acceleration;

    /** TF Listener **/
    tf2_ros::Buffer tf_buffer;

    //// Others
    ros::Time time;
    bool use_imu, use_mag, use_gps, use_gnssraw_fix, use_gnssraw_acceleration, use_vehicle_perception;
    bool update_imu, update_magnetic, update_gps, update_gnssraw_fix, update_gnssraw_acceleration;


    /** Topic **/
    sensor_msgs::Imu imu;
    sensor_msgs::MagneticField magnetic;
    sensor_msgs::NavSatFix gps;
    sensor_msgs::NavSatFix gnssraw_fix;
    geometry_msgs::AccelWithCovarianceStamped gnssraw_acceleration;
    //// topic names
    std::string topic_name_sub_imu, topic_name_sub_magnetic, topic_name_sub_gps, topic_name_sub_gnssraw_fix, topic_name_sub_gnssraw_acceleration;
    std::string topic_name_pub_gps_odom,topic_name_pub_gnssraw_odom;
    //// Frame ID
    std::string origin_frame_id,phone_frame_id,person_frame_id;

    /** Current Status **/
    nav_msgs::Odometry gps_odom;
    nav_msgs::Odometry gnssraw_odom;

    /** GPS fix -> pose **/
    geo_pos_conv geo;
    std::vector<double> origin_fix;

};

Localizer::Localizer(ros::NodeHandle *nh, ros::NodeHandle *private_nh) :
        nh(*nh), private_nh(*private_nh),
        update_imu(false), update_magnetic(false),update_gps(false),update_gnssraw_fix(false),update_gnssraw_acceleration(false){

    ROS_DEBUG("%s", __FUNCTION__);

    /** Set parameter **/

    //// phone name
    this->private_nh.param<std::string>("phone_name", phone_name, PHONE_NAME);

    //// origin
    if(!this->private_nh.getParam("origin_fix",origin_fix)){
        ROS_ERROR("No \"origin_fix\"");
        exit(-1);
    }
    geo.set_plane(origin_fix[0], origin_fix[1]);
    ROS_INFO_STREAM("Origin fix: ["<<origin_fix[0]<<","<<origin_fix[1]<<"]");

    //// origin frame_id
    if (!this->private_nh.getParam("origin_frame_id", origin_frame_id)) {
        ROS_ERROR("Non \"origin_frame_id\"");
        exit(-1);
    }
    ROS_INFO("Origin frame_id: %s",origin_frame_id.c_str());

    //// topic names
    topic_name_sub_imu = TOPIC_NAME_SUB_IMU;
    topic_name_sub_magnetic = TOPIC_NAME_SUB_MAGNETIC;
    topic_name_sub_gps = TOPIC_NAME_SUB_GPS;
    topic_name_sub_gnssraw_fix = TOPIC_NAME_SUB_GNSSRAW_FIX;
    topic_name_sub_gnssraw_acceleration = TOPIC_NAME_SUB_GNSSRAW_ACCELERATION;
    topic_name_pub_gps_odom=TOPIC_NAME_PUB_GPS_ODOM;
    topic_name_pub_gnssraw_odom=TOPIC_NAME_PUB_GNSSRAW_ODOM;

    //// TF Listener
    static tf2_ros::TransformListener tf_listener(tf_buffer);

    //// sensors
    this->private_nh.param("use_imu", use_imu, USE_IMU);
    this->private_nh.param("use_magnetic", use_mag, USE_MAGNETIC);
    this->private_nh.param("use_gps", use_gps, USE_GPS);
    this->private_nh.param("use_gnssraw_fix", use_gnssraw_fix, USE_GNSSRAW_FIX);
    this->private_nh.param("use_gnssraw_acceleration", use_gnssraw_acceleration, USE_GNSSRAW_ACCELERATION);

    /** Set publisher **/
    //// Odom
    gps_odom.header.frame_id = origin_frame_id;
    gps_odom.child_frame_id  = phone_name+"/"+GPS_FRAME_ID;
    pub_gps_odom = this->nh.advertise<nav_msgs::Odometry>(topic_name_pub_gps_odom, 10);
    gnssraw_odom.header.frame_id = origin_frame_id;
    gnssraw_odom.child_frame_id = phone_name+"/"+GNSSRAE_FRAME_ID;
    pub_gnssraw_odom = this->nh.advertise<nav_msgs::Odometry>(topic_name_pub_gnssraw_odom, 10);

    /** Set subscriber **/
    if (use_imu) {
        sub_imu = this->nh.subscribe(topic_name_sub_imu, 10, &Localizer::callback_imu, this);
        ROS_INFO("Subscribe IMU Message");
    }
    if (use_mag) {
        sub_magnetic = this->nh.subscribe(topic_name_sub_magnetic, 10, &Localizer::callback_magnetic, this);
        ROS_INFO("Subscribe Magnetic Message");
    }
    if (use_gps) {
        sub_gps = this->nh.subscribe(topic_name_sub_gps, 10, &Localizer::callback_gps, this);
        ROS_INFO("Subscribe GPS Message");
    }
    if (use_gnssraw_fix) {
        sub_gnssraw_fix = this->nh.subscribe(topic_name_sub_gnssraw_fix, 10, &Localizer::callback_gnssraw_fix, this);
        ROS_INFO("Subscribe GNSSRaw Fix Message");
    }
    if (use_gnssraw_acceleration) {
        sub_gnssraw_acceleration = this->nh.subscribe(topic_name_sub_gnssraw_acceleration, 10,
                                                      &Localizer::callback_gnssraw_acceleration, this);
        ROS_INFO("Subscribe GNSSRaw Velocity Message");
    }



/*
    ROS_DEBUG("%s", __FUNCTION__);

    */
/** Set parameter **//*


    //// origin
    std::string str;
    if (!nh.getParam("origin_fix",str)){
        str=ORIGIN_FIX;
    }
    sscanf(str.c_str(),"%lf,%lf,%lf",&origin_lat,&origin_lon,origin_ele);
    geo.set_plane(origin_lat,origin_lon);

    */
/** Set publisher **//*

//    pub_odom = nh.advertise<geometry_msgs::PoseStamped>(TOPIC_NAME_PUBLISH_ODOM, 1);
    pub_odom = nh.advertise<nav_msgs::Odometry>(TOPIC_NAME_PUBLISH_ODOM, 1);
    if (!nh.getParam("origin_frame_id",str)){
        str=ORIGIN_FRAME_ID;
    }
//    pose.header.frame_id=origin_frame_id=str;
    odom.header.frame_id=origin_frame_id=str;
    if (!nh.getParam("phone_frame_id",str)){
        str=PHONE_FRAME_ID;
    }
    phone_frame_id=str;
    if (!nh.getParam("person_frame_id",str)){
        str=PERSON_FRAME_ID;
    }
//    person_frame_id=str;
    odom.child_frame_id=person_frame_id=str;
//    pub_accel=nh.advertise<visualization_msgs::Marker>(TOPIC_NAME_PUBLISH_ACCCEL,1);
//    accel.header.frame_id="phonesensors_raw_person";
//    accel.type=visualization_msgs::Marker::ARROW;
//    accel.pose.position.x=0;
//    accel.pose.position.y=0;
//    accel.pose.position.z=0;

    */
/** Set subscriber **//*

    sub_imu = nh.subscribe(TOPIC_NAME_SUB_IMU, 1, &Localizer::callback_imu, this);
    sub_magnetic = nh.subscribe(TOPIC_NAME_MAGNETIC, 1, &Localizer::callback_magnetic, this);
    sub_gps=nh.subscribe(TOPIC_NAME_GPS, 1, &Localizer::callback_gps,this);

    ros::spin();
*/
}

Localizer::~Localizer() {}

void Localizer::callback_imu(const sensor_msgs::Imu::ConstPtr &msg) {

    imu = *msg;
    time=msg->header.stamp;
    ROS_DEBUG_STREAM("callback_imu: \n" << imu);
    update_imu = true;
    run();
}

void Localizer::callback_magnetic(const sensor_msgs::MagneticField::ConstPtr &msg) {

    magnetic = *msg;
    time=msg->header.stamp;
    ROS_DEBUG_STREAM("callback_magnetic: \n" << magnetic);
    update_magnetic = true;
    run();
}

void Localizer::callback_gps(const sensor_msgs::NavSatFix::ConstPtr &msg) {

    gps = *msg;
    time = msg->header.stamp;
    ROS_DEBUG_STREAM("callback_gps: \n" << gps);
    update_gps = true;
    run();
}

void Localizer::callback_gnssraw_fix(const sensor_msgs::NavSatFix::ConstPtr &msg) {

    gnssraw_fix = *msg;
    time = msg->header.stamp;
    ROS_DEBUG_STREAM("callback_gnssraw_fix: \n" << gnssraw_fix);
    update_gnssraw_fix = true;
    run();

}

void Localizer::callback_gnssraw_acceleration(const geometry_msgs::AccelWithCovarianceStamped::ConstPtr &msg) {

    gnssraw_acceleration = *msg;
    time = msg->header.stamp;
    ROS_DEBUG_STREAM("callback_gnssraw_acceleration: \n" << gnssraw_acceleration);
    update_gnssraw_acceleration = true;
    run();

}


void Localizer::run(){

    static double yaw;
    static bool init_gps_xy=false,init_gnssraw_xy=false,init_yaw=false;

    if (update_imu) {
        if(init_yaw){
            double rpy[3];
            imu2rpy(imu,rpy);
            static double imu_yaw_pre=rpy[2];
            yaw+=(rpy[2]-imu_yaw_pre);
            tf::Quaternion quat=tf::createQuaternionFromRPY(0.0,0.0,yaw);
            quaternionTFToMsg(quat,gps_odom.pose.pose.orientation);
            quaternionTFToMsg(quat,gnssraw_odom.pose.pose.orientation);
            imu_yaw_pre=rpy[2];
            update_imu=false;
        }
    }

    if (update_gps){
        geo.llh_to_xyz_us(gps.latitude,gps.longitude, gps.altitude);
        gps_odom.pose.pose.position.x=geo.y();
        gps_odom.pose.pose.position.y=-geo.x();
        gps_odom.pose.pose.position.z=0;
        init_gps_xy=true;
        update_gps=false;
    }

    if (update_gnssraw_fix){
        geo.llh_to_xyz_us(gnssraw_fix.latitude,gnssraw_fix.longitude, gnssraw_fix.altitude);
        gnssraw_odom.pose.pose.position.x=geo.y();
        gnssraw_odom.pose.pose.position.y=-geo.x();
        gnssraw_odom.pose.pose.position.z=0;
        init_gnssraw_xy=true;
        update_gnssraw_fix=false;
    }

    if(update_gnssraw_acceleration){
        double gnssraw_vx = gnssraw_acceleration.accel.accel.linear.y;
        double gnssraw_vy = -gnssraw_acceleration.accel.accel.linear.x;
        double gnssraw_yaw=std::atan2(gnssraw_vy,gnssraw_vx);
        tf::Quaternion quat=tf::createQuaternionFromRPY(0.0,0.0,gnssraw_yaw);
        quaternionTFToMsg(quat,gps_odom.pose.pose.orientation);
        quaternionTFToMsg(quat,gnssraw_odom.pose.pose.orientation);
        yaw=gnssraw_yaw;
        init_yaw=true;
    }

    static int seq=0;
    gps_odom.header.seq=gnssraw_odom.header.seq=seq;
    seq++;
    gps_odom.header.stamp=gnssraw_odom.header.stamp=time;
    if(init_gps_xy&&init_yaw){
        pub_gps_odom.publish(gps_odom);
        ROS_DEBUG_STREAM("publish_gps_odom: \n" << gps_odom);
    }
    if(init_gnssraw_xy&&init_yaw){
        pub_gnssraw_odom.publish(gnssraw_odom);
        ROS_DEBUG_STREAM("publish_gnssraw_odom: \n" << gnssraw_odom);
    }

}

void Localizer::imu2rpy(const sensor_msgs::Imu &imu, double *rpy) {

    tf::Quaternion quat(imu.orientation.x, imu.orientation.y, imu.orientation.z, imu.orientation.w);
    tf::Matrix3x3(quat).getRPY(*rpy, *(rpy+1), *(rpy+2));

}

/*
void Localizer::magnetic2yaw(const sensor_msgs::MagneticField& magnetic, double* rpy) {

    Eigen::Vector3f mag(magnetic.magnetic_field.x, magnetic.magnetic_field.y, magnetic.magnetic_field.z);
    Eigen::Matrix3f rp_mat;
    rpy2matrix(-rpy[0],-rpy[1],0.0,&rp_mat);
    Eigen::Vector3f mag_horizontal=rp_mat*mag;
    *(rpy+2)=-std::atan2(-mag_horizontal(0),mag_horizontal(1));
    ROS_DEBUG_STREAM(*rpy<<","<<*(rpy+1)<<","<<*(rpy+2));

}

void Localizer::rpy2matrix(double roll, double pitch, double yaw, Eigen::Matrix3f *rotation_matrix) {

    Eigen::Vector3f axis_dr(1, 0, 0), axis_dp(0, 1, 0), axis_dy(0, 0, 1);
    Eigen::Matrix3f matrix_dr, matrix_dp, matrix_dy;
    matrix_dr = Eigen::AngleAxisf(roll, axis_dr);
    matrix_dp = Eigen::AngleAxisf(pitch, axis_dp);
    matrix_dy = Eigen::AngleAxisf(yaw, axis_dy);
    *rotation_matrix=matrix_dy*matrix_dp*matrix_dr;

}
*/

/**
 *  Main Function
 **/
int main(int argc, char **argv) {

    ros::init(argc, argv, "phone_raw");
    ros::NodeHandle nh,private_nh("~");
    //// debug mode
    bool debug;
    private_nh.param<bool>("debug",debug,false);
    if(debug){
        if (ros::console::set_logger_level(ROSCONSOLE_DEFAULT_NAME, ros::console::levels::Debug)) {
            ros::console::notifyLoggerLevelsChanged();
        }
    }
    Localizer node(&nh,&private_nh);
    ros::spin();

    return (0);
}