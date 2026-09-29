#pragma once

#include <string>

#include "rclcpp/rclcpp.hpp"

namespace qos_demo
{

// Declares the QoS-related ROS parameters and builds an rclcpp::QoS from them.
// This lets us change QoS from the command line (-p name:=value) without recompiling.
inline rclcpp::QoS make_qos_from_params(rclcpp::Node & node)
{
  const auto reliability = node.declare_parameter("reliability", std::string("reliable"));
  const auto durability = node.declare_parameter("durability", std::string("volatile"));
  const auto history = node.declare_parameter("history", std::string("keep_last"));
  const auto depth = node.declare_parameter("depth", 10);
  const auto deadline_ms = node.declare_parameter("deadline_ms", 0);

  // 1) History + depth
  rclcpp::QoS qos = (history == "keep_all") ?
    rclcpp::QoS(rclcpp::KeepAll()) :
    rclcpp::QoS(rclcpp::KeepLast(static_cast<size_t>(depth)));

  // 2) Reliability
  if (reliability == "best_effort") {
    qos.best_effort();
  } else {
    qos.reliable();
  }

  // 3) Durability
  if (durability == "transient_local") {
    qos.transient_local();
  } else {
    qos.durability_volatile();
  }

  // 4) Deadline (0 = no deadline)
  if (deadline_ms > 0) {
    qos.deadline(rclcpp::Duration::from_seconds(deadline_ms / 1000.0));
  }
  return qos;
}

// Human-readable one-liner, e.g. "RELIABLE | VOLATILE | KEEP_LAST(10) | deadline=none"
inline std::string describe_qos(const rclcpp::QoS & qos)
{
  const auto & p = qos.get_rmw_qos_profile();
  std::string s;
  s += (p.reliability == RMW_QOS_POLICY_RELIABILITY_RELIABLE) ? "RELIABLE" : "BEST_EFFORT";
  s += " | ";
  s += (p.durability == RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL) ?
    "TRANSIENT_LOCAL" : "VOLATILE";
  s += " | ";
  s += (p.history == RMW_QOS_POLICY_HISTORY_KEEP_ALL) ?
    std::string("KEEP_ALL") : "KEEP_LAST(" + std::to_string(p.depth) + ")";
  s += " | deadline=";
  if (p.deadline.sec == 0 && p.deadline.nsec == 0) {
    s += "none";
  } else {
    s += std::to_string(p.deadline.sec * 1000 + p.deadline.nsec / 1000000) + "ms";
  }
  return s;
}

// Turns the "which policy clashed?" enum from an incompatible-QoS event into text.
inline const char * policy_name(rmw_qos_policy_kind_t kind)
{
  switch (kind) {
    case RMW_QOS_POLICY_RELIABILITY: return "RELIABILITY";
    case RMW_QOS_POLICY_DURABILITY: return "DURABILITY";
    case RMW_QOS_POLICY_DEADLINE: return "DEADLINE";
    case RMW_QOS_POLICY_LIVELINESS: return "LIVELINESS";
    case RMW_QOS_POLICY_HISTORY: return "HISTORY";
    case RMW_QOS_POLICY_LIFESPAN: return "LIFESPAN";
    default: return "OTHER";
  }
}

}  // namespace qos_demo
