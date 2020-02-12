//
// Created by ohashi on 19/09/13.
//

#include <phone_localizer_ukf.h>

PhoneLocalizerUKF::PhoneLocalizerUKF(ros::NodeHandle *nh, ros::NodeHandle *private_nh) :
        nh(*nh), private_nh(*private_nh),
        update_imu(false), update_magnetic(false), update_gps(false), update_gnssraw_fix(false),
        update_gnssraw_acceleration(false),
        update_vehicle_perception(false), update_vehicle_perception_xy(false), update_vehicle_perception_vr(false) {

    ROS_DEBUG("%s", __FUNCTION__);

    bool start;
    private_nh->param("start", start, true);
    ros::Rate loop(10);
    while (ros::ok() && !start) {
        private_nh->param("start", start, false);
        ROS_WARN("Waiting till %s/start=true", nh->getNamespace().substr(1).c_str());
        loop.sleep();
    }


    std::string path =
            "/home/ohashi/Dropbox/MATLAB/phones_vehicle_fusion" + this->nh.getNamespace().substr(1);
    ofs_state.open(path + "/state.csv");
    ofs_state << "%time,x,y,v,yaw,yawd" << std::endl;
    ofs_cov_pred.open(path + "/cov_pred.csv");
    ofs_cov_pred
            << "%time,e00,e01,e02,e03,e04,e10,e11,e12,e13,e14,e20,e21,e22,e23,e24,e30,e31,e32,e33,e34,e40,e41,e42,e43,e44"
            << std::endl;
    ofs_cov_update.open(path + "/cov_update.csv");
    ofs_cov_pred
            << "%time,e00,e01,e02,e03,e04,e10,e11,e12,e13,e14,e20,e21,e22,e23,e24,e30,e31,e32,e33,e34,e40,e41,e42,e43,e44"
            << std::endl;
    ofs_imu.open(path + "/imu.csv");
    ofs_imu << "%time,yawd" << std::endl;
    ofs_gps.open(path + "/gps.csv");
    ofs_gps << "%time,x,y" << std::endl;
    ofs_gnssraw_accel.open(path + "/gnssraw_accel.csv");
    ofs_gnssraw_accel << "%time,vx,vy" << std::endl;
    ofs_weight.open(path + "/weight.csv");
    ofs_weight << "%time,w_s,w_c" << std::endl;


    /** Set parameter **/

    //// phone name
    if (!this->private_nh.getParam("phone_name", phone_name)) {
        ROS_ERROR("Non \"phone_name\"");
        exit(-1);
    }

    //// origin
    if (!this->private_nh.getParam("/origin_fix", origin_fix)) {
        ROS_ERROR("Non \"/origin_fix\"");
        exit(-1);
    }
    geo.set_plane(origin_fix[0], origin_fix[1]);
    ROS_INFO("Origin fix: [%.3f,%.3f]", origin_fix[0], origin_fix[1]);

    //// origin frame_id
    if (!this->nh.getParam("/origin_frame_id", origin_frame_id)) {
        ROS_ERROR("Not set \"/origin_frame_id\"");
        exit(-1);
    }
    ROS_INFO("Origin frame_id: %s", origin_frame_id.c_str());

    //// topic names
    topic_name_sub_imu = "/android/" + phone_name + "/" + TOPIC_NAME_SUB_IMU;
    topic_name_sub_magnetic = "/android/" + phone_name + "/" + TOPIC_NAME_SUB_MAGNETIC;
    topic_name_sub_gps = "/android/" + phone_name + "/" + TOPIC_NAME_SUB_GPS;
    topic_name_sub_gnssraw_fix = "/android/" + phone_name + "/" + TOPIC_NAME_SUB_GNSSRAW_FIX;
    topic_name_sub_gnssraw_acceleration = "/android/" + phone_name + "/" + TOPIC_NAME_SUB_GNSSRAW_ACCELERATION;
    topic_name_sub_vehicle_perception = "/android/" + phone_name + "/" + TOPIC_NAME_SUB_OBJECTS;
    topic_name_pub_odom = TOPIC_NAME_PUB_ODOM;
    topic_name_pub_particles = TOPIC_NAME_PUB_POSEARRAY;

    //// TF Listener
    static tf2_ros::TransformListener tf_listener(tf_buffer);

    //// UKF parameters
    this->private_nh.param("detection_probability", detection_probability);
    this->private_nh.param("gate_probability", gate_probability);
    this->private_nh.param("gating_thres", gating_thres);

    //// sensors
    this->private_nh.param("use_imu", use_imu, USE_IMU);
    this->private_nh.param("use_magnetic", use_mag, USE_MAGNETIC);
    this->private_nh.param("use_gps", use_gps, USE_GPS);
    if (!this->private_nh.getParam("use_gps_xy", use_gps_xy)) {
        use_gps_xy = use_gps;
    }
    if (!this->private_nh.getParam("use_gps_v", use_gps_v)) {
        use_gps_v = use_gps;
    }
    if (!this->private_nh.getParam("use_gps_r", use_gps_r)) {
        use_gps_r = use_gps;
    }
    this->private_nh.param("use_gnssraw_fix", use_gnssraw_fix, USE_GNSSRAW_FIX);
    this->private_nh.param("use_gnssraw_acceleration", use_gnssraw_acceleration, USE_GNSSRAW_ACCELERATION);
    if (!this->private_nh.getParam("use_gnssraw_acceleration_v", use_gnssraw_acceleration_v)) {
        use_gnssraw_acceleration_v = use_gnssraw_acceleration;
    }
    if (!this->private_nh.getParam("use_gnssraw_acceleration_r", use_gnssraw_acceleration_r)) {
        use_gnssraw_acceleration_r = use_gnssraw_acceleration;
    }
    this->private_nh.param("use_vehicle_perception", use_vehicle_perception, USE_VEHICLE_PERCEPTION);
    if (!this->private_nh.getParam("use_vehicle_perception_xy", use_vehicle_perception_xy)) {
        use_vehicle_perception_xy = use_vehicle_perception;
    }
    if (!this->private_nh.getParam("use_vehicle_perception_vr", use_vehicle_perception_vr)) {
        use_vehicle_perception_vr = use_vehicle_perception;
    }


    /** Set publisher **/

    //// Odom
    odom.header.frame_id = origin_frame_id;
    odom.child_frame_id = phone_frame_id = phone_name;
    pub_odom = this->nh.advertise<nav_msgs::Odometry>(topic_name_pub_odom, 10);

    /** Set subscriber **/
    if (use_imu) {
        sub_imu = this->nh.subscribe(topic_name_sub_imu, 10, &PhoneLocalizerUKF::callback_imu, this);
        ROS_DEBUG("Subscribe IMU Message");
    }
    if (use_mag) {
        sub_magnetic = this->nh.subscribe(topic_name_sub_magnetic, 10, &PhoneLocalizerUKF::callback_magnetic, this);
        ROS_DEBUG("Subscribe Magnetic Message");
    }
    if (use_gps && (use_gps_xy || use_gps_v || use_gps_r)) {
        sub_gps = this->nh.subscribe(topic_name_sub_gps, 10, &PhoneLocalizerUKF::callback_gps, this);
        ROS_DEBUG("Subscribe GPS Message");
    }
    if (use_gnssraw_fix) {
        sub_gnssraw_fix = this->nh.subscribe(topic_name_sub_gnssraw_fix, 10, &PhoneLocalizerUKF::callback_gnssraw_fix,
                                             this);
        ROS_DEBUG("Subscribe GNSSRaw Fix Message");
    }
    if (use_gnssraw_acceleration && (use_gnssraw_acceleration_v || use_gnssraw_acceleration_r)) {
        sub_gnssraw_acceleration = this->nh.subscribe(topic_name_sub_gnssraw_acceleration, 10,
                                                      &PhoneLocalizerUKF::callback_gnssraw_acceleration, this);
        ROS_DEBUG("Subscribe GNSSRaw Velocity Message");
    }
    if (use_vehicle_perception && (use_vehicle_perception_xy || use_vehicle_perception_vr)) {
        sub_vehicle_perception = this->nh.subscribe(topic_name_sub_vehicle_perception, 10,
                                                    &PhoneLocalizerUKF::callback_vehicle_perception, this);
        ROS_DEBUG("Subscribe Vehicle Perception Message");
    }

}

