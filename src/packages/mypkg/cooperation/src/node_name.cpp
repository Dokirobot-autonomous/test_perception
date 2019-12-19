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

#define TOPIC_NAME_SUBSCRIBED ""
#define TOPIC_NAME_PUBLISHED ""

class ClassName{

public:

    ClassName();
    ~ClassName();
    void callback(const sensor_msgs::Imu::ConstPtr &msg);

private:

    ros::NodeHandle nh;
    ros::NodeHandle private_nh;
    ros::Publisher pub;
    ros::Subscriber sub;

};

ClassName::ClassName() : nh(), private_nh(ros::NodeHandle("~"))
{

    ROS_DEBUG("%s",__FUNCTION__);

    /** Set parameter **/

    /** Set publisher **/
    pub=nh.advertise<geometry_msgs::PoseStamped>(TOPIC_NAME_PUBLISHED, 1);

    /** Set subscriber **/
    sub=nh.subscribe(TOPIC_NAME_SUBSCRIBED,1,&ClassName::callback,this);

    ros::spin();
}

ClassName::~ClassName() {}


void ClassName::callback(const sensor_msgs::Imu::ConstPtr &msg) {

    ROS_DEBUG("%s",__FUNCTION__);

    sensor_msgs::Imu msg_in=*msg;

}

// -----------------------------------
//	Main Function
// -----------------------------------
int main(int argc, char **argv) {

    // debug mode
    if (ros::console::set_logger_level(ROSCONSOLE_DEFAULT_NAME, ros::console::levels::Debug)) {
        ros::console::notifyLoggerLevelsChanged();
    }

    // Initialize ROS
    ros::init(argc, argv, "node_name");
    ClassName node;
    ros::spin();

    return (0);
}
