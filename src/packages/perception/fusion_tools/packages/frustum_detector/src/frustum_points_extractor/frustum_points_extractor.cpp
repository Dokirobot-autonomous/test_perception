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
 * frustum_points_extractor_node.cpp
 *
 *  Created on: July, 05th, 2018
 */

#include "frustum_points_extractor/frustum_points_extractor.h"

cv::Point3f
FrustumPointsExtractor::TransformPoint(const pcl::PointXYZI &in_point,
                              const tf::StampedTransform &in_transform) {
    tf::Vector3 tf_point(in_point.x, in_point.y, in_point.z);
    tf::Vector3 tf_point_t = in_transform * tf_point;
    return cv::Point3f(tf_point_t.x(), tf_point_t.y(), tf_point_t.z());
}

cv::Point2i
FrustumPointsExtractor::ProjectPoint(const cv::Point3f &in_point) {
    auto u = int(in_point.x * fx_ / in_point.z + cx_);
    auto v = int(in_point.y * fy_ / in_point.z + cy_);
    return cv::Point2i(u, v);
}


autoware_msgs::DetectedObjectArray
FrustumPointsExtractor::FuseRangeVisionDetections(
        const autoware_msgs::DetectedObjectArray &in_vision_detections,
        const sensor_msgs::PointCloud2 &in_cloud_msg) {
    ROS_DEBUG("FuseRangeVisionDetections");

    autoware_msgs::DetectedObjectArray fusion_detections;

    pcl::PointCloud<pcl::PointXYZI>::Ptr in_cloud(new pcl::PointCloud<pcl::PointXYZI>);
    pcl::fromROSMsg(in_cloud_msg, *in_cloud);

    // Rect内の点群抽出
    std::vector<pcl::PointCloud<pcl::PointXYZI>> cloud_in_rects(in_vision_detections.objects.size());
    for (const auto& point:*in_cloud){
        for (size_t i=0;i<in_vision_detections.objects.size();i++) {
            const auto &object = in_vision_detections.objects[i];

            // Projection
            if (point.x<0){
                continue;
            }
            cv::Point3f point3f_vision = TransformPoint(point, camera_lidar_tf_);
            cv::Point2i point2i_vision = ProjectPoint(point3f_vision);

            cv::Rect vision_rect(object.x, object.y,
                                 object.width, object.height);

            if (point2i_vision.x >= vision_rect.x && point2i_vision.x <= vision_rect.x + vision_rect.width &&
                point2i_vision.y >= vision_rect.y && point2i_vision.y <= vision_rect.y + vision_rect.height) {

                cloud_in_rects[i].points.push_back(point);
            }
        }
    }

    for(size_t i=0;i<in_vision_detections.objects.size();i++){
        sensor_msgs::PointCloud2 tmp;
        pcl::toROSMsg(cloud_in_rects[i],tmp);
        autoware_msgs::DetectedObject object;
        object=in_vision_detections.objects[i];
        object.pointcloud=tmp;
        object.pointcloud.header=in_cloud_msg.header;
        fusion_detections.objects.push_back(object);
    }

    pubColoredPoints(fusion_detections);

    return fusion_detections;
}

void FrustumPointsExtractor::pubColoredPoints(const autoware_msgs::DetectedObjectArray &objects_array) {
    pcl::PointCloud <pcl::PointXYZRGB> colored_cloud;
    for (size_t object_i = 0; object_i < objects_array.objects.size(); object_i++) {
        std::cout << objects_array.objects[object_i].label << std::endl;
        // std::cout << "objct i" << object_i << std::endl;
        pcl::PointCloud <pcl::PointXYZI> object_cloud;
        pcl::fromROSMsg(objects_array.objects[object_i].pointcloud, object_cloud);
        int red = (object_i) % 256;
        int green = (object_i * 7) % 256;
        int blue = (object_i * 13) % 256;
        for (size_t i = 0; i < object_cloud.size(); i++) {
            // std::cout << "point i" << i << "/ size: "<<object_cloud.size()  << std::endl;
            pcl::PointXYZRGB colored_point;
            colored_point.x = object_cloud[i].x;
            colored_point.y = object_cloud[i].y;
            colored_point.z = object_cloud[i].z;
            colored_point.r = red;
            colored_point.g = green;
            colored_point.b = blue;
            colored_cloud.push_back(colored_point);

        }
    }

    sensor_msgs::PointCloud2 output_colored_cloud;
    pcl::toROSMsg(colored_cloud, output_colored_cloud);
    if (!objects_array.objects.empty()){
        output_colored_cloud.header = objects_array.objects[0].pointcloud.header;
    }
    points_pub_.publish(output_colored_cloud);

}


