//
// Created by ohashi on 19/09/13.
//

#include <math.h>

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <limits>

#include <Eigen/Core>
#include <Eigen/LU>
#include <Eigen/Geometry>

#include <ros/ros.h>
#include <ros/console.h>

#include <sensor_msgs/Imu.h>
#include <sensor_msgs/MagneticField.h>
#include <sensor_msgs/NavSatFix.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/PoseArray.h>
#include <nav_msgs/Odometry.h>
#include <tf/transform_broadcaster.h>
#include <tf2_ros/transform_listener.h>
//#include <tf2/convert.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>


#include <gnss/geo_pos_conv.hpp>
#include "../../../../../devel/include/autoware_msgs/DetectedObject.h"
#include "../../../../../devel/include/autoware_msgs/DetectedObjectArray.h"

#include <ParticleFilter.h>
#include <State.h>

#define USE_IMU true
#define USE_MAGNETIC false
#define USE_GPS true
#define USE_VEHICLE_PERCEPTION true

#define ORIGIN_FRAME_ID "odom"
#define PHONE_FRAME_ID "imu"
#define PERSON_FRAME_ID "person"

#define ORIGIN_FIX "37.9181551873,-122.331354939,0.0"
#define PHONE_INITIAL_FIX "37.9180604745,-122.331348873,0.0"
#define PERSON_INITIAL_DERECTION "-0.580381653" // radian from north, opposite clock direction
//#define PHONE_INITIAL_QUATERNION "0.05702500453,0.253443002701,0.902796983719,0.342742472887"

#define ACCELERATION_GROUND_BIAS "-0.03,0.01,0.0"
#define ACCELERATION_X_BIAS -0.03
#define ACCELERATION_Y_BIAS 0.01

#define GYRO_Z_BIAS -0.00
#define PARTICLEFILTER_INITIAL_XY_RADIUS 2      // [m]
#define PARTICLEFILTER_INITIAL_R_RADIUS 3      // [deg]
#define PARTICLEFITLER_INITIAL_V_RADIUS 0.01    // [m/s]

#define PARTICLEFILTER_PARTICLE_SIZE 1000
#define PARTICLEFILTER_SYSTEM_NOISE_STANDARD_DEVIATION "0.2,0.2,0.087263889,0.00001,0.1"  // x,y,r,vx,vy

#define PARTICLEFILTER_ACCELERATION_STANDARD_DEVIATION 0.17    //
#define PARTICLEFILTER_GYRO_STANDARD_DEVIATION 0.0001            // [rad/s]
#define PARTICLEFILTER_IMU_YAW_STANDARD_DEVIATION 0.01*M_PI/180             // [rad]
#define PARTICLEFILTER_MAGNETIC_YAW_STANDARD_DEVIATION 5*M_PI/180 //

#define PARTICLEFILTER_GPS_XY_BIAS 5                      //
#define PARTICLEFILTER_GPS_V_BIAS 0                      //
#define PARTICLEFILTER_GPS_XY_STANDARD_DEVIATION 0.5                      //
#define PARTICLEFILTER_GPS_V_STANDARD_DEVIATION 0.2                      //
#define PARTICLEFILTER_GPS_R_STANDARD_DEVIATION 5*M_PI/180                      //

#define PARTICLEFILTER_PERCEPTION_XY_STANDARD_DEVIATION 0.3                      //
#define PARTICLEFILTER_PERCEPTION_V_STANDARD_DEVIATION 0.01                      //
#define PARTICLEFILTER_PERCEPTION_R_STANDARD_DEVIATION 2.0*M_PI/180                      //
#define PARTICLEFILTER_PERCEPTION_V_TH 1.0                      // 5.0[m/s]


#define TOPIC_NAME_SUB_IMU "/android/imu"
#define TOPIC_NAME_MAGNETIC "/android/magnetic_field"
#define TOPIC_NAME_GPS "/android/fix"
#define TOPIC_NAMES_MKZ_PERCEPTION "/detection/object_tracker/objects"
#define TOPIC_NAME_PUBLISH_ODOM "/phone_localizer/odom"
#define TOPIC_NAME_PUBLISH_POSEARRAY "phone_localizer/particles"


class Localizer : public ParticleFilter<State<>> {

public:

    Localizer();

    ~Localizer();

    void callback_imu(const sensor_msgs::Imu::ConstPtr &msg);
    void callback_magnetic(const sensor_msgs::MagneticField::ConstPtr &msg);
    void callback_gps(const sensor_msgs::NavSatFix::ConstPtr &msg);
    void callback_vehiclePerception(const autoware_msgs::DetectedObjectArrayConstPtr &msg);

    void publish_odom();
    void publish_poseaary();

    void run();

    void particleInitialization();
    void particleTransition();
    void particleLikelihoodIMU();
    void particleLikelihoodMagnetic();
    void particleLikelihoodGPS();
    void particleLikelihoodGpsXY();
    void particleLikelihoodGpsV();
    void particleLikelihoodGpsR();
    bool getPedestrianFromPerception();
    void particleLikelihoodVehiclePerceptionXY();
    void particleLikelihoodVehiclePerceptionV();
    void particleLikelihoodVehiclePerceptionR();


private:

    /** ROS **/
    ros::NodeHandle nh;
    ros::NodeHandle private_nh;
    ros::Publisher pub_odom,pub_particles;
    ros::Subscriber sub_imu,sub_magnetic,sub_gps,sub_vehicle_perception;

