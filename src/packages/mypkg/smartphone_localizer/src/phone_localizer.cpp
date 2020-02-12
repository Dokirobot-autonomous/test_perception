//
// Created by ohashi on 19/09/13.
//

#include <phone_localizer.h>

PhoneLocalizer::PhoneLocalizer(ros::NodeHandle *nh, ros::NodeHandle *private_nh) :
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
    ofs_state << "%time,x,y,r,vx,vy" << std::endl;
    ofs_particle.open(path + "/particle.csv");
    ofs_particle << "%time,x,y,r,vx,vy,w" << std::endl;
    ofs_imu.open(path + "/imu.csv");
    ofs_imu << "%time,yaw_rate" << std::endl;
    ofs_gps.open(path + "/gps.csv");
    ofs_gps << "%time,x,y" << std::endl;
    ofs_gnssraw_accel.open(path + "/gnssraw_accel.csv");
    ofs_gnssraw_accel << "%time,vx,vy" << std::endl;
    ofs_weight.open(path + "/weight.csv");
    ofs_weight << "%time,function,w" << std::endl;
    ofs_resampling.open(path + "/resampling.csv");
    ofs_resampling << "%time,update_gps,update_gnssraw_fix,update_gnssraw_acceleration,update_vehicle_perception"
                   << std::endl;


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

    //// random seed
    mt.seed(rnd());


    /** Set publisher **/

    //// Odom
    odom.header.frame_id = origin_frame_id;
    odom.child_frame_id = phone_frame_id = phone_name;
    pub_odom = this->nh.advertise<nav_msgs::Odometry>(topic_name_pub_odom, 10);

    //// Particles
    posearray.header.frame_id = origin_frame_id;
    pub_particles = this->nh.advertise<geometry_msgs::PoseArray>(topic_name_pub_particles, 10);


    /** Initialize ParticleFilter **/
    particleInitialization();


    /** Set subscriber **/
    if (use_imu) {
        sub_imu = this->nh.subscribe(topic_name_sub_imu, 10, &PhoneLocalizer::callback_imu, this);
        ROS_DEBUG("Subscribe IMU Message");
    }
    if (use_mag) {
        sub_magnetic = this->nh.subscribe(topic_name_sub_magnetic, 10, &PhoneLocalizer::callback_magnetic, this);
        ROS_DEBUG("Subscribe Magnetic Message");
    }
    if (use_gps && (use_gps_xy || use_gps_v || use_gps_r)) {
        sub_gps = this->nh.subscribe(topic_name_sub_gps, 10, &PhoneLocalizer::callback_gps, this);
        ROS_DEBUG("Subscribe GPS Message");
    }
    if (use_gnssraw_fix) {
        sub_gnssraw_fix = this->nh.subscribe(topic_name_sub_gnssraw_fix, 10, &PhoneLocalizer::callback_gnssraw_fix,
                                             this);
        ROS_DEBUG("Subscribe GNSSRaw Fix Message");
    }
    if (use_gnssraw_acceleration && (use_gnssraw_acceleration_v || use_gnssraw_acceleration_r)) {
        sub_gnssraw_acceleration = this->nh.subscribe(topic_name_sub_gnssraw_acceleration, 10,
                                                      &PhoneLocalizer::callback_gnssraw_acceleration, this);
        ROS_DEBUG("Subscribe GNSSRaw Velocity Message");
    }
    if (use_vehicle_perception && (use_vehicle_perception_xy || use_vehicle_perception_vr)) {
        sub_vehicle_perception = this->nh.subscribe(topic_name_sub_vehicle_perception, 10,
                                                    &PhoneLocalizer::callback_vehicle_perception, this);
        ROS_DEBUG("Subscribe Vehicle Perception Message");
    }

}

PhoneLocalizer::~PhoneLocalizer() {};

void PhoneLocalizer::callback_imu(const sensor_msgs::Imu::ConstPtr &msg) {

    imu = *msg;
/*
    time = msg->header.stamp;
*/
    time = ros::Time::now();
    ROS_DEBUG_STREAM("callback_imu: \n" << imu);
    update_imu = true;
    run();

}

void PhoneLocalizer::callback_magnetic(const sensor_msgs::MagneticField::ConstPtr &msg) {

    magnetic = *msg;
/*
    time = msg->header.stamp;
*/
    time = ros::Time::now();
    ROS_DEBUG_STREAM("callback_magnetic: \n" << magnetic);
    update_magnetic = true;
    run();

}

void PhoneLocalizer::callback_gps(const sensor_msgs::NavSatFix::ConstPtr &msg) {

    gps = *msg;
/*
    time = msg->header.stamp;
*/
    time = ros::Time::now();
    ROS_DEBUG_STREAM("callback_gps: \n" << gps);
    update_gps = true;
    run();

}

void PhoneLocalizer::callback_gnssraw_fix(const sensor_msgs::NavSatFix::ConstPtr &msg) {

    gnssraw_fix = *msg;
/*
    time = msg->header.stamp;
*/
    time = ros::Time::now();
    ROS_DEBUG_STREAM("callback_gnssraw_fix: \n" << gnssraw_fix);
    update_gnssraw_fix = true;
    run();

}

