#include "server/RoutineUpdateMethod.h"

#include "server/RoutineManager.h"
#include "server/UserManager.h"

namespace RPCServer {

RoutineUpdateMethod::RoutineUpdateMethod(XmlRpc::XmlRpcServer* server,
                                         std::shared_ptr<UserManager> userManager,
                                         std::shared_ptr<RoutineManager> routineManager)
    : ServiceMethod("routineUpdate", "Actualizar rutina G-code", server),
      userManager_(std::move(userManager)),
      routineManager_(std::move(routineManager)) {}

void RoutineUpdateMethod::execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) {
    if (params.size() != 5) {
        throw XmlRpc::XmlRpcException("routineUpdate requiere 5 parámetros: token, routineId, filename, description, gcodeContent");
    }

    std::string token = static_cast<std::string>(params[0]);
    int routineId = static_cast<int>(params[1]);
    std::string filename = static_cast<std::string>(params[2]);
    std::string description = static_cast<std::string>(params[3]);
    std::string gcodeContent = static_cast<std::string>(params[4]);

    try {
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

} // namespace RPCServer
