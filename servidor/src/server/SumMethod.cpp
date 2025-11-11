#include "server/SumMethod.h"
#include "RPCExceptions.h"

namespace RPCServer {

SumMethod::SumMethod(XmlRpc::XmlRpcServer* server)
    : ServiceMethod("Sumar", "Indique varios numeros reales separados por espacio", server) {}

void SumMethod::execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) {
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

} // namespace RPCServer