void PhoneLocalizer::callback_gnssraw_acceleration(const geometry_msgs::AccelWithCovarianceStamped::ConstPtr &msg) {

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
    ROS_DEBUG_STREAM("callback_gnssraw_acceleration: \n" << gnssraw_acceleration);
    update_gnssraw_acceleration = true;
    run();

}

void PhoneLocalizer::callback_vehicle_perception(const geometry_msgs::PoseWithCovarianceStamped::ConstPtr &msg) {

    //// TODO: PoseWithCovarinceに変更
    vehicle_perception = *msg;
/*
    time = msg->header.stamp;
*/
    time = ros::Time::now();
    ROS_DEBUG_STREAM("callback_vehicle_perception: \n" << vehicle_perception);
    update_vehicle_perception = true;
    run();

}

void PhoneLocalizer::publish_odom(const ros::Time &time) {

    odom.pose.pose.position.x = now_phone_state.x;
    odom.pose.pose.position.y = now_phone_state.y;
    odom.pose.pose.position.z = 0.0;
    tf::Quaternion quat = tf::createQuaternionFromRPY(0.0, 0.0, now_phone_state.r);
    quaternionTFToMsg(quat, odom.pose.pose.orientation);
    odom.twist.twist.linear.x = now_phone_state.vx;
    odom.twist.twist.linear.y = now_phone_state.vy;

    odom.header.stamp = time;

    ofs_state << time.sec << "." << time.nsec << "," << now_phone_state.x << "," << now_phone_state.y << ","
              << now_phone_state.r << "," << now_phone_state.vx << "," << now_phone_state.vy << std::endl;
    pub_odom.publish(odom);

    ROS_DEBUG_STREAM("Published Phone State \n" << now_phone_state);

}

void PhoneLocalizer::publish_poseaary(const ros::Time &time) {

    posearray.poses.clear();
    static size_t seq = 0;

    for (auto particle:getParticles()) {
        State<> s = *particle->getState();
        geometry_msgs::Pose pose;
        pose.position.x = s.x;
        pose.position.y = s.y;
        pose.position.z = 0.0;
        tf::Quaternion q = tf::createQuaternionFromRPY(0.0, 0.0, s.r);
        quaternionTFToMsg(q, pose.orientation);
        posearray.poses.push_back(pose);
        if (seq % 10 == 0) {
            ofs_particle << time.sec << "." << time.nsec << "," << s.x << "," << s.y << "," << s.r << ","
                         << s.vx << "," << s.vy << "," << particle->getWeight() << std::endl;
        }
    }
    posearray.header.stamp = time;

    pub_particles.publish(posearray);

    ROS_DEBUG_STREAM("Published Particle States \n" << getParticles());

    seq++;

}

void PhoneLocalizer::run() {

    std::lock_guard<std::mutex> guard(mtx);

    if (getParticles().empty()) {
        bool init = particleInitialization();
        if (!init) {
            ROS_DEBUG("ParticleFilter hasn't been initialized yet.");
            return;
        }
    }

    for (auto p:getParticles()) {
        p->getState()->normalize_r();
    }

    if (update_imu) {
        particleTransition();
//        particleLikelihoodIMU();
    }

    if (update_magnetic) {
//        particleLikelihoodMagnetic();
    }

    if (update_gps) {
        if (use_gps_xy) {
            particleLikelihoodGpsXY();
        }
        if (use_gps_v) {
            particleLikelihoodGpsV();
        }
        if (use_gps_r) {
            particleLikelihoodGpsR();
        }
    }

    if (update_gnssraw_fix) {
        particleLikelihoodGNSSRawXY();
    }

    if (update_gnssraw_acceleration) {
        if (use_gnssraw_acceleration_v) {
            particleLikelihoodGNSSRawV();
        }
        if (use_gnssraw_acceleration_r) {
            particleLikelihoodGNSSRawR();
        }
    }

    if (update_vehicle_perception) {
        particleLikelihoodVehiclePerceptionXY();

    }

    //// Neff check
    if (isParticleReample()) {
        ROS_INFO("Resampling!\nimu:%d,magnetic:%d,gps:%d,gnssraw_fix:%d,gnssraw_acceleration:%d,vehicle_perception:%d",
                 update_imu, update_magnetic, update_gps, update_gnssraw_fix, update_gnssraw_acceleration,
                 update_vehicle_perception);
        ofs_resampling << time.sec << "." << time.nsec << "," << update_gps << "," << update_gnssraw_fix << ","
                       << update_gnssraw_acceleration << "," << update_vehicle_perception << std::endl;
        for (const auto &particle:getParticles()) {
            State<> s = *particle->getState();
            ofs_particle << time.sec << "." << time.nsec << "," << s.x << "," << s.y << "," << s.r << ","
                         << s.vx << "," << s.vy << "," << particle->getWeight() << std::endl;
        }
        particleResample();

        //// add system noise
        std::vector<double> tmp;
        private_nh.param<std::vector<double>>("particlefilter_system_noise_standard_deviation", tmp,
                                              PARTICLEFILTER_SYSTEM_NOISE_STANDARD_DEVIATION);
        State<> sd(tmp[0], tmp[1], tmp[2], tmp[3], tmp[4]);
        particleAddSystemNoise(sd);

        for (const auto &particle:getParticles()) {
            State<> s = *particle->getState();
            ofs_particle << time.sec << "." << time.nsec << "," << s.x << "," << s.y << "," << s.r << ","
                         << s.vx << "," << s.vy << "," << particle->getWeight() << std::endl;
        }
    }


    /** Publish Current Status **/

    //// Odom
    now_phone_state = getWeightedMean();
    double x = 0.0, y = 0.0;
    for (const auto &p:getParticles()) {
        x += std::cos(p->getState()->r);
        y += std::sin(p->getState()->r);
    }
    x /= getParticles().size();
    y /= getParticles().size();
    now_phone_state.r = std::atan2(y, x);
    publish_odom(time);

    //// Particles
    publish_poseaary(time);

    update_imu = false;
    update_magnetic = false;
    update_gps = false;
    update_gnssraw_fix = false;
    update_gnssraw_acceleration = false;
    update_vehicle_perception = update_vehicle_perception_xy = update_vehicle_perception_vr = false;

    ROS_DEBUG("%s Completed", __FUNCTION__);

}

