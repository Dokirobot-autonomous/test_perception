//
// Created by ohashi on 19/09/13.
//

#include <iostream>
#include <string>
#include <vector>

#include <ros/ros.h>
#include <ros/console.h>

#include <geometry_msgs/PoseStamped.h>
#include <sensor_msgs/NavSatFix.h>
#include <tf2_ros/transform_broadcaster.h>
#include <tf2/LinearMath/Quaternion.h>
#include <gnss/geo_pos_conv.hpp>
#include <tf/LinearMath/Quaternion.h>
#include <tf/transform_datatypes.h>
#include <mutex>

#define TOPIC_NAME_SUB_FIX "fix"
#define TOPIC_NAME_PUB_POSE "pose"
#define TOPIC_NAME_PUB_INIT_FIX "init/fix"
#define TOPIC_NAME_PUB_INIT_POSE "init/pose"
#define TOPIC_NAME_PUB_ORIGIN_FIX "origin/fix"

#define PUTLISH_TF false
#define FRAME_ID_ORIGIN "odom"
#define FRAME_ID_GPS "gps"

class Fix2Pose {

public:

    Fix2Pose();
    ~Fix2Pose();

    void callback_fix(const sensor_msgs::NavSatFix::ConstPtr &msg);

    void run();

    void publish(ros::Time time);

private:


    ros::NodeHandle nh;
    ros::NodeHandle private_nh;
    ros::Publisher pub_pose,pub_init_fix,pub_init_pose,pub_ori_fix;
    ros::Subscriber sub_fix;
    std_msgs::Header header;
    std::mutex mtx;

    tf2_ros::TransformBroadcaster br;

    sensor_msgs::NavSatFix msg_fix;

    std::vector<double> origin_fix;
    std::vector<double> orientation;


    bool publish_tf;

    std::string frame_id_origin,frame_id_gps;

    geo_pos_conv geo;

    bool update_fix;

};

Fix2Pose::Fix2Pose() : nh(), private_nh(ros::NodeHandle("~")),
                                 update_fix(false) {

    ROS_DEBUG("%s", __FUNCTION__);

    /** Set parameter **/
    private_nh.param<std::string>("frame_id_origin", frame_id_origin, FRAME_ID_ORIGIN);
    private_nh.param<std::string>("frame_id_gps", frame_id_gps, FRAME_ID_GPS);
    private_nh.param<bool>("publish_tf",publish_tf,PUTLISH_TF);
    if(!private_nh.getParam("origin_fix",origin_fix)){
        ROS_ERROR("No \"origin_fix\"");
        exit(-1);
    }
    private_nh.param<std::vector<double>>("orientation",orientation,{0.0,0.0,0.0,1.0});
    if(orientation.size()==3){
        tf2::Quaternion quat;
        quat.setRPY(orientation[0],orientation[1],orientation[2]);
        orientation[0]=quat.getX();
        orientation[1]=quat.getY();
        orientation[2]=quat.getZ();
        orientation.push_back(quat.getW());
    }
    ROS_INFO("Orientatioin: [%lf,%lf,%lf,%lf]",orientation[0],orientation[1],orientation[2],orientation[3]);


    /** Set publisher **/
    pub_pose=nh.advertise<geometry_msgs::PoseStamped>(TOPIC_NAME_PUB_POSE,10);
    pub_init_pose=nh.advertise<geometry_msgs::PoseStamped>(TOPIC_NAME_PUB_INIT_POSE,1,true);
    pub_init_fix = nh.advertise<sensor_msgs::NavSatFix>(TOPIC_NAME_PUB_INIT_FIX, 1, true);
    pub_ori_fix = nh.advertise<sensor_msgs::NavSatFix>(TOPIC_NAME_PUB_ORIGIN_FIX, 1, true);

    geo.set_plane(origin_fix[0],origin_fix[1]);
    sensor_msgs::NavSatFix ori_fix;
    ori_fix.latitude=origin_fix[0];
    ori_fix.longitude=origin_fix[1];
    pub_ori_fix.publish(ori_fix);

    /** Set subscriber **/
    sub_fix = nh.subscribe(TOPIC_NAME_SUB_FIX, 10, &Fix2Pose::callback_fix, this);

    ros::spin();
}

Fix2Pose::~Fix2Pose() {}

/**
 * callback_fix
 * @param msg
 */
void Fix2Pose::callback_fix(const sensor_msgs::NavSatFix::ConstPtr &msg) {

    auto tmp = *msg;
    ROS_DEBUG_STREAM("[" << std::string(__FUNCTION__) << "]\n" << tmp);
    std::lock_guard<std::mutex> lock(mtx);
    msg_fix = tmp;
    header = msg_fix.header;
    update_fix = true;
    run();

}

/**
 * run
 */
void Fix2Pose::run() {

    ROS_DEBUG_STREAM("[" << std::string(__FUNCTION__) << "]\n");

     //// set position
     geo.llh_to_xyz_us(msg_fix.latitude, msg_fix.longitude, msg_fix.altitude);
     geometry_msgs::PoseStamped pose;

    pose.header=header;
    pose.header.frame_id=frame_id_origin;
    pose.pose.position.x=geo.y();
    pose.pose.position.y=-geo.x();
    pose.pose.position.z=geo.z();
    pose.pose.orientation.x=orientation[0];
    pose.pose.orientation.y=orientation[1];
    pose.pose.orientation.z=orientation[2];
    pose.pose.orientation.w=orientation[3];
    pub_pose.publish(pose);

    static bool init=false;
    if(!init){
        pub_init_fix.publish(msg_fix);
        pub_init_pose.publish(pose);
        init=true;
    }

    if(publish_tf){
        geometry_msgs::TransformStamped tf;
        tf.header = pose.header;
        tf.child_frame_id = frame_id_gps;
        tf.transform.translation.x = pose.pose.position.x;
        tf.transform.translation.y = pose.pose.position.y;
        tf.transform.translation.z = pose.pose.position.z;
        tf.transform.rotation.x = pose.pose.orientation.x;
        tf.transform.rotation.y = pose.pose.orientation.y;
        tf.transform.rotation.z = pose.pose.orientation.z;
        tf.transform.rotation.w = pose.pose.orientation.w;
        br.sendTransform(tf);
    }

    update_fix = false;

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
    ros::init(argc, argv, "fix2pose");
    Fix2Pose node;
    ros::spin();

    return (0);
}
