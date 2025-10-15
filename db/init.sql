CREATE SCHEMA IF NOT EXISTS finalpoo;
CREATE TABLE IF NOT EXISTS finalpoo.usuario (
    id SERIAL PRIMARY KEY,
    nombre VARCHAR(100) NOT NULL,
    email VARCHAR(100) UNIQUE NOT NULL,
    contrasena VARCHAR(100) NOT NULL
);

INSERT INTO finalpoo.usuario (nombre, email, contrasena) VALUES
('Juan Perez', 'juan.perez@example.com', 'password123');