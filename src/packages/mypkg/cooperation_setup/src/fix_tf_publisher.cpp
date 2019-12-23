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

#include <sensor_msgs/NavSatFix.h>
#include <tf2_ros/static_transform_broadcaster.h>
#include <geometry_msgs/TransformStamped.h>
#include <geometry_msgs/QuaternionStamped.h>


#include <gnss/geo_pos_conv.hpp>

#define TOPIC_NAME_FIX1 "fix1"
#define TOPIC_NAME_FIX2 "fix2"
#define FRAME_NAME_FIX1 "/prius/my_frame"
#define FRAME_NAME_FIX2 "/mkz/my_frame"

class FixTfPublisher {

public:

    FixTfPublisher();

    ~FixTfPublisher();

    void callback_fix1(const sensor_msgs::NavSatFix::ConstPtr &msg);

    void callback_fix2(const sensor_msgs::NavSatFix::ConstPtr &msg);


private:

    ros::NodeHandle nh;
    ros::NodeHandle private_nh;

    sensor_msgs::NavSatFix msg_fix1, msg_fix2;
    std::string frame_id_fix1, frame_id_fix2;

    geo_pos_conv geo;

    bool update_fix1, update_fix2;

};


FixTfPublisher::FixTfPublisher() : nh(), private_nh(ros::NodeHandle("~")),
                                   update_fix1(false), update_fix2(false) {

    ROS_DEBUG("%s", __FUNCTION__);

    if (!private_nh.getParam("frame_id_fix1", frame_id_fix1)) {
        ROS_ERROR("No \"frame_id_fix1\"");
        exit(-1);
    }
    if (!private_nh.getParam("frame_id_fix2", frame_id_fix2)) {
        ROS_ERROR("No \"frame_id_fix2\"");
        exit(-1);
    }

    ros::Subscriber sub_fix1 = nh.subscribe(TOPIC_NAME_FIX1, 100, &FixTfPublisher::callback_fix1, this);
    ros::Subscriber sub_fix2 = nh.subscribe(TOPIC_NAME_FIX2, 100, &FixTfPublisher::callback_fix2, this);
    ros::Rate loop_rate(1000);
    while (ros::ok()) {
        ros::spinOnce();
        if (update_fix1 && update_fix2) {
            break;
        }
        loop_rate.sleep();
    }

    geo.set_plane(msg_fix1.latitude, msg_fix1.longitude);
    geo.llh_to_xyz_us(msg_fix2.latitude, msg_fix2.longitude, msg_fix2.altitude);


    /** Set publisher **/
    static tf2_ros::StaticTransformBroadcaster static_broadcaster;
    geometry_msgs::TransformStamped static_transformStamped;

    std::string from = frame_id_fix1;
    std::string to = frame_id_fix2;

    static_transformStamped.header.frame_id = from;
    static_transformStamped.child_frame_id = to;
    static_transformStamped.transform.translation.x = geo.y();
    static_transformStamped.transform.translation.y = -geo.x();
    static_transformStamped.transform.translation.z = 0.0;
    static_transformStamped.transform.rotation.x = 0.0;
    static_transformStamped.transform.rotation.y = 0.0;
    static_transformStamped.transform.rotation.z = 0.0;
    static_transformStamped.transform.rotation.w = 1.0;
    static_broadcaster.sendTransform(static_transformStamped);


    ROS_INFO("Spinning until killed publishing %s to %s", from.c_str(), to.c_str());

    ros::spin();
}

/**
 * ~FixTfPublisher
 */
FixTfPublisher::~FixTfPublisher() {}

/**
 * callback_fix1
 * @param msg
 */
void FixTfPublisher::callback_fix1(const sensor_msgs::NavSatFix::ConstPtr &msg) {

    ROS_DEBUG("%s", __FUNCTION__);
    msg_fix1 = *msg;
    update_fix1 = true;

}

/**
 * callback_fix2
 * @param msg
 */
void FixTfPublisher::callback_fix2(const sensor_msgs::NavSatFix::ConstPtr &msg) {

    ROS_DEBUG("%s", __FUNCTION__);
    msg_fix2 = *msg;
    update_fix2 = true;

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
    ros::init(argc, argv, "fix_tf_publisher");
    FixTfPublisher node;
    ros::spin();

    return (0);
}
