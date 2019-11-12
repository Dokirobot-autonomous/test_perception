//
// Created by ohashi on 19/09/13.
//

#include <vehicle_phones_manager.h>

VehiclePhonesManager::VehiclePhonesManager( ros::NodeHandle* nh, ros::NodeHandle* private_nh) :
                                        nh(*nh),private_nh(*private_nh){

    ROS_DEBUG("%s",__FUNCTION__);

    /** Set Parameter **/

    //// vehicle name
    this->private_nh.param<std::string>("vehicle_name",vehicle_name,VEHICLE_NAME);

    //// phone names
    std::string phone_names_str;
    this->private_nh.param<std::string>("phone_names",phone_names_str,PHONES_NAMES);
    std::stringstream phone_names_sstr(phone_names_str);
    std::string phone_name_str;
    while(std::getline(phone_names_sstr,phone_name_str,',')){
        phone_names.push_back(phone_name_str);
    }

    //// origin
    std::string origin_fix_str;
    this->nh.param<std::string>("origin_fix", origin_fix_str,ORIGIN_FIX);
    sscanf(origin_fix_str.c_str(), "%lf,%lf,%lf", &origin_lat, &origin_lon, &origin_ele);
    geo.set_plane(origin_lat, origin_lon);

    //// origin frame_id
    this->nh.param<std::string>("origin_frame_id",origin_frame_id,ORIGIN_FRAME_ID);

    //// topic names
    this->nh.param<std::string>("topic_name_objects",topic_name_objects,TOPIC_NAME_OBJECTS);
    for(const auto& pn:phone_names){
        topic_names_phone_odom.push_back("/"+pn+TOPIC_NAME_PHONE_ODOM);
        topic_names_pedestrian2phone.push_back("/"+pn+TOPIC_NAME_PEDESTRIAN2PHONE);
    }

    //// TF Listener
    tf2_ros::TransformListener tf_listener(tf_buffer);


    /** Set publisher **/

    //// pedestrian objects to phone
    pedestrian2phone_objects.resize(phone_names.size());
    pubs_pedestrian2phone.resize(phone_names.size());
    for(size_t i=0;i<phone_names.size();i++){
        pubs_pedestrian2phone[i] = this->nh.advertise<autoware_msgs::DetectedObject>(topic_names_pedestrian2phone[i],1);
    }


    /** Set subscriber **/

    //// vehicle perception
    sub_objects=this->nh.subscribe(topic_name_objects,1, &VehiclePhonesManager::callback_objects,this);

    //// phones particles
    phone_odoms.resize(phone_names.size());
    update_phone_odoms=std::vector<bool>(phone_names.size(),false);
    subs_phone_odoms.resize(phone_names.size());
    for(size_t i=0;i<phone_names.size();i++) {
        subs_phone_odoms[i]=this->nh.subscribe(topic_names_phone_odom[i],1,&VehiclePhonesManager::callback_phone_odom, this);
    }
}



VehiclePhonesManager::~VehiclePhonesManager() {}

void VehiclePhonesManager::callback_objects(const autoware_msgs::DetectedObjectArrayConstPtr &msg) {

    ROS_INFO("%s",__FUNCTION__);

    objects=*msg;
    time=msg->header.stamp;
    update_vehicle_perception=true;
    run();

}

void VehiclePhonesManager::callback_phone_odom(const nav_msgs::OdometryConstPtr &msg) {

    int idx_origin=msg->header.frame_id.find(origin_frame_id);
    std::string phone_name(msg->header.frame_id.c_str(),msg->header.frame_id.c_str()+idx_origin-1);

    ROS_INFO("%s,%s",__FUNCTION__,phone_name.c_str());

    auto itr_phone_name=std::find(phone_names.begin(),phone_names.end(),phone_name);
    int idx_phone=std::distance(phone_names.begin(),itr_phone_name);
    assert(itr_phone_name!=phone_names.end());

    phone_odoms[idx_phone]=*msg;
    time=msg->header.stamp;
    update_phone_odoms[idx_phone]=true;
    run();

}

void VehiclePhonesManager::publish_pedestrian2phone_objects() {

    for(size_t i_ph=0;i_ph<phone_names.size();i_ph++){
        pubs_pedestrian2phone[i_ph].publish(pedestrian2phone_objects[i_ph]);
    }

}

