import xmlrpc from "xmlrpc";
import dotenv from "dotenv";

dotenv.config();

const MOCK_RPC = process.env.MOCK_RPC === "1" || process.env.MOCK_RPC === "true";

// Cliente XML-RPC real
const client = xmlrpc.createClient({
  host: process.env.RPC_HOST || "localhost",
  port: process.env.RPC_PORT || 8080,
  path: "/RPC2"
});

// Función helper para promisificar llamadas XML-RPC
function callMethod(method, params = []) {
  return new Promise((resolve, reject) => {
    client.methodCall(method, params, (error, value) => {
      if (error) {
        console.error(`XML-RPC Error [${method}]:`, error);
        reject(error);
      } else {
        resolve(value);
      }
    });
  });
}

// ---- MOCK BACKEND (dev only) ---------------------------------------------
function makeMock() {
  // two users: admin and operator
  const users = {
    admin: { id: 1, username: "admin", password: "Admin123!", roles: ["ADMIN"] },
    operador: { id: 2, username: "operador", password: "Operador123!", roles: ["OPERATOR"] }
  };
  let lastMove = null;
  let motorsOn = false;
  let gripperOn = false;

  const mkToken = (u) => Buffer.from(`${u.username}|${Date.now()}`).toString("base64");

  return {
    async login(username, password) {
      const u = users[username];
      if (!u || u.password !== password) {
        const err = new Error("Invalid credentials (MOCK)");
        err.status = 401; throw err;
      }
      return { token: mkToken(u), user: { id: u.id, username: u.username }, roles: u.roles };
    },
    async myStatus(token) {
      return {
        mode: "ABS",
        motorsOn,
        gripperOn,
        position: { x: 0, y: 0, z: 0 },
        lastMove,
        info: "This is MOCK status. Your C++ XML-RPC server is not connected."
      };
    },
    async move(token, x, y, z, feed) {
      lastMove = { x, y, z, feed, at: new Date().toISOString() };
      return { ok: true, message: `MOCK move to X${x} Y${y} Z${z} F${feed ?? "-"}` };
    },
    async moveLinear(token, coords) {
      lastMove = { ...coords, at: new Date().toISOString() };
      return { ok: true, message: `MOCK G1 to X${coords.x} Y${coords.y} Z${coords.z} F${coords.feed ?? "-"}` };
    },
    async homing(token) {
      lastMove = { homing: true, at: new Date().toISOString() };
      return { ok: true, message: "MOCK G28 homing" };
    },
    async enableMotors(token, on) {
      motorsOn = !!on; return { ok: true, motorsOn };
    },
    async gripper(token, on) {
      gripperOn = !!on; return { ok: true, gripperOn };
    }
  };
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

  // Estado del robot
  async myStatus(token) {
    try {
      const result = await callMethod("ServerTest");
      return {
        mode: "ABS",
        motorsOn: false,
        gripperOn: false,
        position: { x: 0, y: 0, z: 0 },
        lastMove: null,
        info: "Conectado al servidor C++ XML-RPC"
      };
    } catch (err) {
      throw new Error("Error obteniendo estado: " + err.message);
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

  // Método genérico para llamadas XML-RPC
  methodCall: callMethod
};

// ---- EXPORTAR SERVICIO SEGUN CONFIGURACIÓN --------------------------------
export const rpc = MOCK_RPC ? makeMock() : real;
