#include "Robot.h"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <algorithm>
#include <iostream>

namespace RPCServer {

bool Robot::connect(const std::string& port, int baud){
    std::lock_guard<std::mutex> lk(ioMutex_);
    if (!serial_.open(port, baud)) return false;

    // Lazo de espera para descartar mensajes iniciales (banner, INFO: ROBOT ONLINE, etc.)
    discardInitialBanner(3000);
    
    return true;
}

void Robot::disconnect(){
    std::lock_guard<std::mutex> lk(ioMutex_);
    serial_.close();
}

bool Robot::isConnected() const { return serial_.isOpen(); }

std::string Robot::readLine(int timeoutMs) {
    return serial_.readLine(timeoutMs);
}

// Nuevo: descarta todas las líneas recibidas durante un periodo (banner inicial)
void Robot::discardInitialBanner(int timeoutMs) {
    // No bloqueamos ioMutex_ aquí porque connect() ya lo tiene
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - start).count() < timeoutMs) {
        auto line = serial_.readLine(200);
        // Simplemente descartar sin imprimir
    }
}

// Nuevo: lee múltiples líneas hasta que no hay más actividad (para M114, M119, etc.)
std::vector<std::string> Robot::readMultiLineResponse(int timeoutMs, int idleWindowMs) {
    std::vector<std::string> lines;
    auto startTime = std::chrono::steady_clock::now();
    auto lastLineTime = startTime;
    
    while (true) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
        auto idleTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastLineTime).count();
        
        if (elapsed >= timeoutMs) {
            break;
        }
        
        if (idleTime >= idleWindowMs && !lines.empty()) {
            break;
        }
        
        std::string line = serial_.readLine(100);
        if (!line.empty()) {
            // Descartar líneas "OK" como pidió el profesor
            if (line == "OK") {
                lastLineTime = now;
                continue;
            }
            
            lines.push_back(line);
            lastLineTime = now;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    return lines;
}

// Nuevo: envía comando sin esperar respuesta
bool Robot::sendCommand(const std::string& line) {
    return serial_.writeLine(line);
}

// Método original: mantiene compatibilidad con código existente
bool Robot::sendAndWaitOk(const std::string& line, int timeoutMs){
    std::lock_guard<std::mutex> lk(ioMutex_);
    if (!serial_.writeLine(line)) return false;

    int elapsed = 0;
    while (elapsed <= timeoutMs) {
        auto resp = serial_.readLine(500);
        elapsed += 500;
        if (resp.empty()) continue;

        // Normalizar a minúsculas para buscar "ok"
        std::string low = resp;
        std::transform(low.begin(), low.end(), low.begin(), ::tolower);

        if (low.find("ok") != std::string::npos) return true;
        if (low.rfind("error", 0) == 0) return false; // empieza con "error"
        // Algunos firmwares pueden responder info/posición; seguimos leyendo
    }
    return false; // timeout
}

bool Robot::setMode(bool /*manual*/, bool absolute){
    if (absolute_ != absolute){
        absolute_ = absolute;
        return sendAndWaitOk(absolute_ ? "G90" : "G91", 3000);
    }
    return true;
}

bool Robot::enableMotors(bool on){
    motorsOn_ = on;
    return sendAndWaitOk(on ? "M17" : "M18", 3000);
}

bool Robot::home(){
    return sendAndWaitOk("G28", 8000); // homing puede tardar más
}

bool Robot::move(double x, double y, double z, double vel){
    std::ostringstream ss; ss << std::fixed << std::setprecision(3);
    ss << "G0 X" << x << " Y" << y << " Z" << z;
    if (vel > 0) ss << " F" << vel;
    return sendAndWaitOk(ss.str(), 8000);
}

bool Robot::endEffector(bool on){
    // Ajusta a tu efector real si no es ventilador
    return sendAndWaitOk(on ? "M106" : "M107", 3000);
}

