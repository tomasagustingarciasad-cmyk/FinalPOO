# Servidor RPC para Control de Brazo Robótico# Servidor RPC para Control de Robot



## 📖 Descripción## Descripción



Servidor XML-RPC en C++ que proporciona una API de control remoto para un brazo robótico Arduino. Implementa comunicación serial (G-code/M-code) y expone métodos RPC para integración con clientes web.Servidor XML-RPC que permite controlar un brazo robótico mediante protocolo G-code/M-code a través de puerto serie.



## 🏗️ Arquitectura## Cumplimiento de Requerimientos del TP



```### ✅ Requerimientos del Profesor (Email)

Cliente Web (Node.js) ◄──XML-RPC──► Servidor C++ ◄──Serial──► Arduino (Firmware)

     Puerto 3000              Puerto 8080         115200 baud      /dev/ttyUSB01. **Lazo de espera para descartar mensaje inicial**: Implementado en `discardInitialBanner()` con loop de 3 segundos

```2. **Descarte de respuestas "OK"**: Implementado en `readMultiLineResponse()` 

3. **Manejo de respuestas de 1 línea y multilínea**:

## ⚙️ Características Principales   - M119 (endstops): 1 línea

   - M114 (posición): múltiples líneas (modo, posición, motores, fan)

- ✅ **8 Métodos RPC** (connectRobot, disconnectRobot, setMode, enableMotors, home, move, endEffector, getPosition, getEndstops)4. **Ventanas de idle**: 600ms para M114, 300ms para M119

- ✅ **Comunicación Serial Robusta** con acumulación carácter a carácter y timeouts inteligentes

- ✅ **Parsing Multi-línea** con filtrado automático de respuestas "OK"### ✅ Métodos RPC Implementados

- ✅ **Thread-Safe** mediante mutexes para operaciones concurrentes

- ✅ **Descarte Automático** de banner inicial del firmwareTodos los métodos obligatorios del TP:

- ✅ **Tolerancia a Fallos** con parsing flexible y detección de cambios de estado

- `connectRobot(port, baudrate)` - Conecta al robot y descarta banner inicial

## 🚀 Inicio Rápido- `disconnectRobot()` - Cierra la conexión

- `setMode(manual, absolute)` - Configura modo absoluto/relativo (G90/G91)

### Compilación- `enableMotors(enabled)` - Habilita/deshabilita motores (M17/M18)

```bash- `home()` - Ejecuta homing (G28)

cd servidor- `move(x, y, z, feedrate)` - Movimiento a coordenadas (G0/G1)

make clean && make- `endEffector(enabled)` - Activa/desactiva efector final (M106/M107)

```

Métodos adicionales:

### Ejecución

```bash- `getPosition()` - Consulta posición actual (M114) con parseo multilínea

./servidor 8080- `getEndstops()` - Consulta estado de endstops (M119)

```

### ✅ Arquitectura

### Ejemplo de Uso (Python)

```python- **Servidor**: XML-RPC sobre HTTP (puerto 8080)

import xmlrpc.client- **Comunicación Serial**: POSIX termios, baudrate configurable

- **Thread-Safety**: Mutex para protección de acceso al puerto serie

server = xmlrpc.client.ServerProxy('http://localhost:8080')- **Parseo Robusto**: Manejo de respuestas fragmentadas, timeouts configurables

- **Tolerancia a Fallos**: Parseo tolerante cuando datos no están disponibles

# Conectar al robot

server.connectRobot('/dev/ttyUSB0', 115200)## Compilación



# Habilitar motores y hacer homing```bash

server.enableMotors(True)cd servidor

server.home()make

```

# Mover a posición

server.move(100.0, 120.0, 50.0, 800)## Ejecución



# Consultar estado```bash

pos = server.getPosition()# Terminal 1: Levantar servidor

print(f"Posición: X={pos['x']}, Y={pos['y']}, Z={pos['z']}")./servidor 8080

print(f"Modo: {pos['mode']}, Motores: {pos['motorsEnabled']}")

# Terminal 2: Ejecutar tests

# Desconectarpython3 test_debug.py

