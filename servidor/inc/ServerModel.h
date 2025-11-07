#ifndef _SERVER_MODEL_H_
#define _SERVER_MODEL_H_

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <random>
#include <ctime>
#include <sstream>
#include <iostream>
#include "../lib/XmlRpc.h"
#include "RPCExceptions.h"
#include "Robot.h"
#include "../lib/db_pool.hpp"
#include "CSVLogger.h"

namespace RPCServer {

// Forward declarations
class ServiceMethod;
class Robot;

// Helper function declaration
void registerRobotMethods(XmlRpc::XmlRpcServer* srv,
    std::vector<std::unique_ptr<ServiceMethod>>& methods, Robot* robot);

/**
 * @brief Modelo de datos de configuración del servidor
 */
class ServerConfig {
private:
    int port;
    bool introspectionEnabled;
    int verbosityLevel;

public:
    ServerConfig(int serverPort = 8080, bool enableIntrospection = true, int verbosity = 5)
        : port(serverPort), introspectionEnabled(enableIntrospection), verbosityLevel(verbosity) {}

    int getPort() const { return port; }
    bool isIntrospectionEnabled() const { return introspectionEnabled; }
    int getVerbosityLevel() const { return verbosityLevel; }

    void setPort(int newPort) { port = newPort; }
    void setIntrospectionEnabled(bool enabled) { introspectionEnabled = enabled; }
    void setVerbosityLevel(int level) { verbosityLevel = level; }
};

/**
 * @brief Clase base para métodos de servicio del servidor
 */
class ServiceMethod : public XmlRpc::XmlRpcServerMethod {
protected:
    std::string methodDescription;
    
    // Helper para logging de requests
    void logRequest(const std::string& user, const std::string& clientIP, int responseCode, const std::string& details = "") {
        LOG_REQUEST(_name, user, clientIP, responseCode, details);
    }
    
    // Helper para obtener IP del cliente (implementación básica)
    std::string getClientIP() {
        // En una implementación real, esto debería extraer la IP del contexto de la conexión
        return "127.0.0.1"; // Por ahora localhost
    }

public:
    ServiceMethod(const std::string& name, const std::string& description, XmlRpc::XmlRpcServer* server)
        : XmlRpc::XmlRpcServerMethod(name, server), methodDescription(description) {
        LOG_SYSTEM("ServiceMethod", LogLevel::DEBUG, "Método registrado", "Method: " + name + " - " + description);
    }

    virtual std::string help() override { return methodDescription; }
    virtual void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override = 0;
    virtual ~ServiceMethod() = default;
};

/**
 * @brief Implementación del método de prueba del servidor
 */
class ServerTestMethod : public ServiceMethod {
public:
    ServerTestMethod(XmlRpc::XmlRpcServer* server)
        : ServiceMethod("ServerTest", "Respondo quien soy cuando no hay argumentos", server) {}

    void execute(XmlRpc::XmlRpcValue& /*params*/, XmlRpc::XmlRpcValue& result) override {
        try {
            LOG_DEBUG("ServerTestMethod", "execute", "Procesando test del servidor", "");
            result = "Hi, soy el servidor RPC !!";
            logRequest("anonymous", getClientIP(), 200, "Server test successful");
        } catch (const std::exception& e) {
            LOG_ERROR("ServerTestMethod", "EXEC_ERROR", "Error en ServerTest", e.what());
            logRequest("anonymous", getClientIP(), 500, "Server test failed: " + std::string(e.what()));
            throw MethodExecutionException("ServerTest", e.what());
        }
    }
};

/**
 * @brief Implementación del método Echo
 */
class EchoMethod : public ServiceMethod {
public:
    EchoMethod(XmlRpc::XmlRpcServer* server)
        : ServiceMethod("Eco", "Diga algo y recibira un saludo", server) {}

    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override {
        try {
            if (params.size() < 1) {
                throw InvalidParametersException("Eco", "1 string parameter");
            }

            std::string resultString = "Hola, ";
            resultString += std::string(params[0]);
            resultString += " ";
            resultString += std::string(params[0]);
            result = resultString;
        } catch (const std::exception& e) {
            throw MethodExecutionException("Eco", e.what());
        }
    }
};

/**
 * @brief Implementación del método Suma
 */
class SumMethod : public ServiceMethod {
public:
    SumMethod(XmlRpc::XmlRpcServer* server)
        : ServiceMethod("Sumar", "Indique varios numeros reales separados por espacio", server) {}

    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override {
        try {
            int nArgs = params.size();
            if (nArgs == 0) {
                throw InvalidParametersException("Sumar", "al menos 1 parámetro numérico");
            }

            double sum = 0.0;
            for (int i = 0; i < nArgs; ++i) {
                sum += double(params[i]);
            }
            result = sum;
        } catch (const std::exception& e) {
            throw MethodExecutionException("Sumar", e.what());
        }
    }
};

// Forward declarations para clases que se usarán
class UserManager;
class DatabaseManager;
class AuthLoginMethod;
class AuthLogoutMethod;
class UserCreateMethod;
class UserListMethod;
class UserInfoMethod;
class UserUpdateMethod;
class UserDeleteMethod;

/**
 * @brief Clase modelo del servidor que gestiona el servidor RPC
 */
class ServerModel {
private:
    std::unique_ptr<XmlRpc::XmlRpcServer> server;
    std::unique_ptr<ServerConfig> config;
    std::vector<std::unique_ptr<ServiceMethod>> methods;
    std::unique_ptr<Robot> robot_;
    bool isRunning;
    
    // Managers para base de datos y usuarios
    std::shared_ptr<DatabaseManager> databaseManager_;
    std::shared_ptr<UserManager> userManager_;

public:
    ServerModel(std::unique_ptr<ServerConfig> serverConfig)
      : config(std::move(serverConfig)), isRunning(false) {
        server = std::make_unique<XmlRpc::XmlRpcServer>();
        robot_ = std::make_unique<Robot>(); // inicializar robot
        // Inicializar managers
        databaseManager_ = std::make_shared<DatabaseManager>();
        userManager_ = std::make_shared<UserManager>(databaseManager_);
        
        initializeMethods();
        initializeAuthMethods(); // Llamar después de que todo esté definido
    }
 // Declaración de helper para registrar métodos del Robot (definido más abajo)
    friend inline void registerRobotMethods(XmlRpc::XmlRpcServer*,
        std::vector<std::unique_ptr<ServiceMethod>>&,
        Robot*);
    
    void initializeMethods() {
        try {
            // Métodos básicos del servidor
            methods.push_back(std::make_unique<ServerTestMethod>(server.get()));
            methods.push_back(std::make_unique<EchoMethod>(server.get()));
            methods.push_back(std::make_unique<SumMethod>(server.get()));
            
            // Métodos del Robot (UML ServidorRPC) - ya definidos abajo
            // Los métodos del robot se registran en registerRobotMethods()
            // Registrar métodos relacionados al Robot
            registerRobotMethods(server.get(), methods, robot_.get());
        } catch (const std::exception& e) {
            throw ServerInitializationException("Falló la inicialización de métodos: " + std::string(e.what()));
        }
    }
    
    // Declaración para método que inicializa los métodos de autenticación
    void initializeAuthMethods();
    

    void start() {
        try {
            XmlRpc::setVerbosity(config->getVerbosityLevel());
            server->enableIntrospection(config->isIntrospectionEnabled());
            
            if (!server->bindAndListen(config->getPort())) {
                throw ServerBindingException(config->getPort(), "No se pudo vincular y escuchar");
            }
            
            isRunning = true;
        } catch (const std::exception& e) {
            throw ServerInitializationException("Falló el inicio del servidor: " + std::string(e.what()));
        }
    }

    void run() {
        if (!isRunning) {
            throw ServerInitializationException("Servidor no iniciado");
        }
        
        try {
            server->work(-1.0);
        } catch (const std::exception& e) {
            throw ServerInitializationException("Error de ejecución del servidor: " + std::string(e.what()));
        }
    }

    void stop() {
        isRunning = false;
        server->shutdown();
    }

