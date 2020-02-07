//
// Created by ohashi on 19/09/13.
//


#include <vehicle_phones_manager.h>

VehiclePhonesManager::VehiclePhonesManager(ros::NodeHandle *nh, ros::NodeHandle *private_nh) :
        nh(*nh), private_nh(*private_nh), update_vehicle_perception(false), update_vehicle_odom(false) {

    /** Set Parameter **/
    //// vehicle name
    if (!this->private_nh.getParam("vehicle_name", vehicle_name)) {
        ROS_ERROR("Non \"vehicle_name\"");
        exit(-1);
    }
    //// phone names
    if (!this->private_nh.getParam("phone_names", phone_names)) {
        ROS_ERROR("Non \"phone_names\"");
        exit(-1);
    }
    //// topic names
    if (!this->private_nh.getParam("topic_name_sub_objects", topic_name_sub_objects)) {
        ROS_ERROR("Non \"topic_name_sub_objects\"");
        exit(-1);
    }
    if (!this->private_nh.getParam("topic_name_sub_vehicle_odom", topic_name_sub_vehicle_odom)) {
        ROS_ERROR("Non \"topic_name_sub_vehicle_odom\"");
        exit(-1);
    }
    std::string topic_name_sub_phone_odom, topic_name_pub_pedestrian2phone;
    if (!this->private_nh.getParam("topic_name_sub_phone_odom", topic_name_sub_phone_odom)) {
        ROS_ERROR("Non \"topic_name_sub_phone_odom\"");
        exit(-1);
    }
    if (!this->private_nh.getParam("topic_name_pub_pedestrian2phone", topic_name_pub_pedestrian2phone)) {
        ROS_ERROR("Non \"topic_name_pub_pedestrian2phone\"");
        exit(-1);
    }
    for (const auto &phone_name:phone_names) {
        topic_names_sub_phone_odom.push_back("/android/" + phone_name + "/" + topic_name_sub_phone_odom);
        topic_names_pub_pedestrian2phone.push_back("/android/" + phone_name + "/" + topic_name_pub_pedestrian2phone);
    }


    //// origin frame_id
    this->nh.param<std::string>("origin_frame_id", origin_frame_id, ORIGIN_FRAME_ID);

    //// TF Listener
    static tf2_ros::TransformListener tf_listener(tf_buffer);

    /** Set publisher **/

    //// pedestrian objects to phone
    pedestrian2phone_objects.resize(phone_names.size());
    pubs_pedestrian2phone.resize(phone_names.size());
    for (size_t i = 0; i < phone_names.size(); i++) {
        pubs_pedestrian2phone[i] = this->nh.advertise<geometry_msgs::PoseWithCovarianceStamped>(
                topic_names_pub_pedestrian2phone[i], 10);
    }


    /** Set subscriber **/

    //// vehicle perception
    sub_objects = this->nh.subscribe(topic_name_sub_objects, 1, &VehiclePhonesManager::callback_objects, this);

    //// vehicle odom
    sub_vehicle_odom = this->nh.subscribe(topic_name_sub_vehicle_odom, 1, &VehiclePhonesManager::callback_vehicle_odom,
                                          this);

    //// phones particles
    phone_odoms.resize(phone_names.size());
    update_phone_odoms = std::vector<bool>(phone_names.size(), false);
    subs_phone_odom.resize(phone_names.size());
    for (size_t i = 0; i < phone_names.size(); i++) {
        subs_phone_odom[i].set_subscriber(this->nh, topic_names_sub_phone_odom[i]);
    }

    std::string path =
            "/home/ohashi/Dropbox/MATLAB/phones_vehicle_fusion" + this->nh.getNamespace().substr(1);
    ofs_pedestrian2phone.resize(phone_names.size());
    for (size_t i = 0; i < phone_names.size(); i++) {
        ofs_pedestrian2phone[i].open(path + "/" + phone_names[i] + "_pedestrian2phone.csv");
        ofs_pedestrian2phone[i] << "%time,x,y,r,vx,vy,xy_cov00,xy_cov01,xy_cov10,xy_cov11" << std::endl;
    }
    ofs_vehicle.open(path + "/vehicle.csv");
    ofs_vehicle
            << "%time,x,y,r,xyr_cov00,xyr_cov01,xyr_cov02,xyr_cov10,xyr_cov11,xyr_cov12,xyr_cov20,xyr_cov21,xyr_cov22"
            << std::endl;

}


