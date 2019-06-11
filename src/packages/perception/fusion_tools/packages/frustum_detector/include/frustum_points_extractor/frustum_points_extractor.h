/*
 * Copyright 2018-2019 Autoware Foundation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ********************
 *  v1.0: amc-nu (abrahammonrroy@yahoo.com)
 *
 * frustum_points_extractor_node.h
 *
 *  Created on: July, 05th, 2018
 */

#ifndef PROJECT_FRUSTUM_FUSION_H
#define PROJECT_FRUSTUM_FUSION_H

#define __APP_NAME__ "frustum_points_extractor"

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>

#include <ros/ros.h>
#include <tf/tf.h>
#include <tf/transform_listener.h>

#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/CameraInfo.h>
#include <geometry_msgs/Point.h>

#include <jsk_recognition_utils/geo/cube.h>

#include <message_filters/subscriber.h>
#include <message_filters/synchronizer.h>
#include <message_filters/sync_policies/approximate_time.h>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>

#include <yaml-cpp/yaml.h>

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <pcl_conversions/pcl_conversions.h>
#include <pcl/PCLPointCloud2.h>
#include <pcl_ros/transforms.h>
#include <pcl_ros/point_cloud.h>
#include <pcl/point_types.h>

#include "autoware_msgs/DetectedObjectArray.h"


class FrustumPointsExtractor{

    // Node Handle
    ros::NodeHandle node_handle_;

    // Publisher
    ros::Publisher publisher_fused_objects_;
    ros::Publisher points_pub_;

    // Subscriber
    ros::Subscriber intrinsics_subscriber_;
    ros::Subscriber detections_vision_subscriber_;
    ros::Subscriber cloud_subscriber_;

    // Subscribed Messages
    autoware_msgs::DetectedObjectArray vision_detections_;
    sensor_msgs::PointCloud2 cloud_msg_;

    // TF Settings
    tf::TransformListener *transform_listener_;
    tf::StampedTransform camera_lidar_tf_;

    std::string image_frame_id_;



    // Callback Functions
    void VisionDetectionsCallback(const autoware_msgs::DetectedObjectArray::ConstPtr &in_detections_vision_msg);
    void CloudCallback(const sensor_msgs::PointCloud2::ConstPtr &in_cloud_msg);
    void SyncedDetectionsCallback(
            const autoware_msgs::DetectedObjectArray &in_vision_detections,
            const sensor_msgs::PointCloud2 &in_cloud_msg);

    // Lidar 3D Coordinate => Camera 3D Coordinate
    cv::Point3f TransformPoint(const pcl::PointXYZI &in_point, const tf::StampedTransform &in_transform);

    // Camera 3D Coordinate => Camera 2D Coordinate
    cv::Point2i ProjectPoint(const cv::Point3f &in_point);

    // Lidar 3D Coordinate => Camera 2D Coordinate
    cv::Point2i projectCloud2Img(const pcl::PointXYZI point,const tf::StampedTransform &in_transform);

    autoware_msgs::DetectedObjectArray FuseRangeVisionDetections(
            const autoware_msgs::DetectedObjectArray &in_vision_detections,
            const sensor_msgs::PointCloud2 &in_cloud_msg);

    void pubColoredPoints(const autoware_msgs::DetectedObjectArray &objects);




    message_filters::Subscriber<autoware_msgs::DetectedObjectArray>
            *vision_filter_subscriber_, *range_filter_subscriber_;


    cv::Size image_size_;
    cv::Mat camera_instrinsics_;
    cv::Mat distortion_coefficients_;

    bool processing_;
    bool camera_info_ok_;
    bool camera_lidar_tf_ok_;

    float fx_, fy_, cx_, cy_;

    size_t empty_frames_;

    typedef
    message_filters::sync_policies::ApproximateTime<autoware_msgs::DetectedObjectArray,
            autoware_msgs::DetectedObjectArray> SyncPolicyT;

    message_filters::Synchronizer<SyncPolicyT>
            *detections_synchronizer_;


    /*!
     * Obtains Transformation between two transforms registered in the TF Tree
     * @param in_target_frame
     * @param in_source_frame
     * @return the found transformation in the tree
     */
    tf::StampedTransform
    FindTransform(const std::string &in_target_frame, const std::string &in_source_frame);

    void IntrinsicsCallback(const sensor_msgs::CameraInfo &in_message);

    /*!
     * Reads the config params from the command line
     * @param in_private_handle
     */
    void InitializeROSIo(ros::NodeHandle &in_private_handle);

public:
    void Run();

    FrustumPointsExtractor();
};


#endif //PROJECT_FRUSTUM_FUSION_H
