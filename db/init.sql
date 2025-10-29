CREATE SCHEMA IF NOT EXISTS finalpoo;

CREATE TABLE IF NOT EXISTS finalpoo.rol(
    id SERIAL PRIMARY KEY,
    rol VARCHAR(50) UNIQUE NOT NULL
);

CREATE TABLE IF NOT EXISTS finalpoo.usuario (
    usuario_id SERIAL PRIMARY KEY,
    username VARCHAR(100) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    role VARCHAR(50) NOT NULL,
    active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

INSERT INTO finalpoo.rol (rol) VALUES
('ADMIN'),
('OPERATOR')
ON CONFLICT (rol) DO NOTHING;

CREATE EXTENSION IF NOT EXISTS pgcrypto;

CREATE OR REPLACE FUNCTION hash_password(password_text VARCHAR)
RETURNS VARCHAR AS $$
BEGIN
    RETURN encode(digest(password_text || 'salt_secreto_poo', 'sha256'), 'hex');
END;
$$ LANGUAGE plpgsql;

INSERT INTO finalpoo.usuario (username, password_hash, role) VALUES
('admin', encode(digest('Admin123' || 'salt_secreto_poo', 'sha256'), 'hex'), 'ADMIN'),
('user', encode(digest('User123' || 'salt_secreto_poo', 'sha256'), 'hex'), 'OPERATOR')
ON CONFLICT (username) DO NOTHING;

-- Tabla para almacenar archivos de rutinas G-code
CREATE TABLE IF NOT EXISTS finalpoo.gcode_routine (
    routine_id SERIAL PRIMARY KEY,
    filename VARCHAR(255) NOT NULL,
    original_filename VARCHAR(255) NOT NULL,
    description TEXT,
    gcode_content TEXT NOT NULL,
    file_size INTEGER NOT NULL,
    user_id INTEGER NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_routine_user FOREIGN KEY (user_id) REFERENCES finalpoo.usuario (usuario_id) ON DELETE CASCADE,
    CONSTRAINT unique_filename_per_user UNIQUE (user_id, filename)
);

-- Índices para mejorar el rendimiento de consultas
CREATE INDEX IF NOT EXISTS idx_gcode_routine_user_id ON finalpoo.gcode_routine (user_id);
CREATE INDEX IF NOT EXISTS idx_gcode_routine_filename ON finalpoo.gcode_routine (filename);
CREATE INDEX IF NOT EXISTS idx_gcode_routine_created_at ON finalpoo.gcode_routine (created_at DESC);

-- Insertar algunas rutinas de ejemplo para testing
INSERT INTO finalpoo.gcode_routine (filename, original_filename, description, gcode_content, file_size, user_id) VALUES
('home_routine.gcode', 'home_routine.gcode', 'Rutina básica de home para ejes XYZ', 'G28 ; Home all axes\nG1 Z5 F3000 ; Move Z up 5mm\nM84 ; Disable steppers', 65, 1),
('test_square.gcode', 'test_square.gcode', 'Dibujar un cuadrado de 10x10mm', 'G28 ; Home\nG1 X0 Y0 Z0 F3000\nG1 X10 Y0 F1000\nG1 X10 Y10\nG1 X0 Y10\nG1 X0 Y0\nG28 ; Home', 85, 2),
('calibration.gcode', 'calibration.gcode', 'Rutina de calibración básica', 'G28 ; Home all axes\nG1 Z10 F3000\nG1 X50 Y50 F1500\nG1 Z0 F500\nG1 Z10 F3000\nG28', 95, 1)
ON CONFLICT (user_id, filename) DO NOTHING;
