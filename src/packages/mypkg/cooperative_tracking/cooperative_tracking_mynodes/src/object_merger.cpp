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
#include <nav_msgs/Odometry.h>
#include <geometry_msgs/TransformStamped.h>
#include <tf2_ros/transform_listener.h>
#include <tf/LinearMath/Quaternion.h>
#include <tf/LinearMath/Matrix3x3.h>

#include "../../../../../../devel/include/autoware_msgs/DetectedObjectArray.h"
#include "../../../../../../devel/include/autoware_msgs/DetectedObject.h"

#define VEHICLE_STANDARD_DEVIATION_COEFFICIENT 4.0e+3
#define PEDESTRIAN_STANDARD_DEVIATION 0.4

#define TOPIC_NAME_SUB_OBJECT_MKS "/mks/detection/lidar_detector/objects"
#define TOPIC_NAME_SUB_OBJECT_MKZ "/mkz/detection/lidar_detector/objects"
#define TOPIC_NAME_SUB_ODOM_MKS "/mks/gnss_localizer/odom"
#define TOPIC_NAME_SUB_ODOM_MKZ "/mkz/gnss_localizer/odom"
#define TOPIC_NAME_PUBLISHED ""

class ObjectMerger {

public:

    ObjectMerger(ros::NodeHandle *nh, ros::NodeHandle *private_nh);

    ~ObjectMerger();

    void callback_objects_mks(const autoware_msgs::DetectedObjectArrayConstPtr &msg);

    void callback_objects_mkz(const autoware_msgs::DetectedObjectArrayConstPtr &msg);

    void callback_odom_mks(const nav_msgs::Odometry::ConstPtr &msg);

    void callback_odom_mkz(const nav_msgs::Odometry::ConstPtr &msg);


    void run();

    bool initialization();

private:

    ros::NodeHandle nh;
    ros::NodeHandle private_nh;
    ros::Publisher pub;
    ros::Subscriber sub_objects_mks, sub_objects_mkz, sub_odom_mks, sub_odom_mkz;
    ros::Time current;

    std::string origin_frame_id;

    autoware_msgs::DetectedObjectArray objects_mks, objects_mkz;
    nav_msgs::Odometry odom_mks, odom_mkz;

    /** TF Listener **/
    tf2_ros::Buffer tf_buffer;

    bool update_objects_mks = false, update_objects_mkz = false;
    bool update_odom_mks = false, update_odom_mkz = false;

    Eigen::MatrixXd p_vehicle_mks, p_vehicle_mkz;

};

ObjectMerger::ObjectMerger(ros::NodeHandle *nh, ros::NodeHandle *private_nh) : nh(*nh), private_nh(*private_nh) {

    ROS_DEBUG("%s", __FUNCTION__);

    /** Set parameter **/
    if(!this->nh.getParam("origin_frame_id",origin_frame_id)){
        ROS_ERROR("Non \"origin_frame_id\"");
        exit(-1);
    }

    //// TF Listener
    static tf2_ros::TransformListener tf_listener(tf_buffer);

    /** Set publisher **/
    pub = this->nh.advertise<geometry_msgs::PoseStamped>(TOPIC_NAME_PUBLISHED, 1);

    /** Set subscriber **/
    sub_objects_mks = this->nh.subscribe(TOPIC_NAME_SUB_OBJECT_MKS, 1, &ObjectMerger::callback_objects_mks, this);
    sub_objects_mkz = this->nh.subscribe(TOPIC_NAME_SUB_OBJECT_MKZ, 1, &ObjectMerger::callback_objects_mkz, this);
    sub_odom_mks = this->nh.subscribe(TOPIC_NAME_SUB_ODOM_MKS, 1, &ObjectMerger::callback_odom_mks, this);
    sub_odom_mkz = this->nh.subscribe(TOPIC_NAME_SUB_ODOM_MKZ, 1, &ObjectMerger::callback_odom_mkz, this);


    ros::Rate loop(10);
    while(ros::ok()){
        ros::spinOnce();
        run();
        loop.sleep();
    }


    ros::spin();
}

ObjectMerger::~ObjectMerger() {}

/**
 * callback_mks
 * @param msg
 */
void ObjectMerger::callback_objects_mks(const autoware_msgs::DetectedObjectArrayConstPtr &msg) {

    objects_mks = *msg;
    ROS_DEBUG_STREAM("[" << std::string(__FUNCTION__) << "]\n" << objects_mks);
    current = objects_mks.header.stamp;
    update_objects_mks = true;

}

/**
 * callback_mks
 * @param msg
 */
void ObjectMerger::callback_objects_mkz(const autoware_msgs::DetectedObjectArrayConstPtr &msg) {

    objects_mkz = *msg;
    ROS_DEBUG_STREAM("[" << std::string(__FUNCTION__) << "]\n" << objects_mkz);
    current = objects_mkz.header.stamp;
    update_objects_mkz = true;

}