PhoneLocalizerUKF::~PhoneLocalizerUKF() {};

void PhoneLocalizerUKF::callback_imu(const sensor_msgs::Imu::ConstPtr &msg) {

    imu = *msg;
/*
    time = msg->header.stamp;
*/
    time = ros::Time::now();
//    ROS_DEBUG_STREAM("callback_imu: \n" << imu);
    update_imu = true;
    run();

}

void PhoneLocalizerUKF::callback_magnetic(const sensor_msgs::MagneticField::ConstPtr &msg) {

    magnetic = *msg;
/*
    time = msg->header.stamp;
*/
    time = ros::Time::now();
//    ROS_DEBUG_STREAM("callback_magnetic: \n" << magnetic);
    update_magnetic = true;
    run();

}

void PhoneLocalizerUKF::callback_gps(const sensor_msgs::NavSatFix::ConstPtr &msg) {

    gps = *msg;
/*
    time = msg->header.stamp;
*/
//    time = ros::Time::now();
//    ROS_DEBUG_STREAM("callback_gps: \n" << gps);
    update_gps = true;

    geo.llh_to_xyz_us(gps.latitude,gps.longitude,0.0);
    double gps_x = geo.y(), gps_y = -geo.x();
    ofs_gps << gps.header.stamp.sec << "." << gps.header.stamp.nsec << "," << gps_x << "," << gps_y << std::endl;


//    run();

}

