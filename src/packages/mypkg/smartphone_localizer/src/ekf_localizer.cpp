//
// Created by ohashi on 19/09/13.
//

#include <math.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

#include <Eigen/Core>
#include <Eigen/LU>
#include <Eigen/Geometry>

#include <ros/ros.h>
#include <ros/console.h>

#include <sensor_msgs/Imu.h>
#include <sensor_msgs/MagneticField.h>
#include <sensor_msgs/NavSatFix.h>
#include <geometry_msgs/PoseStamped.h>
#include <tf/transform_broadcaster.h>

#define GRAVITATIONAL_ACCELERATION 9.80665 // m/s^2
#define ACCELERATION_Z_BIAS 0.37

#define TOPIC_NAME_IMU "/android/imu"
#define TOPIC_NAME_MAGNETIC "/android/magnetic_field"
#define TOPIC_NAME_GPS "/android/fix"
#define TOPIC_NAME_PUBLISH_ODOM "/android/pose"
const Eigen::MatrixXd ORIENTATION_PREDICTION_COVARIANCE  = Eigen::MatrixXd::Identity(6,6);
const Eigen::MatrixXd ORIENTATION_OBSERVATION_COVARIANCE = Eigen::MatrixXd::Identity(6,6)*0.00017;
const Eigen::MatrixXd POSE_PREDICTION_COVARIANCE = Eigen::MatrixXd::Identity(9,9);
const Eigen::MatrixXd POSE_OBSERVATION_COVARIANCE = Eigen::MatrixXd::Identity(9,9)*0.00017;



//const double ORIENTATION_PREDICTION_COVARIANCE[36]  = {1, 0, 0, 0, 0, 0,
//                                           0, 1, 0, 0, 0, 0,
//                                           0, 0, 1, 0, 0, 0,
//                                           0, 0, 0, 1, 0, 0,
//                                           0, 0, 0, 0, 1, 0,
//                                           0, 0, 0, 0, 0, 1};
//const double ORIENTATION_OBSERVATION_COVARIANCE[36] = {1, 0, 0, 0, 0, 0,
//                                           0, 1, 0, 0, 0, 0,
//                                           0, 0, 1, 0, 0, 0,
//                                           0, 0, 0, 1, 0, 0,
//                                           0, 0, 0, 0, 1, 0,
//                                           0, 0, 0, 0, 0, 1};
//const double POSE_PREDICTION_COVARIANCE[36]  = {1, 0, 0, 0, 0, 0,
//                                                       0, 1, 0, 0, 0, 0,
//                                                       0, 0, 1, 0, 0, 0,
//                                                       0, 0, 0, 1, 0, 0,
//                                                       0, 0, 0, 0, 1, 0,
//                                                       0, 0, 0, 0, 0, 1};
//const double POSE_OBSERVATION_COVARIANCE[36] = {1, 0, 0, 0, 0, 0,
//                                                       0, 1, 0, 0, 0, 0,
//                                                       0, 0, 1, 0, 0, 0,
//                                                       0, 0, 0, 1, 0, 0,
//                                                       0, 0, 0, 0, 1, 0,
//                                                       0, 0, 0, 0, 0, 1};

class Localizer {

public:

    Localizer();

    ~Localizer();

    void callback_imu(const sensor_msgs::Imu::ConstPtr &msg);
    void callback_magnetic(const sensor_msgs::MagneticField::ConstPtr &msg);
    void callback_gps(const sensor_msgs::NavSatFix::ConstPtr &msg);

    void run();

    void computeOrientation();
    void getOrientation();

    void computePose();

    void switchStates();

private:

    ros::NodeHandle nh;
    ros::NodeHandle private_nh;
    ros::Publisher pub;
    ros::Subscriber sub_imu,sub_magnetic,sub_gps;

    sensor_msgs::Imu imu;
    sensor_msgs::MagneticField magnetic;
    sensor_msgs::NavSatFix gps;
    bool update_imu, update_magnetic, update_gps;

    /** States **/
    std_msgs::Header pose_header;
    geometry_msgs::PoseStamped pose[3];    // [0]: newest, [1]: 1-step before, [2]: 2-step before
    Eigen::Vector3d position[3];
    Eigen::Vector3d rpy[3];
    double time[3];

