# Sistema de Logging Integral - Robot Control System

## Resumen Ejecutivo

Se ha implementado un sistema de logging completo y comprensivo que satisface todos los requisitos académicos especificados:

**Requisito Principal:** "Mantener un archivo con el registro del trabajo (log acumulativo) realizado por el servidor. Este archivo responde a un formato CSV estándar. La información mantenida en caso de peticiones tiene: marca de tiempo de cada petición al servidor, detalle de la petición, usuario que realizó la misma, nodo desde donde se conectó, código de la respuesta dada. En caso de funcionamiento general tiene: marca de tiempo en que se produjo el evento en el lado servidor, módulo, mensaje con el detalle de lo ocurrido. Ofrecer al administrador un reporte en consola con detalles del log de trabajo del servidor, incluyendo la posibilidad de filtrar por al menos 2 criterios"

## Arquitectura del Sistema

### 1. Servidor C++ (servidor/)

#### CSVLogger (servidor/inc/CSVLogger.h, servidor/lib/CSVLogger.cpp)
- **Funcionalidad**: Logging thread-safe con formato CSV estándar
- **Características**:
  - Thread-safe mediante std::mutex
  - Creación automática de directorios
  - Escritura de headers CSV automática
  - Escape de caracteres especiales en CSV
  - Formato de timestamp: YYYY-MM-DD HH:MM:SS
  
**Formato CSV Implementado:**
```
timestamp,type,module,level,method,user,client_ip,response_code,message,details
```

**Tipos de Log:**
- `REQUEST`: Peticiones XML-RPC con usuario, IP, código de respuesta
- `SYSTEM`: Eventos del sistema (startup, shutdown, status checks)
- `ERROR`: Errores de funcionamiento y excepciones
- `DEBUG`: Información de depuración y diagnóstico

**Niveles de Log:**
- `DEBUG`: Información detallada para diagnóstico
- `INFO`: Información general de funcionamiento
- `WARNING`: Advertencias y situaciones potencialmente problemáticas
- `ERROR`: Errores recuperables
- `CRITICAL`: Errores críticos del sistema

#### Integración en ServerModel.h
- **ServiceMethod Base Class**: Añadido helper `logRequest()` para logging estandarizado
- **AuthLoginMethod**: Logging completo de intentos de login, éxitos y fallos
- **MoveMethod**: Logging de comandos de movimiento del robot
- **Todas las clases XML-RPC**: Integración mediante macros LOG_*

#### Macros de Logging
```cpp
#define LOG_REQUEST(user, ip, code, message) \
    CSVLogger::getInstance().logRequest(user, ip, code, message)

#define LOG_SYSTEM(module, message) \
    CSVLogger::getInstance().logSystem(module, "INFO", "", message, "")

#define LOG_DEBUG(module, method, message, details) \
    CSVLogger::getInstance().logDebug(module, method, message, details)

#define LOG_ERROR(module, message) \
    CSVLogger::getInstance().logError(module, message)

#define LOG_WARNING(module, message) \
    CSVLogger::getInstance().logSystem(module, "WARNING", "", message, "")
```

### 2. Cliente Web Node.js (cliente-web/src/services/logger.js)

#### WebLogger Class
- **Funcionalidad**: Logging del lado cliente en formato CSV compatible
- **Características**:
  - Formato CSV idéntico al servidor
  - Logging automático de requests HTTP
  - Categorización de eventos de autenticación
  - Middleware Express para logging transparente

**Métodos Principales:**
- `logRequest(req, method, success, message, details)`: Logging general de requests
- `logAuth(req, method, success, message, details)`: Logging específico de autenticación
- `logSystem(req, method, message, details)`: Logging de eventos del sistema
- `logError(req, method, message, details)`: Logging de errores
- `middleware()`: Middleware Express para logging automático

### 3. Herramienta de Análisis (servidor/log_analyzer.py)

#### LogAnalyzer Class
- **Funcionalidad**: Análisis y reporting de logs con filtrado avanzado
- **Capacidades de Filtrado** (cumple requisito de "al menos 2 criterios"):
  1. **Usuario**: Filtrar por usuario específico
  2. **Fecha/Hora**: Rango de fechas con formato YYYY-MM-DD HH:MM:SS
  3. **Método**: Filtrar por método XML-RPC específico
  4. **Nivel**: Filtrar por nivel de log (DEBUG, INFO, WARNING, ERROR, CRITICAL)
  5. **Tipo**: Filtrar por tipo de evento (REQUEST, SYSTEM, ERROR, DEBUG)

