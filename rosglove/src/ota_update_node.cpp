#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <random>
#include <fstream>
#include <sstream>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_msgs/msg/float32.hpp"
#include "std_srvs/srv/trigger.hpp"

using namespace std::chrono_literals;

// #region agent log
inline void debug_log(const std::string& location, const std::string& message, const std::string& data, const std::string& hypothesis_id) {
  std::ofstream log_file("/workspace/.cursor/debug.log", std::ios::app);
  if (log_file.is_open()) {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    log_file << R"({"sessionId":"debug-session","runId":"run1","hypothesisId":")" << hypothesis_id 
             << R"(","location":")" << location << R"(","message":")" << message 
             << R"(","data":)" << data << R"(,"timestamp":)" << now << "}\n";
    log_file.close();
  }
}
// #endregion

enum class UpdateState {
  IDLE,
  DOWNLOADING,
  INSTALLING,
  COMPLETED,
  FAILED
};

class OTAUpdateNode : public rclcpp::Node
{
public:
  OTAUpdateNode()
  : Node("ota_update_node")
  {
    // #region agent log
    debug_log("ota_update_node.cpp:25", "Constructor entry", "{}", "A");
    // #endregion
    
    // Parameters
    current_version_ = this->declare_parameter<std::string>("current_version", "1.0.0");
    
    // #region agent log
    std::ostringstream oss;
    oss << R"({"current_version":")" << current_version_ << R"("})";
    debug_log("ota_update_node.cpp:30", "Parameter loaded", oss.str(), "A");
    // #endregion
    download_duration_sec_ = this->declare_parameter<double>("download_duration_sec", 10.0);
    install_duration_sec_ = this->declare_parameter<double>("install_duration_sec", 5.0);
    failure_probability_ = this->declare_parameter<double>("failure_probability", 0.1);
    update_rate_hz_ = this->declare_parameter<double>("update_rate_hz", 10.0);

    // Publishers
    status_publisher_ = this->create_publisher<std_msgs::msg::String>("ota/status", 10);
    progress_publisher_ = this->create_publisher<std_msgs::msg::Float32>("ota/progress", 10);
    // Use transient_local QoS so new subscribers (like Foxglove) receive the last published version
    version_publisher_ = this->create_publisher<std_msgs::msg::String>(
      "ota/version", 
      rclcpp::QoS(10).transient_local());
    
    // #region agent log
    debug_log("ota_update_node.cpp:38", "Publishers created", R"({"topic":"ota/version"})", "B");
    // #endregion

    // Service
    update_service_ = this->create_service<std_srvs::srv::Trigger>(
      "ota/start_update",
      std::bind(&OTAUpdateNode::handle_update_request, this,
                std::placeholders::_1, std::placeholders::_2));

    // Timer for update simulation
    update_timer_ = this->create_wall_timer(
      std::chrono::milliseconds(static_cast<int>(1000.0 / update_rate_hz_)),
      std::bind(&OTAUpdateNode::update_callback, this));

    // Status timer for periodic status updates
    status_timer_ = this->create_wall_timer(
      1s,
      std::bind(&OTAUpdateNode::publish_status, this));

    // Initialize state
    state_ = UpdateState::IDLE;
    progress_ = 0.0f;
    update_start_time_ = this->now();

    // Random number generator for failures
    rng_.seed(std::chrono::steady_clock::now().time_since_epoch().count());

    RCLCPP_INFO(this->get_logger(), "OTA Update Node started. Current version: %s", current_version_.c_str());
    
    // #region agent log
    debug_log("ota_update_node.cpp:64", "About to call publish_version", R"({"version":")" + current_version_ + R"("})", "E");
    // #endregion
    
    publish_version();
    
    // #region agent log
    debug_log("ota_update_node.cpp:66", "Constructor completed", "{}", "A");
    // #endregion
  }

private:
  void handle_update_request(
    const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response)
  {
    (void)request;  // Unused parameter

    if (state_ != UpdateState::IDLE) {
      response->success = false;
      response->message = "Update already in progress or system busy";
      RCLCPP_WARN(this->get_logger(), "Update request rejected: system not idle");
      return;
    }

    // Generate new version (increment patch version)
    std::string new_version = generate_next_version(current_version_);
    
    RCLCPP_INFO(this->get_logger(), "Starting OTA update from %s to %s", 
                current_version_.c_str(), new_version.c_str());
    
    target_version_ = new_version;
    state_ = UpdateState::DOWNLOADING;
    progress_ = 0.0f;
    update_start_time_ = this->now();
    download_start_time_ = this->now();

    response->success = true;
    response->message = "OTA update started: " + current_version_ + " -> " + target_version_;
  }