VehiclePhonesManager::~VehiclePhonesManager() {}

/**
 *
 * @param msg
 */
void VehiclePhonesManager::callback_objects(const autoware_msgs::DetectedObjectArrayConstPtr &msg) {

    objects = *msg;
    time = msg->header.stamp;
    ROS_DEBUG_STREAM("callback_objects: \n");
    update_vehicle_perception = true;
    run();

}

/**
 * callback_vehicle_odom
 * @param msg
 */
void VehiclePhonesManager::callback_vehicle_odom(const nav_msgs::Odometry::ConstPtr &msg) {

    vehicle_odom = *msg;
    time = msg->header.stamp;
    ROS_DEBUG_STREAM("callback_vehicle_odom: \n" << vehicle_odom);
    update_vehicle_odom = true;

}

//void VehiclePhonesManager::callback_phone_odom(const nav_msgs::OdometryConstPtr &msg) {
//
//    int idx_origin=msg->header.frame_id.find(origin_frame_id);
//    std::string phone_name(msg->header.frame_id.c_str(),msg->header.frame_id.c_str()+idx_origin-1);
//
//    ROS_INFO("%s,%s",__FUNCTION__,phone_name.c_str());
//
//    auto itr_phone_name=std::find(phone_names.begin(),phone_names.end(),phone_name);
//    int idx_phone=std::distance(phone_names.begin(),itr_phone_name);
//    assert(itr_phone_name!=phone_names.end());
//
//    phone_odoms[idx_phone]=*msg;
//    time=msg->header.stamp;
//    update_phone_odoms[idx_phone]=true;
//    run();
//
//}

void VehiclePhonesManager::publish_pedestrian2phone_objects() {

    for (size_t i_ph = 0; i_ph < phone_names.size(); i_ph++) {
        geometry_msgs::PoseWithCovarianceStamped pose;
        pose.header.frame_id = origin_frame_id;
        pose.header.stamp = time;

        pose.pose.pose.position.x = pedestrian2phone_states[i_ph].x;
        pose.pose.pose.position.y = pedestrian2phone_states[i_ph].y;
        pose.pose.covariance[0] = pedestrian2phone_covariances[i_ph](0, 0);
        pose.pose.covariance[1] = pedestrian2phone_covariances[i_ph](0, 1);
        pose.pose.covariance[6] = pedestrian2phone_covariances[i_ph](1, 0);
        pose.pose.covariance[7] = pedestrian2phone_covariances[i_ph](1, 1);
        pubs_pedestrian2phone[i_ph].publish(pose);

        //// PoseWithCovarianceに変更
        ofs_pedestrian2phone[i_ph] << time.sec << "." << time.nsec << "," << pedestrian2phone_states[i_ph].x << ","
                                   << pedestrian2phone_states[i_ph].y << "," << pedestrian2phone_states[i_ph].r << ","
                                   << pedestrian2phone_states[i_ph].vx << "," << pedestrian2phone_states[i_ph].vy << ","
                                   << pedestrian2phone_covariances[i_ph](0, 0) << ","
                                   << pedestrian2phone_covariances[i_ph](0, 1) << ","
                                   << pedestrian2phone_covariances[i_ph](1, 0) << ","
                                   << pedestrian2phone_covariances[i_ph](1, 1)
                                   << std::endl;
        ROS_DEBUG_STREAM("Published Pedestrian Object to " << phone_names[i_ph] << ": \n"
                                                           << pedestrian2phone_states[i_ph]);
    }

}

