//
// Created by ohashi on 19/09/13.
//

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <chrono>

#include <Eigen/Core>

#include <ros/ros.h>
#include <ros/console.h>

#include <tf/transform_broadcaster.h>
#include <tf2_ros/transform_listener.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>

#include "../../../../../../devel/include/autoware_msgs/DetectedObject.h"
#include "../../../../../../devel/include/autoware_msgs/DetectedObjectArray.h"

#include <phone_localizer.h>

#define USE_MKS false
#define USE_MKZ false
#define USE_PRIUS true

#define PHONE_NAMES "android"

#define ORIGIN_FRAME_ID "odom"
#define ORIGIN_FIX "37.9181551873,-122.331354939,0.0"

#define PARTICLEFILTER_PRIUS_PERCEPTION_XY_STANDARD_DEVIATION 0.3                      //
#define PARTICLEFILTER_PRIUS_PERCEPTION_V_STANDARD_DEVIATION 0.01                      //
#define PARTICLEFILTER_PRIUS_PERCEPTION_R_STANDARD_DEVIATION 2.0*M_PI/180                      //
#define PARTICLEFILTER_PRIUS_PERCEPTION_V_TH 1.0                      // 5.0[m/s]

#define TOPIC_NAMES_MKS_PERCEPTION "/detection/object_tracker/objects"
#define TOPIC_NAMES_MKZ_PERCEPTION "/detection/object_tracker/objects"
#define TOPIC_NAMES_PRIUS_PERCEPTION "/detection/object_tracker/objects"

class PhoneVehicleFusion{

public:

    PhoneVehicleFusion();
    ~PhoneVehicleFusion();

    void callback_mks_perception(const autoware_msgs::DetectedObjectArrayConstPtr &msg);
    void callback_mkz_perception(const autoware_msgs::DetectedObjectArrayConstPtr &msg);
    void callback_prius_perception(const autoware_msgs::DetectedObjectArrayConstPtr &msg);

    void run();

    void getPedestriansFromPriusPerception();
    void transformPedestriansFromPriusToOrigin();
    void assignPriusPedestriansToPhones();
    void particleLikelihoodPriusPerception();

    void particleLikelihoodPriusPerceptionXY();
    void particleLikelihoodPriusPerceptionV();
    void particleLikelihoodPriusPerceptionR();


//
//
//
//    void particleLikelihoodPerception(const std::vector<State<>> )
//    void assignPedestrianToPhoneLocalizer(const std::vector<State<>>& pedestrian_states, std::vector<int>* idx_pedestrians);
//    std::vector<double> particleLikelihoodPerceptionXY(const State<>& pedestrian,const PhoneLocalizer& pl);
//    std::vector<double> particleLikelihoodPerceptionV(const State<>& pedestrian,const PhoneLocalizer& pl);
//    std::vector<double> particleLikelihoodPerceptionR(const State<>& pedestrian,const PhoneLocalizer& pl);

private:

    ros::NodeHandle nh;
    ros::NodeHandle private_nh;
    ros::Subscriber sub_mks_perception,sub_mkz_perception,sub_prius_perception;

    /** Subscribed Topics **/
    autoware_msgs::DetectedObjectArray mks_perception_objects,mkz_perception_objects,prius_perception_objects;
    autoware_msgs::DetectedObjectArray mks_perception_pedestrians,mkz_perception_pedestrians,prius_perception_pedestrians;
    bool mks_perception_update,mkz_perception_update,prius_perception_update;

    //// Frame ID
    std::string origin_frame_id;


    /** TF Listener **/
    tf2_ros::Buffer tf_buffer;

    /** Phone Localizer **/
    size_t phones_size;
    std::vector<std::string> phone_names;
    std::vector<PhoneLocalizer*> phone_localizers;

    /** Vehicle Likelihood **/

//    std::vector<std::vector<State<>>> perceptions_pedestrian_states;

    /** GPS fix -> pose **/
    geo_pos_conv geo;
    double origin_lat,origin_lon,origin_ele;

    /** Others **/
    ros::Time time;


};

