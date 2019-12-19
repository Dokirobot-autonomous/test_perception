//
// Created by ohashi on 19/09/13.
//

#include <phone_localizer.h>

PhoneLocalizer::PhoneLocalizer(ros::NodeHandle& nh, ros::NodeHandle& private_nh) :
                                    nh(nh), private_nh(private_nh),
                                    first_orientation_computation(true), first_pose_computation(true),
                                    update_imu(false), update_magnetic(false), update_gps(false),
                                    particle_size(PARTICLEFILTER_PARTICLE_SIZE) {

    ROS_DEBUG("%s", __FUNCTION__);

    /** Set parameter **/

    //// phone name
    if (phone_name.empty()) {
        if (!private_nh.getParam("phone_name", phone_name)) {
            ROS_DEBUG("phone_name is #define");
            phone_name = PHONE_NAME;
        }
    }

    //// origin
    std::string str;
    if (!nh.getParam("origin_fix", str)) {
        ROS_DEBUG("origin_fix is #define");
        str = ORIGIN_FIX;
    }
    sscanf(str.c_str(), "%lf,%lf,%lf", &origin_lat, &origin_lon, &origin_ele);
    geo.set_plane(origin_lat, origin_lon);

    //// TF Listener
    tf2_ros::TransformListener tf_listener(tf_buffer);

    //// random seed
    mt.seed(rnd());

    /** Set publisher **/

    //// Odom
    pub_odom = private_nh.advertise<nav_msgs::Odometry>("/" + phone_name + "/" + TOPIC_NAME_PUBLISH_ODOM, 1);
    if (!nh.getParam("origin_frame_id", str)) {
        ROS_DEBUG("origin_frame_id is #define");
        str = ORIGIN_FRAME_ID;
    }
    odom.header.frame_id = origin_frame_id = str;
    odom.child_frame_id = phone_frame_id = phone_name;

    //// Particles
    pub_particles = private_nh.advertise<geometry_msgs::PoseArray>(
            "/" + phone_name + "/" + TOPIC_NAME_PUBLISH_POSEARRAY, 1);
    posearray.header.frame_id = origin_frame_id;


    /** Initialize ParticleFilter **/
    particleInitialization();

    /** Set subscriber **/
    bool use_imu, use_mag, use_gps;
    if (!private_nh.getParam("use_imu", use_imu)) {
        ROS_DEBUG("use_imu is #define");
        use_imu = USE_IMU;
    }
    if (!private_nh.getParam("use_magnetic", use_mag)) {
        ROS_DEBUG("use_magnetic is #define");
        use_mag = USE_MAGNETIC;
    }
    if (!private_nh.getParam("use_gps", use_gps)) {
        ROS_DEBUG("use_gps is #define");
        use_gps = USE_GPS;
    }
    if (use_imu) {
        sub_imu = nh.subscribe("/" + phone_name + "/" + TOPIC_NAME_IMU, 1, &PhoneLocalizer::callback_imu, this);
    }
    if (use_mag) {
        sub_magnetic = nh.subscribe("/" + phone_name + "/" + TOPIC_NAME_MAGNETIC, 1, &PhoneLocalizer::callback_magnetic,
                                    this);
    }
    if (use_gps) {
        sub_gps = nh.subscribe("/" + phone_name + "/" + TOPIC_NAME_GPS, 1, &PhoneLocalizer::callback_gps, this);
    }

    ros::spin();
}

PhoneLocalizer::PhoneLocalizer( ros::NodeHandle& nh, ros::NodeHandle& private_nh,
                                std::string phone_name) : phone_name(phone_name) {
    PhoneLocalizer(nh,private_nh);
}

PhoneLocalizer::~PhoneLocalizer() {};

void PhoneLocalizer::set_phone_name(std::string &phone_name) {
    this->phone_name = phone_name;
}

void PhoneLocalizer::callback_imu(const sensor_msgs::Imu::ConstPtr &msg) {

    ROS_DEBUG("%s", __FUNCTION__);

    imu = *msg;
    time = msg->header.stamp;
    update_imu = true;
    run();
}

