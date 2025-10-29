#pragma once
#include <string>
#include <vector>
#include <mutex>
#include "SerialPort.h"

namespace RPCServer {

// Estructura para respuesta de posición (M114)
struct RobotPosition {
    bool valid = false;
    std::string mode; // "ABSOLUTE" o "RELATIVE"
    double x = 0.0, y = 0.0, z = 0.0, e = 0.0;
    bool motorsEnabled = false;
    bool fanEnabled = false;
    std::vector<std::string> rawLines; // líneas originales para debug
};

// Estructura para respuesta de endstops (M119)
struct EndstopStatus {
    bool valid = false;
    int xState = 0, yState = 0, zState = 0;
    std::vector<std::string> rawLines;
};

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
    
    // Nuevos métodos para consulta de estado (respuestas multilínea)
    RobotPosition getPosition();
    EndstopStatus getEndstops();
    
private:
    // Método original (compatible con código existente)
    bool sendAndWaitOk(const std::string& line, int timeoutMs = 5000);
    
    // Nuevos métodos según filosofía del profesor
    void discardInitialBanner(int timeoutMs = 3000);
    std::vector<std::string> readMultiLineResponse(int timeoutMs = 2000, int idleMs = 300);
    bool sendCommand(const std::string& line);
    std::string readLine(int timeoutMs = 500);
};
} // namespace RPCServer.