    /** Topic **/
    //// msgs
    sensor_msgs::Imu imu;
    sensor_msgs::MagneticField magnetic;
    sensor_msgs::NavSatFix gps;
    autoware_msgs::DetectedObjectArray vehicle_perception;
    autoware_msgs::DetectedObject person_perception_object;
    State<> person_perception_origin_state;

    //// Frame ID
    std::string origin_frame_id,phone_frame_id,person_frame_id;

    //// Others
    ros::Time time;
    bool update_imu, update_magnetic, update_gps,update_vehicle_perception;

    /** TF Listener **/
    tf2_ros::Buffer tf_buffer;

    /** Current Status **/
    nav_msgs::Odometry odom;
    geometry_msgs::PoseArray posearray;
    Eigen::Matrix3f phone2person,phone2ground;

    /** GPS fix -> pose **/
    geo_pos_conv geo;
    double origin_lat,origin_lon,origin_ele;


    /** Particle Filter Tracker **/
    State<> person_initial_state;
    int particle_size;
    State<> now_person_state;

    /** Random Seed **/
    std::random_device rnd;     // 非決定的な乱数生成器を生成
    std::mt19937_64 mt;     //  メルセンヌ・ツイスタの64ビット版、引数は初期シード値

    bool first_orientation_computation;
    bool first_pose_computation;

};

Localizer::Localizer() : nh(), private_nh(ros::NodeHandle("~")),
                         first_orientation_computation(true), first_pose_computation(true),
                         update_imu(false), update_magnetic(false),update_gps(false),
                         particle_size(PARTICLEFILTER_PARTICLE_SIZE){

    ROS_DEBUG("%s", __FUNCTION__);

    /** Set parameter **/

    //// origin
    std::string str;
    if (!nh.getParam("origin_fix",str)){
        str=ORIGIN_FIX;
    }
    sscanf(str.c_str(),"%lf,%lf,%lf",&origin_lat,&origin_lon,&origin_ele);
    geo.set_plane(origin_lat,origin_lon);

    tf2_ros::TransformListener tf_listener(tf_buffer);

    //// random seed
    mt.seed(rnd());

    /** Set publisher **/
    //// Odom
    pub_odom = nh.advertise<nav_msgs::Odometry>(TOPIC_NAME_PUBLISH_ODOM, 1);
    if (!nh.getParam("origin_frame_id",str)){
        str=ORIGIN_FRAME_ID;
    }
    odom.header.frame_id=origin_frame_id=str;
    if (!nh.getParam("phone_frame_id",str)){
        str=PHONE_FRAME_ID;
    }
    phone_frame_id=str;
    if (!nh.getParam("person_frame_id",str)){
        str=PERSON_FRAME_ID;
    }
    odom.child_frame_id=person_frame_id=str;
    //// Particles
    pub_particles=nh.advertise<geometry_msgs::PoseArray>(TOPIC_NAME_PUBLISH_POSEARRAY,1);
    posearray.header.frame_id=origin_frame_id;

    /** Initialize ParticleFilter **/
    particleInitialization();

    /** Set subscriber **/
    bool use_imu,use_mag,use_gps,use_vehicle_perception;
    if (!private_nh.getParam("use_imu",use_imu)){
        use_imu=USE_IMU;
    }
    if (!private_nh.getParam("use_magnetic",use_mag)){
        use_mag=USE_MAGNETIC;
    }
    if (!private_nh.getParam("use_gps",use_gps)){
        use_gps=USE_GPS;
    }
    if (!private_nh.getParam("use_vehicle_perception",use_vehicle_perception)){
        use_vehicle_perception=USE_VEHICLE_PERCEPTION;
    }
    if(use_imu){
        sub_imu = nh.subscribe(TOPIC_NAME_SUB_IMU, 1, &Localizer::callback_imu, this);
    }
    if(use_mag){
        sub_magnetic = nh.subscribe(TOPIC_NAME_MAGNETIC, 1, &Localizer::callback_magnetic, this);
    }
    if(use_gps){
        sub_gps=nh.subscribe(TOPIC_NAME_GPS, 1, &Localizer::callback_gps,this);
    }
    if(use_vehicle_perception){
        sub_vehicle_perception=nh.subscribe(TOPIC_NAMES_MKZ_PERCEPTION,1,&Localizer::callback_vehiclePerception,this);
    }

    ros::spin();
}

Localizer::~Localizer() {}

void Localizer::callback_imu(const sensor_msgs::Imu::ConstPtr &msg) {

    ROS_DEBUG("%s", __FUNCTION__);

    imu = *msg;
    time=msg->header.stamp;
    update_imu = true;
    run();
}

void Localizer::callback_magnetic(const sensor_msgs::MagneticField::ConstPtr &msg) {

    ROS_DEBUG("%s", __FUNCTION__);

    magnetic = *msg;
    time=msg->header.stamp;
    update_magnetic = true;
    run();
}

void Localizer::callback_gps(const sensor_msgs::NavSatFix::ConstPtr &msg) {

    ROS_DEBUG("%s", __FUNCTION__);

    gps = *msg;
    time = msg->header.stamp;
    update_gps = true;
    run();
}

void Localizer::callback_vehiclePerception(const autoware_msgs::DetectedObjectArrayConstPtr &msg) {

    ROS_DEBUG("%s", __FUNCTION__);

    vehicle_perception = *msg;
    time = msg->header.stamp;
    update_vehicle_perception = true;
    run();
}