/**
 *
 * @param msg
 */
void ObjectMerger::callback_odom_mks(const nav_msgs::Odometry::ConstPtr &msg) {

    odom_mks = *msg;
    ROS_DEBUG_STREAM("[" << std::string(__FUNCTION__) << "]\n" << odom_mks);
    current = odom_mks.header.stamp;

    p_vehicle_mks = Eigen::MatrixXd(3, 3);
    p_vehicle_mks << odom_mks.pose.covariance[0], odom_mks.pose.covariance[1], odom_mks.pose.covariance[5],
            odom_mks.pose.covariance[6], odom_mks.pose.covariance[7], odom_mks.pose.covariance[11],
            odom_mks.pose.covariance[30], odom_mks.pose.covariance[31], odom_mks.pose.covariance[35];
    ROS_DEBUG_STREAM(
            "[" << std::string(__FUNCTION__) << "]\n" << odom_mks.header << odom_mks.pose.pose << "covariance: \n"
                << p_vehicle_mks);

    update_odom_mks = true;

}

/**
 *
 * @param msg
 */
void ObjectMerger::callback_odom_mkz(const nav_msgs::Odometry::ConstPtr &msg) {

    odom_mkz = *msg;
    ROS_DEBUG_STREAM("[" << std::string(__FUNCTION__) << "]\n" << odom_mkz);
    current = odom_mkz.header.stamp;

    p_vehicle_mkz = Eigen::MatrixXd(3, 3);
    p_vehicle_mkz << odom_mkz.pose.covariance[0], odom_mkz.pose.covariance[1], odom_mkz.pose.covariance[5],
            odom_mkz.pose.covariance[6], odom_mkz.pose.covariance[7], odom_mkz.pose.covariance[11],
            odom_mkz.pose.covariance[30], odom_mkz.pose.covariance[31], odom_mkz.pose.covariance[35];
    ROS_DEBUG_STREAM(
            "[" << std::string(__FUNCTION__) << "]\n" << odom_mkz.header << odom_mkz.pose.pose << "covariance: \n"
                << p_vehicle_mkz);

    update_odom_mkz = true;

}

/**
 * run
 */