void PhoneLocalizerUKF::callback_gnssraw_fix(const sensor_msgs::NavSatFix::ConstPtr &msg) {

    gnssraw_fix = *msg;
/*
    time = msg->header.stamp;
*/
    time = ros::Time::now();
//    ROS_DEBUG_STREAM("callback_gnssraw_fix: \n" << gnssraw_fix);
    update_gnssraw_fix = true;
    run();

}

void PhoneLocalizerUKF::callback_gnssraw_acceleration(const geometry_msgs::AccelWithCovarianceStamped::ConstPtr &msg) {

    geometry_msgs::AccelWithCovarianceStamped tmp = *msg;
    if (gnssraw_acceleration.accel.accel.linear.x == tmp.accel.accel.linear.x &&
        gnssraw_acceleration.accel.accel.linear.y == tmp.accel.accel.linear.y &&
        gnssraw_acceleration.accel.accel.linear.z == tmp.accel.accel.linear.z &&
        gnssraw_acceleration.accel.accel.angular.x == tmp.accel.accel.angular.x &&
        gnssraw_acceleration.accel.accel.angular.y == tmp.accel.accel.angular.y &&
        gnssraw_acceleration.accel.accel.angular.z == tmp.accel.accel.angular.z) {
        ROS_DEBUG_STREAM("callback_gnssraw_acceleration received same message");
        return;
    }
    gnssraw_acceleration = *msg;
/*
    time = msg->header.stamp;
*/
    time = ros::Time::now();
//    ROS_DEBUG_STREAM("callback_gnssraw_acceleration: \n" << gnssraw_acceleration);
    update_gnssraw_acceleration = true;

    double gnssraw_vx = gnssraw_acceleration.accel.accel.linear.y, gnssraw_vy = -gnssraw_acceleration.accel.accel.linear.x;
    ofs_gnssraw_accel << gnssraw_acceleration.header.stamp.sec << "." << gnssraw_acceleration.header.stamp.nsec << ","
                      << gnssraw_vx << "," << gnssraw_vy << std::endl;

    run();

}

void PhoneLocalizerUKF::callback_vehicle_perception(const geometry_msgs::PoseWithCovarianceStamped::ConstPtr &msg) {

    //// TODO: PoseWithCovarinceに変更
    geometry_msgs::PoseWithCovarianceStamped tmp = *msg;
    double dx = tmp.pose.pose.position.x - now_phone_state(0);
    double dy = tmp.pose.pose.position.y - now_phone_state(1);
    double d = std::sqrt(dx * dx + dy * dy);
    if (d > 8) {
        return;
    }

    vehicle_perception = *msg;
/*
    time = msg->header.stamp;
*/
    time = ros::Time::now();
//    ROS_DEBUG_STREAM("callback_vehicle_perception: \n" << vehicle_perception);
    update_vehicle_perception = true;
    run();

}