void PhoneLocalizer::callback_magnetic(const sensor_msgs::MagneticField::ConstPtr &msg) {

    ROS_DEBUG("%s", __FUNCTION__);

    magnetic = *msg;
    time = msg->header.stamp;
    update_magnetic = true;
    run();
}

void PhoneLocalizer::callback_gps(const sensor_msgs::NavSatFix::ConstPtr &msg) {

    ROS_DEBUG("%s", __FUNCTION__);

    gps = *msg;
    time = msg->header.stamp;
    update_gps = true;
    run();
}

void PhoneLocalizer::publish_odom() {

    ROS_DEBUG("%s", __FUNCTION__);

    odom.pose.pose.position.x = now_phone_state.x;
    odom.pose.pose.position.y = now_phone_state.y;
    odom.pose.pose.position.z = 0.0;
    tf::Quaternion quat = tf::createQuaternionFromRPY(0.0, 0.0, now_phone_state.r);
    quaternionTFToMsg(quat, odom.pose.pose.orientation);
    odom.twist.twist.linear.x = now_phone_state.vx;
    odom.twist.twist.linear.y = now_phone_state.vy;

    odom.header.stamp = time;

    pub_odom.publish(odom);

    ROS_DEBUG_STREAM("Phone State \n" << now_phone_state);
}

void PhoneLocalizer::publish_poseaary() {

    ROS_DEBUG("%s", __FUNCTION__);

    posearray.poses.clear();

    for (auto particle:getParticles()) {
        State<> s = *particle->getState();
        geometry_msgs::Pose pose;
        pose.position.x = s.x;
        pose.position.y = s.y;
        pose.position.z = 0.0;
        tf::Quaternion q = tf::createQuaternionFromRPY(0.0, 0.0, s.r);
        quaternionTFToMsg(q, pose.orientation);
        posearray.poses.push_back(pose);
    }
    posearray.header.stamp = time;

    pub_particles.publish(posearray);

    static int i = 0;
    if (i % 100 == 0) {
        ROS_DEBUG_STREAM("Particle States \n" << getParticles());
    }
    i++;

}

void PhoneLocalizer::run() {

    ROS_DEBUG("%s", __FUNCTION__);

    std::lock_guard<std::mutex> guard(lock);

    for (auto p:getParticles()) {
        p->getState()->normalize_r();
    }

    if (update_imu) {
        particleTransition();
//        particleLikelihoodIMU();
        update_imu = false;
    }

    if (update_magnetic) {
        particleLikelihoodMagnetic();
        update_magnetic = false;
    }

    if (update_gps) {
        particleLikelihoodGPS();
        particleLikelihoodGpsXY();
        particleLikelihoodGpsV();
//        particleLikelihoodGpsR();
        update_gps = false;
    }

    //// Neff check
    if (isParticleReample()) {
        ROS_INFO("Resampling!");
        particleResample();

        //// add system noise
        std::string str;
        if (!private_nh.getParam("particlefilter_system_noise_standard_deviation", str)) {
            ROS_DEBUG("particlefilter_system_noise_standard_deviation is #define");
            str = PARTICLEFILTER_SYSTEM_NOISE_STANDARD_DEVIATION;
        }
        State<> sd;
        sscanf(str.c_str(), "%lf,%lf,%lf,%lf,%lf", &sd.x, &sd.y, &sd.r, &sd.vx, &sd.vy);
        particleAddSystemNoise(sd);
    }


    /** Publish Current Status **/

    //// Odom
    now_phone_state = getWeightedMean();
    publish_odom();

    //// Particles
    publish_poseaary();

}

