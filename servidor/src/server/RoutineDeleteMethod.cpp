#include "server/RoutineDeleteMethod.h"

#include "server/RoutineManager.h"
#include "server/UserManager.h"

namespace RPCServer {

RoutineDeleteMethod::RoutineDeleteMethod(XmlRpc::XmlRpcServer* server,
                                         std::shared_ptr<UserManager> userManager,
                                         std::shared_ptr<RoutineManager> routineManager)
    : ServiceMethod("routineDelete", "Eliminar rutina G-code", server),
      userManager_(std::move(userManager)),
      routineManager_(std::move(routineManager)) {}

void RoutineDeleteMethod::execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) {
    if (params.size() != 2) {
        throw XmlRpc::XmlRpcException("routineDelete requiere 2 parámetros: token, routineId");
    }

    std::string token = static_cast<std::string>(params[0]);
    int routineId = static_cast<int>(params[1]);

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

        bool success = routineManager_->deleteRoutine(routineId, currentUser->id, currentUser->role);

        result["success"] = success;
        result["message"] = success ? "Rutina eliminada exitosamente" : "Error eliminando rutina o sin permisos";

    } catch (const std::exception& e) {
        result["success"] = false;
        result["message"] = std::string("Error: ") + e.what();
    }
}

} // namespace RPCServer
