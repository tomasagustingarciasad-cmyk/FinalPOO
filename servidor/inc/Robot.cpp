#include "Robot.h"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <algorithm>

namespace RPCServer {

bool Robot::connect(const std::string& port, int baud){
    std::lock_guard<std::mutex> lk(ioMutex_);
    
    LOG_SYSTEM("Robot", "Intentando conectar robot: " + port + " Baud: " + std::to_string(baud));
    
    if (!serial_.open(port, baud)) {
        LOG_ERROR("Robot", "Error abriendo puerto serie: " + port);
        return false;
    }

    LOG_DEBUG("Robot", "Puerto serie abierto exitosamente", "Esperando banner/sincronización");

    // Intentar sincronizar: algunos firmwares envían "start" o banner
    auto banner = serial_.readLine(3000);
    LOG_DEBUG("Robot", "Banner recibido: '" + banner + "'");
    
    // Si no llegó nada, mandar una línea en blanco para despertar y volver a leer
    if (banner.empty()) {
        LOG_DEBUG("Robot", "Banner vacío, enviando línea en blanco");
        serial_.writeLine("");
        banner = serial_.readLine(2000);
        LOG_DEBUG("Robot", "Segundo intento de banner: '" + banner + "'");
    }
    
    LOG_SYSTEM("Robot", "Robot conectado exitosamente en puerto: " + port);

    // No exigimos texto específico; con la espera tras open suele bastar
    return true;
}

void Robot::disconnect(){
    std::lock_guard<std::mutex> lk(ioMutex_);
    LOG_SYSTEM("Robot", "Desconectando robot");
    serial_.close();
    LOG_SYSTEM("Robot", "Robot desconectado");
}

bool Robot::isConnected() const { return serial_.isOpen(); }

std::string Robot::readLine(int timeoutMs) {
    return serial_.readLine(timeoutMs);
}

bool Robot::sendAndWaitOk(const std::string& line, int timeoutMs){
    std::lock_guard<std::mutex> lk(ioMutex_);
    
    LOG_DEBUG("Robot", "Enviando comando: '" + line + "' (Timeout: " + std::to_string(timeoutMs) + "ms)");
    
    if (!serial_.writeLine(line)) {
        LOG_ERROR("Robot", "Error escribiendo comando al puerto serie: " + line);
        return false;
    }

    int elapsed = 0;
    while (elapsed <= timeoutMs) {
        auto resp = serial_.readLine(500);
        elapsed += 500;
        if (resp.empty()) continue;

        LOG_DEBUG("Robot", "Respuesta recibida: '" + resp + "' (Elapsed: " + std::to_string(elapsed) + "ms)");

        // Normalizar a minúsculas para buscar "ok"
        std::string low = resp;
        std::transform(low.begin(), low.end(), low.begin(), ::tolower);

        if (low.find("ok") != std::string::npos) {
            LOG_DEBUG("Robot", "Comando ejecutado exitosamente (Tiempo: " + std::to_string(elapsed) + "ms)");
            return true;
        }
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
    return sendAndWaitOk("G28", 15000); // homing puede tardar más
}

bool Robot::move(double x, double y, double z, double vel){
    std::ostringstream ss; ss << std::fixed << std::setprecision(3);
    ss << "G0 X" << x << " Y" << y << " Z" << z;
    if (vel > 0) ss << " F" << vel;
    
    bool success = sendAndWaitOk(ss.str(), 15000);
    
    // Actualizar posición si el tracking está habilitado y el movimiento fue exitoso
    if (success) {
        getCurrentPosition();
    }
    
    return success;
}

bool Robot::endEffector(bool on){
    // Ajusta a tu efector real si no es ventilador
    bool success = sendAndWaitOk(on ? "M3" : "M5", 5000);
    
    // Actualizar estado del efector si el tracking está habilitado
    if (success && positionTracking_) {
        setEndEffectorState(on);
    }
    
    return success;
}

bool Robot::sendGcodeCommand(const std::string& command){
    // Método público para enviar comandos G-code directamente
    return sendAndWaitOk(command, 15000);
}

// Implementación de métodos de tracking
Position Robot::getCurrentPosition() const {
    // Attempt to refresh position from robot before returning cached value.
    // We need to cast away const-ness to call the non-const query method.
    Robot* self = const_cast<Robot*>(this);
    // Best-effort: if querying fails or times out, return last known position.
    // Use default (increased) timeout to allow slow devices to respond.
    self->queryCurrentPosition(15000);

    std::lock_guard<std::mutex> lk(positionMutex_);
    return currentPosition_;
}

bool Robot::queryCurrentPosition(int timeoutMs){
    std::lock_guard<std::mutex> lk(ioMutex_);

    LOG_DEBUG("Robot", "Consultando posición actual con M114");

    if (!serial_.writeLine("M114")){
        LOG_ERROR("Robot", "Error escribiendo M114");
        return false;
    }

    int elapsed = 0;
    std::string positionLine;
    const int perReadMs = 1000; // aumentar espera por lectura para dispositivos lentos
    std::string recvBuf; // acumulador para fragmentos parciales

    double tx = 0.0, ty = 0.0, tz = 0.0, te = 0.0;
    bool gotX = false, gotY = false, gotZ = false, gotE = false;

    auto trim = [](const std::string &s) {
        size_t b = s.find_first_not_of(" \t\r\n");
        if (b == std::string::npos) return std::string();
        size_t e = s.find_last_not_of(" \t\r\n");
        return s.substr(b, e - b + 1);
    };

    while (elapsed <= timeoutMs){
        auto line = serial_.readLine(perReadMs);
        elapsed += perReadMs;
        if (line.empty()) continue;

        // Log raw fragment separately to help debugging
        LOG_DEBUG("Robot", "Fragmento recibido (raw)");
        LOG_DEBUG("Robot", "  >> '" + line + "'");

        // Detectar respuestas de error que empiezan con "ERROR" (case-insensitive) en el fragmento
        std::string fragUpper = line;
        std::transform(fragUpper.begin(), fragUpper.end(), fragUpper.begin(), ::toupper);
        if (fragUpper.rfind("ERROR", 0) == 0) {
            LOG_ERROR("Robot", "Respuesta de error al consultar posicion: '" + line + "'");
            return false;
        }
        // Si recibimos un 'ok' podemos salir: el dispositivo terminó su reporte
        if (fragUpper.find("OK") != std::string::npos) {
            LOG_DEBUG("Robot", "Se recibió OK durante consulta de posición, saliendo del bucle");
            break;
        }

        // Acumular fragmento
        if (!recvBuf.empty()) recvBuf.push_back('\n');
        recvBuf.append(line);

        // Intentar detectar bloque con corchetes (formato antiguo)
        auto infoPos = recvBuf.find("INFO:CURRENT POSITION");
        if (infoPos == std::string::npos) infoPos = recvBuf.find("CURRENT POSITION");
        if (infoPos != std::string::npos){
            auto lbr = recvBuf.find('[', infoPos);
            auto rbr = recvBuf.find(']', infoPos);
            if (lbr != std::string::npos && rbr != std::string::npos && rbr > lbr){
                positionLine = recvBuf.substr(infoPos, rbr - infoPos + 1);
                break;
            }
        }

        // Parsear líneas individuales: admitir "INFO: X: 100.00" o "X:100.00"
        {
            std::string t = trim(line);
            if (!t.empty()){
                // Quitar prefijo INFO: si existe
                std::string upper = t;
                std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
                if (upper.rfind("INFO:", 0) == 0) {
                    t = trim(t.substr(5));
                }

                // Buscar token KEY:VALUE
                auto colon = t.find(':');
                if (colon != std::string::npos && colon > 0){
                    std::string key = t.substr(0, colon);
                    std::string val = trim(t.substr(colon + 1));
                    try {
                        double v = std::stod(val);
                        if (key == "X" || key == "x") { tx = v; gotX = true; }
                        else if (key == "Y" || key == "y") { ty = v; gotY = true; }
                        else if (key == "Z" || key == "z") { tz = v; gotZ = true; }
                        else if (key == "E" || key == "e") { te = v; gotE = true; }
                    } catch (...) {
                        // ignore parse errors
                    }
                }
            }
        }

        if (recvBuf.size() > 8192) recvBuf.erase(0, recvBuf.size() - 4096);
    }

    // Si recibimos bloque con corchetes, parsearlo
    if (!positionLine.empty()){
        auto lpos = positionLine.find('[');
        auto rpos = positionLine.find(']');
        if (lpos == std::string::npos || rpos == std::string::npos || rpos <= lpos){
            LOG_ERROR("Robot", "Formato inesperado de linea de posicion: '" + positionLine + "'");
            return false;
        }

        std::string inside = positionLine.substr(lpos + 1, rpos - lpos - 1);
        for (auto &c: inside) if (c == ',') c = ' ';

        std::istringstream iss(inside);
        std::string token;
        while (iss >> token){
            auto colon = token.find(':');
            if (colon == std::string::npos) continue;
            std::string key = token.substr(0, colon);
            std::string val = token.substr(colon + 1);
            try {
                double v = std::stod(val);
                if (key == "X") { tx = v; gotX = true; }
                else if (key == "Y") { ty = v; gotY = true; }
                else if (key == "Z") { tz = v; gotZ = true; }
                else if (key == "E") { te = v; gotE = true; }
            } catch (...) { }
        }
    }

    if (!(gotX || gotY || gotZ || gotE)){
        LOG_DEBUG("Robot", "No se recibieron coordenadas válidas dentro del timeout");
        return false;
    }

    std::lock_guard<std::mutex> plk(positionMutex_);
    if (gotX) currentPosition_.x = tx;
    if (gotY) currentPosition_.y = ty;
    if (gotZ) currentPosition_.z = tz;
    if (gotE) currentPosition_.feedrate = te; // usar feedrate para E si no hay campo separado

    LOG_DEBUG("Robot", "Posicion actualizada desde M114: X=" + std::to_string(currentPosition_.x)
              + " Y=" + std::to_string(currentPosition_.y) + " Z=" + std::to_string(currentPosition_.z)
              + " E=" + std::to_string(te));

    return true;
}

bool Robot::updateCurrentPosition(double x, double y, double z, double feedrate) {
    std::lock_guard<std::mutex> lk(positionMutex_);
    currentPosition_.x = x;
    currentPosition_.y = y;
    currentPosition_.z = z;
    if (feedrate > 0) {
        currentPosition_.feedrate = feedrate;
    }
    return true;
}

void Robot::setEndEffectorState(bool active) {
    std::lock_guard<std::mutex> lk(positionMutex_);
    currentPosition_.endEffectorActive = active;
}

bool Robot::getGripperOn() const {
    std::lock_guard<std::mutex> lk(positionMutex_);
    return currentPosition_.endEffectorActive;
}

} // namespace RPCServer