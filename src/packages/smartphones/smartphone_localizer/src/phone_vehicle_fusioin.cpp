//
// Created by ohashi on 19/09/13.
//

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

#define USE_MKS false
#define USE_MKZ false
#define USE_PRIUS true

#define PHONE_NAMES "android"
#define VEHICLE_NAME "prius"

#define ORIGIN_FRAME_ID "odom"
#define ORIGIN_FIX "37.9181551873,-122.331354939,0.0"

#define ASSIGN_PEDESTRIANS_TO_PHONES_EXTRACTION_PARTICLE_SIZE 100

#define PARTICLEFILTER_VEHICLE_PERCEPTION_XY_STANDARD_DEVIATION 0.3                      //
#define PARTICLEFILTER_VEHICLE_PERCEPTION_V_STANDARD_DEVIATION 0.01                      //
#define PARTICLEFILTER_VEHICLE_PERCEPTION_R_STANDARD_DEVIATION 2.0*M_PI/180                      //

#define TOPIC_NAMES_PERCEPTION VEHICLE_NAME + "/detection/object_tracker/objects"

class PhoneVehicleFusion{

public:

    PhoneVehicleFusion(ros::NodeHandle* nh, ros::NodeHandle* private_nh, std::vector<PhoneLocalizer*>* phone_localizers, std::string vehicle_name=VEHICLE_NAME);
//    PhoneVehicleFusion(ros::NodeHandle& nh, ros::NodeHandle& private_nh);
    ~PhoneVehicleFusion();

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

//
//
//
//    void particleLikelihoodPerception(const std::vector<State<>> )
//    void assignPedestrianToPhoneLocalizer(const std::vector<State<>>& pedestrian_states, std::vector<int>* idx_pedestrians);
//    std::vector<double> particleLikelihoodPerceptionXY(const State<>& pedestrian,const PhoneLocalizer& pl);
//    std::vector<double> particleLikelihoodPerceptionV(const State<>& pedestrian,const PhoneLocalizer& pl);
//    std::vector<double> particleLikelihoodPerceptionR(const State<>& pedestrian,const PhoneLocalizer& pl);

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

//    std::vector<std::vector<State<>>> perceptions_pedestrian_states;

    /** GPS fix -> pose **/
    geo_pos_conv geo;
    double origin_lat,origin_lon,origin_ele;

    /** Others **/
    ros::Time time;


};

PhoneVehicleFusion::PhoneVehicleFusion( ros::NodeHandle* nh, ros::NodeHandle* private_nh,
                                        std::vector<PhoneLocalizer*>* phone_localizers,
                                        std::string vehicle_name) :
                                        nh(*nh),private_nh(*private_nh),
                                        phone_localizers(*phone_localizers),vehicle_name(vehicle_name){

    ROS_DEBUG("%s",__FUNCTION__);

    /** Set Parameter **/

    //// origin
    std::string str;
    if (!this->nh.getParam("origin_fix", str)) {
        str = ORIGIN_FIX;
    }
    sscanf(str.c_str(), "%lf,%lf,%lf", &origin_lat, &origin_lon, &origin_ele);
    geo.set_plane(origin_lat, origin_lon);

    //// TF Listener
    tf2_ros::TransformListener tf_listener(tf_buffer);


    /** Set subscriber **/
    sub_vehicle_perception=this->nh.subscribe(str,1,&PhoneVehicleFusion::callback_vehicle_perception,this);

    ros::spin();
}



PhoneVehicleFusion::~PhoneVehicleFusion() {}

void PhoneVehicleFusion::callback_vehicle_perception(const autoware_msgs::DetectedObjectArrayConstPtr &msg) {

    ROS_INFO("%s",__FUNCTION__);

    vehicle_perception_objects=*msg;
    vehicle_perception_update=true;
    run();
    vehicle_perception_update=false;

}

void PhoneVehicleFusion::run() {

    ROS_DEBUG("%s",__FUNCTION__);

    for(auto& pl:phone_localizers){
        pl->lock.lock();
    }

    if(vehicle_perception_update){

        getPedestriansFromVehiclePerception();
        transformPedestriansFromVehicleToOrigin();
        assignVehiclePedestriansToPhones();
        particleLikelihoodVehiclePerceptionXY();
        particleLikelihoodVehiclePerceptionV();
        particleLikelihoodVehiclePerceptionR();

    }

    for(auto& pl:phone_localizers){
        pl->lock.unlock();
    }


}

