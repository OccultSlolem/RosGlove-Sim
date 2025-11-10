#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <random>
#include <cmath>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_msgs/msg/float32.hpp"
#include "sensor_msgs/msg/imu.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "tf2/LinearMath/Quaternion.h"
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"
#include "tf2_ros/transform_broadcaster.h"


using namespace std::chrono_literals;

#define BATTERY_DEPLETION_INTERVAL 1000ms // Time interval for battery depletion
#define BATTERY_DEPLETION_RATE 0.1f      // Battery depletion rate per interval

class RobotSimulator : public rclcpp::Node
{
public:
  RobotSimulator()
  : Node("robot_simulator")
  {
    // Differential drive parameters
    wheel_radius_ = this->declare_parameter<double>("wheel_radius", 0.033);
    wheel_separation_ = this->declare_parameter<double>("wheel_separation", 0.16);
    update_rate_hz_ = this->declare_parameter<double>("update_rate_hz", 50.0);
    cmd_timeout_sec_ = this->declare_parameter<double>("cmd_timeout_sec", 1.0);
    lin_noise_stddev_ = this->declare_parameter<double>("lin_noise_stddev", 0.01);
    ang_noise_stddev_ = this->declare_parameter<double>("ang_noise_stddev", 0.01);
    base_frame_ = this->declare_parameter<std::string>("base_frame", "base_link");
    odom_frame_ = this->declare_parameter<std::string>("odom_frame", "odom");
    imu_frame_ = this->declare_parameter<std::string>("imu_frame", "imu");

    // Battery/temp parameters
    battery_start_ = this->declare_parameter<float>("battery_start", 100.0f);
    battery_depletion_per_sec_ = this->declare_parameter<float>("battery_depletion_per_sec", 0.1f);
    base_temperature_c = this->declare_parameter<float>("base_temperature_c", 20.0f);
    temperature_jitter_c_ = this->declare_parameter<float>("temperature_jitter_c", 5.0f);

    // Publishers
    odom_publisher_ = this->create_publisher<nav_msgs::msg::Odometry>("odom", 10);
    battery_publisher_ = this->create_publisher<std_msgs::msg::Float32>("battery", 10);
    temperature_publisher_ = this->create_publisher<std_msgs::msg::Float32>("temperature", 10);
    imu_publisher_ = this->create_publisher<sensor_msgs::msg::Imu>("imu", 10);
    joint_publisher_ = this->create_publisher<sensor_msgs::msg::JointState>("joint_states", 10);
    
    // TF
    tf_broadcaster_ = std::make_shared<tf2_ros::TransformBroadcaster>(this);

    // Subscriber (so we can keep the timings updated)
    cmd_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
      "cmd_vel", 10,
      [this](geometry_msgs::msg::Twist::ConstSharedPtr msg) {
        last_cmd_ = *msg;
        last_cmd_time_ = this->now();
      });

    // Timers
    const auto period = std::chrono::duration<double>(1.0 / update_rate_hz_);
    timer_ = this->create_wall_timer(
      std::chrono::duration_cast<std::chrono::nanoseconds>(period),
      std::bind(&RobotSimulator::onUpdate, this)
    );

    // Noise RNG
    rng_.seed(std::random_device{}());
    lin_noise_ = std::normal_distribution<double>(0.0, lin_noise_stddev_);
    ang_noise_ = std::normal_distribution<double>(0.0, ang_noise_stddev_);
    temp_noise_ = std::normal_distribution<double>(0.0, temperature_jitter_c_);

    // Initialize state

    x_ = y_ = yaw_ = 0.0;
    wl_pos_ = wr_pos_ = 0.0;
    wl_vel_ = wr_vel_ = 0.0;
    last_update_time_ = this->now();
    last_cmd_time_ = this->now();
    battery_percent_ = battery_start_;
  }

