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

#define ORIGIN_FIX "37.9181551873,-122.331354939,0.0"
#define PERSON_INITIAL_DERECTION "-0.580381653" // radian from north, opposite clock direction

#define ORIGIN_FRAME_ID "odom"
#define PHONE_FRAME_ID "imu"
#define PERSON_FRAME_ID "person_sensor_raw"

#define TOPIC_NAME_SUB_IMU "/android/imu"
#define TOPIC_NAME_MAGNETIC "/android/magnetic_field"
#define TOPIC_NAME_GPS "/android/fix"
#define TOPIC_NAME_PUBLISH_ODOM "/phonesensors_raw/odom"
#define TOPIC_NAME_PUBLISH_ACCCEL "/phonesensors_raw/accelaration"


class Localizer{

public:

    Localizer();

    ~Localizer();

    void callback_imu(const sensor_msgs::Imu::ConstPtr &msg);
    void callback_magnetic(const sensor_msgs::MagneticField::ConstPtr &msg);
    void callback_gps(const sensor_msgs::NavSatFix::ConstPtr &msg);

    void run();

    void imu2rpy(const sensor_msgs::Imu &imu, double *rpy);
    void magnetic2yaw(const sensor_msgs::MagneticField& magnetic, double* rpy);
    void rpy2matrix(double roll, double pitch, double yaw, Eigen::Matrix3f *rotation_matrix);

private:

    /** ROS **/
    ros::NodeHandle nh;
    ros::NodeHandle private_nh;
    ros::Publisher pub_odom;
//    ros::Publisher pub_accel;
    ros::Subscriber sub_imu,sub_magnetic,sub_gps;
    tf::TransformListener tf_listener;

    /** Topic **/
    sensor_msgs::Imu imu;
    sensor_msgs::MagneticField magnetic;
    sensor_msgs::NavSatFix gps;
    //// Frame ID
    std::string origin_frame_id,phone_frame_id,person_frame_id;
    //// Others
    ros::Time time;
    bool update_imu, update_magnetic, update_gps;

    /** Current Status **/
    nav_msgs::Odometry odom;
//    geometry_msgs::PoseStamped pose;
//    visualization_msgs::Marker accel;

    /** GPS fix -> pose **/
    geo_pos_conv geo;
    double origin_lat,origin_lon,origin_ele;


};

Localizer::Localizer() : nh(), private_nh(ros::NodeHandle("~")),
                         update_imu(false), update_magnetic(false),update_gps(false){

    ROS_DEBUG("%s", __FUNCTION__);

    /** Set parameter **/

    //// origin
    std::string str;
    if (!nh.getParam("origin_fix",str)){
        str=ORIGIN_FIX;
    }
    sscanf(str.c_str(),"%lf,%lf,%lf",&origin_lat,&origin_lon,origin_ele);
    geo.set_plane(origin_lat,origin_lon);

    /** Set publisher **/
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

    /** Set subscriber **/
    sub_imu = nh.subscribe(TOPIC_NAME_SUB_IMU, 1, &Localizer::callback_imu, this);
    sub_magnetic = nh.subscribe(TOPIC_NAME_MAGNETIC, 1, &Localizer::callback_magnetic, this);
    sub_gps=nh.subscribe(TOPIC_NAME_GPS, 1, &Localizer::callback_gps,this);

    ros::spin();
}

Localizer::~Localizer() {}

void Localizer::callback_imu(const sensor_msgs::Imu::ConstPtr &msg) {

    ROS_DEBUG("%s", __FUNCTION__);

    imu = *msg;
    time=msg->header.stamp;
    update_imu = true;
    run();
}

void Localizer::callback_magnetic(const sensor_msgs::MagneticField::ConstPtr &msg) {

    ROS_DEBUG("%s", __FUNCTION__);

    magnetic = *msg;
    time=msg->header.stamp;
    update_magnetic = true;
    run();
}

