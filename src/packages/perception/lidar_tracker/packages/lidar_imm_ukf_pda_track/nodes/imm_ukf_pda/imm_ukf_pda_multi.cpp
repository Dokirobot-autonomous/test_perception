/*
 * Copyright 2018-2019 Autoware Foundation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "imm_ukf_pda_multi.h"

ImmUkfPdaMulti::ImmUkfPdaMulti()
        : target_id_(0),  // assign unique ukf_id_ to each tracking targets
          init_(false),
          frame_count_(0),
          has_subscribed_vectormap_(false),
          private_nh_("~") {
    private_nh_.param<std::string>("tracking_frame", tracking_frame_, "world");
    private_nh_.param<int>("life_time_thres", life_time_thres_, 8);
    private_nh_.param<double>("gating_thres", gating_thres_, 9.22);
    private_nh_.param<double>("gate_probability", gate_probability_, 0.99);
    private_nh_.param<double>("detection_probability", detection_probability_, 0.9);
    private_nh_.param<double>("static_velocity_thres", static_velocity_thres_, 0.5);
    private_nh_.param<int>("static_velocity_history_thres", static_num_history_thres_, 3);
    private_nh_.param<double>("prevent_explosion_thres", prevent_explosion_thres_, 1000);
    private_nh_.param<double>("merge_distance_threshold", merge_distance_threshold_, 0.5);
    private_nh_.param<bool>("use_sukf", use_sukf_, false);

    // for vectormap assisted tracking
    private_nh_.param<bool>("use_vectormap", use_vectormap_, false);
    private_nh_.param<double>("lane_direction_chi_thres", lane_direction_chi_thres_, 2.71);
    private_nh_.param<double>("nearest_lane_distance_thres", nearest_lane_distance_thres_, 1.0);
    private_nh_.param<std::string>("vectormap_frame", vectormap_frame_, "map");

    // rosparam for benchmark
    private_nh_.param<bool>("is_benchmark", is_benchmark_, false);
    private_nh_.param<std::string>("kitti_data_dir", kitti_data_dir_, "");

    if (!private_nh_.getParam("topic_names_odom", topic_names_odom_)) {
        ROS_ERROR("Non \"topic_names_odom\"");
        exit(-1);
    }
    if (!private_nh_.getParam("topic_names_objects", topic_names_objects_)) {
        ROS_ERROR("Non \"topic_names_objects\"");
        exit(-1);
    }

    ofs_.open("/home/ohashi/test_perception/matlab/workspace/cooperative_tracking/log.csv");
    ofs_ << "% timestamp,label,id,x,y,sigma_x,sigma_xy,sigma_yx,sigma_y" << std::endl;

    // debug mode
    bool debug;
    private_nh_.param<bool>("debug", debug, false);
    if (debug) {
        if (ros::console::set_logger_level(ROSCONSOLE_DEFAULT_NAME, ros::console::levels::Debug)) {
            ros::console::notifyLoggerLevelsChanged();
        }
    }


    if (is_benchmark_) {
        result_file_path_ = kitti_data_dir_ + "benchmark_results.txt";
        std::remove(result_file_path_.c_str());
    }
}

void ImmUkfPdaMulti::run() {
    pub_object_array_ = node_handle_.advertise<autoware_msgs::DetectedObjectArray>("/detection/objects", 10);
    sub_detected_array_mks_ = node_handle_.subscribe(topic_names_objects_[0], 10,
                                                     &ImmUkfPdaMulti::callback, this);
    sub_detected_array_mkz_ = node_handle_.subscribe(topic_names_objects_[1], 10,
                                                     &ImmUkfPdaMulti::callback, this);
    sub_detected_array_prius_ = node_handle_.subscribe(topic_names_objects_[2], 10,
                                                       &ImmUkfPdaMulti::callback, this);
    sub_odom_mks_ = node_handle_.subscribe(topic_names_odom_[0], 10,
                                           &ImmUkfPdaMulti::callback_odom, this);
    sub_odom_mkz_ = node_handle_.subscribe(topic_names_odom_[1], 10,
                                           &ImmUkfPdaMulti::callback_odom, this);
    sub_odom_prius_ = node_handle_.subscribe(topic_names_odom_[2], 10,
                                             &ImmUkfPdaMulti::callback_odom, this);
    msg_odom_.resize(topic_names_odom_.size());
    p_vehicle_ = std::vector<Eigen::MatrixXd>(topic_names_odom_.size(), Eigen::MatrixXd::Zero(3, 3));

    if (use_vectormap_) {
        vmap_.subscribe(private_nh_, vector_map::Category::POINT |
                                     vector_map::Category::NODE |
                                     vector_map::Category::LANE, 1);
    }
}

/**
 *
 * @param event
 */
