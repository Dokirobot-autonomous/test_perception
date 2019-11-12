//
// Created by ohashi on 19/09/13.
//

#include <vehicle_perceptor.h>


/**
 * Main Function
 */

int main(int argc, char **argv) {

//    // debug mode
//    if (ros::console::set_logger_level(ROSCONSOLE_DEFAULT_NAME, ros::console::levels::Debug)) {
//        ros::console::notifyLoggerLevelsChanged();
//    }

    //// Initialize ROS
    ros::init(argc, argv, "vehicle_perceptor");
    ros::NodeHandle nh,private_nh("~");

    //// Phone Localizer
    std::vector<PhoneLocalizer*> phone_localizers;
    std::string phone_names, token;
    if (!private_nh.getParam("phone_names", phone_names)) {
        phone_names = PHONE_NAMES;
    }
    std::stringstream ss(phone_names);
    while (std::getline(ss, token, ',')) {
        PhoneLocalizer *pl = new PhoneLocalizer(&nh, &private_nh, token);
        phone_localizers.push_back(pl);
    }

    //// Vehicle Perceptor
    std::string vehicle_name=VEHICLE_NAME;
    private_nh.getParam("vehicle_name",vehicle_name);
    VehiclePhoneManager node(&nh,&private_nh,&phone_localizers,vehicle_name);

    ros::spin();

    return (0);
}