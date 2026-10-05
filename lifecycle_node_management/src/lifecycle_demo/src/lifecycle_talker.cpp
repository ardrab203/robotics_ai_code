#include <chrono>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"
#include "rclcpp_lifecycle/lifecycle_publisher.hpp"
#include "std_msgs/msg/string.hpp"

using CallbackReturn =
  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

class LifecycleTalker : public rclcpp_lifecycle::LifecycleNode
{
public:
  explicit LifecycleTalker(const std::string & node_name)
  : rclcpp_lifecycle::LifecycleNode(node_name)
  {
    // Parameters = settings on the wall. Safe to declare early.
    declare_parameter<double>("publish_rate_hz", 1.0);
    declare_parameter<std::string>("simulate", "none");  // none|failure|error

    RCLCPP_INFO(get_logger(), "[constructor] Building is up (unconfigured)");
  }

  // configure: hire staff, stock the kitchen (allocate, but do NOT run yet)
  CallbackReturn on_configure(const rclcpp_lifecycle::State &) override
  {
    RCLCPP_INFO(get_logger(), "[on_configure] Hiring staff, stocking...");

    rate_hz_ = get_parameter("publish_rate_hz").as_double();
    const auto simulate = get_parameter("simulate").as_string();

    if (rate_hz_ <= 0.0 || simulate == "failure") {
      RCLCPP_WARN(get_logger(), "[on_configure] Not ready -> FAILURE");
      return CallbackReturn::FAILURE;   // stay in unconfigured
    }

    pub_ = create_publisher<std_msgs::msg::String>("chatter", 10);
    count_ = 0;
    return CallbackReturn::SUCCESS;     // -> inactive
  }

  // activate: flip the sign to OPEN (start the real work)
  CallbackReturn on_activate(const rclcpp_lifecycle::State &) override
  {
    RCLCPP_INFO(get_logger(), "[on_activate] Sign flipped to OPEN");

    if (get_parameter("simulate").as_string() == "error") {
      RCLCPP_ERROR(get_logger(), "[on_activate] Kitchen fire! -> ERROR");
      return CallbackReturn::ERROR;     // -> ErrorProcessing -> on_error
    }

    pub_->on_activate();                // publisher now really sends

    const auto period =
      std::chrono::milliseconds(static_cast<int>(1000.0 / rate_hz_));
    timer_ = create_wall_timer(period, [this]() {
      std_msgs::msg::String msg;
      msg.data = "Order #" + std::to_string(++count_) + " is ready!";
      RCLCPP_INFO(get_logger(), "Serving: '%s'", msg.data.c_str());
      pub_->publish(msg);
    });
    return CallbackReturn::SUCCESS;     // -> active
  }

  // deactivate: flip the sign to CLOSED (staff and kitchen stay)
  CallbackReturn on_deactivate(const rclcpp_lifecycle::State &) override
  {
    RCLCPP_INFO(get_logger(), "[on_deactivate] Sign flipped to CLOSED");
    timer_.reset();
    pub_->on_deactivate();
    return CallbackReturn::SUCCESS;     // -> inactive
  }

  // cleanup: send staff home, empty the kitchen (undo on_configure)
  CallbackReturn on_cleanup(const rclcpp_lifecycle::State &) override
  {
    RCLCPP_INFO(get_logger(), "[on_cleanup] Emptying the kitchen");
    pub_.reset();
    return CallbackReturn::SUCCESS;     // -> unconfigured
  }

  // shutdown: close for good
  CallbackReturn on_shutdown(const rclcpp_lifecycle::State & state) override
  {
    RCLCPP_INFO(get_logger(), "[on_shutdown] Closing for good (was: %s)",
      state.label().c_str());
    timer_.reset();
    pub_.reset();
    return CallbackReturn::SUCCESS;     // -> finalized
  }

  // error: fire brigade
  CallbackReturn on_error(const rclcpp_lifecycle::State &) override
  {
    RCLCPP_ERROR(get_logger(), "[on_error] Fire brigade! Resetting");
    timer_.reset();
    pub_.reset();
    return CallbackReturn::SUCCESS;     // SUCCESS -> unconfigured, else finalized
  }

private:
  rclcpp_lifecycle::LifecyclePublisher<std_msgs::msg::String>::SharedPtr pub_;
  rclcpp::TimerBase::SharedPtr timer_;
  double rate_hz_{1.0};
  size_t count_{0};
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<LifecycleTalker>("lifecycle_talker");
  // LifecycleNode is not an rclcpp::Node, so spin its base interface
  rclcpp::spin(node->get_node_base_interface());
  rclcpp::shutdown();
  return 0;
}