void PhoneVehicleFusion::getPedestriansFromVehiclePerception() {

    /** Extracting pedestrian object in Detected objects **/
    vehicle_perception_pedestrians.header=vehicle_perception_objects.header;
    vehicle_perception_pedestrians.objects.clear();

    for(const autoware_msgs::DetectedObject& obj:vehicle_perception_objects.objects){
        if (obj.label=="pedestrian"){
            vehicle_perception_pedestrians.objects.push_back(obj);
        }
    }

    if(!vehicle_perception_pedestrians.objects.empty()){
        ROS_DEBUG_STREAM("Pedestrians: \n"<< vehicle_perception_pedestrians);
    }
    else{
        ROS_DEBUG("No Pedestrians");
    }
}

void PhoneVehicleFusion::transformPedestriansFromVehicleToOrigin(){

    for(const autoware_msgs::DetectedObject obj:vehicle_perception_pedestrians.objects){

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

            pedestrian_origin_states.push_back(pedestrian);
            ROS_DEBUG_STREAM("Pedestrian origin states: \n"<< pedestrian_origin_states);
    }
}

void PhoneVehicleFusion::assignVehiclePedestriansToPhones() {

    ROS_DEBUG("%s",__FUNCTION__);

    static std::vector<int> idxs(PARTICLEFILTER_PARTICLE_SIZE);
    std::iota(idxs.begin(),idxs.end(),0);

    std::vector<int> ped_idxs_to_phone=std::vector<int>(phone_localizers.size(),-1);

    Eigen::MatrixXd w_sum_table(pedestrian_origin_states.size(),phone_localizers.size());
//    std::vector<double> w_pl(phones_size);

    /** Creating weight table **/
    for(size_t i_pl=0;i_pl<phone_localizers.size();i_pl++){
        auto& pl=phone_localizers[i_pl];
        std::random_shuffle(pl->getParticles().begin(),pl->getParticles().end());
        std::vector<Particle<State<>>*> particles(ASSIGN_PEDESTRIANS_TO_PHONES_EXTRACTION_PARTICLE_SIZE);
        std::copy(pl->getParticles().begin(),pl->getParticles().begin()+ASSIGN_PEDESTRIANS_TO_PHONES_EXTRACTION_PARTICLE_SIZE,particles.begin());
        for(size_t i_pos=0;i_pos<pedestrian_origin_states.size();i_pos++){
            const auto& pos=pedestrian_origin_states[i_pos];
            std::vector<double> weights;
            for(auto& p:particles){
                double w_xy=computeWeightXY(pos,*p->getState());
                double w_v=computeWeightV(pos,*p->getState());
                double w_r=computeWeightR(pos,*p->getState());
                weights.push_back(w_xy*w_v*w_r);
            }
            w_sum_table(i_pos,i_pl)=std::accumulate(weights.begin(),weights.end(),0.0);
        }
    }

    /** Assign weights **/
    //// Find max index
    Eigen::MatrixXd w_sum_table_tmp=w_sum_table;
    while(true){
        Eigen::MatrixXd::Index idx_max_pe,idx_max_pl;
        w_sum_table_tmp.maxCoeff(&idx_max_pe,&idx_max_pl);
        ped_idxs_to_phone[idx_max_pl]=idx_max_pe;
        w_sum_table_tmp.row(idx_max_pe).setZero();
        w_sum_table_tmp.col(idx_max_pl).setZero();
        if(std::find(ped_idxs_to_phone.begin(),ped_idxs_to_phone.end(),-1)==ped_idxs_to_phone.end()){
            assert(w_sum_table_tmp.isZero());
            break;
        }
    }

    pedestrian_indices_assigned_to_phones=ped_idxs_to_phone;

}

void PhoneVehicleFusion::particleLikelihoodVehiclePerceptionXY() {

    ROS_DEBUG("%s", __FUNCTION__);

    for(size_t i_pl=0;i_pl<phone_localizers.size();i_pl++){
        auto& pl=phone_localizers[i_pl];
        State<> ped_s=pedestrian_origin_states[pedestrian_indices_assigned_to_phones[i_pl]];

        std::vector<double> weights;
        for(auto& particle:pl->getParticles()){
            State<> par_s=*particle->getState();
            weights.push_back(computeWeightXY(ped_s,par_s));
        }

        if(std::accumulate(weights.begin(),weights.end(),0.0)!=0.0){
            for(size_t i=0;i<weights.size();i++){
                double w=weights[i]*pl->getParticles()[i]->getWeight();
                pl->getParticles()[i]->setWeight(w);
            }
            pl->particleNormalize();
//    std::cout << "Particle States \n" << pl->getParticles() << std::endl;
        }
        else{
            ROS_DEBUG("Skip likelihood: %s",__FUNCTION__);
        }
    }

}

