//
// Created by ohashi on 19/10/30.
//

#ifndef SRC_VEHICLE_PERCEPTOR_H
#define SRC_VEHICLE_PERCEPTOR_H

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>

#include <Eigen/Core>

#include <ros/ros.h>
#include <ros/console.h>

#include <tf/transform_broadcaster.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>

#include <nav_msgs/Odometry.h>
#include <geometry_msgs/PoseWithCovarianceStamped.h>
#include "../../../../../devel/include/autoware_msgs/DetectedObject.h"
#include "../../../../../devel/include/autoware_msgs/DetectedObjectArray.h"

#include <gnss/geo_pos_conv.hpp>
#include <State.h>

#define VEHICLE_NAME ""
#define PHONES_NAMES "android"

#define ORIGIN_FRAME_ID "odom"
#define ORIGIN_FIX "37.9181551873,-122.331354939,0.0"

#define VEHICLE_STANDARD_DEVIATION_COEFFICIENT 6.0e+3
#define PEDESTRIAN_STANDARD_DEVIATION 0.5

#define ASSIGN_PEDESTRIANS2PHONE_WEIGHT_XY 1.0
#define ASSIGN_PEDESTRIANS2PHONE_WEIGHT_V 1.0
#define ASSIGN_PEDESTRIANS2PHONE_WEIGHT_R 1.0

#define TOPIC_NAME_OBJECTS "detection/object_tracker/objects"
#define TOPIC_NAME_VEHICLE_ODOM "/mkz/gnss_localizer/odom"
#define TOPIC_NAME_PHONE_ODOM "odom"
#define TOPIC_NAME_PEDESTRIAN2PHONE "pedestrian2phone"

class VehiclePhonesManager{

public:

    VehiclePhonesManager(ros::NodeHandle* nh, ros::NodeHandle* private_nh);
    ~VehiclePhonesManager();

    void callback_objects(const autoware_msgs::DetectedObjectArrayConstPtr &msg);
    void callback_vehicle_odom(const nav_msgs::Odometry::ConstPtr &msg);
    //// TODO: callback_vehicle_odom

    void publish_pedestrian2phone_objects();

    void run();

    void getPedestrianObjectsFromObjects();
    bool convertPedestrianObjects2States();
    void generatePedestrianCovariances();
    void convertPhoneOdom2States();
    void assignPedestrians2Phones();
    bool isOrientationActive(const geometry_msgs::Quaternion& q);

private:

    /** Subscriber Class **/
    class MySubscriber{

    public:
        /**
         * My Subscriber
         * @param nh
         * @param topic_name
         */
        inline MySubscriber():is_upadte(false){};
        inline void set_subscriber(const ros::NodeHandle& nh,const std::string& topic_name){
            this->nh=nh;
            this->topic_name=topic_name;
            sub=this->nh.subscribe(this->topic_name,1,&VehiclePhonesManager::MySubscriber::callback,this);
        }
        inline void callback(const nav_msgs::OdometryConstPtr& msg){
            this->msg=*msg;
            is_upadte=true;
        };
        inline nav_msgs::Odometry getMsg() const {return msg;}
        inline bool isUpdate(){return is_upadte;};
        inline void setUpdate(bool b){is_upadte=b;};

    private:
        ros::NodeHandle nh;
        ros::Subscriber sub;
        std::string topic_name;
        nav_msgs::Odometry msg;
        bool is_upadte;
    };


    std::string vehicle_name;
    std::vector<std::string> phone_names;

    ros::NodeHandle& nh;
    ros::NodeHandle& private_nh;
    std::vector<ros::Publisher> pubs_pedestrian2phone;
    ros::Subscriber sub_objects,sub_vehicle_odom;
    std::vector<MySubscriber> subs_phone_odom;


    /** Topics **/

    //// publish_objects msgs
    std::vector<autoware_msgs::DetectedObject> pedestrian2phone_objects; //// TODO: PoseWithCovに変更

    //// subscribe msgs
    autoware_msgs::DetectedObjectArray objects;
    autoware_msgs::DetectedObjectArray pedestrian_objects;
    std::vector<nav_msgs::Odometry> phone_odoms;
    nav_msgs::Odometry vehicle_odom;

    //// topic names
    std::string topic_name_sub_objects,topic_name_sub_vehicle_odom;
    std::vector<std::string> topic_names_sub_phone_odom;
    std::vector<std::string> topic_names_pub_pedestrian2phone;

    //// update flag
    bool update_vehicle_perception,update_vehicle_odom;
    std::vector<bool> update_phone_odoms;

    //// Frame ID
    std::string origin_frame_id;

    /** TF Listener **/
    tf2_ros::Buffer tf_buffer;


    /** Vehicle Likelihood **/
    std::vector<State<>> pedestrian_states;
    std::vector<Eigen::MatrixXd> pedestrian_covariances;
    std::vector<State<>> phone_states;
    std::vector<State<>> pedestrian2phone_states;
    std::vector<Eigen::MatrixXd> pedestrian2phone_covariances;

    /** Others **/
    ros::Time time;

    std::vector<std::ofstream> ofs_pedestrian2phone;
    std::ofstream ofs_vehicle;

};


#endif //SRC_VEHICLE_PERCEPTOR_H