// Nuevo: obtener posición del robot (M114) - respuesta multilínea
RobotPosition Robot::getPosition() {
    std::lock_guard<std::mutex> lk(ioMutex_);
    RobotPosition result;
    
    if (!sendCommand("M114")) return result;
    
    auto lines = readMultiLineResponse(4000, 600);
    result.rawLines = lines;
    
    // Concatenar todas las líneas en un solo string para buscar patrones
    std::string allText;
    for (const auto& line : lines) {
        allText += line + " ";
    }
    
    // Parsear líneas típicas:
    // INFO: ABSOLUTE MODE / RELATIVE MODE
    // INFO: CURRENT POSITION: [X:100.0 Y:120.0 Z:50.0 E:0.0]
    // INFO: MOTORS ENABLED/DISABLED
    // INFO: FAN ENABLED/DISABLED
    
    std::string allLower = allText;
    std::transform(allLower.begin(), allLower.end(), allLower.begin(), ::tolower);
    
    // Buscar modo
    if (allLower.find("absolute mode") != std::string::npos) {
        result.mode = "ABSOLUTE";
    } else if (allLower.find("relative mode") != std::string::npos) {
        result.mode = "RELATIVE";
    }
    
    // Buscar posición - buscar en el texto concatenado
    size_t xpos = allText.find("X:");
    size_t ypos = allText.find("Y:");
    size_t zpos = allText.find("Z:");
    size_t epos = allText.find("E:");
    
    if (xpos != std::string::npos && ypos != std::string::npos && 
        zpos != std::string::npos && epos != std::string::npos) {
        try {
            result.x = std::stod(allText.substr(xpos + 2));
            result.y = std::stod(allText.substr(ypos + 2));
            result.z = std::stod(allText.substr(zpos + 2));
            result.e = std::stod(allText.substr(epos + 2));
            result.valid = true;
        } catch (const std::exception&) {
            // Error parseando, mantener valid=false
        }
    } else {
        // Si no hay coordenadas, aún es válido si tenemos modo/motores/fan
        if (!result.mode.empty()) {
            result.valid = true;  // Válido aunque las coords estén en 0
        }
    }
    
    // Buscar estado de motores
    if (allLower.find("motors enabled") != std::string::npos) {
        result.motorsEnabled = true;
    } else if (allLower.find("motors disabled") != std::string::npos) {
        result.motorsEnabled = false;
    }
    
    // Buscar estado de fan
    if (allLower.find("fan enabled") != std::string::npos) {
        result.fanEnabled = true;
    } else if (allLower.find("fan disabled") != std::string::npos) {
        result.fanEnabled = false;
    }
    
    return result;
}

// Nuevo: obtener estado de endstops (M119) - respuesta 1 línea
EndstopStatus Robot::getEndstops() {
    std::lock_guard<std::mutex> lk(ioMutex_);
    EndstopStatus result;
    
    if (!sendCommand("M119")) return result;
    
    auto lines = readMultiLineResponse(1500, 300);
    result.rawLines = lines;
    
    // Parsear: INFO: ENDSTOP: [X:0 Y:1 Z:0]
    for (const auto& line : lines) {
        std::string low = line;
        std::transform(low.begin(), low.end(), low.begin(), ::tolower);
        
        if (low.find("endstop") != std::string::npos) {
            // Buscar X:, Y:, Z:
            size_t xpos = line.find("X:");
            size_t ypos = line.find("Y:");
            size_t zpos = line.find("Z:");
            
            if (xpos != std::string::npos) {
                result.xState = std::stoi(line.substr(xpos + 2));
            }
            if (ypos != std::string::npos) {
                result.yState = std::stoi(line.substr(ypos + 2));
            }
            if (zpos != std::string::npos) {
                result.zState = std::stoi(line.substr(zpos + 2));
            }
            result.valid = true;
            break;
        }
    }
    
    return result;
}

} // namespace RPCServer