PhoneVehicleFusion::PhoneVehicleFusion() : nh(), private_nh(ros::NodeHandle("~"))
{

    ROS_DEBUG("%s",__FUNCTION__);

    /** Set Parameter **/

    //// phone name
    std::string str;
    if (!private_nh.getParam("phone_names", str)) {
        str = PHONE_NAMES;
    }
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, ',')) {
        phone_names.push_back(token);
    }
    phones_size=phone_names.size();

    //// origin
    if (!nh.getParam("origin_fix", str)) {
        str = ORIGIN_FIX;
    }
    sscanf(str.c_str(), "%lf,%lf,%lf", &origin_lat, &origin_lon, &origin_ele);
    geo.set_plane(origin_lat, origin_lon);

    //// TF Listener
    tf2_ros::TransformListener tf_listener(tf_buffer);


    /** Initialize Phone Localizer **/
    for(size_t iph=0;iph<phones_size;iph++){
        PhoneLocalizer* pl=new PhoneLocalizer;
        pl->set_phone_name(phone_names[iph]);
        phone_localizers.push_back(pl);
    }


    /** Set subscriber **/
    bool use_mks,use_mkz,use_prius;
    if (!private_nh.getParam("use_mks",use_mks)){
        use_mks=USE_MKS;
    }
    if (!private_nh.getParam("use_mkz",use_mkz)){
        use_mkz=USE_MKZ;
    }
    if (!private_nh.getParam("use_prius",use_prius)){
        use_prius=USE_PRIUS;
    }
    if(use_mks){
        if (!private_nh.getParam("topic_names_mks_perception", str)) {
            str = TOPIC_NAMES_MKS_PERCEPTION;
        }
        sub_mks_perception=nh.subscribe(str,1,&PhoneVehicleFusion::callback_mks_perception,this);
    }
    if(use_mkz){
        if (!private_nh.getParam("topic_names_mkz_perception", str)) {
            str = TOPIC_NAMES_MKZ_PERCEPTION;
        }
        sub_mkz_perception=nh.subscribe(str,1,&PhoneVehicleFusion::callback_mkz_perception,this);
    }
    if(use_prius){
        if (!private_nh.getParam("topic_names_prius_perception", str)) {
            str = TOPIC_NAMES_PRIUS_PERCEPTION;
        }
        sub_prius_perception=nh.subscribe(str,1,&PhoneVehicleFusion::callback_prius_perception,this);
    }

    ros::spin();
}

PhoneVehicleFusion::~PhoneVehicleFusion() {
    for(auto& pl:phone_localizers){
        delete pl;
    }
}

void PhoneVehicleFusion::callback_mks_perception(const autoware_msgs::DetectedObjectArrayConstPtr &msg) {

    ROS_INFO("%s",__FUNCTION__);

    mks_perception_objects=*msg;
    mks_perception_update=true;
    run();
    mks_perception_update=false;
}

void PhoneVehicleFusion::callback_mkz_perception(const autoware_msgs::DetectedObjectArrayConstPtr &msg) {

    ROS_INFO("%s",__FUNCTION__);

    mkz_perception_objects=*msg;
    mkz_perception_update=true;
    run();
    mkz_perception_update=false;

}

void PhoneVehicleFusion::callback_prius_perception(const autoware_msgs::DetectedObjectArrayConstPtr &msg) {

    ROS_INFO("%s",__FUNCTION__);

    prius_perception_objects=*msg;
    prius_perception_update=true;
    run();
    prius_perception_update=false;

}

void PhoneVehicleFusion::run() {

    ROS_DEBUG("%s",__FUNCTION__);
    for(auto& pl:phone_localizers){
        pl->lock.lock();
    }

    if(mks_perception_update){

    }

    if(mkz_perception_update){

    }

    if(prius_perception_update){
        getPedestriansStatesFromPriusPerception();
        particleLikelihoodPriusPerception();
    }



    /** Extracting pedestrian object in Detected objects **/
    std::vector<State<>> pedestrians_states;
    for(const autoware_msgs::DetectedObject obj:perception_objects.objects){
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

            State<> pedestrian;
            pedestrian.x=out.pose.position.x;
            pedestrian.y=out.pose.position.y;
            pedestrian.vx=obj.velocity.linear.y;
            pedestrian.vy=obj.velocity.linear.x;
            tf::Quaternion quat(out.pose.orientation.x,
                                out.pose.orientation.y,
                                out.pose.orientation.z,
                                out.pose.orientation.w);
            double rpy[3];
            tf::Matrix3x3(quat).getRPY(rpy[0], rpy[1], rpy[2]);
            pedestrian.r=State<>::normalize_radian(rpy[2]-M_PI/2.0);

            pedestrians_states.push_back(pedestrian);
            ROS_DEBUG_STREAM("Pedestrian"<< pedestrians_states.size() <<" state: "<<pedestrian);
        }
    }


    /** Particle Likelihdood **/
    std::vector<int> assigned_pedestrians_idx_to_phones;




    std::vector<State<>> ped_states;
    getPedestriansStates(perception_objects,&ped_states);
    std::vector<int> idx_ped;
    assignPedestrianToPhoneLocalizer(ped_states,idx_ped);
    for(size_t pi=0;pi<phones_size;pi++){
        State<> ped=ped_states[pi];
        particleLikelihoodPerceptionXY(ped,phone_localizers[pi]);

    }
    for(auto& pl:phone_localizers){
        particleLikelihoodPerceptionXY(ped_states[idx_ped[]])
    }


    std::vector<int> idx_pedestrian_assigned_localizers;
    for(auto& pl:phone_localizers){
        pl->lock.lock();
    }

    for(size_t pi=0;pi<perceptions_size;pi++){

        if(perceptions_update[pi]==false){
            continue;
        }

        //// Get Pedestrian states
        std::vector<State<>> ped_states=getPedestriansStates(perceptions[pi]);
        perceptions_pedestrian_states[pi]=ped_states;

        //// Assign pedestrian states to phone loclizer
        idx_pedestrian_assigned_localizers=assignPedestrianToPhoneLocalizer(ped_states);



    }

    for(size_t iph=0;iph<phones_size;iph++){
        ROS_DEBUG_STREAM(phone_names[iph]<<": ");
        (*phone_localizers[iph]).publish_odom();
        (*phone_localizers[iph]).publish_poseaary();
        perceptions_update[iph]=false;
    }

    for(auto& pl:phone_localizers){
        pl->lock.unlock();
    }


}