void
FrustumPointsExtractor::SyncedDetectionsCallback(
        const autoware_msgs::DetectedObjectArray &in_vision_detections,
        const sensor_msgs::PointCloud2 &in_cloud_msg) {
    autoware_msgs::DetectedObjectArray fusion_objects;
    fusion_objects.objects.clear();

    if (empty_frames_ > 5) {
        ROS_INFO("[%s] Empty Detections. Make sure the vision and range detectors are running.", __APP_NAME__);
    }

//    if (nullptr == in_vision_detections
//        && nullptr == in_cloud_msg) {
//        empty_frames_++;
//        return;
//    }

//    if (nullptr == in_vision_detections
//        && nullptr != in_cloud_msg
//        && !in_cloud_msg->objects.empty()) {
//        publisher_fused_objects_.publish(in_range_detections);
//        empty_frames_++;
//        return;
//    }

//    if (nullptr == in_cloud_msg
//        && nullptr != in_cloud_msg
//        && !in_vision_detections->objects.empty()) {
//        publisher_fused_objects_.publish(in_vision_detections);
//        empty_frames_++;
//        return;
//    }

    if (!camera_lidar_tf_ok_) {
        camera_lidar_tf_ = FindTransform(image_frame_id_,
                                         in_cloud_msg.header.frame_id);
    }
    if (
            !camera_lidar_tf_ok_ ||
            !camera_info_ok_) {
        ROS_INFO("[%s] Missing Camera-LiDAR TF or CameraInfo", __APP_NAME__);
        return;
    }

    fusion_objects = FuseRangeVisionDetections(in_vision_detections, in_cloud_msg);

    publisher_fused_objects_.publish(fusion_objects);
    empty_frames_ = 0;

//    vision_detections_ = nullptr;
//    cloud_msg_ = nullptr;

}

void
FrustumPointsExtractor::VisionDetectionsCallback(
        const autoware_msgs::DetectedObjectArray::ConstPtr &in_vision_detections) {
    ROS_DEBUG("[%s] Subscribe VisionObjects", __APP_NAME__);
    if (!processing_ && !in_vision_detections->objects.empty()) {
        processing_ = true;
        vision_detections_ = *in_vision_detections;
        SyncedDetectionsCallback(vision_detections_, cloud_msg_);
        processing_ = false;
    }
}

void
FrustumPointsExtractor::CloudCallback(const sensor_msgs::PointCloud2::ConstPtr &in_cloud_msg) {
    ROS_DEBUG("[%s]Subscribe PointCloud ", __APP_NAME__);
    if (!processing_ && !in_cloud_msg->data.empty()) {
        processing_ = true;
        cloud_msg_ = *in_cloud_msg;
        SyncedDetectionsCallback(vision_detections_, cloud_msg_);
        processing_ = false;
    }

}

void
FrustumPointsExtractor::IntrinsicsCallback(const sensor_msgs::CameraInfo &in_message) {
    ROS_DEBUG("[%s: IntrinsicsCallback()] Callback camera_info", __APP_NAME__);
    image_size_.height = in_message.height;
    image_size_.width = in_message.width;

//    ROS_DEBUG("[%s: IntrinsicsCallback()] 1",__APP_NAME__);
    camera_instrinsics_ = cv::Mat(3, 3, CV_64F);
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            camera_instrinsics_.at<double>(row, col) = in_message.K[row * 3 + col];
        }
    }
//    ROS_DEBUG("[%s: IntrinsicsCallback()] 2",__APP_NAME__);

    distortion_coefficients_ = cv::Mat(1, 5, CV_64F);
    if (in_message.D.empty()) {
        distortion_coefficients_ = cv::Mat::zeros(1, 5, CV_64F);
    } else {
        for (int col = 0; col < 5; col++) {
            distortion_coefficients_.at<double>(col) = in_message.D[col];
        }
    }

    ROS_DEBUG("[%s: IntrinsicsCallback()] 3", __APP_NAME__);

    fx_ = static_cast<float>(in_message.P[0]);
    fy_ = static_cast<float>(in_message.P[5]);
    cx_ = static_cast<float>(in_message.P[2]);
    cy_ = static_cast<float>(in_message.P[6]);

    intrinsics_subscriber_.shutdown();
    camera_info_ok_ = true;
    image_frame_id_ = in_message.header.frame_id;
    ROS_INFO("[%s: IntrinsicsCallback()] CameraIntrinsics obtained.", __APP_NAME__);
}

tf::StampedTransform
FrustumPointsExtractor::FindTransform(const std::string &in_target_frame, const std::string &in_source_frame) {
    tf::StampedTransform transform;

    ROS_INFO("%s - > %s", in_source_frame.c_str(), in_target_frame.c_str());
    camera_lidar_tf_ok_ = false;
    try {
        transform_listener_->lookupTransform(in_target_frame, in_source_frame, ros::Time(0), transform);
        camera_lidar_tf_ok_ = true;
        ROS_INFO("[%s] Camera-Lidar TF obtained", __APP_NAME__);
    }
    catch (tf::TransformException &ex) {
        ROS_ERROR("[%s] %s", __APP_NAME__, ex.what());
    }

    return transform;
}

