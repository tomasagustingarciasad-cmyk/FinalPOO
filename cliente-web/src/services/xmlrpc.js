import xmlrpc from "xmlrpc";
import dotenv from "dotenv";

dotenv.config();

const MOCK = process.env.MOCK_RPC === "1" || process.env.MOCK_RPC === "true";

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

// ---- REAL XML-RPC CLIENT --------------------------------------------------
const realClient = xmlrpc.createClient({
  host: process.env.RPC_HOST || "localhost",
  port: Number(process.env.RPC_PORT || 8081),
  path: process.env.RPC_PATH || "/"
});

function call(methodName, ...params) {
  return new Promise((resolve, reject) => {
    realClient.methodCall(methodName, params, (err, value) => {
      if (err) return reject(err);
      resolve(value);
    });
  });
}

const real = {
  login: (username, password, meta) => call("auth.login", username, password, meta?.ua || "web", meta?.ip || "0.0.0.0"),
  myStatus: (token) => call("report.myStatus", token),
  moveLinear: (token, coords) => call("robot.moveLinear", token, coords),
  homing: (token) => call("robot.homing", token),
  enableMotors: (token, on) => call("robot.enableMotors", token, !!on),
  gripper: (token, on) => call("robot.gripper", token, !!on)
};

export const rpc = MOCK ? makeMock() : real;
