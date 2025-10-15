#include "db/db_pool.hpp"
#include <iostream>

int main() {
    try {
        PgConfig cfg;
        cfg.host = "localhost";
        cfg.port = 31432;
        cfg.dbname = "poo";
        cfg.user = "postgres";
        cfg.password = "pass123";
        cfg.ssl_disable = true;

        PgPool pool{cfg, 2, 10};

        // Checkout de conexión (RAII)
        auto h = pool.acquire();

        pqxx::work tx{h.conn()};
        pqxx::result r = tx.exec_params(
            "INSERT INTO finalpoo.usuario (nombre, email, contrasena) VALUES ($1, $2, $3) RETURNING id, nombre, email, contrasena;",
            "Maria Gomez", "maria.gomez@example.com", "password456"
        );

        for (auto const& row : r) {
            int id = row[0].as<int>();
            std::string nombre = row[1].as<std::string>();
            std::string email = row[2].as<std::string>();
            std::string contrasena = row[3].as<std::string>();
            std::cout << "Row: id=" << id << ", nombre=" << nombre << ", email=" << email << ", contrasena=" << contrasena << "\n";
        }

        tx.commit();

        // Al salir de scope, h.~PgConnHandle() devuelve la conexión al pool.
    } catch (std::exception const& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
