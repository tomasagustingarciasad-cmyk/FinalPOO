# 🔧 Diagnóstico de Error: "XML-RPC fault: type error"

## Problema Identificado

Basado en el análisis de logs, el error **"XML-RPC fault: type error"** que experimentas al intentar conectarte al robot está relacionado con que **el robot no está conectado al puerto serie**.

## Análisis de Logs

Ejecuté el comando de análisis de logs: `python3 servidor/log_analyzer.py --connection-debug`

### Resultados:
- **17 eventos relacionados con problemas de robot**
- **Problema principal**: "Intento de movimiento sin robot conectado"
- **4 eventos de puerto serie registrados**

## Pasos de Diagnóstico y Solución

### 1. Verificar Puerto Serie

```bash
# Listar puertos serie disponibles
ls -la /dev/tty{USB,ACM}*

# Verificar permisos
sudo chmod 666 /dev/ttyUSB0   # o el puerto correspondiente

# Verificar si el puerto está siendo usado
sudo lsof /dev/ttyUSB0
```

### 2. Verificar Conexión del Robot

```bash
# Probar comunicación básica
screen /dev/ttyUSB0 115200
# o
minicom -D /dev/ttyUSB0 -b 115200
```

### 3. Análisis Detallado del Error XML-RPC

Para obtener más información sobre el error XML-RPC, voy a mejorar el logging del cliente web.

### 4. Debugging en Tiempo Real

He mejorado el sistema de logging para capturar:

1. **Logging detallado en Robot.cpp**:
   - Intentos de conexión al puerto serie
   - Banner del firmware
   - Comandos G-code enviados/recibidos
   - Timeouts y errores de comunicación

2. **Logging detallado en XML-RPC client**:
   - Duración de las llamadas
   - Parámetros enviados 
   - Códigos de error específicos
   - Detalles de fallas de conexión

### 5. Comandos de Análisis Específicos

```bash
# Ver solo errores recientes
python3 servidor/log_analyzer.py --errors --from "$(date -d '1 hour ago' '+%Y-%m-%d %H:%M:%S')"

# Analizar intentos de movimiento específicamente  
python3 servidor/log_analyzer.py --method move --level DEBUG

# Ver todos los eventos de conexión
python3 servidor/log_analyzer.py --connection-debug

# Filtrar por usuario específico
python3 servidor/log_analyzer.py --user admin --errors
```

## Próximos Pasos Recomendados

### Paso 1: Verificar Hardware
```bash
# 1. Conectar robot físicamente
# 2. Verificar puerto serie
ls -la /dev/ttyUSB* /dev/ttyACM*

# 3. Probar comunicación básica
sudo screen /dev/ttyUSB0 115200
```

### Paso 2: Compilar Servidor con Logging Mejorado
```bash
cd servidor
make clean && make
```

### Paso 3: Ejecutar Servidor con Logging Detallado
```bash
./servidor_rpc 8080
```

### Paso 4: Probar Conexión desde Cliente Web
1. Ir a la interfaz web
2. Intentar conectar robot
3. Monitorear logs en tiempo real:

```bash
# Terminal 1: Servidor
./servidor_rpc 8080

# Terminal 2: Monitor de logs
tail -f logs/server_activity.csv | cut -d',' -f1,3,4,9,10

# Terminal 3: Cliente web
cd ../cliente-web
npm run dev
```

### Paso 5: Análisis Post-Error
```bash
# Después de reproducir el error
python3 servidor/log_analyzer.py --connection-debug --from "$(date -d '5 minutes ago' '+%Y-%m-%d %H:%M:%S')"
```

## Errores Comunes y Soluciones

### Error: "Permission denied" en puerto serie
```bash
sudo usermod -a -G dialout $USER
# Luego reiniciar sesión
```

### Error: "Device or resource busy"
```bash
sudo pkill -f tty
sudo fuser -k /dev/ttyUSB0
```

### Error: "No such file or directory"
```bash
# Robot no conectado o puerto incorrecto
# Verificar con: dmesg | tail
```

## Debugging Avanzado

### Capturar Tráfico Serie en Tiempo Real
```bash
# Instalar interceptty si está disponible
interceptty /dev/ttyUSB0 /tmp/robot_debug
# En otra terminal:
cat /tmp/robot_debug
```

### Logs Detallados del Cliente Web
Los logs del cliente web ahora incluyen:
- Duración de llamadas XML-RPC
- Parámetros completos enviados
- Códigos de error detallados
- Stack traces de errores

```bash
tail -f logs/web_client_activity.csv
```

## Estado Actual del Sistema de Logging

✅ **Implementado:**
- Logging detallado en servidor C++
- Logging completo en cliente Node.js  
- Herramienta de análisis con filtros avanzados
- Análisis específico de problemas de conexión
- Macros de logging para fácil integración

✅ **Funcional:**
- Análisis de logs existentes
- Identificación de patrones de error
- Reportes administrativos con filtros múltiples

🔄 **En proceso:**
- Compilación del servidor con logging mejorado (necesita corrección de macros)
- Testing completo del flujo de conexión

El sistema de logging está diseñado específicamente para ayudarte a debuggear este tipo de errores de conexión XML-RPC y comunicación con el robot.