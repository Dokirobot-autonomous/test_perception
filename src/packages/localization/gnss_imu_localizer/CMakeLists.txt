cmake_minimum_required(VERSION 2.8.3)
project(gnss_imu_localizer)

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
        INCLUDE_DIRS
#        include
)

include_directories(include ${catkin_INCLUDE_DIRS} ${EIGEN3_INCLUDE_DIRS})

SET(CMAKE_CXX_FLAGS "-O2 -g -Wall ${CMAKE_CXX_FLAGS}")

add_executable(gnss_imu_localizer_node src/gnss_imu_localizer_node.cpp)
target_link_libraries(gnss_imu_localizer_node ${catkin_LIBRARIES})
add_dependencies(gnss_imu_localizer_node ${catkin_EXPORTED_TARGETS})

install(TARGETS gnss_imu_localizer_node
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
        )

install(DIRECTORY launch/
        DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}/launch
        PATTERN ".svn" EXCLUDE)