void Localizer::publish_odom() {

    ROS_DEBUG("%s", __FUNCTION__);

    odom.pose.pose.position.x=now_person_state.x;
    odom.pose.pose.position.y=now_person_state.y;
    odom.pose.pose.position.z=0.0;
    tf::Quaternion quat=tf::createQuaternionFromRPY(0.0,0.0,now_person_state.r);
    quaternionTFToMsg(quat,odom.pose.pose.orientation);
    odom.twist.twist.linear.x=now_person_state.vx;
    odom.twist.twist.linear.y=now_person_state.vy;

    odom.header.stamp=time;

    pub_odom.publish(odom);

    ROS_DEBUG_STREAM("Person State \n" << now_person_state);
}

void Localizer::publish_poseaary() {

    ROS_DEBUG("%s", __FUNCTION__);

    posearray.poses.clear();

    for (auto particle:getParticles()){
        State<> s=*particle->getState();
        geometry_msgs::Pose pose;
        pose.position.x=s.x;
        pose.position.y=s.y;
        pose.position.z=0.0;
        tf::Quaternion q=tf::createQuaternionFromRPY(0.0,0.0,s.r);
        quaternionTFToMsg(q,pose.orientation);
        posearray.poses.push_back(pose);
    }
    posearray.header.stamp=time;

    pub_particles.publish(posearray);

    static int i=0;
    if (i%100==0){
        ROS_DEBUG_STREAM("Particle States \n"<<getParticles());
    }
    i++;

}

void Localizer::run(){

    ROS_DEBUG("%s", __FUNCTION__);

    for(auto p:getParticles()){
        p->getState()->normalize_r();
    }

    if (update_imu) {
        particleTransition();
//        particleLikelihoodIMU();
        update_imu=false;
    }

    if (update_magnetic){
        particleLikelihoodMagnetic();
        update_magnetic=false;
    }

    if (update_gps){
        particleLikelihoodGPS();
        particleLikelihoodGpsXY();
        particleLikelihoodGpsV();
//        particleLikelihoodGpsR();
        update_gps=false;
    }

    if (update_vehicle_perception){
        if(getPedestrianFromPerception()){
            particleLikelihoodVehiclePerceptionXY();
            particleLikelihoodVehiclePerceptionV();
            particleLikelihoodVehiclePerceptionR();
        }
        update_vehicle_perception=false;
    }

    //// Neff check
    if (isParticleReample()){
        ROS_INFO("Resampling!");
        particleResample();

        //// add system noise
        std::string str;
        if (!private_nh.getParam("particlefilter_system_noise_standard_deviation",str)){
            str=PARTICLEFILTER_SYSTEM_NOISE_STANDARD_DEVIATION;
        }
        State<> sd;
        sscanf(str.c_str(),"%lf,%lf,%lf,%lf,%lf",&sd.x,&sd.y,&sd.r,&sd.vx,&sd.vy);
        particleAddSystemNoise(sd);
    }


    /** Publish Current Status **/

    //// Odom
    now_person_state=getWeightedMean();
    publish_odom();

    //// Particles
    publish_poseaary();

}

void Localizer::particleInitialization() {

    ROS_DEBUG("%s", __FUNCTION__);

    getParticles().clear();

    //// Preparing person initial parameter
    double person_initial_lat,person_initial_lon,person_initial_ele;
    std::string str;
    if (!private_nh.getParam("person_initial_fix",str)){
        str=PHONE_INITIAL_FIX;
    }
    sscanf(str.c_str(),"%lf,%lf,%lf",&person_initial_lat,&person_initial_lon,&person_initial_ele);
    if (!private_nh.getParam("person_initial_direction",str)){
        str=PERSON_INITIAL_DERECTION;
    }
    double person_initial_direction=std::stod(str);
//    if (!nh.getParam("phone_initial_quaternion",str)){
//        str=PHONE_INITIAL_QUATERNION;
//    }
//    double tmp[4],rpy[3];
//    sscanf(str.c_str(),"%lf,%lf,%lf,%lf",&tmp[0],&tmp[1],&tmp[2],&tmp[3]);
//    tf::Quaternion quat(tmp[0],tmp[1],tmp[2],tmp[3]),quat_person;
//    tf::Matrix3x3(quat).getRPY(rpy[0], rpy[1], rpy[2]);
//    tf_listener.transformQuaternion(phone_frame_id,ros::Time::now(),tmp,person_frame_id,quat_person);
//    tf::Matrix3x3(quat_person).getRPY(rpy[0], rpy[1], rpy[2]);

    //// Setting person initial state
    geo.llh_to_xyz(person_initial_lat,person_initial_lon, 0);
    person_initial_state.x=geo.x();
    person_initial_state.y=geo.y();
//    person_initial_state.r=rpy[2];
    person_initial_state.r=person_initial_direction;
    person_initial_state.vx=0.0;
    person_initial_state.vy=0.0;
    now_person_state=person_initial_state;


    //// Setting particles sampling range
    double xy_radius,r_radius,v_radius;
    if (!private_nh.getParam("particlefilter_initial_xy_radius",xy_radius)){
        xy_radius=PARTICLEFILTER_INITIAL_XY_RADIUS;
    }
    if (!private_nh.getParam("particlefilter_initial_r_radius",r_radius)){
        r_radius=PARTICLEFILTER_INITIAL_R_RADIUS;
    }
    if (!private_nh.getParam("particlefilter_initial_v_radius",v_radius)){
        v_radius=PARTICLEFITLER_INITIAL_V_RADIUS;
    }
    std::uniform_real_distribution<double> rand_xy(-xy_radius,xy_radius);
    std::uniform_real_distribution<double> rand_theta(-M_PI,M_PI);
    std::uniform_real_distribution<double> rand_r(-r_radius*M_PI/180,r_radius*M_PI/180);
    std::uniform_real_distribution<double> rand_v(-v_radius,v_radius);


    //// Sampling particles
    if (!private_nh.getParam("particlefilter_particle_size",particle_size)){
        particle_size=PARTICLEFILTER_PARTICLE_SIZE;
    }
    for(size_t i=0;i<particle_size;i++){
        Particle<State<>>* p = new Particle<State<>>;
        State<>* s=new State<>;
        double rand_x=rand_xy(mt);
        double rand_y=rand_xy(mt);
        double rand_vx=rand_v(mt);
        double rand_vy=rand_v(mt);
        while(rand_x*rand_x+rand_y*rand_y>xy_radius*xy_radius){
            rand_x=rand_xy(mt);
            rand_y=rand_xy(mt);
        }
        while(rand_vx*rand_vx+rand_vy*rand_vy>v_radius*v_radius){
            rand_vx=rand_v(mt);
            rand_vy=rand_v(mt);
        }

        double x=rand_x+person_initial_state.x;
        double y=rand_y+person_initial_state.y;
        double r=rand_r(mt)+person_initial_state.r;
//        double vx=rand_vx+person_initial_state.vx;
        double vx=0.0;
        double vy=rand_vy+person_initial_state.vy;

        s->set(x,y,r,vx,vy);
        p->setState(s);
        p->setWeight(1.0);
        getParticles().push_back(p);
    }

    particleNormalize();

    time=ros::Time::now();

    publish_odom();
    publish_poseaary();

}

