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
#include <tf/LinearMath/Quaternion.h>
#include <tf/transform_datatypes.h>
#include <geometry_msgs/TwistStamped.h>
#include <mutex>

#define TOPIC_NAME_SUB_FIX "navsat/fix"
#define TOPIC_NAME_SUB_TWIST "vehicle/twist"
#define TOPIC_NAME_PUB_ODOM "navsat/odom"
#define TOPIC_NAME_PUB_INIT_FIX "init/fix"

#define PUTLISH_TF "true"
#define FRAME_ID_WORLD "odom"
#define FRAME_ID_VEHICLE "base_link"

class GnssLocalizer {

public:

    GnssLocalizer();

    ~GnssLocalizer();

    void callback_fix(const sensor_msgs::NavSatFix::ConstPtr &msg);

    void callback_twist(const geometry_msgs::TwistStamped::ConstPtr &msg);

    void run();

    bool initializationFix();
    bool initializationQuat();

    void publish_odom(ros::Time time);

private:


    ros::NodeHandle nh;
    ros::NodeHandle private_nh;
    ros::Publisher pub_init_fix, pub_odom;
    ros::Subscriber sub_fix, sub_twist;
    ros::Time current;
    std::mutex mtx;

    tf2_ros::TransformBroadcaster br;

    sensor_msgs::NavSatFix msg_fix;
    geometry_msgs::TwistStamped msg_twist;

    geometry_msgs::Pose pose;
    std::vector<double> init_quat;

    bool publish_tf;

    std::string frame_id_world, frame_id_vehicle;

    geo_pos_conv geo_origin, geo_pre;

    bool update_fix, update_twist;

};

GnssLocalizer::GnssLocalizer() : nh(), private_nh(ros::NodeHandle("~")),
                                 update_fix(false), update_twist(false) {

    ROS_DEBUG("%s", __FUNCTION__);

    /** Set parameter **/
    private_nh.param<std::string>("frame_id_world", frame_id_world, FRAME_ID_WORLD);
    private_nh.param<std::string>("frame_id_vehicle", frame_id_vehicle, FRAME_ID_VEHICLE);
    private_nh.param<std::vector<double>>("initital_quaternion",init_quat,{});
    private_nh.param<bool>("publish_tf",publish_tf,PUTLISH_TF);


    /** Set publisher **/
    pub_init_fix = nh.advertise<sensor_msgs::NavSatFix>(TOPIC_NAME_PUB_INIT_FIX, 1, true);
    pub_odom = nh.advertise<nav_msgs::Odometry>(TOPIC_NAME_PUB_ODOM, 10);

    /** Set subscriber **/
    sub_fix = nh.subscribe(TOPIC_NAME_SUB_FIX, 10, &GnssLocalizer::callback_fix, this);
    sub_twist = nh.subscribe(TOPIC_NAME_SUB_TWIST, 10, &GnssLocalizer::callback_twist, this);

    ros::spin();
}

GnssLocalizer::~GnssLocalizer() {}

/**
 * callback_fix
 * @param msg
 */
void GnssLocalizer::callback_fix(const sensor_msgs::NavSatFix::ConstPtr &msg) {

    auto tmp = *msg;
    ROS_DEBUG_STREAM("[" << std::string(__FUNCTION__) << "]\n" << tmp);
    std::lock_guard<std::mutex> lock(mtx);
    msg_fix = tmp;
    current = msg_fix.header.stamp;
    update_fix = true;
    run();

}

/**
 * callback_twist
 * @param msg
 */
void GnssLocalizer::callback_twist(const geometry_msgs::TwistStamped::ConstPtr &msg) {

    auto tmp = *msg;
    ROS_DEBUG_STREAM("[" << std::string(__FUNCTION__) << "]\n" << tmp);
    std::lock_guard<std::mutex> lock(mtx);
    msg_twist = *msg;
    current = msg_twist.header.stamp;
    update_twist = true;
    run();

}

/**
 * run
 */