    bool getIsRunning() const { return isRunning; }
    int getPort() const { return config->getPort(); }
};

class ConnectRobotMethod : public ServiceMethod {
    Robot* robot;
public:
    ConnectRobotMethod(XmlRpc::XmlRpcServer* server, Robot* r)
      : ServiceMethod("connectRobot", "Conecta al puerto serie del robot", server), robot(r) {}
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override {
        std::string clientIP = getClientIP();
        std::string user = "unknown"; // TODO: Extraer del token
        
        try {
            if (params.size() < 1) {
                LOG_WARNING("ConnectRobotMethod", "Parámetros insuficientes para connectRobot");
                throw InvalidParametersException("connectRobot", "port:string [, baud:int=9600]");
            }
            
            std::string port = static_cast<std::string>(params[0]);
            int baud = 115200;
            if (params.size() >= 2) baud = static_cast<int>(params[1]);
            
            g_logger.logSystem("ConnectRobotMethod", LogLevel::INFO, "Intento de conexión robot", "Puerto: " + port + " Baud: " + std::to_string(baud));
            
            if (robot->connect(port, baud)) {
                // Habilitar tracking automáticamente al conectar para que los movimientos actualicen la posición
                robot->enablePositionTracking(true);
                
                result["ok"] = true; 
                result["message"] = "Conectado";
                g_logger.logSystem("ConnectRobotMethod", LogLevel::INFO, "Robot conectado exitosamente con tracking habilitado", "");
                logRequest(user, clientIP, 200, "Robot connected successfully to " + port + " with tracking enabled");
            }
            else { 
                result["ok"] = false; 
                result["message"] = "Fallo conectando";
                g_logger.logError("ConnectRobotMethod", "", "Fallo conectando robot", "");
                logRequest(user, clientIP, 500, "Robot connection failed to " + port);
            }
        } catch (const std::exception& e) {
            g_logger.logError("ConnectRobotMethod", "", "Excepción en connectRobot", e.what());
            logRequest(user, clientIP, 500, "Exception in connectRobot: " + std::string(e.what()));
            throw MethodExecutionException("connectRobot", e.what());
        }
    }
};


class DisconnectRobotMethod : public ServiceMethod {
    Robot* robot;
public:
    DisconnectRobotMethod(XmlRpc::XmlRpcServer* server, Robot* r)
      : ServiceMethod("disconnectRobot", "Desconecta el puerto serie", server), robot(r) {}
    void execute(XmlRpc::XmlRpcValue& /*params*/, XmlRpc::XmlRpcValue& result) override {
        try { robot->disconnect(); result["ok"]=true; result["message"]="Desconectado"; }
        catch (const std::exception& e) { throw MethodExecutionException("disconnectRobot", e.what()); }
    }
};

class SetModeMethod : public ServiceMethod {
    Robot* robot;
public:
    SetModeMethod(XmlRpc::XmlRpcServer* server, Robot* r)
      : ServiceMethod("setMode", "Configura modo manual/absoluto", server), robot(r) {}
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override {
        try {
            if (params.size() < 2) throw InvalidParametersException("setMode", "manual:bool, absolute:bool");
            if (!robot->isConnected()) { result["ok"]=false; result["message"]="No conectado"; return; }
            bool manual = bool(params[0]); bool absolute = bool(params[1]);
            bool ok = robot->setMode(manual, absolute);
            result["ok"]=ok; result["message"]= ok ? "OK" : "Fallo setMode";
        } catch (const std::exception& e) { throw MethodExecutionException("setMode", e.what()); }
    }
};


class EnableMotorsMethod : public ServiceMethod {
    Robot* robot;
public:
    EnableMotorsMethod(XmlRpc::XmlRpcServer* server, Robot* r)
      : ServiceMethod("enableMotors", "Enciende/Apaga motores", server), robot(r) {}
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override {
        try {
            if (params.size() < 1) throw InvalidParametersException("enableMotors", "on:bool");
            if (!robot->isConnected()) { result["ok"]=false; result["message"]="No conectado"; return; }
            bool on = static_cast<bool>(params[0]);
            bool ok = robot->enableMotors(on);
            result["ok"]=ok; result["message"]= ok ? (on?"Motores ON":"Motores OFF") : "Fallo enableMotors";
        } catch (const std::exception& e) { throw MethodExecutionException("enableMotors", e.what()); }
    }
};

class HomeMethod : public ServiceMethod {
    Robot* robot;
public:
    HomeMethod(XmlRpc::XmlRpcServer* server, Robot* r)
      : ServiceMethod("home", "Homing del robot", server), robot(r) {}
    void execute(XmlRpc::XmlRpcValue& /*params*/, XmlRpc::XmlRpcValue& result) override {
        try {
            if (!robot->isConnected()) { result["ok"]=false; result["message"]="No conectado"; return; }
            bool ok = robot->home();
            result["ok"]=ok; result["message"]= ok ? "Home ejecutado" : "Fallo home";
        } catch (const std::exception& e) { throw MethodExecutionException("home", e.what()); }
    }
};



class MoveMethod : public ServiceMethod {
    Robot* robot;
public:
    MoveMethod(XmlRpc::XmlRpcServer* server, Robot* r)
      : ServiceMethod("move", "Movimiento cartesiano", server), robot(r) {}
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override {
        std::string clientIP = getClientIP();
        std::string user = "unknown"; // TODO: Obtener del token cuando esté disponible
        
        try {
            if (params.size() < 4) {
                LOG_WARNING("MoveMethod", "Parámetros insuficientes para move");
                logRequest(user, clientIP, 400, "Insufficient parameters");
                throw InvalidParametersException("move", "x:double, y:double, z:double, vel:double");
            }
            
            if (!robot->isConnected()) { 
                LOG_WARNING("MoveMethod", "Intento de movimiento sin robot conectado");
                logRequest(user, clientIP, 409, "Robot not connected");
                result["ok"]=false; 
                result["message"]="No conectado"; 
                return; 
            }
            
            // Conversión robusta de XmlRpcValue a double (soporta int y double)
            auto toDouble = [](XmlRpc::XmlRpcValue& val) -> double {
                if (val.getType() == XmlRpc::XmlRpcValue::TypeInt) {
                    int intVal = val;  // Conversión implícita de XmlRpcValue a int
                    return static_cast<double>(intVal);
                } else {
                    return val;  // Conversión implícita de XmlRpcValue a double
                }
            };
            
            double x = toDouble(params[0]);
            double y = toDouble(params[1]); 
            double z = toDouble(params[2]);
            double vel = toDouble(params[3]);
            
            LOG_DEBUG("MoveMethod", "execute", "Ejecutando movimiento", 
                     "X:" + std::to_string(x) + " Y:" + std::to_string(y) + 
                     " Z:" + std::to_string(z) + " Vel:" + std::to_string(vel));
            
            bool ok = robot->move(x,y,z,vel);
            
            if (ok) {
                LOG_INFO("MoveMethod", "Movimiento ejecutado exitosamente");
                logRequest(user, clientIP, 200, "Movement successful - X:" + std::to_string(x) + 
                          " Y:" + std::to_string(y) + " Z:" + std::to_string(z));
            } else {
                LOG_WARNING("MoveMethod", "Fallo en ejecución de movimiento");
                logRequest(user, clientIP, 500, "Movement failed");
            }
            
            result["ok"]=ok; 
            result["message"]= ok ? "Movimiento enviado" : "Fallo move";
            
        } catch (const std::exception& e) { 
            LOG_ERROR("MoveMethod", "MOVE_EXCEPTION", "Error durante movimiento", e.what());
            logRequest(user, clientIP, 500, "Move exception: " + std::string(e.what()));
            throw MethodExecutionException("move", e.what()); 
        }
    }
};

class EndEffectorMethod : public ServiceMethod {
    Robot* robot;
public:
    EndEffectorMethod(XmlRpc::XmlRpcServer* server, Robot* r)
      : ServiceMethod("endEffector", "Control gripper/endeffector", server), robot(r) {}
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override {
        try {
            if (params.size() < 1) throw InvalidParametersException("endEffector", "on:bool");
            if (!robot->isConnected()) { result["ok"]=false; result["message"]="No conectado"; return; }
            bool on = static_cast<bool>(params[0]);
            bool ok = robot->endEffector(on);
            result["ok"]=ok; result["message"]= ok ? (on?"Gripper ON":"Gripper OFF") : "Fallo endEffector";
        } catch (const std::exception& e) { throw MethodExecutionException("endEffector", e.what()); }
    }
};

class ExecuteGcodeMethod : public ServiceMethod {
    Robot* robot;
public:
    ExecuteGcodeMethod(XmlRpc::XmlRpcServer* server, Robot* r)
      : ServiceMethod("executeGcode", "Ejecuta código G-code en el robot", server), robot(r) {}
    
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override {
        try {
            if (params.size() < 1) {
                throw InvalidParametersException("executeGcode", "gcode:string");
            }
            
            if (!robot->isConnected()) {
                result["success"] = false;
                result["message"] = "Robot no conectado";
                return;
            }
            
            std::string gcode = std::string(params[0]);
            
            // Parsear y ejecutar línea por línea
            std::istringstream stream(gcode);
            std::string line;
            int lineCount = 0;
            int successCount = 0;
            std::vector<std::string> errors;
            
            while (std::getline(stream, line)) {
                lineCount++;
                
                // Limpiar la línea (quitar espacios y comentarios al final)
                size_t commentPos = line.find(';');
                if (commentPos != std::string::npos) {
                    line = line.substr(0, commentPos);
                }
                
                // Quitar espacios al inicio y final
                line.erase(0, line.find_first_not_of(" \t\r\n"));
                line.erase(line.find_last_not_of(" \t\r\n") + 1);
                
                // Saltar líneas vacías
                if (line.empty()) {
                    continue;
                }
                
                // Enviar comando al robot
                if (robot->sendGcodeCommand(line)) {
                    successCount++;
                } else {
                    errors.push_back("Línea " + std::to_string(lineCount) + ": " + line);
                }
            }
            
            if (errors.empty()) {
                result["success"] = true;
                result["message"] = "G-code ejecutado exitosamente";
                result["linesProcessed"] = successCount;
            } else {
                result["success"] = false;
                result["message"] = "Algunos comandos fallaron";
                result["linesProcessed"] = successCount;
                
                // Convertir vector de errores a XmlRpcValue array
                result["errors"].setSize(errors.size());
                for (size_t i = 0; i < errors.size(); ++i) {
                    result["errors"][i] = errors[i];
                }
            }
            
        } catch (const std::exception& e) {
            throw MethodExecutionException("executeGcode", e.what());
        }
    }
};

/**
 * @brief Método XML-RPC para obtener la posición actual del robot
 */
class GetPositionMethod : public ServiceMethod {
private:
    Robot* robot_;
    
public:
    GetPositionMethod(XmlRpc::XmlRpcServer* srv, Robot* r)
        : ServiceMethod("getPosition", "Obtener posición actual del robot", srv), robot_(r) {}
    
