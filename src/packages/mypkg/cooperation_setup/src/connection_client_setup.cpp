//
// Created by ohashi on 19/09/13.
//

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>

#include "../../../../../devel/include/autoware_msgs/DetectedObject.h"
#include "../../../../../devel/include/autoware_msgs/DetectedObjectArray.h"

#include <ros/ros.h>
#include <ros/console.h>

#define TOPIC_NAME_OBJECTS "detection/lidar_detector/objects"
#define TOPIC_NAME_ODOM "navsat/odom"
#define TOPIC_NAME_FIX "vehicle/gps/fix"
#define TOPIC_NAME_INIT_FIX "origin_fix"
#define FRAME_NAME_PRIUS_ORIGIN "/prius/my_frame"
#define FRAME_NAME_ConnectionClient_ORIGIN "/mkz/my_frame"
#define FRAME_NAME_ODOM "odom"

class ConnectionClientSetup {

public:

    ConnectionClientSetup();

    ~ConnectionClientSetup();

    void callback_objects(const autoware_msgs::DetectedObjectArray::ConstPtr &msg);

    void publish(const std::string &objects_str);

    void mode_subscriber();

    void mode_publisher();

private:

    ros::NodeHandle nh;
    ros::NodeHandle private_nh;
    std::string client_mode;
    ros::Publisher pub_objects;
    ros::Subscriber sub_objects;

    std::string origin_fix_str;

    //// Shared Memory
    std::string file_path;
    int id;
    key_t key;
    int size;
    int seg_id;
    char *shared_memory;

};


ConnectionClientSetup::ConnectionClientSetup() : nh(), private_nh(ros::NodeHandle("~")){

    ROS_DEBUG("%s", __FUNCTION__);

    /** Set parameter **/
    if (!private_nh.getParam("client_mode", client_mode)) {
        ROS_ERROR("Non \"client_mode\"");
        exit(-1);
    }

    file_path="./key.dat";
    id=50;
    // 空のファイル作成
    FILE *fp;
    fp = fopen(file_path.c_str(), "w");
    fclose(fp);

    // IPC keyの取得
    key=ftok(file_path.c_str(), id);
    if (key == -1) {
        ROS_ERROR("Failed to acquire key");
        exit(-1);
    }

    ROS_INFO("MODE: %s", client_mode.c_str());
    if (client_mode == "subscriber") {
        mode_subscriber();
    } else if (client_mode == "publisher") {
        mode_publisher();
    } else {
        ROS_ERROR("\"client_mode\" should be \"subscriber\" or \"publisher \"");
        exit(-1);
    }

    ros::spin();
}

/**
 * ~ConnectionClientSetup
 */
ConnectionClientSetup::~ConnectionClientSetup() {

    // 共有メモリをプロセスから切り離す
    shmdt(shared_memory);

    // 共有メモリを解放する
    if (client_mode == "subscriber") {
        shmctl(seg_id, IPC_RMID, NULL);
    }
}

/**
 *
 */
