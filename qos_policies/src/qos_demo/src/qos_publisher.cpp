#include <chrono>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

#include "qos_utils.hpp"

class QosPublisher : public rclcpp::Node
{
public:
  QosPublisher()
  : Node("qos_publisher")
  {
    const auto topic = declare_parameter("topic", std::string("qos_chatter"));
    const auto rate_hz = declare_parameter("rate_hz", 2.0);
    max_msgs_ = declare_parameter("max_msgs", 0);  // 0 = publish forever

    // Build the QoS from parameters (reliability, durability, history, depth, deadline_ms)
    const rclcpp::QoS qos = qos_demo::make_qos_from_params(*this);

    // QoS events: the middleware tells us when something goes wrong
    rclcpp::PublisherOptions options;
    options.event_callbacks.incompatible_qos_callback =
      [this](rclcpp::QOSOfferedIncompatibleQoSInfo & info) {
        RCLCPP_ERROR(
          get_logger(),
          "INCOMPATIBLE QoS! A subscriber wants more than I offer. Policy: %s (count=%d)",
          qos_demo::policy_name(info.last_policy_kind), info.total_count);
      };
    options.event_callbacks.deadline_callback =
      [this](rclcpp::QOSDeadlineOfferedInfo & info) {
        RCLCPP_WARN(get_logger(), "DEADLINE MISSED: I was too slow (total=%d)", info.total_count);
      };

    pub_ = create_publisher<std_msgs::msg::String>(topic, qos, options);

    timer_ = create_wall_timer(
      std::chrono::duration<double>(1.0 / rate_hz), [this]() {tick();});

    RCLCPP_INFO(get_logger(), "Publishing on '%s' with QoS: %s",
      topic.c_str(), qos_demo::describe_qos(qos).c_str());
  }

private:
  void tick()
  {
    if (max_msgs_ > 0 && count_ >= max_msgs_) {
      if (!done_logged_) {
        RCLCPP_INFO(get_logger(), "Sent %d messages. Going quiet (node stays alive).", count_);
        done_logged_ = true;
      }
      return;
    }
    std_msgs::msg::String msg;
    msg.data = "Message #" + std::to_string(++count_);
    pub_->publish(msg);
    RCLCPP_INFO(get_logger(), "Published: '%s'", msg.data.c_str());
  }

  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_;
  rclcpp::TimerBase::SharedPtr timer_;
  int max_msgs_{0};
  int count_{0};
  bool done_logged_{false};
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<QosPublisher>());
  rclcpp::shutdown();
  return 0;
}
