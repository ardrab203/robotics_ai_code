#include <chrono>
#include <memory>
#include <string>
#include <thread>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

#include "qos_utils.hpp"

class QosSubscriber : public rclcpp::Node
{
public:
  QosSubscriber()
  : Node("qos_subscriber")
  {
    const auto topic = declare_parameter("topic", std::string("qos_chatter"));
    delay_ms_ = declare_parameter("process_delay_ms", 0);  // fake "slow work" per message

    const rclcpp::QoS qos = qos_demo::make_qos_from_params(*this);

    rclcpp::SubscriptionOptions options;
    options.event_callbacks.incompatible_qos_callback =
      [this](rclcpp::QOSRequestedIncompatibleQoSInfo & info) {
        RCLCPP_ERROR(
          get_logger(),
          "INCOMPATIBLE QoS! I ask for more than offered. Clashing policy: %s (count=%d)",
          qos_demo::policy_name(info.last_policy_kind), info.total_count);
      };
    options.event_callbacks.deadline_callback =
      [this](rclcpp::QOSDeadlineRequestedInfo & info) {
        RCLCPP_WARN(
          get_logger(), "DEADLINE MISSED: no message in time (total=%d)", info.total_count);
      };

    sub_ = create_subscription<std_msgs::msg::String>(
      topic, qos,
      [this](std_msgs::msg::String::ConstSharedPtr msg) {on_message(msg);},
      options);

    RCLCPP_INFO(get_logger(), "Listening on '%s' with QoS: %s",
      topic.c_str(), qos_demo::describe_qos(qos).c_str());
  }

private:
  void on_message(std_msgs::msg::String::ConstSharedPtr msg)
  {
    // Message text is "Message #N": pull out N so we can spot gaps
    const int idx = std::stoi(msg->data.substr(msg->data.find('#') + 1));
    if (last_idx_ > 0 && idx != last_idx_ + 1) {
      RCLCPP_WARN(get_logger(), "Skipped %d message(s): jumped from #%d to #%d",
        idx - last_idx_ - 1, last_idx_, idx);
    }
    last_idx_ = idx;

    RCLCPP_INFO(get_logger(), "Received: '%s'", msg->data.c_str());

    if (delay_ms_ > 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms_));
    }
  }

  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub_;
  int delay_ms_{0};
  int last_idx_{0};
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<QosSubscriber>());
  rclcpp::shutdown();
  return 0;
}