void GnssLocalizer::run() {

    ROS_DEBUG_STREAM("[" << std::string(__FUNCTION__) << "]\n");

    /** Initialization **/
    static bool init_fix = false, init_quat = false;
    if (!init_fix)
        init_fix = initializationFix();
    if (!init_quat)
        init_quat = initializationQuat();

    if (!(init_fix && init_quat)) {
        ROS_WARN("Not Completed Initialization \ninit_fix: %d, init_quat: %d", init_fix, init_quat);
        return;
    }

    if(update_fix){
        //// set position
        geo_origin.llh_to_xyz_us(msg_fix.latitude, msg_fix.longitude, msg_fix.altitude);
        pose.position.x=geo_origin.y();
        pose.position.y=-geo_origin.x();
        pose.position.z=geo_origin.z();
        //// set orientation
        geo_pre.llh_to_xyz_us(msg_fix.latitude, msg_fix.longitude, msg_fix.altitude);
        if(geo_pre.diff()>0.2){
            double yaw;
            if(msg_twist.twist.linear.x>=0.0)
                yaw=std::atan2(-geo_pre.x(),geo_pre.y());
            else
                yaw=std::atan2(geo_pre.x(),-geo_pre.y());   //// back pattern
            pose.orientation = tf::createQuaternionMsgFromYaw(yaw);
            geo_pre.set_plane(msg_fix.latitude, msg_fix.longitude);
        }
    }

    /** Generate Message **/
    //// header
    nav_msgs::Odometry msg_odom;
    static int seq = 0;
    msg_odom.header.frame_id = frame_id_world;
    msg_odom.header.stamp = current;
    msg_odom.child_frame_id = frame_id_vehicle;
    msg_odom.header.seq = seq;
    seq++;
    //// set pose
    msg_odom.pose.pose=pose;
    //// set twist
    msg_odom.twist.twist=msg_twist.twist;
    pub_odom.publish(msg_odom);

    if(publish_tf){
        geometry_msgs::TransformStamped tf;
        tf.header = msg_odom.header;
        tf.child_frame_id = msg_odom.child_frame_id;
        tf.transform.translation.x = msg_odom.pose.pose.position.x;
        tf.transform.translation.y = msg_odom.pose.pose.position.y;
        tf.transform.translation.z = msg_odom.pose.pose.position.z;
        tf.transform.rotation.x = msg_odom.pose.pose.orientation.x;
        tf.transform.rotation.y = msg_odom.pose.pose.orientation.y;
        tf.transform.rotation.z = msg_odom.pose.pose.orientation.z;
        tf.transform.rotation.w = msg_odom.pose.pose.orientation.w;
        br.sendTransform(tf);
    }

    update_fix = false;
    update_twist=false;


}

/**
 * initializationFix
 */
bool GnssLocalizer::initializationFix() {

    ROS_DEBUG_STREAM("[" << std::string(__FUNCTION__) << "]\n");

    if (!update_fix)
        return false;

    geo_origin.set_plane(msg_fix.latitude, msg_fix.longitude);
    geo_origin.llh_to_xyz_us(msg_fix.latitude, msg_fix.longitude,msg_fix.altitude);
    pose.position.x=geo_origin.y();
    pose.position.y=-geo_origin.x();
    pose.position.z=geo_origin.z();

    pub_init_fix.publish(msg_fix);
    ROS_INFO("Set Initial Fix \nLatitude: %lf \nLongitude: %lf \nAltitude: %lf",
             msg_fix.latitude, msg_fix.longitude, msg_fix.altitude);
    return true;

}

/**
 * initializationQuat
 */
bool GnssLocalizer::initializationQuat() {

    ROS_DEBUG_STREAM("[" << std::string(__FUNCTION__) << "]\n");

    if(init_quat.empty()){
        if (!update_fix)
            return false;

        if(geo_pre.empty_plane()){
            geo_pre.set_plane(msg_fix.latitude,msg_fix.longitude);
        }
        geo_pre.llh_to_xyz_us(msg_fix.latitude, msg_fix.longitude, msg_fix.altitude);

        if(geo_pre.diff()<5.0){
            ROS_WARN("Quaternion has not been initialized yet. Please move ahead about 0.2m");
            return false;
        }

        pose.orientation=tf::createQuaternionMsgFromYaw(std::atan2(-geo_pre.x(),geo_pre.y()));

    }
    else{
        if(init_quat==std::vector<double>(4,0.0)){
            ROS_ERROR("Initial quaternion must not be all 0.0");
            exit(-1);
        }
        pose.orientation.x=init_quat[0];
        pose.orientation.y=init_quat[1];
        pose.orientation.z=init_quat[2];
        pose.orientation.w=init_quat[3];
    }

    geometry_msgs::QuaternionStamped quat;
    quat.header = msg_fix.header;
    quat.quaternion = pose.orientation;
    geo_pre.set_plane(msg_fix.latitude, msg_fix.longitude);
    return true;

}

/**
 * publish_odom
 * @param time
 */
void GnssLocalizer::publish_odom(ros::Time time) {

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
    ros::init(argc, argv, "gnss_localizer_node");
    GnssLocalizer node;
    ros::spin();

    return (0);
}