void VehiclePhonesManager::run() {

    for (auto &sub:subs_phone_odom) {
        if (!sub.isUpdate()) {
            return;
        }
    }

    for (int i = 0; i < phone_names.size(); i++) {
        phone_odoms[i] = subs_phone_odom[i].getMsg();
    }


    ROS_DEBUG("%s", __FUNCTION__);

    getPedestrianObjectsFromObjects();
    if(!convertPedestrianObjects2States()){
        return;
    }
    generatePedestrianCovariances();
    //// TODO: generatePedestrianCovariance();
    if (!pedestrian_states.empty()) {
        convertPhoneOdom2States();
        assignPedestrians2Phones();
    } else {
        return;
    }

    publish_pedestrian2phone_objects();

    for (auto &sub:subs_phone_odom) {
        sub.setUpdate(false);
    }

    ROS_DEBUG("%s Completed", __FUNCTION__);

}

void VehiclePhonesManager::getPedestrianObjectsFromObjects() {

//    ROS_DEBUG("%s", __FUNCTION__);

    /** Extracting pedestrian object in Detected objects **/
    pedestrian_objects.header = objects.header;
    pedestrian_objects.objects.clear();

    for (const autoware_msgs::DetectedObject &obj:objects.objects) {
        if (obj.label == "pedestrian") {
            pedestrian_objects.objects.push_back(obj);
        }
    }

    for (size_t i = 0; i < pedestrian_objects.objects.size(); i++) {
        ROS_DEBUG_STREAM("Pedestrian" << i << " from " << vehicle_name << ": \n"
                                      << pedestrian_objects.objects[i].pose);
    }
}

/**
 *
 * @return
 */
bool VehiclePhonesManager::convertPedestrianObjects2States() {

//    ROS_DEBUG("%s", __FUNCTION__);

    pedestrian_states.clear();

    for (size_t i_pe = 0; i_pe < pedestrian_objects.objects.size(); i_pe++) {
        autoware_msgs::DetectedObject obj = pedestrian_objects.objects[i_pe];

        geometry_msgs::PoseStamped in, out;
        in.header = obj.header;
        in.pose = obj.pose;
        out.header.frame_id = origin_frame_id;
        out.header.stamp = in.header.stamp;
        try {
            geometry_msgs::TransformStamped transform = tf_buffer.lookupTransform(origin_frame_id, in.header.frame_id,
                                                                                  in.header.stamp, ros::Duration(1.0));
            if (isOrientationActive(in.pose.orientation))
                tf2::doTransform(in, out, transform);
            else
                tf2::doTransform(in.pose.position, out.pose.position, transform);
        } catch (tf2::TransformException &ex) {
            ROS_WARN("Could NOT transform %s to %s: %s", obj.header.frame_id.c_str(), origin_frame_id.c_str(),
                     ex.what());
            return false;
        }
        State<> pedestrian;
        pedestrian.x = out.pose.position.x;
        pedestrian.y = out.pose.position.y;
        pedestrian.vx = obj.velocity.linear.x;
        pedestrian.vy = obj.velocity.linear.y;
        if (isOrientationActive(out.pose.orientation)) {
            tf::Quaternion quat(out.pose.orientation.x, out.pose.orientation.y, out.pose.orientation.z,
                                out.pose.orientation.w);
            double rpy[3];
            tf::Matrix3x3(quat).getRPY(rpy[0], rpy[1], rpy[2]);
            pedestrian.r = State<>::normalize_radian(rpy[2]);
        }

        pedestrian_states.push_back(pedestrian);
    }

    ROS_DEBUG_STREAM("Pedestrian Origin States: \n" << pedestrian_states);

    return true;
}

/**
 * generatePedestrianCovariances()
 */