void PhoneLocalizer::particleInitialization() {

    ROS_DEBUG("%s", __FUNCTION__);

    getParticles().clear();

    //// Preparing phone initial parameter
    double phone_initial_lat, phone_initial_lon, phone_initial_ele;
    std::string str;
    if (!private_nh.getParam("phone_initial_fix", str)) {
        ROS_DEBUG("phone_initial_fix is #define");
        str = PHONE_INITIAL_FIX;
    }
    sscanf(str.c_str(), "%lf,%lf,%lf", &phone_initial_lat, &phone_initial_lon, &phone_initial_ele);
    if (!private_nh.getParam("phone_initial_direction", str)) {
        ROS_DEBUG("phone_initial_direction is #define");
        str = PHONE_INITIAL_DERECTION;
    }
    double phone_initial_direction = std::stod(str);
//    if (!private_nh.getParam("phone_initial_quaternion",str)){
//        str=PHONE_INITIAL_QUATERNION;
//    }
//    double tmp[4],rpy[3];
//    sscanf(str.c_str(),"%lf,%lf,%lf,%lf",&tmp[0],&tmp[1],&tmp[2],&tmp[3]);
//    tf::Quaternion quat(tmp[0],tmp[1],tmp[2],tmp[3]),quat_phone;
//    tf::Matrix3x3(quat).getRPY(rpy[0], rpy[1], rpy[2]);
//    tf_listener.transformQuaternion(phone_frame_id,ros::Time::now(),tmp,phone_frame_id,quat_phone);
//    tf::Matrix3x3(quat_phone).getRPY(rpy[0], rpy[1], rpy[2]);

    //// Setting phone initial state
    geo.llh_to_xyz(phone_initial_lat, phone_initial_lon, 0);
    phone_initial_state.x = geo.x();
    phone_initial_state.y = geo.y();
//    phone_initial_state.r=rpy[2];
    phone_initial_state.r = phone_initial_direction;
    phone_initial_state.vx = 0.0;
    phone_initial_state.vy = 0.0;
    now_phone_state = phone_initial_state;


    //// Setting particles sampling range
    double xy_radius, r_radius, v_radius;
    if (!private_nh.getParam("particlefilter_initial_xy_radius", xy_radius)) {
        ROS_DEBUG("particlefilter_initial_xy_radius is #define");
        xy_radius = PARTICLEFILTER_INITIAL_XY_RADIUS;
    }
    if (!private_nh.getParam("particlefilter_initial_r_radius", r_radius)) {
        ROS_DEBUG("particlefilter_initial_r_radius is #define");
        r_radius = PARTICLEFILTER_INITIAL_R_RADIUS;
    }
    if (!private_nh.getParam("particlefilter_initial_v_radius", v_radius)) {
        ROS_DEBUG("particlefilter_initial_v_radius is #define");
        v_radius = PARTICLEFITLER_INITIAL_V_RADIUS;
    }
    std::uniform_real_distribution<double> rand_xy(-xy_radius, xy_radius);
    std::uniform_real_distribution<double> rand_theta(-M_PI, M_PI);
    std::uniform_real_distribution<double> rand_r(-r_radius * M_PI / 180, r_radius * M_PI / 180);
    std::uniform_real_distribution<double> rand_v(-v_radius, v_radius);


    //// Sampling particles
    for (size_t i = 0; i < particle_size; i++) {
        Particle<State<>> *p = new Particle<State<>>;
        State<> *s = new State<>;
        double rand_x = rand_xy(mt);
        double rand_y = rand_xy(mt);
        double rand_vx = rand_v(mt);
        double rand_vy = rand_v(mt);
        while (rand_x * rand_x + rand_y * rand_y > xy_radius * xy_radius) {
            rand_x = rand_xy(mt);
            rand_y = rand_xy(mt);
        }
        while (rand_vx * rand_vx + rand_vy * rand_vy > v_radius * v_radius) {
            rand_vx = rand_v(mt);
            rand_vy = rand_v(mt);
        }

        double x = rand_x + phone_initial_state.x;
        double y = rand_y + phone_initial_state.y;
        double r = rand_r(mt) + phone_initial_state.r;
//        double vx=rand_vx+phone_initial_state.vx;
        double vx = 0.0;
        double vy = rand_vy + phone_initial_state.vy;

        s->set(x, y, r, vx, vy);
        p->setState(s);
        p->setWeight(1.0);
        getParticles().push_back(p);
    }

    particleNormalize();

    time = ros::Time::now();

    publish_odom();
    publish_poseaary();

}

