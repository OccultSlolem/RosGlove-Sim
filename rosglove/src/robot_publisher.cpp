#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_msgs/msg/float32.hpp"
#include "sensor_msgs/msg/imu.hpp"

using namespace std::chrono_literals;

#define BATTERY_DEPLETION_INTERVAL 1000ms // Time interval for battery depletion
#define BATTERY_DEPLETION_RATE 0.1f      // Battery depletion rate per interval

class RobotSimulator : public rclcpp::Node
{
public:
  RobotSimulator()
  : Node("robot_simulator"), battery_level_(100.0f), temperature_(20.0f)
  {
    battery_publisher_ = this->create_publisher<std_msgs::msg::Float32>("battery", 10);
    temperature_publisher_ = this->create_publisher<std_msgs::msg::Float32>("temperature", 10);
    imu_publisher_ = this->create_publisher<sensor_msgs::msg::Imu>("imu", 10); 
    timer_ = this->create_wall_timer(
      BATTERY_DEPLETION_INTERVAL, std::bind(&RobotSimulator::timer_callback, this));

  }

private:
  void timer_callback()
  {
    if (battery_level_ > 0.0f) {
      auto battery_level_msg = std_msgs::msg::Float32();
      battery_level_msg.data = battery_level_;
      RCLCPP_INFO(this->get_logger(), "Publishing battery level: %.2f%%", battery_level_msg.data);
      battery_publisher_->publish(battery_level_msg);
      battery_level_ -= BATTERY_DEPLETION_RATE;
    } else {
      RCLCPP_INFO(this->get_logger(), "Battery depleted! Stopping simulation.");
      timer_->cancel();
    }

    auto temp_msg = std_msgs::msg::Float32();
    temp_msg.data = temperature_ + (rand() % 10 - 5); // Simulated temperature range
    RCLCPP_INFO(this->get_logger(), "Publishing battery temp: %.2f°C", temp_msg.data);
    temperature_publisher_->publish(temp_msg);
  }

  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr battery_publisher_;
  rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr temperature_publisher_;
  rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr imu_publisher_;
  float battery_level_;
  float temperature_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<RobotSimulator>());
  rclcpp::shutdown();
  return 0;
}

