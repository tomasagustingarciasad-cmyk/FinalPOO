#pragma once

#include <memory>
#include <string>

#include "server/ServiceMethod.h"

namespace RPCServer {

class UserManager;
class RoutineManager;
class Robot;

class GenerateGcodeFromMovementsMethod : public ServiceMethod {
private:
    std::shared_ptr<UserManager> userManager_;
    std::shared_ptr<RoutineManager> routineManager_;
    Robot* robot_;

public:
    GenerateGcodeFromMovementsMethod(XmlRpc::XmlRpcServer* server,
                                     std::shared_ptr<UserManager> userManager,
                                     std::shared_ptr<RoutineManager> routineManager,
                                     Robot* robot);
    void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) override;
};

} // namespace RPCServer
