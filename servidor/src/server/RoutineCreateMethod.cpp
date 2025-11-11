#include "server/RoutineCreateMethod.h"

#include "server/RoutineManager.h"
#include "server/UserManager.h"

namespace RPCServer {

RoutineCreateMethod::RoutineCreateMethod(XmlRpc::XmlRpcServer* server,
                                         std::shared_ptr<UserManager> userManager,
                                         std::shared_ptr<RoutineManager> routineManager)
    : ServiceMethod("routineCreate", "Crear nueva rutina G-code", server),
      userManager_(std::move(userManager)),
      routineManager_(std::move(routineManager)) {}

void RoutineCreateMethod::execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) {
    if (params.size() != 5) {
        throw XmlRpc::XmlRpcException("routineCreate requiere 5 parámetros: token, filename, originalFilename, description, gcodeContent");
    }

    std::string token = static_cast<std::string>(params[0]);
    std::string filename = static_cast<std::string>(params[1]);
    std::string originalFilename = static_cast<std::string>(params[2]);
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

        int routineId = routineManager_->createRoutine(filename, originalFilename, description, gcodeContent, currentUser->id);

        if (routineId > 0) {
            result["success"] = true;
            result["routineId"] = routineId;
            result["message"] = "Rutina creada exitosamente";
        } else {
            result["success"] = false;
            result["message"] = "Error creando rutina (posiblemente ya existe)";
        }

    } catch (const std::exception& e) {
        result["success"] = false;
        result["message"] = std::string("Error: ") + e.what();
    }
}

} // namespace RPCServer