void PhoneLocalizer::particleTransition() {

    ROS_DEBUG("%s", __FUNCTION__);

    static bool init = true;
    static std::vector<double> now_imu_rpy(3), pre_imu_rpy(3);
    static double now_time, pre_time;

    /** Cordinate transformation from phone to world **/
    std::vector<double> imu_rpy(3);
    tf::Quaternion quat(imu.orientation.x, imu.orientation.y, imu.orientation.z, imu.orientation.w);
    tf::Matrix3x3(quat).getRPY(imu_rpy[0], imu_rpy[1], imu_rpy[2]);
    Eigen::Vector3f axis_dr(1, 0, 0), axis_dp(0, 1, 0), axis_dy(0, 0, 1);
    Eigen::Matrix3f matrix_dr, matrix_dp, matrix_dy;
    matrix_dr = Eigen::AngleAxisf(imu_rpy[0], axis_dr);
    matrix_dp = Eigen::AngleAxisf(imu_rpy[1], axis_dp);
    matrix_dy = Eigen::AngleAxisf(now_phone_state.r, axis_dy);
    phone2phone = matrix_dp * matrix_dr;
    phone2ground = matrix_dy * phone2phone;

    //// If initial, set previous time and skip calculation
    if (init) {
        now_imu_rpy = pre_imu_rpy = imu_rpy;
        now_time = pre_time = imu.header.stamp.toSec();
        init = false;
        return;
    }

    pre_imu_rpy = now_imu_rpy;
    pre_time = now_time;
    now_imu_rpy = imu_rpy;
    now_time = imu.header.stamp.toSec();
    double d_t = now_time - pre_time;


    /** Gyro and acceleration **/
//    //// acceleration
    double axb, ayb, gzb;
//    if (!private_nh.getParam("acceleration_x_bias",axb)){
//        axb=ACCELERATION_X_BIAS;
//    }
//    if (!private_nh.getParam("acceleration_y_bias",ayb)){
//        ayb=ACCELERATION_Y_BIAS;
//    }
//    Eigen::Vector3f accel(imu.linear_acceleration.x,imu.linear_acceleration.y,imu.linear_acceleration.z);
//    Eigen::Vector3f phone_accel=-1*phone2phone*accel-Eigen::Vector3f(axb,ayb,0.0);   // 座標系ではカメラ反対方向を正で定義しているにもかかわらず，カメラ方向を正で出力している
    //// gyro
    if (!private_nh.getParam("gyro_z_bias", gzb)) {
        ROS_DEBUG("gyro_z_bias is #define");
        gzb = GYRO_Z_BIAS;
    }
    double diff_phone_yaw = State<>::normalize_radian(now_imu_rpy[2] - pre_imu_rpy[2]);
//    std::cout<<now_imu_rpy[2]<<","<<pre_imu_rpy[2]<<","<<diff_phone_yaw<<std::endl;


    /** Particle Transition **/
    double asd, gsd;
//    if (!private_nh.getParam("particlefilter_acceleration_standard_deviation",asd)){
//        asd=PARTICLEFILTER_ACCELERATION_STANDARD_DEVIATION;
//    }
    if (!private_nh.getParam("particlefilter_gyro_standard_deviation", gsd)) {
        ROS_DEBUG("particlefilter_gyro_standard_deviation is #define");
        gsd = PARTICLEFILTER_GYRO_STANDARD_DEVIATION;
    }

//    std::normal_distribution<double> rand_accel(0,asd);
    std::normal_distribution<double> rand_gyro(0, gsd);
    for (auto particle:getParticles()) {

        State<> s = *particle->getState();

        //// Adding transition noise
//        double ax=phone_accel(0)+rand_accel(mt);
//        double ay=phone_accel(1)+rand_accel(mt);
//        double gz=diff_phone_yaw+rand_gyro(mt);

        //// Axis conversion
        Eigen::Matrix3f phone2ground_p;
        phone2ground_p = Eigen::AngleAxisf(s.r, axis_dy);
        Eigen::Vector3f ground_velocity = phone2ground_p * Eigen::Vector3f(s.vx, s.vy, 0.0);
//        Eigen::Vector3f ground_accel=phone2ground_p*Eigen::Vector3f(ax,ay,0.0);

        //// Transition
//        s.x=s.x+ground_velocity(0)*d_t+ground_accel(0)*d_t*d_t/2.0;
//        s.y=s.y+ground_velocity(1)*d_t+ground_accel(1)*d_t*d_t/2.0;
        s.x = s.x + ground_velocity(0) * d_t;
        s.y = s.y + ground_velocity(1) * d_t;
        s.vx = s.vx;
        s.vy = s.vy;
        s.r = s.r + diff_phone_yaw + rand_gyro(mt) * d_t;
//        s.r=s.r+gz*d_t;

        *particle->getState() = s;
    }

//    std::cout<<*getParticles()[0]->getState()<<std::endl;

//    static double sum[4]={0.0,0.0,0.0,0.0};
//    static double num=0.0;
//    sum[0]+=phone_accel(0);
//    sum[1]+=phone_accel(1);
//    sum[2]+=phone_accel(2);
//    sum[3]+=phone_gyro(2);

//    //// Debug
//    Eigen::Vector3f ground_accel=phone2ground*accel;
//    ROS_DEBUG_STREAM(
//            "Values in "<< __FUNCTION__ <<"\n"<<
//                        "d_t: " <<d_t<<
//                        "\t accel(0): "<<ground_accel(0) <<
//                        "\t accel(1): "<<ground_accel(1) <<
//                        "\t accel(2): "<<ground_accel(2));

}

