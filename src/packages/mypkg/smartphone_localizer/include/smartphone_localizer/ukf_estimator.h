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
 */

#ifndef UKF_ESTIMATOR_H
#define UKF_ESTIMATOR_H

#include "Eigen/Dense"
#include <ros/ros.h>
#include <vector>
#include <string>
#include <fstream>

#include "../../../../../../devel/include/autoware_msgs/DetectedObject.h"
#include "../../../../../../devel/include/autoware_msgs/DetectedObjectArray.h"

enum MotionModel : int
{
    CV = 0,    // constant velocity
    CTRV = 1,  // constant turn rate and velocity
    RM = 2,    // random motion
};

enum MeasurementSensor : int
{
    IMU = 0,
    GNSSRAW_ACCELERATION = 1,
    PERCEPTION = 2,
};

class UkfEstimator
{
    /*
    cv: Constant Velocity
    ctrv: Constatnt Turn Rate and Velocity
    rm: Random Motion
    */

public:

    bool init=false;

    int ukf_id_;

    int num_state_;

/*
    int num_lidar_state_;

    int num_lidar_direction_state_;

    int num_motion_model_;
*/

/*
    //* state vector: [pos1 pos2 vel_abs yaw_angle yaw_rate] in SI units and rad
    Eigen::MatrixXd x_merge_;

    //* state vector: [pos1 pos2 vel_abs yaw_angle yaw_rate] in SI units and rad
    Eigen::MatrixXd x_cv_;
*/

    //* state vector: [pos1 pos2 vel_abs yaw_angle yaw_rate] in SI units and rad
    Eigen::MatrixXd x_ctrv_;

/*
    //* state vector: [pos1 pos2 vel_abs yaw_angle yaw_rate] in SI units and rad
    Eigen::MatrixXd x_rm_;

    //* state covariance matrix
    Eigen::MatrixXd p_merge_;

    //* state covariance matrix
    Eigen::MatrixXd p_cv_;
*/

    //* state covariance matrix
    Eigen::MatrixXd p_ctrv_;

/*
    //* state covariance matrix
    Eigen::MatrixXd p_rm_;

    //* predicted sigma points matrix
    Eigen::MatrixXd x_sig_pred_cv_;
*/

    //* predicted sigma points matrix
    Eigen::MatrixXd x_sig_pred_ctrv_;

/*
    //* predicted sigma points matrix
    Eigen::MatrixXd x_sig_pred_rm_;
*/

    //* time when the state is true, in us
    double time_;

    //* Process noise standard deviation longitudinal acceleration in m/s^2
    double std_a_ctrv_;
/*
    double std_a_cv_;
    double std_a_rm_;
*/

    // CTRV
    double std_ctrv_yawdd_;
/*
    // CV
    double std_cv_yawdd_;

    double std_rm_yawdd_;
*/

    /** Sensor Measurement Noise **/
/*
    double std_laspx_,std_laspy_; // Laser measurement noise standard deviation position1 in m
    double std_imu_yaw;
    double std_gnssraw_v,std_gnssraw_r;
*/


    //* Weights of sigma points
    Eigen::VectorXd weights_c_;
    Eigen::VectorXd weights_s_;

    //* Sigma point spreading parameter
    double lambda_;

/*
    double mode_match_prob_cv2cv_;
    double mode_match_prob_ctrv2cv_;
    double mode_match_prob_rm2cv_;

    double mode_match_prob_cv2ctrv_;
    double mode_match_prob_ctrv2ctrv_;
    double mode_match_prob_rm2ctrv_;

    double mode_match_prob_cv2rm_;
    double mode_match_prob_ctrv2rm_;
    double mode_match_prob_rm2rm_;

    double mode_match_prob_cv_;

    double mode_match_prob_ctrv_;

    double mode_match_prob_rm_;

    double mode_prob_cv_;
    double mode_prob_ctrv_;
    double mode_prob_rm_;
    std::vector<double> p1_;

    std::vector<double> p2_;

    std::vector<double> p3_;

*/

/*
    //// TODO: 5x5次元で予測する　or 次元数だけ予測する
    Eigen::VectorXd z_pred_cv_,z_pred_ctrv_,z_pred_rm_;
    Eigen::MatrixXd s_cv_,s_ctrv_,s_rm_;
    Eigen::MatrixXd k_cv_,k_ctrv_,k_rm_;
*/
    //// TODO: 5x5次元で予測する　or 次元数だけ予測する
    Eigen::VectorXd z_pred_ctrv_;
    Eigen::MatrixXd s_ctrv_;
    Eigen::MatrixXd k_ctrv_;

