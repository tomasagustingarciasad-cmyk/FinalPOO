#include "server/EchoMethod.h"
#include "RPCExceptions.h"

namespace RPCServer {

EchoMethod::EchoMethod(XmlRpc::XmlRpcServer* server)
    : ServiceMethod("Eco", "Diga algo y recibira un saludo", server) {}

void EchoMethod::execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) {
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

} // namespace RPCServer
