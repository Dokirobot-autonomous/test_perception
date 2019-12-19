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
#include <nmea_msgs/Sentence.h>
#include <tf2_ros/static_transform_broadcaster.h>
#include <geometry_msgs/TransformStamped.h>
#include <geometry_msgs/QuaternionStamped.h>
#include <std_msgs/String.h>

#include "cooperation_setup/geo_pos_conv.hpp"

#define TOPIC_NAME_NMEA "nmea_sentence"
#define TOPIC_NAME_ORIGIN "origin"
#define FRAME_NAME_PRIUS_ORIGIN "/prius/my_frame"
#define FRAME_NAME_MKZ_ORIGIN "/mkz/my_frame"

class MKZSetup {

public:

    MKZSetup();

    ~MKZSetup();

    void callback_nmea(const nmea_msgs::Sentence::ConstPtr &msg);


private:

    ros::NodeHandle nh;
    ros::NodeHandle private_nh;
    ros::Publisher pub_origin_fix;

    std::string origin_fix_str;
    std::string origin_quat_str;
    geo_pos_conv geo;

    nmea_msgs::Sentence msg_nmea;
    sensor_msgs::NavSatFix msg_origin_fix;

    bool update_nmea;

};


MKZSetup::MKZSetup() : nh(), private_nh(ros::NodeHandle("~")) {

    ROS_DEBUG("%s", __FUNCTION__);

    /** Set parameter **/
    if (!nh.getParam("origin_fix", origin_fix_str)) {
        ROS_ERROR("Non \"origin_fix\"");
        exit(-1);
    }
    if (!nh.getParam("origin_quat", origin_quat_str)) {
        ROS_ERROR("Non \"origin_quat\"");
        exit(-1);
    }

    double origin_fix[3];
    sscanf(origin_fix_str.c_str(), "%lf,%lf,%lf", &origin_fix[0], &origin_fix[1], &origin_fix[2]);
    geo.set_plane(origin_fix[0], origin_fix[1]);
    double origin_quat[4];
    sscanf(origin_quat_str.c_str(), "%lf,%lf,%lf,%lf", &origin_quat[0], &origin_quat[1], &origin_quat[2],
           &origin_quat[3]);

    double fix[3];
    {
        ros::Subscriber sub_fix = nh.subscribe(TOPIC_NAME_NMEA, 100, &MKZSetup::callback_nmea, this);
        ros::Rate loop_rate(1000);
        std::string key = "<     SOL_COMPUTED PPP ";
        while (ros::ok()) {
            ros::spinOnce();
            if (update_nmea) {
                std::string sen = msg_nmea.sentence;
                size_t found = sen.find(key);
                if (found != std::string::npos) {
                    std::string sen_sub = sen.substr(key.size());
                    std::cout << sen_sub << std::endl;
                    sscanf(sen_sub.c_str(),"%lf %lf %lf",&fix[0],&fix[1],&fix[2]);
                    break;
                }
                update_nmea = false;
            }
            loop_rate.sleep();
        }
    }

    //// Set

    geo.llh_to_xyz(fix[0], fix[1], fix[2]);

    /** Set publisher **/
    static tf2_ros::StaticTransformBroadcaster static_broadcaster;
    geometry_msgs::TransformStamped static_transformStamped;

    std::string from = FRAME_NAME_PRIUS_ORIGIN;
    std::string to = FRAME_NAME_MKZ_ORIGIN;

    static_transformStamped.header.frame_id=msg_nmea.header.frame_id;
    static_transformStamped.header.frame_id = from;
    static_transformStamped.child_frame_id = to;
    static_transformStamped.transform.translation.x = geo.y();
    static_transformStamped.transform.translation.y = -geo.x();
    static_transformStamped.transform.translation.z = geo.z();
    static_transformStamped.transform.rotation.x = origin_quat[0];
    static_transformStamped.transform.rotation.y = origin_quat[1];
    static_transformStamped.transform.rotation.z = origin_quat[2];
    static_transformStamped.transform.rotation.w = origin_quat[3];
    static_broadcaster.sendTransform(static_transformStamped);
    ROS_INFO("Spinning until killed publishing %s to %s", from.c_str(), to.c_str());

    pub_origin_fix = nh.advertise<sensor_msgs::NavSatFix>(TOPIC_NAME_ORIGIN, 1, true);
    msg_origin_fix.header.frame_id=FRAME_NAME_MKZ_ORIGIN;
    msg_origin_fix.latitude=fix[0];
    msg_origin_fix.longitude=fix[1];
    msg_origin_fix.altitude=fix[2];

    pub_origin_fix.publish(msg_origin_fix);

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
void MKZSetup::callback_nmea(const nmea_msgs::Sentence::ConstPtr &msg) {

    ROS_DEBUG("%s", __FUNCTION__);
    msg_nmea = *msg;
    update_nmea = true;

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