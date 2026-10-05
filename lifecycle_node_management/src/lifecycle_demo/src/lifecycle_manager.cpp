#include <chrono>
#include <memory>
#include <string>
#include <thread>

#include "lifecycle_msgs/msg/transition.hpp"
#include "lifecycle_msgs/srv/change_state.hpp"
#include "lifecycle_msgs/srv/get_state.hpp"
#include "rclcpp/rclcpp.hpp"

using namespace std::chrono_literals;
using ChangeState = lifecycle_msgs::srv::ChangeState;
using GetState = lifecycle_msgs::srv::GetState;
using Transition = lifecycle_msgs::msg::Transition;

class LifecycleManager : public rclcpp::Node
{
public:
  LifecycleManager()
  : Node("lifecycle_manager")
  {
    change_client_ =
      create_client<ChangeState>("/lifecycle_talker/change_state");
    get_client_ = create_client<GetState>("/lifecycle_talker/get_state");
  }

  bool wait_for_services()
  {
    return change_client_->wait_for_service(10s) &&
           get_client_->wait_for_service(10s);
  }

  std::string get_state()
  {
    auto future =
      get_client_->async_send_request(std::make_shared<GetState::Request>());
    if (rclcpp::spin_until_future_complete(shared_from_this(), future, 5s) !=
      rclcpp::FutureReturnCode::SUCCESS)
    {
      return "unknown";
    }
    return future.get()->current_state.label;
  }

  bool change_state(uint8_t transition_id)
  {
    auto request = std::make_shared<ChangeState::Request>();
    request->transition.id = transition_id;
    auto future = change_client_->async_send_request(request);
    if (rclcpp::spin_until_future_complete(shared_from_this(), future, 10s) !=
      rclcpp::FutureReturnCode::SUCCESS)
    {
      RCLCPP_ERROR(get_logger(), "Service call timed out");
      return false;
    }
    return future.get()->success;
  }

  void step(const std::string & name, uint8_t transition_id)
  {
    RCLCPP_INFO(get_logger(), ">>> %s (state now: %s)",
      name.c_str(), get_state().c_str());
    const bool ok = change_state(transition_id);
    RCLCPP_INFO(get_logger(), "    %s | state is now: %s",
      ok ? "OK" : "REFUSED", get_state().c_str());
  }

private:
  rclcpp::Client<ChangeState>::SharedPtr change_client_;
  rclcpp::Client<GetState>::SharedPtr get_client_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto manager = std::make_shared<LifecycleManager>();

  if (!manager->wait_for_services()) {
    RCLCPP_ERROR(manager->get_logger(), "lifecycle_talker not found. Running?");
    rclcpp::shutdown();
    return 1;
  }

  manager->step("configure", Transition::TRANSITION_CONFIGURE);
  std::this_thread::sleep_for(2s);
  manager->step("activate", Transition::TRANSITION_ACTIVATE);
  std::this_thread::sleep_for(6s);          // the restaurant is open
  manager->step("deactivate", Transition::TRANSITION_DEACTIVATE);
  std::this_thread::sleep_for(2s);
  manager->step("cleanup", Transition::TRANSITION_CLEANUP);
  manager->step("shutdown", Transition::TRANSITION_UNCONFIGURED_SHUTDOWN);

  rclcpp::shutdown();
  return 0;
}