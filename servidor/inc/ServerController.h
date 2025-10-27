#ifndef _SERVER_CONTROLLER_H_
#define _SERVER_CONTROLLER_H_

#include <iostream>
#include <memory>
#include <string>
#include "ServerModel.h"
#include "RPCExceptions.h"

namespace RPCServer {

/**
 * @brief Clase controladora para operaciones del servidor - separa presentación del modelo
 */
class ServerController {
private:
    std::unique_ptr<ServerModel> model;

    void displayUsage(const std::string& programName) const {
        std::cerr << "Uso: " << programName << " <puerto>\n";
        std::cerr << "  puerto: Puerto en el que el servidor escuchará conexiones\n";
        std::cerr << "Ejemplo: " << programName << " 8080\n";
    }

    void displayStartupInfo() const {
        std::cout << "=== Servidor RPC Iniciado ===" << std::endl;
        std::cout << "Puerto: " << model->getPort() << std::endl;
        std::cout << "Métodos disponibles:" << std::endl;
        std::cout << "  - ServerTest: Prueba de conexión" << std::endl;
        std::cout << "  - Eco: Echo con saludo personalizado" << std::endl;
        std::cout << "  - Sumar: Suma de números reales" << std::endl;
        std::cout << "Presione Ctrl+C para detener el servidor" << std::endl;
        std::cout << "==============================" << std::endl;
    }

    void displayError(const std::string& error) const {
        std::cerr << "ERROR: " << error << std::endl;
    }

    int parsePort(const std::string& portStr) const {
        try {
            int port = std::stoi(portStr);
            if (port <= 0 || port > 65535) {
                throw RPCServer::InvalidParametersException("parsePort", "Puerto debe estar entre 1 y 65535");
            }
            return port;
        } catch (const std::invalid_argument&) {
            throw RPCServer::InvalidParametersException("parsePort", "Puerto debe ser un número válido");
        } catch (const std::out_of_range&) {
            throw RPCServer::InvalidParametersException("parsePort", "Puerto fuera de rango válido");
        }
    }

public:
    ServerController() = default;

    int run(int argc, char* argv[]) {
        try {
            // Validar argumentos
            if (argc != 2) {
                displayUsage(argv[0]);
                return 1;
            }

            // Analizar puerto
            int port = parsePort(argv[1]);

            // Crear modelo con configuración
            auto config = std::make_unique<ServerConfig>(port, true, 5);
            model = std::make_unique<ServerModel>(std::move(config));

            // Iniciar servidor
            model->start();
            displayStartupInfo();

            // Ejecutar servidor
            model->run();

            return 0;

        } catch (const RPCException& e) {
            displayError(e.what());
            return 2;
        } catch (const std::exception& e) {
            displayError("Error inesperado: " + std::string(e.what()));
            return 3;
        }
    }

    void shutdown() {
        if (model && model->getIsRunning()) {
            model->stop();
            std::cout << "Servidor detenido correctamente." << std::endl;
        }
    }

    ~ServerController() {
        shutdown();
    }
};

} // namespace RPCServer

#endif // _SERVER_CONTROLLER_H_
