#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include <sstream>
#include <memory>
#include <string>
#include <vector>
#include <map>

// Minimal stubs and simplified XmlRpcValue representation for tests
// The real project uses a custom XmlRpc library; to keep tests lightweight we mock behavior

namespace XmlRpc {
    class XmlRpcValue {
    public:
        enum Type { TYPE_INT, TYPE_DOUBLE, TYPE_STRING, TYPE_BOOL, TYPE_ARRAY, TYPE_STRUCT };
        Type type = TYPE_STRING;
        std::string s;
        double d = 0.0;
        int i = 0;
        bool b = false;
        std::vector<XmlRpcValue> arr;
        std::map<std::string, XmlRpcValue> members;

        XmlRpcValue() {}
        XmlRpcValue(int v) { type = TYPE_INT; i = v; }
        XmlRpcValue(double v) { type = TYPE_DOUBLE; d = v; }
        XmlRpcValue(const std::string& v) { type = TYPE_STRING; s = v; }
        XmlRpcValue(const char* v) { type = TYPE_STRING; s = v ? v : std::string(); }
        XmlRpcValue(bool v) { type = TYPE_BOOL; b = v; }

        void setSize(size_t n) { arr.resize(n); }
        size_t size() const { return arr.size(); }
        XmlRpcValue& operator[](size_t idx) { return arr[idx]; }
        XmlRpcValue& operator[](const std::string& key) { return members[key]; }
        bool hasMember(const std::string& key) const { return members.find(key) != members.end(); }
        operator std::string() const { return s; }
        operator double() const { return d; }
        operator int() const { return i; }
        operator bool() const { return b; }
    };

    class XmlRpcServer {};
    class XmlRpcException : public std::exception { public: XmlRpcException(const std::string& m):msg(m){} const char* what() const noexcept override { return msg.c_str(); } std::string msg; };
}

// Simplified versions of classes referenced by ServerModel
namespace RPCServer {
    struct UserManagerStub {
        bool validateToken(const std::string& token) { return token == "valid-token"; }
        struct UserInfo { int id; std::string username; std::string role; };
        UserInfo getUserByToken(const std::string& token) { return {1, "testuser", "OPERATOR"}; }
    };

    struct RoutineManagerStub {
        int createRoutine(const std::string& filename, const std::string& originalFilename,
                          const std::string& description, const std::string& gcodeContent, int userId) {
            // Return a fake routine id if content non-empty
            return gcodeContent.empty() ? -1 : 42;
        }
    };

    class GenerateGcodeFromMovementsMethod {
    private:
        std::shared_ptr<UserManagerStub> userManager_;
        std::shared_ptr<RoutineManagerStub> routineManager_;
    public:
        GenerateGcodeFromMovementsMethod(std::shared_ptr<UserManagerStub> u, std::shared_ptr<RoutineManagerStub> r)
            : userManager_(u), routineManager_(r) {}

        void execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) {
            if (params.size() != 4) throw XmlRpc::XmlRpcException("Invalid params");
            std::string token = std::string(params[0]);
            std::string routineName = std::string(params[1]);
            std::string description = std::string(params[2]);
            XmlRpc::XmlRpcValue movements = params[3];

            if (!userManager_->validateToken(token)) {
                result[std::string("success")] = XmlRpc::XmlRpcValue(false);
                result[std::string("message")] = XmlRpc::XmlRpcValue(std::string("Token inválido"));
                return;
            }

            std::ostringstream gcode;
            // Create a simple gcode from movements array where each element is a struct with x,y,z
            for (size_t i = 0; i < movements.size(); ++i) {
                XmlRpc::XmlRpcValue mv = movements[i];
                double x = 0.0, y = 0.0, z = 0.0;
                if (mv.hasMember("x")) x = double(mv.members.at("x"));
                if (mv.hasMember("y")) y = double(mv.members.at("y"));
                if (mv.hasMember("z")) z = double(mv.members.at("z"));
                gcode << "G1 X" << x << " Y" << y << " Z" << z << "\n";
            }

            std::string gcodeStr = gcode.str();
            int routineId = routineManager_->createRoutine(routineName, routineName, description, gcodeStr, 1);
            if (routineId < 0) {
                result[std::string("success")] = XmlRpc::XmlRpcValue(false);
                result[std::string("message")] = XmlRpc::XmlRpcValue(std::string("Failed to create routine"));
                return;
            }

            result[std::string("success")] = XmlRpc::XmlRpcValue(true);
            result[std::string("routine_id")] = XmlRpc::XmlRpcValue(routineId);
            result[std::string("gcode")] = XmlRpc::XmlRpcValue(gcodeStr);
        }
    };
}

TEST_CASE("GenerateGcodeFromMovementsMethod success") {
    using namespace XmlRpc;
    using namespace RPCServer;

    auto userMgr = std::make_shared<UserManagerStub>();
    auto routineMgr = std::make_shared<RoutineManagerStub>();
    GenerateGcodeFromMovementsMethod method(userMgr, routineMgr);

    XmlRpcValue params; // we'll use params.arr as the array of params
    params.setSize(4);
    params[0] = XmlRpcValue(std::string("valid-token"));
    params[1] = XmlRpcValue(std::string("routine1"));
    params[2] = XmlRpcValue(std::string("desc"));

    XmlRpcValue movements;
    movements.setSize(2);
    movements[0].members["x"] = XmlRpcValue(1.0);
    movements[0].members["y"] = XmlRpcValue(2.0);
    movements[0].members["z"] = XmlRpcValue(3.0);
    movements[1].members["x"] = XmlRpcValue(4.0);
    movements[1].members["y"] = XmlRpcValue(5.0);
    movements[1].members["z"] = XmlRpcValue(6.0);

    params[3] = movements;

    XmlRpcValue result;
    method.execute(params, result);

    CHECK(result.hasMember("success"));
    CHECK(bool(result.members.at("success")) == true);
    CHECK(result.hasMember("routine_id"));
    CHECK(int(result.members.at("routine_id")) == 42);
    CHECK(result.hasMember("gcode"));
    std::string g = std::string(result.members.at("gcode"));
    CHECK(g.find("G1 X1 Y2 Z3") != std::string::npos);
}
