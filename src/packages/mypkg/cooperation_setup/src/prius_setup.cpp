//
// Created by ohashi on 19/09/13.
//

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>


#include <ros/ros.h>
#include <ros/console.h>

#include <nav_msgs/Odometry.h>
#include <sensor_msgs/NavSatFix.h>
#include <tf2_ros/static_transform_broadcaster.h>
#include <geometry_msgs/TransformStamped.h>
#include <geometry_msgs/QuaternionStamped.h>
#include <std_msgs/String.h>
#include <tf/LinearMath/Quaternion.h>

#define TOPIC_NAME_ODOM "navsat/odom"
#define TOPIC_NAME_FIX "navsat/fix"
#define TOPIC_NAME_INIT_FIX "init/fix"
#define TOPIC_NAME_INIT_QUAT "init/quat"
#define FRAME_NAME_PRIUS_ORIGIN "prius/my_frame"
#define FRAME_NAME_ODOM "odom"

class PriusSetup {

public:

    PriusSetup();

    ~PriusSetup();

    void callback_odom(const nav_msgs::Odometry::ConstPtr &msg);
    void callback_fix(const sensor_msgs::NavSatFix::ConstPtr &msg);

private:

    ros::NodeHandle nh;
    ros::NodeHandle private_nh;
    ros::Publisher pub_init_fix,pub_init_quat;

    nav_msgs::Odometry msg_odom;
    geometry_msgs::QuaternionStamped msg_quat;
    sensor_msgs::NavSatFix msg_fix;
    bool update_odom, update_fix;

};

PriusSetup::PriusSetup() : nh(), private_nh(ros::NodeHandle("~")),
                           update_odom(false), update_fix(false) {

    ROS_DEBUG("%s", __FUNCTION__);

    /** Set subscriber **/
    {
        ros::Subscriber sub_odom = nh.subscribe(TOPIC_NAME_ODOM, 1, &PriusSetup::callback_odom, this);
        ros::Subscriber sub_fix = nh.subscribe(TOPIC_NAME_FIX, 1, &PriusSetup::callback_fix, this);
        ros::Rate loop_rate(10);
        while (ros::ok()) {
            ros::spinOnce();
            if (update_odom && update_fix)
                break;
            loop_rate.sleep();
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    ros::spinOnce();

    /** Set publisher **/
    static tf2_ros::StaticTransformBroadcaster static_broadcaster;
    geometry_msgs::TransformStamped static_transformStamped;

    std::string from = FRAME_NAME_PRIUS_ORIGIN;
    std::string to = FRAME_NAME_ODOM;

    static_transformStamped.header.stamp = msg_odom.header.stamp;
    static_transformStamped.header.frame_id = from;
    static_transformStamped.child_frame_id = to;
    static_transformStamped.transform.translation.x = -msg_odom.pose.pose.position.x;
    static_transformStamped.transform.translation.y = -msg_odom.pose.pose.position.y;
    static_transformStamped.transform.translation.z = -msg_odom.pose.pose.position.z;
/*
    static_transformStamped.transform.rotation.x = msg_odom.pose.pose.orientation.x;
    static_transformStamped.transform.rotation.y = msg_odom.pose.pose.orientation.y;
    static_transformStamped.transform.rotation.z = msg_odom.pose.pose.orientation.z;
    static_transformStamped.transform.rotation.w = msg_odom.pose.pose.orientation.w;
*/
    static_transformStamped.transform.rotation.x = 0.0;
    static_transformStamped.transform.rotation.y = 0.0;
    static_transformStamped.transform.rotation.z = 0.0;
    static_transformStamped.transform.rotation.w = 1.0;
    static_broadcaster.sendTransform(static_transformStamped);
    ROS_INFO("Spinning until killed publishing %s to %s", from.c_str(), to.c_str());

    pub_init_fix = nh.advertise<sensor_msgs::NavSatFix>(TOPIC_NAME_INIT_FIX, 1, true);
    msg_fix.header.frame_id=FRAME_NAME_PRIUS_ORIGIN;
    pub_init_fix.publish(msg_fix);

    pub_init_quat=nh.advertise<geometry_msgs::QuaternionStamped>(TOPIC_NAME_INIT_QUAT,1,true);
    msg_quat.header=msg_odom.header;
    msg_quat.header.frame_id=FRAME_NAME_PRIUS_ORIGIN;
    msg_quat.quaternion=msg_odom.pose.pose.orientation;
    pub_init_quat.publish(msg_quat);


    ros::spin();
}

PriusSetup::~PriusSetup() {}

/**
 * callback_odom
 * @param msg
 */
void PriusSetup::callback_odom(const nav_msgs::Odometry::ConstPtr &msg) {

    ROS_DEBUG("%s", __FUNCTION__);
    msg_odom = *msg;
    update_odom = true;

}

/**
 * callback_fix
 * @param msg
 */
void PriusSetup::callback_fix(const sensor_msgs::NavSatFix::ConstPtr &msg) {

    ROS_DEBUG("%s", __FUNCTION__);
    msg_fix = *msg;
    update_fix = true;

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
    ros::init(argc, argv, "prius_setup");
    PriusSetup node;
    ros::spin();

    return (0);
}
