#include "server/ConnectRobotMethod.h"

#include "RPCExceptions.h"
#include "Robot.h"

namespace RPCServer {

ConnectRobotMethod::ConnectRobotMethod(XmlRpc::XmlRpcServer* server, Robot* robot)
    : ServiceMethod("connectRobot", "Conecta al puerto serie del robot", server), robot_(robot) {}

void ConnectRobotMethod::execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) {
    std::string clientIP = getClientIP();
    std::string user = "unknown";

    try {
        if (params.size() < 1) {
            LOG_WARNING("ConnectRobotMethod", "Parámetros insuficientes para connectRobot");
            throw InvalidParametersException("connectRobot", "port:string [, baud:int=9600]");
        }

        std::string port = static_cast<std::string>(params[0]);
        int baud = 115200;
        if (params.size() >= 2) {
            baud = static_cast<int>(params[1]);
        }

    g_logger.logSystem("ConnectRobotMethod", LogLevel::INFO, "Intento de conexión robot", "Puerto: " + port + " Baud: " + std::to_string(baud));

        if (robot_->connect(port, baud)) {
            robot_->enablePositionTracking(true);

            result["ok"] = true;
            result["message"] = "Conectado";
            g_logger.logSystem("ConnectRobotMethod", LogLevel::INFO, "Robot conectado exitosamente con tracking habilitado", "");
            logRequest(user, clientIP, 200, "Robot connected successfully to " + port + " with tracking enabled");
        } else {
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

} // namespace RPCServer
