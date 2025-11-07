import Router from "express-promise-router";
import { requireLogin } from "../middleware/auth.js";
import { rpc } from "../services/xmlrpc.js";
import { logAuth, logSystem, logError } from "../services/logger.js";

const router = new Router();

router.get("/", (req, res) => res.redirect("/panel"));

router.get("/panel", requireLogin, async (req, res) => {
  try {
    const username = req.session?.user?.username || 'unknown';
    logSystem(req, 'panel_access', `Acceso al panel principal por usuario: ${username}`);
    
    console.log("Panel route hit!");
    console.log("Session:", req.session);
    console.log("User:", req.session.user);
    
    let status = null;
    let error = null;
    
    // Obtener estado del robot
    try {
      status = await rpc.myStatus(req.session.token);
      logSystem(req, 'status_check', `Estado del robot obtenido exitosamente`);
    } catch (e) {
      error = "Error obteniendo estado: " + (e?.message || String(e));
      console.error("Error getting status:", e);
      logError(req, 'status_check', `Error obteniendo estado del robot: ${e?.message || String(e)}`);
    }
    
    console.log("About to render template");
    
    res.render("panel/index", { 
      status, 
      error, 
      user: req.session.user 
    });
    
    console.log("Template rendered successfully");
  } catch (err) {
    console.error("Panel route error:", err);
    res.status(500).send("Error interno: " + err.message);
  }
});

// Ruta para gestión de usuarios (solo ADMIN)
router.get("/panel/users", requireLogin, async (req, res) => {
  // Verificar que el usuario sea ADMIN
  if (!req.session.roles?.includes('ADMIN')) {
    return res.status(403).render("403", { 
      message: "Acceso denegado - Se requiere rol de administrador" 
    });
  }
  
  res.render("panel/users");
});

// API Routes para gestión de usuarios
router.get("/api/users", requireLogin, async (req, res) => {
  try {
    // Verificar que el usuario sea ADMIN
    if (!req.session.roles?.includes('ADMIN')) {
      return res.status(403).json({ 
        success: false, 
        message: "Acceso denegado - Se requiere rol de administrador" 
      });
    }

    const result = await rpc.listUsers(req.session.token);
    res.json(result);
  } catch (error) {
    res.status(500).json({ 
      success: false, 
      message: error.message 
    });
  }
});

router.post("/api/users", requireLogin, async (req, res) => {
  try {
    // Verificar que el usuario sea ADMIN
    if (!req.session.roles?.includes('ADMIN')) {
      return res.status(403).json({ 
        success: false, 
        message: "Acceso denegado - Se requiere rol de administrador" 
      });
    }

    const { username, password, role } = req.body;
    
    if (!username || !password || !role) {
      return res.status(400).json({
        success: false,
        message: "Faltan campos requeridos"
      });
    }

    const result = await rpc.createUser(req.session.token, username, password, role);
    res.json(result);
  } catch (error) {
    res.status(500).json({ 
      success: false, 
      message: error.message 
    });
  }
});

router.get("/api/users/:username", requireLogin, async (req, res) => {
  try {
    // Verificar que el usuario sea ADMIN
    if (!req.session.roles?.includes('ADMIN')) {
      return res.status(403).json({ 
        success: false, 
        message: "Acceso denegado - Se requiere rol de administrador" 
      });
    }

    const result = await rpc.getUserInfo(req.session.token, req.params.username);
    res.json(result);
  } catch (error) {
    res.status(500).json({ 
      success: false, 
      message: error.message 
    });
  }
});

// Actualizar usuario
router.put("/api/users/:username", requireLogin, async (req, res) => {
  try {
    // Verificar que el usuario sea ADMIN
    if (!req.session.roles?.includes('ADMIN')) {
      return res.status(403).json({ 
        success: false, 
        message: "Acceso denegado - Se requiere rol de administrador" 
      });
    }

    const { password, role } = req.body;
    const username = req.params.username;
    
    // Construir objeto updates solo con campos que se quieren cambiar
    const updates = {};
    if (password) updates.password = password;
    if (role) updates.role = role;
    
    if (Object.keys(updates).length === 0) {
      return res.status(400).json({
        success: false,
        message: "No se proporcionaron cambios"
      });
    }

    const result = await rpc.updateUser(req.session.token, username, updates);
    res.json(result);
  } catch (error) {
    res.status(500).json({ 
      success: false, 
      message: error.message 
    });
  }
});

// Eliminar usuario
router.delete("/api/users/:username", requireLogin, async (req, res) => {
  try {
    // Verificar que el usuario sea ADMIN
    if (!req.session.roles?.includes('ADMIN')) {
      return res.status(403).json({ 
        success: false, 
        message: "Acceso denegado - Se requiere rol de administrador" 
      });
    }

    const username = req.params.username;
    
    // Verificar que no se elimine a sí mismo
    if (username === req.session.user.username) {
      return res.status(400).json({
        success: false,
        message: "No puedes eliminarte a ti mismo"
      });
    }

    const result = await rpc.deleteUser(req.session.token, username);
    res.json(result);
  } catch (error) {
    res.status(500).json({ 
      success: false, 
      message: error.message 
    });
  }
});

export default router;