//
// Created by ohashi on 19/09/13.
//

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <time.h>

#include <ros/ros.h>
#include <ros/console.h>

#include <std_msgs/Float64.h>


class AndroidSetup {

public:

    AndroidSetup();

    ~AndroidSetup();

private:

    ros::NodeHandle nh;
    ros::NodeHandle private_nh;
    std::vector<ros::Publisher> pubs;
    std::vector<std_msgs::Float64> msgs;

};

AndroidSetup::AndroidSetup() : nh(), private_nh(ros::NodeHandle("~")) {

    ROS_DEBUG("%s", __FUNCTION__);

    std::string ifpath;
    if(!private_nh.getParam("ifpath",ifpath)){
        ROS_ERROR("No \"ifpath\"");
        exit(-1);
    }

    std::string date_str;
    if(!private_nh.getParam("date",date_str)){
        time_t now = std::time(nullptr);
        struct tm *localNow = std::localtime(&now);
        char date_char[50];
        sprintf(date_char, "%d-%02d-%02d", localNow->tm_year+1900, localNow->tm_mon+1, localNow->tm_mday);
        date_str=date_char;
    }

    std::string ifname=ifpath+"."+date_str+"-android-dt.txt";
    std::ifstream ifs(ifname);
    if(ifs.fail()){
        ROS_ERROR("No \"%s\"",ifname.c_str());
        exit(-1);
    }

    std::string token;
    while(std::getline(ifs,token)){

        std::string phone_name;
        double dt_s,dt_nsec;
        std::istringstream iss(token);
        iss>>phone_name>>dt_s>>dt_nsec;

        ros::Publisher pub=nh.advertise<std_msgs::Float64>("android/"+phone_name+"/"+"dt",1,true);

        double dt=dt_s+dt_nsec*1E-9;
        std_msgs::Float64 msg;
        msg.data=dt;

        pub.publish(msg);

        msgs.push_back(msg);
        pubs.push_back(pub);
    }

    ros::spin();
}

AndroidSetup::~AndroidSetup() {}


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
    ros::init(argc, argv, "android_setup");
    AndroidSetup node;
    ros::spin();

    return (0);
}
