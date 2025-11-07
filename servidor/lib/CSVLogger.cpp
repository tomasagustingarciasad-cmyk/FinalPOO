#include "../inc/CSVLogger.h"
#include <iostream>
#include <filesystem>

// Instancia global del logger
CSVLogger g_logger;

CSVLogger::CSVLogger(const std::string& filePath) : logFilePath(filePath) {
    try {
        // Crear directorio de logs si no existe
        std::filesystem::path logDir = std::filesystem::path(filePath).parent_path();
        if (!logDir.empty()) {
            std::filesystem::create_directories(logDir);
        }
        
        // Verificar si el archivo ya existe para decidir si escribir header
        bool fileExists = std::filesystem::exists(filePath);
        
        logFile.open(filePath, std::ios::app);
        if (logFile.is_open()) {
            if (!fileExists) {
                writeHeader();
            }
            logSystem("CSVLogger", LogLevel::INFO, "Logger inicializado correctamente", 
                     "Archivo: " + filePath);
        } else {
            std::cerr << "Error: No se pudo abrir el archivo de log: " << filePath << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error inicializando logger: " << e.what() << std::endl;
    }
}

CSVLogger::~CSVLogger() {
    if (logFile.is_open()) {
        logSystem("CSVLogger", LogLevel::INFO, "Logger finalizando", "Cerrando archivo de log");
        logFile.close();
    }
}

std::string CSVLogger::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    
    return ss.str();
}

std::string CSVLogger::escapeCSV(const std::string& field) {
    std::string escaped = field;
    
    // Reemplazar comillas dobles por comillas dobles escapadas
    size_t pos = 0;
    while ((pos = escaped.find('"', pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\"\"");
        pos += 2;
    }
    
    // Si contiene comas, saltos de línea o comillas, envolver en comillas
    if (escaped.find(',') != std::string::npos || 
        escaped.find('\n') != std::string::npos || 
        escaped.find('\r') != std::string::npos ||
        escaped.find('"') != std::string::npos) {
        escaped = "\"" + escaped + "\"";
    }
    
    return escaped;
}

void CSVLogger::writeHeader() {
    if (logFile.is_open()) {
        logFile << "timestamp,type,module,level,method,user,client_ip,response_code,message,details\n";
        logFile.flush();
    }
}

void CSVLogger::logRequest(const std::string& method, const std::string& user, 
                          const std::string& clientIP, int responseCode, 
                          const std::string& details) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    if (!logFile.is_open()) return;
    
    try {
        logFile << escapeCSV(getCurrentTimestamp()) << ","
                << escapeCSV("REQUEST") << ","
                << escapeCSV("XMLRPCServer") << ","
                << escapeCSV("INFO") << ","
                << escapeCSV(method) << ","
                << escapeCSV(user) << ","
                << escapeCSV(clientIP) << ","
                << responseCode << ","
                << escapeCSV("Petición procesada") << ","
                << escapeCSV(details) << "\n";
        
        logFile.flush();
    } catch (const std::exception& e) {
        std::cerr << "Error escribiendo log de request: " << e.what() << std::endl;
    }
}

void CSVLogger::logSystem(const std::string& module, LogLevel level, 
                         const std::string& message, const std::string& details) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    if (!logFile.is_open()) return;
    
    std::string levelStr;
    switch (level) {
        case LogLevel::DEBUG: levelStr = "DEBUG"; break;
        case LogLevel::INFO: levelStr = "INFO"; break;
        case LogLevel::WARNING: levelStr = "WARNING"; break;
        case LogLevel::ERROR: levelStr = "ERROR"; break;
        case LogLevel::CRITICAL: levelStr = "CRITICAL"; break;
    }
    
    try {
        logFile << escapeCSV(getCurrentTimestamp()) << ","
                << escapeCSV("SYSTEM") << ","
                << escapeCSV(module) << ","
                << escapeCSV(levelStr) << ","
                << escapeCSV("") << ","  // method (vacío para eventos de sistema)
                << escapeCSV("") << ","  // user (vacío para eventos de sistema)
                << escapeCSV("") << ","  // client_ip (vacío para eventos de sistema)
                << "0,"                  // response_code (0 para eventos de sistema)
                << escapeCSV(message) << ","
                << escapeCSV(details) << "\n";
        
        logFile.flush();
    } catch (const std::exception& e) {
        std::cerr << "Error escribiendo log de sistema: " << e.what() << std::endl;
    }
}

void CSVLogger::logDebug(const std::string& module, const std::string& function, 
                        const std::string& message, const std::string& details) {
    std::string debugDetails = "Function: " + function;
    if (!details.empty()) {
        debugDetails += " | " + details;
    }
    
    logSystem(module, LogLevel::DEBUG, message, debugDetails);
}

void CSVLogger::logError(const std::string& module, const std::string& errorCode, 
                        const std::string& message, const std::string& details) {
    std::string errorDetails = "ErrorCode: " + errorCode;
    if (!details.empty()) {
        errorDetails += " | " + details;
    }
    
    logSystem(module, LogLevel::ERROR, message, errorDetails);
}

void CSVLogger::flush() {
    std::lock_guard<std::mutex> lock(logMutex);
    if (logFile.is_open()) {
        logFile.flush();
    }
}

bool CSVLogger::isOpen() const {
    return logFile.is_open();
}