private:
  void onUpdate()
  {
    const auto now = this->now();
    const double dt = (now - last_cmd_time_).seconds();
    if (dt <= 0.0) return;
    last_update_time_ = now;

    // Timeout stale commands
    geometry_msgs::msg::Twist cmd = last_cmd_;
    if ((now - last_cmd_time_).seconds() > cmd_timeout_sec_) {
      cmd.linear.x = 0.0;
      cmd.angular.z = 0.0;
    }

    // Kinematics integration
    double v = cmd.linear.x + lin_noise_(rng_);
    double w = cmd.angular.z + ang_noise_(rng_);

    x_ += v * std::cos(yaw_) * dt;
    y_ += v * std::sin(yaw_) * dt;
    yaw_ += w * dt;

    // Constrain yaw to [-pi, pi]
    yaw_ = std::atan2(std::sin(yaw_), std::cos(yaw_));

    // Wheel angular and linear velocities
    const double v_l = v - w * wheel_separation_ / 2.0;
    const double v_r = v + w * wheel_separation_ / 2.0;
    wl_vel_ = v_l / wheel_radius_;
    wr_vel_ = v_r / wheel_radius_;
    wl_pos_ += wl_vel_ * dt;
    wr_pos_ += wr_vel_ * dt;

    publishOdomTF(now, v, w);
    publishImu(now, w);
    publishJointStates(now);
    publish_battery_and_temperature(dt);

  }

  void publishOdomTF(const rclcpp::Time& stamp, double v, double w)
  {
    nav_msgs::msg::Odometry odom;
    odom.header.stamp = stamp;
    odom.header.frame_id = odom_frame_;
    odom.child_frame_id = base_frame_;
    odom.pose.pose.position.x = x_;
    odom.pose.pose.position.y = y_;
    odom.pose.pose.position.z = 0.0;

    tf2::Quaternion q;
    q.setRPY(0.0, 0.0, yaw_);
    odom.pose.pose.orientation = tf2::toMsg(q);

    odom.twist.twist.linear.x = v;
    odom.twist.twist.angular.z = w;

    // Light covariances
    odom.pose.covariance[0] = 1e-3;
    odom.pose.covariance[7] = 1e-3;
    odom.pose.covariance[35] = 1e-2;
    odom.twist.covariance[0] = 1e-2;
    odom.twist.covariance[35] = 1e-2;

    odom_publisher_->publish(odom);

    // TF: odom -> base_link
    geometry_msgs::msg::TransformStamped tf;
    tf.header.stamp = stamp;
    tf.header.frame_id = odom_frame_;
    tf.child_frame_id = base_frame_;
    tf.transform.translation.x = x_;
    tf.transform.translation.y = y_;
    tf.transform.translation.z = 0.0;
    tf.transform.rotation = odom.pose.pose.orientation;
    tf_broadcaster_->sendTransform(tf);
  }

  void publishImu(const rclcpp::Time& stamp, double w)
  {
    sensor_msgs::msg::Imu imu;
    imu.header.stamp = stamp;
    imu.header.frame_id = imu_frame_;

    tf2::Quaternion q;
    q.setRPY(0.0, 0.0, yaw_);
    imu.orientation = tf2::toMsg(q);

    imu.angular_velocity.z = w;
    imu.linear_acceleration.x = 0.0;
    imu.linear_acceleration.y = 0.0;
    imu.linear_acceleration.z = 9.81; // Constant of gravity

    imu_publisher_->publish(imu);
  }

  void publishJointStates(const rclcpp::Time& stamp)
  {
    sensor_msgs::msg::JointState js;
    js.header.stamp = stamp;
    js.name = {"wheel_joint_left", "wheel_joint_right"};
    js.position = {wl_pos_, wr_pos_};
    js.velocity = {wl_vel_, wr_vel_};
    joint_publisher_->publish(js);
  }

  void publish_battery_and_temperature(double dt) 
  {
    if (battery_percent_ > 0.0) {
      battery_percent_ = std::max(0.0, battery_percent_ - battery_depletion_per_sec_ * dt);
    }

    std_msgs::msg::Float32 b;
    b.data = static_cast<float>(battery_percent_);
    battery_publisher_->publish(b);

    std_msgs::msg::Float32 t;
    t.data = static_cast<float>(base_temperature_c + temp_noise_(rng_));
    temperature_publisher_->publish(t);
  }

  double wheel_radius_;
  double wheel_separation_;
  double update_rate_hz_;
  double cmd_timeout_sec_;
  double lin_noise_stddev_;
  double ang_noise_stddev_;
  std::string base_frame_, odom_frame_, imu_frame_;

  double battery_start_;
  double battery_depletion_per_sec_;
  float battery_level_;
  double base_temperature_c;
  double temperature_jitter_c_;
  double battery_percent_;

  // State
  double x_{0}, y_{0}, yaw_{0};
  double wl_pos_{0}, wr_pos_{0};
  double wl_vel_{0}, wr_vel_{0};

  // ROS2 interfaces
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odom_publisher_;
  rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr battery_publisher_;
  rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr temperature_publisher_;
  rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_publisher_;
  rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_publisher_;
  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_sub_;
  std::shared_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_;

  // Timing
  rclcpp::Time last_update_time_;
  rclcpp::Time last_cmd_time_;
  geometry_msgs::msg::Twist last_cmd_;

  // Noise
  std::mt19937 rng_;
  std::normal_distribution<double> lin_noise_;
  std::normal_distribution<double> ang_noise_;
  std::normal_distribution<double> temp_noise_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<RobotSimulator>());
  rclcpp::shutdown();
  return 0;
}