  void update_callback()
  {
    if (state_ == UpdateState::IDLE) {
      return;
    }

    auto now = this->now();
    // double elapsed = (now - update_start_time_).seconds();

    if (state_ == UpdateState::DOWNLOADING) {
      double download_elapsed = (now - download_start_time_).seconds();
      progress_ = static_cast<float>(download_elapsed / download_duration_sec_ * 100.0);
      
      if (progress_ >= 100.0f) {
        progress_ = 100.0f;
        RCLCPP_INFO(this->get_logger(), "Download completed. Starting installation...");
        state_ = UpdateState::INSTALLING;
        install_start_time_ = now;
        progress_ = 0.0f;
      }
    } else if (state_ == UpdateState::INSTALLING) {
      double install_elapsed = (now - install_start_time_).seconds();
      progress_ = static_cast<float>(install_elapsed / install_duration_sec_ * 100.0);
      
      if (progress_ >= 100.0f) {
        progress_ = 100.0f;
        
        // Simulate potential failure
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        if (dist(rng_) < failure_probability_) {
          RCLCPP_ERROR(this->get_logger(), "Update installation failed!");
          state_ = UpdateState::FAILED;
        } else {
          RCLCPP_INFO(this->get_logger(), "Update installation completed successfully!");
          current_version_ = target_version_;
          state_ = UpdateState::COMPLETED;
          publish_version();
        }
        
        // Reset after a delay
        reset_timer_ = this->create_wall_timer(
          3s,
          [this]() {
            if (state_ == UpdateState::COMPLETED || state_ == UpdateState::FAILED) {
              state_ = UpdateState::IDLE;
              progress_ = 0.0f;
              reset_timer_->cancel();
              RCLCPP_INFO(this->get_logger(), "System ready for next update");
            }
          });
      }
    }
  }

  void publish_status()
  {
    std_msgs::msg::String status_msg;
    std_msgs::msg::Float32 progress_msg;

    switch (state_) {
      case UpdateState::IDLE:
        status_msg.data = "IDLE";
        progress_msg.data = 0.0f;
        break;
      case UpdateState::DOWNLOADING:
        status_msg.data = "DOWNLOADING";
        progress_msg.data = progress_;
        break;
      case UpdateState::INSTALLING:
        status_msg.data = "INSTALLING";
        progress_msg.data = progress_;
        break;
      case UpdateState::COMPLETED:
        status_msg.data = "COMPLETED";
        progress_msg.data = 100.0f;
        break;
      case UpdateState::FAILED:
        status_msg.data = "FAILED";
        progress_msg.data = progress_;
        break;
    }

    status_publisher_->publish(status_msg);
    progress_publisher_->publish(progress_msg);
  }

  void publish_version()
  {
    // #region agent log
    std::ostringstream oss;
    oss << R"({"version":")" << current_version_ << R"(","publisher_null":)" << (version_publisher_ == nullptr ? "true" : "false") << "}";
    debug_log("ota_update_node.cpp:184", "publish_version entry", oss.str(), "B");
    // #endregion
    
    std_msgs::msg::String version_msg;
    version_msg.data = current_version_;
    
    // #region agent log
    debug_log("ota_update_node.cpp:189", "Before publish call", R"({"msg_data":")" + current_version_ + R"("})", "B");
    // #endregion
    
    version_publisher_->publish(version_msg);
    
    // #region agent log
    debug_log("ota_update_node.cpp:191", "After publish call", "{}", "B");
    // #endregion
  }

  std::string generate_next_version(const std::string& current)
  {
    // Simple version increment: increment patch version
    size_t last_dot = current.find_last_of('.');
    if (last_dot != std::string::npos) {
      int patch = std::stoi(current.substr(last_dot + 1));
      return current.substr(0, last_dot + 1) + std::to_string(patch + 1);
    }
    return current + ".1";
  }

  // Parameters
  std::string current_version_;
  std::string target_version_;
  double download_duration_sec_;
  double install_duration_sec_;
  double failure_probability_;
  double update_rate_hz_;

  // State
  UpdateState state_;
  float progress_;
  rclcpp::Time update_start_time_;
  rclcpp::Time download_start_time_;
  rclcpp::Time install_start_time_;

  // ROS interfaces
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_publisher_;
  rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr progress_publisher_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr version_publisher_;
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr update_service_;
  rclcpp::TimerBase::SharedPtr update_timer_;
  rclcpp::TimerBase::SharedPtr status_timer_;
  rclcpp::TimerBase::SharedPtr reset_timer_;

  // Random number generator
  std::mt19937 rng_;
};

int main(int argc, char * argv[])
{
  // #region agent log
  debug_log("ota_update_node.cpp:232", "main entry", "{}", "A");
  // #endregion
  
  rclcpp::init(argc, argv);
  
  // #region agent log
  debug_log("ota_update_node.cpp:235", "rclcpp initialized", "{}", "A");
  // #endregion
  
  rclcpp::spin(std::make_shared<OTAUpdateNode>());
  
  // #region agent log
  debug_log("ota_update_node.cpp:237", "spin completed", "{}", "A");
  // #endregion
  
  rclcpp::shutdown();
  return 0;
}
