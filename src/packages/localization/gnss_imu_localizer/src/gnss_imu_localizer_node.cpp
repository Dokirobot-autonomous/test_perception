//
// Created by ohashi on 19/09/13.
//

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

#include <Eigen/Core>

#include <ros/ros.h>
#include <ros/console.h>

#include <sensor_msgs/Imu.h>
#include <geometry_msgs/PoseStamped.h>
#include <sensor_msgs/NavSatFix.h>
#include <tf2_ros/transform_broadcaster.h>
#include <gnss/geo_pos_conv.hpp>
#include <nav_msgs/Odometry.h>
#include <geometry_msgs/QuaternionStamped.h>

#define TOPIC_NAME_SUB_IMU "imu_raw"
#define TOPIC_NAME_SUB_ODOM "navsat/odom"
#define TOPIC_NAME_SUB_FIX "navsat/fix"
#define QUATERNION_SENSOR "imu"
#define TOPIC_NAME_PUB_ODOM "odom"
#define TOPIC_NAME_PUB_INIT_FIX "init/fix"
#define TOPIC_NAME_PUB_INIT_QUAT "init/quat"

#define FRAME_ID_WORLD "odom"
#define FRAME_ID_VEHICLE "base_link"

class GnssImuLocalizer{

public:

    GnssImuLocalizer();
    ~GnssImuLocalizer();

    void callback_imu(const sensor_msgs::Imu::ConstPtr &msg);
    void callback_odom(const nav_msgs::Odometry::ConstPtr &msg);
    void callback_fix(const sensor_msgs::NavSatFix::ConstPtr &msg);

    void run();

    double llh2m(double lat1, double lon1, double lat2,double lon2);

private:

    ros::NodeHandle nh;
    ros::NodeHandle private_nh;
    ros::Publisher pub_init_fix,pub_init_quat,pub_odom;
    ros::Subscriber sub_imu,sub_odom,sub_fix;
    ros::Time current;

    tf2_ros::TransformBroadcaster br;

    sensor_msgs::Imu msg_imu;
    nav_msgs::Odometry msg_quat_odom;
    sensor_msgs::NavSatFix msg_fix;

    nav_msgs::Odometry msg_odom;

    std::string frame_id_world,frame_id_vehicle;

    geo_pos_conv geo;

    double init_xyz[3];

    std::string quaternion_sensor;

    bool update_imu,update_odom,update_fix;

};

GnssImuLocalizer::GnssImuLocalizer() : nh(), private_nh(ros::NodeHandle("~")),
                                        update_imu(false),update_odom(false),update_fix(false)
{

    ROS_DEBUG("%s",__FUNCTION__);

    /** Set parameter **/
    private_nh.param<std::string>("frame_id_world",frame_id_world,FRAME_ID_WORLD);
    private_nh.param<std::string>("frame_id_vehicle",frame_id_vehicle,FRAME_ID_VEHICLE);
    private_nh.param<std::string>("quaternion_sensor",quaternion_sensor,QUATERNION_SENSOR);

    /** Set publisher **/
    pub_init_fix=nh.advertise<sensor_msgs::NavSatFix>(TOPIC_NAME_PUB_INIT_FIX, 1, true);
    pub_init_quat=nh.advertise<geometry_msgs::QuaternionStamped>(TOPIC_NAME_PUB_INIT_QUAT, 1, true);
    pub_odom=nh.advertise<nav_msgs::Odometry>(TOPIC_NAME_PUB_ODOM, 10);

    /** Set subscriber **/
    if(quaternion_sensor=="imu"){
        sub_imu=nh.subscribe(TOPIC_NAME_SUB_IMU, 10, &GnssImuLocalizer::callback_imu, this);
    }
    else if(quaternion_sensor=="odom"){
        sub_odom=nh.subscribe(TOPIC_NAME_SUB_ODOM, 10, &GnssImuLocalizer::callback_odom, this);
    }
    sub_fix=nh.subscribe(TOPIC_NAME_SUB_FIX,10,&GnssImuLocalizer::callback_fix,this);

    ros::spin();
}

GnssImuLocalizer::~GnssImuLocalizer() {}

/**
 * callback_imu
 * @param msg
 */
void GnssImuLocalizer::callback_imu(const sensor_msgs::Imu::ConstPtr &msg) {

    msg_imu=*msg;
    ROS_DEBUG_STREAM("["<<std::string(__FUNCTION__)<<"]\n"<<msg_imu);
    current=msg_imu.header.stamp;
    update_imu=true;
    run();

}

/**
 * callback_odom
 * @param msg
 */