    Eigen::MatrixXd orientation_prediction_covariance, orientation_observation_covariance;
    Eigen::MatrixXd pose_prediction_covariance, pose_observation_covariance;

    bool first_orientation_computation;
    bool first_pose_computation;

};

Localizer::Localizer() : nh(), private_nh(ros::NodeHandle("~")),
                         first_orientation_computation(true), first_pose_computation(true),
                         orientation_prediction_covariance(ORIENTATION_PREDICTION_COVARIANCE),
                         orientation_observation_covariance(ORIENTATION_OBSERVATION_COVARIANCE),
                         pose_prediction_covariance(POSE_PREDICTION_COVARIANCE),
                         pose_observation_covariance(POSE_OBSERVATION_COVARIANCE),
                         update_imu(false), update_magnetic(false) {
    ROS_DEBUG("%s", __FUNCTION__);

//    /** Set parameter **/
//    for (size_t i=0;i<6;i++){
//        for (size_t j=0;j<6;j++){
//            orientation_prediction_covariance(i,j)=ORIENTATION_PREDICTION_COVARIANCE[i*6+j];
//            orientation_observation_covariance(i,j)=ORIENTATION_OBSERVATION_COVARIANCE[i*6+j];
//        }
//    }

    /** Set publisher **/
    pub = nh.advertise<geometry_msgs::PoseStamped>(TOPIC_NAME_PUBLISH_ODOM, 1);

    /** Set subscriber **/
    sub_imu = nh.subscribe(TOPIC_NAME_IMU, 1, &Localizer::callback_imu, this);
    sub_magnetic = nh.subscribe(TOPIC_NAME_MAGNETIC, 1, &Localizer::callback_magnetic, this);
    sub_gps=nh.subscribe(TOPIC_NAME_GPS, 1, &Localizer::callback_gps,this);

    ros::spin();
}

Localizer::~Localizer() {}

void Localizer::run(){
    switchStates();
    time[0]=pose_header.stamp.toSec();
    getOrientation();
    //    computeOrientation();
    computePose();
    pose[0].header=pose_header;
    pub.publish(pose[0]);
    update_imu = false;
    update_magnetic = false;
    update_gps=false;
}

void Localizer::callback_imu(const sensor_msgs::Imu::ConstPtr &msg) {

    ROS_DEBUG("%s", __FUNCTION__);

    imu = *msg;
    if (update_imu && update_magnetic) {
        pose_header=msg->header;
        run();
    } else {
        update_imu = true;
    }
}

void Localizer::callback_magnetic(const sensor_msgs::MagneticField::ConstPtr &msg) {

    ROS_DEBUG("%s", __FUNCTION__);

    magnetic = *msg;
    if (update_imu && update_magnetic) {
        pose_header=msg->header;
        run();
    } else {
        update_magnetic = true;
    }

}

void Localizer::callback_gps(const sensor_msgs::NavSatFix::ConstPtr &msg) {

    ROS_DEBUG("%s", __FUNCTION__);

    gps = *msg;
    if (update_imu && update_magnetic && update_gps) {
        pose_header = msg->header;
        run();
    }
    else {
        update_gps = true;
    }

}