void PhoneLocalizerUKF::publish_odom(const ros::Time &time) {

    odom.pose.pose.position.x = now_phone_state(0, 0);
    odom.pose.pose.position.y = now_phone_state(1, 0);
    odom.pose.pose.position.z = 0.0;
    odom.twist.twist.linear.x = now_phone_state(2, 0);
    tf::Quaternion quat = tf::createQuaternionFromRPY(0.0, 0.0, now_phone_state(3, 0));
    quaternionTFToMsg(quat, odom.pose.pose.orientation);

    odom.pose.covariance[0] = ukf.p_ctrv_(0, 0);
    odom.pose.covariance[7] = ukf.p_ctrv_(1, 1);
    odom.pose.covariance[35] = ukf.p_ctrv_(3, 3);
    odom.pose.covariance[1] = odom.pose.covariance[6] = ukf.p_ctrv_(0, 1);
    odom.pose.covariance[5] = odom.pose.covariance[30] = ukf.p_ctrv_(0, 3);
    odom.pose.covariance[11] = odom.pose.covariance[31] = ukf.p_ctrv_(1, 3);

    odom.header.stamp = time;

    ofs_state << time.sec << "." << time.nsec << "," << now_phone_state(0, 0) << "," << now_phone_state(1, 0) << ","
              << now_phone_state(2, 0) << "," << now_phone_state(3, 0) << "," << now_phone_state(4, 0) << std::endl;
    ofs_cov_update << time.sec << "." << time.nsec << ","
                   << ukf.p_ctrv_(0, 0) << "," << ukf.p_ctrv_(0, 1) << "," << ukf.p_ctrv_(0, 2) << ","
                   << ukf.p_ctrv_(0, 3) << "," << ukf.p_ctrv_(0, 4) << ","
                   << ukf.p_ctrv_(1, 0) << "," << ukf.p_ctrv_(1, 1) << "," << ukf.p_ctrv_(1, 2) << ","
                   << ukf.p_ctrv_(1, 3) << "," << ukf.p_ctrv_(1, 4) << ","
                   << ukf.p_ctrv_(2, 0) << "," << ukf.p_ctrv_(2, 1) << "," << ukf.p_ctrv_(2, 2) << ","
                   << ukf.p_ctrv_(2, 3) << "," << ukf.p_ctrv_(2, 4) << ","
                   << ukf.p_ctrv_(3, 0) << "," << ukf.p_ctrv_(3, 1) << "," << ukf.p_ctrv_(3, 2) << ","
                   << ukf.p_ctrv_(3, 3) << "," << ukf.p_ctrv_(3, 4) << ","
                   << ukf.p_ctrv_(4, 0) << "," << ukf.p_ctrv_(4, 1) << "," << ukf.p_ctrv_(4, 2) << ","
                   << ukf.p_ctrv_(4, 3) << "," << ukf.p_ctrv_(4, 4);

    pub_odom.publish(odom);

    ROS_DEBUG_STREAM("Published Phone State \n" << now_phone_state);

}