**Tipos de Reportes:**
- **Reporte Resumen**: Estadísticas generales de actividad
- **Reporte por Usuario**: Actividad específica por usuario
- **Reporte de Errores**: Todos los errores y warnings del sistema
- **Conteo**: Solo mostrar número de entradas que coinciden con filtros

## Implementación Completa por Módulos

### Servidor C++

**Archivos Modificados/Creados:**
- `servidor/inc/CSVLogger.h` ✅ - Clase principal de logging
- `servidor/lib/CSVLogger.cpp` ✅ - Implementación del logger
- `servidor/inc/ServerModel.h` ✅ - Integración en métodos XML-RPC
- `servidor/main_servidor.cpp` ✅ - Logging de startup/shutdown
- `servidor/Makefile` ✅ - Compilación de CSVLogger

**Integración XML-RPC:** ✅
- AuthLoginMethod: Login attempts, successes, failures
- AuthLogoutMethod: Logout tracking
- UserCreateMethod, UserDeleteMethod: User management
- MoveMethod: Robot movement commands
- HomingMethod: Robot homing operations
- RoutineUploadMethod, RoutineListMethod, etc.: Routine operations

### Cliente Web Node.js

**Archivos Modificados/Creados:**
- `cliente-web/src/services/logger.js` ✅ - WebLogger service
- `cliente-web/src/app.js` ✅ - Middleware integration
- `cliente-web/src/routes/auth.js` ✅ - Authentication logging
- `cliente-web/src/routes/panel.js` ✅ - Panel access logging
- `cliente-web/src/routes/robot.js` ✅ - Robot control logging  
- `cliente-web/src/routes/routines.js` ✅ - Routine management logging
- `cliente-web/src/routes/learning.js` ✅ - Learning mode logging

**Eventos Logged:**
- Login attempts, successes, failures
- Page access (login, panel, robot movement, learning)
- Robot movement commands and results
- Robot homing operations
- Routine listing and management
- System status checks
- Error conditions and exceptions

### Herramientas de Análisis

**archivo:** `servidor/log_analyzer.py` ✅
- **CLI Interface:** Interfaz completa de línea de comandos
- **Multiple Filters:** Usuario, fecha, método, nivel, tipo
- **Report Types:** Resumen, por usuario, errores, conteo
- **CSV Processing:** Lectura y análisis de archivos CSV
- **Date Parsing:** Soporte para rangos de fechas

## Ejemplos de Uso

### 1. Compilación y Ejecución

```bash
# Compilar servidor con logging
cd servidor
make clean && make

# Ejecutar servidor (genera logs/server_activity.csv)
./servidor_rpc

# Ejecutar cliente web (genera logs/web_client.csv)
cd ../cliente-web
npm start
```

### 2. Análisis de Logs

```bash
# Reporte resumen general
python3 log_analyzer.py --summary

# Actividad de usuario específico
python3 log_analyzer.py --user admin

# Errores del sistema
python3 log_analyzer.py --errors

# Actividad en un rango de tiempo
python3 log_analyzer.py --from "2025-01-07 09:00:00" --to "2025-01-07 18:00:00"

# Métodos de autenticación específicos
python3 log_analyzer.py --method authLogin

# Solo errores críticos
python3 log_analyzer.py --level ERROR

# Combinar filtros: errores de un usuario específico
python3 log_analyzer.py --user admin --errors

# Contar eventos de login en un día
python3 log_analyzer.py --method authLogin --from "2025-01-07 00:00:00" --to "2025-01-07 23:59:59" --count
```

### 3. Ejemplos de Salida CSV

**Servidor (server_activity.csv):**
```csv
timestamp,type,module,level,method,user,client_ip,response_code,message,details
2025-01-07 14:30:15,SYSTEM,Server,INFO,startup,system,localhost,200,Servidor XML-RPC iniciado,Puerto: 8080
2025-01-07 14:30:45,REQUEST,AuthLoginMethod,INFO,authLogin,admin,192.168.1.100,200,Login exitoso,Roles: ["ADMIN"]
2025-01-07 14:31:02,REQUEST,MoveMethod,INFO,move,admin,192.168.1.100,200,Movimiento exitoso,Coordenadas: x=100 y=50 z=25
2025-01-07 14:31:15,ERROR,MoveMethod,ERROR,move,operator,192.168.1.101,400,Error en movimiento,Coordenadas fuera de rango
```