void VehiclePhonesManager::run() {

    bool update_all_phone_odoms=std::find(update_phone_odoms.begin(),update_phone_odoms.end(),false)==update_phone_odoms.end();
    if( !update_all_phone_odoms || !update_vehicle_perception){
        return;
    }

    ROS_DEBUG("%s",__FUNCTION__);

    getPedestrianObjectsFromObjects();
    convertPedestrianObjects2States();
    if(!pedestrian_states.empty()) {
        convertPhoneOdom2States();
        assignPedestrians2Phones();
    }

    publish_pedestrian2phone_objects();

}

void VehiclePhonesManager::getPedestrianObjectsFromObjects() {

    ROS_DEBUG("%s", __FUNCTION__);

    /** Extracting pedestrian object in Detected objects **/
    pedestrian_objects.header=objects.header;
    pedestrian_objects.objects.clear();

    for(const autoware_msgs::DetectedObject& obj:objects.objects){
        if (obj.label=="pedestrian"){
            pedestrian_objects.objects.push_back(obj);
        }
    }

    ROS_DEBUG_STREAM("Pedestrian size in objects: "<< pedestrian_objects.objects.size());
}

void VehiclePhonesManager::convertPedestrianObjects2States(){

    ROS_DEBUG("%s", __FUNCTION__);

    pedestrian_states=std::vector<State<>>(pedestrian_objects.objects.size());

    for(size_t i_pe=0;i_pe<pedestrian_objects.objects.size();i_pe++){
        autoware_msgs::DetectedObject obj=pedestrian_objects.objects[i_pe];

        if (std::isnan(obj.pose.orientation.x) ||
            std::isnan(obj.pose.orientation.y) ||
            std::isnan(obj.pose.orientation.z) ||
            std::isnan(obj.pose.orientation.w) ||
            obj.pose.orientation.x == obj.pose.orientation.y == obj.pose.orientation.z == obj.pose.orientation.w == 0) {
            continue;
        }

        geometry_msgs::PoseStamped in, out;
        in.header = obj.header;
        in.pose = obj.pose;
        out.header.frame_id = origin_frame_id;
        out.header.stamp = in.header.stamp;
        geometry_msgs::TransformStamped transform = tf_buffer.lookupTransform(origin_frame_id, in.header.frame_id,
                                                                              in.header.stamp, ros::Duration(1.0));
        tf2::doTransform(in, out, transform);
        State<> pedestrian;
        pedestrian.x = out.pose.position.x;
        pedestrian.y = out.pose.position.y;
        pedestrian.vx = obj.velocity.linear.y;
        pedestrian.vy = obj.velocity.linear.x;
        tf::Quaternion quat(out.pose.orientation.x,
                            out.pose.orientation.y,
                            out.pose.orientation.z,
                            out.pose.orientation.w);
        double rpy[3];
        tf::Matrix3x3(quat).getRPY(rpy[0], rpy[1], rpy[2]);
        pedestrian.r = State<>::normalize_radian(rpy[2]);

        pedestrian_states[i_pe]=pedestrian;
    }

    ROS_DEBUG_STREAM("Pedestrian origin states: \n" << pedestrian_states);

}

void VehiclePhonesManager::convertPhoneOdom2States(){

    ROS_DEBUG("%s", __FUNCTION__);

    phone_states=std::vector<State<>>(phone_names.size(),State<>());

    for(size_t i_ph=0;i_ph<phone_names.size();i_ph++){
        nav_msgs::Odometry& odom=phone_odoms[i_ph];

        if (std::isnan(odom.pose.pose.orientation.x) ||
            std::isnan(odom.pose.pose.orientation.y) ||
            std::isnan(odom.pose.pose.orientation.z) ||
            std::isnan(odom.pose.pose.orientation.w) ||
            (odom.pose.pose.orientation.x==0.0 &&
            odom.pose.pose.orientation.y==0.0 &&
            odom.pose.pose.orientation.z==0.0 &&
            odom.pose.pose.orientation.w==0.0)){
            ROS_WARN_STREAM("Unable quaternion"<< odom.header.frame_id<<": \n"<<odom.pose.pose);
            continue;
        }

        geometry_msgs::PoseStamped in,out;
        in.header=odom.header;
        in.pose=odom.pose.pose;
        out.header.frame_id=origin_frame_id;
        out.header.stamp=in.header.stamp;
        std::cout<<origin_frame_id<<","<<in.header.frame_id<<std::endl;
        geometry_msgs::TransformStamped transform= tf_buffer.lookupTransform(origin_frame_id,in.header.frame_id,ros::Time::now(),ros::Duration(1.0));
        std::cout<<__FILE__<<","<<__LINE__<<std::endl;
        tf2::doTransform(in,out,transform);
        std::cout<<__FILE__<<","<<__LINE__<<std::endl;

        State<> phone_state;
        phone_state.x=out.pose.position.x;
        phone_state.y=out.pose.position.y;
        phone_state.vx=odom.twist.twist.linear.x;
        phone_state.vy=odom.twist.twist.linear.y;
        tf::Quaternion quat(out.pose.orientation.x,
                            out.pose.orientation.y,
                            out.pose.orientation.z,
                            out.pose.orientation.w);
        double rpy[3];
        tf::Matrix3x3(quat).getRPY(rpy[0], rpy[1], rpy[2]);
        phone_state.r=State<>::normalize_radian(rpy[2]);

        phone_states[i_ph]=phone_state;
    }

}