void Localizer::particleTransition() {

    ROS_DEBUG("%s", __FUNCTION__);

    static bool init=true;
    static std::vector<double> now_imu_rpy(3),pre_imu_rpy(3);
    static double now_time,pre_time;

    /** Cordinate transformation from phone to world **/
    std::vector<double> imu_rpy(3);
    tf::Quaternion quat(imu.orientation.x, imu.orientation.y, imu.orientation.z, imu.orientation.w);
    tf::Matrix3x3(quat).getRPY(imu_rpy[0], imu_rpy[1], imu_rpy[2]);
    Eigen::Vector3f axis_dr(1,0,0),axis_dp(0,1,0),axis_dy(0,0,1);
    Eigen::Matrix3f matrix_dr,matrix_dp,matrix_dy;
    matrix_dr=Eigen::AngleAxisf(imu_rpy[0],axis_dr);
    matrix_dp=Eigen::AngleAxisf(imu_rpy[1],axis_dp);
    matrix_dy=Eigen::AngleAxisf(now_person_state.r,axis_dy);
    phone2person=matrix_dp*matrix_dr;
    phone2ground=matrix_dy*phone2person;

    //// If initial, set previous time and skip calculation
    if (init){
        now_imu_rpy=pre_imu_rpy=imu_rpy;
        now_time=pre_time=imu.header.stamp.toSec();
        init=false;
        return;
    }

    pre_imu_rpy=now_imu_rpy;
    pre_time=now_time;
    now_imu_rpy=imu_rpy;
    now_time=imu.header.stamp.toSec();
    double d_t=now_time-pre_time;


    /** Gyro and acceleration **/
//    //// acceleration
    double axb,ayb,gzb;
//    if (!nh.getParam("acceleration_x_bias",axb)){
//        axb=ACCELERATION_X_BIAS;
//    }
//    if (!nh.getParam("acceleration_y_bias",ayb)){
//        ayb=ACCELERATION_Y_BIAS;
//    }
//    Eigen::Vector3f accel(imu.linear_acceleration.x,imu.linear_acceleration.y,imu.linear_acceleration.z);
//    Eigen::Vector3f person_accel=-1*phone2person*accel-Eigen::Vector3f(axb,ayb,0.0);   // 座標系ではカメラ反対方向を正で定義しているにもかかわらず，カメラ方向を正で出力している
    //// gyro
    if (!private_nh.getParam("gyro_z_bias",gzb)){
        gzb=GYRO_Z_BIAS;
    }
    double diff_person_yaw=State<>::normalize_radian(now_imu_rpy[2]-pre_imu_rpy[2]);
//    std::cout<<now_imu_rpy[2]<<","<<pre_imu_rpy[2]<<","<<diff_person_yaw<<std::endl;


    /** Particle Transition **/
    double asd,gsd;
//    if (!nh.getParam("particlefilter_acceleration_standard_deviation",asd)){
//        asd=PARTICLEFILTER_ACCELERATION_STANDARD_DEVIATION;
//    }
    if (!private_nh.getParam("particlefilter_gyro_standard_deviation",gsd)){
        gsd=PARTICLEFILTER_GYRO_STANDARD_DEVIATION;
    }

//    std::normal_distribution<double> rand_accel(0,asd);
    std::normal_distribution<double> rand_gyro(0,gsd);
    for (auto particle:getParticles()){

        State<> s=*particle->getState();

        //// Adding transition noise
//        double ax=person_accel(0)+rand_accel(mt);
//        double ay=person_accel(1)+rand_accel(mt);
//        double gz=diff_person_yaw+rand_gyro(mt);

        //// Axis conversion
        Eigen::Matrix3f person2ground_p;
        person2ground_p=Eigen::AngleAxisf(s.r,axis_dy);
        Eigen::Vector3f ground_velocity=person2ground_p*Eigen::Vector3f(s.vx,s.vy,0.0);
//        Eigen::Vector3f ground_accel=person2ground_p*Eigen::Vector3f(ax,ay,0.0);

        //// Transition
//        s.x=s.x+ground_velocity(0)*d_t+ground_accel(0)*d_t*d_t/2.0;
//        s.y=s.y+ground_velocity(1)*d_t+ground_accel(1)*d_t*d_t/2.0;
        s.x=s.x+ground_velocity(0)*d_t;
        s.y=s.y+ground_velocity(1)*d_t;
        s.vx=s.vx;
        s.vy=s.vy;
        s.r=s.r+diff_person_yaw+rand_gyro(mt)*d_t;
//        s.r=s.r+gz*d_t;

        *particle->getState()=s;
    }

    std::cout<<*getParticles()[0]->getState()<<std::endl;

//    static double sum[4]={0.0,0.0,0.0,0.0};
//    static double num=0.0;
//    sum[0]+=person_accel(0);
//    sum[1]+=person_accel(1);
//    sum[2]+=person_accel(2);
//    sum[3]+=person_gyro(2);

//    //// Debug
//    Eigen::Vector3f ground_accel=phone2ground*accel;
//    ROS_DEBUG_STREAM(
//            "Values in "<< __FUNCTION__ <<"\n"<<
//                        "d_t: " <<d_t<<
//                        "\t accel(0): "<<ground_accel(0) <<
//                        "\t accel(1): "<<ground_accel(1) <<
//                        "\t accel(2): "<<ground_accel(2));

}

