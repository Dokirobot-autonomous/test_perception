/*
 *  Copyright (c) 2015, Nagoya University

 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of Autoware nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "nmea2tfpose_core.h"

namespace gnss_localizer
{
// Constructor
Nmea2TFPoseNode::Nmea2TFPoseNode()
  : private_nh_("~")
  , MAP_FRAME_("map")
  , GPS_FRAME_("gps")
  , ODOM_FRAME_("odom")
  , roll_(0)
  , pitch_(0)
  , yaw_(0)
  , orientation_time_(0)
  , position_time_(0)
  , current_time_(0)
  , orientation_stamp_(0)
  , init_(true)
{
  initForROS();
  geo_.set_plane(plane_number_);
}

// Destructor
Nmea2TFPoseNode::~Nmea2TFPoseNode()
{
}

void Nmea2TFPoseNode::initForROS()
{
  // ros parameter settings
  private_nh_.getParam("plane", plane_number_);

  // setup subscriber
  sub1_ = nh_.subscribe("nmea_sentence", 100, &Nmea2TFPoseNode::callbackFromNmeaSentence, this);

  // setup subscriber
  sub2_ = nh_.subscribe("odom_pose", 100, &Nmea2TFPoseNode::callbackFromOdomPose, this);

  // setup publisher
  pub1_ = nh_.advertise<geometry_msgs::PoseStamped>("gnss_pose", 10);
}

void Nmea2TFPoseNode::run()
{
  ros::spin();
}

void Nmea2TFPoseNode::publishPoseStamped()
{
  ROS_INFO("Publish GNSS pose");
  geometry_msgs::PoseStamped pose;
  pose.header.frame_id = MAP_FRAME_;
  pose.header.stamp = current_time_;
  pose.pose.position.x = geo_.x();
  pose.pose.position.y = geo_.y();
  pose.pose.position.z = geo_.z();
  pose.pose.orientation = tf::createQuaternionMsgFromRollPitchYaw(roll_, pitch_, yaw_);
  pub1_.publish(pose);
}

void Nmea2TFPoseNode::publishTF()
{
  ROS_INFO("Publish TF from map to gps");
  tf::Transform transform;
  transform.setOrigin(tf::Vector3(geo_.x(), geo_.y(), geo_.z()));
  tf::Quaternion quaternion;
  quaternion.setRPY(roll_, pitch_, yaw_);
  transform.setRotation(quaternion);
  br_.sendTransform(tf::StampedTransform(transform, current_time_, MAP_FRAME_, GPS_FRAME_));
}

void Nmea2TFPoseNode::publishTFMap2Odom()
{
  ROS_INFO("Publish TF from map to odom");
  tf::Transform transform;
  transform.setOrigin(tf::Vector3(tf_map2odom[0], tf_map2odom[1], tf_map2odom[2]));
  tf::Quaternion quaternion;
  quaternion.setRPY(tf_map2odom[3], tf_map2odom[4], tf_map2odom[5]);
  transform.setRotation(quaternion);
  br_.sendTransform(tf::StampedTransform(transform, current_time_, MAP_FRAME_, ODOM_FRAME_));
}


void Nmea2TFPoseNode::createOrientation()
{
  yaw_ = atan2(geo_.y() - last_geo_.y(), geo_.x() - last_geo_.x());
  roll_ = 0;
  pitch_ = 0;
}

/*void Nmea2TFPoseNode::computeTransformationMap2Odom()
{
	ROS_INFO("computeTransformationMap2Odom");

  // compute transformation of orientation
  tf::Quaternion q(odom_pose.orientation.x,odom_pose.orientation.y,odom_pose.orientation.z,odom_pose.orientation.w);
  tf::Matrix3x3 m(q);
  double odom_roll,odom_pitch,odom_yaw;
  m.getRPY(odom_roll,odom_pitch,odom_yaw);
//  std::cout<<odom_roll<<","<<odom_pitch<<","<<odom_yaw<<std::endl;
//  std::cout<<roll_<<","<<pitch_<<","<<yaw_<<std::endl;
  tf_map2odom[3]=roll_-odom_roll;
  tf_map2odom[4]=pitch_-odom_pitch;
  tf_map2odom[5]=yaw_-odom_yaw;

  tf::Transform transform;
  transform

  // compute transformation of position
//  tf_map2odom[0]=geo_.x()-(odom_pose.position.x*cos(tf_map2odom[5])*cos(tf_map2odom[5]));
  tf_map2odom[1]=geo_.y()-odom_pose.position.y;
  tf_map2odom[2]=geo_.z()-odom_pose.position.z;
}*/

/*void Nmea2TFPoseNode::computeTransformationMap2Odom()
{
  ROS_INFO("computeTransformationMap2Odom");
  // convert gnss_pose type from geometry to tf
  tf::Transform gnss_pose_tf;
  gnss_pose_tf.setOrigin(tf::Vector3(geo_.x(),geo_.y(),geo_.z()));
  gnss_pose_tf.setRotation(tf::Quaternion(roll_,pitch_,yaw_));

  // convert odom_pose type from geometry to tf
  tf::Transform odom_pose_tf_conjugate;
  odom_pose_tf_conjugate.setOrigin(tf::Vector3(-odom_pose.position.x,-odom_pose.position.y,-odom_pose.position.z));
  odom_pose_tf_conjugate.setRotation(
		  tf::Quaternion(-odom_pose.orientation.x,-odom_pose.orientation.y,-odom_pose.orientation.z,odom_pose.orientation.w));

  tf_map2odom.setOrigin(odom_pose_tf_conjugate*gnss_pose_tf.getOrigin());
  tf_map2odom.setRotation(odom_pose_tf_conjugate*gnss_pose_tf.getRotation());
}*/

