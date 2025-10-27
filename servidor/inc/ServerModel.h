#ifndef _SERVER_MODEL_H_
#define _SERVER_MODEL_H_

#include <string>
#include <vector>
#include <memory>
#include "../lib/XmlRpc.h"
#include "RPCExceptions.h"

namespace RPCServer {

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

/**
 * @brief Clase modelo del servidor que gestiona el servidor RPC
 */
class ServerModel {
private:
    std::unique_ptr<XmlRpc::XmlRpcServer> server;
    std::unique_ptr<ServerConfig> config;
    std::vector<std::unique_ptr<ServiceMethod>> methods;
    bool isRunning;

public:
    ServerModel(std::unique_ptr<ServerConfig> serverConfig)
        : config(std::move(serverConfig)), isRunning(false) {
        server = std::make_unique<XmlRpc::XmlRpcServer>();
        initializeMethods();
    }

    void initializeMethods() {
        try {
            methods.push_back(std::make_unique<ServerTestMethod>(server.get()));
            methods.push_back(std::make_unique<EchoMethod>(server.get()));
            methods.push_back(std::make_unique<SumMethod>(server.get()));
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
