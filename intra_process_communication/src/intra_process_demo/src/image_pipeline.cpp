#include <algorithm>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"

using namespace std::chrono_literals;
using Image = sensor_msgs::msg::Image;

// ---------- Desk 1: creates the image ----------
class Camera : public rclcpp::Node
{
public:
  explicit Camera(const rclcpp::NodeOptions & options)
  : Node("camera", options)
  {
    pub_ = create_publisher<Image>("image_raw", 10);

    timer_ = create_wall_timer(33ms, [this]() {
      auto img = std::make_unique<Image>();
      img->header.stamp = now();
      img->header.frame_id = "camera";
      img->width = 1280;
      img->height = 720;
      img->encoding = "rgb8";
      img->step = 1280 * 3;
      img->data.assign(1280 * 720 * 3, 100);

      if (++count_ % 30 == 0) {
        RCLCPP_INFO(get_logger(), "[camera] frame %d  data @ %p",
          count_, static_cast<void *>(img->data.data()));
      }
      pub_->publish(std::move(img));
    });
  }

private:
  rclcpp::Publisher<Image>::SharedPtr pub_;
  rclcpp::TimerBase::SharedPtr timer_;
  int count_{0};
};

// ---------- Desk 2: edits the image in place ----------
class Filter : public rclcpp::Node
{
public:
  explicit Filter(const rclcpp::NodeOptions & options)
  : Node("filter", options)
  {
    pub_ = create_publisher<Image>("image_filtered", 10);

    sub_ = create_subscription<Image>(
      "image_raw", 10,
      [this](Image::UniquePtr msg) {
        // We OWN this file now, so we can edit it directly
        for (auto & px : msg->data) {
          px = static_cast<uint8_t>(std::min(255, px + 20));
        }

        if (++count_ % 30 == 0) {
          RCLCPP_INFO(get_logger(), "[filter] frame %d  data @ %p",
            count_, static_cast<void *>(msg->data.data()));
        }
        pub_->publish(std::move(msg));   // pass the same file onward
      });
  }

private:
  rclcpp::Publisher<Image>::SharedPtr pub_;
  rclcpp::Subscription<Image>::SharedPtr sub_;
  int count_{0};
};

// ---------- Desk 3: measures the trip ----------
class Viewer : public rclcpp::Node
{
public:
  explicit Viewer(const rclcpp::NodeOptions & options)
  : Node("viewer", options)
  {
    sub_ = create_subscription<Image>(
      "image_filtered", 10,
      [this](Image::UniquePtr msg) {
        total_ms_ += (now() - rclcpp::Time(msg->header.stamp)).seconds() * 1000.0;

        if (++count_ % 30 == 0) {
          RCLCPP_INFO(get_logger(),
            "[viewer] frame %d  data @ %p | avg latency: %.2f ms",
            count_, static_cast<void *>(msg->data.data()), total_ms_ / 30.0);
          total_ms_ = 0.0;
        }
      });
  }

private:
  rclcpp::Subscription<Image>::SharedPtr sub_;
  int count_{0};
  double total_ms_{0.0};
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  const bool use_ipc = (argc > 1 && std::string(argv[1]) == "ipc");
  rclcpp::NodeOptions options;
  options.use_intra_process_comms(use_ipc);

  auto camera = std::make_shared<Camera>(options);
  auto filter = std::make_shared<Filter>(options);
  auto viewer = std::make_shared<Viewer>(options);

  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(camera);
  executor.add_node(filter);
  executor.add_node(viewer);

  RCLCPP_INFO(camera->get_logger(), "Intra-process: %s", use_ipc ? "ON" : "OFF");
  executor.spin();

  rclcpp::shutdown();
  return 0;
}