    void execute(XmlRpc::XmlRpcValue& /*params*/, XmlRpc::XmlRpcValue& result) override {
        try {
            if (!robot_->isConnected()) {
                result["success"] = false;
                result["message"] = "Robot no conectado";
                return;
            }
            
            Position pos = robot_->getCurrentPosition();
            
            result["success"] = true;
            result["position"]["x"] = pos.x;
            result["position"]["y"] = pos.y;
            result["position"]["z"] = pos.z;
            result["position"]["feedrate"] = pos.feedrate;
            result["position"]["endEffectorActive"] = pos.endEffectorActive;
            
        } catch (const std::exception& e) {
            result["success"] = false;
            result["message"] = std::string("Error: ") + e.what();
        }
    }
};

/**
 * @brief Método XML-RPC para controlar el tracking de posición
 */
class SetPositionTrackingMethod : public ServiceMethod {
private:
    Robot* robot_;
    
public:
    SetPositionTrackingMethod(XmlRpc::XmlRpcServer* srv, Robot* r)
        : ServiceMethod("setPositionTracking", "Habilitar/deshabilitar tracking de posición", srv), robot_(r) {}
    
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override {
        if (params.size() != 1) {
            throw XmlRpc::XmlRpcException("setPositionTracking requiere 1 parámetro: enable:bool");
        }
        
        try {
            if (!robot_->isConnected()) {
                result["success"] = false;
                result["message"] = "Robot no conectado";
                return;
            }
            
            bool enable = bool(params[0]);
            robot_->enablePositionTracking(enable);
            
            result["success"] = true;
            result["message"] = enable ? "Tracking habilitado" : "Tracking deshabilitado";
            result["trackingEnabled"] = enable;
            
        } catch (const std::exception& e) {
            result["success"] = false;
            result["message"] = std::string("Error: ") + e.what();
        }
    }
};

/**
 * @brief Método XML-RPC para consultar si el robot está conectado
 */
class IsConnectedMethod : public ServiceMethod {
private:
    Robot* robot_;

public:
    IsConnectedMethod(XmlRpc::XmlRpcServer* srv, Robot* r)
        : ServiceMethod("isConnected", "Indica si el robot está conectado", srv), robot_(r) {}

    void execute(XmlRpc::XmlRpcValue& /*params*/, XmlRpc::XmlRpcValue& result) override {
        try {
            bool connected = robot_->isConnected();
            result["success"] = true;
            result["connected"] = connected;
        } catch (const std::exception& e) {
            result["success"] = false;
            result["message"] = std::string("Error: ") + e.what();
        }
    }
};

/**
 * @brief Método XML-RPC para obtener el estado completo del robot
 */
class GetRobotStatusMethod : public ServiceMethod {
private:
    Robot* robot_;

public:
    GetRobotStatusMethod(XmlRpc::XmlRpcServer* srv, Robot* r)
        : ServiceMethod("getRobotStatus", "Obtiene estado completo del robot", srv), robot_(r) {}

    void execute(XmlRpc::XmlRpcValue& /*params*/, XmlRpc::XmlRpcValue& result) override {
        try {
            bool connected = robot_->isConnected();
            result["success"] = true;
            result["connected"] = connected;
            
            if (connected) {
                // Estado de los motores (del Robot interno)
                result["motorsOn"] = robot_->getMotorsOn();
                result["gripperOn"] = robot_->getGripperOn();
                
                // Intentar obtener posición
                try {
                    // Usar el Robot para obtener posición actual si es posible
                    result["position"]["x"] = 0.0;
                    result["position"]["y"] = 0.0; 
                    result["position"]["z"] = 0.0;
                } catch (...) {
                    result["position"]["x"] = 0.0;
                    result["position"]["y"] = 0.0;
                    result["position"]["z"] = 0.0;
                }
            } else {
                result["motorsOn"] = false;
                result["gripperOn"] = false;
                result["position"]["x"] = 0.0;
                result["position"]["y"] = 0.0;
                result["position"]["z"] = 0.0;
            }
        } catch (const std::exception& e) {
            result["success"] = false;
            result["message"] = std::string("Error: ") + e.what();
        }
    }
};

// Helper: registrar aquí los métodos relacionados al Robot
inline void registerRobotMethods(XmlRpc::XmlRpcServer* srv,
    std::vector<std::unique_ptr<ServiceMethod>>& methods, Robot* robot) {
    methods.push_back(std::make_unique<ConnectRobotMethod>(srv, robot));
    methods.push_back(std::make_unique<DisconnectRobotMethod>(srv, robot));
    methods.push_back(std::make_unique<SetModeMethod>(srv, robot));
    methods.push_back(std::make_unique<EnableMotorsMethod>(srv, robot));
    methods.push_back(std::make_unique<HomeMethod>(srv, robot));
    methods.push_back(std::make_unique<MoveMethod>(srv, robot));
    methods.push_back(std::make_unique<EndEffectorMethod>(srv, robot));
    methods.push_back(std::make_unique<ExecuteGcodeMethod>(srv, robot));
    
    // Nuevos métodos para tracking y aprendizaje
    methods.push_back(std::make_unique<GetPositionMethod>(srv, robot));
    methods.push_back(std::make_unique<SetPositionTrackingMethod>(srv, robot));
    methods.push_back(std::make_unique<IsConnectedMethod>(srv, robot));
    methods.push_back(std::make_unique<GetRobotStatusMethod>(srv, robot));
}

/**
 * @brief Clase centralizada para manejo de base de datos PostgreSQL
 */
class DatabaseManager {
private:
    std::shared_ptr<PgPool> dbPool_;
    
    std::string hashPassword(const std::string& password) {
        // Usar la función hash_password de PostgreSQL directamente en la consulta
        // En lugar de hacer el hash aquí, lo haremos en la consulta SQL
        return password;  // Retornar la contraseña sin hash para usar en la consulta SQL
    }
    
public:
    DatabaseManager() {
        PgConfig cfg;
        cfg.host = "localhost";
        cfg.port = 31432;  // Puerto del Docker
        cfg.dbname = "poo";
        cfg.user = "postgres";
        cfg.password = "pass123";
        cfg.ssl_disable = true;
        
        try {
            dbPool_ = std::make_shared<PgPool>(cfg, 2, 10);
            std::cout << "DatabaseManager: Conectado a PostgreSQL" << std::endl;
            createDefaultUsers();
        } catch (const std::exception& e) {
            std::cerr << "ERROR DatabaseManager: " << e.what() << std::endl;
            dbPool_ = nullptr;
        }
    }
    
    bool isConnected() const { return dbPool_ != nullptr; }
    
    // === GESTIÓN DE USUARIOS ===
    
    struct User {
        int id;
        std::string username;
        std::string role;
        bool active;
        std::time_t createdAt;
    };
    
    void createDefaultUsers() {
        if (!dbPool_) return;
        
        try {
            auto h = dbPool_->acquire();
            pqxx::work txn(h.conn());
            
            // Verificar si admin existe
            auto result = txn.exec_params(
                "SELECT usuario_id FROM finalpoo.usuario WHERE username = $1", "admin");
            
            if (result.empty()) {
                // Crear admin usando hash_password de PostgreSQL
                txn.exec_params(
                    "INSERT INTO finalpoo.usuario (username, password_hash, role) VALUES ($1, hash_password($2), $3)",
                    "admin", "Admin123", "ADMIN");
            }
            
            // Verificar si user existe
            result = txn.exec_params(
                "SELECT usuario_id FROM finalpoo.usuario WHERE username = $1", "user");
            
            if (result.empty()) {
                // Crear user usando hash_password de PostgreSQL
                txn.exec_params(
                    "INSERT INTO finalpoo.usuario (username, password_hash, role) VALUES ($1, hash_password($2), $3)",
                    "user", "User123", "OPERATOR");
            }
            
            txn.commit();
            
        } catch (const std::exception& e) {
            std::cerr << "Error creando usuarios por defecto: " << e.what() << std::endl;
        }
    }
    
    int createUser(const std::string& username, const std::string& password, const std::string& role = "OPERATOR") {
        if (!dbPool_) return -1;
        
        try {
            auto h = dbPool_->acquire();
            pqxx::work txn(h.conn());
            
            // Verificar si usuario ya existe
            auto result = txn.exec_params(
                "SELECT usuario_id FROM finalpoo.usuario WHERE username = $1", username);
            
            if (!result.empty()) {
                return -1; // Usuario ya existe
            }
            
            // Crear nuevo usuario
            auto insert_result = txn.exec_params(
                "INSERT INTO finalpoo.usuario (username, password_hash, role) "
                "VALUES ($1, hash_password($2), $3) RETURNING usuario_id",
                username, password, role
            );
            
            txn.commit();
            
            if (!insert_result.empty()) {
                return insert_result[0][0].as<int>();
            }
            return -1;
            
        } catch (const std::exception& e) {
            std::cerr << "Error creating user: " << e.what() << std::endl;
            return -1;
        }
    }
    
