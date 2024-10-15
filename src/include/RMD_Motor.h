#ifndef RMD_MOTOR_H
#define RMD_MOTOR_H

#include <iostream>
#include <cstring>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

class Motor {
public:
    Motor(int id, bool debug = false);
    ~Motor();

    void spin(float speed_mps);
    void brakes_on();
    void brakes_off();
    void shutdown();
    void stop();

private:
    int id_;
    int can_socket_;
    bool brakes_engaged_;
    bool debug_;

    void setupCanSocket();
    void sendCommand(const std::vector<uint8_t>& command);
    void sendFrame(const struct can_frame& frame);
};

class MotorGroup {
public:
    MotorGroup(std::vector<Motor*> motors, bool debug = false);
    void set_speeds(float speed_mps);
    void brakes_on();
    void brakes_off();
    void shutdown();
    void stop();

private:
    std::vector<Motor*> motors_;
    bool brakes_engaged_;
    bool debug_;
};

class RoverMotors {
public:
    RoverMotors(MotorGroup* left_motors, MotorGroup* right_motors, float limit = 1.0f, bool debug = false);
    void set_speed(float speed_mps);
    void brakes_on();
    void brakes_off();
    void shutdown();
    void stop();

private:
    MotorGroup* left_motors_;
    MotorGroup* right_motors_;
    float limit_;
    bool debug_;
    bool brakes_engaged_;
};

#endif // RMD_MOTOR_H 