void PhoneVehicleFusion::getPedestriansStatesFromPriusPerception() {

    /** Extracting pedestrian object in Detected objects **/
    std::vector<State<>> pedestrians_states;
    for(const autoware_msgs::DetectedObject obj:perception_objects.objects){
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

            State<> pedestrian;
            pedestrian.x=out.pose.position.x;
            pedestrian.y=out.pose.position.y;
            pedestrian.vx=obj.velocity.linear.y;
            pedestrian.vy=obj.velocity.linear.x;
            tf::Quaternion quat(out.pose.orientation.x,
                                out.pose.orientation.y,
                                out.pose.orientation.z,
                                out.pose.orientation.w);
            double rpy[3];
            tf::Matrix3x3(quat).getRPY(rpy[0], rpy[1], rpy[2]);
            pedestrian.r=State<>::normalize_radian(rpy[2]-M_PI/2.0);

            pedestrians_states.push_back(pedestrian);
            ROS_DEBUG_STREAM("Pedestrian"<< pedestrians_states.size() <<" state: "<<pedestrian);
        }
    }


}



void PhoneVehicleFusion::getPedestriansStates(const autoware_msgs::DetectedObjectArray& perception_objects,std::vector<State<>>* pedestrians_states) {

    ROS_DEBUG("%s", __FUNCTION__);

    /** Extracting pedestrian object in Detected objects **/
    for(const autoware_msgs::DetectedObject obj:perception_objects.objects){
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

            State<> pedestrian;
            pedestrian.x=out.pose.position.x;
            pedestrian.y=out.pose.position.y;
            pedestrian.vx=obj.velocity.linear.y;
            pedestrian.vy=obj.velocity.linear.x;
            tf::Quaternion quat(out.pose.orientation.x,
                                out.pose.orientation.y,
                                out.pose.orientation.z,
                                out.pose.orientation.w);
            double rpy[3];
            tf::Matrix3x3(quat).getRPY(rpy[0], rpy[1], rpy[2]);
            pedestrian.r=State<>::normalize_radian(rpy[2]-M_PI/2.0);

            pedestrians_states->push_back(pedestrian);
            ROS_DEBUG_STREAM("Pedestrian"<< pedestrians_states->size() <<" state: "<<pedestrian);
        }
    }

}

