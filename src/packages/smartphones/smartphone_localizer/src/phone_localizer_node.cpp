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
    PhoneLocalizer node;
    ros::spin();

    return (0);
}