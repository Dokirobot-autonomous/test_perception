cmake_minimum_required(VERSION 2.8.3)
project(smartphone_localizer)

## Compile as C++11, supported in ROS Kinetic and newer
SET (CMAKE_CXX_FLAGS "-O3 -std=c++11 -Wall")

## Find catkin macros and libraries
## if COMPONENTS list like find_package(catkin REQUIRED COMPONENTS xyz)
## is used, also find other catkin packages
find_package(catkin REQUIRED COMPONENTS
        roscpp
        rospy
        sensor_msgs
        std_msgs
        tf
        eigen_conversions
        tf_conversions
        geometry_msgs
        autoware_msgs
        )

find_package(Eigen3 QUIET)

if (NOT EIGEN3_FOUND)
    # Fallback to cmake_modules
    find_package(cmake_modules REQUIRED)
    find_package(Eigen REQUIRED)
    set(EIGEN3_INCLUDE_DIRS ${EIGEN_INCLUDE_DIRS})
    set(EIGEN3_LIBRARIES ${EIGEN_LIBRARIES})  # Not strictly necessary as Eigen is head only
    # Possibly map additional variables to the EIGEN3_ prefix.
else ()
    set(EIGEN3_INCLUDE_DIRS ${EIGEN3_INCLUDE_DIR})
endif ()

catkin_package(
        CATKIN_DEPENDS
        sensor_msgs
        roscpp
        tf
        eigen_conversions
        tf_conversions
        geometry_msgs
        autoware_msgs
        INCLUDE_DIRS
        include
)

include_directories(include include/autoware_msgs/ include/${PROJECT_NAME}/  ${catkin_INCLUDE_DIRS} ${EIGEN3_INCLUDE_DIRS})

SET(CMAKE_CXX_FLAGS "-O2 -g -Wall ${CMAKE_CXX_FLAGS}")

install(DIRECTORY include/${PROJECT_NAME}/ include/autoware_msgs/
        DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
        PATTERN ".svn" EXCLUDE
        )

add_executable(phone_localizer
        src/phone_localizer_node.cpp
        src/phone_localizer.cpp
        src/geo_pos_conv.cpp)
target_link_libraries(phone_localizer ${catkin_LIBRARIES})
add_dependencies(phone_localizer ${catkin_EXPORTED_TARGETS})
install(TARGETS phone_localizer
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        )

add_executable(phone_vehicle_fusion
        src/phone_vehicle_fusioin.cpp
        src/phone_localizer.cpp
        src/geo_pos_conv.cpp)
target_link_libraries(phone_vehicle_fusion ${catkin_LIBRARIES})
add_dependencies(phone_vehicle_fusion ${catkin_EXPORTED_TARGETS})
install(TARGETS phone_vehicle_fusion
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        )


add_executable(smartphone_sensors src/smartphone_node.cpp src/geo_pos_conv.cpp)
target_link_libraries(smartphone_sensors ${catkin_LIBRARIES})
add_dependencies(smartphone_sensors ${catkin_EXPORTED_TARGETS})
install(TARGETS smartphone_sensors
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        )

add_executable(fusion src/fusion_node.cpp src/geo_pos_conv.cpp)
target_link_libraries(fusion ${catkin_LIBRARIES})
add_dependencies(fusion ${catkin_EXPORTED_TARGETS})
install(TARGETS fusion
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        )


add_executable(phonesensors_raw src/smartphone_raw.cpp src/geo_pos_conv.cpp)
target_link_libraries(phonesensors_raw ${catkin_LIBRARIES})
add_dependencies(phonesensors_raw ${catkin_EXPORTED_TARGETS})
install(TARGETS phonesensors_raw
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        )

install(DIRECTORY launch/
        DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}/launch
        PATTERN ".svn" EXCLUDE)