void
FrustumPointsExtractor::InitializeROSIo(ros::NodeHandle &in_private_handle) {
    //get params
    std::string camera_info_src, detected_objects_vision, min_car_dimensions, min_person_dimensions, min_truck_dimensions;
    std::string cloud_src, fused_topic_str = "/detection/frustum_detector/objects_before_segmentation";
    std::string name_space_str = ros::this_node::getNamespace();
    bool sync_topics = false;

    ROS_INFO(
            "[%s] This node requires: Registered TF(Lidar-Camera), CameraInfo, Vision and Range Detections being published.",
            __APP_NAME__);
    in_private_handle.param<std::string>("points_raw", cloud_src,
                                         "/points_raw");
    ROS_INFO("[%s] pointcloud: %s", __APP_NAME__, cloud_src.c_str());

    in_private_handle.param<std::string>("detected_objects_vision", detected_objects_vision,
                                         "/detection/image_detector/objects");
    ROS_INFO("[%s] detected_objects_vision: %s", __APP_NAME__, detected_objects_vision.c_str());

    in_private_handle.param<std::string>("camera_info_src", camera_info_src, "/camera_info");
    ROS_INFO("[%s] camera_info_src: %s", __APP_NAME__, camera_info_src.c_str());


    in_private_handle.param<bool>("sync_topics", sync_topics, false);
    ROS_INFO("[%s] sync_topics: %d", __APP_NAME__, sync_topics);

    if (name_space_str != "/") {
        if (name_space_str.substr(0, 2) == "//") {
            name_space_str.erase(name_space_str.begin());
        }
        camera_info_src = name_space_str + camera_info_src;
    }

    //generate subscribers and sychronizers
    ROS_INFO("[%s] Subscribing to... %s", __APP_NAME__, camera_info_src.c_str());
    intrinsics_subscriber_ = in_private_handle.subscribe(camera_info_src,
                                                         1,
                                                         &FrustumPointsExtractor::IntrinsicsCallback, this);

    ROS_INFO("[%s] Subscribing to... %s", __APP_NAME__, detected_objects_vision.c_str());
    ROS_INFO("[%s] Subscribing to... %s", __APP_NAME__, cloud_src.c_str());
    if (!sync_topics) {
        detections_vision_subscriber_ = in_private_handle.subscribe(detected_objects_vision,
                                                                    1,
                                                                    &FrustumPointsExtractor::VisionDetectionsCallback,
                                                                    this);

        cloud_subscriber_ = in_private_handle.subscribe(cloud_src,
                                                        1,
                                                        &FrustumPointsExtractor::CloudCallback,
                                                        this);
    } else {
//        vision_filter_subscriber_ = new message_filters::Subscriber<autoware_msgs::DetectedObjectArray>(node_handle_,
//                                                                                                        detected_objects_vision,
//                                                                                                        1);
//        range_filter_subscriber_ = new message_filters::Subscriber<autoware_msgs::DetectedObjectArray>(node_handle_,
//                                                                                                       detected_objects_range,
//                                                                                                       1);
//        detections_synchronizer_ =
//                new message_filters::Synchronizer<SyncPolicyT>(SyncPolicyT(10),
//                                                               *vision_filter_subscriber_,
//                                                               *range_filter_subscriber_);
//        detections_synchronizer_->registerCallback(
//                boost::bind(&FrustumPointsExtractor::SyncedDetectionsCallback, this, _1, _2));
    }

    publisher_fused_objects_ = node_handle_.advertise<autoware_msgs::DetectedObjectArray>(fused_topic_str, 1);
    points_pub_ = node_handle_.advertise<sensor_msgs::PointCloud2>("/detection/frustum_detector/points_cluster_before_segmentation", 1);

    ROS_INFO("[%s] Publishing fused objects in %s", __APP_NAME__, fused_topic_str.c_str());

}


void
FrustumPointsExtractor::Run() {
    ros::NodeHandle private_node_handle("~");
    tf::TransformListener transform_listener;

    transform_listener_ = &transform_listener;

    InitializeROSIo(private_node_handle);

    ROS_INFO("[%s] Ready. Waiting for data...", __APP_NAME__);

    ros::spin();

    ROS_INFO("[%s] END", __APP_NAME__);
}

FrustumPointsExtractor::FrustumPointsExtractor() {
    camera_lidar_tf_ok_ = false;
    camera_info_ok_ = false;
    processing_ = false;
    image_frame_id_ = "";
    empty_frames_ = 0;
}