bool PhoneLocalizer::particleInitialization() {

    ROS_DEBUG("%s", __FUNCTION__);

    getParticles().clear();

    /** Preparing phone initial parameter **/
//    double initial_lat, initial_lon, initial_vx, initial_vy, initial_dir;

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

    now_phone_state = phone_initial_state;


    /** ParticleFilter Initializetion **/

    //// Particle size
    private_nh.getParam("particlefilter_particle_size", particle_size);

    //// Setting particles sampling range
    double xy_radius, r_radius, v_radius;
    private_nh.getParam("particlefilter_initial_xy_radius", xy_radius);
    private_nh.getParam("particlefilter_initial_r_radius", r_radius);
    private_nh.getParam("particlefilter_initial_v_radius", v_radius);
    std::uniform_real_distribution<double> rand_xy(-xy_radius, xy_radius);
//    std::uniform_real_distribution<double> rand_theta(-M_PI, M_PI);
    std::uniform_real_distribution<double> rand_r(-r_radius, r_radius);
    std::uniform_real_distribution<double> rand_v(-v_radius, v_radius);

    //// Sampling particles
//    #pragma omp parallel for
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
        double vx = r_vx + phone_initial_state.vx;
//        double vy = r_vy + phone_initial_state.vy;
        double vy = 0.0;

        s->set(x, y, r, vx, vy);
        p->setState(s);
        p->setWeight(1.0);
        getParticles().push_back(p);
    }

    particleNormalize();

    publish_odom(time);
    publish_poseaary(time);

    return true;
}

void PhoneLocalizer::particleTransition() {

    ROS_DEBUG("%s", __FUNCTION__);

    static bool init = false;
    static std::vector<double> now_imu_rpy(3), pre_imu_rpy(3);
    static double now_time, pre_time;

    /** Cordinate transformation from phone to world **/
    std::vector<double> imu_rpy(3);
    imu2rpy(imu, imu_rpy.data());
    rpy2matrix(imu_rpy[0], imu_rpy[1], 0.0, &phone2horizontal);

    //// If initial, set previous time and skip calculation
    if (!init) {
        now_imu_rpy = pre_imu_rpy = imu_rpy;
        now_time = pre_time = imu.header.stamp.toSec();
        init = true;
        return;
    }

    pre_imu_rpy = now_imu_rpy;
    pre_time = now_time;
    now_imu_rpy = imu_rpy;
    now_time = imu.header.stamp.toSec();
    double d_t = now_time - pre_time;

    /** Gyro and acceleration bias **/
    double axb, ayb, gzb;
    private_nh.param("acceleration_x_bias", axb, ACCELERATION_X_BIAS);
    private_nh.param("acceleration_y_bias", ayb, ACCELERATION_Y_BIAS);
    private_nh.param("gyro_z_bias", gzb, GYRO_Z_BIAS);

    double diff_phone_yaw = State<>::normalize_radian(now_imu_rpy[2] - pre_imu_rpy[2]);

    ofs_imu << time.sec << "." << time.nsec << "," << diff_phone_yaw << std::endl;

    /** Particle Transition **/

    //// Noise set
    double asd, gsd, vsd;
    private_nh.param("particlefilter_acceleration_standard_deviation", asd,
                     PARTICLEFILTER_ACCELERATION_STANDARD_DEVIATION);
/*
    private_nh.param("particlefilter_gyro_standard_deviation", gsd, PARTICLEFILTER_GYRO_STANDARD_DEVIATION);
*/
    private_nh.param("particlefilter_transition_v_standard_deviation", vsd,
                     PARTICLEFILTER_TRANSITION_V_STANDARD_DEVIATION);
    gsd = diff_phone_yaw / 1.0;
    std::normal_distribution<double> rand_accel(0.0, asd);
    std::normal_distribution<double> rand_gyro(0.0, gsd);
    std::normal_distribution<double> rand_v(0.0, vsd);


    //// Translate
    // #pragma omp parallel for
    for (size_t i = 0; i < getParticles().size(); i++) {

        auto &particle = getParticles()[i];

        State<> s = *particle->getState();

        //// Adding transition noise
//        double ax=phone_accel(0)+rand_accel(mt);
//        double ay=phone_accel(1)+rand_accel(mt);
//        double gz=diff_phone_yaw+rand_gyro(mt);

        //// Axis conversion
        Eigen::Matrix3f particle2origin_particle;
        rpy2matrix(0.0, 0.0, s.r, &particle2origin_particle);
        Eigen::Vector3f ground_velocity = particle2origin_particle * Eigen::Vector3f(s.vx, 0.0, 0.0);


        //// Transition with noise
        s.x = s.x + ground_velocity(0) * d_t;
        s.y = s.y + ground_velocity(1) * d_t;
//        s.vx = s.vx;
        s.vx = s.vx + rand_v(mt) * d_t;
        if (s.vx < 0) {
            s.vx = 0;
        }
        s.vy = s.vy;
        s.r = s.r + diff_phone_yaw + rand_gyro(mt);

        *particle->getState() = s;
    }

}

