#ifndef CSVLOGGER_H
#define CSVLOGGER_H

#include <string>
#include <fstream>
#include <chrono>
#include <mutex>
#include <iomanip>
#include <sstream>

enum class LogType {
    REQUEST,     // Peticiones al servidor
    SYSTEM,      // Eventos del sistema
    ERROR,       // Errores
    DEBUG        // Debug info
};

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

class CSVLogger {
private:
    std::string logFilePath;
    std::ofstream logFile;
    std::mutex logMutex;
    
    std::string getCurrentTimestamp();
    std::string escapeCSV(const std::string& field);
    void writeHeader();
    
public:
    CSVLogger(const std::string& filePath = "logs/server_activity.csv");
    ~CSVLogger();
    
    // Para peticiones al servidor
    void logRequest(
        const std::string& method,
        const std::string& user,
        const std::string& clientIP,
        int responseCode,
        const std::string& details = ""
    );
    
    // Para eventos del sistema
    void logSystem(
        const std::string& module,
        LogLevel level,
        const std::string& message,
        const std::string& details = ""
    );
    
    // Para debug
    void logDebug(
        const std::string& module,
        const std::string& function,
        const std::string& message,
        const std::string& details = ""
    );
    
    // Para errores específicos
    void logError(
        const std::string& module,
        const std::string& errorCode,
        const std::string& message,
        const std::string& details = ""
    );
    
    void flush();
    bool isOpen() const;
};

// Instancia global del logger
extern CSVLogger g_logger;

// Macros para facilitar el logging - Sobrecarga por número de argumentos
#define LOG_REQUEST(method, user, ip, code, details) \
    g_logger.logRequest(method, user, ip, code, details)

// Macros principales - adaptables a diferentes usos
#define LOG_SYSTEM(...) LOG_SYSTEM_IMPL(__VA_ARGS__, LOG_SYSTEM_4, LOG_SYSTEM_3, LOG_SYSTEM_2)(__VA_ARGS__)
#define LOG_ERROR(...) LOG_ERROR_IMPL(__VA_ARGS__, LOG_ERROR_4, LOG_ERROR_3, LOG_ERROR_2)(__VA_ARGS__)
#define LOG_DEBUG(...) LOG_DEBUG_IMPL(__VA_ARGS__, LOG_DEBUG_4, LOG_DEBUG_3, LOG_DEBUG_2)(__VA_ARGS__)
#define LOG_INFO(...) LOG_INFO_IMPL(__VA_ARGS__, LOG_INFO_4, LOG_INFO_3, LOG_INFO_2)(__VA_ARGS__)
#define LOG_WARNING(...) LOG_WARNING_IMPL(__VA_ARGS__, LOG_WARNING_4, LOG_WARNING_3, LOG_WARNING_2)(__VA_ARGS__)

// Implementaciones internas
#define LOG_SYSTEM_IMPL(_1, _2, _3, _4, NAME, ...) NAME
#define LOG_ERROR_IMPL(_1, _2, _3, _4, NAME, ...) NAME  
#define LOG_DEBUG_IMPL(_1, _2, _3, _4, NAME, ...) NAME
#define LOG_INFO_IMPL(_1, _2, _3, _4, NAME, ...) NAME
#define LOG_WARNING_IMPL(_1, _2, _3, _4, NAME, ...) NAME

// Implementaciones específicas por número de argumentos
#define LOG_SYSTEM_2(module, message) g_logger.logSystem(module, LogLevel::INFO, message, "")
#define LOG_SYSTEM_3(module, level, message) g_logger.logSystem(module, level, message, "")
#define LOG_SYSTEM_4(module, level, message, details) g_logger.logSystem(module, level, message, details)

#define LOG_ERROR_2(module, message) g_logger.logError(module, "", message, "")
#define LOG_ERROR_3(module, message, details) g_logger.logError(module, "", message, details)
#define LOG_ERROR_4(module, code, message, details) g_logger.logError(module, code, message, details)

#define LOG_DEBUG_2(module, message) g_logger.logDebug(module, "", message, "")
#define LOG_DEBUG_3(module, message, details) g_logger.logDebug(module, "", message, details)
#define LOG_DEBUG_4(module, function, message, details) g_logger.logDebug(module, function, message, details)

#define LOG_INFO_2(module, message) g_logger.logSystem(module, LogLevel::INFO, message, "")
#define LOG_INFO_3(module, message, details) g_logger.logSystem(module, LogLevel::INFO, message, details)

#define LOG_WARNING_2(module, message) g_logger.logSystem(module, LogLevel::WARNING, message, "")
#define LOG_WARNING_3(module, message, details) g_logger.logSystem(module, LogLevel::WARNING, message, details)

#endif // CSVLOGGER_H