void ConnectionClientSetup::mode_subscriber() {

    ROS_DEBUG("%s", __FUNCTION__);


    // 共有メモリIDの取得
    size=0x6400;
    seg_id = shmget(key, size, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    if (seg_id == -1) {
        ROS_ERROR("Failed to acquire segment");
        exit(-1);
    }

    // 共有メモリをプロセスにアタッチする
    shared_memory = reinterpret_cast<char *>(shmat(seg_id, 0, 0));

    sub_objects = nh.subscribe(TOPIC_NAME_OBJECTS, 100, &ConnectionClientSetup::callback_objects, this);

}

/**
 *
 */
void ConnectionClientSetup::mode_publisher() {

    ROS_DEBUG("%s", __FUNCTION__);

    pub_objects=nh.advertise<autoware_msgs::DetectedObjectArray>(TOPIC_NAME_OBJECTS,100);

    seg_id = shmget(key, 0, 0);
    if (seg_id == -1) {
        ROS_ERROR("Failed to acquire segment");
        exit(-1);
    }

    // 共有メモリをプロセスにアタッチする
    shared_memory = reinterpret_cast<char *>(shmat(seg_id, 0, 0));

    std::string str, str_pre;
    while (ros::ok()) {

        str = shared_memory;
        if (str != str_pre) {
            std::thread thread([this, &str]() { this->publish(str); });
            thread.detach();
        }

        str_pre=str;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

}


/**
 * callback_fix
 * @param msg
 */
void ConnectionClientSetup::callback_objects(const autoware_msgs::DetectedObjectArray::ConstPtr &msg) {

    ROS_DEBUG("%s", __FUNCTION__);

    autoware_msgs::DetectedObjectArray objects=*msg;

    std::stringstream sstr;
    sstr << objects.header.seq << " " << objects.header.stamp.sec << " "
         << objects.header.stamp.nsec << " " << objects.header.frame_id << std::endl;
    for (const auto &ob:objects.objects) {
        sstr << ob.header.seq << " " << ob.header.stamp.sec << " " << ob.header.stamp.nsec << " "
             << ob.header.frame_id << " " << ob.label << " " << ob.id << " " << ob.score << " "
             << ob.pose.position.x << " " << ob.pose.position.y << " " << ob.pose.position.z << " "
             << ob.pose.orientation.x << " " << ob.pose.orientation.y << " " << ob.pose.orientation.z << " "
             << ob.pose.orientation.w << " "
             << ob.velocity.linear.x << " " << ob.velocity.linear.y << " " << ob.velocity.linear.z << " "
             << ob.velocity.angular.x << " " << ob.velocity.angular.y << " " << ob.velocity.angular.z << " "
             << ob.acceleration.linear.x << " " << ob.acceleration.linear.y << " " << ob.acceleration.linear.z << " "
             << ob.acceleration.angular.x << " " << ob.acceleration.angular.y << " " << ob.acceleration.angular.z << " "
             << ob.dimensions.x << " " << ob.dimensions.y << " " << ob.dimensions.z << std::endl;

    }
    ROS_DEBUG("%s,%s",__FUNCTION__,sstr.str().c_str());

    // 共有メモリに書き込む
    sprintf(shared_memory, sstr.str().c_str());

}

/**
 *
 */
void ConnectionClientSetup::publish(const std::string &objects_str) {

    ROS_DEBUG("%s,%s", __FUNCTION__,objects_str.c_str());

    autoware_msgs::DetectedObjectArray objects;

 /*   std::stringstream sstr(objects_str);
    std::string token;
    std::getline(sstr,token);
    std::istringstream istr(token);

    istr >> objects.header.seq;
    istr >> objects.header.stamp.sec;
    istr >> objects.header.stamp.nsec;
    istr >> objects.header.frame_id;
    while(std::getline(sstr,token)){
        autoware_msgs::DetectedObject ob;
        istr.str(token);
        istr >> ob.header.seq;
        istr >> ob.header.stamp.sec;
        istr >> ob.header.stamp.nsec;
        istr >> ob.header.frame_id;
        istr >> ob.label;
        istr >> ob.id;
        istr >> ob.score;
        istr >> ob.pose.position.x;
        istr >> ob.pose.position.y;
        istr >> ob.pose.position.z;
        istr >> ob.pose.orientation.x;
        istr >> ob.pose.orientation.y;
        istr >> ob.pose.orientation.z;
        istr >> ob.pose.orientation.w;
        istr >> ob.velocity.linear.x;
        istr >> ob.velocity.linear.y;
        istr >> ob.velocity.linear.z;
        istr >> ob.velocity.angular.x;
        istr >> ob.velocity.angular.y;
        istr >> ob.velocity.angular.z;
        istr >> ob.acceleration.linear.x;
        istr >> ob.acceleration.linear.y;
        istr >> ob.acceleration.linear.z;
        istr >> ob.acceleration.angular.x;
        istr >> ob.acceleration.angular.y;
        istr >> ob.acceleration.angular.z;
        istr >> ob.dimensions.x;
        istr >> ob.dimensions.y;
        istr >> ob.dimensions.z;
        objects.objects.push_back(ob);
    }
*/

    std::stringstream sstr(objects_str);
    std::string token;
    std::getline(sstr,token);
    {
        std::istringstream istr(token);
        istr >> objects.header.seq >> objects.header.stamp.sec >> objects.header.stamp.nsec
             >> objects.header.frame_id;
    }
    while(std::getline(sstr,token)){
        autoware_msgs::DetectedObject ob;
        std::istringstream istr(token);
        istr >> ob.header.seq >> ob.header.stamp.sec >> ob.header.stamp.nsec
             >> ob.header.frame_id >> ob.label >> ob.id >> ob.score
             >> ob.pose.position.x >> ob.pose.position.y >> ob.pose.position.z
             >> ob.pose.orientation.x >> ob.pose.orientation.y >> ob.pose.orientation.z
             >> ob.pose.orientation.w
             >> ob.velocity.linear.x >> ob.velocity.linear.y >> ob.velocity.linear.z
             >> ob.velocity.angular.x >> ob.velocity.angular.y >> ob.velocity.angular.z
             >> ob.acceleration.linear.x >> ob.acceleration.linear.y >> ob.acceleration.linear.z
             >> ob.acceleration.angular.x >> ob.acceleration.angular.y >> ob.acceleration.angular.z
             >> ob.dimensions.x >> ob.dimensions.y >> ob.dimensions.z;
        objects.objects.push_back(ob);
    }

    pub_objects.publish(objects);

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
    ros::init(argc, argv, "connection_client_setup");
    ConnectionClientSetup node;
    ros::spin();

    return (0);
}