void Localizer::particleLikelihoodIMU(){

    ROS_DEBUG("%s", __FUNCTION__);

    double rpy[3];
    tf::Quaternion quat(imu.orientation.x, imu.orientation.y, imu.orientation.z, imu.orientation.w);
    tf::Matrix3x3(quat).getRPY(rpy[0], rpy[1], rpy[2]);

    double sd;
    if (!private_nh.getParam("particlefitler_imu_yaw_standard_deviation",sd)){
        sd=PARTICLEFILTER_IMU_YAW_STANDARD_DEVIATION;
    }

    for(auto particle:getParticles()){
        double r=particle->getState()->r-rpy[2];
        r=State<>::normalize_radian(r);
        double w=std::exp(-r*r/(2*sd*sd));
        w*=particle->getWeight();
        particle->setWeight(w);
    }

//    std::cout << "Particle States \n" << getParticles() << std::endl;
    particleNormalize();

}

void Localizer::particleLikelihoodMagnetic() {

    ROS_DEBUG("%s", __FUNCTION__);

    ROS_WARN("Non implementation!");

//    Eigen::Vector3f mag(magnetic.magnetic_field.x, magnetic.magnetic_field.y, magnetic.magnetic_field.z);
//    Eigen::Vector3f mag_person=phone2person*mag;
//    double yaw_mag=std::atan2(mag_person(1),mag_person(0));    // if only atan2(), degree is from west.
//    std::cout<<yaw_mag*180/M_PI<<std::endl;
//    double tmp[3];
//    tf::Quaternion quat(imu.orientation.x, imu.orientation.y, imu.orientation.z, imu.orientation.w);
//    tf::Matrix3x3(quat).getRPY(tmp[0], tmp[1], tmp[2]);
//    std::cout<<-tmp[2]*180/M_PI<<std::endl;  // acceleration同様に逆になっている
//
//    return;
//
//    double var;
//    if (!private_nh.getParam("particlefitler_magnetic_yaw_variance",var)){
//        var=PARTICLEFILTER_MAGNETIC_YAW_STANDARD_DEVIATION;
//    }
//
//    for(auto particle:getParticles()){
//        double pr=particle->getState()->r;
//        double w=std::exp(-std::pow(pr-yaw_mag,2)/(2*var));
//        w*=particle->getWeight();
//        particle->setWeight(w);
//    }
//
//    std::cout << "Particle States \n" << getParticles() << std::endl;
//    particleNormalize();

}

