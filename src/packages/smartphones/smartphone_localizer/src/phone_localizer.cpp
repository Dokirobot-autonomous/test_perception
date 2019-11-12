//
// Created by ohashi on 19/09/13.
//

#include <phone_localizer.h>

PhoneLocalizer::PhoneLocalizer(ros::NodeHandle* nh, ros::NodeHandle* private_nh) :
                                    nh(*nh), private_nh(*private_nh),
                                    update_imu(false), update_magnetic(false), update_gps(false),
                                    particle_size(PARTICLEFILTER_PARTICLE_SIZE) {

    ROS_DEBUG("%s", __FUNCTION__);

    /** Set parameter **/

    //// phone name
    this->private_nh.param<std::string>("phone_name",phone_name,PHONE_NAME);

    //// origin
    std::string origin_fix_str;
    this->nh.param<std::string>("origin_fix", origin_fix_str,ORIGIN_FIX);
    sscanf(origin_fix_str.c_str(), "%lf,%lf,%lf", &origin_lat, &origin_lon, &origin_ele);
    geo.set_plane(origin_lat, origin_lon);

    //// origin frame_id
    this->nh.param<std::string>("origin_frame_id",origin_frame_id,ORIGIN_FRAME_ID);
    std::cout<<origin_frame_id<<std::endl;
    phone_origin_frame_id=phone_name+"/"+origin_frame_id;


    //// topic names
    topic_name_imu="/" + phone_name + "/" + TOPIC_NAME_IMU;
    topic_name_magnetic="/" + phone_name + "/" + TOPIC_NAME_MAGNETIC;
    topic_name_gps="/" + phone_name + "/" + TOPIC_NAME_GPS;
    topic_name_vehicle_perception="/" + phone_name + "/" + TOPIC_NAME_OBJECTS;

    //// TF Listener
    tf2_ros::TransformListener tf_listener(tf_buffer);

    //// sensors
    bool use_imu,use_mag,use_gps,use_vehicle_perception;
    this->private_nh.param("use_imu", use_imu,USE_IMU);
    this->private_nh.param("use_magnetic", use_mag,USE_MAGNETIC);
    this->private_nh.param("use_gps", use_gps,USE_GPS);
    this->private_nh.param("use_vehicle_perception", use_vehicle_perception,USE_VEHICLE_PERCEPTION);

    //// random seed
    mt.seed(rnd());


    /** Set phone_origin frame_id **/
    static tf2_ros::StaticTransformBroadcaster static_broadcaster;
    geometry_msgs::TransformStamped static_transformStamped;
    static_transformStamped.header.stamp = ros::Time::now();
    static_transformStamped.header.frame_id = origin_frame_id;
    static_transformStamped.child_frame_id = phone_origin_frame_id;
    static_transformStamped.transform.translation.x = 0.0;
    static_transformStamped.transform.translation.y = 0.0;
    static_transformStamped.transform.translation.z = 0.0;
    static_transformStamped.transform.rotation.x = 0.0;
    static_transformStamped.transform.rotation.y = 0.0;
    static_transformStamped.transform.rotation.z = 0.0;
    static_transformStamped.transform.rotation.w = 1.0;
    static_broadcaster.sendTransform(static_transformStamped);
    ROS_DEBUG("Spinning until killed publishing %s to %s", phone_origin_frame_id.c_str(),origin_frame_id.c_str());

    /** Set publisher **/

    //// Odom
    odom.header.frame_id = phone_origin_frame_id;
    odom.child_frame_id = phone_frame_id = phone_name;
    pub_odom = this->nh.advertise<nav_msgs::Odometry>("/" + phone_name + "/" + TOPIC_NAME_PUBLISH_ODOM, 1);

    //// Particles
    posearray.header.frame_id = phone_origin_frame_id;
    pub_particles = this->nh.advertise<geometry_msgs::PoseArray>("/" + phone_name + "/" + TOPIC_NAME_PUBLISH_POSEARRAY, 1);


    /** Initialize ParticleFilter **/
    particleInitialization();


    /** Set subscriber **/
    if (use_imu) {
        sub_imu = this->nh.subscribe(topic_name_imu, 1, &PhoneLocalizer::callback_imu, this);
    }
    if (use_mag) {
        sub_magnetic = this->nh.subscribe(topic_name_magnetic, 1, &PhoneLocalizer::callback_magnetic, this);
    }
    if (use_gps) {
        sub_gps = this->nh.subscribe(topic_name_gps, 1, &PhoneLocalizer::callback_gps, this);
    }
    if (use_vehicle_perception) {
        sub_vehicle_perception = this->nh.subscribe(topic_name_vehicle_perception, 1, &PhoneLocalizer::callback_vehicle_perception, this);
    }

}

