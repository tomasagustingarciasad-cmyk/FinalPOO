import { Router } from "express";
import xmlrpc from "xmlrpc";
import { requireLogin } from "../middleware/auth.js";
import { logAuth, logSystem, logError } from "../services/logger.js";

const router = Router();
const client = xmlrpc.createClient({
    host: "localhost",
    port: 8080,
    path: "/RPC2"
});

// Promisificar las llamadas XML-RPC
const callRPC = (method, params) => {
    return new Promise((resolve, reject) => {
        client.methodCall(method, params, (error, value) => {
            if (error) reject(error);
            else resolve(value);
        });
    });
};

// Página principal del modo de aprendizaje
router.get("/", requireLogin, (req, res) => {
    const username = req.session?.user?.username || 'unknown';
    logSystem(req, 'learning_access', `Acceso al modo de aprendizaje por usuario: ${username}`);
    
    res.render("learning/index", {
        title: "Modo de Aprendizaje",
        user: req.session.user
    });
});

// API para obtener posición actual del robot
router.get("/api/position", requireLogin, async (req, res) => {
    try {
        const result = await callRPC("getPosition", []);
        res.json(result);
    } catch (error) {
        console.error("Error getting position:", error);
        res.status(500).json({ 
            success: false, 
            message: "Error al obtener posición: " + error.message 
        });
    }
});

// API para habilitar/deshabilitar tracking
router.post("/api/tracking", requireLogin, async (req, res) => {
    try {
        const { enable } = req.body;
        const result = await callRPC("setPositionTracking", [enable]);
        res.json(result);
    } catch (error) {
        console.error("Error setting tracking:", error);
        res.status(500).json({ 
            success: false, 
            message: "Error al configurar tracking: " + error.message 
        });
    }
});

// API para mover el robot (esto se trackeará automáticamente)
router.post("/api/move", requireLogin, async (req, res) => {
    try {
        const { x, y, z, feedrate } = req.body;
        const result = await callRPC("move", [
            parseFloat(x), 
            parseFloat(y), 
            parseFloat(z), 
            parseFloat(feedrate) || 1000
        ]);
        res.json(result);
    } catch (error) {
        console.error("Error moving robot:", error);
        res.status(500).json({ 
            success: false, 
            message: "Error al mover robot: " + error.message 
        });
    }
});

// API para controlar el efector final
router.post("/api/endeffector", requireLogin, async (req, res) => {
    try {
        const { state } = req.body;
        const result = await callRPC("endEffector", [state]);
        res.json(result);
    } catch (error) {
        console.error("Error controlling end effector:", error);
        res.status(500).json({ 
            success: false, 
            message: "Error al controlar efector: " + error.message 
        });
    }
});

// API para generar y guardar rutina G-code desde movimientos
router.post("/api/save-learned-routine", requireLogin, async (req, res) => {
    try {
        const { routineName, description, movements } = req.body;
        const token = req.session.token;
        
        if (!routineName || !movements || movements.length === 0) {
            return res.status(400).json({
                success: false,
                message: "Nombre de rutina y movimientos son requeridos"
            });
        }
        
        const result = await callRPC("generateGcodeFromMovements", [
            token,
            routineName,
            description || "Trayectoria aprendida en modo manual",
            movements
        ]);
        
        res.json(result);
    } catch (error) {
        console.error("Error saving learned routine:", error);
        res.status(500).json({ 
            success: false, 
            message: "Error al guardar rutina: " + error.message 
        });
    }
});

export default router;