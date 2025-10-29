#pragma once
#include <string>
#include <mutex>
#include "SerialPort.h"

namespace RPCServer {
class Robot {
    SerialPort serial_;
    bool manual_ = true;
    bool absolute_ = true;
    bool motorsOn_ = false;
    std::mutex ioMutex_;
public:
    bool connect(const std::string& port, int baud);
    void disconnect();
    bool isConnected() const;
    bool setMode(bool manual, bool absolute); // manual se deja por compatibilidad
    bool enableMotors(bool on);
    bool home();
    bool move(double x, double y, double z, double vel);
    bool endEffector(bool on);
    bool sendGcodeCommand(const std::string& command);
private:
    bool sendAndWaitOk(const std::string& line, int timeoutMs = 5000);
    std::string readLine(int timeoutMs = 500);
};
} // namespace RPCServer.