server.disconnectRobot()python3 test_robot_multiline.py

``````



## 📡 Métodos RPC## Tests



| Método | Parámetros | Descripción |### test_debug.py

|--------|------------|-------------|Test básico que verifica:

| `connectRobot` | `port, baudrate` | Conecta al puerto serial |- Conexión al robot

| `disconnectRobot` | - | Cierra conexión serial |- Consulta de posición (M114)

| `setMode` | `mode` ("ABSOLUTE"/"RELATIVE") | Cambia sistema de coordenadas |- Consulta de endstops (M119)

| `enableMotors` | `enable` (bool) | Activa/desactiva motores |- Desconexión

| `home` | - | Ejecuta homing (G28) |

| `move` | `x, y, z, feedrate` | Movimiento coordenado |### test_robot_multiline.py

| `endEffector` | `enable` (bool) | Controla gripper/fan |Test completo que verifica:

| `getPosition` | - | Consulta posición y estados ⭐ |- Conexión y configuración

| `getEndstops` | - | Estado de finales de carrera ⭐ |- Homing (G28)

- Movimientos múltiples

⭐ = Métodos con respuesta multi-línea- Control de efector final

- Detección de cambios de estado (motores, fan)

## 🔧 Configuración del Firmware- Consultas de posición y endstops



**`Firmware/robotArm_v0.62sim/config.h`**:## Notas sobre Simulación

```cpp

#define BAUD 115200El firmware en modo `SIMULATION=true` no actualiza las coordenadas X/Y/Z/E durante los movimientos, ya que no hay actuadores físicos. Sin embargo:

#define SIMULATION true

#define USE_ESP8266 false- ✅ Los comandos de movimiento se envían correctamente

```- ✅ El firmware acepta y procesa los comandos

- ✅ M114 parsea correctamente la respuesta multilínea

Para subir el firmware:- ✅ Los estados disponibles se detectan correctamente (modo, motores, fan)

1. Abrir `Firmware/robotArm_v0.62sim/robotArm_v0.62sim.ino` en Arduino IDE

2. Seleccionar **Tools → Board → Arduino Uno/Mega**Con hardware real, las coordenadas se actualizarían después de cada movimiento.

3. Seleccionar **Tools → Port → /dev/ttyUSB0**

4. Click **Upload** (Ctrl+U)## Funcionalidades Destacadas



## 📂 Estructura del Proyecto1. **Parseo Multilínea Robusto**: Implementa ventanas de idle para determinar cuándo terminó la respuesta multilínea

2. **Manejo de Fragmentación**: El método `readLine()` acumula caracteres correctamente incluso cuando llegan fragmentados

```3. **Descarte Automático**: Filtra automáticamente respuestas "OK" y banner inicial

servidor/4. **Detección de Estado**: Parsea información de modo, motores y fan desde la respuesta M114

├── main_servidor.cpp          # Punto de entrada5. **Tolerancia a Fallos**: Retorna éxito aunque algunos datos no estén disponibles (coordenadas en simulación)

├── inc/

│   ├── Robot.h               # Interfaz de comunicación con firmware## Estructura del Código

│   ├── SerialPort.h          # Comunicación serial POSIX

│   └── ServerModel.h         # Definición de métodos RPC```

├── lib/servidor/

│   ├── Robot.cpp             # Lógica de control del robot├── main_servidor.cpp      # Punto de entrada

│   ├── SerialPort.cpp        # Implementación serial├── inc/

│   └── XmlRpc*.cpp           # Librería XML-RPC│   ├── Robot.h           # Interfaz de control del robot

└── Makefile│   ├── SerialPort.h      # Comunicación serie POSIX

```│   └── ServerModel.h     # Métodos RPC

├── lib/

## 🛠️ Implementación Técnica│   ├── Robot.cpp         # Implementación con parseo multilínea

│   ├── SerialPort.cpp    # Manejo robusto de lectura serie

### Manejo de Respuestas Multi-línea│   └── XmlRpc*.cpp       # Librería XML-RPC

└── test_*.py             # Scripts de prueba

El servidor implementa "ventanas de inactividad" (idle windows) para detectar el fin de respuestas multi-línea:```