void GnssImuLocalizer::callback_odom(const nav_msgs::Odometry::ConstPtr &msg) {

    msg_quat_odom=*msg;
    ROS_DEBUG_STREAM("[" << std::string(__FUNCTION__) << "]\n" << msg_quat_odom);
    current=msg_quat_odom.header.stamp;
    update_odom=true;
    run();

}

/**
 *
 * @param msg
 */
void GnssImuLocalizer::callback_fix(const sensor_msgs::NavSatFix::ConstPtr &msg) {

    msg_fix=*msg;
    ROS_DEBUG_STREAM("["<<std::string(__FUNCTION__)<<"]\n"<<msg_fix);
    current=msg_fix.header.stamp;
    update_fix=true;
    run();

}

/**
 *
 */
void GnssImuLocalizer::run() {

    ROS_DEBUG_STREAM("["<<std::string(__FUNCTION__)<<"]\n");

    static bool init=true;
    if(init){
        if(update_fix && update_imu){

            geo.set_plane(msg_fix.latitude,msg_fix.longitude);
            pub_init_fix.publish(msg_fix);

            geometry_msgs::QuaternionStamped msg_init_quat;
            msg_init_quat.header=msg_imu.header;
            msg_init_quat.quaternion=msg_imu.orientation;
            pub_init_quat.publish(msg_init_quat);

            init=false;
        }
        else if(update_fix && update_odom){

            geo.set_plane(msg_fix.latitude,msg_fix.longitude);
            pub_init_fix.publish(msg_fix);

            geometry_msgs::QuaternionStamped msg_init_quat;
            msg_init_quat.header=msg_quat_odom.header;
            msg_init_quat.quaternion=msg_quat_odom.pose.pose.orientation;
            pub_init_quat.publish(msg_init_quat);

            init=false;
        }
        else{
            return;
        }
    }

    geo.llh_to_xyz_us(msg_fix.latitude,msg_fix.longitude,msg_fix.altitude);

    static int seq=0;
    msg_odom.header.frame_id=frame_id_world;
    msg_odom.header.stamp=current;
    msg_odom.header.seq=seq;
    seq++;
    msg_odom.child_frame_id=frame_id_vehicle;
    msg_odom.pose.pose.position.x=geo.y();
    msg_odom.pose.pose.position.y=-geo.x();
    msg_odom.pose.pose.position.z=geo.z();
    if(update_imu){
        msg_odom.pose.pose.orientation=msg_imu.orientation;
    }
    else if(update_odom){
        msg_odom.pose.pose.orientation=msg_quat_odom.pose.pose.orientation;
    }
    pub_odom.publish(msg_odom);

    geometry_msgs::TransformStamped tf;
    tf.header=msg_odom.header;
    tf.child_frame_id=msg_odom.child_frame_id;
    tf.transform.translation.x=msg_odom.pose.pose.position.x;
    tf.transform.translation.y=msg_odom.pose.pose.position.y;
    tf.transform.translation.z=msg_odom.pose.pose.position.z;
    tf.transform.rotation.x=msg_odom.pose.pose.orientation.x;
    tf.transform.rotation.y=msg_odom.pose.pose.orientation.y;
    tf.transform.rotation.z=msg_odom.pose.pose.orientation.z;
    tf.transform.rotation.w=msg_odom.pose.pose.orientation.w;
    br.sendTransform(tf);

    update_fix=false;
    update_odom=false;
    update_imu=false;


}

/**
 * llh2m
 * @param lat
 * @param lon
 * @param ele
 */
double GnssImuLocalizer::llh2m(double lat1, double lon1, double lat2,double lon2) {

    ROS_DEBUG("%s", __FUNCTION__);

    double R = 6378.137; // Radius of earth in KM
    double dLat = lat2 * M_PI / 180 - lat1 * M_PI / 180;
    double dLon = lon2 * M_PI / 180 - lon1 * M_PI / 180;
    double a = std::sin(dLat/2) * std::sin(dLat/2) +
               std::cos(lat1 * M_PI / 180) * std::cos(lat2 * M_PI / 180) *
               std::sin(dLon/2) * std::sin(dLon/2);
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
    double d = R * c;
    return d * 1000; // meters


}



/**
 * Main Function
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char **argv) {

    // debug mode
    if (ros::console::set_logger_level(ROSCONSOLE_DEFAULT_NAME, ros::console::levels::Debug)) {
        ros::console::notifyLoggerLevelsChanged();
    }

    // Initialize ROS
    ros::init(argc, argv, "gnss_imu_localizer_node");
    GnssImuLocalizer node;
    ros::spin();

    return (0);
}