void ObjectMerger::run() {

    ROS_DEBUG_STREAM("[" << std::string(__FUNCTION__) << "]\n");

    /** Transform objects from vehicle to origin **/
    autoware_msgs::DetectedObjectArray objects_origin_mks,objects_origin_mkz;

    ////MKS
    for(size_t i=0;i<objects_mks.objects.size();i++){
        autoware_msgs::DetectedObject obj = objects_mks.objects[i];
        geometry_msgs::PoseStamped in, out;
        in.header = obj.header;
        in.pose = obj.pose;
        out.header.frame_id = origin_frame_id;
        out.header.stamp = in.header.stamp;
        try {
/*
            geometry_msgs::TransformStamped transform = tf_buffer.lookupTransform(origin_frame_id, in.header.frame_id,
                                                                                  in.header.stamp, ros::Duration(1.0));
*/
            geometry_msgs::TransformStamped transform = tf_buffer.lookupTransform(origin_frame_id, in.header.frame_id,
                                                                                  ros::Time::now(), ros::Duration(1.0));
            tf2::doTransform(in.pose.position, out.pose.position, transform);
        } catch (tf2::TransformException &ex) {
            ROS_WARN("Could NOT transform %s to %s: %s", obj.header.frame_id.c_str(), origin_frame_id.c_str(),
                     ex.what());
            return;
        }
        objects_origin_mks.objects.push_back(obj);
    }

    ////MKZ
    for(size_t i=0;i<objects_mkz.objects.size();i++) {
        autoware_msgs::DetectedObject obj = objects_mkz.objects[i];
        geometry_msgs::PoseStamped in, out;
        in.header = obj.header;
        in.pose = obj.pose;
        out.header.frame_id = origin_frame_id;
        out.header.stamp = in.header.stamp;
        try {
/*
            geometry_msgs::TransformStamped transform = tf_buffer.lookupTransform(origin_frame_id, in.header.frame_id,
                                                                                  in.header.stamp, ros::Duration(1.0));
*/
            geometry_msgs::TransformStamped transform = tf_buffer.lookupTransform(origin_frame_id, in.header.frame_id,
                                                                                  ros::Time::now(), ros::Duration(1.0));
            tf2::doTransform(in.pose.position, out.pose.position, transform);
        } catch (tf2::TransformException &ex) {
            ROS_WARN("Could NOT transform %s to %s: %s", obj.header.frame_id.c_str(), origin_frame_id.c_str(),
                     ex.what());
            return;
        }
        objects_origin_mkz.objects.push_back(obj);
    }


    /** Compute covariances **/
    std::vector<Eigen::MatrixXd> covariance_objects_mks(objects_mks.objects.size());
    std::vector<Eigen::MatrixXd> covariance_objects_mkz(objects_mkz.objects.size());

    //// Set param
    double vsd_coff, psd;
    private_nh.param("vehicle_standard_deviation_coefficient", vsd_coff, VEHICLE_STANDARD_DEVIATION_COEFFICIENT);
    private_nh.param("pedestrian_standard_deviation", psd, PEDESTRIAN_STANDARD_DEVIATION);

    ////MKS
    Eigen::MatrixXd mks_sigma = Eigen::MatrixXd(3, 3);
    double mks_rpy[3];
    if(update_odom_mks){
        mks_sigma =vsd_coff*p_vehicle_mks;
        tf::Quaternion quat(odom_mks.pose.pose.orientation.x, odom_mks.pose.pose.orientation.y,
                            odom_mks.pose.pose.orientation.z, odom_mks.pose.pose.orientation.w);
        tf::Matrix3x3(quat).getRPY(mks_rpy[0], mks_rpy[1], mks_rpy[2]);
    } else {
        mks_sigma = Eigen::MatrixXd::Zero(3, 3);
        mks_rpy[0]=mks_rpy[1]=mks_rpy[2]=0.0;
    }
    for (size_t i = 0; i < objects_mks.objects.size(); i++) {
        const auto &p = objects_mks.objects[i].pose;
        Eigen::MatrixXd p_sigma, transform_v2p(2, 3), transform_pl2pw(2, 2);
        p_sigma = Eigen::MatrixXd::Identity(2, 2) * psd * psd;
        transform_v2p << 1.0, 0.0, -p.position.x * sin(mks_rpy[2]) - p.position.y * cos(mks_rpy[2]),
                0.0, 1.0, p.position.x * cos(mks_rpy[2]) - p.position.y * sin(mks_rpy[2]);
        transform_pl2pw << cos(mks_rpy[2]), -sin(mks_rpy[2]), sin(mks_rpy[2]), cos(mks_rpy[2]);
        covariance_objects_mks[i] = transform_v2p * mks_sigma * transform_v2p.transpose() +
                             transform_pl2pw * p_sigma * transform_pl2pw.transpose();
    }

    ////MKZ
    Eigen::MatrixXd mkz_sigma = Eigen::MatrixXd(3, 3);
    double mkz_rpy[3];
    if(update_odom_mks){
        mkz_sigma =vsd_coff*p_vehicle_mks;
        tf::Quaternion quat(odom_mks.pose.pose.orientation.x, odom_mks.pose.pose.orientation.y,
                            odom_mks.pose.pose.orientation.z, odom_mks.pose.pose.orientation.w);
        tf::Matrix3x3(quat).getRPY(mkz_rpy[0], mkz_rpy[1], mkz_rpy[2]);
    } else {
        mkz_sigma = Eigen::MatrixXd::Zero(3, 3);
        mkz_rpy[0]=mkz_rpy[1]=mkz_rpy[2]=0.0;
    }
    for (size_t i = 0; i < objects_mks.objects.size(); i++) {
        const auto &p = objects_mks.objects[i].pose;
        Eigen::MatrixXd p_sigma, transform_v2p(2, 3), transform_pl2pw(2, 2);
        p_sigma = Eigen::MatrixXd::Identity(2, 2) * psd * psd;
        transform_v2p << 1.0, 0.0, -p.position.x * sin(mkz_rpy[2]) - p.position.y * cos(mkz_rpy[2]),
                0.0, 1.0, p.position.x * cos(mkz_rpy[2]) - p.position.y * sin(mkz_rpy[2]);
        transform_pl2pw << cos(mkz_rpy[2]), -sin(mkz_rpy[2]), sin(mkz_rpy[2]), cos(mkz_rpy[2]);
        covariance_objects_mks[i] = transform_v2p * mkz_sigma * transform_v2p.transpose() +
                                    transform_pl2pw * p_sigma * transform_pl2pw.transpose();
    }

    /** Merge objects **/




}

/**
 * Main Function
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char **argv) {

    // Initialize ROS
    ros::init(argc, argv, "object_merger");
    ros::NodeHandle nh, private_nh("~");
    // debug mode
    bool debug;
    private_nh.param<bool>("debug", debug, false);
    if (debug) {
        if (ros::console::set_logger_level(ROSCONSOLE_DEFAULT_NAME, ros::console::levels::Debug)) {
            ros::console::notifyLoggerLevelsChanged();
        }
    }
    ObjectMerger node(&nh, &private_nh);
    ros::spin();

    return (0);
}
