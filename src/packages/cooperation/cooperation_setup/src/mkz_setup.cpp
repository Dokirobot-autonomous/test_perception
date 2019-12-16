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

#include "cooperation_setup/geo_pos_conv.hpp"

#define TOPIC_NAME_ODOM "navsat/odom"
#define TOPIC_NAME_FIX "vehicle/gps/fix"
#define TOPIC_NAME_INIT_FIX "origin_fix"
#define FRAME_NAME_PRIUS_ORIGIN "/prius/my_frame"
#define FRAME_NAME_MKZ_ORIGIN "/mkz/my_frame"
#define FRAME_NAME_ODOM "odom"

class MKZSetup{

public:

    MKZSetup();
    ~MKZSetup();

    void callback_fix(const sensor_msgs::NavSatFix::ConstPtr &msg);


private:

    ros::NodeHandle nh;
    ros::NodeHandle private_nh;
    ros::Publisher pub_origin_fix;
    ros::Subscriber sub_fix;

    std::string origin_fix_str;
    geo_pos_conv geo;

    sensor_msgs::NavSatFix msg_fix;

};


MKZSetup::MKZSetup() : nh(), private_nh(ros::NodeHandle("~"))
{

    ROS_DEBUG("%s",__FUNCTION__);

    /** Set parameter **/
    if(!nh.getParam("origin_fix",origin_fix_str)){
        ROS_ERROR("Non \"origin_fix\"");
        exit(-1);
    }
    double origin_lat,origin_lon,origin_ele;
    sscanf(origin_fix_str.c_str(), "%lf,%lf,%lf", &origin_lat, &origin_lon, &origin_ele);
    geo.set_plane(origin_lat, origin_lon);

    sub_fix=nh.subscribe(TOPIC_NAME_FIX,1,&MKZSetup::callback_fix,this);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    ros::spinOnce();

    //// Set
    geo.llh_to_xyz(msg_fix.latitude, msg_fix.longitude, msg_fix.altitude);

    /** Set publisher **/
    static tf2_ros::StaticTransformBroadcaster static_broadcaster;
    geometry_msgs::TransformStamped static_transformStamped;

    std::string from=FRAME_NAME_PRIUS_ORIGIN;
    std::string to=FRAME_NAME_MKZ_ORIGIN;

    static_transformStamped.header.stamp = ros::Time::now();
    static_transformStamped.header.frame_id = from;
    static_transformStamped.child_frame_id = to;
    static_transformStamped.transform.translation.x = geo.y();
    static_transformStamped.transform.translation.y = -geo.x();
    static_transformStamped.transform.translation.z = geo.z();
    static_transformStamped.transform.rotation.x = 0.0;
    static_transformStamped.transform.rotation.y = 0.0;
    static_transformStamped.transform.rotation.z = 0.0;
    static_transformStamped.transform.rotation.w = 1.0;
    static_broadcaster.sendTransform(static_transformStamped);
    ROS_INFO("Spinning until killed publishing %s to %s", from.c_str(),to.c_str());

    pub_origin_fix=nh.advertise<sensor_msgs::NavSatFix>(TOPIC_NAME_INIT_FIX,1,true);
    pub_origin_fix.publish(msg_fix);

    ros::spin();
}

/**
 * ~MKZSetup
 */
MKZSetup::~MKZSetup() {}

/**
 * callback_fix
 * @param msg
 */
void MKZSetup::callback_fix(const sensor_msgs::NavSatFix::ConstPtr & msg) {

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
    ros::init(argc, argv, "mkz_setup");
    MKZSetup node;
    ros::spin();

    return (0);
}
