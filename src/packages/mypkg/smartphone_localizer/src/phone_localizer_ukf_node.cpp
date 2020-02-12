//
// Created by ohashi on 19/09/13.
//

#include <phone_localizer_ukf.h>

/**
 *  Main Function
 **/
int main(int argc, char **argv) {

    ros::init(argc, argv, "phone_localizer_ukf_node");
    ros::NodeHandle nh,private_nh("~");
    //// debug mode
    bool debug;
    private_nh.param<bool>("debug",debug,false);
    if(debug){
        if (ros::console::set_logger_level(ROSCONSOLE_DEFAULT_NAME, ros::console::levels::Debug)) {
            ros::console::notifyLoggerLevelsChanged();
        }
    }
    PhoneLocalizerUKF node(&nh,&private_nh);
    ros::spin();

    return (0);
}