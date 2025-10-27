#ifndef _RPC_EXCEPTIONS_H_
#define _RPC_EXCEPTIONS_H_

#include <exception>
#include <string>

namespace RPCServer {

/**
 * @brief Clase base de excepción para operaciones RPC
 */
class RPCException : public std::exception {
protected:
    std::string message;

public:
    RPCException(const std::string& msg) : message(msg) {}
    
    virtual const char* what() const noexcept override {
        return message.c_str();
    }
    
    virtual ~RPCException() = default;
};

/**
 * @brief Excepción lanzada cuando falla la inicialización del servidor
 */
class ServerInitializationException : public RPCException {
public:
    ServerInitializationException(const std::string& msg) 
        : RPCException("Error de inicialización del servidor: " + msg) {}
};

/**
 * @brief Excepción lanzada cuando falla la ejecución de un método
 */
class MethodExecutionException : public RPCException {
public:
    MethodExecutionException(const std::string& methodName, const std::string& details)
        : RPCException("Falló la ejecución del método '" + methodName + "': " + details) {}
};

/**
 * @brief Excepción lanzada cuando se proporcionan parámetros inválidos
 */
class InvalidParametersException : public RPCException {
public:
    InvalidParametersException(const std::string& methodName, const std::string& expected)
        : RPCException("Parámetros inválidos para el método '" + methodName + "'. Se esperaba: " + expected) {}
};

/**
 * @brief Excepción lanzada cuando falla la vinculación del servidor
 */
class ServerBindingException : public RPCException {
public:
    ServerBindingException(int port, const std::string& details)
        : RPCException("Falló la vinculación del servidor al puerto " + std::to_string(port) + ": " + details) {}
};

} // namespace RPCServer

namespace RPCClient {

/**
 * @brief Clase base de excepción para operaciones del cliente RPC
 */
class ClientException : public std::exception {
protected:
    std::string message;

public:
    ClientException(const std::string& msg) : message(msg) {}
    
    virtual const char* what() const noexcept override {
        return message.c_str();
    }
    
    virtual ~ClientException() = default;
};

/**
 * @brief Excepción lanzada cuando falla la conexión del cliente
 */
class ConnectionException : public ClientException {
public:
    ConnectionException(const std::string& host, int port)
        : ClientException("Falló la conexión al servidor " + host + ":" + std::to_string(port)) {}
};

/**
 * @brief Excepción lanzada cuando falla la llamada RPC
 */
class RPCCallException : public ClientException {
public:
    RPCCallException(const std::string& methodName, const std::string& details)
        : ClientException("Falló la llamada RPC a '" + methodName + "': " + details) {}
};

/**
 * @brief Excepción lanzada cuando se proporcionan argumentos inválidos
 */
class InvalidArgumentsException : public ClientException {
public:
    InvalidArgumentsException(const std::string& details)
        : ClientException("Argumentos inválidos: " + details) {}
};

} // namespace RPCClient

#endif // _RPC_EXCEPTIONS_H_