void ImmUkfPdaMulti::callback(const ros::MessageEvent<autoware_msgs::DetectedObjectArray const> &event) {

    const ros::M_string &header = event.getConnectionHeader();
    std::string topic = header.at("topic");
    tn_ = topic;
    int v_idx = std::distance(topic_names_objects_.begin(),
                              std::find(topic_names_objects_.begin(), topic_names_objects_.end(), topic));
    const autoware_msgs::DetectedObjectArray input = *(event.getMessage());

    input_header_ = input.header;

    {
        std::ostringstream ostr;
        ostr << input_header_ << std::endl;
        for (const autoware_msgs::DetectedObject &obj:input.objects) {
            ostr << obj.label << std::endl;
            ostr << obj.pose;
            ostr << obj.velocity;
        }
        ROS_DEBUG_STREAM("Input Objects: \n" << ostr.str());
    }

    if (use_vectormap_) {
        checkVectormapSubscription();
    }

    bool success = updateNecessaryTransform();
    if (!success) {
        ROS_WARN("Could not find coordiante transformation");
        return;
    }

    //// done: TODO inputのreliability_vecを計算（inputがある時点でreliabilityは確定する）
    std::vector<Eigen::MatrixXd> reliability_vec = std::vector<Eigen::MatrixXd>(input.objects.size(),
                                                                                Eigen::MatrixXd(2,
                                                                                                2)); // covariance matrix based on odom frame
    makeReliabilityVec(input, msg_odom_[v_idx], p_vehicle_[v_idx], reliability_vec);


    autoware_msgs::DetectedObjectArray transformed_input;
    autoware_msgs::DetectedObjectArray detected_objects_output;
    transformPoseToGlobal(input, transformed_input);

    //// done: TODO ２つのvehicleのinputを一つのtransformed_inputにまとめる．0.1秒周期でtrackerを更新
    static std::vector<autoware_msgs::DetectedObjectArray> transformed_input_vec(topic_names_objects_.size());
    static std::vector<std::vector<Eigen::MatrixXd>> reliability_vec_vec(topic_names_objects_.size());
    static std::vector<bool> update(topic_names_objects_.size(), false);
    transformed_input_vec[v_idx] = transformed_input;
    reliability_vec_vec[v_idx] = reliability_vec;
    update[v_idx] = true;
    if (!update[0] || !update[1]) {
        return;
    }
    autoware_msgs::DetectedObjectArray transformed_input_all;
    transformed_input_all.header = input_header_;
    for (size_t i = 0; i < transformed_input_vec.size(); i++) {
        for (size_t j = 0; j < transformed_input_vec[i].objects.size(); j++) {
            transformed_input_all.objects.push_back(transformed_input_vec[i].objects[j]);
            reliability_vec_.push_back(reliability_vec_vec[i][j]);
        }
    }

    tracker(transformed_input_all, detected_objects_output);
    detected_objects_output.header = input_header_;
//    transformPoseToLocal(detected_objects_output);

    autoware_msgs::DetectedObjectArray tmp;
    for (auto &obj:detected_objects_output.objects) {
        if (std::isnan(obj.pose.orientation.x) ||
            std::isnan(obj.pose.orientation.y) ||
            std::isnan(obj.pose.orientation.z) ||
            std::isnan(obj.pose.orientation.w)) {
            obj.pose.orientation.x = obj.pose.orientation.y = obj.pose.orientation.z = obj.pose.orientation.w = 0.0;
        }
        if (obj.label == "pedestrian") {
            tmp.objects.push_back(obj);
        }
    }
    tmp.header = input_header_;
    tmp.header.frame_id = tracking_frame_;
    for (auto &obj:tmp.objects) {
        obj.header = tmp.header;
    }
    detected_objects_output = tmp;

    pub_object_array_.publish(detected_objects_output);

    {
        std::ostringstream ostr;
        ostr << detected_objects_output.header << std::endl;
        for (const autoware_msgs::DetectedObject &obj:detected_objects_output.objects) {
            ostr << obj.label << std::endl;
            ostr << obj.pose;
            ostr << obj.velocity;
        }
        ROS_DEBUG_STREAM("Output Objects: \n" << ostr.str());
    }

    if (is_benchmark_) {
        dumpResultText(detected_objects_output);
    }

    transformed_input_vec = std::vector<autoware_msgs::DetectedObjectArray>(topic_names_objects_.size());
    reliability_vec_vec = std::vector<std::vector<Eigen::MatrixXd>>(topic_names_objects_.size());
    reliability_vec_ = std::vector<Eigen::MatrixXd>(0);
    update = std::vector<bool>(topic_names_objects_.size(), false);
}

/**
 * callback_odom
 * @param input
 */
void ImmUkfPdaMulti::callback_odom(const ros::MessageEvent<nav_msgs::Odometry const> &event) {

    //// done: TODO Vehicle ごとにOdometryを入手し，ClassMemberとして保存
    const ros::M_string &header = event.getConnectionHeader();
    std::string topic = header.at("topic");
    int idx = std::distance(topic_names_odom_.begin(),
                            std::find(topic_names_odom_.begin(), topic_names_odom_.end(), topic));
    msg_odom_[idx] = *(event.getMessage());
    p_vehicle_[idx] = Eigen::MatrixXd(3, 3);
    p_vehicle_[idx] <<
                    msg_odom_[idx].pose.covariance[0], msg_odom_[idx].pose.covariance[1], msg_odom_[idx].pose.covariance[5],
            msg_odom_[idx].pose.covariance[6], msg_odom_[idx].pose.covariance[7], msg_odom_[idx].pose.covariance[11],
            msg_odom_[idx].pose.covariance[30], msg_odom_[idx].pose.covariance[31], msg_odom_[idx].pose.covariance[35];
    ROS_DEBUG_STREAM(
            "[" << std::string(__FUNCTION__) << "]\n" << msg_odom_[idx] << "covariance: \n" << p_vehicle_[idx]);

}

void ImmUkfPdaMulti::checkVectormapSubscription() {
    if (use_vectormap_ && !has_subscribed_vectormap_) {
        lanes_ = vmap_.findByFilter([](const vector_map_msgs::Lane &lane) { return true; });
        if (lanes_.empty()) {
            ROS_INFO("Has not subscribed vectormap");
        } else {
            has_subscribed_vectormap_ = true;
        }
    }
}