void VehiclePhonesManager::generatePedestrianCovariances() {

    pedestrian_covariances = std::vector<Eigen::MatrixXd>(pedestrian_states.size());

    double vsd_coff, psd;
    private_nh.param("vehicle_standard_deviation_coefficient", vsd_coff, VEHICLE_STANDARD_DEVIATION_COEFFICIENT);
    private_nh.param("pedestrian_standard_deviation", psd, PEDESTRIAN_STANDARD_DEVIATION);

    Eigen::MatrixXd v_sigma = Eigen::MatrixXd(3, 3);
    double vehicle_rpy[3];
    if (update_vehicle_odom) {
        v_sigma << vehicle_odom.pose.covariance[0], vehicle_odom.pose.covariance[1], vehicle_odom.pose.covariance[5],
                vehicle_odom.pose.covariance[6], vehicle_odom.pose.covariance[7], vehicle_odom.pose.covariance[11],
                vehicle_odom.pose.covariance[30], vehicle_odom.pose.covariance[31], vehicle_odom.pose.covariance[35];
        v_sigma*=vsd_coff;
        tf::Quaternion quat(vehicle_odom.pose.pose.orientation.x, vehicle_odom.pose.pose.orientation.y,
                            vehicle_odom.pose.pose.orientation.z, vehicle_odom.pose.pose.orientation.w);
        tf::Matrix3x3(quat).getRPY(vehicle_rpy[0], vehicle_rpy[1], vehicle_rpy[2]);
    } else {
        v_sigma = Eigen::MatrixXd::Zero(3, 3);
        vehicle_rpy[0]=vehicle_rpy[1]=vehicle_rpy[2]=0.0;
    }

    Eigen::MatrixXd p_sigma = Eigen::MatrixXd::Identity(2, 2) * psd * psd;

    ///// Vehicle Coordinateの歩行者を変換すること
    for (size_t i_pe = 0; i_pe < pedestrian_objects.objects.size(); i_pe++) {
        autoware_msgs::DetectedObject obj = pedestrian_objects.objects[i_pe];

        Eigen::MatrixXd transform_v2p(2, 3), transform_pl2pw(2, 2);
        transform_v2p << 1.0, 0.0, -obj.pose.position.x * sin(vehicle_rpy[2]) -
                                   obj.pose.position.y * cos(vehicle_rpy[2]),
                0.0, 1.0, obj.pose.position.x * cos(vehicle_rpy[2]) - obj.pose.position.y * sin(vehicle_rpy[2]);
        transform_pl2pw << cos(vehicle_rpy[2]), -sin(vehicle_rpy[2]), sin(vehicle_rpy[2]), cos(vehicle_rpy[2]);
        pedestrian_covariances[i_pe] = transform_v2p * v_sigma * transform_v2p.transpose() +
                                       transform_pl2pw * p_sigma * transform_pl2pw.transpose();

    }

    ofs_vehicle << time.sec << "." << time.nsec << "," << vehicle_odom.pose.pose.position.x << ","
                << vehicle_odom.pose.pose.position.y << "," << vehicle_rpy[2] << ","
                << v_sigma(0, 0) << "," << v_sigma(0, 1) << "," << v_sigma(0, 2) << ","
                << v_sigma(1, 0) << "," << v_sigma(1, 1) << "," << v_sigma(1, 2) << ","
                << v_sigma(2, 0) << "," << v_sigma(2, 1) << "," << v_sigma(2, 2) << std::endl;

}
//// TODO: VehiclePhonesManager::generatePedestrianCovariance() vehicleのステートを座標変換しCovarianceMatrix作成，imm_trackerを参照

/**
 *
 */
void VehiclePhonesManager::convertPhoneOdom2States() {

//    ROS_DEBUG("%s", __FUNCTION__);

    phone_states = std::vector<State<>>(phone_names.size(), State<>());

    for (size_t i_ph = 0; i_ph < phone_names.size(); i_ph++) {
        nav_msgs::Odometry &odom = phone_odoms[i_ph];

/*
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
*/

        State<> phone_state;
        phone_state.x = odom.pose.pose.position.x;
        phone_state.y = odom.pose.pose.position.y;
        phone_state.vx = odom.twist.twist.linear.x;
        phone_state.vy = odom.twist.twist.linear.y;
        tf::Quaternion quat(odom.pose.pose.orientation.x,
                            odom.pose.pose.orientation.y,
                            odom.pose.pose.orientation.z,
                            odom.pose.pose.orientation.w);
        double rpy[3];
        tf::Matrix3x3(quat).getRPY(rpy[0], rpy[1], rpy[2]);
        phone_state.r = State<>::normalize_radian(rpy[2]);

        phone_states[i_ph] = phone_state;
    }

    ROS_DEBUG_STREAM("Phone States: \n" << phone_states);

}

/**
 *
 */