void Localizer::particleLikelihoodGPS() {

    ROS_DEBUG("%s", __FUNCTION__);

    geo.llh_to_xyz(gps.latitude,gps.longitude, gps.altitude);

    static bool init=true;
    static Eigen::Vector3d now_gps,pre_gps;
    static double now_time,pre_time;
    if (init){
        now_gps=pre_gps=Eigen::Vector3d(geo.x(),geo.y(),geo.z());
        now_time=pre_time=time.toSec();
        init=false;
        return;
    }
    pre_gps=now_gps;
    pre_time=now_time;
    now_gps=Eigen::Vector3d(geo.x(),geo.y(),geo.z());
    now_time=time.toSec();

    double d_t=now_time-pre_time;
    double v_x=(now_gps(0)-pre_gps(0))/d_t;
    double v_y=(now_gps(1)-pre_gps(1))/d_t;

    double xy_bias,v_bias,xy_sd,v_sd;
    if (!private_nh.getParam("particlefitler_gps_xy_bias",xy_bias)){
        xy_bias=PARTICLEFILTER_GPS_XY_BIAS;
    }
    if (!private_nh.getParam("particlefitler_gps_v_bias",v_bias)){
        v_bias=PARTICLEFILTER_GPS_V_BIAS;
    }
    if (!private_nh.getParam("particlefitler_gps_xy_standard_deviation",xy_sd)){
        xy_sd=PARTICLEFILTER_GPS_XY_STANDARD_DEVIATION;
    }
    if (!private_nh.getParam("particlefitler_gps_v_standard_deviation",v_sd)){
        v_sd=PARTICLEFILTER_GPS_V_STANDARD_DEVIATION;
    }


    /** Compute Likelihood **/
    std::vector<double> w_xy_vec,w_v_vec;
    for (auto particle:getParticles()){
        State<> s=*particle->getState();

        //// Likelihood XY
        double r_xy=std::sqrt(std::pow(s.x-now_gps(0),2)+std::pow(s.y-now_gps(1),2));
        double w_xy;
        if (r_xy<=xy_bias){
            w_xy=1.0;
        }
        else{
            w_xy=std::exp(-std::pow(r_xy-xy_bias,2)/(2*xy_sd*xy_sd));
        }
        w_xy_vec.push_back(w_xy);

        //// Axis conversion
        Eigen::Vector3f axis_dy(0,0,1);
        Eigen::Matrix3f person2ground_p;
        person2ground_p=Eigen::AngleAxisf(s.r,axis_dy);
        Eigen::Vector3f ground_velocity=person2ground_p*Eigen::Vector3f(s.vx,s.vy,0.0);

        //// Likelihood V
        double w_v;
        double r_v=std::sqrt(std::pow(ground_velocity(0)-v_x,2)+std::pow(ground_velocity(1)-v_y,2));
//        std::cout<<r_v<<std::endl;
        if (r_v<v_bias){
            w_v=1.0;
        }
        else{
            w_v=std::exp(-std::pow(r_v-v_bias,2)/(2*v_sd*v_sd));
        }
        w_v_vec.push_back(w_v);

    }

    double w_xy_sum=std::accumulate(w_xy_vec.begin(),w_xy_vec.end(),0.0);
    double w_v_sum=std::accumulate(w_v_vec.begin(),w_v_vec.end(),0.0);
    for(size_t i=0;i<getParticles().size();i++){
        double w=getParticles()[i]->getWeight();
        if(w_xy_sum!=0.0){
            w*=w_xy_vec[i];
        }
        if(w_v_sum!=0.0){
            w*=w_v_vec[i];
        }
        getParticles()[i]->setWeight(w);
    }

//    std::cout << "Particle States \n" << getParticles() << std::endl;
    particleNormalize();

}

void Localizer::particleLikelihoodGpsXY() {

    ROS_DEBUG("%s", __FUNCTION__);

    geo.llh_to_xyz(gps.latitude,gps.longitude, gps.altitude);

    double xy_bias,xy_sd;
    if (!private_nh.getParam("particlefitler_gps_xy_bias",xy_bias)){
        xy_bias=PARTICLEFILTER_GPS_XY_BIAS;
    }
    if (!private_nh.getParam("particlefitler_gps_xy_standard_deviation",xy_sd)){
        xy_sd=PARTICLEFILTER_GPS_XY_STANDARD_DEVIATION;
    }

    /** Compute Likelihood **/
    std::vector<double> w_vec;
    for (auto particle:getParticles()) {
        State<> s = *particle->getState();

        //// Likelihood XY
        double r_xy = std::sqrt(std::pow(s.x - geo.x(), 2) + std::pow(s.y - geo.y(), 2));
        double w;
        if (r_xy <= xy_bias) {
            w = 1.0;
        } else {
            w = std::exp(-std::pow(r_xy - xy_bias, 2) / (2 * xy_sd * xy_sd));
        }

        w_vec.push_back(w);
    }

    if(!std::accumulate(w_vec.begin(),w_vec.end(),0.0)==0.0){
        for(size_t i=0;i<getParticles().size();i++){
            double w=w_vec[i]*getParticles()[i]->getWeight();
            getParticles()[i]->setWeight(w);
        }
    }


//    std::cout << "Particle States \n" << getParticles() << std::endl;
    particleNormalize();

}