**Cliente Web (web_client.csv):**
```csv
timestamp,type,module,level,method,user,client_ip,response_code,message,details
2025-01-07 14:30:45,REQUEST,auth,INFO,login_success,admin,192.168.1.100,200,Login exitoso para usuario: admin,Roles: ["ADMIN"]
2025-01-07 14:30:50,SYSTEM,panel,INFO,page_access,admin,192.168.1.100,200,Acceso al panel principal,
2025-01-07 14:31:00,SYSTEM,robot,INFO,robot_move_attempt,admin,192.168.1.100,200,Intento de movimiento,Coordenadas: x=100 y=50 z=25
```

## Verificación de Cumplimiento de Requisitos

### ✅ Archivo de registro acumulativo CSV
- **Implementado**: CSVLogger con formato estándar CSV
- **Ubicación**: `logs/server_activity.csv` (servidor), `logs/web_client.csv` (cliente)

### ✅ Información de peticiones XML-RPC
- **Marca de tiempo**: ✅ Formato YYYY-MM-DD HH:MM:SS
- **Detalle de petición**: ✅ Método XML-RPC y parámetros
- **Usuario**: ✅ Username extraído del token
- **Nodo origen**: ✅ IP del cliente
- **Código respuesta**: ✅ HTTP status codes (200, 400, 401, 500)

### ✅ Información de funcionamiento general
- **Marca de tiempo**: ✅ En todos los eventos
- **Módulo**: ✅ Identificación del componente (Server, AuthLoginMethod, etc.)
- **Detalle del evento**: ✅ Mensajes descriptivos con contexto

### ✅ Reporte administrativo con filtros
- **Herramienta**: ✅ `log_analyzer.py` con CLI completa
- **Criterio 1 - Usuario**: ✅ `--user <username>`
- **Criterio 2 - Fecha**: ✅ `--from` y `--to` con timestamps
- **Criterio 3 - Método**: ✅ `--method <xml-rpc-method>`
- **Criterio 4 - Nivel**: ✅ `--level <DEBUG|INFO|WARNING|ERROR|CRITICAL>`
- **Tipos de reporte**: ✅ Resumen, por usuario, errores, conteo

### ✅ Logging integral en toda la aplicación
- **Servidor C++**: ✅ Todos los métodos XML-RPC integrados
- **Cliente Web**: ✅ Todas las rutas con logging de requests
- **Manejo de errores**: ✅ Logging automático de excepciones
- **Eventos del sistema**: ✅ Startup, shutdown, status checks

## Estado de Compilación y Testing

### ✅ Servidor C++
- **Compilación**: ✅ Exitosa con `make`
- **Executable**: ✅ `servidor_rpc` generado
- **Dependencias**: ✅ CSVLogger.o integrado en Makefile

### ✅ Cliente Web Node.js
- **Dependencias**: ✅ `npm install` exitoso
- **Logger Service**: ✅ WebLogger class implementada
- **Route Integration**: ✅ Todos los routes con logging

### ✅ Herramientas de Análisis
- **Python Script**: ✅ `log_analyzer.py` funcional
- **CLI Interface**: ✅ Help system y argumentos implementados
- **Filtering**: ✅ Múltiples criterios de filtrado operativos

## Conclusión

El sistema de logging implementado cumple completamente con los requisitos académicos especificados y proporciona una infraestructura robusta para el monitoreo, auditoría y análisis del sistema Robot Control. La implementación incluye:

1. **Logging CSV estándar** con formato consistente entre servidor y cliente
2. **Trazabilidad completa** de peticiones XML-RPC con usuario, IP y códigos de respuesta
3. **Eventos del sistema** con timestamps, módulos y detalles
4. **Herramienta administrativa** con filtrado por múltiples criterios
5. **Integración integral** en toda la aplicación
6. **Thread-safety** para entornos multi-usuario
7. **Escalabilidad** para crecimiento futuro del sistema

El sistema está listo para uso en producción y cumple con los estándares de logging empresarial.