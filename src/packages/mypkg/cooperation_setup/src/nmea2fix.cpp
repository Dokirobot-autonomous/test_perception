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
#include <geometry_msgs/QuaternionStamped.h>
#include <tf2_ros/transform_broadcaster.h>

#include <gnss/geo_pos_conv.hpp>

#define TOPIC_NAME_NMEA "nmea_sentence"
#define TOPIC_NAME_FIX_SOL_COMPUTED "fix"

class Nmea2Fix {

public:

    Nmea2Fix();

    ~Nmea2Fix();

    void callback_nmea(const nmea_msgs::Sentence::ConstPtr &msg);

    void run();

private:

    ros::NodeHandle nh;
    ros::NodeHandle private_nh;
    ros::Publisher pub_fix;
    ros::Subscriber sub_nmea;
    ros::Time current;
    
    nmea_msgs::Sentence msg_nmea;
    double fix[3];
    sensor_msgs::NavSatFix msg_fix;
    std::string vehicle_name;

    bool update_nmea=false;

};


Nmea2Fix::Nmea2Fix() : nh(), private_nh(ros::NodeHandle("~")) {

    ROS_DEBUG("%s", __FUNCTION__);

    /** Set Parameter **/
    if(!private_nh.getParam("vehicle_name",vehicle_name)){
        ROS_ERROR("No \"vehicle_name\"");
        exit(-1);
    }

    pub_fix=nh.advertise<sensor_msgs::NavSatFix>(TOPIC_NAME_FIX_SOL_COMPUTED, 10);

    sub_nmea=nh.subscribe(TOPIC_NAME_NMEA, 100, &Nmea2Fix::callback_nmea, this);

}

/**
 * ~Nmea2Fix
 */
Nmea2Fix::~Nmea2Fix() {}

/**
 * callback_fix
 * @param msg
 */
void Nmea2Fix::callback_nmea(const nmea_msgs::Sentence::ConstPtr &msg) {

    msg_nmea = *msg;
    ROS_DEBUG_STREAM("[" << std::string(__FUNCTION__) << "]\n" << msg_nmea);

    std::string sen = msg_nmea.sentence;
    if(vehicle_name.compare("mkz")==0){
        std::vector<std::string> keys={"<     SOL_COMPUTED PPP ","<     SOL_COMPUTED PPP_CONVERGING "};

        for(const auto key:keys){
            size_t found = sen.find(key);
            if (found != std::string::npos) {
                std::string sen_sub = sen.substr(key.size());
                sscanf(sen_sub.c_str(), "%lf %lf %lf", &fix[0], &fix[1], &fix[2]);
                msg_fix.header=msg_nmea.header;
                msg_fix.latitude=fix[0];
                msg_fix.longitude=fix[1];
                msg_fix.altitude=fix[2];
                current=msg_nmea.header.stamp;
                pub_fix.publish(msg_fix);
            }
        }
    }
    else if(vehicle_name.compare("mks")==0){
        static std::string key = ";SOL_COMPUTED,PPP,";
        size_t found = sen.find(key);
        if (found != std::string::npos) {
            std::string sen_sub = sen.substr(found+key.size());
            sscanf(sen_sub.c_str(), "%lf,%lf,%lf", &fix[0], &fix[1], &fix[2]);
            msg_fix.header=msg_nmea.header;
            msg_fix.latitude=fix[0];
            msg_fix.longitude=fix[1];
            msg_fix.altitude=fix[2];
            current=msg_nmea.header.stamp;
            pub_fix.publish(msg_fix);
        }
    }

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
    ros::init(argc, argv, "nmea2fix");
    Nmea2Fix node;
    ros::spin();

    return (0);
}
