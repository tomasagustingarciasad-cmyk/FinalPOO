import Router from "express-promise-router";
import { requireLogin } from "../middleware/auth.js";
import { rpc } from "../services/xmlrpc.js";
import { logAuth, logSystem, logError } from "../services/logger.js";


const router = new Router();


router.get("/move", requireLogin, (req, res) => {
    const username = req.session?.user?.username || 'unknown';
    logSystem(req, 'page_access', `Acceso a página de movimiento del robot por usuario: ${username}`);
    res.render("robot/move", { result: null, error: null, last: null });
});


router.post("/move", requireLogin, async (req, res) => {
    const { x, y, z, feed } = req.body;
    const username = req.session?.user?.username || 'unknown';
    
    try {
        logSystem(req, 'robot_move_attempt', `Intento de movimiento por ${username}`, 
                  `Coordenadas: x=${x}, y=${y}, z=${z}, feed=${feed || 100}`);
        
        const result = await rpc.move(req.session.token, Number(x), Number(y), Number(z), feed ? Number(feed) : 100);
        
        logSystem(req, 'robot_move_success', `Movimiento exitoso por ${username}`, 
                  `Resultado: ${JSON.stringify(result)}`);
        
        res.render("robot/move", { result, error: null, last: { x, y, z, feed } });
    } catch (e) {
        logError(req, 'robot_move', `Error en movimiento por ${username}: ${e?.message || String(e)}`, 
                 `Coordenadas solicitadas: x=${x}, y=${y}, z=${z}, feed=${feed || 100}`);
        
        res.status(400).render("robot/move", { result: null, error: e?.message || String(e), last: { x, y, z, feed } });
    }
});


router.post("/homing", requireLogin, async (req, res) => {
    const username = req.session?.user?.username || 'unknown';
    
    try {
        logSystem(req, 'robot_homing_attempt', `Intento de homing por usuario: ${username}`);
        
        const result = await rpc.homing(req.session.token);
        
        logSystem(req, 'robot_homing_success', `Homing exitoso por usuario: ${username}`, 
                  `Resultado: ${JSON.stringify(result)}`);
        
        res.redirect("/panel");
    } catch (e) {
        logError(req, 'robot_homing', `Error en homing por ${username}: ${e?.message || String(e)}`);
        
        res.status(400).render("panel/index", { status: null, error: e?.message || String(e) });
    }
});


router.post("/motors", requireLogin, async (req, res) => {
try {
const on = req.body.on === "1";
await rpc.enableMotors(req.session.token, on);
res.redirect("/panel");
} catch (e) {
res.status(400).render("panel/index", { status: null, error: e?.message || String(e) });
}
});


router.post("/gripper", requireLogin, async (req, res) => {
try {
const on = req.body.on === "1";
await rpc.gripper(req.session.token, on);
res.redirect("/panel");
} catch (e) {
res.status(400).render("panel/index", { status: null, error: e?.message || String(e) });
}
});

router.post("/connect", requireLogin, async (req, res) => {
    const username = req.session?.user?.username || 'unknown';
    
try {
    const { port, baudrate } = req.body;
    
    // Convertir baudrate a número para evitar "type error" en XML-RPC
    const portStr = port || "/dev/ttyUSB0";
    const baudrateNum = parseInt(baudrate || "115200", 10);
    
    logSystem(req, 'robot_connect_attempt', `Intento de conexión del robot por ${username}`, 
              `Puerto: ${portStr}, Baudrate: ${baudrateNum}`);
    
    const result = await rpc.connectRobot(req.session.token, portStr, baudrateNum);
    
    logSystem(req, 'robot_connect_success', `Robot conectado exitosamente por ${username}`, 
              `Respuesta: ${JSON.stringify(result)}`);
    
    res.redirect("/panel?success=" + encodeURIComponent("Robot conectado: " + result.message));
} catch (e) {
    logError(req, 'robot_connect', `Error conectando robot por ${username}: ${e?.message || String(e)}`);
    res.status(400).render("panel/index", { status: null, error: e?.message || String(e) });
}
});

router.post("/disconnect", requireLogin, async (req, res) => {
    const username = req.session?.user?.username || 'unknown';
    
try {
    logSystem(req, 'robot_disconnect_attempt', `Intento de desconexión del robot por ${username}`);
    
    const result = await rpc.disconnectRobot(req.session.token);
    
    logSystem(req, 'robot_disconnect_success', `Robot desconectado exitosamente por ${username}`, 
              `Respuesta: ${JSON.stringify(result)}`);
    
    res.redirect("/panel?success=" + encodeURIComponent("Robot desconectado: " + result.message));
} catch (e) {
    logError(req, 'robot_disconnect', `Error desconectando robot por ${username}: ${e?.message || String(e)}`);
res.status(400).render("panel/index", { status: null, error: e?.message || String(e) });
}
});


export default router;