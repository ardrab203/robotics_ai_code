cmake_minimum_required(VERSION 3.8)
project(intra_process_demo)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)

add_executable(address_demo src/address_demo.cpp)
ament_target_dependencies(address_demo rclcpp std_msgs)

add_executable(image_pipeline src/image_pipeline.cpp)
ament_target_dependencies(image_pipeline rclcpp sensor_msgs)

install(TARGETS address_demo image_pipeline
  DESTINATION lib/${PROJECT_NAME})

ament_package()
