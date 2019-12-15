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

#define TOPIC_NAME_ODOM "navsat/odom"
#define TOPIC_NAME_FIX "navsat/fix"
#define TOPIC_NAME_INIT_FIX "/prius/origin_fix"
#define FRAME_NAME_PRIUS_ORIGIN "/prius/my_frame"
#define FRAME_NAME_ODOM "odom"

class PriusSetup{

public:

    PriusSetup();
    ~PriusSetup();
    void callback_odom(const nav_msgs::Odometry::ConstPtr &msg);
    void callback_fix(const sensor_msgs::NavSatFix::ConstPtr &msg);

private:

    ros::NodeHandle nh;
    ros::NodeHandle private_nh;
    ros::Publisher pub_origin_fix;
    ros::Subscriber sub_odom,sub_fix;

    nav_msgs::Odometry msg_odom;
    sensor_msgs::NavSatFix msg_fix;
    bool update_odom,update_fix;

};

PriusSetup::PriusSetup() : nh(), private_nh(ros::NodeHandle("~"))
{

    ROS_DEBUG("%s",__FUNCTION__);

    /** Set subscriber **/
    sub_odom=nh.subscribe(TOPIC_NAME_ODOM,1,&PriusSetup::callback_odom,this);
    sub_fix=nh.subscribe(TOPIC_NAME_FIX,1,&PriusSetup::callback_fix,this);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    ros::spinOnce();

    /** Set publisher **/
    static tf2_ros::StaticTransformBroadcaster static_broadcaster;
    geometry_msgs::TransformStamped static_transformStamped;

    std::string from=FRAME_NAME_PRIUS_ORIGIN;
    std::string to=FRAME_NAME_ODOM;

    static_transformStamped.header.stamp = ros::Time::now();
    static_transformStamped.header.frame_id = from;
    static_transformStamped.child_frame_id = to;
    static_transformStamped.transform.translation.x = -msg_odom.pose.pose.position.x;
    static_transformStamped.transform.translation.y = -msg_odom.pose.pose.position.y;
    static_transformStamped.transform.translation.z = -msg_odom.pose.pose.position.z;
    static_transformStamped.transform.rotation.x = -msg_odom.pose.pose.orientation.x;
    static_transformStamped.transform.rotation.y = -msg_odom.pose.pose.orientation.x;
    static_transformStamped.transform.rotation.z = -msg_odom.pose.pose.orientation.x;
    static_transformStamped.transform.rotation.w = msg_odom.pose.pose.orientation.x;
    static_broadcaster.sendTransform(static_transformStamped);
    ROS_INFO("Spinning until killed publishing %s to %s", from.c_str(),to.c_str());

    pub_origin_fix=nh.advertise<sensor_msgs::NavSatFix>(TOPIC_NAME_INIT_FIX,1,true);
    pub_origin_fix.publish(msg_fix);
    ros::spin();
}

PriusSetup::~PriusSetup() {}

/**
 * callback_odom
 * @param msg
 */
void PriusSetup::callback_odom(const nav_msgs::Odometry::ConstPtr & msg) {

    ROS_DEBUG("%s",__FUNCTION__);
    msg_odom=*msg;

}

/**
 * callback_fix
 * @param msg
 */
void PriusSetup::callback_fix(const sensor_msgs::NavSatFix::ConstPtr & msg) {

    ROS_DEBUG("%s",__FUNCTION__);
    msg_fix=*msg;

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
