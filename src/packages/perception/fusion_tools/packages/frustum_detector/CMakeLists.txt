cmake_minimum_required(VERSION 2.8.3)
project(frustum_detector)

SET (CMAKE_CXX_FLAGS "-O3 -std=c++11 -Wall")

#find_package(autoware_build_flags REQUIRED)

find_package(autoware_msgs REQUIRED)
find_package(catkin REQUIRED COMPONENTS
        cv_bridge
        sensor_msgs
        image_transport
        roscpp
        tf
        jsk_recognition_utils
        eigen_conversions
        tf_conversions
        jsk_topic_tools
        image_geometry
        geometry_msgs
        jsk_topic_tools
        autoware_msgs
        pcl_conversions
        pcl_ros
        )

find_package(OpenCV REQUIRED)

find_package(Eigen3 QUIET)

find_package(OpenGL REQUIRED)

find_package(PCL REQUIRED)

find_package(CUDA)

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

IF (CUDA_FOUND)
    include_directories(${CUDA_INCLUDE_DIRS})

    IF ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "^arm")
        link_directories(/usr/lib/arm-linux-gnueabihf/tegra)
    endif ()
ENDIF ()
###CAFFE
set(CAFFE_PATH "$ENV{HOME}/caffe/distribute")

find_package(PkgConfig REQUIRED)
pkg_check_modules(YAML_CPP REQUIRED yaml-cpp)
find_path(YAML_CPP_INCLUDE_DIR NAMES yaml_cpp.h PATHS ${YAML_CPP_INCLUDE_DIRS})
find_library(YAML_CPP_LIBRARY NAMES YAML_CPP PATHS ${YAML_CPP_LIBRARIES})

catkin_package(
        CATKIN_DEPENDS
        cv_bridge
        sensor_msgs
        image_transport
        roscpp
        tf
        autoware_msgs
        jsk_recognition_utils
        eigen_conversions
        tf_conversions
        jsk_topic_tools
        image_geometry
        jsk_topic_tools
        pcl_conversions
        pcl_ros
        geometry_msgs
        INCLUDE_DIRS
        include
)

#fusion Library
add_library(frustum_points_extractor_lib SHARED
        src/frustum_points_extractor/frustum_points_extractor.cpp
        include/frustum_points_extractor/frustum_points_extractor.h
        )

if (OPENMP_FOUND)
    set_target_properties(frustum_points_extractor_lib PROPERTIES
            COMPILE_FLAGS ${OpenMP_CXX_FLAGS}
            LINK_FLAGS ${OpenMP_CXX_FLAGS}
            )
endif ()

target_include_directories(frustum_points_extractor_lib PRIVATE
        ${OpenCV_INCLUDE_DIR}
        ${catkin_INCLUDE_DIRS}
        ${autoware_msgs_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIRS}
        ${EIGEN3_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIRS}
        ${YAML_CPP_INCLUDE_DIRS}
        ${PCL_INCLUDE_DIRS}
        include
        )

target_link_libraries(frustum_points_extractor_lib
        ${OpenCV_LIBRARIES}
        ${EIGEN3_LIBRARIES}
        ${OPENGL_LIBRARIES}
        ${YAML_CPP_LIBRARIES}
        ${catkin_LIBRARIES}
        ${PCL_LIBRARIES}
        )

add_dependencies(frustum_points_extractor_lib
        ${catkin_EXPORTED_TARGETS}
        )

#Fusion node
add_executable(frustum_points_extractor
        src/frustum_points_extractor/frustum_points_extractor_node.cpp
        )
target_include_directories(frustum_points_extractor PRIVATE
        ${OpenCV_INCLUDE_DIR}
        ${catkin_INCLUDE_DIRS}
        ${autoware_msgs_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIRS}
        ${PCL_INCLUDE_DIRS}
        include
        )

target_link_libraries(frustum_points_extractor
        frustum_points_extractor_lib)

install(TARGETS frustum_points_extractor frustum_points_extractor_lib
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION})

include_directories(
        include
        ${catkin_INCLUDE_DIRS}
        ${OpenCV_INCLUDE_DIRS}
        ${CAFFE_PATH}/include
)

# apollo cnn segmentation
add_executable(apollo_cnn_segmentation
        src/apollo_cnn_segmentation/apollo_cnn_segmentation_node.cpp
        src/apollo_cnn_segmentation/cnn_segmentation.cpp
        src/apollo_cnn_segmentation/cluster2d.cpp
        src/apollo_cnn_segmentation/feature_generator.cpp
        )
target_link_libraries(apollo_cnn_segmentation
        ${catkin_LIBRARIES}
        ${OpenCV_LIBRARIES}
        ${CUDA_LIBRARIES}
        ${CUDA_CUBLAS_LIBRARIES}
        ${CUDA_curand_LIBRARY}
        ${CAFFE_PATH}/lib/libcaffe.so
        glog
        )
add_dependencies(apollo_cnn_segmentation
        ${catkin_EXPORTED_TARGETS}
        )
install(TARGETS
        apollo_cnn_segmentation
        ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
        RUNTIME DESTINATION ${CATKIN_GLOBAL_BIN_DESTINATION}
        )

include_directories(
        include
        ${catkin_INCLUDE_DIRS}
        ${autoware_msgs_INCLUDE_DIRS}
        ${OpenCV_INCLUDE_DIRS}
)
link_directories(${OpenCV_LIBRARY_DIRS})

install(DIRECTORY include/${PROJECT_NAME}/
        DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION})

install(DIRECTORY launch/
        DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}/launch
        PATTERN ".svn" EXCLUDE)

include_directories(/home/ohashi/test_perception/devel/include/)
