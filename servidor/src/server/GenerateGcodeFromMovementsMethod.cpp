#include "server/GenerateGcodeFromMovementsMethod.h"

#include "Robot.h"
#include "server/RoutineManager.h"
#include "server/UserManager.h"

namespace RPCServer {

GenerateGcodeFromMovementsMethod::GenerateGcodeFromMovementsMethod(
    XmlRpc::XmlRpcServer* server,
    std::shared_ptr<UserManager> userManager,
    std::shared_ptr<RoutineManager> routineManager,
    Robot* robot)
    : ServiceMethod("generateGcodeFromMovements", "Generar G-code desde movimientos y guardarlo como rutina", server),
      userManager_(std::move(userManager)),
      routineManager_(std::move(routineManager)),
      robot_(robot) {}

void GenerateGcodeFromMovementsMethod::execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) {
    if (params.size() != 3) {
        throw XmlRpc::XmlRpcException("generateGcodeFromMovements requiere 3 parámetros: token, routineName, description");
    }

    std::string token = static_cast<std::string>(params[0]);
    std::string routineName = static_cast<std::string>(params[1]);
    std::string description = static_cast<std::string>(params[2]);

    try {
        if (!robot_->isConnected()) {
            result["success"] = false;
            result["message"] = "No conectado";
            return;
        }

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

        bool learning = robot_->robotLearning(routineName, description);
        if (learning) {
            result["success"] = true;
            result["message"] = "Aprendizaje Activado";
            return;
        }

        std::string gcodeContent = robot_->getLearnedGcode();
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

} // namespace RPCServer
