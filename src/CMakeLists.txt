cmake_minimum_required(VERSION 3.8)
project(p_drive_testing)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(can_msgs REQUIRED)

# Include directories (if you have header files in include/)
include_directories(include)

# Add executable
add_executable(rover_control src/RoverControl.cpp src/RMD_Motor.cpp)


# Link libraries to the executable
ament_target_dependencies(rover_control rclcpp can_msgs)

# Install targets
install(TARGETS
  rover_control
  DESTINATION lib/${PROJECT_NAME}
)

# Enable testing if required
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

# Mark the package
ament_package()