void Nmea2TFPoseNode::computeTransformationMap2Odom()
{
  ROS_INFO("computeTransformationMap2Odom");
  // quaternion odom_pose
  tf::Quaternion q(odom_pose.orientation.x,odom_pose.orientation.y,odom_pose.orientation.z,odom_pose.orientation.w);
  tf::Matrix3x3 m(q);
  double odom_roll,odom_pitch,odom_yaw;
  m.getRPY(odom_roll,odom_pitch,odom_yaw);

  //
  float dr,dp,dy;
  dr=roll_-odom_roll;
  dp=pitch_-odom_pitch;
  dy=yaw_-odom_yaw;

  // create rotation matrix
  Eigen::Vector3f axis_dr(1,0,0),axis_dp(0,1,0),axis_dy(0,0,1);
  Eigen::Matrix3f matrix_dr,matrix_dp,matrix_dy;
  matrix_dr=Eigen::AngleAxisf(dr,axis_dr);
  matrix_dp=Eigen::AngleAxisf(dp,axis_dp);
  matrix_dy=Eigen::AngleAxisf(dy,axis_dy);

  // rotate odom_pose
  Eigen::Vector3f odom_pose_rotated=matrix_dy*matrix_dp*matrix_dr*
		  Eigen::Vector3f(odom_pose.position.x,odom_pose.position.y,odom_pose.position.z);

  tf_map2odom[0]=geo_.x()-odom_pose_rotated(0);
  tf_map2odom[1]=geo_.y()-odom_pose_rotated(1);
  tf_map2odom[2]=geo_.z()-odom_pose_rotated(2);
  tf_map2odom[3]=dr;
  tf_map2odom[4]=dp;
  tf_map2odom[5]=dy;
}



void Nmea2TFPoseNode::convert(std::vector<std::string> nmea, ros::Time current_stamp)
{
  try
  {
    if (nmea.at(0).compare(0, 2, "QQ") == 0)
    {
      orientation_time_ = stod(nmea.at(3));
      roll_ = stod(nmea.at(4)) * M_PI / 180.;
      pitch_ = -1 * stod(nmea.at(5)) * M_PI / 180.;
      yaw_ = -1 * stod(nmea.at(6)) * M_PI / 180. + M_PI / 2;
      orientation_stamp_ = current_stamp;
      ROS_INFO("QQ is subscribed.");
    }
    else if (nmea.at(0) == "$PASHR")
    {
      orientation_time_ = stod(nmea.at(1));
      roll_ = stod(nmea.at(4)) * M_PI / 180.;
      pitch_ = -1 * stod(nmea.at(5)) * M_PI / 180.;
      yaw_ = -1 * stod(nmea.at(2)) * M_PI / 180. + M_PI / 2;
      ROS_INFO("PASHR is subscribed.");
    }
    else if(nmea.at(0).compare(3, 3, "GGA") == 0)
    {
      position_time_ = stod(nmea.at(1));
      double lat = stod(nmea.at(2));
      double lon = stod(nmea.at(4));
      double h = stod(nmea.at(9));
      geo_.set_llh_nmea_degrees(lat, lon, h);
      ROS_INFO("GGA is subscribed.");
    }
    else if(nmea.at(0) == "$GPRMC")
    {
      position_time_ = stoi(nmea.at(1));
      double lat = stod(nmea.at(3));
      double lon = stod(nmea.at(5));
      double h = 0.0;
      geo_.set_llh_nmea_degrees(lat, lon, h);
      ROS_INFO("GPRMC is subscribed.");
    }
  }catch (const std::exception &e)
  {
    ROS_WARN_STREAM("Message is invalid : " << e.what());
  }
}

void Nmea2TFPoseNode::callbackFromNmeaSentence(const nmea_msgs::Sentence::ConstPtr &msg)
{
      ROS_INFO("NMEA sentence is subscrived.");
  current_time_ = msg->header.stamp;
  convert(split(msg->sentence), msg->header.stamp);

  double timeout = 10.0;
  if (fabs(orientation_stamp_.toSec() - msg->header.stamp.toSec()) > timeout)
  {
	if (init_)
	{
      computeTransformationMap2Odom();
      publishPoseStamped();
      publishTF();
      publishTFMap2Odom();
      last_geo_ = geo_;
      init_=false;
	}
	else{
		double dt = sqrt(pow(geo_.x() - last_geo_.x(), 2) + pow(geo_.y() - last_geo_.y(), 2));
		double threshold = 0.5;
		if (dt > threshold)
		{
			ROS_INFO("QQ is not subscribed. Orientation is created by atan2");
			createOrientation();
      computeTransformationMap2Odom();
      publishPoseStamped();
      publishTF();
      publishTFMap2Odom();
      last_geo_ = geo_;
    }
	}
    return;
  }

  double e = 1e-2;
  if (fabs(orientation_time_ - position_time_) < e)
  {
    publishPoseStamped();
    publishTF();
    return;
  }
}

void Nmea2TFPoseNode::callbackFromOdomPose(const nav_msgs::Odometry::ConstPtr &msg)
{
  ROS_INFO("Odometory pose is subscrived");
  odom_pose=msg->pose.pose;
  publishTFMap2Odom();  // odom poseを反映しない
}


std::vector<std::string> split(const std::string &string)
{
  std::vector<std::string> str_vec_ptr;
  std::string token;
  std::stringstream ss(string);

  while (getline(ss, token, ','))
    str_vec_ptr.push_back(token);

  return str_vec_ptr;
}

}  // gnss_localizer