    User* authenticateUser(const std::string& username, const std::string& password) {
        if (!dbPool_) return nullptr;
        
        try {
            auto h = dbPool_->acquire();
            pqxx::work txn(h.conn());
            
            // Usar la función hash_password de PostgreSQL directamente
            auto result = txn.exec_params(
                "SELECT usuario_id, username, role, active, "
                "EXTRACT(EPOCH FROM created_at)::INTEGER as created_at "
                "FROM finalpoo.usuario WHERE username = $1 AND password_hash = hash_password($2) AND active = true",
                username, password
            );
            
            if (result.empty()) {
                return nullptr;
            }
            
            static User user;
            user.id = result[0][0].as<int>();
            user.username = result[0][1].as<std::string>();
            user.role = result[0][2].as<std::string>();
            user.active = result[0][3].as<bool>();
            user.createdAt = static_cast<std::time_t>(result[0][4].as<int>());
            
            return &user;
            
        } catch (const std::exception& e) {
            std::cerr << "Error authenticating user: " << e.what() << std::endl;
            return nullptr;
        }
    }
    
    User* getUserByUsername(const std::string& username) {
        if (!dbPool_) return nullptr;
        
        try {
            auto h = dbPool_->acquire();
            pqxx::work txn(h.conn());
            
            auto result = txn.exec_params(
                "SELECT usuario_id, username, role, active, "
                "EXTRACT(EPOCH FROM created_at)::INTEGER as created_at "
                "FROM finalpoo.usuario WHERE username = $1",
                username
            );
            
            if (result.empty()) {
                return nullptr;
            }
            
            static User user;
            user.id = result[0][0].as<int>();
            user.username = result[0][1].as<std::string>();
            user.role = result[0][2].as<std::string>();
            user.active = result[0][3].as<bool>();
            user.createdAt = static_cast<std::time_t>(result[0][4].as<int>());
            
            return &user;
            
        } catch (const std::exception& e) {
            std::cerr << "Error getting user by username: " << e.what() << std::endl;
            return nullptr;
        }
    }
    
    User* getUserById(int userId) {
        if (!dbPool_) return nullptr;
        
        try {
            auto h = dbPool_->acquire();
            pqxx::work txn(h.conn());
            
            auto result = txn.exec_params(
                "SELECT usuario_id, username, role, active, "
                "EXTRACT(EPOCH FROM created_at)::INTEGER as created_at "
                "FROM finalpoo.usuario WHERE usuario_id = $1",
                userId
            );
            
            if (result.empty()) {
                return nullptr;
            }
            
            static User user;
            user.id = result[0][0].as<int>();
            user.username = result[0][1].as<std::string>();
            user.role = result[0][2].as<std::string>();
            user.active = result[0][3].as<bool>();
            user.createdAt = static_cast<std::time_t>(result[0][4].as<int>());
            
            return &user;
            
        } catch (const std::exception& e) {
            std::cerr << "Error getting user by ID: " << e.what() << std::endl;
            return nullptr;
        }
    }
    
