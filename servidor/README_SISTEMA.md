# Sistema de Control de Robot - Arduino Real

## 📋 Componentes del Sistema

### 1. Firmware Arduino (`Firmware/robotArm_v0.62sim/`)
- **Estado**: Original del profesor (SIMULATION=true por defecto)
- **Placa**: Arduino Mega 2560
- **Puerto**: `/dev/ttyUSB0`
- **Baudrate**: 115200

### 2. Servidor C++ XML-RPC (`servidor/`)
- **Archivo**: `servidor_rpc`
- **Puerto**: 8080
- **Funciones**: 9 métodos RPC para control del robot
- **Comunicación**: Serial con Arduino

### 3. Tests (`servidor/test_verification.py`)
- **Cliente**: XML-RPC en Python
- **Pruebas**: 16 tests de funcionalidad completa

---

## 🚀 Uso del Sistema

### Terminal 1: Iniciar Servidor C++

```bash
cd servidor
./servidor_rpc 8080
```

**Salida esperada:**
```
=== Servidor RPC Iniciado ===
Puerto: 8080
Métodos disponibles:
  - connectRobot
  - disconnectRobot
  - setMode
  - enableMotors
  - home
  - move
  - endEffector
  - getPosition
  - getEndstops
```

### Terminal 2: Ejecutar Tests

```bash
cd servidor
python3 test_verification.py
```

**Requisito:** Arduino Mega 2560 conectado en `/dev/ttyUSB0`

---

## 🔧 Firmware Arduino

### Configuración en `config.h`:

```cpp
#define BAUD 115200
#define SIMULATION true              // Para testing sin hardware
#define HOME_X_STEPPER true           // Endstops (true si tienes físicos)
#define HOME_Y_STEPPER true
#define HOME_Z_STEPPER true
```

### Geometría del Robot:

```cpp
LOW_SHANK_LENGTH = 120 mm
HIGH_SHANK_LENGTH = 120 mm
END_EFFECTOR_OFFSET = 50 mm

// Posición inicial tras homing (G28)
INITIAL_X = 0.0
INITIAL_Y = 170.0  // HIGH_SHANK + END_EFFECTOR_OFFSET
INITIAL_Z = 120.0  // LOW_SHANK

// Límites de workspace
Z_MIN = -115 mm
Z_MAX = 150 mm
R_MIN ≈ 54.9 mm   // Radio mínimo en plano XY
R_MAX ≈ 230.3 mm  // Radio máximo en plano XY
```

---

## 📡 Protocolo de Comunicación

### Comandos G-Code (Movimiento):

| Comando | Ejemplo | Respuesta | Descripción |
|---------|---------|-----------|-------------|
| `G28` | `G28` | `INFO: HOMING COMPLETE` | Homing a posición inicial |
| `G0` | `G0 X100 Y120 Z50` | `INFO: LINEAR MOVE: [...]` | Movimiento rápido |
| `G1` | `G1 X100 Y120 Z50 F800` | `INFO: LINEAR MOVE: [...]` | Movimiento lineal |
| `G90` | `G90` | `INFO: ABSOLUTE MODE ON` | Modo coordenadas absolutas |
| `G91` | `G91` | `INFO: RELATIVE MODE ON` | Modo coordenadas relativas |
| `G92` | `G92 X0 Y0 Z0` | `INFO: POSITION OFFSET: [...]` | Define offset |

### Comandos M-Code (Máquina):

| Comando | Ejemplo | Respuesta | Descripción |
|---------|---------|-----------|-------------|
| `M3` | `M3` | `INFO: GRIPPER ON` | Activa gripper |
| `M5` | `M5` | `INFO: GRIPPER OFF` | Desactiva gripper |
| `M17` | `M17` | `INFO: MOTORS ENABLED` | Habilita motores |
| `M18` | `M18` | `INFO: MOTORS DISABLED` | Deshabilita motores |
| `M106` | `M106` | `INFO: FAN ENABLED` | Activa ventilador |
| `M107` | `M107` | `INFO: FAN DISABLED` | Desactiva ventilador |
| `M114` | `M114` | Multilínea (ver abajo) | Consulta posición |
| `M119` | `M119` | Multilínea (ver abajo) | Estado endstops |

### Respuestas Multilínea:

**M114 - Posición actual:**
```
INFO: CURRENT POSITION: [X:100.00 Y:120.00 Z:50.00 E:0.00]
mode: ABSOLUTE
motorsEnabled: 1
fanEnabled: 1
```

**M119 - Estado endstops:**
```
INFO: ENDSTOPS STATUS
xState: 0
yState: 0
zState: 0
```

---

## 🔌 Métodos XML-RPC del Servidor

### 1. `connectRobot(port, baudrate)`
```python
server.connectRobot("/dev/ttyUSB0", 115200)
# → {'ok': 1, 'message': 'Conectado'}
```

### 2. `disconnectRobot()`
```python
server.disconnectRobot()
# → {'ok': 1, 'message': 'Desconectado'}
```

### 3. `setMode(manual, absolute)`
```python
server.setMode(True, True)   # Manual + Absoluto
# → {'ok': 1, 'message': 'OK'}
```

