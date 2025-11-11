#include "server/ExecuteGcodeMethod.h"

#include <sstream>
#include <string>
#include <vector>

#include "Robot.h"
#include "RPCExceptions.h"

namespace RPCServer {

ExecuteGcodeMethod::ExecuteGcodeMethod(XmlRpc::XmlRpcServer* server, Robot* robot)
    : ServiceMethod("executeGcode", "Ejecuta código G-code en el robot", server), robot_(robot) {}

void ExecuteGcodeMethod::execute(XmlRpc::XmlRpcValue& params, XmlRpc::XmlRpcValue& result) {
    try {
        if (params.size() < 1) {
            throw InvalidParametersException("executeGcode", "gcode:string");
        }

        if (!robot_->isConnected()) {
            result["success"] = false;
            result["message"] = "Robot no conectado";
            return;
        }

        std::string gcode = std::string(params[0]);

        LOG_DEBUG("ExecuteGcodeMethod", "execute", "Iniciando ejecución de G-code",
                  "Longitud: " + std::to_string(gcode.length()) + " bytes");

        std::istringstream stream(gcode);
        std::string line;
        int lineCount = 0;
        int successCount = 0;
        int skippedCount = 0;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;

        while (std::getline(stream, line)) {
            lineCount++;

            size_t commentPos = line.find(';');
            if (commentPos != std::string::npos) {
                line = line.substr(0, commentPos);
            }

            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);

            if (line.empty()) {
                skippedCount++;
                continue;
            }

            LOG_DEBUG("ExecuteGcodeMethod", "execute",
                      "Ejecutando línea " + std::to_string(lineCount),
                      "Comando: " + line);

            bool cmdSuccess = robot_->sendGcodeCommand(line);

            if (cmdSuccess) {
                successCount++;
                LOG_DEBUG("ExecuteGcodeMethod", "execute",
                          "Línea ejecutada exitosamente",
                          "Línea " + std::to_string(lineCount) + ": " + line);
            } else {
                std::string warnMsg = "Línea " + std::to_string(lineCount) + ": " + line;
                warnings.push_back(warnMsg);
                LOG_WARNING("ExecuteGcodeMethod",
                           "Advertencia en línea " + std::to_string(lineCount) + ": " + line);

                if (line.rfind("G28", 0) == 0 || line.rfind("M17", 0) == 0) {
                    errors.push_back("ERROR CRÍTICO - " + warnMsg);
                }
            }
        }

        if (errors.empty()) {
            result["success"] = true;
            result["message"] = "G-code ejecutado exitosamente";
            result["linesProcessed"] = successCount;
            result["linesTotal"] = lineCount;
            result["linesSkipped"] = skippedCount;

            if (!warnings.empty()) {
                result["warnings"].setSize(warnings.size());
                for (size_t i = 0; i < warnings.size(); ++i) {
                    result["warnings"][i] = warnings[i];
                }
                LOG_INFO("ExecuteGcodeMethod",
                         "G-code ejecutado con " + std::to_string(warnings.size()) + " advertencias");
            }

            LOG_SYSTEM("ExecuteGcodeMethod", LogLevel::INFO,
                       "G-code ejecutado exitosamente",
                       "Procesadas: " + std::to_string(successCount) + "/" +
                           std::to_string(lineCount - skippedCount));
        } else {
            result["success"] = false;
            result["message"] = "Algunos comandos críticos fallaron";
            result["linesProcessed"] = successCount;
            result["linesTotal"] = lineCount;
            result["linesSkipped"] = skippedCount;

            result["errors"].setSize(errors.size());
            for (size_t i = 0; i < errors.size(); ++i) {
                result["errors"][i] = errors[i];
            }

            LOG_ERROR("ExecuteGcodeMethod", "GCODE_EXECUTION_ERROR",
                      "Errores críticos durante ejecución de G-code",
                      std::to_string(errors.size()) + " errores críticos");
        }

    } catch (const std::exception& e) {
        LOG_ERROR("ExecuteGcodeMethod", "EXCEPTION", "Excepción durante executeGcode", e.what());
        throw MethodExecutionException("executeGcode", e.what());
    }
}

} // namespace RPCServer