void Localizer::computeOrientation() {

    ROS_DEBUG("%s", __FUNCTION__);

    /** Computing roll, pitch, yaw by acceleration and magnetic field**/
    Eigen::Vector3d observe_rpy;
    Eigen::Vector3d accel(imu.linear_acceleration.x, imu.linear_acceleration.y, imu.linear_acceleration.z);
    Eigen::Vector3d gyro(imu.angular_velocity.x, imu.angular_velocity.y, imu.angular_velocity.z);
    Eigen::Vector3d magne(magnetic.magnetic_field.x, magnetic.magnetic_field.y, magnetic.magnetic_field.z);
    observe_rpy[0] = atan2(accel(1), accel(2));
    observe_rpy[1] = atan2(-accel(0), sqrt(pow(accel(1), 2) + pow(accel(2), 2)));
    observe_rpy[2] = atan2((magne(2) * sin(observe_rpy[0]) - (magne(1) * cos(observe_rpy[0]))),
                           (magne(0) * cos(observe_rpy[1] + magne(1) * sin(observe_rpy[1]) * sin(observe_rpy[0]) +
                                           magne[2] * sin(observe_rpy[1]) * cos(observe_rpy[0]))));


    /** Kalman filter parameter **/
    static Eigen::VectorXd x(6);
    static Eigen::MatrixXd V(6,6);
    Eigen::MatrixXd Q,R;
    Q=orientation_prediction_covariance;
    R=orientation_observation_covariance;

    if (first_orientation_computation){
        rpy[0]=rpy[1]=rpy[2]=observe_rpy;
        tf::quaternionTFToMsg(
                tf::createQuaternionFromRPY(observe_rpy(0),observe_rpy(1),observe_rpy(2)),pose[2].pose.orientation);
        pose[0].pose.orientation=pose[1].pose.orientation=pose[2].pose.orientation;
        pose[0].header=pose[1].header=pose[2].header=pose_header;

//        tf::Quaternion quat(imu.orientation.x,imu.orientation.y,imu.orientation.z,imu.orientation.w);
//        Eigen::Vector3d imu_pry;
//        tf::Matrix3x3(quat).getRPY(imu_pry[0],imu_pry[1],imu_pry[2]);
//        ROS_DEBUG_STREAM(observe_rpy);
//        ROS_DEBUG_STREAM(imu_pry);

        x<<rpy[0],gyro.reverse();
        V=orientation_observation_covariance;

        pub.publish(pose[0]);
//        ROS_DEBUG_STREAM(current_pose);
//        first_orientation_computation=false;
        return;
    }

    /** Filtering **/

    // Prediction
    {
        Eigen::MatrixXd F=Eigen::MatrixXd::Identity(6,6);
        double a[3]={rpy[1](0),rpy[1](1),rpy[1](2)};
        F.block<3, 3>(0, 3) <<  cos(a[1]), sin(a[0]) * sin(a[1]), cos(a[0]) * sin(a[1]),
                0.0, cos(a[0]) * cos(a[1]), -sin(a[0]) * cos(a[1]),
                0.0, sin(a[0]), cos(a[0]);
        x=F*x;
        V=F*V*F.transpose()+Q;
    }
    // Filtering
    {
        Eigen::VectorXd y(6);
        y<<observe_rpy,gyro.reverse();
        Eigen::MatrixXd tmp=V+R;
        Eigen::MatrixXd K=V*tmp.inverse();
        x=x+K*(y-x);
        V=V-K*V;
    }

    /** Update current values **/
    rpy[0]=x.head(3);
    tf::quaternionTFToMsg(
            tf::createQuaternionFromRPY(x(0),x(1),x(2)),pose[0].pose.orientation);

    /** Console out **/
    tf::Quaternion quat(imu.orientation.x,imu.orientation.y,imu.orientation.z,imu.orientation.w);
    Eigen::Vector3d imu_pry;
    tf::Matrix3x3(quat).getRPY(imu_pry[0],imu_pry[1],imu_pry[2]);
//    ROS_DEBUG_STREAM(observe_rpy);
//    ROS_DEBUG_STREAM(imu_pry);
//
//    ROS_DEBUG_STREAM(pose[0]);

}

void Localizer::getOrientation() {

    ROS_DEBUG("%s", __FUNCTION__);

    /** Update current values **/
    tf::Quaternion quat(imu.orientation.x,imu.orientation.y,imu.orientation.z,imu.orientation.w);
    tf::Matrix3x3(quat).getRPY(rpy[0](0),rpy[0](1),rpy[0](2));

    Eigen::Vector3d mag(magnetic.magnetic_field.x, magnetic.magnetic_field.y, magnetic.magnetic_field.z);

    rpy[0](2) = atan2((mag(2) * sin(rpy[0](0)) - (mag(1) * cos(rpy[0](0)))),
                (mag(0) * cos(rpy[0](1) + mag(1) * sin(rpy[0](1)) * sin(rpy[0](0)) +
                mag[2] * sin(rpy[0](1)) * cos(rpy[0](0)))));

    tf::quaternionTFToMsg(tf::createQuaternionFromRPY(rpy[0](0),rpy[0](1),rpy[0](2)),pose[0].pose.orientation);

    if(first_orientation_computation){
        rpy[1]=rpy[2]=rpy[0];
        pose[1].pose.orientation=pose[2].pose.orientation=pose[0].pose.orientation;
        pose[0].header=pose[1].header=pose[2].header=pose_header;
    }

}