bool ImmUkfPdaMulti::updateNecessaryTransform() {
    bool success = true;
    try {
        tf_listener_.waitForTransform(input_header_.frame_id, tracking_frame_, ros::Time(0), ros::Duration(1.0));
        tf_listener_.lookupTransform(tracking_frame_, input_header_.frame_id, ros::Time(0), local2global_);
    }
    catch (tf::TransformException ex) {
        ROS_ERROR("%s", ex.what());
        success = false;
    }
    if (use_vectormap_ && has_subscribed_vectormap_) {
        try {
            tf_listener_.waitForTransform(vectormap_frame_, tracking_frame_, ros::Time(0), ros::Duration(1.0));
            tf_listener_.lookupTransform(vectormap_frame_, tracking_frame_, ros::Time(0), tracking_frame2lane_frame_);
            tf_listener_.lookupTransform(tracking_frame_, vectormap_frame_, ros::Time(0), lane_frame2tracking_frame_);
        }
        catch (tf::TransformException ex) {
            ROS_ERROR("%s", ex.what());
        }
    }
    return success;
}

void ImmUkfPdaMulti::transformPoseToGlobal(const autoware_msgs::DetectedObjectArray &input,
                                           autoware_msgs::DetectedObjectArray &transformed_input) {
    transformed_input.header = input_header_;
    for (auto const &object: input.objects) {
        geometry_msgs::Pose out_pose = getTransformedPose(object.pose, local2global_);

        autoware_msgs::DetectedObject dd;
        dd.header = input.header;
        dd = object;
        dd.pose = out_pose;

        transformed_input.objects.push_back(dd);
    }
}

void ImmUkfPdaMulti::transformPoseToLocal(autoware_msgs::DetectedObjectArray &detected_objects_output) {
    detected_objects_output.header = input_header_;

    tf::Transform inv_local2global = local2global_.inverse();
    tf::StampedTransform global2local;
    global2local.setData(inv_local2global);
    for (auto &object : detected_objects_output.objects) {
        geometry_msgs::Pose out_pose = getTransformedPose(object.pose, global2local);
        object.header = input_header_;
        object.pose = out_pose;
    }
}

geometry_msgs::Pose ImmUkfPdaMulti::getTransformedPose(const geometry_msgs::Pose &in_pose,
                                                       const tf::StampedTransform &tf_stamp) {
    tf::Transform transform;
    geometry_msgs::PoseStamped out_pose;
    transform.setOrigin(tf::Vector3(in_pose.position.x, in_pose.position.y, in_pose.position.z));
    transform.setRotation(
            tf::Quaternion(in_pose.orientation.x, in_pose.orientation.y, in_pose.orientation.z, in_pose.orientation.w));
    geometry_msgs::PoseStamped pose_out;
    tf::poseTFToMsg(tf_stamp * transform, out_pose.pose);
    return out_pose.pose;
}

void ImmUkfPdaMulti::measurementValidation(const autoware_msgs::DetectedObjectArray &input, UKF &target,
                                           const bool second_init, const Eigen::VectorXd &max_det_z,
                                           const Eigen::MatrixXd &max_det_s,
                                           std::vector<autoware_msgs::DetectedObject> &object_vec,
                                           std::vector<bool> &matching_vec) {
    // alert: different from original imm-pda filter, here picking up most likely measurement
    // if making it allows to have more than one measurement, you will see non semipositive definite covariance
    bool exists_smallest_nis_object = false;
    double smallest_nis = std::numeric_limits<double>::max();
    double smallest_det = std::numeric_limits<double>::max();
    int smallest_nis_ind = 0;
    int smallest_det_ind = 0;
    for (size_t i = 0; i < input.objects.size(); i++) {
        double x = input.objects[i].pose.position.x;
        double y = input.objects[i].pose.position.y;

        Eigen::VectorXd meas = Eigen::VectorXd(2);
        meas << x, y;

        Eigen::VectorXd diff = meas - max_det_z;
        double nis = diff.transpose() * max_det_s.inverse() * diff;
        double det = reliability_vec_[i].determinant();

        //// done: TODO もともとあるTargetと新規detectionのNIS．Measの分散円が大きくてもOldが小さければ，一致しないとして弾かれる（必要ない）
        if (nis < gating_thres_) {
/*
            if (nis < smallest_nis) {
                smallest_nis = nis;
                target.object_ = input.objects[i];
                smallest_nis_ind = i;
                exists_smallest_nis_object = true;
            }
*/
            if (det < smallest_det) {
                smallest_det = det;
                target.object_ = input.objects[i];
                smallest_det_ind = i;
                exists_smallest_nis_object = true;
            }
        }
    }
    if (exists_smallest_nis_object) {
        //// TODO matchingのobjにsmallest_nis_indを使うか，smallest_det_indを使うかは今後検討．今のデータではどちらでも良いはず
/*
        matching_vec[smallest_nis_ind] = true;
*/
        matching_vec[smallest_det_ind] = true;
        if (use_vectormap_ && has_subscribed_vectormap_) {
            autoware_msgs::DetectedObject direction_updated_object;
            bool use_direction_meas =
                    updateDirection(smallest_nis, target.object_, direction_updated_object, target);
            if (use_direction_meas) {
                object_vec.push_back(direction_updated_object);
            } else {
                object_vec.push_back(target.object_);
            }
        } else {
            object_vec.push_back(target.object_);
        }
    }
}

bool ImmUkfPdaMulti::updateDirection(const double smallest_nis, const autoware_msgs::DetectedObject &in_object,
                                     autoware_msgs::DetectedObject &out_object, UKF &target) {
    bool use_lane_direction = false;
    target.is_direction_cv_available_ = false;
    target.is_direction_ctrv_available_ = false;
    bool get_lane_success = storeObjectWithNearestLaneDirection(in_object, out_object);
    if (!get_lane_success) {
        return use_lane_direction;
    }
    target.checkLaneDirectionAvailability(out_object, lane_direction_chi_thres_, use_sukf_);
    if (target.is_direction_cv_available_ || target.is_direction_ctrv_available_) {
        use_lane_direction = true;
    }
    return use_lane_direction;
}

