/**
 * @file main_servidor.cpp
 * @brief Módulo launcher para Servidor RPC
 * @author Generado para POO TP2 Req4
 */

#include <iostream>
#include <csignal>
#include <memory>
#include "inc/ServerController.h"
#include "inc/CSVLogger.h"

// Instancia global del controlador para manejo de señales
std::unique_ptr<RPCServer::ServerController> g_serverController;

/**
 * @brief Manejador de señales para apagado controlado
 */
void signalHandler(int signal) {
    std::cout << "\n\nRecibida señal " << signal << ". Deteniendo servidor..." << std::endl;
    LOG_SYSTEM("MainServer", LogLevel::INFO, "Señal de terminación recibida", 
               "Signal: " + std::to_string(signal));
    
    if (g_serverController) {
        g_serverController->shutdown();
    }
    
    LOG_SYSTEM("MainServer", LogLevel::INFO, "Servidor detenido correctamente", "");
    exit(0);
}

/**
 * @brief Punto de entrada principal para la aplicación del servidor
 */
int main(int argc, char* argv[]) {
    try {
        // Inicializar logging
        LOG_SYSTEM("MainServer", LogLevel::INFO, "Iniciando servidor Robot XML-RPC", 
                   "Arguments: " + std::to_string(argc));
        
        // Log de argumentos de línea de comandos
        for (int i = 0; i < argc; i++) {
            LOG_DEBUG("MainServer", "main", "Argumento de línea de comandos", 
                     "argv[" + std::to_string(i) + "] = " + std::string(argv[i]));
        }
        
        // Configurar manejadores de señales para apagado controlado
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);
        
        LOG_SYSTEM("MainServer", LogLevel::INFO, "Manejadores de señales configurados", "SIGINT, SIGTERM");

        // Crear y ejecutar controlador del servidor
        LOG_SYSTEM("MainServer", LogLevel::INFO, "Creando controlador del servidor", "");
        g_serverController = std::make_unique<RPCServer::ServerController>();
        
        LOG_SYSTEM("MainServer", LogLevel::INFO, "Iniciando ejecución del servidor", "");
        int result = g_serverController->run(argc, argv);
        
        LOG_SYSTEM("MainServer", LogLevel::INFO, "Servidor finalizado", 
                   "Exit code: " + std::to_string(result));
        return result;

    } catch (const std::exception& e) {
        std::cerr << "Error fatal: " << e.what() << std::endl;
        LOG_ERROR("MainServer", "FATAL_ERROR", "Error fatal durante ejecución", e.what());
        return 1;
    }
}
