#ifndef _SERVER_MODEL_H_
#define _SERVER_MODEL_H_

#include <string>
#include <vector>
#include <memory>
#include "../lib/XmlRpc.h"
#include "RPCExceptions.h"
#include "Robot.h"

namespace RPCServer {

// Forward declarations
class ServiceMethod;
class Robot;

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

public:
    ServiceMethod(const std::string& name, const std::string& description, XmlRpc::XmlRpcServer* server)
        : XmlRpc::XmlRpcServerMethod(name, server), methodDescription(description) {}

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
            result = "Hi, soy el servidor RPC !!";
        } catch (const std::exception& e) {
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

// ========== Métodos del Robot ==========

class ConnectRobotMethod : public ServiceMethod {
    Robot* robot;
public:
    ConnectRobotMethod(XmlRpc::XmlRpcServer* server, Robot* r)
      : ServiceMethod("connectRobot", "Conecta al puerto serie del robot", server), robot(r) {}
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override {
        try {
            if (params.size() < 1) throw InvalidParametersException("connectRobot", "port:string [, baud:int=115200]");
            std::string port = std::string(params[0]);
            int baud = 115200;
            if (params.size() >= 2) baud = int(params[1]);
            if (robot->connect(port, baud)) { result["ok"] = true; result["message"] = "Conectado"; }
            else { result["ok"] = false; result["message"] = "Fallo conectando"; }
        } catch (const std::exception& e) {
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
            bool on = bool(params[0]);
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
        try {
            if (params.size() < 4) throw InvalidParametersException("move", "x:double, y:double, z:double, vel:double");
            if (!robot->isConnected()) { result["ok"]=false; result["message"]="No conectado"; return; }
            double x = double(params[0]), y = double(params[1]), z = double(params[2]), vel = double(params[3]);
            bool ok = robot->move(x,y,z,vel);
            result["ok"]=ok; result["message"]= ok ? "Movimiento enviado" : "Fallo move";
        } catch (const std::exception& e) { throw MethodExecutionException("move", e.what()); }
    }
};

class EndEffectorMethod : public ServiceMethod {
    Robot* robot;
public:
    EndEffectorMethod(XmlRpc::XmlRpcServer* server, Robot* r)
      : ServiceMethod("endEffector", "Activa/Desactiva efector final", server), robot(r) {}
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override {
        try {
            if (params.size() < 1) throw InvalidParametersException("endEffector", "on:bool");
            if (!robot->isConnected()) { result["ok"]=false; result["message"]="No conectado"; return; }
            bool on = bool(params[0]);
            bool ok = robot->endEffector(on);
            result["ok"]=ok; result["message"]= ok ? (on?"Efector ON":"Efector OFF") : "Fallo endEffector";
        } catch (const std::exception& e) { throw MethodExecutionException("endEffector", e.what()); }
    }
};

class GetPositionMethod : public ServiceMethod {
    Robot* robot;
public:
    GetPositionMethod(XmlRpc::XmlRpcServer* server, Robot* r)
      : ServiceMethod("getPosition", "Obtiene posición actual del robot (M114)", server), robot(r) {}
    void execute(XmlRpc::XmlRpcValue& /*params*/, XmlRpc::XmlRpcValue& result) override {
        try {
            if (!robot->isConnected()) { result["ok"]=false; result["message"]="No conectado"; return; }
            auto pos = robot->getPosition();
            result["ok"] = pos.valid;
            if (pos.valid) {
                result["mode"] = pos.mode;
                result["x"] = pos.x;
                result["y"] = pos.y;
                result["z"] = pos.z;
                result["e"] = pos.e;
                result["motorsEnabled"] = pos.motorsEnabled;
                result["fanEnabled"] = pos.fanEnabled;
                result["message"] = "Posición obtenida";
            } else {
                result["message"] = "No se pudo obtener posición";
            }
        } catch (const std::exception& e) { throw MethodExecutionException("getPosition", e.what()); }
    }
};

class GetEndstopsMethod : public ServiceMethod {
    Robot* robot;
public:
    GetEndstopsMethod(XmlRpc::XmlRpcServer* server, Robot* r)
      : ServiceMethod("getEndstops", "Obtiene estado de endstops (M119)", server), robot(r) {}
    void execute(XmlRpc::XmlRpcValue& /*params*/, XmlRpc::XmlRpcValue& result) override {
        try {
            if (!robot->isConnected()) { result["ok"]=false; result["message"]="No conectado"; return; }
            auto status = robot->getEndstops();
            result["ok"] = status.valid;
            if (status.valid) {
                result["xState"] = status.xState;
                result["yState"] = status.yState;
                result["zState"] = status.zState;
                result["message"] = "Endstops obtenidos";
            } else {
                result["message"] = "No se pudo obtener endstops";
            }
        } catch (const std::exception& e) { throw MethodExecutionException("getEndstops", e.what()); }
    }
};

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

public:
    ServerModel(std::unique_ptr<ServerConfig> serverConfig)
      : config(std::move(serverConfig)), isRunning(false) {
        server = std::make_unique<XmlRpc::XmlRpcServer>();
        robot_ = std::make_unique<Robot>(); // inicializar robot
        initializeMethods();
    }
    void initializeMethods() {
        try {
            methods.push_back(std::make_unique<ServerTestMethod>(server.get()));
            methods.push_back(std::make_unique<EchoMethod>(server.get()));
            methods.push_back(std::make_unique<SumMethod>(server.get()));
            // Métodos del Robot (UML ServidorRPC)
            methods.push_back(std::make_unique<ConnectRobotMethod>(server.get(), robot_.get()));
            methods.push_back(std::make_unique<DisconnectRobotMethod>(server.get(), robot_.get()));
            methods.push_back(std::make_unique<SetModeMethod>(server.get(), robot_.get()));
            methods.push_back(std::make_unique<EnableMotorsMethod>(server.get(), robot_.get()));
            methods.push_back(std::make_unique<HomeMethod>(server.get(), robot_.get()));
            methods.push_back(std::make_unique<MoveMethod>(server.get(), robot_.get()));
            methods.push_back(std::make_unique<EndEffectorMethod>(server.get(), robot_.get()));
            methods.push_back(std::make_unique<GetPositionMethod>(server.get(), robot_.get()));
            methods.push_back(std::make_unique<GetEndstopsMethod>(server.get(), robot_.get()));
        } catch (const std::exception& e) {
            throw ServerInitializationException("Falló la inicialización de métodos: " + std::string(e.what()));
        }
    }

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

} // namespace RPCServer

#endif // _SERVER_MODEL_H_
