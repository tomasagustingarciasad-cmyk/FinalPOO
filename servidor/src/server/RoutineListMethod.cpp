#include "server/RoutineListMethod.h"

#include "server/RoutineManager.h"
#include "server/UserManager.h"

namespace RPCServer {

RoutineListMethod::RoutineListMethod(XmlRpc::XmlRpcServer* server,
                                     std::shared_ptr<UserManager> userManager,
                                     std::shared_ptr<RoutineManager> routineManager)
    : ServiceMethod("routineList", "Listar rutinas G-code", server),
      userManager_(std::move(userManager)),
      routineManager_(std::move(routineManager)) {}

void RoutineListMethod::execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) {
    if (params.size() != 1) {
        throw XmlRpc::XmlRpcException("routineList requiere 1 parámetro: token");
    }

    std::string token = static_cast<std::string>(params[0]);

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

        auto routines = routineManager_->getAllRoutines(currentUser->id, currentUser->role);

        result["success"] = true;
        result["routines"].setSize(routines.size());

        for (size_t i = 0; i < routines.size(); ++i) {
            result["routines"][i]["id"] = routines[i].id;
            result["routines"][i]["filename"] = routines[i].filename;
            result["routines"][i]["originalFilename"] = routines[i].originalFilename;
            result["routines"][i]["description"] = routines[i].description;
            result["routines"][i]["fileSize"] = routines[i].fileSize;
            result["routines"][i]["userId"] = routines[i].userId;
            result["routines"][i]["createdAt"] = static_cast<int>(routines[i].createdAt);
        }

    } catch (const std::exception& e) {
        result["success"] = false;
        result["message"] = std::string("Error: ") + e.what();
    }
}

} // namespace RPCServer