void VehiclePhonesManager::assignPedestrians2Phones() {

    ROS_DEBUG("%s",__FUNCTION__);

    //// set rosparam
    double w_xy,w_v,w_r;
    private_nh.param("assign_pedestrians2phone_weight_xy",w_xy,ASSIGN_PEDESTRIANS2PHONE_WEIGHT_XY);
    private_nh.param("assign_pedestrians2phone_weight_v",w_v,ASSIGN_PEDESTRIANS2PHONE_WEIGHT_V);
    private_nh.param("assign_pedestrians2phone_weight_r",w_r,ASSIGN_PEDESTRIANS2PHONE_WEIGHT_R);


    /** Create Weight Table **/
    Eigen::MatrixXd weight_table(pedestrian_states.size(),phone_names.size());

    for(size_t i_pe=0;i_pe<pedestrian_states.size();i_pe++) {
        for(size_t i_ph=0;i_ph<phone_names.size();i_ph++){
            if(phone_states[i_ph]==State<>()){
                continue;
                std::cout<<__FILE__<<","<<__LINE__<<std::endl;
            }
            std::cout<<__FILE__<<","<<__LINE__<<std::endl;
            State<> ds=pedestrian_states[i_pe]-phone_states[i_ph];
            double d_xy=std::sqrt(ds.x*ds.x+ds.y*ds.y);
            double d_v=std::sqrt(ds.vx*ds.vx+ds.vy*ds.vy);
            double d_r=ds.r;
            weight_table(i_pe,i_ph)=w_xy*d_xy+w_v*d_v+w_r*d_r;
        }
    }


    ROS_DEBUG_STREAM("Weight Table: \n" << weight_table);

    /** Assign Pedestrians to Phones **/
    //// Find max index
    pedestrian2phone_states.clear();
    pedestrian2phone_objects.clear();
    pedestrian2phone_states.resize(phone_names.size());
    pedestrian2phone_objects.resize(phone_names.size());
    Eigen::MatrixXd weight_table_tmp=weight_table;

    if (weight_table.isZero()){
        return;
    }

    while(true){
        Eigen::MatrixXd::Index idx_max_pe,idx_max_ph;
        std::cout<<__FILE__<<","<<__LINE__<<std::endl;
        weight_table_tmp.maxCoeff(&idx_max_pe,&idx_max_ph);
        std::cout<<__FILE__<<","<<__LINE__<<std::endl;
        pedestrian2phone_states[idx_max_ph]=pedestrian_states[idx_max_pe];
        std::cout<<__FILE__<<","<<__LINE__<<std::endl;
        pedestrian2phone_objects[idx_max_ph]=pedestrian_objects.objects[idx_max_pe];
        std::cout<<__FILE__<<","<<__LINE__<<std::endl;
        weight_table_tmp.row(idx_max_pe).setZero();
        weight_table_tmp.col(idx_max_ph).setZero();
        std::cout<<__FILE__<<","<<__LINE__<<std::endl;
        bool is_assign_to_all_phones=std::find(pedestrian2phone_states.begin(),pedestrian2phone_states.end(),State<>())==pedestrian2phone_states.end();
        std::cout<<__FILE__<<","<<__LINE__<<std::endl;
        if(is_assign_to_all_phones){
            std::cout<<__FILE__<<","<<__LINE__<<std::endl;
            assert(weight_table_tmp.isZero());
            break;
        }
    }
}


/**
 * Main Function
 */

int main(int argc, char **argv) {

//    // debug mode
//    if (ros::console::set_logger_level(ROSCONSOLE_DEFAULT_NAME, ros::console::levels::Debug)) {
//        ros::console::notifyLoggerLevelsChanged();
//    }

    //// Initialize ROS
    ros::init(argc, argv, "vehicle_phones_manager");
    ros::NodeHandle nh,private_nh("~");
    VehiclePhonesManager node(&nh,&private_nh);

    ros::spin();

    return (0);
}