void PhoneLocalizer::particleLikelihoodIMU() {

    ROS_DEBUG("%s", __FUNCTION__);

    double rpy[3];
    tf::Quaternion quat(imu.orientation.x, imu.orientation.y, imu.orientation.z, imu.orientation.w);
    tf::Matrix3x3(quat).getRPY(rpy[0], rpy[1], rpy[2]);

    double sd;
    if (!private_nh.getParam("particlefitler_imu_yaw_standard_deviation", sd)) {
        ROS_DEBUG("particlefitler_imu_yaw_standard_deviation is #define");
        sd = PARTICLEFILTER_IMU_YAW_STANDARD_DEVIATION;
    }

    for (auto particle:getParticles()) {
        double r = particle->getState()->r - rpy[2];
        r = State<>::normalize_radian(r);
        double w = std::exp(-r * r / (2 * sd * sd));
        w *= particle->getWeight();
        particle->setWeight(w);
    }

//    std::cout << "Particle States \n" << getParticles() << std::endl;
    particleNormalize();

}

void PhoneLocalizer::particleLikelihoodMagnetic() {

    ROS_DEBUG("%s", __FUNCTION__);

    ROS_WARN("Non implementation!");

//    Eigen::Vector3f mag(magnetic.magnetic_field.x, magnetic.magnetic_field.y, magnetic.magnetic_field.z);
//    Eigen::Vector3f mag_phone=phone2phone*mag;
//    double yaw_mag=std::atan2(mag_phone(1),mag_phone(0));    // if only atan2(), degree is from west.
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

void PhoneLocalizer::particleLikelihoodGPS() {

    ROS_DEBUG("%s", __FUNCTION__);

    geo.llh_to_xyz(gps.latitude, gps.longitude, gps.altitude);

    static bool init = true;
    static Eigen::Vector3d now_gps, pre_gps;
    static double now_time, pre_time;
    if (init) {
        now_gps = pre_gps = Eigen::Vector3d(geo.x(), geo.y(), geo.z());
        now_time = pre_time = time.toSec();
        init = false;
        return;
    }
    pre_gps = now_gps;
    pre_time = now_time;
    now_gps = Eigen::Vector3d(geo.x(), geo.y(), geo.z());
    now_time = time.toSec();

    double d_t = now_time - pre_time;
    double v_x = (now_gps(0) - pre_gps(0)) / d_t;
    double v_y = (now_gps(1) - pre_gps(1)) / d_t;

    double xy_bias, v_bias, xy_sd, v_sd;
    if (!private_nh.getParam("particlefitler_gps_xy_bias", xy_bias)) {
        ROS_DEBUG("particlefitler_gps_xy_bias is #define");
        xy_bias = PARTICLEFILTER_GPS_XY_BIAS;
    }
    if (!private_nh.getParam("particlefitler_gps_v_bias", v_bias)) {
        ROS_DEBUG("particlefitler_gps_v_bias is #define");
        v_bias = PARTICLEFILTER_GPS_V_BIAS;
    }
    if (!private_nh.getParam("particlefitler_gps_xy_standard_deviation", xy_sd)) {
        ROS_DEBUG("particlefitler_gps_xy_standard_deviation is #define");
        xy_sd = PARTICLEFILTER_GPS_XY_STANDARD_DEVIATION;
    }
    if (!private_nh.getParam("particlefitler_gps_v_standard_deviation", v_sd)) {
        ROS_DEBUG("particlefitler_gps_v_standard_deviation is #define");
        v_sd = PARTICLEFILTER_GPS_V_STANDARD_DEVIATION;
    }


    /** Compute Likelihood **/
    std::vector<double> w_xy_vec, w_v_vec;
    for (auto particle:getParticles()) {
        State<> s = *particle->getState();

        //// Likelihood XY
        double r_xy = std::sqrt(std::pow(s.x - now_gps(0), 2) + std::pow(s.y - now_gps(1), 2));
        double w_xy;
        if (r_xy <= xy_bias) {
            w_xy = 1.0;
        } else {
            w_xy = std::exp(-std::pow(r_xy - xy_bias, 2) / (2 * xy_sd * xy_sd));
        }
        w_xy_vec.push_back(w_xy);

        //// Axis conversion
        Eigen::Vector3f axis_dy(0, 0, 1);
        Eigen::Matrix3f phone2ground_p;
        phone2ground_p = Eigen::AngleAxisf(s.r, axis_dy);
        Eigen::Vector3f ground_velocity = phone2ground_p * Eigen::Vector3f(s.vx, s.vy, 0.0);

        //// Likelihood V
        double w_v;
        double r_v = std::sqrt(std::pow(ground_velocity(0) - v_x, 2) + std::pow(ground_velocity(1) - v_y, 2));
//        std::cout<<r_v<<std::endl;
        if (r_v < v_bias) {
            w_v = 1.0;
        } else {
            w_v = std::exp(-std::pow(r_v - v_bias, 2) / (2 * v_sd * v_sd));
        }
        w_v_vec.push_back(w_v);

    }

    double w_xy_sum = std::accumulate(w_xy_vec.begin(), w_xy_vec.end(), 0.0);
    double w_v_sum = std::accumulate(w_v_vec.begin(), w_v_vec.end(), 0.0);
    for (size_t i = 0; i < getParticles().size(); i++) {
        double w = getParticles()[i]->getWeight();
        if (w_xy_sum != 0.0) {
            w *= w_xy_vec[i];
        }
        if (w_v_sum != 0.0) {
            w *= w_v_vec[i];
        }
        getParticles()[i]->setWeight(w);
    }

//    std::cout << "Particle States \n" << getParticles() << std::endl;
    particleNormalize();

}

void PhoneLocalizer::particleLikelihoodGpsXY() {

    ROS_DEBUG("%s", __FUNCTION__);

    geo.llh_to_xyz(gps.latitude, gps.longitude, gps.altitude);

    double xy_bias, xy_sd;
    if (!private_nh.getParam("particlefitler_gps_xy_bias", xy_bias)) {
        ROS_DEBUG("particlefitler_gps_xy_bias is #define");
        xy_bias = PARTICLEFILTER_GPS_XY_BIAS;
    }
    if (!private_nh.getParam("particlefitler_gps_xy_standard_deviation", xy_sd)) {
        ROS_DEBUG("particlefitler_gps_xy_standard_deviation is #define");
        xy_sd = PARTICLEFILTER_GPS_XY_STANDARD_DEVIATION;
    }

    /** Compute Likelihood **/
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
        w *= particle->getWeight();
        particle->setWeight(w);
    }

//    std::cout << "Particle States \n" << getParticles() << std::endl;
    particleNormalize();

}

