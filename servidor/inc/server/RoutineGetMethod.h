#pragma once

#include <memory>

#include "server/ServiceMethod.h"

namespace RPCServer {

class UserManager;
class RoutineManager;

class RoutineGetMethod : public ServiceMethod {
private:
    std::shared_ptr<UserManager> userManager_;
    std::shared_ptr<RoutineManager> routineManager_;

public:
    RoutineGetMethod(XmlRpc::XmlRpcServer* server,
                     std::shared_ptr<UserManager> userManager,
                     std::shared_ptr<RoutineManager> routineManager);
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override;
};

} // namespace RPCServer
