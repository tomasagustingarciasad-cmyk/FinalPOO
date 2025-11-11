import xmlrpc from "xmlrpc";
import dotenv from "dotenv";
import { logSystem, logError } from "./logger.js";

dotenv.config();

// Cliente XML-RPC real
const client = xmlrpc.createClient({
  host: process.env.RPC_HOST || "localhost",
  port: process.env.RPC_PORT || 8080,
  path: "/RPC2"
});

// Función helper para promisificar llamadas XML-RPC
function callMethod(method, params = []) {
  return new Promise((resolve, reject) => {
    const startTime = Date.now();
    
    // Log de intento de llamada
    logSystem({ ip: 'client', headers: { 'user-agent': 'xmlrpc-client' } }, 
               'xmlrpc_call', 
               `Llamada XML-RPC: ${method}`, 
               `Parámetros: ${JSON.stringify(params).substring(0, 200)}...`);
    
    client.methodCall(method, params, (error, value) => {
      const duration = Date.now() - startTime;
      
      if (error) {
        console.error(`XML-RPC Error [${method}]:`, error);
        
        // Log detallado del error
        logError({ ip: 'client', headers: { 'user-agent': 'xmlrpc-client' } }, 
                  'xmlrpc_error', 
                  `Error XML-RPC [${method}]: ${error.message}`, 
                  `Código: ${error.code || 'unknown'} | Duración: ${duration}ms | Detalles: ${JSON.stringify(error)}`);
        
        reject(error);
      } else {
        // Log de éxito
        logSystem({ ip: 'client', headers: { 'user-agent': 'xmlrpc-client' } }, 
                   'xmlrpc_success', 
                   `XML-RPC exitoso [${method}]`, 
                   `Duración: ${duration}ms | Respuesta: ${JSON.stringify(value).substring(0, 200)}...`);
        
        resolve(value);
      }
    });
  });
}