PhoneLocalizer::~PhoneLocalizer() {};

void PhoneLocalizer::set_phone_name(std::string &phone_name) {
    this->phone_name = phone_name;
}

void PhoneLocalizer::callback_imu(const sensor_msgs::Imu::ConstPtr &msg) {

    static int n=0;
    if(n%100==0){
        ROS_DEBUG("%s x 100", __FUNCTION__);
    }
    n++;

    imu = *msg;
    time = msg->header.stamp;
    update_imu = true;
    run();

}

void PhoneLocalizer::callback_magnetic(const sensor_msgs::MagneticField::ConstPtr &msg) {

    static int n=0;
    if(n%100==0){
        ROS_DEBUG("%s x 100", __FUNCTION__);
    }
    n++;

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

void PhoneLocalizer::callback_vehicle_perception(const autoware_msgs::DetectedObjectConstPtr &msg) {

    ROS_DEBUG("%s", __FUNCTION__);

    vehicle_perception = *msg;
    time = msg->header.stamp;
    update_vehicle_perception = true;
    run();

}

void PhoneLocalizer::publish_odom(const ros::Time& time) {

    static int n=0;
    if(n%100==0) ROS_DEBUG("%s x 100", __FUNCTION__);

    odom.pose.pose.position.x = now_phone_state.x;
    odom.pose.pose.position.y = now_phone_state.y;
    odom.pose.pose.position.z = 0.0;
    tf::Quaternion quat = tf::createQuaternionFromRPY(0.0, 0.0, now_phone_state.r);
    quaternionTFToMsg(quat, odom.pose.pose.orientation);
    odom.twist.twist.linear.x = now_phone_state.vx;
    odom.twist.twist.linear.y = now_phone_state.vy;

    odom.header.stamp = time;

    pub_odom.publish(odom);

    if(n%100==0) ROS_DEBUG_STREAM("Phone State \n" << now_phone_state);
    n++;
}

void PhoneLocalizer::publish_poseaary(const ros::Time& time) {

    static int n=0;
    if(n%100==0) ROS_DEBUG("%s x 100", __FUNCTION__);

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

    if(n%100==0) ROS_DEBUG_STREAM("Particle States \n" << getParticles());

    n++;
}

void PhoneLocalizer::run() {

    static int n=-1;
    n++;
    if(n%100==0) ROS_DEBUG("%s x 100", __FUNCTION__);

    if(getParticles().empty()){
        if(n%100==0) ROS_DEBUG("ParticleFilter hasn't been initialized!");
        return;
    }

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
        particleLikelihoodGpsXY();
        particleLikelihoodGpsV();
        particleLikelihoodGpsR();
        update_gps = false;
    }

    if(update_vehicle_perception){
        vehicle_perception_state.clear();
        object2state(vehicle_perception,&vehicle_perception_state);
        if(!(vehicle_perception_state==State<>(0.0,0.0,0.0,0.0,0.0))){
            particleLikelihoodVehiclePerceptionXY();
            particleLikelihoodVehiclePerceptionV();
            particleLikelihoodVehiclePerceptionR();
        }
        update_vehicle_perception=false;
    }

    //// Neff check
    if (isParticleReample()) {
        ROS_INFO("Resampling!");
        particleResample();

        //// add system noise
        std::string str;
        private_nh.param<std::string>("particlefilter_system_noise_standard_deviation", str,PARTICLEFILTER_SYSTEM_NOISE_STANDARD_DEVIATION);
        State<> sd;
        sscanf(str.c_str(), "%lf,%lf,%lf,%lf,%lf", &sd.x, &sd.y, &sd.r, &sd.vx, &sd.vy);
        particleAddSystemNoise(sd);
    }


    /** Publish Current Status **/

    //// Odom
    now_phone_state = getWeightedMean();
    publish_odom(time);

    //// Particles
    publish_poseaary(time);

}