bool ImmUkfPdaMulti::storeObjectWithNearestLaneDirection(const autoware_msgs::DetectedObject &in_object,
                                                         autoware_msgs::DetectedObject &out_object) {
    geometry_msgs::Pose lane_frame_pose = getTransformedPose(in_object.pose, tracking_frame2lane_frame_);
    double min_dist = std::numeric_limits<double>::max();

    double min_yaw = 0;
    for (auto const &lane : lanes_) {
        vector_map_msgs::Node node = vmap_.findByKey(vector_map::Key<vector_map_msgs::Node>(lane.bnid));
        vector_map_msgs::Point point = vmap_.findByKey(vector_map::Key<vector_map_msgs::Point>(node.pid));
        double distance = std::sqrt(std::pow(point.bx - lane_frame_pose.position.y, 2) +
                                    std::pow(point.ly - lane_frame_pose.position.x, 2));
        if (distance < min_dist) {
            min_dist = distance;
            vector_map_msgs::Node front_node = vmap_.findByKey(vector_map::Key<vector_map_msgs::Node>(lane.fnid));
            vector_map_msgs::Point front_point = vmap_.findByKey(
                    vector_map::Key<vector_map_msgs::Point>(front_node.pid));
            min_yaw = std::atan2((front_point.bx - point.bx), (front_point.ly - point.ly));
        }
    }

    bool success = false;
    if (min_dist < nearest_lane_distance_thres_) {
        success = true;
    } else {
        return success;
    }

    // map yaw in rotation matrix representation
    tf::Quaternion map_quat = tf::createQuaternionFromYaw(min_yaw);
    tf::Matrix3x3 map_matrix(map_quat);

    // vectormap_frame to tracking_frame rotation matrix
    tf::Quaternion rotation_quat = lane_frame2tracking_frame_.getRotation();
    tf::Matrix3x3 rotation_matrix(rotation_quat);

    // rotated yaw in matrix representation
    tf::Matrix3x3 rotated_matrix = rotation_matrix * map_matrix;
    double roll, pitch, yaw;
    rotated_matrix.getRPY(roll, pitch, yaw);

    out_object = in_object;
    out_object.angle = yaw;
    return success;
}

void ImmUkfPdaMulti::updateTargetWithAssociatedObject(const std::vector<autoware_msgs::DetectedObject> &object_vec,
                                                      UKF &target) {
    target.lifetime_++;
    if (!target.object_.label.empty() && target.object_.label != "unknown") {
        target.label_ = target.object_.label;
    }
    updateTrackingNum(object_vec, target);
    if (target.tracking_num_ == TrackingState::Stable || target.tracking_num_ == TrackingState::Occlusion) {
        target.is_stable_ = true;
    }
}

void ImmUkfPdaMulti::updateBehaviorState(const UKF &target, autoware_msgs::DetectedObject &object) {
    if (target.mode_prob_cv_ > target.mode_prob_ctrv_ && target.mode_prob_cv_ > target.mode_prob_rm_) {
        object.behavior_state = MotionModel::CV;
    } else if (target.mode_prob_ctrv_ > target.mode_prob_cv_ && target.mode_prob_ctrv_ > target.mode_prob_rm_) {
        object.behavior_state = MotionModel::CTRV;
    } else {
        object.behavior_state = MotionModel::RM;
    }
}

void ImmUkfPdaMulti::initTracker(const autoware_msgs::DetectedObjectArray &input, double timestamp) {
    for (size_t i = 0; i < input.objects.size(); i++) {
        double px = input.objects[i].pose.position.x;
        double py = input.objects[i].pose.position.y;
        Eigen::VectorXd init_meas = Eigen::VectorXd(2);
        init_meas << px, py;

        UKF ukf;
        ukf.initialize(init_meas, timestamp, target_id_);
        targets_.push_back(ukf);
        target_id_++;
    }
    timestamp_ = timestamp;
    init_ = true;
}

void ImmUkfPdaMulti::secondInit(UKF &target, const std::vector<autoware_msgs::DetectedObject> &object_vec, double dt) {
    if (object_vec.size() == 0) {
        target.tracking_num_ = TrackingState::Die;
        ROS_DEBUG_STREAM("[Die] secondInit: " << target.ukf_id_);
        return;
    }

    // record first_orientation_computation measurement for env classification
    target.init_meas_ << target.x_merge_(0), target.x_merge_(1);

    // state update
    double target_x = object_vec[0].pose.position.x;
    double target_y = object_vec[0].pose.position.y;
    double target_diff_x = target_x - target.x_merge_(0);
    double target_diff_y = target_y - target.x_merge_(1);
    double target_yaw = atan2(target_diff_y, target_diff_x);
    double dist = sqrt(target_diff_x * target_diff_x + target_diff_y * target_diff_y);
    double target_v = dist / dt;

    while (target_yaw > M_PI)
        target_yaw -= 2. * M_PI;
    while (target_yaw < -M_PI)
        target_yaw += 2. * M_PI;

    target.x_merge_(0) = target.x_cv_(0) = target.x_ctrv_(0) = target.x_rm_(0) = target_x;
    target.x_merge_(1) = target.x_cv_(1) = target.x_ctrv_(1) = target.x_rm_(1) = target_y;
    target.x_merge_(2) = target.x_cv_(2) = target.x_ctrv_(2) = target.x_rm_(2) = target_v;
    target.x_merge_(3) = target.x_cv_(3) = target.x_ctrv_(3) = target.x_rm_(3) = target_yaw;

    target.tracking_num_++;
    return;
}

