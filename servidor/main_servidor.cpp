/**
 * @file main_servidor.cpp
 * @brief Módulo launcher para Servidor RPC
 * @author Generado para POO TP2 Req4
 */

#include <iostream>
#include <csignal>
#include <memory>
#include "inc/ServerController.h"

// Instancia global del controlador para manejo de señales
std::unique_ptr<RPCServer::ServerController> g_serverController;

/**
 * @brief Manejador de señales para apagado controlado
 */
void signalHandler(int signal) {
    std::cout << "\n\nRecibida señal " << signal << ". Deteniendo servidor..." << std::endl;
    if (g_serverController) {
        g_serverController->shutdown();
    }
    exit(0);
}

/**
 * @brief Punto de entrada principal para la aplicación del servidor
 */
int main(int argc, char* argv[]) {
    try {
        // Configurar manejadores de señales para apagado controlado
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);

        // Crear y ejecutar controlador del servidor
        g_serverController = std::make_unique<RPCServer::ServerController>();
        return g_serverController->run(argc, argv);

    } catch (const std::exception& e) {
        std::cerr << "Error fatal: " << e.what() << std::endl;
        return 1;
    }
}
