#pragma once

#include <memory>
#include <string>

#include "server/ServiceMethod.h"

namespace RPCServer {

class UserManager;
class RoutineManager;

class RoutineCreateMethod : public ServiceMethod {
private:
    std::shared_ptr<UserManager> userManager_;
    std::shared_ptr<RoutineManager> routineManager_;

public:
    RoutineCreateMethod(XmlRpc::XmlRpcServer* server,
                        std::shared_ptr<UserManager> userManager,
                        std::shared_ptr<RoutineManager> routineManager);
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override;
};

} // namespace RPCServer