void VehiclePhonesManager::assignPedestrians2Phones() {

    ROS_DEBUG("%s", __FUNCTION__);

    //// set rosparam
    double w_xy, w_v, w_r;
    private_nh.param("assign_pedestrians2phone_weight_xy", w_xy, ASSIGN_PEDESTRIANS2PHONE_WEIGHT_XY);
    private_nh.param("assign_pedestrians2phone_weight_v", w_v, ASSIGN_PEDESTRIANS2PHONE_WEIGHT_V);
    private_nh.param("assign_pedestrians2phone_weight_r", w_r, ASSIGN_PEDESTRIANS2PHONE_WEIGHT_R);


    /** Create Weight Table **/
    Eigen::MatrixXd weight_table(pedestrian_states.size(), phone_names.size());

    for (size_t i_pe = 0; i_pe < pedestrian_states.size(); i_pe++) {
        for (size_t i_ph = 0; i_ph < phone_names.size(); i_ph++) {
            if (phone_states[i_ph] == State<>()) {
                continue;
            }
            State<> ds = pedestrian_states[i_pe] - phone_states[i_ph];
            double w = 0.0;
            w += w_xy / std::sqrt(ds.x * ds.x + ds.y * ds.y);
//            w+=w_v*std::sqrt(ds.vx*ds.vx+ds.vy*ds.vy);
//            w+=w_r*ds.r;
            weight_table(i_pe, i_ph) = w;
        }
    }

    ROS_DEBUG_STREAM("Weight Table: \n" << weight_table);


    /** Assign Pedestrians to Phones **/

    //// Find max index
    pedestrian2phone_states = std::vector<State<>>(phone_names.size());
    pedestrian2phone_covariances = std::vector<Eigen::MatrixXd>(phone_names.size());
    pedestrian2phone_objects = std::vector<autoware_msgs::DetectedObject>(phone_names.size());
    Eigen::MatrixXd weight_table_tmp = weight_table;

    if (weight_table.isZero()) {
        return;
    }

    while (true) {
        Eigen::MatrixXd::Index idx_max_pe, idx_max_ph;
        weight_table_tmp.maxCoeff(&idx_max_pe, &idx_max_ph);
        pedestrian2phone_states[idx_max_ph] = pedestrian_states[idx_max_pe];
        pedestrian2phone_covariances[idx_max_ph] = pedestrian_covariances[idx_max_pe];
        pedestrian2phone_objects[idx_max_ph] = pedestrian_objects.objects[idx_max_pe];
        weight_table_tmp.row(idx_max_pe).setZero();
        weight_table_tmp.col(idx_max_ph).setZero();
        bool is_assign_to_all_phones =
                std::find(pedestrian2phone_states.begin(), pedestrian2phone_states.end(), State<>()) ==
                pedestrian2phone_states.end();
        if (is_assign_to_all_phones) {
            assert(weight_table_tmp.isZero());
            break;
        }
    }

    for (size_t i = 0; i < pedestrian2phone_objects.size(); i++) {
        ROS_DEBUG_STREAM("Pedestrian Assigned to " << phone_names[i] << ": \n"
                                                   << pedestrian2phone_states[i]);
    }

}

/**
 *
 * @param q
 * @return
 */
bool VehiclePhonesManager::isOrientationActive(const geometry_msgs::Quaternion &q) {

    if (std::isnan(q.x) || std::isnan(q.y) || std::isnan(q.z) || std::isnan(q.w) ||
        (q.x == 0.0 && q.y == 0.0 && q.z == 0.0 && q.w == 0.0)) {
        return false;
    } else {
        return true;
    }

}


/**
 * Main Functioin
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char **argv) {

    //// Initialize ROS
    ros::init(argc, argv, "vehicle_phones_manager");
    ros::NodeHandle nh, private_nh("~");
    // debug mode
    bool debug;
    private_nh.param<bool>("debug", debug, false);
    if (debug) {
        if (ros::console::set_logger_level(ROSCONSOLE_DEFAULT_NAME, ros::console::levels::Debug)) {
            ros::console::notifyLoggerLevelsChanged();
        }
    }
    VehiclePhonesManager node(&nh, &private_nh);

    ros::spin();

    return (0);
}