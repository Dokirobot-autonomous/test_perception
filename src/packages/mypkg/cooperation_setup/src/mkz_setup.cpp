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
#include <sensor_msgs/Imu.h>
#include <tf2_ros/transform_broadcaster.h>

#include "cooperation_setup/geo_pos_conv.hpp"

#define TOPIC_NAME_NMEA "nmea_sentence"
#define TOPIC_NAME_IMU "xsens/imu/data"
#define TOPIC_NAME_INIT_FIX "init/fix"
#define TOPIC_NAME_INIT_IMU "init/imu"
#define TOPIC_NAME_ODOM "odom"
//#define FRAME_NAME_PRIUS_ORIGIN "/prius/my_frame"
#define FRAME_NAME_MKZ_ORIGIN "mkz/my_frame"
#define FRAME_NMAE_MKZ_GPSIMU "mkz/gps_imu"

class MKZSetup {

public:

    MKZSetup();

    ~MKZSetup();

    void callback_nmea(const nmea_msgs::Sentence::ConstPtr &msg);
    void callback_imu(const sensor_msgs::Imu::ConstPtr &msg);

    void run();

private:

    ros::NodeHandle nh;
    ros::NodeHandle private_nh;
    ros::Publisher pub_init_fix,pub_init_imu,pub_odom;
    ros::Subscriber sub_nmea,sub_imu;
    ros::Time current;

    tf2_ros::TransformBroadcaster br;

    geo_pos_conv geo;

    nmea_msgs::Sentence msg_nmea;
    double fix[3];
    sensor_msgs::Imu msg_imu;
    sensor_msgs::NavSatFix msg_init_fix;
    sensor_msgs::Imu msg_init_imu;

    bool update_nmea=false,update_imu=false;

};


MKZSetup::MKZSetup() : nh(), private_nh(ros::NodeHandle("~")) {

    ROS_DEBUG("%s", __FUNCTION__);

    pub_init_fix=nh.advertise<sensor_msgs::NavSatFix>(TOPIC_NAME_INIT_FIX,100,true);
    pub_init_imu=nh.advertise<sensor_msgs::Imu>(TOPIC_NAME_INIT_IMU,100,true);
    pub_odom=nh.advertise<nav_msgs::Odometry>(TOPIC_NAME_ODOM,100);

    sub_nmea=nh.subscribe(TOPIC_NAME_NMEA, 100, &MKZSetup::callback_nmea, this);
    sub_imu=nh.subscribe(TOPIC_NAME_IMU,100,&MKZSetup::callback_imu,this);

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

    static std::string key = "<     SOL_COMPUTED PPP ";

    msg_nmea = *msg;
    std::string sen = msg_nmea.sentence;
    size_t found = sen.find(key);
    if (found != std::string::npos) {
        std::string sen_sub = sen.substr(key.size());
        sscanf(sen_sub.c_str(), "%lf %lf %lf", &fix[0], &fix[1], &fix[2]);
        current=msg_nmea.header.stamp;
        update_nmea = true;
        run();
    }
}

/**
 * callback_fix
 * @param msg
 */
void MKZSetup::callback_imu(const sensor_msgs::Imu::ConstPtr & msg) {

    ROS_DEBUG("%s", __FUNCTION__);
    msg_imu = *msg;
    current=msg_imu.header.stamp;
    update_imu = true;
    run();
}

/**
 *
 */
void MKZSetup::run() {

    ROS_DEBUG("%s", __FUNCTION__);

    static bool init=true;
    if(init){
        if(update_nmea && update_imu){

            geo.set_plane(fix[0], fix[1]);
            msg_init_fix.header=msg_nmea.header;
            msg_init_fix.header.frame_id=FRAME_NAME_MKZ_ORIGIN;
            msg_init_fix.latitude=fix[0];
            msg_init_fix.longitude=fix[1];
            msg_init_fix.altitude=fix[2];
            pub_init_fix.publish(msg_init_fix);

            msg_init_imu=msg_imu;
            pub_init_imu.publish(msg_init_imu);

            init=false;
        }
        else{
            return;
        }
    }

    geo.llh_to_xyz(fix[0],fix[1],fix[2]);

    static int seq=0;
    nav_msgs::Odometry odom;
    odom.header.frame_id=FRAME_NAME_MKZ_ORIGIN;
    odom.header.stamp=current;
    odom.header.seq=seq;
    seq++;
    odom.child_frame_id=FRAME_NMAE_MKZ_GPSIMU;
    odom.pose.pose.position.x=geo.y();
    odom.pose.pose.position.y=-geo.x();
    odom.pose.pose.position.z=geo.z();
    odom.pose.pose.orientation=msg_imu.orientation;
    pub_odom.publish(odom);

    geometry_msgs::TransformStamped tf;
    tf.header=odom.header;
    tf.child_frame_id=odom.child_frame_id;
    tf.transform.translation.x=odom.pose.pose.position.x;
    tf.transform.translation.y=odom.pose.pose.position.y;
    tf.transform.translation.z=odom.pose.pose.position.z;
    tf.transform.rotation.x=odom.pose.pose.orientation.x;
    tf.transform.rotation.y=odom.pose.pose.orientation.y;
    tf.transform.rotation.z=odom.pose.pose.orientation.z;
    tf.transform.rotation.w=odom.pose.pose.orientation.w;
    br.sendTransform(tf);

    update_nmea=false;
    update_imu=false;

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
