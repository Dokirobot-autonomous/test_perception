//
// Created by ohashi on 19/09/13.
//

#include <vehicle_perceptor.h>

VehiclePhoneManager::VehiclePhoneManager( ros::NodeHandle* nh, ros::NodeHandle* private_nh,
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
    std::string topic_name_vehicle_perception=TOPIC_NAME_VEHICLE_PERCEPTION;
    this->nh.getParam("topic_name_vehicle_perception",topic_name_vehicle_perception);
    sub_vehicle_perception=this->nh.subscribe(topic_name_vehicle_perception,1,&VehiclePhoneManager::callback_vehicle_perception,this);

}



VehiclePhoneManager::~VehiclePhoneManager() {}

void VehiclePhoneManager::callback_vehicle_perception(const autoware_msgs::DetectedObjectArrayConstPtr &msg) {

    ROS_INFO("%s",__FUNCTION__);

    vehicle_perception_objects=*msg;
    time=msg->header.stamp;
    vehicle_perception_update=true;
    run();
    vehicle_perception_update=false;

}

void VehiclePhoneManager::run() {

    ROS_DEBUG("%s",__FUNCTION__);


    for(auto& pl:phone_localizers){
        pl->lock.lock();
    }

    if(vehicle_perception_update){

        getPedestriansFromVehiclePerception();
        transformPedestriansFromVehicleToOrigin();
        if(!pedestrian_origin_states.empty()){
        assignVehiclePedestriansToPhones();
        particleLikelihoodVehiclePerceptionXY();
        particleLikelihoodVehiclePerceptionV();
        particleLikelihoodVehiclePerceptionR();
        }

    }

    for(auto& pl:phone_localizers){
        pl->publish_odom(time);
        pl->publish_poseaary(time);
    }

    for(auto& pl:phone_localizers){
        pl->lock.unlock();
    }


}

void VehiclePhoneManager::getPedestriansFromVehiclePerception() {

    /** Extracting pedestrian object in Detected objects **/
    vehicle_perception_pedestrians.header=vehicle_perception_objects.header;
    vehicle_perception_pedestrians.objects.clear();

    for(const autoware_msgs::DetectedObject& obj:vehicle_perception_objects.objects){
        if (obj.label=="pedestrian"){
            vehicle_perception_pedestrians.objects.push_back(obj);
        }
    }

        ROS_DEBUG_STREAM("Pedestrian size in objects: "<< vehicle_perception_pedestrians.objects.size());
}

void VehiclePhoneManager::transformPedestriansFromVehicleToOrigin(){

    for(const autoware_msgs::DetectedObject obj:vehicle_perception_pedestrians.objects){

            if (std::isnan(obj.pose.orientation.x) ||
                std::isnan(obj.pose.orientation.y) ||
                std::isnan(obj.pose.orientation.z) ||
                std::isnan(obj.pose.orientation.w) ||
                obj.pose.orientation.x==obj.pose.orientation.y==obj.pose.orientation.z==obj.pose.orientation.w==0){
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

void VehiclePhoneManager::assignVehiclePedestriansToPhones() {

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

    std::cout<<w_sum_table<<std::endl;

    /** Assign weights **/
    //// Find max index
    Eigen::MatrixXd w_sum_table_tmp=w_sum_table;
    while(true){
        Eigen::MatrixXd::Index idx_max_pe,idx_max_pl;
        std::cout<<w_sum_table_tmp<<std::endl;
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

void VehiclePhoneManager::particleLikelihoodVehiclePerceptionXY() {

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

void VehiclePhoneManager::particleLikelihoodVehiclePerceptionV() {

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

void VehiclePhoneManager::particleLikelihoodVehiclePerceptionR() {

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

double VehiclePhoneManager::computeWeightXY(const State<> &pedestrian, const State<> &particle) {

    ROS_DEBUG("%s", __FUNCTION__);

    static double xy_sd=PARTICLEFILTER_VEHICLE_PERCEPTION_XY_STANDARD_DEVIATION;
    private_nh.getParam("particlefilter_vehicle_perception_xy_statndard_deviation",xy_sd);
    std::cout<<xy_sd<<std::endl;

    //// Likelihood XY
    double r_xy = std::sqrt(std::pow(pedestrian.x - particle.x, 2) + std::pow(pedestrian.y - particle.y, 2));
    double w = std::exp(-r_xy*r_xy / (2 * xy_sd * xy_sd));

    return w;
}

double VehiclePhoneManager::computeWeightV(const State<> &pedestrian, const State<> &particle) {

    double ped_v=std::sqrt(pedestrian.vx*pedestrian.vx+pedestrian.vy*pedestrian.vy);
    double par_v=std::sqrt(particle.vx*particle.vx+particle.vy*particle.vy);

    static double sd=PARTICLEFILTER_VEHICLE_PERCEPTION_V_STANDARD_DEVIATION;
    private_nh.getParam("particlefilter_vehicle_perception_v_statndard_deviation",sd);

    double w=std::exp(-std::pow(ped_v-par_v,2)/(2*sd*sd));
    return w;

}

double VehiclePhoneManager::computeWeightR(const State<> &pedestrian, const State<> &particle) {

    static double sd=PARTICLEFILTER_VEHICLE_PERCEPTION_R_STANDARD_DEVIATION;
    private_nh.getParam("particlefilter_vehicle_perception_r_statndard_deviation",sd);

    double w=std::exp(-std::pow(pedestrian.r-particle.r,2)/(2*sd*sd));
    return w;

}