void Localizer::callback_gps(const sensor_msgs::NavSatFix::ConstPtr &msg) {

    ROS_DEBUG("%s", __FUNCTION__);

    gps = *msg;
    time = msg->header.stamp;
    update_gps = true;
    run();
}


void Localizer::run(){

    ROS_DEBUG("%s", __FUNCTION__);

    if (update_imu && update_magnetic) {

        double rpy[3];
        imu2rpy(imu,rpy);
//        static double imu_initial_yaw=rpy[2];
        magnetic2yaw(magnetic,rpy);



        double person_initial_direction=rpy[2];
        double person_yaw=person_initial_direction;
//        double person_yaw=person_initial_direction+(rpy[2]-imu_initial_yaw);

        tf::Quaternion quat2=tf::createQuaternionFromRPY(0.0,0.0,person_yaw);
//        quaternionTFToMsg(quat2,pose.pose.orientation);
        quaternionTFToMsg(quat2,odom.pose.pose.orientation);





//        geometry_msgs::QuaternionStamped quat_imu;
//        quat_imu.header=imu.header;
//        quat_imu.quaternion.x=imu.orientation.x;
//        quat_imu.quaternion.y=imu.orientation.y;
//        quat_imu.quaternion.z=imu.orientation.z;
//        quat_imu.quaternion.w=imu.orientation.w;
//        geometry_msgs::QuaternionStamped quat_pose;
//        tf_listener.transformQuaternion(imu.header.frame_id,imu.header.stamp,quat_imu,person_frame_id,quat_pose);
//        pose.pose.orientation=quat_pose.quaternion;
//        geometry_msgs::PoseStamped quat_imu;
//        quat_imu.header=imu.header;
//        quat_imu.pose.orientation.x=imu.orientation.x;
//        quat_imu.pose.orientation.y=imu.orientation.y;
//        quat_imu.pose.orientation.z=imu.orientation.z;
//        quat_imu.pose.orientation.w=imu.orientation.w;
//        geometry_msgs::PoseStamped quat_pose;
//        quat_pose.header.frame_id=pose.header.frame_id;
//        quat_pose.header.stamp=imu.header.stamp;
//        tf_listener.transformPose(imu.header.frame_id,ros::Time::now(),quat_imu,person_frame_id,quat_pose);
//        std::cout<<quat_imu<<std::endl;
//        std::cout<<quat_pose<<std::endl;
//        pose.pose.orientation=quat_pose.pose.orientation;
//        update_imu=false;
//
        update_imu=false;
        update_magnetic=false;
    }

    if (update_gps){
        geo.llh_to_xyz(gps.latitude,gps.longitude, gps.altitude);
//        pose.pose.position.x=geo.x();
//        pose.pose.position.y=geo.y();
//        pose.pose.position.z=0;
        odom.pose.pose.position.x=geo.y();
        odom.pose.pose.position.y=-geo.x();
        odom.pose.pose.position.z=0;
        update_gps=false;
    }

    odom.header.frame_id=origin_frame_id;
    odom.header.stamp=time;
    pub_odom.publish(odom);

//    accel.header.stamp=time;
//    pub_accel.publish_objects(accel);
//    ROS_INFO_STREAM(odom);

}

void Localizer::imu2rpy(const sensor_msgs::Imu &imu, double *rpy) {

    tf::Quaternion quat(imu.orientation.x, imu.orientation.y, imu.orientation.z, imu.orientation.w);
    tf::Matrix3x3(quat).getRPY(*rpy, *(rpy+1), *(rpy+2));
    ROS_DEBUG_STREAM(*rpy<<","<<*(rpy+1)<<","<<*(rpy+2));

}

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

/**
 *  Main Function
 **/
int main(int argc, char **argv) {

//    // debug mode
//    if (ros::console::set_logger_level(ROSCONSOLE_DEFAULT_NAME, ros::console::levels::Debug)) {
//        ros::console::notifyLoggerLevelsChanged();
//    }

    ros::init(argc, argv, "phonesensors_raw");
    Localizer node;
    ros::spin();

    return (0);
}