void ImmUkfPdaMulti::updateTrackingNum(const std::vector<autoware_msgs::DetectedObject> &object_vec, UKF &target) {
    if (object_vec.size() > 0) {
        if (target.tracking_num_ < TrackingState::Stable) {
            target.tracking_num_++;
        } else if (target.tracking_num_ == TrackingState::Stable) {
            target.tracking_num_ = TrackingState::Stable;
        } else if (target.tracking_num_ >= TrackingState::Stable && target.tracking_num_ < TrackingState::Lost) {
            target.tracking_num_ = TrackingState::Stable;
        } else if (target.tracking_num_ == TrackingState::Lost) {
            target.tracking_num_ = TrackingState::Die;
            ROS_DEBUG_STREAM("[Die] target.tracking_num_ == TrackingState::Lost: " << target.ukf_id_);
        }
    } else {
        if (target.tracking_num_ < TrackingState::Stable) {
            target.tracking_num_ = TrackingState::Die;
            ROS_DEBUG_STREAM(
                    "[Die] target.tracking_num_ < TrackingState::Stable && : object_vec.size() <= 0" << target.ukf_id_);
        } else if (target.tracking_num_ >= TrackingState::Stable && target.tracking_num_ < TrackingState::Lost) {
            target.tracking_num_++;
        } else if (target.tracking_num_ == TrackingState::Lost) {
            target.tracking_num_ = TrackingState::Die;
            ROS_DEBUG_STREAM("[Die] target.tracking_num_ == TrackingState::Lost: " << target.ukf_id_);
        }
    }

    return;
}

//// done: TODO probabilisticDataAssociationの引数にodomのreliabilityを追加（必要ない）
bool
ImmUkfPdaMulti::probabilisticDataAssociation(const autoware_msgs::DetectedObjectArray &input, const double timestamp,
                                             std::vector<bool> &matching_vec,
                                             std::vector<autoware_msgs::DetectedObject> &object_vec, UKF &target) {
    double det_s = 0;
    Eigen::VectorXd max_det_z;
    Eigen::MatrixXd max_det_s;
    bool success = true;
    double dt = timestamp - target.time_;

    if (use_sukf_) {
        max_det_z = target.z_pred_ctrv_;
        max_det_s = target.s_ctrv_;
        det_s = max_det_s.determinant();
    } else {
        // find maxDetS associated with predZ
        target.findMaxZandS(max_det_z, max_det_s);
        det_s = max_det_s.determinant();
    }

    // prevent ukf not to explode
    if (std::isnan(det_s) || det_s > prevent_explosion_thres_) {
        target.tracking_num_ = TrackingState::Die;
        ROS_DEBUG_STREAM("[Die] std::isnan(det_s) || det_s > prevent_explosion_thres_: " << target.ukf_id_);
        success = false;
        return success;
    }

    bool is_second_init;
    if (target.tracking_num_ == TrackingState::Init) {
        is_second_init = true;
    } else {
        is_second_init = false;
    }

    // measurement gating
    //// done: TODO Matching時にodomのreliabilityが考慮されるようにする（必要ない）
    measurementValidation(input, target, is_second_init, max_det_z, max_det_s, object_vec, matching_vec);

    // second detection for a target: update v and yaw
    if (is_second_init) {
        secondInit(target, object_vec, dt);
        success = false;
        return success;
    }

    updateTargetWithAssociatedObject(object_vec, target);

    if (target.tracking_num_ == TrackingState::Die) {
        success = false;
        return success;
    }
    return success;
}

//// done: TODO: makeNewTargetsのr_cv_にreliability_vec[i]を追加
void ImmUkfPdaMulti::makeNewTargets(const double timestamp, const autoware_msgs::DetectedObjectArray &input,
                                    const std::vector<bool> &matching_vec,
                                    const std::vector<Eigen::MatrixXd> &reliability_vec) {
    for (size_t i = 0; i < input.objects.size(); i++) {
        if (matching_vec[i] == false) {
            //// done: TODO NIS検定が古い奴に対して同一と判定した場合，新しく生成しない
            bool skip = false;
            for (size_t j = 0; j < targets_.size(); j++) {
                double x = targets_[j].x_merge_(0);
                double y = targets_[j].x_merge_(1);

                Eigen::VectorXd target = Eigen::VectorXd(2);
                target << x, y;

                Eigen::VectorXd meas(2);
                meas << input.objects[i].pose.position.x, input.objects[i].pose.position.y;


                Eigen::VectorXd diff = target - meas;
                double nis = diff.transpose() * reliability_vec[i].inverse() * diff;
                ROS_DEBUG_STREAM("nis" << nis);

                //// nis<gating_thres_ということは，targetの中にmeasに対応する点がある
                if (nis < gating_thres_) {
                    skip = true;
                    break;
                }
            }
            if (skip)
                continue;

            double px = input.objects[i].pose.position.x; // ここのinputはすでにodomフレームに変換済み
            double py = input.objects[i].pose.position.y;
            Eigen::VectorXd init_meas = Eigen::VectorXd(2);
            init_meas << px, py;

            UKF ukf;
            //// done: TODO ukfのp_merge_の初期値をreliability_vec[i]に変更
            ukf.initialize(init_meas, timestamp, target_id_);
            ukf.r_cv_ = reliability_vec[i];
            ukf.r_cv_ = reliability_vec[i];
            ukf.r_cv_ = reliability_vec[i];
            ukf.p_merge_.block<2, 2>(1, 1) += reliability_vec[i];

            ukf.object_ = input.objects[i];
            targets_.push_back(ukf);
            target_id_++;
        }
    }
}

