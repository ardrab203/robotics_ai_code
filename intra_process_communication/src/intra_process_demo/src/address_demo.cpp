#include <chrono>
#include <memory>
#include <string>
#include <utility>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/int32.hpp"

using namespace std::chrono_literals;

class Producer : public rclcpp::Node
{
public:
  explicit Producer(const rclcpp::NodeOptions & options)
  : Node("producer", options)
  {
    pub_ = create_publisher<std_msgs::msg::Int32>("number", 10);

    timer_ = create_wall_timer(1s, [this]() {
      auto msg = std::make_unique<std_msgs::msg::Int32>();
      msg->data = ++count_;

      // Print the address BEFORE we give the file away
      RCLCPP_INFO(get_logger(), "Published %d  | address: %p",
        msg->data, static_cast<void *>(msg.get()));

      pub_->publish(std::move(msg));   // hand over the original
    });
  }

private:
  rclcpp::Publisher<std_msgs::msg::Int32>::SharedPtr pub_;
  rclcpp::TimerBase::SharedPtr timer_;
  int count_{0};
};

class Consumer : public rclcpp::Node
{
public:
  explicit Consumer(const rclcpp::NodeOptions & options)
  : Node("consumer", options)
  {
    sub_ = create_subscription<std_msgs::msg::Int32>(
      "number", 10,
      [this](std_msgs::msg::Int32::UniquePtr msg) {
        RCLCPP_INFO(get_logger(), "Received  %d  | address: %p",
          msg->data, static_cast<void *>(msg.get()));
      });
  }

private:
  rclcpp::Subscription<std_msgs::msg::Int32>::SharedPtr sub_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  // usage: address_demo ipc | address_demo no_ipc
  const bool use_ipc = (argc > 1 && std::string(argv[1]) == "ipc");

  rclcpp::NodeOptions options;
  options.use_intra_process_comms(use_ipc);   // THE switch

  auto producer = std::make_shared<Producer>(options);
  auto consumer = std::make_shared<Consumer>(options);

  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(producer);
  executor.add_node(consumer);

  RCLCPP_INFO(producer->get_logger(), "Intra-process: %s", use_ipc ? "ON" : "OFF");
  executor.spin();

  rclcpp::shutdown();
  return 0;
}