#include "server/MoveMethod.h"

#include "Robot.h"
#include "RPCExceptions.h"

namespace RPCServer {

MoveMethod::MoveMethod(XmlRpc::XmlRpcServer* server, Robot* robot)
    : ServiceMethod("move", "Movimiento cartesiano", server), robot_(robot) {}

void MoveMethod::execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) {
    std::string clientIP = getClientIP();
    std::string user = "unknown";

    try {
        if (params.size() < 4) {
            LOG_WARNING("MoveMethod", "Parámetros insuficientes para move");
            logRequest(user, clientIP, 400, "Insufficient parameters");
            throw InvalidParametersException("move", "x:double, y:double, z:double, vel:double");
        }

        if (!robot_->isConnected()) {
            LOG_WARNING("MoveMethod", "Intento de movimiento sin robot conectado");
            logRequest(user, clientIP, 409, "Robot not connected");
            result["ok"] = false;
            result["message"] = "No conectado";
            return;
        }

        auto toDouble = [](XmlRpc::XmlRpcValue& val) -> double {
            if (val.getType() == XmlRpc::XmlRpcValue::TypeInt) {
                int intVal = val;
                return static_cast<double>(intVal);
            }
            return val;
        };

        const double x = toDouble(params[0]);
        const double y = toDouble(params[1]);
        const double z = toDouble(params[2]);
        const double vel = toDouble(params[3]);

        LOG_DEBUG("MoveMethod", "execute", "Ejecutando movimiento",
                  "X:" + std::to_string(x) + " Y:" + std::to_string(y) +
                  " Z:" + std::to_string(z) + " Vel:" + std::to_string(vel));

        const bool ok = robot_->move(x, y, z, vel);

        if (ok) {
            LOG_INFO("MoveMethod", "Movimiento ejecutado exitosamente");
            logRequest(user, clientIP, 200, "Movement successful - X:" + std::to_string(x) +
                                               " Y:" + std::to_string(y) +
                                               " Z:" + std::to_string(z));
        } else {
            LOG_WARNING("MoveMethod", "Fallo en ejecución de movimiento");
            logRequest(user, clientIP, 500, "Movement failed");
        }

        result["ok"] = ok;
        result["message"] = ok ? "Movimiento enviado" : "Fallo move";

    } catch (const std::exception& e) {
        LOG_ERROR("MoveMethod", "MOVE_EXCEPTION", "Error durante movimiento", e.what());
        logRequest(user, clientIP, 500, "Move exception: " + std::string(e.what()));
        throw MethodExecutionException("move", e.what());
    }
}

} // namespace RPCServer