void Localizer::computePose() {

    ROS_DEBUG("%s", __FUNCTION__);

    Eigen::Vector3d accel(imu.linear_acceleration.x,imu.linear_acceleration.y,imu.linear_acceleration.z+ACCELERATION_Z_BIAS);
    Eigen::Matrix3d matrix_dr,matrix_dp,matrix_dy;
    matrix_dr=Eigen::AngleAxisd(rpy[0](0),Eigen::Vector3d::UnitX());
    matrix_dp=Eigen::AngleAxisd(rpy[0](1),Eigen::Vector3d::UnitY());
    matrix_dy=Eigen::AngleAxisd(rpy[0](2),Eigen::Vector3d::UnitZ());
    Eigen::Vector3d acceleration=-(matrix_dy*matrix_dp*matrix_dr*accel-Eigen::Vector3d(0,0,GRAVITATIONAL_ACCELERATION));

    ROS_DEBUG_STREAM("accel\n"<<accel);
    ROS_DEBUG_STREAM("rpy\n"<<rpy[0]);
    ROS_DEBUG_STREAM("acceleration\n"<<acceleration);//    computeOrientation();



    static Eigen::VectorXd x(9);
    static Eigen::MatrixXd V(9,9);


    /** First pose processing **/
    if (first_pose_computation){

        x<<Eigen::VectorXd::Zero(6),acceleration;
        V=pose_observation_covariance;

        first_pose_computation=false;
        return;
    }

    /** Filtering **/

    Eigen::MatrixXd Q,R;
    Q=pose_prediction_covariance;
    R=pose_observation_covariance;

    //// Prediction
    {
        double dt01=time[0]-time[1];
        Eigen::MatrixXd F=Eigen::MatrixXd::Identity(9,9);
        F.block<3,3>(0,6)=F.block<3,3>(0,6)+Eigen::MatrixXd::Identity(3,3)*pow(dt01,2)/2.0;
        F.block<6, 6>(0, 3) = F.block<6,6>(0,3)+Eigen::MatrixXd::Identity(6,6)*dt01;
        x=F*x;
        V=F*V*F.transpose()+Q;
    }

    //// Filtering
    if (update_gps){
        //// To Do
    }
    else{
        Eigen::VectorXd y(9);

        double dt12=time[1]-time[2];
        Eigen::Vector3d dp12=position[1]-position[2];
        Eigen::Vector3d speed;
        if(dt12!=0){
            speed=dp12/dt12;
        }
        else{
            speed=Eigen::Vector3d::Zero();
        }

        y<<x.head(3),speed,acceleration;
        Eigen::MatrixXd tmp=V+R;
        Eigen::MatrixXd K=V*tmp.inverse();
        x=x+K*(y-x);
        V=V-K*V;
    }


    /** Update current values **/
    position[0]=x.head(3);
    pose[0].pose.position.x=x(0);
    pose[0].pose.position.y=x(1);
    pose[0].pose.position.z=x(2);


    /** Console out **/
    ROS_DEBUG_STREAM(x);
    ROS_INFO_STREAM(pose[0]);

}

void Localizer::switchStates() {
    pose[2]=pose[1];
    pose[1]=pose[0];
    position[2]=position[1];
    position[1]=position[0];
    rpy[2]=rpy[1];
    rpy[1]=rpy[0];
    time[2]=time[1];
    time[1]=time[0];
}


// -----------------------------------
//	Main Function
// -----------------------------------
int main(int argc, char **argv) {

//    // debug mode
//    if (ros::console::set_logger_level(ROSCONSOLE_DEFAULT_NAME, ros::console::levels::Debug)) {
//        ros::console::notifyLoggerLevelsChanged();
//    }

    // particleInitialization ROS
    ros::init(argc, argv, "localizer_node");
    Localizer node;
    ros::spin();

    return (0);
}