void PhoneLocalizer::particleLikelihoodGpsV() {

    ROS_DEBUG("%s", __FUNCTION__);

    geo.llh_to_xyz(gps.latitude, gps.longitude, gps.altitude);

    static bool init = true;
    static Eigen::Vector3d now_gps, pre_gps;
    static double now_time, pre_time;
    if (init) {
        now_gps = pre_gps = Eigen::Vector3d(geo.x(), geo.y(), geo.z());
        now_time = pre_time = time.toSec();
        init = false;
        return;
    }
    pre_gps = now_gps;
    pre_time = now_time;
    now_gps = Eigen::Vector3d(geo.x(), geo.y(), geo.z());
    now_time = time.toSec();

    double dx = now_gps(0) - pre_gps(0);
    double dy = now_gps(1) - pre_gps(1);
    double d_t = now_time - pre_time;
    double gps_v = std::sqrt(dx * dx + dy * dy) / d_t;

    double v_bias, v_sd;
    if (!private_nh.getParam("particlefitler_gps_v_bias", v_bias)) {
        ROS_DEBUG("particlefitler_gps_v_bias is #define");
        v_bias = PARTICLEFILTER_GPS_V_BIAS;
    }
    if (!private_nh.getParam("particlefitler_gps_v_standard_deviation", v_sd)) {
        ROS_DEBUG("particlefitler_gps_v_standard_deviation is #define");
        v_sd = PARTICLEFILTER_GPS_V_STANDARD_DEVIATION;
    }

    /** Compute Likelihood **/
    for (auto particle:getParticles()) {
        State<> s = *particle->getState();

        //// Likelihood V
        double w;
        double s_v = std::sqrt(s.vx * s.vx + s.vy * s.vy);
        double r_v = std::abs(s_v - gps_v);
        if (r_v < v_bias) {
            w = 1.0;
        } else {
            w = std::exp(-std::pow(r_v - v_bias, 2) / (2 * v_sd * v_sd));
        }

        w *= particle->getWeight();
        particle->setWeight(w);
    }

//    std::cout << "Particle States \n" << getParticles() << std::endl;
    particleNormalize();

}