void PhoneLocalizerUKF::run() {

    static bool init = false;
    if (!init) {
        init = ukfInitialization();
    }

    //// done TODO: UKF prediction
    ukf.prediction(time.toSec());
    ofs_cov_pred << time.sec << "." << time.nsec << ","
                 << ukf.p_ctrv_(0, 0) << "," << ukf.p_ctrv_(0, 1) << "," << ukf.p_ctrv_(0, 2) << ","
                 << ukf.p_ctrv_(0, 3) << "," << ukf.p_ctrv_(0, 4) << ","
                 << ukf.p_ctrv_(1, 0) << "," << ukf.p_ctrv_(1, 1) << "," << ukf.p_ctrv_(1, 2) << ","
                 << ukf.p_ctrv_(1, 3) << "," << ukf.p_ctrv_(1, 4) << ","
                 << ukf.p_ctrv_(2, 0) << "," << ukf.p_ctrv_(2, 1) << "," << ukf.p_ctrv_(2, 2) << ","
                 << ukf.p_ctrv_(2, 3) << "," << ukf.p_ctrv_(2, 4) << ","
                 << ukf.p_ctrv_(3, 0) << "," << ukf.p_ctrv_(3, 1) << "," << ukf.p_ctrv_(3, 2) << ","
                 << ukf.p_ctrv_(3, 3) << "," << ukf.p_ctrv_(3, 4) << ","
                 << ukf.p_ctrv_(4, 0) << "," << ukf.p_ctrv_(4, 1) << "," << ukf.p_ctrv_(4, 2) << ","
                 << ukf.p_ctrv_(4, 3) << "," << ukf.p_ctrv_(4, 4);


    if (update_imu) {
        ukfUpdateIMU();
    }

    if (update_gnssraw_acceleration) {
        ukfUpdateGnssrawAcceleration();
    }

    if (update_vehicle_perception) {
        ukfUpdateVehiclePerception();
    }

    /** Publish Current Status **/

    //// Odom
    now_phone_state = ukf.x_ctrv_.col(0);
    publish_odom(time);

    update_imu = false;
    update_magnetic = false;
    update_gps = false;
    update_gnssraw_fix = false;
    update_gnssraw_acceleration = false;
    update_vehicle_perception = update_vehicle_perception_xy = update_vehicle_perception_vr = false;

    ROS_DEBUG("%s Completed", __FUNCTION__);

}

/**
 *
 * @return
 */
