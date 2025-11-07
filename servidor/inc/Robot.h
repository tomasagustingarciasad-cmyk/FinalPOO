#pragma once
#include <string>
#include <mutex>
#include "SerialPort.h"
#include "CSVLogger.h"

namespace RPCServer {

struct Position {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double feedrate = 1000.0;
    bool endEffectorActive = false;
};

class Robot {
    SerialPort serial_;
    bool manual_ = true;
    bool absolute_ = true;
    bool motorsOn_ = false;
    std::mutex ioMutex_;
    
    // Tracking de posición
    Position currentPosition_;
    bool positionTracking_ = false;
    mutable std::mutex positionMutex_;  // mutable para permitir lock en métodos const
    
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
    
    // Nuevos métodos para tracking de posición
    Position getCurrentPosition() const;
    bool updateCurrentPosition(double x, double y, double z, double feedrate = 1000.0);
    void setEndEffectorState(bool active);
    bool isPositionTrackingEnabled() const { return positionTracking_; }
    void enablePositionTracking(bool enable) { positionTracking_ = enable; }
private:
    bool sendAndWaitOk(const std::string& line, int timeoutMs = 5000);
    std::string readLine(int timeoutMs = 500);
};
} // namespace RPCServer.