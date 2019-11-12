//
// Created by ohashi on 19/10/30.
//

#ifndef SRC_VEHICLE_PERCEPTOR_H
#define SRC_VEHICLE_PERCEPTOR_H

#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include <Eigen/Core>

#include <ros/ros.h>
#include <ros/console.h>

#include <tf/transform_broadcaster.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>

#include "../../../../../devel/include/autoware_msgs/DetectedObject.h"
#include "../../../../../devel/include/autoware_msgs/DetectedObjectArray.h"

#include <phone_localizer.h>

#define PHONE_NAMES "android"
#define VEHICLE_NAME "prius"

#define ORIGIN_FRAME_ID "odom"
#define ORIGIN_FIX "37.9181551873,-122.331354939,0.0"

#define ASSIGN_PEDESTRIANS_TO_PHONES_EXTRACTION_PARTICLE_SIZE 100

#define PARTICLEFILTER_VEHICLE_PERCEPTION_XY_STANDARD_DEVIATION 0.3                      //
#define PARTICLEFILTER_VEHICLE_PERCEPTION_V_STANDARD_DEVIATION 0.01                      //
#define PARTICLEFILTER_VEHICLE_PERCEPTION_R_STANDARD_DEVIATION 2.0*M_PI/180                      //

#define TOPIC_NAME_VEHICLE_PERCEPTION "/detection/object_tracker/objects"

class VehiclePhoneManager{

public:

    VehiclePhoneManager(ros::NodeHandle* nh, ros::NodeHandle* private_nh, std::vector<PhoneLocalizer*>* phone_localizers, std::string vehicle_name);
//    VehiclePhoneManager(ros::NodeHandle& nh, ros::NodeHandle& private_nh);
    ~VehiclePhoneManager();

    void callback_vehicle_perception(const autoware_msgs::DetectedObjectArrayConstPtr &msg);

    void run();

    void getPedestriansFromVehiclePerception();
    void transformPedestriansFromVehicleToOrigin();
    void assignVehiclePedestriansToPhones();
    void particleLikelihoodVehiclePerceptionXY();
    void particleLikelihoodVehiclePerceptionV();
    void particleLikelihoodVehiclePerceptionR();

    double computeWeightXY(const State<>& pedestrian,const State<>& particle);
    double computeWeightV(const State<>& pedestrian,const State<>& particle);
    double computeWeightR(const State<>& pedestrian,const State<>& particle);

private:

    std::string vehicle_name;

    ros::NodeHandle& nh;
    ros::NodeHandle& private_nh;
    ros::Subscriber sub_vehicle_perception;

    /** Subscribed Topics **/
    autoware_msgs::DetectedObjectArray vehicle_perception_objects;
    autoware_msgs::DetectedObjectArray vehicle_perception_pedestrians;
    bool vehicle_perception_update;

    //// Frame ID
    std::string origin_frame_id;

    /** TF Listener **/
    tf2_ros::Buffer tf_buffer;

    /** Phone Localizer (external)**/
    std::vector<PhoneLocalizer*>& phone_localizers;

    /** Vehicle Likelihood **/
    std::vector<State<>> pedestrian_origin_states;
    std::vector<int> pedestrian_indices_assigned_to_phones;

    /** GPS fix -> pose **/
    geo_pos_conv geo;
    double origin_lat,origin_lon,origin_ele;

    /** Others **/
    ros::Time time;

};


#endif //SRC_VEHICLE_PERCEPTOR_H