    std::vector<User> getAllUsers() {
        std::vector<User> result;
        if (!dbPool_) return result;
        
        try {
            auto h = dbPool_->acquire();
            pqxx::work txn(h.conn());
            
            auto query_result = txn.exec(
                "SELECT usuario_id, username, role, active, "
                "EXTRACT(EPOCH FROM created_at)::INTEGER as created_at "
                "FROM finalpoo.usuario ORDER BY created_at DESC"
            );
            
            for (const auto& row : query_result) {
                User user;
                user.id = row[0].as<int>();
                user.username = row[1].as<std::string>();
                user.role = row[2].as<std::string>();
                user.active = row[3].as<bool>();
                user.createdAt = static_cast<std::time_t>(row[4].as<int>());
                result.push_back(user);
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error getting all users: " << e.what() << std::endl;
        }
        
        return result;
    }
    
    bool updateUser(int userId, const std::string& newUsername, const std::string& newRole) {
        if (!dbPool_) return false;
        
        try {
            auto h = dbPool_->acquire();
            pqxx::work txn(h.conn());
            
            auto result = txn.exec_params(
                "UPDATE finalpoo.usuario SET username = $1, role = $2 WHERE usuario_id = $3",
                newUsername, newRole, userId
            );
            
            txn.commit();
            return result.affected_rows() > 0;
            
        } catch (const std::exception& e) {
            std::cerr << "Error updating user: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool updateUserPassword(const std::string& username, const std::string& newPassword) {
        if (!dbPool_) return false;
        
        try {
            auto h = dbPool_->acquire();
            pqxx::work txn(h.conn());
            
            auto result = txn.exec_params(
                "UPDATE finalpoo.usuario SET password_hash = hash_password($1) WHERE username = $2",
                newPassword, username
            );
            
            txn.commit();
            return result.affected_rows() > 0;
            
        } catch (const std::exception& e) {
            std::cerr << "Error updating user password: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool deleteUser(int userId) {
        if (!dbPool_) return false;
        
        try {
            auto h = dbPool_->acquire();
            pqxx::work txn(h.conn());
            
            // No permitir eliminar admin
            auto userCheck = txn.exec_params(
                "SELECT username FROM finalpoo.usuario WHERE usuario_id = $1", userId);
            
            if (!userCheck.empty() && userCheck[0][0].as<std::string>() == "admin") {
                return false;
            }
            
            auto result = txn.exec_params(
                "DELETE FROM finalpoo.usuario WHERE usuario_id = $1", userId
            );
            
            txn.commit();
            return result.affected_rows() > 0;
            
        } catch (const std::exception& e) {
            std::cerr << "Error deleting user: " << e.what() << std::endl;
            return false;
        }
    }
    
    // === GESTIÓN DE RUTINAS G-CODE ===
    
    struct Routine {
        int id;
        std::string filename;
        std::string originalFilename;
        std::string description;
        std::string gcodeContent;
        int fileSize;
        int userId;
        std::time_t createdAt;
    };
    
    int createRoutine(const std::string& filename, const std::string& originalFilename,
                     const std::string& description, const std::string& gcodeContent, int userId) {
        if (!dbPool_) return -1;
        
        try {
            auto h = dbPool_->acquire();
            pqxx::work txn(h.conn());
            
            auto result = txn.exec_params(
                "INSERT INTO finalpoo.gcode_routine (filename, original_filename, description, gcode_content, file_size, user_id) "
                "VALUES ($1, $2, $3, $4, $5, $6) RETURNING routine_id",
                filename, originalFilename, description, gcodeContent, static_cast<int>(gcodeContent.size()), userId
            );
            
            txn.commit();
            
            if (!result.empty()) {
                return result[0][0].as<int>();
            }
            return -1;
            
        } catch (const std::exception& e) {
            std::cerr << "Error creating routine: " << e.what() << std::endl;
            return -1;
        }
    }
    
    Routine* getRoutineById(int routineId) {
        if (!dbPool_) return nullptr;
        
        try {
            auto h = dbPool_->acquire();
            pqxx::work txn(h.conn());
            
            auto result = txn.exec_params(
                "SELECT routine_id, filename, original_filename, description, gcode_content, file_size, user_id, "
                "EXTRACT(EPOCH FROM created_at)::INTEGER as created_at "
                "FROM finalpoo.gcode_routine WHERE routine_id = $1",
                routineId
            );
            
            if (result.empty()) {
                return nullptr;
            }
            
            static Routine routine;
            routine.id = result[0][0].as<int>();
            routine.filename = result[0][1].as<std::string>();
            routine.originalFilename = result[0][2].as<std::string>();
            routine.description = result[0][3].as<std::string>();
            routine.gcodeContent = result[0][4].as<std::string>();
            routine.fileSize = result[0][5].as<int>();
            routine.userId = result[0][6].as<int>();
            routine.createdAt = static_cast<std::time_t>(result[0][7].as<int>());
            
            return &routine;
            
        } catch (const std::exception& e) {
            std::cerr << "Error getting routine by ID: " << e.what() << std::endl;
            return nullptr;
        }
    }
    
    std::vector<Routine> getAllRoutines() {
        std::vector<Routine> result;
        if (!dbPool_) return result;
        
        try {
            auto h = dbPool_->acquire();
            pqxx::work txn(h.conn());
            
            auto query_result = txn.exec(
                "SELECT routine_id, filename, original_filename, description, gcode_content, file_size, user_id, "
                "EXTRACT(EPOCH FROM created_at)::INTEGER as created_at "
                "FROM finalpoo.gcode_routine ORDER BY created_at DESC"
            );
            
            for (const auto& row : query_result) {
                Routine routine;
                routine.id = row[0].as<int>();
                routine.filename = row[1].as<std::string>();
                routine.originalFilename = row[2].as<std::string>();
                routine.description = row[3].as<std::string>();
                routine.gcodeContent = row[4].as<std::string>();
                routine.fileSize = row[5].as<int>();
                routine.userId = row[6].as<int>();
                routine.createdAt = static_cast<std::time_t>(row[7].as<int>());
                result.push_back(routine);
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error getting all routines: " << e.what() << std::endl;
        }
        
        return result;
    }
    
    std::vector<Routine> getRoutinesByUser(int userId) {
        std::vector<Routine> result;
        if (!dbPool_) return result;
        
        try {
            auto h = dbPool_->acquire();
            pqxx::work txn(h.conn());
            
            auto query_result = txn.exec_params(
                "SELECT routine_id, filename, original_filename, description, gcode_content, file_size, user_id, "
                "EXTRACT(EPOCH FROM created_at)::INTEGER as created_at "
                "FROM finalpoo.gcode_routine WHERE user_id = $1 ORDER BY created_at DESC",
                userId
            );
            
            for (const auto& row : query_result) {
                Routine routine;
                routine.id = row[0].as<int>();
                routine.filename = row[1].as<std::string>();
                routine.originalFilename = row[2].as<std::string>();
                routine.description = row[3].as<std::string>();
                routine.gcodeContent = row[4].as<std::string>();
                routine.fileSize = row[5].as<int>();
                routine.userId = row[6].as<int>();
                routine.createdAt = static_cast<std::time_t>(row[7].as<int>());
                result.push_back(routine);
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error getting routines by user: " << e.what() << std::endl;
        }
        
        return result;
    }
    
    bool updateRoutine(int routineId, const std::string& filename, const std::string& description, const std::string& gcodeContent) {
        if (!dbPool_) return false;
        
        try {
            auto h = dbPool_->acquire();
            pqxx::work txn(h.conn());
            
            auto result = txn.exec_params(
                "UPDATE finalpoo.gcode_routine SET filename = $1, description = $2, gcode_content = $3, file_size = $4 "
                "WHERE routine_id = $5",
                filename, description, gcodeContent, static_cast<int>(gcodeContent.size()), routineId
            );
            
            txn.commit();
            return result.affected_rows() > 0;
            
        } catch (const std::exception& e) {
            std::cerr << "Error updating routine: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool deleteRoutine(int routineId) {
        if (!dbPool_) return false;
        
        try {
            auto h = dbPool_->acquire();
            pqxx::work txn(h.conn());
            
            auto result = txn.exec_params(
                "DELETE FROM finalpoo.gcode_routine WHERE routine_id = $1", routineId
            );
            
            txn.commit();
            return result.affected_rows() > 0;
            
        } catch (const std::exception& e) {
            std::cerr << "Error deleting routine: " << e.what() << std::endl;
            return false;
        }
    }
};

/**
 * @brief Gestor de rutinas G-code con control de permisos
 */
class RoutineManager {
private:
    std::shared_ptr<DatabaseManager> dbManager_;
    
public:
    RoutineManager(std::shared_ptr<DatabaseManager> dbMgr) : dbManager_(dbMgr) {}
    
    struct RoutineInfo {
        int id;
        std::string filename;
        std::string originalFilename;
        std::string description;
        int fileSize;
        int userId;
        std::time_t createdAt;
    };
    
    int createRoutine(const std::string& filename, const std::string& originalFilename,
                     const std::string& description, const std::string& gcodeContent, int userId) {
        if (!dbManager_) return -1;
        return dbManager_->createRoutine(filename, originalFilename, description, gcodeContent, userId);
    }
    
    DatabaseManager::Routine* getRoutineById(int routineId, int requestUserId, const std::string& userRole) {
        if (!dbManager_) return nullptr;
        
        auto routine = dbManager_->getRoutineById(routineId);
        if (!routine) return nullptr;
        
        // Admin puede ver todas las rutinas, user solo las propias
        if (userRole == "ADMIN" || routine->userId == requestUserId) {
            return routine;
        }
        
        return nullptr; // Sin permisos
    }
    
    std::vector<RoutineInfo> getAllRoutines(int requestUserId, const std::string& userRole) {
        std::vector<RoutineInfo> result;
        if (!dbManager_) return result;
        
        std::vector<DatabaseManager::Routine> routines;
        
        if (userRole == "ADMIN") {
            // Admin ve todas las rutinas
            routines = dbManager_->getAllRoutines();
        } else {
            // User solo ve sus rutinas
            routines = dbManager_->getRoutinesByUser(requestUserId);
        }
        
        // Convertir a RoutineInfo (sin contenido G-code para lista)
        for (const auto& routine : routines) {
            RoutineInfo info;
            info.id = routine.id;
            info.filename = routine.filename;
            info.originalFilename = routine.originalFilename;
            info.description = routine.description;
            info.fileSize = routine.fileSize;
            info.userId = routine.userId;
            info.createdAt = routine.createdAt;
            result.push_back(info);
        }
        
        return result;
    }
    
    bool updateRoutine(int routineId, const std::string& filename, const std::string& description, 
                      const std::string& gcodeContent, int requestUserId, const std::string& userRole) {
        if (!dbManager_) return false;
        
        // Verificar permisos
        auto routine = dbManager_->getRoutineById(routineId);
        if (!routine) return false;
        
        // Solo el propietario o admin pueden modificar
        if (userRole != "ADMIN" && routine->userId != requestUserId) {
            return false;
        }
        
        return dbManager_->updateRoutine(routineId, filename, description, gcodeContent);
    }
    
    bool deleteRoutine(int routineId, int requestUserId, const std::string& userRole) {
        if (!dbManager_) return false;
        
        // Verificar permisos
        auto routine = dbManager_->getRoutineById(routineId);
        if (!routine) return false;
        
        // Solo el propietario o admin pueden eliminar
        if (userRole != "ADMIN" && routine->userId != requestUserId) {
            return false;
        }
        
        return dbManager_->deleteRoutine(routineId);
    }
};

// Clase para manejo de usuarios con sesiones basadas en cookies
class UserManager {
private:
    std::shared_ptr<DatabaseManager> dbManager_;
    std::random_device rd_;
    std::mt19937 gen_;
    
    // Mapa de sesiones en memoria (con cookies)
    struct Session {
        std::string token;
        int userId;
        std::time_t createdAt;
        std::time_t lastAccess;
        std::string userAgent;
        std::string ip;
    };
    
    std::unordered_map<std::string, Session> sessions_;
    
    std::string generateToken() {
        std::uniform_int_distribution<> dis(0, 15);
        std::stringstream ss;
        for (int i = 0; i < 32; ++i) {
            ss << std::hex << dis(gen_);
        }
        return ss.str();
    }

public:
    UserManager(std::shared_ptr<DatabaseManager> dbMgr) : dbManager_(dbMgr), gen_(rd_()) {}
    
    struct UserInfo {
        int id;
        std::string username;
        std::string role;
        bool active;
        std::time_t createdAt;
    };
    
    std::string login(const std::string& username, const std::string& password, 
                     const std::string& userAgent = "", const std::string& ip = "") {
        if (!dbManager_) return "";
        
        auto user = dbManager_->authenticateUser(username, password);
        if (!user) return "";
        
        // Crear sesión en memoria con cookie
        std::string token = generateToken();
        Session session = {
            token, user->id, std::time(nullptr), std::time(nullptr), userAgent, ip
        };
        sessions_[token] = session;
        
        return token;
    }
    
    bool validateToken(const std::string& token) {
        auto it = sessions_.find(token);
        if (it != sessions_.end()) {
            // Actualizar último acceso
            it->second.lastAccess = std::time(nullptr);
            return true;
        }
        return false;
    }
    
    UserInfo* getUserByToken(const std::string& token) {
        auto sessionIt = sessions_.find(token);
        if (sessionIt == sessions_.end()) return nullptr;
        
        auto user = dbManager_->getUserById(sessionIt->second.userId);
        if (!user) return nullptr;
        
        static UserInfo userInfo;
        userInfo.id = user->id;
        userInfo.username = user->username;
        userInfo.role = user->role;
        userInfo.active = user->active;
        userInfo.createdAt = user->createdAt;
        
        return &userInfo;
    }
    
    bool logout(const std::string& token) {
        return sessions_.erase(token) > 0;
    }
    
    int createUser(const std::string& username, const std::string& password, const std::string& role = "OPERATOR") {
        if (!dbManager_) return -1;
        return dbManager_->createUser(username, password, role);
    }
    
    std::vector<UserInfo> getAllUsers() {
        std::vector<UserInfo> result;
        if (!dbManager_) return result;
        
        auto users = dbManager_->getAllUsers();
        for (const auto& user : users) {
            UserInfo info;
            info.id = user.id;
            info.username = user.username;
            info.role = user.role;
            info.active = user.active;
            info.createdAt = user.createdAt;
            result.push_back(info);
        }
        
        return result;
    }
    
    UserInfo* getUserByUsername(const std::string& username) {
        if (!dbManager_) return nullptr;
        
        auto user = dbManager_->getUserByUsername(username);
        if (!user) return nullptr;
        
        static UserInfo userInfo;
        userInfo.id = user->id;
        userInfo.username = user->username;
        userInfo.role = user->role;
        userInfo.active = user->active;
        userInfo.createdAt = user->createdAt;
        
        return &userInfo;
    }
    
    bool updateUser(int userId, const std::string& newUsername, const std::string& newRole) {
        if (!dbManager_) return false;
        return dbManager_->updateUser(userId, newUsername, newRole);
    }
    
    bool updateUserPassword(const std::string& username, const std::string& newPassword) {
        if (!dbManager_) return false;
        return dbManager_->updateUserPassword(username, newPassword);
    }
    
    bool deleteUser(int userId) {
        if (!dbManager_) return false;
        
        // Invalidar todas las sesiones del usuario que se va a eliminar
        for (auto it = sessions_.begin(); it != sessions_.end();) {
            if (it->second.userId == userId) {
                it = sessions_.erase(it);
            } else {
                ++it;
            }
        }
        
        return dbManager_->deleteUser(userId);
    }
    
    bool deleteUser(const std::string& username) {
        if (!dbManager_) return false;
        
        // Obtener el usuario para saber su ID
        auto user = dbManager_->getUserByUsername(username);
        if (!user) return false;
        
        return deleteUser(user->id);
    }
    
    // Limpiar sesiones expiradas (llamar periódicamente)
    void cleanExpiredSessions(int maxAgeHours = 24) {
        std::time_t now = std::time(nullptr);
        std::time_t maxAge = maxAgeHours * 3600;
        
        for (auto it = sessions_.begin(); it != sessions_.end();) {
            if ((now - it->second.lastAccess) > maxAge) {
                it = sessions_.erase(it);
            } else {
                ++it;
            }
        }
    }
};

/**
 * @brief Método XML-RPC para login de usuarios
 */
class AuthLoginMethod : public ServiceMethod {
private:
    std::shared_ptr<UserManager> userManager_;
    
public:
    AuthLoginMethod(XmlRpc::XmlRpcServer* srv, std::shared_ptr<UserManager> userMgr)
        : ServiceMethod("authLogin", "Autenticación de usuarios", srv), userManager_(userMgr) {}
    
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override {
        std::string clientIP = getClientIP();
        std::string username = "unknown";
        
        if (params.size() != 2) {
            LOG_WARNING("AuthLoginMethod", "Intento de login con parámetros incorrectos");
            logRequest("unknown", clientIP, 400, "Invalid parameters count");
            throw XmlRpc::XmlRpcException("authLogin requiere 2 parámetros: username, password");
        }
        
        username = static_cast<std::string>(params[0]);
        std::string password = static_cast<std::string>(params[1]);
        
        LOG_DEBUG("AuthLoginMethod", "execute", "Intento de login", "Usuario: " + username);
        
        try {
            std::string token = userManager_->login(username, password);
            
            if (token.empty()) {
                LOG_WARNING("AuthLoginMethod", "Login fallido - credenciales inválidas para usuario: " + username);
                logRequest(username, clientIP, 401, "Invalid credentials");
                result["success"] = false;
                result["message"] = "Credenciales inválidas";
                return;
            }
            
            auto userInfo = userManager_->getUserByToken(token);
            if (!userInfo) {
                LOG_ERROR("AuthLoginMethod", "TOKEN_ERROR", "Error obteniendo usuario por token", "Token: " + token);
                logRequest(username, clientIP, 500, "Internal server error");
                result["success"] = false;
                result["message"] = "Error interno del servidor";
                return;
            }
            
            LOG_INFO("AuthLoginMethod", "Login exitoso para usuario: " + username + " (ID: " + std::to_string(userInfo->id) + ")");
            logRequest(username, clientIP, 200, "Login successful - Role: " + userInfo->role);
            
            result["success"] = true;
            result["token"] = token;
            result["user"]["id"] = userInfo->id;
            result["user"]["username"] = userInfo->username;
            result["user"]["role"] = userInfo->role;
            result["user"]["active"] = userInfo->active;
            
        } catch (const std::exception& e) {
            LOG_ERROR("AuthLoginMethod", "LOGIN_EXCEPTION", "Excepción durante login", "Usuario: " + username + " Error: " + e.what());
            logRequest(username, clientIP, 500, "Login exception: " + std::string(e.what()));
            result["success"] = false;
            result["message"] = std::string("Error de login: ") + e.what();
        }
    }
};

/**
 * @brief Método XML-RPC para logout de usuarios
 */
class AuthLogoutMethod : public ServiceMethod {
private:
    std::shared_ptr<UserManager> userManager_;
    
public:
    AuthLogoutMethod(XmlRpc::XmlRpcServer* srv, std::shared_ptr<UserManager> userMgr)
        : ServiceMethod("authLogout", "Logout de usuarios", srv), userManager_(userMgr) {}
    
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override {
        if (params.size() != 1) {
            throw XmlRpc::XmlRpcException("authLogout requiere 1 parámetro: token");
        }
        
        std::string token = static_cast<std::string>(params[0]);
        
        try {
            bool success = userManager_->logout(token);
            result["success"] = success;
            result["message"] = success ? "Logout exitoso" : "Token no encontrado";
            
        } catch (const std::exception& e) {
            result["success"] = false;
            result["message"] = std::string("Error de logout: ") + e.what();
        }
    }
};

/**
 * @brief Método XML-RPC para crear usuarios
 */
class UserCreateMethod : public ServiceMethod {
private:
    std::shared_ptr<UserManager> userManager_;
    
public:
    UserCreateMethod(XmlRpc::XmlRpcServer* srv, std::shared_ptr<UserManager> userMgr)
        : ServiceMethod("userCreate", "Crear nuevo usuario", srv), userManager_(userMgr) {}
    
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override {
        if (params.size() != 4) {
            throw XmlRpc::XmlRpcException("userCreate requiere 4 parámetros: token, username, password, role");
        }
        
        std::string token = static_cast<std::string>(params[0]);
        std::string username = static_cast<std::string>(params[1]);
        std::string password = static_cast<std::string>(params[2]);
        std::string role = static_cast<std::string>(params[3]);
        
        try {
            // Verificar token y permisos
            if (!userManager_->validateToken(token)) {
                result["success"] = false;
                result["message"] = "Token inválido";
                return;
            }
            
            auto currentUser = userManager_->getUserByToken(token);
            if (!currentUser || currentUser->role != "ADMIN") {
                result["success"] = false;
                result["message"] = "Permisos insuficientes";
                return;
            }
            
            int userId = userManager_->createUser(username, password, role);
            
            if (userId > 0) {
                result["success"] = true;
                result["userId"] = userId;
                result["message"] = "Usuario creado exitosamente";
            } else {
                result["success"] = false;
                result["message"] = "Error creando usuario (posiblemente ya existe)";
            }
            
        } catch (const std::exception& e) {
            result["success"] = false;
            result["message"] = std::string("Error: ") + e.what();
        }
    }
};

/**
 * @brief Método XML-RPC para listar usuarios
 */
class UserListMethod : public ServiceMethod {
private:
    std::shared_ptr<UserManager> userManager_;
    
public:
    UserListMethod(XmlRpc::XmlRpcServer* srv, std::shared_ptr<UserManager> userMgr)
        : ServiceMethod("userList", "Listar usuarios", srv), userManager_(userMgr) {}
    
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override {
        if (params.size() != 1) {
            throw XmlRpc::XmlRpcException("userList requiere 1 parámetro: token");
        }
        
        std::string token = static_cast<std::string>(params[0]);
        
        try {
            // Verificar token y permisos
            if (!userManager_->validateToken(token)) {
                result["success"] = false;
                result["message"] = "Token inválido";
                return;
            }
            
            auto currentUser = userManager_->getUserByToken(token);
            if (!currentUser || currentUser->role != "ADMIN") {
                result["success"] = false;
                result["message"] = "Permisos insuficientes";
                return;
            }
            
            auto users = userManager_->getAllUsers();
            
            result["success"] = true;
            result["users"].setSize(users.size());
            
            for (size_t i = 0; i < users.size(); ++i) {
                result["users"][i]["id"] = users[i].id;
                result["users"][i]["username"] = users[i].username;
                result["users"][i]["role"] = users[i].role;
                result["users"][i]["active"] = users[i].active;
                result["users"][i]["createdAt"] = static_cast<int>(users[i].createdAt);
            }
            
        } catch (const std::exception& e) {
            result["success"] = false;
            result["message"] = std::string("Error: ") + e.what();
        }
    }
};

/**
 * @brief Método XML-RPC para obtener información de usuario
 */
class UserInfoMethod : public ServiceMethod {
private:
    std::shared_ptr<UserManager> userManager_;
    
public:
    UserInfoMethod(XmlRpc::XmlRpcServer* srv, std::shared_ptr<UserManager> userMgr)
        : ServiceMethod("userInfo", "Información de usuario", srv), userManager_(userMgr) {}
    
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override {
        if (params.size() != 2) {
            throw XmlRpc::XmlRpcException("userInfo requiere 2 parámetros: token, username");
        }
        
        std::string token = static_cast<std::string>(params[0]);
        std::string username = static_cast<std::string>(params[1]);
        
        try {
            // Verificar token
            if (!userManager_->validateToken(token)) {
                result["success"] = false;
                result["message"] = "Token inválido";
                return;
            }
            
            auto userInfo = userManager_->getUserByUsername(username);
            if (!userInfo) {
                result["success"] = false;
                result["message"] = "Usuario no encontrado";
                return;
            }
            
            result["success"] = true;
            result["user"]["id"] = userInfo->id;
            result["user"]["username"] = userInfo->username;
            result["user"]["role"] = userInfo->role;
            result["user"]["active"] = userInfo->active;
            result["user"]["createdAt"] = static_cast<int>(userInfo->createdAt);
            
        } catch (const std::exception& e) {
            result["success"] = false;
            result["message"] = std::string("Error: ") + e.what();
        }
    }
};

/**
 * @brief Método XML-RPC para actualizar usuario
 */
class UserUpdateMethod : public ServiceMethod {
private:
    std::shared_ptr<UserManager> userManager_;
    
public:
    UserUpdateMethod(XmlRpc::XmlRpcServer* srv, std::shared_ptr<UserManager> userMgr)
        : ServiceMethod("userUpdate", "Actualizar usuario", srv), userManager_(userMgr) {}
    
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override {
        if (params.size() != 3) {
            throw XmlRpc::XmlRpcException("userUpdate requiere 3 parámetros: token, username, updates");
        }
        
        std::string token = static_cast<std::string>(params[0]);
        std::string username = static_cast<std::string>(params[1]);
        XmlRpc::XmlRpcValue updates = params[2];
        
        try {
            // Verificar token y permisos
            if (!userManager_->validateToken(token)) {
                result["success"] = false;
                result["message"] = "Token inválido";
                return;
            }
            
            auto currentUser = userManager_->getUserByToken(token);
            if (!currentUser || currentUser->role != "ADMIN") {
                result["success"] = false;
                result["message"] = "Permisos insuficientes";
                return;
            }
            
            // No permitir actualizar admin
            if (username == "admin") {
                result["success"] = false;
                result["message"] = "No se puede modificar el usuario admin";
                return;
            }
            
            bool success = false;
            std::string message = "Usuario actualizado exitosamente";
            
            // Actualizar password si se proporciona
            if (updates.hasMember("password")) {
                std::string newPassword = updates["password"];
                success = userManager_->updateUserPassword(username, newPassword);
                if (!success) {
                    message = "Error actualizando contraseña";
                }
            }
            
            // Actualizar username y role si se proporcionan
            if (updates.hasMember("newUsername") || updates.hasMember("role")) {
                auto userInfo = userManager_->getUserByUsername(username);
                if (userInfo) {
                    std::string newUsername = updates.hasMember("newUsername") ? 
                                            std::string(updates["newUsername"]) : userInfo->username;
                    std::string newRole = updates.hasMember("role") ? 
                                        std::string(updates["role"]) : userInfo->role;
                    
                    success = userManager_->updateUser(userInfo->id, newUsername, newRole);
                    if (!success) {
                        message = "Error actualizando información del usuario";
                    }
                }
            }
            
            result["success"] = success;
            result["message"] = message;
            
        } catch (const std::exception& e) {
            result["success"] = false;
            result["message"] = std::string("Error: ") + e.what();
        }
    }
};

/**
 * @brief Método XML-RPC para eliminar usuario
 */
class UserDeleteMethod : public ServiceMethod {
private:
    std::shared_ptr<UserManager> userManager_;
    
public:
    UserDeleteMethod(XmlRpc::XmlRpcServer* srv, std::shared_ptr<UserManager> userMgr)
        : ServiceMethod("userDelete", "Eliminar usuario", srv), userManager_(userMgr) {}
    
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override {
        if (params.size() != 2) {
            throw XmlRpc::XmlRpcException("userDelete requiere 2 parámetros: token, username");
        }
        
        std::string token = static_cast<std::string>(params[0]);
        std::string username = static_cast<std::string>(params[1]);
        
        try {
            // Verificar token y permisos
            if (!userManager_->validateToken(token)) {
                result["success"] = false;
                result["message"] = "Token inválido";
                return;
            }
            
            auto currentUser = userManager_->getUserByToken(token);
            if (!currentUser || currentUser->role != "ADMIN") {
                result["success"] = false;
                result["message"] = "Permisos insuficientes";
                return;
            }
            
            // No permitir eliminar admin
            if (username == "admin") {
                result["success"] = false;
                result["message"] = "No se puede eliminar el usuario admin";
                return;
            }
            
            bool success = userManager_->deleteUser(username);
            
            result["success"] = success;
            result["message"] = success ? "Usuario eliminado exitosamente" : "Error eliminando usuario";
            
        } catch (const std::exception& e) {
            result["success"] = false;
            result["message"] = std::string("Error: ") + e.what();
        }
    }
};

/**
 * @brief Método XML-RPC para crear rutinas G-code
 */
class RoutineCreateMethod : public ServiceMethod {
private:
    std::shared_ptr<UserManager> userManager_;
    std::shared_ptr<RoutineManager> routineManager_;
    
public:
    RoutineCreateMethod(XmlRpc::XmlRpcServer* srv, std::shared_ptr<UserManager> userMgr, std::shared_ptr<RoutineManager> routineMgr)
        : ServiceMethod("routineCreate", "Crear nueva rutina G-code", srv), userManager_(userMgr), routineManager_(routineMgr) {}
    
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override {
        if (params.size() != 5) {
            throw XmlRpc::XmlRpcException("routineCreate requiere 5 parámetros: token, filename, originalFilename, description, gcodeContent");
        }
        
        std::string token = static_cast<std::string>(params[0]);
        std::string filename = static_cast<std::string>(params[1]);
        std::string originalFilename = static_cast<std::string>(params[2]);
        std::string description = static_cast<std::string>(params[3]);
        std::string gcodeContent = static_cast<std::string>(params[4]);
        
        try {
            // Verificar token
            if (!userManager_->validateToken(token)) {
                result["success"] = false;
                result["message"] = "Token inválido";
                return;
            }
            
            auto currentUser = userManager_->getUserByToken(token);
            if (!currentUser) {
                result["success"] = false;
                result["message"] = "Usuario no encontrado";
                return;
            }
            
            int routineId = routineManager_->createRoutine(filename, originalFilename, description, gcodeContent, currentUser->id);
            
            if (routineId > 0) {
                result["success"] = true;
                result["routineId"] = routineId;
                result["message"] = "Rutina creada exitosamente";
            } else {
                result["success"] = false;
                result["message"] = "Error creando rutina (posiblemente ya existe)";
            }
            
        } catch (const std::exception& e) {
            result["success"] = false;
            result["message"] = std::string("Error: ") + e.what();
        }
    }
};

/**
 * @brief Método XML-RPC para listar rutinas G-code
 */
class RoutineListMethod : public ServiceMethod {
private:
    std::shared_ptr<UserManager> userManager_;
    std::shared_ptr<RoutineManager> routineManager_;
    
public:
    RoutineListMethod(XmlRpc::XmlRpcServer* srv, std::shared_ptr<UserManager> userMgr, std::shared_ptr<RoutineManager> routineMgr)
        : ServiceMethod("routineList", "Listar rutinas G-code", srv), userManager_(userMgr), routineManager_(routineMgr) {}
    
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override {
        if (params.size() != 1) {
            throw XmlRpc::XmlRpcException("routineList requiere 1 parámetro: token");
        }
        
        std::string token = static_cast<std::string>(params[0]);
        
        try {
            // Verificar token
            if (!userManager_->validateToken(token)) {
                result["success"] = false;
                result["message"] = "Token inválido";
                return;
            }
            
            auto currentUser = userManager_->getUserByToken(token);
            if (!currentUser) {
                result["success"] = false;
                result["message"] = "Usuario no encontrado";
                return;
            }
            
            auto routines = routineManager_->getAllRoutines(currentUser->id, currentUser->role);
            
            result["success"] = true;
            result["routines"].setSize(routines.size());
            
            for (size_t i = 0; i < routines.size(); ++i) {
                result["routines"][i]["id"] = routines[i].id;
                result["routines"][i]["filename"] = routines[i].filename;
                result["routines"][i]["originalFilename"] = routines[i].originalFilename;
                result["routines"][i]["description"] = routines[i].description;
                result["routines"][i]["fileSize"] = routines[i].fileSize;
                result["routines"][i]["userId"] = routines[i].userId;
                result["routines"][i]["createdAt"] = static_cast<int>(routines[i].createdAt);
            }
            
        } catch (const std::exception& e) {
            result["success"] = false;
            result["message"] = std::string("Error: ") + e.what();
        }
    }
};

/**
 * @brief Método XML-RPC para obtener rutina G-code específica
 */
class RoutineGetMethod : public ServiceMethod {
private:
    std::shared_ptr<UserManager> userManager_;
    std::shared_ptr<RoutineManager> routineManager_;
    
public:
    RoutineGetMethod(XmlRpc::XmlRpcServer* srv, std::shared_ptr<UserManager> userMgr, std::shared_ptr<RoutineManager> routineMgr)
        : ServiceMethod("routineGet", "Obtener rutina G-code", srv), userManager_(userMgr), routineManager_(routineMgr) {}
    
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override {
        if (params.size() != 2) {
            throw XmlRpc::XmlRpcException("routineGet requiere 2 parámetros: token, routineId");
        }
        
        std::string token = static_cast<std::string>(params[0]);
        int routineId = static_cast<int>(params[1]);
        
        try {
            // Verificar token
            if (!userManager_->validateToken(token)) {
                result["success"] = false;
                result["message"] = "Token inválido";
                return;
            }
            
            auto currentUser = userManager_->getUserByToken(token);
            if (!currentUser) {
                result["success"] = false;
                result["message"] = "Usuario no encontrado";
                return;
            }
            
            auto routine = routineManager_->getRoutineById(routineId, currentUser->id, currentUser->role);
            if (!routine) {
                result["success"] = false;
                result["message"] = "Rutina no encontrada o sin permisos";
                return;
            }
            
            result["success"] = true;
            result["routine"]["id"] = routine->id;
            result["routine"]["filename"] = routine->filename;
            result["routine"]["originalFilename"] = routine->originalFilename;
            result["routine"]["description"] = routine->description;
            result["routine"]["gcodeContent"] = routine->gcodeContent;
            result["routine"]["fileSize"] = routine->fileSize;
            result["routine"]["userId"] = routine->userId;
            result["routine"]["createdAt"] = static_cast<int>(routine->createdAt);
            
        } catch (const std::exception& e) {
            result["success"] = false;
            result["message"] = std::string("Error: ") + e.what();
        }
    }
};

/**
 * @brief Método XML-RPC para actualizar rutina G-code
 */
class RoutineUpdateMethod : public ServiceMethod {
private:
    std::shared_ptr<UserManager> userManager_;
    std::shared_ptr<RoutineManager> routineManager_;
    
public:
    RoutineUpdateMethod(XmlRpc::XmlRpcServer* srv, std::shared_ptr<UserManager> userMgr, std::shared_ptr<RoutineManager> routineMgr)
        : ServiceMethod("routineUpdate", "Actualizar rutina G-code", srv), userManager_(userMgr), routineManager_(routineMgr) {}
    
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override {
        if (params.size() != 5) {
            throw XmlRpc::XmlRpcException("routineUpdate requiere 5 parámetros: token, routineId, filename, description, gcodeContent");
        }
        
        std::string token = static_cast<std::string>(params[0]);
        int routineId = static_cast<int>(params[1]);
        std::string filename = static_cast<std::string>(params[2]);
        std::string description = static_cast<std::string>(params[3]);
        std::string gcodeContent = static_cast<std::string>(params[4]);
        
        try {
            // Verificar token
            if (!userManager_->validateToken(token)) {
                result["success"] = false;
                result["message"] = "Token inválido";
                return;
            }
            
            auto currentUser = userManager_->getUserByToken(token);
            if (!currentUser) {
                result["success"] = false;
                result["message"] = "Usuario no encontrado";
                return;
            }
            
            bool success = routineManager_->updateRoutine(routineId, filename, description, gcodeContent, currentUser->id, currentUser->role);
            
            result["success"] = success;
            result["message"] = success ? "Rutina actualizada exitosamente" : "Error actualizando rutina o sin permisos";
            
        } catch (const std::exception& e) {
            result["success"] = false;
            result["message"] = std::string("Error: ") + e.what();
        }
    }
};

/**
 * @brief Método XML-RPC para eliminar rutina G-code
 */
class RoutineDeleteMethod : public ServiceMethod {
private:
    std::shared_ptr<UserManager> userManager_;
    std::shared_ptr<RoutineManager> routineManager_;
    
public:
    RoutineDeleteMethod(XmlRpc::XmlRpcServer* srv, std::shared_ptr<UserManager> userMgr, std::shared_ptr<RoutineManager> routineMgr)
        : ServiceMethod("routineDelete", "Eliminar rutina G-code", srv), userManager_(userMgr), routineManager_(routineMgr) {}
    
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override {
        if (params.size() != 2) {
            throw XmlRpc::XmlRpcException("routineDelete requiere 2 parámetros: token, routineId");
        }
        
        std::string token = static_cast<std::string>(params[0]);
        int routineId = static_cast<int>(params[1]);
        
        try {
            // Verificar token
            if (!userManager_->validateToken(token)) {
                result["success"] = false;
                result["message"] = "Token inválido";
                return;
            }
            
            auto currentUser = userManager_->getUserByToken(token);
            if (!currentUser) {
                result["success"] = false;
                result["message"] = "Usuario no encontrado";
                return;
            }
            
            bool success = routineManager_->deleteRoutine(routineId, currentUser->id, currentUser->role);
            
            result["success"] = success;
            result["message"] = success ? "Rutina eliminada exitosamente" : "Error eliminando rutina o sin permisos";
            
        } catch (const std::exception& e) {
            result["success"] = false;
            result["message"] = std::string("Error: ") + e.what();
        }
    }
};

/**
 * @brief Método XML-RPC para generar G-code desde una secuencia de movimientos
 */
class GenerateGcodeFromMovementsMethod : public ServiceMethod {
private:
    std::shared_ptr<UserManager> userManager_;
    std::shared_ptr<RoutineManager> routineManager_;
    
public:
    GenerateGcodeFromMovementsMethod(XmlRpc::XmlRpcServer* srv, std::shared_ptr<UserManager> userMgr, std::shared_ptr<RoutineManager> routineMgr)
        : ServiceMethod("generateGcodeFromMovements", "Generar G-code desde movimientos y guardarlo como rutina", srv), 
          userManager_(userMgr), routineManager_(routineMgr) {}
    
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override {
        if (params.size() != 4) {
            throw XmlRpc::XmlRpcException("generateGcodeFromMovements requiere 4 parámetros: token, routineName, description, movements");
        }
        
        std::string token = static_cast<std::string>(params[0]);
        std::string routineName = static_cast<std::string>(params[1]);
        std::string description = static_cast<std::string>(params[2]);
        XmlRpc::XmlRpcValue movements = params[3];
        
        try {
            // Verificar token
            if (!userManager_->validateToken(token)) {
                result["success"] = false;
                result["message"] = "Token inválido";
                return;
            }
            
            auto currentUser = userManager_->getUserByToken(token);
            if (!currentUser) {
                result["success"] = false;
                result["message"] = "Usuario no encontrado";
                return;
            }
            
            // Generar G-code desde los movimientos
            std::ostringstream gcode;
            gcode << "; Trayectoria aprendida: " << routineName << "\n";
            gcode << "; Generado automáticamente desde modo aprendizaje\n";
            gcode << "; Descripción: " << description << "\n\n";
            
            gcode << "G28 ; Home all axes\n";
            gcode << "G90 ; Absolute positioning\n";
            gcode << "G21 ; Units in millimeters\n\n";
            
            // Procesar cada movimiento
            for (int i = 0; i < movements.size(); ++i) {
                auto move = movements[i];
                
                if (move.hasMember("x") && move.hasMember("y") && move.hasMember("z")) {
                    double x = double(move["x"]);
                    double y = double(move["y"]);
                    double z = double(move["z"]);
                    double feedrate = move.hasMember("feedrate") ? double(move["feedrate"]) : 1000.0;
                    bool endEffector = move.hasMember("endEffectorActive") ? bool(move["endEffectorActive"]) : false;
                    
                    gcode << "; Punto " << (i + 1) << "\n";
                    gcode << "G1 X" << std::fixed << std::setprecision(3) << x 
                          << " Y" << y << " Z" << z << " F" << feedrate << "\n";
                    
                    // Control del efector final si cambió de estado
                    if (i == 0 || (i > 0 && bool(movements[i-1]["endEffectorActive"]) != endEffector)) {
                        gcode << (endEffector ? "M106 ; End effector ON\n" : "M107 ; End effector OFF\n");
                    }
                    
                    if (move.hasMember("notes") && !std::string(move["notes"]).empty()) {
                        gcode << "; " << std::string(move["notes"]) << "\n";
                    }
                    gcode << "\n";
                }
            }
            
            gcode << "M107 ; End effector OFF\n";
            gcode << "G28 ; Return to home\n";
            gcode << "M18 ; Disable steppers\n";
            
            std::string gcodeContent = gcode.str();
            
            // Guardar como rutina en la base de datos
            std::string filename = routineName + ".gcode";
            int routineId = routineManager_->createRoutine(filename, filename, description, gcodeContent, currentUser->id);
            
            if (routineId > 0) {
                result["success"] = true;
                result["routineId"] = routineId;
                result["filename"] = filename;
                result["gcodeContent"] = gcodeContent;
                result["message"] = "Trayectoria guardada exitosamente como rutina G-code";
            } else {
                result["success"] = false;
                result["message"] = "Error guardando la rutina (posiblemente ya existe)";
            }
            
        } catch (const std::exception& e) {
            result["success"] = false;
            result["message"] = std::string("Error: ") + e.what();
        }
    }
};

/**
 * @brief Implementación del método initializeAuthMethods() para ServerModel
 */
inline void ServerModel::initializeAuthMethods() {
    try {
        // Métodos de autenticación y usuarios
        methods.push_back(std::make_unique<AuthLoginMethod>(server.get(), userManager_));
        methods.push_back(std::make_unique<AuthLogoutMethod>(server.get(), userManager_));
        methods.push_back(std::make_unique<UserCreateMethod>(server.get(), userManager_));
        methods.push_back(std::make_unique<UserListMethod>(server.get(), userManager_));
        methods.push_back(std::make_unique<UserInfoMethod>(server.get(), userManager_));
        methods.push_back(std::make_unique<UserUpdateMethod>(server.get(), userManager_));
        methods.push_back(std::make_unique<UserDeleteMethod>(server.get(), userManager_));
        
        // Métodos de rutinas G-code
        auto routineManager = std::make_shared<RoutineManager>(databaseManager_);
        methods.push_back(std::make_unique<RoutineCreateMethod>(server.get(), userManager_, routineManager));
        methods.push_back(std::make_unique<RoutineListMethod>(server.get(), userManager_, routineManager));
        methods.push_back(std::make_unique<RoutineGetMethod>(server.get(), userManager_, routineManager));
        methods.push_back(std::make_unique<RoutineUpdateMethod>(server.get(), userManager_, routineManager));
        methods.push_back(std::make_unique<RoutineDeleteMethod>(server.get(), userManager_, routineManager));
        
        // Método para generar G-code desde movimientos aprendidos
        methods.push_back(std::make_unique<GenerateGcodeFromMovementsMethod>(server.get(), userManager_, routineManager));
    } catch (const std::exception& e) {
        throw ServerInitializationException("Falló la inicialización de métodos de autenticación: " + std::string(e.what()));
    }
}

} // namespace RPCServer

#endif // _SERVER_MODEL_H_
