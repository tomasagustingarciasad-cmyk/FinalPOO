#include "server/RoutineGetMethod.h"

#include "server/RoutineManager.h"
#include "server/UserManager.h"

namespace RPCServer {

RoutineGetMethod::RoutineGetMethod(XmlRpc::XmlRpcServer* server,
                                   std::shared_ptr<UserManager> userManager,
                                   std::shared_ptr<RoutineManager> routineManager)
    : ServiceMethod("routineGet", "Obtener rutina G-code", server),
      userManager_(std::move(userManager)),
      routineManager_(std::move(routineManager)) {}

void RoutineGetMethod::execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) {
    if (params.size() != 2) {
        throw XmlRpc::XmlRpcException("routineGet requiere 2 parámetros: token, routineId");
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

        auto routine = routineManager_->getRoutineById(routineId, currentUser->id, currentUser->role);
        if (!routine) {
            result["success"] = false;
            result["message"] = "Rutina no encontrada o sin permisos";
            return;
        }

        result["success"] = true;
        result["routine"]["id"] = routine->id;
        result["routine"]["filename"] = routine->filename;
        result["routine"]["originalFilename"] = routine->originalFilename;
        result["routine"]["description"] = routine->description;
        result["routine"]["gcodeContent"] = routine->gcodeContent;
        result["routine"]["fileSize"] = routine->fileSize;
        result["routine"]["userId"] = routine->userId;
        result["routine"]["createdAt"] = static_cast<int>(routine->createdAt);

    } catch (const std::exception& e) {
        result["success"] = false;
        result["message"] = std::string("Error: ") + e.what();
    }
}

} // namespace RPCServer