void PhoneVehicleFusion::assignPedestrianToPhoneLocalizer(const std::vector<State<>>& pedestrian_states, std::vector<int>* idx_pedestrians) {

    ROS_DEBUG("%s",__FUNCTION__);

    *idx_pedestrians=std::vector<int>(phones_size,-1);


    std::vector<std::vector<std::vector<double>>> w_table(phones_size,std::vector<std::vector<double>>(pedestrian_states.size(),std::vector<double>(0)));
    Eigen::MatrixXd w_sum_table(pedestrian_states.size(),phones_size);
//    std::vector<double> w_pl(phones_size);

    /** Creating weight table **/
    for(size_t ipl=0;ipl<phones_size;ipl++){
        auto& pl=phone_localizers[ipl];
        for(size_t ips=0;ips<pedestrian_states.size();ips++){
            auto& ps=pedestrian_states[ips];
            std::vector<double> w_xy=particleLikelihoodPerceptionXY(ps,pl);
            std::vector<double> w_v=particleLikelihoodPerceptionV(ps,pl);
            std::vector<double> w_r=particleLikelihoodPerceptionR(ps,pl);
            for(int i=0;i<w_xy.size();i++){
                double w=w_xy[i]*w_v[i]*w_r[i];
                w_table[ipl][ips].push_back(w);
            }
            w_sum_table(ips,ipl)=std::accumulate(w_table[ipl][ips].begin(),w_table[ipl][ips].end(),0.0);
        }
    }

    /** Assign weights **/
    //// Find max index
    Eigen::MatrixXd w_sum_table_tmp=w_sum_table;
    while(true){
        Eigen::MatrixXd::Index idx_max_pe,idx_max_pl;
        w_sum_table_tmp.maxCoeff(&idx_max_pe,&idx_max_pl);
        idx_pedestrians[idx_max_pl]=idx_max_pe;
        w_sum_table_tmp.row(idx_max_pe).setZero();
        w_sum_table_tmp.col(idx_max_pl).setZero();
        if(std::find(idx_pedestrians.begin(),idx_pedestrians.end(),-1)==idx_pedestrians.end()){
            assert(w_sum_table_tmp.isZero());
            break;
        }
    }

    /** Set likelihood **/
    for(size_t ipl=0;ipl<phones_size;ipl++){
        int ipe=idx_pedestrians[ipl];
        auto& particles=phone_localizers[ipl]->getParticles();
        for(int ipa=0;ipa<particles.size();ipa++){
            double w=particles[ipa]->getWeight()*w_table[ipl][ipe][ipa];
            particles[ipa]->setWeight(w);
        }
        phone_localizers[ipl]->particleNormalize();
    }

}

std::vector<double> PhoneVehicleFusion::particleLikelihoodPerceptionXY(const State<>& pedestrian,PhoneLocalizer& pl) {

    ROS_DEBUG("%s", __FUNCTION__);

    double per_x=pedestrian.x;
    double per_y=pedestrian.y;
    assert(pedestrian.x!=0.0);
    assert(pedestrian.y!=0.0);

    double xy_sd;
    if (!private_nh.getParam("particlefitler_perception_xy_standard_deviation",xy_sd)){
        xy_sd=PARTICLEFILTER_PERCEPTION_XY_STANDARD_DEVIATION;
    }

    /** Compute Likelihood **/
    for (auto particle:pl.getParticles()) {
        State<> s = *particle->getState();
        double w=particle->getWeight();

        //// Likelihood XY
        double r_xy = std::sqrt(std::pow(s.x - per_x, 2) + std::pow(s.y - per_y, 2));
        w *= std::exp(-r_xy*r_xy / (2 * xy_sd * xy_sd));

        particle->setWeight(w);
    }

//    std::cout << "Particle States \n" << getParticles() << std::endl;

}

std::vector<double> PhoneVehicleFusion::particleLikelihoodPerceptionV(const State<>& pedestrian,PhoneLocalizer& pl) {

    ROS_DEBUG("%s", __FUNCTION__);

    double per_vx=pedestrian.vx;
    double per_vy=pedestrian.vy;

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

    for (auto particle:pl.getParticles()) {
        State<> s = *particle->getState();
        double w = particle->getWeight();

        double s_v=std::sqrt(s.vx*s.vx+s.vy*s.vy);
        w*=std::exp(-std::pow(s_v-per_v,2)/(2*sd*sd));

        particle->setWeight(w);
    }

//    std::cout << "Particle States \n" << getParticles() << std::endl;

}

std::vector<double> PhoneVehicleFusion::particleLikelihoodPerceptionR(const State<>& pedestrian,PhoneLocalizer& pl) {

    ROS_DEBUG("%s", __FUNCTION__);

//    double per_r=person_perception_origin_state.r;
//    assert(person_perception_origin_state.r!=0.0);

    double per_vx=pedestrian.vx;
    double per_vy=pedestrian.vy;

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

    for (auto particle:pl.getParticles()) {
        State<> s = *particle->getState();
        double w = particle->getWeight();

        double r_r=State<>::normalize_radian(pedestrian.r-s.r);
        w*=std::exp(-r_r*r_r/(2*sd*sd));

        particle->setWeight(w);
    }

//    std::cout << "Particle States \n" << getParticles() << std::endl;

}


// -----------------------------------
//	Main Function
// -----------------------------------
int main(int argc, char **argv) {

//    // debug mode
//    if (ros::console::set_logger_level(ROSCONSOLE_DEFAULT_NAME, ros::console::levels::Debug)) {
//        ros::console::notifyLoggerLevelsChanged();
//    }

    // Initialize ROS
    ros::init(argc, argv, "phone_vehicle_fusion");
    PhoneVehicleFusion node;
    ros::spin();

    return (0);
}