### 4. `enableMotors(enable)`
```python
server.enableMotors(True)    # Habilita motores (M17)
# → {'ok': 1, 'message': 'Motores ON'}
```

### 5. `home()`
```python
server.home()                # Ejecuta G28
# → {'ok': 1, 'message': 'Home ejecutado'}
```

### 6. `move(x, y, z, feedrate)`
```python
server.move(100.0, 120.0, 50.0, 800.0)
# → {'ok': 1, 'message': 'Movimiento enviado'}
```

### 7. `endEffector(enable)`
```python
server.endEffector(True)     # Fan ON (M106)
# → {'ok': 1, 'message': 'Efector ON'}
```

### 8. `getPosition()`
```python
pos = server.getPosition()   # Envía M114 y parsea respuesta
# → {
#     'ok': 1,
#     'message': 'Posición obtenida',
#     'x': 100.0,
#     'y': 120.0,
#     'z': 50.0,
#     'e': 0.0,
#     'mode': 'ABSOLUTE',
#     'motorsEnabled': 1,
#     'fanEnabled': 1
# }
```

### 9. `getEndstops()`
```python
endstops = server.getEndstops()  # Envía M119 y parsea respuesta
# → {
#     'ok': 1,
#     'message': 'Endstops obtenidos',
#     'xState': 0,
#     'yState': 0,
#     'zState': 0
# }
```

---

## ✅ Flujo de Test Típico

```python
import xmlrpc.client

# 1. Conectar
server = xmlrpc.client.ServerProxy("http://localhost:8080")
server.connectRobot("/dev/ttyUSB0", 115200)

# 2. Configurar
server.setMode(True, True)        # Modo manual + absoluto
server.enableMotors(True)         # Habilitar motores

# 3. Homing
server.home()                     # G28
time.sleep(3)

# 4. Verificar posición inicial
pos = server.getPosition()
print(f"Pos: X={pos['x']}, Y={pos['y']}, Z={pos['z']}")
# Esperado: X=0.0, Y=170.0, Z=120.0

# 5. Mover
server.move(100.0, 120.0, 50.0, 800.0)
time.sleep(2)

# 6. Verificar nueva posición
pos = server.getPosition()
print(f"Pos: X={pos['x']}, Y={pos['y']}, Z={pos['z']}")
# Esperado: X=100.0, Y=120.0, Z=50.0

# 7. Control de efector
server.endEffector(True)          # Fan ON
time.sleep(1)
server.endEffector(False)         # Fan OFF

# 8. Desconectar
server.disconnectRobot()
```

---

## ⚠️ Troubleshooting

### 1. "Permission denied" en /dev/ttyUSB0
```bash
sudo usermod -a -G dialout $USER
newgrp dialout
# O usar sudo temporalmente
sudo ./servidor_rpc 8080
```

### 2. Puerto no encontrado
```bash
# Verificar puertos disponibles
ls /dev/ttyUSB* /dev/ttyACM*

# Si aparece /dev/ttyACM0, usar ese en lugar de ttyUSB0
server.connectRobot("/dev/ttyACM0", 115200)
```

### 3. Coordenadas no se actualizan
- **Causa**: Firmware en modo SIMULATION=true
- **Solución**: Las coordenadas solo se actualizan con hardware real y SIMULATION=false
- **Verificación**: En modo simulación, M114 siempre retorna 0,0,0

### 4. Servidor se cierra inmediatamente
```bash
# Recompilar
cd servidor
make clean
make
./servidor_rpc 8080
```

---

## 📊 Verificación de Sistema Funcional

### Test mínimo:
```bash
# Terminal 1
cd servidor && ./servidor_rpc 8080

# Terminal 2
python3 -c "
import xmlrpc.client
s = xmlrpc.client.ServerProxy('http://localhost:8080')
print('Conectando...')
r = s.connectRobot('/dev/ttyUSB0', 115200)
print(r)
r = s.getPosition()
print(r)
s.disconnectRobot()
"
```

**Salida esperada:**
```
Conectando...
{'ok': 1, 'message': 'Conectado'}
{'ok': 1, 'x': 0.0, 'y': 0.0, 'z': 0.0, ...}
```

---

## 📝 Notas Importantes

1. **Modo SIMULATION**: Con `SIMULATION=true`, el firmware NO actualiza coordenadas reales
2. **Endstops**: Con endstops físicos, el homing funciona correctamente
3. **Parsing multilínea**: El servidor maneja correctamente respuestas M114 y M119
4. **Thread-safety**: El servidor usa mutexes para comunicación serial segura
5. **Timeouts**: Los métodos esperan hasta 600ms para respuestas M114, 300ms para M119

---

## 🎯 Resumen

El sistema permite:
- ✅ Control completo del robot vía XML-RPC
- ✅ Comunicación serial bidireccional con Arduino
- ✅ Parsing de respuestas multilínea del firmware
- ✅ Testing automatizado con Python
- ✅ 9 métodos RPC para todas las funciones del robot

**Arquitectura:**
```
Test Python → XML-RPC → Servidor C++ → Serial → Arduino Firmware
                                                      ↓
                                              Motores + Sensores
```