void Localizer::particleLikelihoodGpsV() {

    ROS_DEBUG("%s", __FUNCTION__);

    geo.llh_to_xyz(gps.latitude,gps.longitude, gps.altitude);

    static bool init=true;
    static Eigen::Vector3d now_gps,pre_gps;
    static double now_time,pre_time;
    if (init){
        now_gps=pre_gps=Eigen::Vector3d(geo.x(),geo.y(),geo.z());
        now_time=pre_time=time.toSec();
        init=false;
        return;
    }
    pre_gps=now_gps;
    pre_time=now_time;
    now_gps=Eigen::Vector3d(geo.x(),geo.y(),geo.z());
    now_time=time.toSec();

    double dx=now_gps(0)-pre_gps(0);
    double dy=now_gps(1)-pre_gps(1);
    double d_t=now_time-pre_time;
    double gps_v=std::sqrt(dx*dx+dy*dy)/d_t;

    double v_bias,v_sd;
    if (!private_nh.getParam("particlefitler_gps_v_bias",v_bias)){
        v_bias=PARTICLEFILTER_GPS_V_BIAS;
    }
    if (!private_nh.getParam("particlefitler_gps_v_standard_deviation",v_sd)){
        v_sd=PARTICLEFILTER_GPS_V_STANDARD_DEVIATION;
    }

    /** Compute Likelihood **/
    std::vector<double> w_vec;
    for (auto particle:getParticles()){
        State<> s=*particle->getState();

        //// Likelihood V
        double w;
        double s_v=std::sqrt(s.vx*s.vx+s.vy*s.vy);
        double r_v=std::abs(s_v-gps_v);
        if (r_v<v_bias){
            w=1.0;
        }
        else{
            w=std::exp(-std::pow(r_v-v_bias,2)/(2*v_sd*v_sd));
        }

        w_vec.push_back(w);

    }

    if(!std::accumulate(w_vec.begin(),w_vec.end(),0.0)==0.0){
        for(size_t i=0;i<getParticles().size();i++){
            double w=w_vec[i]*getParticles()[i]->getWeight();
            getParticles()[i]->setWeight(w);
        }
    }



//    std::cout << "Particle States \n" << getParticles() << std::endl;
    particleNormalize();

}

void Localizer::particleLikelihoodGpsR() {

    ROS_DEBUG("%s", __FUNCTION__);

    geo.llh_to_xyz(gps.latitude,gps.longitude, gps.altitude);

    static bool init=true;
    static Eigen::Vector3d now_gps,pre_gps;
    static double now_time,pre_time;
    if (init){
        now_gps=pre_gps=Eigen::Vector3d(geo.x(),geo.y(),geo.z());
        now_time=pre_time=time.toSec();
        init=false;
        return;
    }
    pre_gps=now_gps;
    pre_time=now_time;
    now_gps=Eigen::Vector3d(geo.x(),geo.y(),geo.z());
    now_time=time.toSec();

    double dx=now_gps(0)-pre_gps(0);
    double dy=now_gps(1)-pre_gps(1);
    if(std::sqrt(dx*dx+dy*dy)<1.0){
        return;
    }

    double gps_r=std::atan2(dy,dx)-M_PI/2.0;
    gps_r=State<>::normalize_radian(gps_r);
    ROS_DEBUG_STREAM("Mean R: " << now_person_state.r <<", GPS R: " << gps_r);

    double r_sd;
    if (!private_nh.getParam("particlefitler_gps_r_standard_deviation",r_sd)){
        r_sd=PARTICLEFILTER_GPS_R_STANDARD_DEVIATION;
    }


    /** Compute Likelihood **/
    std::vector<double> w_vec;
    for (auto particle:getParticles()){
        State<> s=*particle->getState();

        //// Likelihood R
        double r_r=std::abs(State<>::normalize_radian(s.r-gps_r));
        double w=std::exp(-std::pow(r_r,2)/(2*r_sd*r_sd));

        w_vec.push_back(w);
    }

    if(!std::accumulate(w_vec.begin(),w_vec.end(),0.0)==0.0){
        for(size_t i=0;i<getParticles().size();i++){
            double w=w_vec[i]*getParticles()[i]->getWeight();
            getParticles()[i]->setWeight(w);
        }
    }

//    std::cout << "Particle States \n" << getParticles() << std::endl;
    particleNormalize();

}

bool Localizer::getPedestrianFromPerception() {

    ROS_DEBUG("%s", __FUNCTION__);

    /** Extracting pedestrian object in Detected objects **/
    double dr_min=100;
    bool isPersonInDetectedObjects=false;
    std_msgs::Header vph=vehicle_perception.header;
    for(const autoware_msgs::DetectedObject obj:vehicle_perception.objects){
        if (obj.label=="pedestrian"){

//            std::cout<<obj.pose<<std::endl;
            if (std::isnan(obj.pose.orientation.x) ||
                std::isnan(obj.pose.orientation.y) ||
                std::isnan(obj.pose.orientation.z) ||
                std::isnan(obj.pose.orientation.w)){
                continue;
            }

            geometry_msgs::PoseStamped in,out;
            in.header=obj.header;
            in.pose=obj.pose;
            out.header.frame_id=origin_frame_id;
            out.header.stamp=in.header.stamp;
            geometry_msgs::TransformStamped transform= tf_buffer.lookupTransform(origin_frame_id,in.header.frame_id,in.header.stamp,ros::Duration(1.0));
            tf2::doTransform(in,out,transform);
            double dr=std::sqrt(std::pow(now_person_state.x-out.pose.position.x,2)+
                                std::pow(now_person_state.y-out.pose.position.y,2));
            std::cout<<in<<std::endl<<out<<std::endl;
            if(dr<dr_min){
                this->person_perception_object=obj;
                this->person_perception_object.header=vehicle_perception.header;
                person_perception_origin_state.x=out.pose.position.x;
                person_perception_origin_state.y=out.pose.position.y;
                person_perception_origin_state.vx=obj.velocity.linear.y;
                person_perception_origin_state.vy=obj.velocity.linear.x;
                tf::Quaternion quat(out.pose.orientation.x,
                                    out.pose.orientation.y,
                                    out.pose.orientation.z,
                                    out.pose.orientation.w);
                double rpy[3];
                tf::Matrix3x3(quat).getRPY(rpy[0], rpy[1], rpy[2]);
                person_perception_origin_state.r=State<>::normalize_radian(rpy[2]-M_PI/2.0);
                dr_min=dr;
            }
            isPersonInDetectedObjects=true;
        }
    }
    if(isPersonInDetectedObjects){
        ROS_DEBUG_STREAM("Person with phone origin state: "<<person_perception_origin_state);
    }
    if(!isPersonInDetectedObjects){
        ROS_ERROR("No pedestrian object in Detected Objects");
        autoware_msgs::DetectedObject tmp;
        this->person_perception_object=tmp;
        person_perception_origin_state.clear();
    }

    return isPersonInDetectedObjects;

}