void ImmUkfPdaMulti::staticClassification() {
    for (size_t i = 0; i < targets_.size(); i++) {
        // targets_[i].x_merge_(2) is referred for estimated velocity
        double current_velocity = std::abs(targets_[i].x_merge_(2));
        targets_[i].vel_history_.push_back(current_velocity);
        if (targets_[i].tracking_num_ == TrackingState::Stable && targets_[i].lifetime_ > life_time_thres_) {
            int index = 0;
            double sum_vel = 0;
            double avg_vel = 0;
            for (auto rit = targets_[i].vel_history_.rbegin(); index < static_num_history_thres_; ++rit) {
                index++;
                sum_vel += *rit;
            }
            avg_vel = double(sum_vel / static_num_history_thres_);

            if (avg_vel < static_velocity_thres_ && current_velocity < static_velocity_thres_) {
                targets_[i].is_static_ = true;
            }
        }
    }
}

bool
ImmUkfPdaMulti::arePointsClose(const geometry_msgs::Point &in_point_a,
                               const geometry_msgs::Point &in_point_b,
                               float in_radius) {
    return (fabs(in_point_a.x - in_point_b.x) <= in_radius) && (fabs(in_point_a.y - in_point_b.y) <= in_radius);
}

bool
ImmUkfPdaMulti::arePointsEqual(const geometry_msgs::Point &in_point_a,
                               const geometry_msgs::Point &in_point_b) {
    return arePointsClose(in_point_a, in_point_b, CENTROID_DISTANCE);
}

bool
ImmUkfPdaMulti::isPointInPool(const std::vector<geometry_msgs::Point> &in_pool,
                              const geometry_msgs::Point &in_point) {
    for (size_t j = 0; j < in_pool.size(); j++) {
        if (arePointsEqual(in_pool[j], in_point)) {
            return true;
        }
    }
    return false;
}

autoware_msgs::DetectedObjectArray
ImmUkfPdaMulti::removeRedundantObjects(const autoware_msgs::DetectedObjectArray &in_detected_objects,
                                       const std::vector<size_t> in_tracker_indices) {
    if (in_detected_objects.objects.size() != in_tracker_indices.size())
        return in_detected_objects;

    autoware_msgs::DetectedObjectArray resulting_objects;
    resulting_objects.header = in_detected_objects.header;

    std::vector<geometry_msgs::Point> centroids;
    //create unique points
    for (size_t i = 0; i < in_detected_objects.objects.size(); i++) {
        if (!isPointInPool(centroids, in_detected_objects.objects[i].pose.position)) {
            centroids.push_back(in_detected_objects.objects[i].pose.position);
        }
    }
    //assign objects to the points
    std::vector<std::vector<size_t>> matching_objects(centroids.size());
    for (size_t k = 0; k < in_detected_objects.objects.size(); k++) {
        const auto &object = in_detected_objects.objects[k];
        for (size_t i = 0; i < centroids.size(); i++) {
            if (arePointsClose(object.pose.position, centroids[i], merge_distance_threshold_)) {
                matching_objects[i].push_back(k);//store index of matched object to this point
            }
        }
    }
    //get oldest object on each point
    for (size_t i = 0; i < matching_objects.size(); i++) {
        size_t oldest_object_index = 0;
        int oldest_lifespan = -1;
        std::string best_label;
        for (size_t j = 0; j < matching_objects[i].size(); j++) {
            size_t current_index = matching_objects[i][j];
            int current_lifespan = targets_[in_tracker_indices[current_index]].lifetime_;
            if (current_lifespan > oldest_lifespan) {
                oldest_lifespan = current_lifespan;
                oldest_object_index = current_index;
            }
            if (!targets_[in_tracker_indices[current_index]].label_.empty() &&
                targets_[in_tracker_indices[current_index]].label_ != "unknown") {
                best_label = targets_[in_tracker_indices[current_index]].label_;
            }
        }
        // delete nearby targets except for the oldest target
        for (size_t j = 0; j < matching_objects[i].size(); j++) {
            size_t current_index = matching_objects[i][j];
            if (current_index != oldest_object_index) {
                targets_[in_tracker_indices[current_index]].tracking_num_ = TrackingState::Die;
                ROS_DEBUG_STREAM("[Die] delete nearby targets except for the oldest target: "
                                         << targets_[in_tracker_indices[current_index]].ukf_id_);
            }
        }
        autoware_msgs::DetectedObject best_object;
        best_object = in_detected_objects.objects[oldest_object_index];
        if (best_label != "unknown"
            && !best_label.empty()) {
            best_object.label = best_label;
        }

        resulting_objects.objects.push_back(best_object);
    }

    return resulting_objects;

}

