# Install script for directory: /home/ohashi/test_perception/src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  
      if (NOT EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}")
        file(MAKE_DIRECTORY "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}")
      endif()
      if (NOT EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/.catkin")
        file(WRITE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/.catkin" "")
      endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/local/_setup_util.py")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/usr/local" TYPE PROGRAM FILES "/home/ohashi/test_perception/src/cmake-build-debug/catkin_generated/installspace/_setup_util.py")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/local/env.sh")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/usr/local" TYPE PROGRAM FILES "/home/ohashi/test_perception/src/cmake-build-debug/catkin_generated/installspace/env.sh")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/local/setup.bash;/usr/local/local_setup.bash")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/usr/local" TYPE FILE FILES
    "/home/ohashi/test_perception/src/cmake-build-debug/catkin_generated/installspace/setup.bash"
    "/home/ohashi/test_perception/src/cmake-build-debug/catkin_generated/installspace/local_setup.bash"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/local/setup.sh;/usr/local/local_setup.sh")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/usr/local" TYPE FILE FILES
    "/home/ohashi/test_perception/src/cmake-build-debug/catkin_generated/installspace/setup.sh"
    "/home/ohashi/test_perception/src/cmake-build-debug/catkin_generated/installspace/local_setup.sh"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/local/setup.zsh;/usr/local/local_setup.zsh")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/usr/local" TYPE FILE FILES
    "/home/ohashi/test_perception/src/cmake-build-debug/catkin_generated/installspace/setup.zsh"
    "/home/ohashi/test_perception/src/cmake-build-debug/catkin_generated/installspace/local_setup.zsh"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/local/.rosinstall")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/usr/local" TYPE FILE FILES "/home/ohashi/test_perception/src/cmake-build-debug/catkin_generated/installspace/.rosinstall")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/ohashi/test_perception/src/cmake-build-debug/gtest/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/localization/imu_tools/imu_tools/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/msgs/autoware_config_msgs/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/msgs/dbw_mkz_msgs/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/msgs/autoware_system_msgs/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/data/packages/vector_map_msgs/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/localization/lib/gnss/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/msgs/autoware_can_msgs/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/msgs/lgsvl_msgs/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/perception/vision_detector/libs/dpm_ttic/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/data/packages/obj_db/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/msgs/autoware_msgs/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/perception/visualizers/packages/detected_objects_visualizer/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/localization/imu_tools/imu_complementary_filter/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/perception/lidar_detector/packages/lidar_naive_l_shape_detect/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/perception/lidar_detector/packages/lidar_point_pillars/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/perception/fusion_tools/packages/pixel_cloud_fusion/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/perception/object_filter/packages/roi_object_filter/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/localization/imu_tools/imu_filter_madgwick/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/perception/object_filter/packages/lidar_shape_estimation/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/mypkg/cooperation/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/mypkg/cooperation_setup/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/localization/gnss_imu_localizer/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/perception/lidar_detector/packages/lidar_apollo_cnn_seg_detect/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/perception/lidar_detector/packages/lidar_fake_perception/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/perception/fusion_tools/packages/range_vision_fusion/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/mypkg/smartphone_localizer/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/utils/packages/autoware_rviz_plugins/autoware_launcher_rviz/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/utils/packages/autoware_rviz_plugins/state_panel/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/perception/visualizers/packages/integrated_viewer/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/system/autoware_health_checker/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/data/packages/pos_db/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/utils/packages/autoware_rviz_plugins/rosbag_controller/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/localization/imu_tools/rviz_imu_plugin/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/data/packages/vector_map/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/perception/lidar_tracker/packages/lidar_imm_ukf_pda_track/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/data/packages/map_file/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/semantics/object_map/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/data/packages/vector_map_server/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/perception/lidar_detector/packages/lidar_euclidean_cluster_detect/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/filter/points_preprocessor/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/perception/vision_tracker/packages/vision_beyond_track/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/perception/vision_detector/packages/vision_darknet_detect/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/perception/vision_detector/packages/vision_dpm_ttic_detect/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/perception/vision_detector/packages/vision_lane_detect/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/perception/vision_detector/packages/vision_segment_enet_detect/cmake_install.cmake")
  include("/home/ohashi/test_perception/src/cmake-build-debug/packages/perception/vision_detector/packages/vision_ssd_detect/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/home/ohashi/test_perception/src/cmake-build-debug/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