void PhoneLocalizer::particleInitialization() {

    ROS_DEBUG("%s", __FUNCTION__);

    getParticles().clear();

    /** Preparing phone initial parameter **/
    double initial_lat,initial_lon,initial_dir;

    std::string mode_initial_state_setting;
    nh.param<std::string>("mode_initial_state_setting",mode_initial_state_setting,MODE_INITIAL_STATE_SETTING);
    if(mode_initial_state_setting=="phone_sensors"){
        sub_imu=nh.subscribe(topic_name_imu,1,&PhoneLocalizer::callback_imu,this);
        sub_magnetic=nh.subscribe(topic_name_magnetic,1,&PhoneLocalizer::callback_magnetic, this);
        sub_gps = nh.subscribe(topic_name_gps, 1, &PhoneLocalizer::callback_gps, this);
        while(true){
            ros::spinOnce();
            if(update_imu && update_magnetic && update_gps){
                break;
            }
            if(!ros::ok()){
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        double rpy[3];
        imu2rpy(imu,rpy);
        magnetic2yaw(magnetic,rpy);
        initial_lat=gps.latitude;
        initial_lon=gps.longitude;
        initial_dir=rpy[2];
    }
    else if(mode_initial_state_setting=="rosparam"){
        double lat,lon,ele;
        std::string phone_initial_fix,phone_initial_direction;
        private_nh.param<std::string>("phone_initial_fix",phone_initial_fix,PHONE_INITIAL_FIX);
        private_nh.param<std::string>("phone_initial_direction",phone_initial_direction,PHONE_INITIAL_DERECTION);
        sscanf(phone_initial_fix.c_str(), "%lf,%lf,%lf", &lat, &lon, &ele);
        initial_lat=lat;
        initial_lon=lon;
        initial_dir=std::stod(phone_initial_direction);
    }
    else{
        ROS_ERROR("Wrong MODE_INITIAL_STATE_SETTING: %s",mode_initial_state_setting.c_str());
        exit(-1);
    }

    //// Set
    geo.llh_to_xyz(initial_lat, initial_lon, 0);
    phone_initial_state.x = geo.y();
    phone_initial_state.y = -geo.x();
    phone_initial_state.vx=0.0;
    phone_initial_state.vy=0.0;
    phone_initial_state.r = initial_dir;
    now_phone_state = phone_initial_state;


    /** ParticleFilter Initializetion **/

    //// Setting particles sampling range
    double xy_radius, r_radius, v_radius;
    private_nh.param("particlefilter_initial_xy_radius", xy_radius,PARTICLEFILTER_INITIAL_XY_RADIUS);
    private_nh.param("particlefilter_initial_r_radius", r_radius,PARTICLEFILTER_INITIAL_R_RADIUS);
    private_nh.param("particlefilter_initial_v_radius", v_radius,PARTICLEFITLER_INITIAL_V_RADIUS);
    std::uniform_real_distribution<double> rand_xy(-xy_radius, xy_radius);
    std::uniform_real_distribution<double> rand_theta(-M_PI, M_PI);
    std::uniform_real_distribution<double> rand_r(-r_radius * M_PI / 180, r_radius * M_PI / 180);
    std::uniform_real_distribution<double> rand_v(-v_radius, v_radius);

    //// Sampling particles
    for (size_t i = 0; i < particle_size; i++) {
        Particle<State<>> *p = new Particle<State<>>;
        State<> *s = new State<>;

        double r_x = rand_xy(mt);
        double r_y = rand_xy(mt);
        double r_vx = rand_v(mt);
        double r_vy = rand_v(mt);
        while (r_x * r_x + r_y * r_y > xy_radius * xy_radius) {
            r_x = rand_xy(mt);
            r_y = rand_xy(mt);
        }
        while (r_vx * r_vx + r_vy * r_vy > v_radius * v_radius) {
            r_vx = rand_v(mt);
            r_vy = rand_v(mt);
        }

        double x = r_x + phone_initial_state.x;
        double y = r_y + phone_initial_state.y;
        double r = rand_r(mt) + phone_initial_state.r;
        double vx=r_vx+phone_initial_state.vx;
//        double vy = r_vy + phone_initial_state.vy;
        double vy=0.0;

        s->set(x, y, r, vx, vy);
        p->setState(s);
        p->setWeight(1.0);
        getParticles().push_back(p);
    }

    particleNormalize();

    publish_odom(time);
    publish_poseaary(time);

}

void PhoneLocalizer::particleTransition() {

    static int n=0;
    if(n%100==0){
        ROS_DEBUG("%s x 100", __FUNCTION__);
    }
    n++;


    static bool init = true;
    static std::vector<double> now_imu_rpy(3), pre_imu_rpy(3);
    static double now_time, pre_time;

    /** Cordinate transformation from phone to world **/
    std::vector<double> imu_rpy(3);
    imu2rpy(imu,imu_rpy.data());
    rpy2matrix(imu_rpy[0],imu_rpy[1],0.0,&phone2horizontal);

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


    /** Gyro and acceleration bias **/
    double axb, ayb, gzb;
    private_nh.param("acceleration_x_bias",axb,ACCELERATION_X_BIAS);
    private_nh.param("acceleration_y_bias",ayb,ACCELERATION_Y_BIAS);
    private_nh.param("gyro_z_bias",gzb,GYRO_Z_BIAS);

    double diff_phone_yaw = State<>::normalize_radian(now_imu_rpy[2] - pre_imu_rpy[2]);


    /** Particle Transition **/

    //// Noise set
    double asd, gsd;
    private_nh.param("particlefilter_acceleration_standard_deviation",asd,PARTICLEFILTER_ACCELERATION_STANDARD_DEVIATION);
    private_nh.param("particlefilter_gyro_standard_deviation",gsd,PARTICLEFILTER_GYRO_STANDARD_DEVIATION);
    std::normal_distribution<double> rand_accel(0,asd);
    std::normal_distribution<double> rand_gyro(0, gsd);


    //// Translate
    for (auto particle:getParticles()) {

        State<> s = *particle->getState();

        //// Adding transition noise
//        double ax=phone_accel(0)+rand_accel(mt);
//        double ay=phone_accel(1)+rand_accel(mt);
//        double gz=diff_phone_yaw+rand_gyro(mt);

        //// Axis conversion
        Eigen::Matrix3f particle2origin_particle;
        rpy2matrix(0.0,0.0,s.r,&particle2origin_particle);
//        Eigen::Matrix3f phone2origin_particle=horizontal2origin_particle*phone2horizontal;
        Eigen::Vector3f ground_velocity = particle2origin_particle * Eigen::Vector3f(s.vx, s.vy, 0.0);
//        Eigen::Vector3f ground_accel=phone2ground_p*Eigen::Vector3f(ax,ay,0.0);

        //// Transition with noise
        s.x = s.x + ground_velocity(0) * d_t;
        s.y = s.y + ground_velocity(1) * d_t;
        s.vx = s.vx;
        s.vy = s.vy;
        s.r = s.r + diff_phone_yaw + rand_gyro(mt) * d_t;

        *particle->getState() = s;
    }

}

void PhoneLocalizer::particleLikelihoodIMU() {

    static int n=0;
    if(n%100==0){
        ROS_DEBUG("%s x 100", __FUNCTION__);
    }
    n++;


    double rpy[3];
    tf::Quaternion quat(imu.orientation.x, imu.orientation.y, imu.orientation.z, imu.orientation.w);
    tf::Matrix3x3(quat).getRPY(rpy[0], rpy[1], rpy[2]);

    double sd;
    private_nh.param("particlefitler_imu_yaw_standard_deviation",sd,PARTICLEFILTER_IMU_YAW_STANDARD_DEVIATION);

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

    static int n=0;
    if(n%100==0){
        ROS_DEBUG("%s x 100", __FUNCTION__);
        ROS_WARN("Non implementation! %s", __FUNCTION__);
    }
    n++;


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

void PhoneLocalizer::particleLikelihoodGpsXY() {

    ROS_DEBUG("%s", __FUNCTION__);

    geo.llh_to_xyz(gps.latitude, gps.longitude, gps.altitude);
    double gps_x=geo.y();
    double gps_y=-geo.x();

    double xy_bias, xy_sd;
    private_nh.param("particlefitler_gps_xy_bias", xy_bias,PARTICLEFILTER_GPS_XY_BIAS);
    private_nh.param("particlefitler_gps_xy_standard_deviation", xy_sd,PARTICLEFILTER_GPS_XY_STANDARD_DEVIATION);

    /** Compute Likelihood **/
    std::vector<double> weights;
    for (auto particle:getParticles()) {
        State<> s = *particle->getState();

        //// Likelihood XY
        double r_xy = std::sqrt(std::pow(s.x - gps_x, 2) + std::pow(s.y - gps_y, 2));
        double w;
        if (r_xy <= xy_bias) {
            w = 1.0;
        } else {
            w = std::exp(-std::pow(r_xy - xy_bias, 2) / (2 * xy_sd * xy_sd));
        }
        weights.push_back(w);
    }

    if(std::accumulate(weights.begin(),weights.end(),0.0)!=0.0){
        for(size_t i=0;i<weights.size();i++){
            double w=weights[i]*getParticles()[i]->getWeight();
            getParticles()[i]->setWeight(w);
        }
        particleNormalize();
    }
    else{
        ROS_DEBUG("All weights are 0: %s",__FUNCTION__);
    }

}

void PhoneLocalizer::particleLikelihoodGpsV() {

    ROS_DEBUG("%s", __FUNCTION__);

    geo.llh_to_xyz(gps.latitude, gps.longitude, gps.altitude);

    static bool init = true;
    static Eigen::Vector3d now_gps, pre_gps;
    static double now_time, pre_time;
    if (init) {
        now_gps = pre_gps = Eigen::Vector3d(geo.y(), -geo.x(), geo.z());
        now_time = pre_time = time.toSec();
        init = false;
        return;
    }
    pre_gps = now_gps;
    pre_time = now_time;
    now_gps = Eigen::Vector3d(geo.y(), -geo.x(), geo.z());
    now_time = time.toSec();

    double dx = now_gps(0) - pre_gps(0);
    double dy = now_gps(1) - pre_gps(1);
    double d_t = now_time - pre_time;
    double gps_v = std::sqrt(dx * dx + dy * dy) / d_t;

    double v_bias, v_sd;
    private_nh.param("particlefitler_gps_v_bias", v_bias, PARTICLEFILTER_GPS_V_BIAS);
    private_nh.param("particlefitler_gps_v_standard_deviation", v_sd, PARTICLEFILTER_GPS_V_STANDARD_DEVIATION);

    /** Compute Likelihood **/
    std::vector<double> weights;
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

        weights.push_back(w);
    }

    if(std::accumulate(weights.begin(),weights.end(),0.0)!=0.0){
        for(size_t i=0;i<weights.size();i++){
            double w=weights[i]*getParticles()[i]->getWeight();
            getParticles()[i]->setWeight(w);
        }
        particleNormalize();
    }
    else{
        ROS_DEBUG("All weights are 0: %s",__FUNCTION__);
    }

}

void PhoneLocalizer::particleLikelihoodGpsR() {

    ROS_DEBUG("%s", __FUNCTION__);

    geo.llh_to_xyz(gps.latitude, gps.longitude, gps.altitude);

    static bool init = true;
    static Eigen::Vector3d now_gps, pre_gps;
    static double now_time, pre_time;
    if (init) {
        now_gps = pre_gps = Eigen::Vector3d(geo.y(), -geo.x(), geo.z());
        now_time = pre_time = time.toSec();
        init = false;
        return;
    }
    pre_gps = now_gps;
    pre_time = now_time;
    now_gps = Eigen::Vector3d(geo.y(), -geo.x(), geo.z());
    now_time = time.toSec();

    double dx = now_gps(0) - pre_gps(0);
    double dy = now_gps(1) - pre_gps(1);
    if (std::sqrt(dx * dx + dy * dy) < 1.0) {
        return;
    }

    double gps_r = std::atan2(dy, dx);
    gps_r = State<>::normalize_radian(gps_r);
    std::cout<<gps_r<<std::endl;
//    ROS_DEBUG_STREAM("Mean R: " << now_phone_state.r << ", GPS R: " << gps_r);

    double r_sd;
    private_nh.param("particlefitler_gps_r_standard_deviation", r_sd, PARTICLEFILTER_GPS_R_STANDARD_DEVIATION);


    /** Compute Likelihood **/
    std::vector<double> weights;
    for (auto particle:getParticles()) {
        State<> s = *particle->getState();

        //// Likelihood V
        double w;
        double r_r = std::abs(State<>::normalize_radian(s.r - gps_r));
        w = std::exp(-std::pow(r_r, 2) / (2 * r_sd * r_sd));

        weights.push_back(w);

    }

    if(std::accumulate(weights.begin(),weights.end(),0.0)!=0.0){
        for(size_t i=0;i<weights.size();i++){
            double w=weights[i]*getParticles()[i]->getWeight();
            getParticles()[i]->setWeight(w);
        }
        particleNormalize();
    }
    else{
        ROS_DEBUG("All weights are 0: %s",__FUNCTION__);
    }


}

void PhoneLocalizer::particleLikelihoodVehiclePerceptionXY() {

    ROS_DEBUG("%s", __FUNCTION__);

    static double xy_sd;
    private_nh.param("particlefilter_vehicle_perception_xy_statndard_deviation",xy_sd,PARTICLEFILTER_VEHICLE_PERCEPTION_XY_STANDARD_DEVIATION);

    std::vector<double> weights;
    for(auto& particle:getParticles()){
        State<> par_s=*particle->getState();

        //// Likelihood XY
        double d_x=vehicle_perception_state.x-par_s.x;
        double d_y=vehicle_perception_state.y-par_s.y;

        double d_xy = std::sqrt(d_x*d_x + d_y*d_y);
        double w = std::exp(-d_xy*d_xy / (2 * xy_sd * xy_sd));

        weights.push_back(w);
    }

    if(std::accumulate(weights.begin(),weights.end(),0.0)!=0.0){
        for(size_t i=0;i<weights.size();i++){
            double w=weights[i]*getParticles()[i]->getWeight();
            getParticles()[i]->setWeight(w);
        }
        particleNormalize();
//    std::cout << "Particle States \n" << pl->getParticles() << std::endl;
    }
    else{
        ROS_DEBUG("Skip likelihood: %s",__FUNCTION__);
    }

}

void PhoneLocalizer::particleLikelihoodVehiclePerceptionV() {

    ROS_DEBUG("%s", __FUNCTION__);

    double v_sd;
    private_nh.param("particlefilter_vehicle_perception_v_statndard_deviation",v_sd,PARTICLEFILTER_VEHICLE_PERCEPTION_V_STANDARD_DEVIATION);

    std::vector<double> weights;
    for(auto& particle:getParticles()){
        State<> par_s=*particle->getState();

        //// Likelihood XY
        double d_vx=vehicle_perception_state.x-par_s.vx;
        double d_vy=vehicle_perception_state.y-par_s.vy;

        double d_v = std::sqrt(d_vx*d_vx + d_vy*d_vy);
        double w = std::exp(-d_v*d_v / (2 * v_sd * v_sd));

        weights.push_back(w);
    }

    if(std::accumulate(weights.begin(),weights.end(),0.0)!=0.0){
        for(size_t i=0;i<weights.size();i++){
            double w=weights[i]*getParticles()[i]->getWeight();
            getParticles()[i]->setWeight(w);
        }
        particleNormalize();
//    std::cout << "Particle States \n" << pl->getParticles() << std::endl;
    }
    else{
        ROS_DEBUG("Skip likelihood: %s",__FUNCTION__);
    }

}

void PhoneLocalizer::particleLikelihoodVehiclePerceptionR() {

    ROS_DEBUG("%s", __FUNCTION__);

    double v_sd;
    private_nh.param("particlefilter_vehicle_perception_r_statndard_deviation",v_sd,PARTICLEFILTER_VEHICLE_PERCEPTION_R_STANDARD_DEVIATION);

    std::vector<double> weights;
    for(auto& particle:getParticles()){
        State<> par_s=*particle->getState();

        //// Likelihood XY
        double d_r=vehicle_perception_state.r-par_s.r;
        if(d_r>M_PI){
            d_r=M_PI*2.0-d_r;
        }
        else if(d_r<-M_PI){
            d_r=-M_PI*2.0-d_r;
        }

        double w = std::exp(-d_r*d_r / (2 * v_sd * v_sd));

        weights.push_back(w);
    }

    if(std::accumulate(weights.begin(),weights.end(),0.0)!=0.0){
        for(size_t i=0;i<weights.size();i++){
            double w=weights[i]*getParticles()[i]->getWeight();
            getParticles()[i]->setWeight(w);
        }
        particleNormalize();
//    std::cout << "Particle States \n" << pl->getParticles() << std::endl;
    }
    else{
        ROS_DEBUG("Skip likelihood: %s",__FUNCTION__);
    }

}

void PhoneLocalizer::imu2rpy(const sensor_msgs::Imu &imu, double *rpy) {

    tf::Quaternion quat(imu.orientation.x, imu.orientation.y, imu.orientation.z, imu.orientation.w);
    tf::Matrix3x3(quat).getRPY(*rpy, *(rpy+1), *(rpy+2));

}

void PhoneLocalizer::magnetic2yaw(const sensor_msgs::MagneticField& magnetic, double* rpy) {

    Eigen::Vector3f mag(magnetic.magnetic_field.x, magnetic.magnetic_field.y, magnetic.magnetic_field.z);
    Eigen::Matrix3f rp_mat;
    rpy2matrix(-rpy[0],-rpy[1],0.0,&rp_mat);
    Eigen::Vector3f mag_horizontal=rp_mat*mag;
    *(rpy+2)=-std::atan2(-mag_horizontal(0),mag_horizontal(1));

}

void PhoneLocalizer::rpy2matrix(double roll, double pitch, double yaw, Eigen::Matrix3f *rotation_matrix) {

    Eigen::Vector3f axis_dr(1, 0, 0), axis_dp(0, 1, 0), axis_dy(0, 0, 1);
    Eigen::Matrix3f matrix_dr, matrix_dp, matrix_dy;
    matrix_dr = Eigen::AngleAxisf(roll, axis_dr);
    matrix_dp = Eigen::AngleAxisf(pitch, axis_dp);
    matrix_dy = Eigen::AngleAxisf(yaw, axis_dy);
    *rotation_matrix=matrix_dy*matrix_dp*matrix_dr;

}

void PhoneLocalizer::object2state(const autoware_msgs::DetectedObject& object, State<>* state){

    if (std::isnan(object.pose.orientation.x) ||
        std::isnan(object.pose.orientation.y) ||
        std::isnan(object.pose.orientation.z) ||
        std::isnan(object.pose.orientation.w) ||
        object.pose.orientation.x == object.pose.orientation.y == object.pose.orientation.z ==
        object.pose.orientation.w == 0) {
        return;
    }

    geometry_msgs::PoseStamped in, out;
    in.header = object.header;
    in.pose = object.pose;
    out.header.frame_id = origin_frame_id;
    out.header.stamp = in.header.stamp;
    geometry_msgs::TransformStamped transform = tf_buffer.lookupTransform(origin_frame_id, in.header.frame_id,
                                                                          in.header.stamp, ros::Duration(1.0));
    tf2::doTransform(in, out, transform);

    state->x = out.pose.position.x;
    state->y = out.pose.position.y;
    state->vx = object.velocity.linear.x;
    state->vy = object.velocity.linear.y;
    tf::Quaternion quat(out.pose.orientation.x,
                        out.pose.orientation.y,
                        out.pose.orientation.z,
                        out.pose.orientation.w);
    double rpy[3];
    tf::Matrix3x3(quat).getRPY(rpy[0], rpy[1], rpy[2]);
    state->r = State<>::normalize_radian(rpy[2]);


    ROS_DEBUG_STREAM("Pedestrian origin states: \n" << *state);

}