bool PhoneLocalizerUKF::ukfInitialization() {

    ROS_DEBUG("%s", __FUNCTION__);

    /** Preparing phone initial parameter **/
    std::vector<std::string> sensors_for_initial_state;
    if (!private_nh.getParam("sensors_for_initial_state", sensors_for_initial_state)) {
        ROS_ERROR("Non \"sensors_for_initial_state\"");
        exit(-1);
    }
    if (sensors_for_initial_state.size() != 3) {
        ROS_ERROR("rosparam \"sensors_for_initial_state\" should be vector of 3 elements");
        exit(-1);
    }
    ROS_DEBUG("sensors_for_initial_state: [%s,%s,%s]", sensors_for_initial_state[0].c_str(),
              sensors_for_initial_state[1].c_str(), sensors_for_initial_state[2].c_str());
/*
    if (sensors_for_initial_state[0] == "gnssraw_fix" && update_gnssraw_fix &&
        sensors_for_initial_state[1] == "gnssraw_acceleration" && update_gnssraw_acceleration &&
        sensors_for_initial_state[2] == "magnetic" && update_magnetic) {
        geo.llh_to_xyz_us(gps.latitude, gps.longitude, 0);
        double gnssraw_vx = gnssraw_acceleration.accel.accel.linear.y;
        double gnssraw_vy = -gnssraw_acceleration.accel.accel.linear.x;
        double rpy[3];
        imu2rpy(imu, rpy);
        magnetic2yaw(magnetic, rpy);
        phone_initial_state.x = geo.y();
        phone_initial_state.y = -geo.x();
        phone_initial_state.r = rpy[2];
        phone_initial_state.vx = std::sqrt(gnssraw_vx * gnssraw_vx + gnssraw_vy * gnssraw_vy);
        phone_initial_state.vy = 0.0;
    } else if (sensors_for_initial_state[0] == "gps" && update_gps &&
               sensors_for_initial_state[1] == "gps" && update_gps &&
               sensors_for_initial_state[2] == "magnetic" && update_magnetic) {
        geo.llh_to_xyz_us(gps.latitude, gps.longitude, 0);
        double x = geo.y(), y = -geo.x(), s = gps.header.stamp.toSec();
        static double x_first = s, y_first = y, s_first = s;
        double dx = x - x_first, dy = y - y_first, dt = s - s_first;
        if (std::sqrt(dx * dx + dy * dy) < 1.0) {
            ROS_WARN("Not initialized yed. Waiting for second gps data.");
            return false;
        }
        double rpy[3];
        imu2rpy(imu, rpy);
        magnetic2yaw(magnetic, rpy);
        phone_initial_state.x = x;
        phone_initial_state.y = y;
        phone_initial_state.r = rpy[2];
        phone_initial_state.vx = std::sqrt(dx * dx + dy * dy) / dt;
        phone_initial_state.vy = 0.0;
    } else if (sensors_for_initial_state[0] == "gps" && update_gps &&
               sensors_for_initial_state[1] == "gnssraw_acceleration" && update_gnssraw_acceleration &&
               sensors_for_initial_state[2] == "gnssraw_acceleration" && update_gnssraw_acceleration) {
        geo.llh_to_xyz_us(gps.latitude, gps.longitude, 0);
        double gnssraw_vx = gnssraw_acceleration.accel.accel.linear.y;
        double gnssraw_vy = -gnssraw_acceleration.accel.accel.linear.x;
        double gnssraw_yaw = std::atan2(gnssraw_vy, gnssraw_vx);
        phone_initial_state.x = geo.y();
        phone_initial_state.y = -geo.x();
        phone_initial_state.r = gnssraw_yaw;
        phone_initial_state.vx = std::sqrt(gnssraw_vx * gnssraw_vx + gnssraw_vy * gnssraw_vy);
        phone_initial_state.vy = 0.0;
    } else if (sensors_for_initial_state[0] == "gps" && update_gps &&
               sensors_for_initial_state[1] == "gps" && update_gps &&
               sensors_for_initial_state[2] == "gps" && update_gps) {
        geo.llh_to_xyz_us(gps.latitude, gps.longitude, 0);
        double x = geo.y(), y = -geo.x(), s = gps.header.stamp.toSec();
        static double x_first = x, y_first = y, s_first = s;
        double dx = x - x_first, dy = y - y_first, dt = s - s_first;
        if (std::sqrt(dx * dx + dy * dy) < 1.0) {
            ROS_WARN("Not initialized yed. Waiting for second gps data.");
            return false;
        }
        phone_initial_state.x = x;
        phone_initial_state.y = y;
        phone_initial_state.r = std::atan2(dy, dx);
        phone_initial_state.vx = std::sqrt(dx * dx + dy * dy) / dt;
        phone_initial_state.vy = 0.0;
    } else if (sensors_for_initial_state[0] == "vehicle_perception" && update_vehicle_perception &&
               sensors_for_initial_state[1] == "gnssraw_acceleration" && update_gnssraw_acceleration &&
               sensors_for_initial_state[2] == "gnssraw_acceleration" && update_gnssraw_acceleration) {
        double gnssraw_vx = gnssraw_acceleration.accel.accel.linear.y;
        double gnssraw_vy = -gnssraw_acceleration.accel.accel.linear.x;
        double gnssraw_yaw = std::atan2(gnssraw_vy, gnssraw_vx);
        phone_initial_state.x = vehicle_perception.pose.pose.position.x;
        phone_initial_state.y = vehicle_perception.pose.pose.position.y;
        phone_initial_state.r = gnssraw_yaw;
        phone_initial_state.vx = std::sqrt(gnssraw_vx * gnssraw_vx + gnssraw_vy * gnssraw_vy);
        phone_initial_state.vy = 0.0;
    } else if (sensors_for_initial_state[0] == "manual" &&
               sensors_for_initial_state[1] == "manual" &&
               sensors_for_initial_state[2] == "manual") {
        std::vector<double> pis;
        if (!private_nh.getParam("phone_initial_state", pis)) {
            ROS_ERROR("Non \"phone_initial_state\"");
            exit(-1);
        }
        phone_initial_state.set(pis[0], pis[1], pis[2], pis[3], pis[4]);
    } else {
        ROS_WARN("Not initialized yed. Waiting for sensors data. \nimu:%d mag:%d gps:%d raw_fix:%d raw_acc:%d ",
                 update_imu, update_magnetic, update_gps, update_gnssraw_fix, update_gnssraw_acceleration);
        return false;
    }
    ROS_INFO("Initialize PF using phone sensors");
*/
    if (sensors_for_initial_state[0] == "manual" &&
        sensors_for_initial_state[1] == "manual" &&
        sensors_for_initial_state[2] == "manual") {
        std::vector<double> pis;
        if (!private_nh.getParam("phone_initial_state", pis)) {
            ROS_ERROR("Non \"phone_initial_state\"");
            exit(-1);
        }
        phone_initial_state = Eigen::MatrixXd(5, 1);
        phone_initial_state << pis[0], pis[1], pis[3], pis[2], 0.0;
    } else {
        ROS_WARN("Not initialized yed. Waiting for sensors data. \nimu:%d mag:%d gps:%d raw_fix:%d raw_acc:%d ",
                 update_imu, update_magnetic, update_gps, update_gnssraw_fix, update_gnssraw_acceleration);
        return false;
    }

    time = ros::Time::now();

    now_phone_state = phone_initial_state;
    ukf.initialize(phone_initial_state, time.toSec());

    publish_odom(time);

    ROS_DEBUG("Complete UKF Initialization");

    return true;
}