void Localizer::particleLikelihoodVehiclePerceptionXY() {

    ROS_DEBUG("%s", __FUNCTION__);

    double per_x=person_perception_origin_state.x;
    double per_y=person_perception_origin_state.y;
    assert(person_perception_origin_state.x!=0.0);
    assert(person_perception_origin_state.y!=0.0);

    double xy_sd;
    if (!private_nh.getParam("particlefitler_perception_xy_standard_deviation",xy_sd)){
        xy_sd=PARTICLEFILTER_PERCEPTION_XY_STANDARD_DEVIATION;
    }

    /** Compute Likelihood **/
    std::vector<double> w_vec;
    for (auto particle:getParticles()) {
        State<> s = *particle->getState();

        //// Likelihood XY
        double r_xy = std::sqrt(std::pow(s.x - per_x, 2) + std::pow(s.y - per_y, 2));
        double w = std::exp(-r_xy*r_xy / (2 * xy_sd * xy_sd));

        w_vec.push_back(w);
    }

    if(!std::accumulate(w_vec.begin(),w_vec.end(),0.0)==0.0){
        for(size_t i=0;i<getParticles().size();i++){
            double w=w_vec[i]*getParticles()[i]->getWeight();
            getParticles()[i]->setWeight(w);
        }
    }


//    std::cout << "Particle States \n" << getParticles() << std::endl;
    particleNormalize();



}

void Localizer::particleLikelihoodVehiclePerceptionV() {

    ROS_DEBUG("%s", __FUNCTION__);

    double per_vx=person_perception_origin_state.vx;
    double per_vy=person_perception_origin_state.vy;

    double per_v=std::sqrt(per_vx*per_vx+per_vy*per_vy);

    double v_th;
    if (!private_nh.getParam("particlefitler_perception_v_th",v_th)){
        v_th=PARTICLEFILTER_PERCEPTION_V_TH;
    }
    if (per_v<v_th){
        ROS_DEBUG("Skip Likelihood V");
    }

    double sd;
    if (!private_nh.getParam("particlefitler_perception_v_standard_deviation",sd)){
        sd=PARTICLEFILTER_PERCEPTION_V_STANDARD_DEVIATION;
    }

    std::vector<double> w_vec;
    for (auto particle:getParticles()) {
        State<> s = *particle->getState();

        double s_v=std::sqrt(s.vx*s.vx+s.vy*s.vy);
        double w=std::exp(-std::pow(s_v-per_v,2)/(2*sd*sd));

        w_vec.push_back(w);
    }

    if(!std::accumulate(w_vec.begin(),w_vec.end(),0.0)==0.0){
        for(size_t i=0;i<getParticles().size();i++){
            double w=w_vec[i]*getParticles()[i]->getWeight();
            getParticles()[i]->setWeight(w);
        }
    }


//    std::cout << "Particle States \n" << getParticles() << std::endl;
    particleNormalize();

}

void Localizer::particleLikelihoodVehiclePerceptionR() {

    ROS_DEBUG("%s", __FUNCTION__);

//    double per_r=person_perception_origin_state.r;
//    assert(person_perception_origin_state.r!=0.0);

    double per_vx=person_perception_origin_state.vx;
    double per_vy=person_perception_origin_state.vy;

    double per_v=std::sqrt(per_vx*per_vx+per_vy*per_vy);

    double v_th;
    if (!private_nh.getParam("particlefitler_perception_v_th",v_th)){
        v_th=PARTICLEFILTER_PERCEPTION_V_TH;
    }
    if (per_v<v_th){
        ROS_DEBUG("Skip Likelihood R");
    }

    double sd;
    if (!private_nh.getParam("particlefitler_perception_r_standard_deviation",sd)){
        sd=PARTICLEFILTER_PERCEPTION_R_STANDARD_DEVIATION;
    }

    std::vector<double> w_vec;
    for (auto particle:getParticles()) {
        State<> s = *particle->getState();

        double r_r=State<>::normalize_radian(person_perception_origin_state.r-s.r);
        double w=std::exp(-r_r*r_r/(2*sd*sd));

        w_vec.push_back(w);

    }

    if(!std::accumulate(w_vec.begin(),w_vec.end(),0.0)==0.0){
        for(size_t i=0;i<getParticles().size();i++){
            double w=w_vec[i]*getParticles()[i]->getWeight();
            getParticles()[i]->setWeight(w);
        }
    }


//    std::cout << "Particle States \n" << getParticles() << std::endl;
    particleNormalize();

}

/**
 *  Main Function
 **/
int main(int argc, char **argv) {

    // debug mode
    if (ros::console::set_logger_level(ROSCONSOLE_DEFAULT_NAME, ros::console::levels::Debug)) {
        ros::console::notifyLoggerLevelsChanged();
    }

    ros::init(argc, argv, "smartphone_localizer_node");
    Localizer node;
    ros::spin();

    return (0);
}