void ImmUkfPdaMulti::makeOutput(const autoware_msgs::DetectedObjectArray &input,
                                const std::vector<bool> &matching_vec,
                                autoware_msgs::DetectedObjectArray &detected_objects_output) {
    autoware_msgs::DetectedObjectArray tmp_objects;
    tmp_objects.header = input.header;
    std::vector<size_t> used_targets_indices;
    for (size_t i = 0; i < targets_.size(); i++) {

        double tx = targets_[i].x_merge_(0);
        double ty = targets_[i].x_merge_(1);

        double tv = targets_[i].x_merge_(2);
        double tyaw = targets_[i].x_merge_(3);
        double tyaw_rate = targets_[i].x_merge_(4);

        while (tyaw > M_PI)
            tyaw -= 2. * M_PI;
        while (tyaw < -M_PI)
            tyaw += 2. * M_PI;

        tf::Quaternion q = tf::createQuaternionFromYaw(tyaw);

        autoware_msgs::DetectedObject dd;
        dd = targets_[i].object_;
        dd.id = targets_[i].ukf_id_;
        dd.velocity.linear.x = tv;
        dd.acceleration.linear.y = tyaw_rate;
        dd.velocity_reliable = targets_[i].is_stable_;
        dd.pose_reliable = targets_[i].is_stable_;


//        if (!targets_[i].is_static_ && targets_[i].is_stable_) {
        // Aligh the longest side of dimentions with the estimated orientation
        if (targets_[i].object_.dimensions.x < targets_[i].object_.dimensions.y) {
            dd.dimensions.x = targets_[i].object_.dimensions.y;
            dd.dimensions.y = targets_[i].object_.dimensions.x;
        }

        dd.pose.position.x = tx;
        dd.pose.position.y = ty;

        if (!std::isnan(q[0]))
            dd.pose.orientation.x = q[0];
        if (!std::isnan(q[1]))
            dd.pose.orientation.y = q[1];
        if (!std::isnan(q[2]))
            dd.pose.orientation.z = q[2];
        if (!std::isnan(q[3]))
            dd.pose.orientation.w = q[3];
//        }
        updateBehaviorState(targets_[i], dd);

        if (targets_[i].is_stable_ || (targets_[i].tracking_num_ >= TrackingState::Init &&
                                       targets_[i].tracking_num_ < TrackingState::Stable)) {
            tmp_objects.objects.push_back(dd);
            used_targets_indices.push_back(i);
        }
    }
    detected_objects_output = removeRedundantObjects(tmp_objects, used_targets_indices);
}

void ImmUkfPdaMulti::removeUnnecessaryTarget() {
    std::vector<UKF> temp_targets;
    for (size_t i = 0; i < targets_.size(); i++) {
        if (targets_[i].tracking_num_ != TrackingState::Die) {
            temp_targets.push_back(targets_[i]);
        }
    }
    std::vector<UKF>().swap(targets_);
    targets_ = temp_targets;
}

void ImmUkfPdaMulti::dumpResultText(autoware_msgs::DetectedObjectArray &detected_objects) {
    std::ofstream outputfile(result_file_path_, std::ofstream::out | std::ofstream::app);
    for (size_t i = 0; i < detected_objects.objects.size(); i++) {
        double yaw = tf::getYaw(detected_objects.objects[i].pose.orientation);

        // KITTI tracking benchmark data format:
        // (frame_number,tracked_id, object type, truncation, occlusion, observation angle, x1,y1,x2,y2, h, w, l, cx, cy,
        // cz, yaw)
        // x1, y1, x2, y2 are for 2D bounding box.
        // h, w, l, are for height, width, length respectively
        // cx, cy, cz are for object centroid

        // Tracking benchmark is based on frame_number, tracked_id,
        // bounding box dimentions and object pose(centroid and orientation) from bird-eye view
        outputfile << std::to_string(frame_count_) << " " << std::to_string(detected_objects.objects[i].id) << " "
                   << "Unknown"
                   << " "
                   << "-1"
                   << " "
                   << "-1"
                   << " "
                   << "-1"
                   << " "
                   << "-1 -1 -1 -1"
                   << " " << std::to_string(detected_objects.objects[i].dimensions.x) << " "
                   << std::to_string(detected_objects.objects[i].dimensions.y) << " "
                   << "-1"
                   << " " << std::to_string(detected_objects.objects[i].pose.position.x) << " "
                   << std::to_string(detected_objects.objects[i].pose.position.y) << " "
                   << "-1"
                   << " " << std::to_string(yaw) << "\n";
    }
    frame_count_++;
}

void ImmUkfPdaMulti::makeReliabilityVec(const autoware_msgs::DetectedObjectArray &input,
                                        const nav_msgs::Odometry &vehicle_odoom,
                                        const Eigen::MatrixXd &p_vehicle,
                                        std::vector<Eigen::MatrixXd> &reliability_vec) {

    //// done: TODO makeReliabilityVec関数を作成
    tf::Quaternion quat(vehicle_odoom.pose.pose.orientation.x, vehicle_odoom.pose.pose.orientation.y,
                        vehicle_odoom.pose.pose.orientation.z, vehicle_odoom.pose.pose.orientation.w);
    double rpy[3];
    tf::Matrix3x3(quat).getRPY(rpy[0], rpy[1], rpy[2]);


    for (size_t i = 0; i < input.objects.size(); i++) {
        const auto &p = input.objects[i].pose;
        Eigen::MatrixXd v_sigma, p_sigma, transform_v2p(2, 3), transform_pl2pw(2, 2);
//        v_sigma = p_vehicle * 1.0e+2 * 5;
        v_sigma = p_vehicle * 1.0e+2 * 6;
        p_sigma = Eigen::MatrixXd::Identity(2, 2) * 0.15 * 0.15;
        transform_v2p << 1.0, 0.0, -p.position.x * sin(rpy[2]) - p.position.y * cos(rpy[2]),
                0.0, 1.0, p.position.x * cos(rpy[2]) - p.position.y * sin(rpy[2]);
        transform_pl2pw << cos(rpy[2]), -sin(rpy[2]), sin(rpy[2]), cos(rpy[2]);
        reliability_vec[i] = transform_v2p * v_sigma * transform_v2p.transpose() +
                             transform_pl2pw * p_sigma * transform_pl2pw.transpose();
/*
        reliability_vec[i] = transform_pl2pw * p_sigma * transform_pl2pw.transpose();
*/
    }
    std::ostringstream os;
    os << "v_sigma: \n" << p_vehicle << std::endl;
    os << "reliability_vec_: " << std::endl;
    for (const auto &mat:reliability_vec) {
        os << mat << std::endl;
    }
    ROS_DEBUG_STREAM(os.str());

}

