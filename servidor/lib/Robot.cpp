#include "Robot.h"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <algorithm>

namespace RPCServer {

bool Robot::connect(const std::string& port, int baud){
    std::lock_guard<std::mutex> lk(ioMutex_);
    if (!serial_.open(port, baud)) return false;

    // Intentar sincronizar: algunos firmwares envían "start" o banner
    auto banner = serial_.readLine(3000);
    // Si no llegó nada, mandar una línea en blanco para despertar y volver a leer
    if (banner.empty()) {
        serial_.writeLine("");
        banner = serial_.readLine(2000);
    }
    // No exigimos texto específico; con la espera tras open suele bastar
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

} // namespace RPCServer