/**
 *
 */
void PhoneLocalizerUKF::ukfUpdateIMU() {

    //// TODO UKF IMU update
    static bool init = false;
    static std::vector<double> rpy(3), rpy_pre(3);
    static double t, t_pre;
    if (!init) {
        imu2rpy(imu, &rpy_pre[0], &rpy_pre[1], &rpy_pre[2]);
        t_pre = time.toSec();
        ukf.time_ = time.toSec();
        init = true;
        return;
    }
    imu2rpy(imu, &rpy[0], &rpy[1], &rpy[2]);
    t = time.toSec();
    double dt = t - t_pre;
    if (dt <= 0.0) {
        return;
    }
    double yawd = (rpy[2] - rpy_pre[2]) / dt;
    rpy_pre = rpy;
    t_pre = t;

    ofs_imu << time.sec << "." << time.nsec << "," << yawd << std::endl;

    double gsd;
    private_nh.param("ukf_imu_r_sd", gsd, UKF_IMU_R_STANDARD_DEVIATION);

    std::vector<int> update_state = {4};
    Eigen::MatrixXd r(1, 1);
    r << gsd * gsd;
    Eigen::VectorXd z(1, 1);
    z << yawd;

    std::cout << __FUNCTION__ << "\n" << z << "\n" << r << std::endl;

    ukf.r_ctrv_ = r;
/*
    ukf.predictionMeasurement(MotionModel::CV, update_state);
*/
    ukf.predictionMeasurement(MotionModel::CTRV, update_state);
/*
    ukf.predictionMeasurement(MotionModel::RM, update_state);
    ukf.updateKalmanGain(MotionModel::CV, update_state);
*/
    ukf.updateKalmanGain(MotionModel::CTRV, update_state);
/*
    ukf.updateKalmanGain(MotionModel::RM, update_state);
*/

    std::vector<double> lambda_vec;
    ukf.updateEachMotion(detection_probability, gate_probability, gating_thres, z, lambda_vec);
/*
    ukf.updateModeProb(lambda_vec);
    ukf.mergeEstimationAndCovariance();
*/

}

/**
 * ukfUpdateGnssrawAcceleration
 */
void PhoneLocalizerUKF::ukfUpdateGnssrawAcceleration() {

    double gnssraw_vx = gnssraw_acceleration.accel.accel.linear.y;
    double gnssraw_vy = -gnssraw_acceleration.accel.accel.linear.x;
    double gnssraw_v = std::sqrt(gnssraw_vx * gnssraw_vx + gnssraw_vy * gnssraw_vy);
    double gnssraw_r = std::atan2(gnssraw_vy, gnssraw_vx);
    if (gnssraw_v < 0.1) {
        gnssraw_v = 0.0;
    }

    double v_sd, r_sd;
    private_nh.param("ukf_gnssraw_v_standard_deviation", v_sd, UKF_GNSSRAW_V_STANDARD_DEVIATION);
    private_nh.param("ukf_gnssraw_r_standard_deviation", r_sd, UKF_GNSSRAW_R_STANDARD_DEVIATION);

    std::vector<int> update_state;
    Eigen::MatrixXd r;
    Eigen::VectorXd z;
    if (use_gnssraw_acceleration_v && use_gnssraw_acceleration_r && gnssraw_v >= 0.1) {
        //// TODO UKF GNSS_VR update
        ROS_DEBUG("GNSS_VR update");
        update_state = {2, 3};
        r = Eigen::MatrixXd(2, 2);
        r << v_sd * v_sd, 0.0, 0.0, r_sd * r_sd;
        z = Eigen::VectorXd(2);
        z << gnssraw_v, gnssraw_r;
    } else {
        //// TODO UKF GNSS_VR update
        ROS_DEBUG("GNSS_V update");
        update_state = {2};
        r = Eigen::MatrixXd(1, 1);
        r << v_sd * v_sd;
        z = Eigen::VectorXd(1);
        z << gnssraw_v;
    }

    ukf.r_ctrv_ = r;
    ukf.predictionMeasurement(MotionModel::CTRV, update_state);
    ukf.updateKalmanGain(MotionModel::CTRV, update_state);
    std::vector<double> lambda_vec;
    ukf.updateEachMotion(detection_probability, gate_probability, gating_thres, z, lambda_vec);

}

