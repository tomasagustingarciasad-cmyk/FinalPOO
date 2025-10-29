CREATE SCHEMA IF NOT EXISTS finalpoo;
CREATE TABLE IF NOT EXISTS finalpoo.rol(
    id SERIAL PRIMARY KEY,
    rol VARCHAR(50) UNIQUE NOT NULL
);
CREATE TABLE IF NOT EXISTS finalpoo.usuario (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    rol VARCHAR(100) UNIQUE NOT NULL,
    password VARCHAR(255) NOT NULL,
    CONSTRAINT fk_rol FOREIGN KEY (rol) REFERENCES finalpoo.rol (rol)
);

INSERT INTO finalpoo.rol (rol) VALUES
('admin'),
('user');

INSERT INTO finalpoo.usuario (name, rol, password) VALUES
('Juan Perez', 'admin', 'password123');