void ImmUkfPdaMulti::tracker(const autoware_msgs::DetectedObjectArray &input,
                             autoware_msgs::DetectedObjectArray &detected_objects_output) {

    double timestamp = input.header.stamp.toSec();
//    ros::Time t = ros::Time::now();
//    double timestamp = t.toSec();
    std::vector<bool> matching_vec(input.objects.size(), false);

    if (!init_) {
        initTracker(input, timestamp);
        makeOutput(input, matching_vec, detected_objects_output);
        return;
    }

//    double dt = (timestamp - timestamp_);
//    timestamp_ = timestamp;

    for (size_t i = 0; i < input.objects.size(); i++) {
        //// done: TODO timestampとlabel=targets，id,targetsのx,y,Pをofs出力
        ofs_ << timestamp << "," << input.objects[i].header.frame_id << "," << i << ","
             << input.objects[i].pose.position.x << ","
             << input.objects[i].pose.position.y << "," << reliability_vec_[i](0, 0) << "," << reliability_vec_[i](0, 1)
             << "," << reliability_vec_[i](1, 0) << "," << reliability_vec_[i](1, 1) << std::endl;
    }

    // start UKF process
    for (size_t i = 0; i < targets_.size(); i++) {

        //// done: TODO timestampとlabel=targets，id,targetsのx,y,Pをofs出力
        ofs_ << timestamp << ",target," << targets_[i].ukf_id_ << "," << targets_[i].z_pred_ctrv_(0) << ","
             << targets_[i].z_pred_ctrv_(1)
             << "," << targets_[i].s_ctrv_(0, 0) << "," << targets_[i].s_ctrv_(0, 1) << "," << targets_[i].s_ctrv_(1, 0)
             << "," << targets_[i].s_ctrv_(1, 1) << std::endl;

        //// ignore: TODO input_mksとinput_mkzの両方に対してmatching_vec_mksとmatching_vec_mkzを作成

        std::vector<autoware_msgs::DetectedObject> object_vec_tmp;
        double det_s = 0;
        Eigen::VectorXd max_det_z;
        Eigen::MatrixXd max_det_s;
        if (use_sukf_) {
            max_det_z = targets_[i].z_pred_ctrv_;
            max_det_s = targets_[i].s_ctrv_;
            det_s = max_det_s.determinant();
        } else {
            // find maxDetS associated with predZ
            targets_[i].findMaxZandS(max_det_z, max_det_s);
            det_s = max_det_s.determinant();
        }
        // measurement gatingukf_id_
        measurementValidation(input, targets_[i], false, max_det_z, max_det_s, object_vec_tmp, matching_vec);
        if (object_vec_tmp.empty() && targets_[i].num_pass <= 10) {
            targets_[i].num_pass++;
//            targets_[i].prediction(use_sukf_, has_subscribed_vectormap_, dt);
            continue;
        }
        targets_[i].num_pass = 0;

        //// done: TODO inputの中に両方のobjを入れれば，matching_vec生成時に勝手に適切なものを選択してくれるはず
        //// done: TODO 最大0.05秒の時間差のあるLidar detection情報を利用することになるが，歩行者の歩行速度を最大1m/sとしても5cmしかずれないため，大きな影響はないはず
        //// ignore: TODO input_mks，matching_vec_mks，reliability_vec_mksとinput_mkz，matching_vec_mkz,reliability_vec_mkzを作成
        //// ignore: TODO matching_vecがtrueの全ての要素のreliability_vecのdetが最小のものをinput_mergeにpush_back，matching_vec_merge,reliability_vecも作成


        targets_[i].is_stable_ = false;
        targets_[i].is_static_ = false;

        if (targets_[i].tracking_num_ == TrackingState::Die) {
            continue;
        }
        // prevent ukf not to explode
        if (targets_[i].p_merge_.determinant() > prevent_explosion_thres_ ||
            targets_[i].p_merge_(4, 4) > prevent_explosion_thres_) {
            targets_[i].tracking_num_ = TrackingState::Die;
            ROS_DEBUG_STREAM("[Die] prevent ukf not to explode: " << targets_[i].ukf_id_);
            continue;
        }

        for (size_t j = 0; j < input.objects.size(); j++) {
            if (matching_vec[j]) {
                //// done: TODO targets_[i]のr_cv_などを変更するreliability_vec[j]の値を基に変更する（Matchingは基本一つであるとしておこう）
                targets_[i].r_cv_ = targets_[i].r_ctrv_ = targets_[i].r_rm_ = reliability_vec_[j];
            }
        }

        //// done: TODO targets_[i]のr_cv_などが利用される（この前でr_cvを変更しておく）
        targets_[i].prediction(use_sukf_, has_subscribed_vectormap_, timestamp);

        std::vector<autoware_msgs::DetectedObject> object_vec;
        //// done TODO probabilisticDataAssociationの引数にodomのreliabilityを追加（たぶんいらない）
        bool success = probabilisticDataAssociation(input, timestamp, matching_vec, object_vec, targets_[i]);
        if (!success) {
            targets_[i].time_ = timestamp;
            continue;
        }

        targets_[i].update(use_sukf_, detection_probability_, gate_probability_, gating_thres_, object_vec);

        targets_[i].time_ = timestamp;

    }
    // end UKF process

    //// done: TODO makeNewTargetsにodomのreliabilityを考慮したinputのliabilityを追加
    // making new ukf target for no data association objects
    makeNewTargets(timestamp, input, matching_vec, reliability_vec_);

    // static dynamic classification
    staticClassification();

    // making output for visualization
    makeOutput(input, matching_vec, detected_objects_output);

    // remove unnecessary ukf object
    removeUnnecessaryTarget();
}
