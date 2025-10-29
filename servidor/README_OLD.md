# Servidor RPC para Control de Robot

## Descripción

Servidor XML-RPC que permite controlar un brazo robótico mediante protocolo G-code/M-code a través de puerto serie.

## Cumplimiento de Requerimientos del TP

### ✅ Requerimientos del Profesor (Email)

1. **Lazo de espera para descartar mensaje inicial**: Implementado en `discardInitialBanner()` con loop de 3 segundos
2. **Descarte de respuestas "OK"**: Implementado en `readMultiLineResponse()` 
3. **Manejo de respuestas de 1 línea y multilínea**:
   - M119 (endstops): 1 línea
   - M114 (posición): múltiples líneas (modo, posición, motores, fan)
4. **Ventanas de idle**: 600ms para M114, 300ms para M119

### ✅ Métodos RPC Implementados

Todos los métodos obligatorios del TP:

- `connectRobot(port, baudrate)` - Conecta al robot y descarta banner inicial
- `disconnectRobot()` - Cierra la conexión
- `setMode(manual, absolute)` - Configura modo absoluto/relativo (G90/G91)
- `enableMotors(enabled)` - Habilita/deshabilita motores (M17/M18)
- `home()` - Ejecuta homing (G28)
- `move(x, y, z, feedrate)` - Movimiento a coordenadas (G0/G1)
- `endEffector(enabled)` - Activa/desactiva efector final (M106/M107)

Métodos adicionales:

- `getPosition()` - Consulta posición actual (M114) con parseo multilínea
- `getEndstops()` - Consulta estado de endstops (M119)

### ✅ Arquitectura

- **Servidor**: XML-RPC sobre HTTP (puerto 8080)
- **Comunicación Serial**: POSIX termios, baudrate configurable
- **Thread-Safety**: Mutex para protección de acceso al puerto serie
- **Parseo Robusto**: Manejo de respuestas fragmentadas, timeouts configurables
- **Tolerancia a Fallos**: Parseo tolerante cuando datos no están disponibles

## Compilación

```bash
cd servidor
make
```

## Ejecución

```bash
# Terminal 1: Levantar servidor
./servidor 8080

# Terminal 2: Ejecutar tests
python3 test_debug.py
python3 test_robot_multiline.py
```

## Tests

### test_debug.py
Test básico que verifica:
- Conexión al robot
- Consulta de posición (M114)
- Consulta de endstops (M119)
- Desconexión

### test_robot_multiline.py
Test completo que verifica:
- Conexión y configuración
- Homing (G28)
- Movimientos múltiples
- Control de efector final
- Detección de cambios de estado (motores, fan)
- Consultas de posición y endstops

## Notas sobre Simulación

El firmware en modo `SIMULATION=true` no actualiza las coordenadas X/Y/Z/E durante los movimientos, ya que no hay actuadores físicos. Sin embargo:

- ✅ Los comandos de movimiento se envían correctamente
- ✅ El firmware acepta y procesa los comandos
- ✅ M114 parsea correctamente la respuesta multilínea
- ✅ Los estados disponibles se detectan correctamente (modo, motores, fan)

Con hardware real, las coordenadas se actualizarían después de cada movimiento.

## Funcionalidades Destacadas

1. **Parseo Multilínea Robusto**: Implementa ventanas de idle para determinar cuándo terminó la respuesta multilínea
2. **Manejo de Fragmentación**: El método `readLine()` acumula caracteres correctamente incluso cuando llegan fragmentados
3. **Descarte Automático**: Filtra automáticamente respuestas "OK" y banner inicial
4. **Detección de Estado**: Parsea información de modo, motores y fan desde la respuesta M114
5. **Tolerancia a Fallos**: Retorna éxito aunque algunos datos no estén disponibles (coordenadas en simulación)

## Estructura del Código

```
servidor/
├── main_servidor.cpp      # Punto de entrada
├── inc/
│   ├── Robot.h           # Interfaz de control del robot
│   ├── SerialPort.h      # Comunicación serie POSIX
│   └── ServerModel.h     # Métodos RPC
├── lib/
│   ├── Robot.cpp         # Implementación con parseo multilínea
│   ├── SerialPort.cpp    # Manejo robusto de lectura serie
│   └── XmlRpc*.cpp       # Librería XML-RPC
└── test_*.py             # Scripts de prueba
```

## Autor

Implementado como parte del Trabajo Final de POO - 2025
