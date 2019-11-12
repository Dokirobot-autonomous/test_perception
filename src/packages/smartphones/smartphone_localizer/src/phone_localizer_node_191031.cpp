//
// Created by ohashi on 19/09/13.
//

#include <phone_localizer.h>

/**
 *  Main Function
 **/
int main(int argc, char **argv) {

//    // debug mode
//    if (ros::console::set_logger_level(ROSCONSOLE_DEFAULT_NAME, ros::console::levels::Debug)) {
//        ros::console::notifyLoggerLevelsChanged();
//    }

    ros::init(argc, argv, "phone_localizer_node");
    ros::NodeHandle nh,private_nh("~");
    std::string phone_name;
    if(!private_nh.getParam("phone_name",phone_name)){
        phone_name=PHONE_NAME;
    }
    PhoneLocalizer node(&nh,&private_nh,phone_name);
    ros::spin();

    return (0);
}