// ---- REAL XML-RPC BACKEND ------------------------------------------------
const real = {
  // Autenticación real
  async login(username, password) {
    try {
      const result = await callMethod("authLogin", [username, password]);
      if (result && result.success) {
        return {
          token: result.token,
          user: result.user,
          roles: [result.user.role] // Convertir role único a array para compatibilidad
        };
      } else {
        const error = new Error(result.message || "Credenciales inválidas");
        error.status = 401;
        throw error;
      }
    } catch (err) {
      console.error("Login error:", err);
      const error = new Error("Error de autenticación: " + (err.message || "Desconocido"));
      error.status = 401;
      throw error;
    }
  },

  // Logout
  async logout(token) {
    try {
      const result = await callMethod("authLogout", [token]);
      return result;
    } catch (err) {
      console.error("Logout error:", err);
      throw new Error("Error en logout: " + err.message);
    }
  },

  // Gestión de usuarios
  async createUser(token, username, password, role) {
    try {
      const result = await callMethod("userCreate", [token, username, password, role]);
      return result;
    } catch (err) {
      console.error("Create user error:", err);
      throw new Error("Error creando usuario: " + err.message);
    }
  },

  async listUsers(token) {
    try {
      const result = await callMethod("userList", [token]);
      return result;
    } catch (err) {
      console.error("List users error:", err);
      throw new Error("Error listando usuarios: " + err.message);
    }
  },

  async getUserInfo(token, username) {
    try {
      const result = await callMethod("userInfo", [token, username]);
      return result;
    } catch (err) {
      console.error("Get user info error:", err);
      throw new Error("Error obteniendo información del usuario: " + err.message);
    }
  },

  async updateUser(token, username, updates) {
    try {
      const result = await callMethod("userUpdate", [token, username, updates]);
      return result;
    } catch (err) {
      console.error("Update user error:", err);
      throw new Error("Error actualizando usuario: " + err.message);
    }
  },

  async deleteUser(token, username) {
    try {
      const result = await callMethod("userDelete", [token, username]);
      return result;
    } catch (err) {
      console.error("Delete user error:", err);
      throw new Error("Error eliminando usuario: " + err.message);
    }
  },

  // Estado del robot - método mejorado usando getRobotStatus
  async myStatus(token) {
    try {
      // Usar el nuevo método getRobotStatus que da el estado real
      const statusResult = await callMethod("getRobotStatus");
      
      if (!statusResult.success) {
        throw new Error("Error obteniendo estado del servidor");
      }
      
      const connected = !!statusResult.connected;
      let position = { x: 0, y: 0, z: 0 };
      let info = connected ? "Robot conectado" : "Robot no conectado";
      
      // El servidor ya nos da los valores reales de motorsOn y gripperOn
      const motorsOn = !!statusResult.motorsOn;
      const gripperOn = !!statusResult.gripperOn;
      
      // Intentar obtener posición detallada si está conectado
      if (connected) {
        try {
          const posResult = await callMethod("getPosition");
          if (posResult.success && posResult.position) {
            position = posResult.position;
            info += " - Posición actualizada";
          } else {
            // Usar la posición básica del getRobotStatus
            position = statusResult.position || { x: 0, y: 0, z: 0 };
          }
        } catch (e) {
          position = statusResult.position || { x: 0, y: 0, z: 0 };
          info += " - Posición básica";
        }
      }

      return {
        mode: "ABS",
        motorsOn,
        gripperOn,
        connected,
        position,
        lastMove: null,
        info,
        timestamp: new Date().toISOString()
      };
    } catch (err) {
      throw new Error("Error obteniendo estado: " + err.message);
    }
  },

  // Método para verificar que el servidor está funcionando (llamada explícita)
  async pingServer() {
    try {
      const result = await callMethod("ServerTest");
      return { ok: true, message: "Servidor disponible", result };
    } catch (err) {
      throw new Error("Servidor no disponible: " + err.message);
    }
  },

  // Método para verificar si el robot está conectado
  async isRobotConnected(token) {
    try {
      const result = await callMethod("isConnected");
      return result;
    } catch (err) {
      throw new Error("Error verificando conexión del robot: " + err.message);
    }
  },

  // Método separado para obtener posición cuando se necesite explícitamente
  async getRobotPosition(token) {
    try {
      const result = await callMethod("getPosition");
      return result;
    } catch (err) {
      throw new Error("Error obteniendo posición: " + err.message);
    }
  },

  // Método para obtener estado real del robot (solo cuando se necesite)
  async getRealRobotStatus(token) {
    try {
      // Verificar conexión del robot intentando obtener posición
      const posResult = await callMethod("getPosition");
      
      return {
        mode: "ABS",
        motorsOn: true, // Asumimos que están encendidos si el robot responde
        gripperOn: false,
        connected: true,
        position: posResult.position || { x: 0, y: 0, z: 0 },
        lastMove: null,
        info: "Estado obtenido del robot conectado",
        timestamp: new Date().toISOString()
      };
    } catch (err) {
      return {
        mode: "ABS",
        motorsOn: false,
        gripperOn: false,
        connected: false,
        position: { x: 0, y: 0, z: 0 },
        lastMove: null,
        info: "Robot no conectado o no responde: " + err.message,
        timestamp: new Date().toISOString()
      };
    }
  },

  // Movimientos del robot
  async move(token, x, y, z, feed) {
    try {
      const result = await callMethod("move", [x, y, z, feed || 100]);
      return result;
    } catch (err) {
      throw new Error("Error en movimiento: " + err.message);
    }
  },

  async moveLinear(token, coords) {
    try {
      const result = await callMethod("moveLinear", [coords.x, coords.y, coords.z, coords.feed || 1000]);
      return result;
    } catch (err) {
      throw new Error("Error en movimiento lineal: " + err.message);
    }
  },

  async homing(token) {
    try {
      const result = await callMethod("home");
      return result;
    } catch (err) {
      throw new Error("Error en homing: " + err.message);
    }
  },

  async enableMotors(token, on) {
    try {
      const result = await callMethod("enableMotors", [on]);
      return result;
    } catch (err) {
      throw new Error("Error controlando motores: " + err.message);
    }
  },

  async controlGripper(token, on) {
    try {
      const result = await callMethod("endEffector", [on]);
      return result;
    } catch (err) {
      throw new Error("Error controlando gripper: " + err.message);
    }
  },

  async connectRobot(token, port, baudrate) {
    try {
      const result = await callMethod("connectRobot", [port, baudrate]);
      return result;
    } catch (err) {
      throw new Error("Error conectando robot: " + err.message);
    }
  },

  async disconnectRobot(token) {
    try {
      const result = await callMethod("disconnectRobot", []);
      return result;
    } catch (err) {
      throw new Error("Error desconectando robot: " + err.message);
    }
  },

  async startLearning(token, routineName, description) {
    try {
      return await callMethod("generateGcodeFromMovements", [
        token,
        routineName,
        description || "Rutina aprendida"
      ]);
    } catch (err) {
      throw new Error("Error iniciando aprendizaje: " + err.message);
    }
  },

  async stopLearning(token, routineName, description) {
    try {
      return await callMethod("generateGcodeFromMovements", [
        token,
        routineName,
        description || "Rutina aprendida"
      ]);
    } catch (err) {
      throw new Error("Error finalizando aprendizaje: " + err.message);
    }
  },

  // Método genérico para llamadas XML-RPC
  methodCall: callMethod
};

// ---- EXPORTAR SERVICIO SEGUN CONFIGURACIÓN --------------------------------
export const rpc = real;