void PhoneVehicleFusion::particleLikelihoodVehiclePerceptionV() {

    ROS_DEBUG("%s", __FUNCTION__);

    for(size_t i_pl=0;i_pl<phone_localizers.size();i_pl++){
        auto& pl=phone_localizers[i_pl];
        State<> ped_s=pedestrian_origin_states[pedestrian_indices_assigned_to_phones[i_pl]];

        std::vector<double> weights;
        for(auto& particle:pl->getParticles()){
            State<> par_s=*particle->getState();
            weights.push_back(computeWeightV(ped_s,par_s));
        }

        if(std::accumulate(weights.begin(),weights.end(),0.0)!=0.0){
            for(size_t i=0;i<weights.size();i++){
                double w=weights[i]*pl->getParticles()[i]->getWeight();
                pl->getParticles()[i]->setWeight(w);
            }
            pl->particleNormalize();
//    std::cout << "Particle States \n" << pl->getParticles() << std::endl;
        }
        else{
            ROS_DEBUG("Skip likelihood: %s",__FUNCTION__);
        }
    }

}

void PhoneVehicleFusion::particleLikelihoodVehiclePerceptionR() {

    ROS_DEBUG("%s", __FUNCTION__);

    for(size_t i_pl=0;i_pl<phone_localizers.size();i_pl++){
        auto& pl=phone_localizers[i_pl];
        State<> ped_s=pedestrian_origin_states[pedestrian_indices_assigned_to_phones[i_pl]];

        std::vector<double> weights;
        for(auto& particle:pl->getParticles()){
            State<> par_s=*particle->getState();
            weights.push_back(computeWeightR(ped_s,par_s));
        }

        if(std::accumulate(weights.begin(),weights.end(),0.0)!=0.0){
            for(size_t i=0;i<weights.size();i++){
                double w=weights[i]*pl->getParticles()[i]->getWeight();
                pl->getParticles()[i]->setWeight(w);
            }
            pl->particleNormalize();
//    std::cout << "Particle States \n" << pl->getParticles() << std::endl;
        }
        else{
            ROS_DEBUG("Skip likelihood: %s",__FUNCTION__);
        }
    }

}

double PhoneVehicleFusion::computeWeightXY(const State<> &pedestrian, const State<> &particle) {

    ROS_DEBUG("%s", __FUNCTION__);

    static double xy_sd=PARTICLEFILTER_VEHICLE_PERCEPTION_XY_STANDARD_DEVIATION;
    private_nh.getParam("particlefilter_vehicle_perception_xy_statndard_deviation",xy_sd);
    std::cout<<xy_sd<<std::endl;

    //// Likelihood XY
    double r_xy = std::sqrt(std::pow(pedestrian.x - particle.x, 2) + std::pow(pedestrian.y - particle.y, 2));
    double w = std::exp(-r_xy*r_xy / (2 * xy_sd * xy_sd));

    return w;
}

double PhoneVehicleFusion::computeWeightV(const State<> &pedestrian, const State<> &particle) {

    double ped_v=std::sqrt(pedestrian.vx*pedestrian.vx+pedestrian.vy*pedestrian.vy);
    double par_v=std::sqrt(particle.vx*particle.vx+particle.vy*particle.vy);

    static double sd=PARTICLEFILTER_VEHICLE_PERCEPTION_V_STANDARD_DEVIATION;
    private_nh.getParam("particlefilter_vehicle_perception_v_statndard_deviation",sd);

    double w=std::exp(-std::pow(ped_v-par_v,2)/(2*sd*sd));
    return w;

}

double PhoneVehicleFusion::computeWeightR(const State<> &pedestrian, const State<> &particle) {

    static double sd=PARTICLEFILTER_VEHICLE_PERCEPTION_R_STANDARD_DEVIATION;
    private_nh.getParam("particlefilter_vehicle_perception_r_statndard_deviation",sd);

    double w=std::exp(-std::pow(pedestrian.r-particle.r,2)/(2*sd*sd));
    return w;

}



/**************************************
 *      Main Function
 **************************************/

int main(int argc, char **argv) {

//    // debug mode
//    if (ros::console::set_logger_level(ROSCONSOLE_DEFAULT_NAME, ros::console::levels::Debug)) {
//        ros::console::notifyLoggerLevelsChanged();
//    }

    //// Initialize ROS
    ros::init(argc, argv, "vehicle_perceptor");
    ros::NodeHandle nh,private_nh("~");

    //// Phone Localizer
    std::vector<PhoneLocalizer*> phone_localizers;
    std::string phone_names, token;
    if (!private_nh.getParam("phone_names", phone_names)) {
        phone_names = PHONE_NAMES;
    }
    std::stringstream ss(phone_names);
    while (std::getline(ss, token, ',')) {
        PhoneLocalizer *pl = new PhoneLocalizer(&nh, &private_nh, token);
        phone_localizers.push_back(pl);
    }

    //// Vehicle Perceptor
    PhoneVehicleFusion node(&nh,&private_nh,&phone_localizers);
    ros::spin();

    return (0);
}