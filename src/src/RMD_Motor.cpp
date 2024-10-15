#include "RMD_Motor.h"
#include <iostream>
#include <cstring>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

Motor::Motor(int id, bool debug) : id_(id), brakes_engaged_(true), debug_(debug) {
    setupCanSocket();
}

Motor::~Motor() {
    close(can_socket_);
}

void Motor::spin(float speed_mps) {
    if (brakes_engaged_) {
        std::cout << "Disengage brakes first. Use brakes_off()" << std::endl;
        return;
    }

    struct can_frame frame;
    frame.can_id = id_;
    frame.can_dlc = 8;
    frame.data[0] = 0xA2;
    frame.data[1] = 0x00;
    frame.data[2] = 0x00;
    frame.data[3] = 0x00;

    int vel = static_cast<int>(speed_mps * 1091 * 36);
    frame.data[4] = vel & 0xFF;
    frame.data[5] = (vel >> 8) & 0xFF;
    frame.data[6] = (vel >> 16) & 0xFF;
    frame.data[7] = (vel >> 24) & 0xFF;

    sendFrame(frame);
}

void Motor::brakes_on() {
    sendCommand({0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
    brakes_engaged_ = true;
}

void Motor::brakes_off() {
    sendCommand({0x77, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
    brakes_engaged_ = false;
}

void Motor::shutdown() {
    sendCommand({0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
}

void Motor::stop() {
    sendCommand({0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
}

void Motor::setupCanSocket() {
    can_socket_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    struct ifreq ifr;
    strcpy(ifr.ifr_name, "can0");
    ioctl(can_socket_, SIOCGIFINDEX, &ifr);

    struct sockaddr_can addr;
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    bind(can_socket_, (struct sockaddr *)&addr, sizeof(addr));
}

void Motor::sendCommand(const std::vector<uint8_t>& command) {
    struct can_frame frame;
    frame.can_id = id_;
    frame.can_dlc = command.size();
    std::copy(command.begin(), command.end(), frame.data);

    sendFrame(frame);
}

void Motor::sendFrame(const struct can_frame& frame) {
    if (debug_) {
        std::cout << "Sending CAN frame: ID=0x" << std::hex << frame.can_id << " Data=";
        for (int i = 0; i < frame.can_dlc; ++i) {
            std::cout << std::hex << static_cast<int>(frame.data[i]) << " ";
        }
        std::cout << std::dec << std::endl;
    } else {
        write(can_socket_, &frame, sizeof(frame));
    }
}

MotorGroup::MotorGroup(std::vector<Motor*> motors, bool debug)
    : motors_(motors), debug_(debug), brakes_engaged_(true) {}

void MotorGroup::set_speeds(float speed_mps) {
    if (brakes_engaged_) {
        std::cout << "Disengage brakes first. Use brakes_off()" << std::endl;
        return;
    }
    for (auto& motor : motors_) {
        motor->spin(speed_mps);
    }
}

void MotorGroup::brakes_on() {
    for (auto& motor : motors_) {
        motor->brakes_on();
    }
    brakes_engaged_ = true;
}

void MotorGroup::brakes_off() {
    for (auto& motor : motors_) {
        motor->brakes_off();
    }
    brakes_engaged_ = false;
}

void MotorGroup::shutdown() {
    for (auto& motor : motors_) {
        motor->shutdown();
    }
}

void MotorGroup::stop() {
    for (auto& motor : motors_) {
        motor->stop();
    }
}

RoverMotors::RoverMotors(MotorGroup* left_motors, MotorGroup* right_motors, float limit, bool debug)
    : left_motors_(left_motors), right_motors_(right_motors), limit_(limit), debug_(debug), brakes_engaged_(true) {}

void RoverMotors::set_speed(float speed_mps) {
    if (brakes_engaged_) {
        std::cout << "Disengage brakes first. Use brakes_off()" << std::endl;
        return;
    } else if (speed_mps > limit_) {
        std::cout << "Speed limit is: " << limit_ << std::endl;
        return;
    }
    left_motors_->set_speeds(speed_mps);
    right_motors_->set_speeds(-speed_mps);
}

void RoverMotors::brakes_on() {
    left_motors_->brakes_on();
    right_motors_->brakes_on();
    brakes_engaged_ = true;
}

void RoverMotors::brakes_off() {
    left_motors_->brakes_off();
    right_motors_->brakes_off();
    brakes_engaged_ = false;
}

void RoverMotors::shutdown() {
    left_motors_->shutdown();
    right_motors_->shutdown();
}

void RoverMotors::stop() {
    left_motors_->stop();
    right_motors_->stop();
}