void PhoneLocalizerUKF::ukfUpdateVehiclePerception() {

    //// TODO UKF Perception update
    std::vector<int> update_state = {0, 1};
    Eigen::MatrixXd r(2, 2);
    r << vehicle_perception.pose.covariance[0], vehicle_perception.pose.covariance[1],
            vehicle_perception.pose.covariance[6], vehicle_perception.pose.covariance[7];
    Eigen::VectorXd z(2);
    z << vehicle_perception.pose.pose.position.x, vehicle_perception.pose.pose.position.y;

    ukf.r_ctrv_ = r;
    ukf.predictionMeasurement(MotionModel::CTRV, update_state);
    ukf.updateKalmanGain(MotionModel::CTRV, update_state);

    std::vector<double> lambda_vec;
    ukf.updateEachMotion(detection_probability, gate_probability, gating_thres, z, lambda_vec);

}

void PhoneLocalizerUKF::imu2rpy(const sensor_msgs::Imu &imu, double *rpy) {

    tf::Quaternion quat(imu.orientation.x, imu.orientation.y, imu.orientation.z, imu.orientation.w);
    tf::Matrix3x3(quat).getRPY(*rpy, *(rpy + 1), *(rpy + 2));

}

void PhoneLocalizerUKF::imu2rpy(const sensor_msgs::Imu &imu, double *roll, double *pitch, double *yaw) {

    tf::Quaternion quat(imu.orientation.x, imu.orientation.y, imu.orientation.z, imu.orientation.w);
    tf::Matrix3x3(quat).getRPY(*roll, *pitch, *yaw);

}

void PhoneLocalizerUKF::magnetic2yaw(const sensor_msgs::MagneticField &magnetic, double *rpy) {

    Eigen::Vector3f mag(magnetic.magnetic_field.x, magnetic.magnetic_field.y, magnetic.magnetic_field.z);
    Eigen::Matrix3f rp_mat;
    rpy2matrix(-rpy[0], -rpy[1], 0.0, &rp_mat);
    Eigen::Vector3f mag_horizontal = rp_mat * mag;
    *(rpy + 2) = -std::atan2(-mag_horizontal(0), mag_horizontal(1));

}

void PhoneLocalizerUKF::rpy2matrix(double roll, double pitch, double yaw, Eigen::Matrix3f *rotation_matrix) {
    Eigen::Vector3f axis_dr(1, 0, 0), axis_dp(0, 1, 0), axis_dy(0, 0, 1);
    Eigen::Matrix3f matrix_dr, matrix_dp, matrix_dy;
    matrix_dr = Eigen::AngleAxisf(roll, axis_dr);
    matrix_dp = Eigen::AngleAxisf(pitch, axis_dp);
    matrix_dy = Eigen::AngleAxisf(yaw, axis_dy);
    *rotation_matrix = matrix_dy * matrix_dp * matrix_dr;
}

void
PhoneLocalizerUKF::quaternion2matrix(double qx, double qy, double qz, double qw, Eigen::Matrix3f *rotation_matrix) {
    tf::Quaternion quat(qx, qy, qz, qw);
    double rpy[3];
    tf::Matrix3x3(quat).getRPY(rpy[0], rpy[1], rpy[2]);
    rpy2matrix(rpy[0], rpy[1], rpy[2], rotation_matrix);
}