```cpp## Autor

std::string readMultiLineResponse(int timeoutMs, int idleWindowMs) {

    std::string fullResponse;Implementado como parte del Trabajo Final de POO - 2025

    auto lastDataTime = std::chrono::steady_clock::now();
    
    while (true) {
        std::string line;
        if (serial_.readLine(line, 100)) {
            if (line == "OK") continue;  // Filtrar automáticamente
            fullResponse += line + "\n";
            lastDataTime = std::chrono::steady_clock::now();
        }
        
        auto elapsed = millisSince(lastDataTime);
        if (elapsed >= idleWindowMs) break;  // No hay más datos
    }
    return fullResponse;
}
```

**Estrategia**:
- **M114** (4-5 líneas): Idle window de 600ms
- **M119** (1 línea): Idle window de 300ms
- **Filtrado automático** de líneas "OK"

### Comunicación Serial Robusta

Acumulación carácter a carácter con reset de timeout:

```cpp
bool SerialPort::readLine(std::string& line, int timeoutMs) {
    auto start = std::chrono::steady_clock::now();
    
    while (true) {
        struct timeval tv = {0, 10000};  // 10ms timeout
        int ret = select(fd_ + 1, &readfds, nullptr, nullptr, &tv);
        
        if (ret > 0) {
            char c;
            if (read(fd_, &c, 1) > 0) {
                if (c == '\n') return true;
                if (c != '\r') line += c;
                start = std::chrono::steady_clock::now();  // Reset
            }
        }
        
        if (millisSince(start) >= timeoutMs) break;
    }
    return false;
}
```

### Thread-Safety

```cpp
class Robot {
private:
    SerialPort serial_;
    std::mutex serialMutex_;
    
public:
    RobotPosition getPosition() {
        std::lock_guard<std::mutex> lock(serialMutex_);
        // Operaciones seguras sobre serial_
    }
};
```

## 🧪 Validación

### Test de Cambio de Estado (Evidencia de Parsing Correcto)

```python
# Activar efector
server.endEffector(True)
pos1 = server.getPosition()
print(pos1['fanEnabled'])  # → 1

# Desactivar efector
server.endEffector(False)
pos2 = server.getPosition()
print(pos2['fanEnabled'])  # → 0 ✅ Cambio detectado
```

**Conclusión**: La detección de cambios de estado (fan ON→OFF, motores, modo) demuestra que el parsing multi-línea funciona correctamente.

## ⚠️ Notas sobre Modo Simulación

El firmware está en modo simulación (`SIMULATION=true`):

- ✅ **Detección de estados**: Fan, motores, modo → Funciona correctamente
- ✅ **Comandos**: Todos los comandos se ejecutan sin errores
- ⚠️ **Coordenadas**: Permanecen en 0.0 (no hay actuadores físicos)

**Esto es comportamiento esperado**. Para ver coordenadas reales:
1. Conectar hardware (motores + endstops)
2. Cambiar `SIMULATION` a `false` en `config.h`
3. Recompilar y subir firmware

## 📊 Resumen de Cumplimiento

| Requisito | Estado |
|-----------|--------|
| Métodos RPC obligatorios (7) | ✅ |
| Métodos adicionales (2) | ✅ |
| Descarte de banner inicial | ✅ |
| Filtrado de respuestas "OK" | ✅ |
| Manejo de respuestas multi-línea | ✅ |
| Parsing tolerante | ✅ |
| Thread-safety | ✅ |
| Validación completa | ✅ |

## 📚 Documentación Detallada

Para información técnica completa (arquitectura, protocolo G-code, detalles de implementación, troubleshooting), consultar:

**`DOCUMENTACION_TECNICA.md`** - Documentación exhaustiva con:
- Requisitos del profesor y su implementación
- Protocolo de comunicación completo
- Detalles de implementación críticos
- Logs de pruebas y validación
- Notas de debugging y decisiones de diseño

## 👤 Autor

Trabajo Final - Programación Orientada a Objetos 2025
