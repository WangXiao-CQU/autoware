#include <iostream>
#include <vector>
#include <chrono>

#include <ros/ros.h>
#include <geometry_msgs/PoseArray.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/PoseWithCovarianceStamped.h>
#include <geometry_msgs/TwistStamped.h>
#include <sensor_msgs/Imu.h>
#include <std_msgs/Float64.h>
#include <std_msgs/Float64MultiArray.h>
#include <tf/transform_datatypes.h>
#include <tf2/utils.h>

#include <autoware_msgs/VehicleStatus.h>
#include <tf/transform_broadcaster.h>

#include "kalman_filter/kalman_filter.h"
#include "kalman_filter/kalman_filter_delayed_measurement.h"

class KalmanFilterNode
{
public:
  KalmanFilterNode();
  ~KalmanFilterNode();

private:
  ros::NodeHandle nh_, pnh_;
  ros::Publisher pub_pose_, pub_pose_cov_, pub_twist_, pub_debug_, pub_ndt_pose_;
  ros::Subscriber sub_ndt_pose_, sub_vehicle_status_, sub_imu_;
  ros::Timer timer_control_, timer_tf_;
  tf::TransformBroadcaster tf_br_;


  KalmanFilterDelayedMeasurement kf_;

  /* parameters */
  bool show_debug_info_;
  double kf_rate_;          // kalman filter predict rate
  double kf_dt_;            // = 1 / kf_rate_
  double tf_rate_;          // tf publish rate
  double wheelbase_;        // to convert steering angle to angular velocity

  int dim_x_;               // dimension of kalman state
  int extend_state_step_;   // for time delay compensation
  int dim_x_ex_;            // dimension of extended kalman state (dim_x_ * extended_state_step)

  /* NDT */
  double ndt_additional_delay_; // compensated ndt delay time = (ndt.header.stamp - now) + additional_delay [s]
  double ndt_measure_uncertainty_time_; // added for measurement covariance
  double ndt_rate_;             // ndt rate [s], used for covariance calculation
  double ndt_gate_dist_;        // ndt measurement is ignored if the maharanobis distance is larger than this value.
  double ndt_stddev_x_;
  double ndt_stddev_y_;
  double ndt_stddev_yaw_;

  /* twist */
  double twist_additional_delay_;
  double twist_rate_;
  double twist_gate_dist_;
  double twist_stddev_vx_;
  double twist_stddev_wz_;

  /* process noise variance for discrete model */
  double cov_proc_yaw_d_;      // discrete yaw process noise
  double cov_proc_yaw_bias_d_; // discrete yaw bias process noise
  double cov_vx_d_;


  enum  IDX {
    X = 0,
    Y = 1,
    YAW = 2,
    YAWB = 3,
    VX = 4,
    WZ = 5,
  };

  /* for model prediction */
  // autoware_msgs::VehicleStatus current_vehicle_status_;
  std::shared_ptr<geometry_msgs::TwistStamped> current_twist_ptr_;
  std::shared_ptr<geometry_msgs::PoseStamped> current_ndt_pose_ptr_;
  std::shared_ptr<sensor_msgs::Imu> current_imu_ptr_;
  geometry_msgs::PoseStamped current_kf_pose_;
  geometry_msgs::TwistStamped current_kf_twist_;

  void timerCallback(const ros::TimerEvent &e);
  void timerTFCallback(const ros::TimerEvent &e);
  void callbackNDTPose(const geometry_msgs::PoseStamped::ConstPtr &msg);
  void callbackVehicleStatus(const autoware_msgs::VehicleStatus &msg);
  void callbackIMU(const sensor_msgs::Imu::ConstPtr &msg);
  void callbackTwist(const geometry_msgs::TwistStamped::ConstPtr &msg);

  void initKalmanFilter();
  void predictKinematicsModel();
  void measurementUpdateNDTPose(const geometry_msgs::PoseStamped &ndt_pose);
  void measurementUpdateIMU(const sensor_msgs::Imu &msg);
  void measurementUpdateTwist(const geometry_msgs::TwistStamped &twist);

  int getDelayStep(const double &delay_time);
  void setCurrentResult();
  void publishEstimatedPose();
};