void PhoneLocalizer::particleLikelihoodGpsR() {

    ROS_DEBUG("%s", __FUNCTION__);

    geo.llh_to_xyz(gps.latitude, gps.longitude, gps.altitude);

    static bool init = true;
    static Eigen::Vector3d now_gps, pre_gps;
    static double now_time, pre_time;
    if (init) {
        now_gps = pre_gps = Eigen::Vector3d(geo.x(), geo.y(), geo.z());
        now_time = pre_time = time.toSec();
        init = false;
        return;
    }
    pre_gps = now_gps;
    pre_time = now_time;
    now_gps = Eigen::Vector3d(geo.x(), geo.y(), geo.z());
    now_time = time.toSec();

    double dx = now_gps(0) - pre_gps(0);
    double dy = now_gps(1) - pre_gps(1);
    if (std::sqrt(dx * dx + dy * dy) < 1.0) {
        return;
    }

    double gps_r = std::atan2(dy, dx) - M_PI / 2.0;
    gps_r = State<>::normalize_radian(gps_r);
    ROS_DEBUG_STREAM("Mean R: " << now_phone_state.r << ", GPS R: " << gps_r);

    double r_sd;
    if (!private_nh.getParam("particlefitler_gps_r_standard_deviation", r_sd)) {
        ROS_DEBUG("particlefitler_gps_r_standard_deviation is #define");
        r_sd = PARTICLEFILTER_GPS_R_STANDARD_DEVIATION;
    }


    /** Compute Likelihood **/
    for (auto particle:getParticles()) {
        State<> s = *particle->getState();

        //// Likelihood V
        double w;
        double r_r = std::abs(State<>::normalize_radian(s.r - gps_r));
        w = std::exp(-std::pow(r_r, 2) / (2 * r_sd * r_sd));

        w *= particle->getWeight();
        particle->setWeight(w);
    }

//    std::cout << "Particle States \n" << getParticles() << std::endl;
    particleNormalize();

}