void PhoneLocalizer::particleLikelihoodIMU() {

    static int n = 0;
    if (n % 100 == 0) {
        ROS_DEBUG("%s x 100", __FUNCTION__);
    }
    n++;


    double rpy[3];
    tf::Quaternion quat(imu.orientation.x, imu.orientation.y, imu.orientation.z, imu.orientation.w);
    tf::Matrix3x3(quat).getRPY(rpy[0], rpy[1], rpy[2]);

    double sd;
    private_nh.param("particlefitler_imu_yaw_standard_deviation", sd, PARTICLEFILTER_IMU_YAW_STANDARD_DEVIATION);

    // #pragma omp parallel for
    for (size_t i = 0; i < getParticles().size(); i++) {
        auto &particle = getParticles()[i];
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

    static int n = 0;
    if (n % 100 == 0) {
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

    geo.llh_to_xyz_us(gps.latitude, gps.longitude, gps.altitude);
    double gps_x = geo.y();
    double gps_y = -geo.x();

    ofs_gps << time.sec << "." << time.nsec << "," << gps_x << "," << gps_y << std::endl;

    double xy_bias, xy_sd;
    private_nh.param("particlefitler_gps_xy_bias", xy_bias, PARTICLEFILTER_GPS_XY_BIAS);
    private_nh.param("particlefitler_gps_xy_standard_deviation", xy_sd, PARTICLEFILTER_GPS_XY_STANDARD_DEVIATION);

    /** Compute Likelihood **/
    std::vector<double> weights;
// #pragma omp parallel for
    for (size_t i = 0; i < getParticles().size(); i++) {
        auto &particle = getParticles()[i];
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

    if (std::accumulate(weights.begin(), weights.end(), 0.0) != 0.0) {
        for (size_t i = 0; i < weights.size(); i++) {
            double w = weights[i] * getParticles()[i]->getWeight();
            getParticles()[i]->setWeight(w);
            ofs_weight << time.sec << "." << time.nsec << "," << std::string(__FUNCTION__) << "," << weights[i]
                       << std::endl;
        }
        particleNormalize();
    } else {
        ROS_DEBUG("All weights are 0: %s", __FUNCTION__);
    }

}

void PhoneLocalizer::particleLikelihoodGpsV() {

    ROS_DEBUG("%s", __FUNCTION__);

    geo.llh_to_xyz_us(gps.latitude, gps.longitude, gps.altitude);

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
// #pragma omp parallel for
    for (size_t i = 0; i < getParticles().size(); i++) {
        auto &particle = getParticles()[i];
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

    if (std::accumulate(weights.begin(), weights.end(), 0.0) != 0.0) {
        for (size_t i = 0; i < weights.size(); i++) {
            double w = weights[i] * getParticles()[i]->getWeight();
            getParticles()[i]->setWeight(w);
            ofs_weight << time.sec << "." << time.nsec << "," << std::string(__FUNCTION__) << "," << weights[i]
                       << std::endl;
        }
        particleNormalize();
    } else {
        ROS_DEBUG("All weights are 0: %s", __FUNCTION__);
    }

}

void PhoneLocalizer::particleLikelihoodGpsR() {

    ROS_DEBUG("%s", __FUNCTION__);

    geo.llh_to_xyz_us(gps.latitude, gps.longitude, gps.altitude);

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
    std::cout << gps_r << std::endl;
//    ROS_DEBUG_STREAM("Mean R: " << now_phone_state.r << ", GPS R: " << gps_r);

    double r_sd;
    private_nh.param("particlefitler_gps_r_standard_deviation", r_sd, PARTICLEFILTER_GPS_R_STANDARD_DEVIATION);


    /** Compute Likelihood **/
    std::vector<double> weights;
// #pragma omp parallel for
    for (size_t i = 0; i < getParticles().size(); i++) {
        auto &particle = getParticles()[i];
        State<> s = *particle->getState();

        //// Likelihood V
        double w;
        double r_r = std::abs(State<>::normalize_radian(s.r - gps_r));
        w = std::exp(-std::pow(r_r, 2) / (2 * r_sd * r_sd));

        weights.push_back(w);

    }

    if (std::accumulate(weights.begin(), weights.end(), 0.0) != 0.0) {
        for (size_t i = 0; i < weights.size(); i++) {
            double w = weights[i] * getParticles()[i]->getWeight();
            getParticles()[i]->setWeight(w);
            ofs_weight << time.sec << "." << time.nsec << "," << std::string(__FUNCTION__) << "," << weights[i]
                       << std::endl;
        }
        particleNormalize();
    } else {
        ROS_DEBUG("All weights are 0: %s", __FUNCTION__);
    }


}

/**
 * particleLikelihoodGNSSRawXY
 */
void PhoneLocalizer::particleLikelihoodGNSSRawXY() {

    ROS_DEBUG("%s", __FUNCTION__);

    geo.llh_to_xyz_us(gnssraw_fix.latitude, gnssraw_fix.longitude, gnssraw_fix.altitude);
    double gnssraw_x = geo.y();
    double gnssraw_y = -geo.x();

    double xy_bias, xy_sd;
    private_nh.param("particlefitler_gnssraw_xy_bias", xy_bias, PARTICLEFILTER_GNSSRAW_XY_BIAS);
    private_nh.param("particlefitler_gnssraw_xy_standard_deviation", xy_sd,
                     PARTICLEFILTER_GNSSRAW_XY_STANDARD_DEVIATION);

    /** Compute Likelihood **/
    std::vector<double> weights;
// #pragma omp parallel for
    for (size_t i = 0; i < getParticles().size(); i++) {
        auto &particle = getParticles()[i];
        State<> s = *particle->getState();

        //// Likelihood XY
        double r_xy = std::sqrt(std::pow(s.x - gnssraw_x, 2) + std::pow(s.y - gnssraw_y, 2));
        double w;
        if (r_xy <= xy_bias) {
            w = 1.0;
        } else {
            w = std::exp(-std::pow(r_xy - xy_bias, 2) / (2 * xy_sd * xy_sd));
        }
        weights.push_back(w);
    }

    if (std::accumulate(weights.begin(), weights.end(), 0.0) != 0.0) {
        for (size_t i = 0; i < weights.size(); i++) {
            double w = weights[i] * getParticles()[i]->getWeight();
            getParticles()[i]->setWeight(w);
            ofs_weight << time.sec << "." << time.nsec << "," << std::string(__FUNCTION__) << "," << weights[i]
                       << std::endl;
        }
        particleNormalize();
    } else {
        ROS_DEBUG("All weights are 0: %s", __FUNCTION__);
    }

}

/**
 * particleLikelihoodGNSSRawV
 */
void PhoneLocalizer::particleLikelihoodGNSSRawV() {

    ROS_DEBUG("%s", __FUNCTION__);

    double gnssraw_vx = gnssraw_acceleration.accel.accel.linear.y;
    double gnssraw_vy = -gnssraw_acceleration.accel.accel.linear.x;
    double gnssraw_v = std::sqrt(gnssraw_vx * gnssraw_vx + gnssraw_vy * gnssraw_vy);
    if (gnssraw_v < 0.2) {
        gnssraw_v = 0.0;
    }

    ofs_gnssraw_accel << time.sec << "." << time.nsec << "," << gnssraw_vx << "," << gnssraw_vy << std::endl;

    double v_sd;
    private_nh.param("particlefitler_gnssraw_v_standard_deviation", v_sd,
                     PARTICLEFILTER_GNSSRAW_V_STANDARD_DEVIATION);

    /** Compute Likelihood **/
    std::vector<double> weights;
// #pragma omp parallel for
    for (size_t i = 0; i < getParticles().size(); i++) {
        auto &particle = getParticles()[i];
        State<> s = *particle->getState();

        //// Likelihood V
        double w;
        double s_v = std::sqrt(s.vx * s.vx + s.vy * s.vy);
        double r_v = std::abs(s_v - gnssraw_v);
        w = std::exp(-std::pow(r_v, 2) / (2 * v_sd * v_sd));

        weights.push_back(w);
    }

    if (std::accumulate(weights.begin(), weights.end(), 0.0) != 0.0) {
        for (size_t i = 0; i < weights.size(); i++) {
            double w = weights[i] * getParticles()[i]->getWeight();
            getParticles()[i]->setWeight(w);
            ofs_weight << time.sec << "." << time.nsec << "," << std::string(__FUNCTION__) << "," << weights[i]
                       << std::endl;
        }
        particleNormalize();
    } else {
        ROS_DEBUG("All weights are 0: %s", __FUNCTION__);
    }

}

/**
 * particleLikelihoodGNSSRawR
 */
void PhoneLocalizer::particleLikelihoodGNSSRawR() {

    ROS_DEBUG("%s", __FUNCTION__);

    double gnssraw_vx = gnssraw_acceleration.accel.accel.linear.y;
    double gnssraw_vy = -gnssraw_acceleration.accel.accel.linear.x;

    Eigen::Matrix3f phone2ground;
    quaternion2matrix(imu.orientation.x, imu.orientation.y, imu.orientation.z, imu.orientation.w, &phone2ground);
/*
    Eigen::Vector3f rpy_r =
            phone2ground * Eigen::Vector3f(imu.angular_velocity.x, imu.angular_velocity.y, imu.angular_velocity.z);
*/
    Eigen::Vector3f rpy_r = Eigen::Vector3f::Zero();

    ROS_DEBUG_STREAM("rpy_r(2): " << rpy_r(2));

    if (std::sqrt(gnssraw_vx * gnssraw_vx + gnssraw_vy * gnssraw_vy) < 0.3 ||
        std::abs(rpy_r(2)) > 0.6) {
        return;
    }
    double gnssraw_r = std::atan2(gnssraw_vy, gnssraw_vx);


    double r_sd;
    private_nh.param("particlefitler_gnssraw_r_standard_deviation", r_sd,
                     PARTICLEFILTER_GNSSRAW_R_STANDARD_DEVIATION);

    /** Compute Likelihood **/
    std::vector<double> weights;
// #pragma omp parallel for
    for (size_t i = 0; i < getParticles().size(); i++) {
        auto &particle = getParticles()[i];
        State<> s = *particle->getState();

        //// Likelihood V
        double w;
        double r_r = std::abs(State<>::normalize_radian(s.r - gnssraw_r));
        w = std::exp(-std::pow(r_r, 2) / (2 * r_sd * r_sd));

        weights.push_back(w);

    }

    if (std::accumulate(weights.begin(), weights.end(), 0.0) != 0.0) {
        for (size_t i = 0; i < weights.size(); i++) {
            double w = weights[i] * getParticles()[i]->getWeight();
            getParticles()[i]->setWeight(w);
            ofs_weight << time.sec << "." << time.nsec << "," << std::string(__FUNCTION__) << "," << weights[i]
                       << std::endl;
        }
        particleNormalize();
    } else {
        ROS_DEBUG("All weights are 0: %s", __FUNCTION__);
    }

}

/**
 * particleLikelihoodVehiclePerceptionXY
 */

/*
void PhoneLocalizer::particleLikelihoodVehiclePerceptionXY() {

//    ROS_DEBUG("%s", __FUNCTION__);

    geometry_msgs::Pose pose = vehicle_perception.pose.pose;
    double dist = std::sqrt(std::pow(pose.position.x - now_phone_state.x, 2) +
                            std::pow(pose.position.y - now_phone_state.y, 2));
    if (dist > 10) {
        return;
    }

    Eigen::MatrixXd covariance(2, 2);
    covariance << vehicle_perception.pose.covariance[0], vehicle_perception.pose.covariance[1],
            vehicle_perception.pose.covariance[6], vehicle_perception.pose.covariance[7];

    static double xy_sd;
    private_nh.param("particlefilter_vehicle_perception_xy_statndard_deviation", xy_sd,
                     PARTICLEFILTER_VEHICLE_PERCEPTION_XY_STANDARD_DEVIATION);

    std::vector<double> weights;
// #pragma omp parallel for
    for (size_t i = 0; i < getParticles().size(); i++) {
        auto &particle = getParticles()[i];
        State<> par_s = *particle->getState();

        //// Likelihood XY
        double d_x = pose.position.x - par_s.x;
        double d_y = pose.position.y - par_s.y;

        Eigen::MatrixXd x(1, 2);
        x << d_x, d_y;

        Eigen::MatrixXd ans = x * covariance.inverse() * x.transpose();
        std::cout<<ans(0,0)<<std::endl;
        double d_xy = std::sqrt(d_x * d_x + d_y * d_y);
        double w = std::exp(-ans(0, 0) / 2);

        weights.push_back(w);
    }

    std::ostringstream ostr;
    for (int i = 0; i < 10; i++) {
        ostr << *getParticles()[i]->getState() << ",\t w: " << weights[i] << std::endl;
    }
    ROS_DEBUG_STREAM("[particleLikelihoodVehiclePerceptionXY] Particle Status: " << ostr.str());

    if (std::accumulate(weights.begin(), weights.end(), 0.0) != 0.0) {
        for (size_t i = 0; i < weights.size(); i++) {
            double w = weights[i] * getParticles()[i]->getWeight();
            getParticles()[i]->setWeight(w);
            ofs_weight << time.sec << "." << time.nsec << "," << std::string(__FUNCTION__) << "," << weights[i]
                       << std::endl;
        }
        particleNormalize();
//    std::cout << "Particle States \n" << pl->getParticles() << std::endl;
    } else {
        ROS_DEBUG("Skip likelihood: %s", __FUNCTION__);
    }

}
*/

void PhoneLocalizer::particleLikelihoodVehiclePerceptionXY() {

//    ROS_DEBUG("%s", __FUNCTION__);

    geometry_msgs::Pose pose = vehicle_perception.pose.pose;
    double dist = std::sqrt(std::pow(pose.position.x - now_phone_state.x, 2) +
                            std::pow(pose.position.y - now_phone_state.y, 2));
    if (dist > 10) {
        return;
    }

    Eigen::MatrixXd covariance(2, 2);
    covariance << vehicle_perception.pose.covariance[0], vehicle_perception.pose.covariance[1],
            vehicle_perception.pose.covariance[6], vehicle_perception.pose.covariance[7];

    static double xy_sd;
    private_nh.param("particlefilter_vehicle_perception_xy_statndard_deviation", xy_sd,
                     PARTICLEFILTER_VEHICLE_PERCEPTION_XY_STANDARD_DEVIATION);
    static double nis_th;
    private_nh.param("particlefilter_vehicle_perception_xy_nis_th",nis_th,PARTICLEFILTER_VEHICLE_PERCEPTION_XY_NIS_TH);

    std::vector<double> weights;
// #pragma omp parallel for
    for (size_t i = 0; i < getParticles().size(); i++) {
        auto &particle = getParticles()[i];
        State<> par_s = *particle->getState();

        //// Likelihood XY
        double d_x = pose.position.x - par_s.x;
        double d_y = pose.position.y - par_s.y;

        Eigen::MatrixXd x(1, 2);
        x << d_x, d_y;

        Eigen::MatrixXd ans = x * covariance.inverse() * x.transpose();
//        std::cout<<ans(0,0)<<std::endl;

        double w;
        if(ans(0,0)<nis_th){
            w=1.0;
        }
        else{
            w=0.0;
        }

        weights.push_back(w);
    }

    std::ostringstream ostr;
    for (int i = 0; i < 10; i++) {
        ostr << *getParticles()[i]->getState() << ",\t w: " << weights[i] << std::endl;
    }
    ROS_DEBUG_STREAM("[particleLikelihoodVehiclePerceptionXY] Particle Status: " << ostr.str());

    if (std::accumulate(weights.begin(), weights.end(), 0.0) != 0.0) {
        for (size_t i = 0; i < weights.size(); i++) {
            double w = weights[i] * getParticles()[i]->getWeight();
            getParticles()[i]->setWeight(w);
            ofs_weight << time.sec << "." << time.nsec << "," << std::string(__FUNCTION__) << "," << weights[i]
                       << std::endl;
        }
        particleNormalize();
//    std::cout << "Particle States \n" << pl->getParticles() << std::endl;
    } else {
        ROS_DEBUG("Skip likelihood: %s", __FUNCTION__);
    }

}



void PhoneLocalizer::particleLikelihoodVehiclePerceptionV() {

    ROS_DEBUG("%s", __FUNCTION__);

    double v_sd;
    private_nh.param("particlefilter_vehicle_perception_v_statndard_deviation", v_sd,
                     PARTICLEFILTER_VEHICLE_PERCEPTION_V_STANDARD_DEVIATION);

    if (vehicle_perception_state.vx == 0 && vehicle_perception_state.vy == 0) {
        return;
    }

    std::vector<double> weights;
// #pragma omp parallel for
    for (size_t i = 0; i < getParticles().size(); i++) {
        auto &particle = getParticles()[i];
        State<> par_s = *particle->getState();

        //// Likelihood XY
        double d_vx = vehicle_perception_state.x - par_s.vx;
        double d_vy = vehicle_perception_state.y - par_s.vy;

        double d_v = std::sqrt(d_vx * d_vx + d_vy * d_vy);
        double w = std::exp(-d_v * d_v / (2 * v_sd * v_sd));

        weights.push_back(w);
    }

    std::ostringstream ostr;
    for (int i = 0; i < 10; i++) {
        ostr << *getParticles()[i]->getState() << ",\t w: " << weights[i] << std::endl;
    }
    ROS_DEBUG_STREAM("[particleLikelihoodVehiclePerceptionV] Particle Status: " << ostr.str());

    if (std::accumulate(weights.begin(), weights.end(), 0.0) != 0.0) {
        for (size_t i = 0; i < weights.size(); i++) {
            double w = weights[i] * getParticles()[i]->getWeight();
            getParticles()[i]->setWeight(w);
            ofs_weight << time.sec << "." << time.nsec << "," << std::string(__FUNCTION__) << "," << weights[i]
                       << std::endl;
        }
        particleNormalize();
//    std::cout << "Particle States \n" << pl->getParticles() << std::endl;
    } else {
        ROS_DEBUG("Skip likelihood: %s", __FUNCTION__);
    }

}

void PhoneLocalizer::particleLikelihoodVehiclePerceptionR() {

    ROS_DEBUG("%s", __FUNCTION__);

    if (vehicle_perception_state.r == 0.0) {
        return;
    }

    double v_sd;
    private_nh.getParam("particlefilter_vehicle_perception_r_statndard_deviation", v_sd);

    std::vector<double> weights;
// #pragma omp parallel for
    for (size_t i = 0; i < getParticles().size(); i++) {
        auto &particle = getParticles()[i];
        State<> par_s = *particle->getState();

        //// Likelihood XY
        double d_r = vehicle_perception_state.r - par_s.r;
        if (d_r > M_PI) {
            d_r = M_PI * 2.0 - d_r;
        } else if (d_r < -M_PI) {
            d_r = -M_PI * 2.0 - d_r;
        }

        double w = std::exp(-d_r * d_r / (2 * v_sd * v_sd));

        weights.push_back(w);
    }

    std::ostringstream ostr;
    for (int i = 0; i < 10; i++) {
        ostr << *getParticles()[i]->getState() << ",\t w: " << weights[i] << std::endl;
    }
    ROS_DEBUG_STREAM("[particleLikelihoodVehiclePerceptionR] Particle Status: " << ostr.str());

    if (std::accumulate(weights.begin(), weights.end(), 0.0) != 0.0) {
        for (size_t i = 0; i < weights.size(); i++) {
            double w = weights[i] * getParticles()[i]->getWeight();
            getParticles()[i]->setWeight(w);
            ofs_weight << time.sec << "." << time.nsec << "," << std::string(__FUNCTION__) << "," << weights[i]
                       << std::endl;
        }
        particleNormalize();
//    std::cout << "Particle States \n" << pl->getParticles() << std::endl;
    } else {
        ROS_DEBUG("Skip likelihood: %s", __FUNCTION__);
    }

}

void PhoneLocalizer::imu2rpy(const sensor_msgs::Imu &imu, double *rpy) {

    tf::Quaternion quat(imu.orientation.x, imu.orientation.y, imu.orientation.z, imu.orientation.w);
    tf::Matrix3x3(quat).getRPY(*rpy, *(rpy + 1), *(rpy + 2));

}

void PhoneLocalizer::magnetic2yaw(const sensor_msgs::MagneticField &magnetic, double *rpy) {

    Eigen::Vector3f mag(magnetic.magnetic_field.x, magnetic.magnetic_field.y, magnetic.magnetic_field.z);
    Eigen::Matrix3f rp_mat;
    rpy2matrix(-rpy[0], -rpy[1], 0.0, &rp_mat);
    Eigen::Vector3f mag_horizontal = rp_mat * mag;
    *(rpy + 2) = -std::atan2(-mag_horizontal(0), mag_horizontal(1));

}

void PhoneLocalizer::rpy2matrix(double roll, double pitch, double yaw, Eigen::Matrix3f *rotation_matrix) {
    Eigen::Vector3f axis_dr(1, 0, 0), axis_dp(0, 1, 0), axis_dy(0, 0, 1);
    Eigen::Matrix3f matrix_dr, matrix_dp, matrix_dy;
    matrix_dr = Eigen::AngleAxisf(roll, axis_dr);
    matrix_dp = Eigen::AngleAxisf(pitch, axis_dp);
    matrix_dy = Eigen::AngleAxisf(yaw, axis_dy);
    *rotation_matrix = matrix_dy * matrix_dp * matrix_dr;
}

void PhoneLocalizer::quaternion2matrix(double qx, double qy, double qz, double qw, Eigen::Matrix3f *rotation_matrix) {
    tf::Quaternion quat(qx, qy, qz, qw);
    double rpy[3];
    tf::Matrix3x3(quat).getRPY(rpy[0], rpy[1], rpy[2]);
    rpy2matrix(rpy[0], rpy[1], rpy[2], rotation_matrix);
}

void PhoneLocalizer::object2state(const autoware_msgs::DetectedObject &object, State<> *state) {

    autoware_msgs::DetectedObject obj = object;

    geometry_msgs::PoseStamped in, out;
    in.header = obj.header;
    in.pose = obj.pose;
    out.header.frame_id = origin_frame_id;
    out.header.stamp = in.header.stamp;
    try {
        geometry_msgs::TransformStamped transform = tf_buffer.lookupTransform(origin_frame_id, in.header.frame_id,
                                                                              in.header.stamp, ros::Duration(1.0));
        if (use_vehicle_perception_vr && isOrientationActive(in.pose.orientation)) {
            tf2::doTransform(in, out, transform);
            update_vehicle_perception_xy = update_vehicle_perception_vr = true;
        } else {
            tf2::doTransform(in.pose.position, out.pose.position, transform);
            update_vehicle_perception_xy = true;
            update_vehicle_perception_vr = false;
            if (use_vehicle_perception_vr) {
                ROS_WARN("Quaternion in object pose is not active. Not using object orientation and speed. ");
            }
        }
    } catch (tf2::TransformException &ex) {
        ROS_WARN("Could NOT transform %s to %s: %s", obj.header.frame_id.c_str(), origin_frame_id.c_str(),
                 ex.what());
        return;
    }

    state->x = out.pose.position.x;
    state->y = out.pose.position.y;
    state->vx = obj.velocity.linear.x;
    state->vy = obj.velocity.linear.y;
    tf::Quaternion quat(out.pose.orientation.x,
                        out.pose.orientation.y,
                        out.pose.orientation.z,
                        out.pose.orientation.w);
    double rpy[3];
    tf::Matrix3x3(quat).getRPY(rpy[0], rpy[1], rpy[2]);
    state->r = State<>::normalize_radian(rpy[2]);


    ROS_DEBUG_STREAM("Pedestrian origin states: \n" << *state);

}

bool PhoneLocalizer::isOrientationActive(const geometry_msgs::Quaternion &q) {

    if (std::isnan(q.x) || std::isnan(q.y) || std::isnan(q.z) || std::isnan(q.w) ||
        (q.x == 0.0 && q.y == 0.0 && q.z == 0.0 && q.w == 0.0)) {
        return false;
    } else {
        return true;
    }

}
