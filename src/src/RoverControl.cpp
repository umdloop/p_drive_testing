#include <memory>
#include <chrono>
#include <iostream>
#include <thread>
#include <unistd.h>
#include <termios.h>
#include "rclcpp/rclcpp.hpp"
#include "RMD_Motor.h"

using namespace std::chrono_literals;

class RoverControl : public rclcpp::Node {
public:
    RoverControl() : Node("rover_control"), distance_covered_(0.0), moving_(false) {
        // Set up motors directly in the constructor
        auto m1 = std::make_shared<Motor>(145);
        auto m2 = std::make_shared<Motor>(143);
        auto m3 = std::make_shared<Motor>(142);
        auto m4 = std::make_shared<Motor>(146);

        auto left_motors = std::make_shared<MotorGroup>(std::vector<Motor*>{m1.get(), m2.get()});
        auto right_motors = std::make_shared<MotorGroup>(std::vector<Motor*>{m3.get(), m4.get()});

        rover_ = std::make_shared<RoverMotors>(left_motors.get(), right_motors.get(), 1.0);

        if (!rover_) {
            RCLCPP_ERROR(this->get_logger(), "Failed to initialize rover.");
            return;
        }

        // Disengage brakes before starting
        rover_->brakes_off();

        // Configure terminal for non-blocking input
        configureTerminal();

        // Set the timer to periodically call the function to check for input
        timer_ = this->create_wall_timer(
            100ms, std::bind(&RoverControl::keyControl, this)
        );
    }

    ~RoverControl() {
        // Restore terminal settings before exiting
        restoreTerminal();
    }

private:
    void keyControl() {
        char input = readKey();
        if (input == 'w' || input == 'W') {
            if (!moving_) {
                startMoving();
            }
        } else if (input == 'b' || input == 'B') {
            stopMoving();
        }

        // If moving, update the distance covered
        if (moving_) {
            auto current_time = std::chrono::steady_clock::now();
            std::chrono::duration<float> elapsed_time = current_time - start_time_;

            // Speed is 0.5 m/s
            distance_covered_ = speed_ * elapsed_time.count();

            // Check if distance target is reached
            if (distance_covered_ >= 40.0) {
                RCLCPP_INFO(this->get_logger(), "Reached 40 meters. Stopping...");
                stopMoving();
            }
        }
    }

    void startMoving() {
        moving_ = true;
        speed_ = 0.5; // Speed in meters per second
        start_time_ = std::chrono::steady_clock::now();
        distance_covered_ = 0.0;

        rover_->brakes_off();
        rover_->set_speed(speed_);
        RCLCPP_INFO(this->get_logger(), "Moving forward at speed: %f m/s", speed_);
    }

    void stopMoving() {
        if (moving_) {
            moving_ = false;
            rover_->stop();
            rover_->brakes_on();
            distance_covered_ = 0.0;
            RCLCPP_INFO(this->get_logger(), "Braking and stopping the rover.");
        }
    }

    char readKey() {
        char buf = 0;
        struct timeval tv = {0, 0}; // Non-blocking read
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        if (select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv) > 0) {
            read(STDIN_FILENO, &buf, 1);
        }
        return buf;
    }

    void configureTerminal() {
        tcgetattr(STDIN_FILENO, &original_terminal_);
        termios new_terminal = original_terminal_;
        new_terminal.c_lflag &= ~(ICANON | ECHO); 
        tcsetattr(STDIN_FILENO, TCSANOW, &new_terminal);
    }

    void restoreTerminal() {
        tcsetattr(STDIN_FILENO, TCSANOW, &original_terminal_);
    }

    std::shared_ptr<RoverMotors> rover_;
    rclcpp::TimerBase::SharedPtr timer_;
    bool moving_;
    float speed_;
    float distance_covered_;
    std::chrono::steady_clock::time_point start_time_;
    termios original_terminal_;
};

int main(int argc, char *argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<RoverControl>());
    rclcpp::shutdown();
    return 0;
}