    //// TODO: 後半を全て消す
/*
    Eigen::VectorXd z_pred_imu_cv_,z_pred_imu_ctrv_,z_pred_imu_rm_;
    Eigen::MatrixXd s_imu_cv_,s_imu_ctrv_,s_imu_rm_;
    Eigen::MatrixXd k_imu_cv_,k_imu_ctrv_,k_imu_rm_;
    Eigen::VectorXd z_pred_gnssraw_cv_,z_pred_gnssraw_ctrv_,z_pred_gnssraw_rm_;
    Eigen::MatrixXd s_gnssraw_cv_,s_gnssraw_ctrv_,s_gnssraw_rm_;
    Eigen::MatrixXd k_gnssraw_cv_,k_gnssraw_ctrv_,k_gnssraw_rm_;
*/

/*
    double pd_;
    double pg_;

    int lifetime_;
    bool is_static_;
*/

    // object msg information
/*
    bool is_stable_;
    autoware_msgs::DetectedObject object_;
    std::string label_;
    double min_assiciation_distance_;
*/

/*
    // for env classification
    Eigen::VectorXd init_meas_;
    std::vector<double> vel_history_;

    double x_merge_yaw_;

    int tracking_num_;
*/

/*
    Eigen::VectorXd cv_meas_;
    Eigen::VectorXd ctrv_meas_;
    Eigen::VectorXd rm_meas_;
*/

    Eigen::MatrixXd q_ctrv_;
/*
    Eigen::MatrixXd q_cv_;
    Eigen::MatrixXd q_rm_;
*/



    //// TODO 5x5軸で生成
    Eigen::MatrixXd r_ctrv_;
/*
    Eigen::MatrixXd r_cv_,r_ctrv_,r_rm_;
    Eigen::MatrixXd r_imu_cv_,r_imu_ctrv_,r_imu_rm_;
    Eigen::MatrixXd r_gnssraw_cv_,r_gnssraw_ctrv_,r_gnssraw_rm_;
*/

/*
    double nis_cv_;
    double nis_ctrv_;
    double nis_rm_;

    Eigen::MatrixXd new_x_sig_cv_;
    Eigen::MatrixXd new_x_sig_ctrv_;
    Eigen::MatrixXd new_x_sig_rm_;

    Eigen::MatrixXd new_z_sig_cv_;
    Eigen::MatrixXd new_z_sig_ctrv_;
    Eigen::MatrixXd new_z_sig_rm_;

    Eigen::VectorXd new_z_pred_cv_;
    Eigen::VectorXd new_z_pred_ctrv_;
    Eigen::VectorXd new_z_pred_rm_;

    Eigen::MatrixXd new_s_cv_;
    Eigen::MatrixXd new_s_ctrv_;
    Eigen::MatrixXd new_s_rm_;
*/

    size_t num_pass=0;

    /**
     * Constructor
     */
    UkfEstimator();

    void updateYawWithHighProb();

    void initialize(const Eigen::VectorXd& z, const double timestamp);

    void updateModeProb(const std::vector<double>& lambda_vec);

    void mergeEstimationAndCovariance();

    void mixingProbability();

    void interaction();

    void predictionIMMUKF(const double dt);

    void findMaxZandS(Eigen::VectorXd& max_det_z, Eigen::MatrixXd& max_det_s);
    void updateEachMotion(const double detection_probability, const double gate_probability, const double gating_thres,
                          const Eigen::VectorXd z, std::vector<double>& lambda_vec);

    /** Prediction **/
    void ctrv(const double p_x, const double p_y, const double v, const double yaw, const double yawd,
              const double delta_t, std::vector<double>& state);
    void cv(const double p_x, const double p_y, const double v, const double yaw, const double yawd, const double delta_t,
            std::vector<double>& state);
    void randomMotion(const double p_x, const double p_y, const double v, const double yaw, const double yawd,
                      const double delta_t, std::vector<double>& state);
    void initCovarQs(const double dt, const double yaw);
    void predictionMotion(const double delta_t, const int model_ind);
    void prediction(const double timestamp);

    void checkLaneDirectionAvailability(const autoware_msgs::DetectedObject& in_object,
                                        const double lane_direction_chi_thres, const bool use_sukf);

    //// TODO: どこまで必要？
    void predictionMeasurement(const int motion_ind,std::vector<int> update_state);
/*
    void predictionLidarMeasurement(const int motion_ind, const int num_meas_state);
    void predictionIMUMeasurement(const int motion_ind);
    void predictionGnssVRMeasurement(const int motion_ind);
    void predictionGnssVMeasurement(const int motion_ind);
*/

    double calculateNIS(const autoware_msgs::DetectedObject& in_object, const int motion_ind);

    bool isLaneDirectionAvailable(const autoware_msgs::DetectedObject& in_object, const int motion_ind,
                                  const double lane_direction_chi_thres);

    // ;
    void updateKalmanGain(const int motion_ind,std::vector<int> update_state);
/*
    void updateKalmanGainIMU(const int motion_ind);
    void updateKalmanGainLidar(const int motion_ind);
*/

    double normalizeAngle(const double angle);

};

#endif /* UKF_ESTIMATOR_H */
