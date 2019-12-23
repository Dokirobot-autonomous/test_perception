cmake_minimum_required(VERSION 2.8.3)
project(cooperation_setup)

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
        gnss
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

include_directories(include include/autoware_msgs include/${PROJECT_NAME}/ ${catkin_INCLUDE_DIRS} ${EIGEN3_INCLUDE_DIRS})

SET(CMAKE_CXX_FLAGS "-O2 -g -Wall ${CMAKE_CXX_FLAGS}")

add_executable(prius_setup src/prius_setup.cpp)
target_link_libraries(prius_setup ${catkin_LIBRARIES})
add_dependencies(prius_setup ${catkin_EXPORTED_TARGETS})
install(TARGETS prius_setup
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        )

add_executable(mkz_setup src/mkz_setup.cpp)
target_link_libraries(mkz_setup ${catkin_LIBRARIES})
add_dependencies(mkz_setup ${catkin_EXPORTED_TARGETS})
install(TARGETS mkz_setup
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        )

add_executable(connection_client_setup src/connection_client_setup.cpp)
target_link_libraries(connection_client_setup ${catkin_LIBRARIES})
add_dependencies(connection_client_setup ${catkin_EXPORTED_TARGETS})
install(TARGETS connection_client_setup
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        )

add_executable(android_setup src/android_setup.cpp)
target_link_libraries(android_setup ${catkin_LIBRARIES})
add_dependencies(android_setup ${catkin_EXPORTED_TARGETS})
install(TARGETS android_setup
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        )

add_executable(nmea2fix src/nmea2fix.cpp)
target_link_libraries(nmea2fix ${catkin_LIBRARIES})
add_dependencies(nmea2fix ${catkin_EXPORTED_TARGETS})
install(TARGETS nmea2fix
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        )

add_executable(fix_tf_publisher src/fix_tf_publisher.cpp)
target_link_libraries(fix_tf_publisher ${catkin_LIBRARIES})
add_dependencies(fix_tf_publisher ${catkin_EXPORTED_TARGETS})
install(TARGETS fix_tf_publisher
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        )

install(DIRECTORY launch/
        DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}/launch
        PATTERN ".svn" EXCLUDE)

