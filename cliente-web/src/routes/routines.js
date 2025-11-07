import Router from "express-promise-router";
import multer from "multer";
import path from "path";
import fs from "fs/promises";
import { requireLogin } from "../middleware/auth.js";
import { rpc } from "../services/xmlrpc.js";
import { logAuth, logSystem, logError } from "../services/logger.js";

const router = new Router();

// Configurar multer para subida de archivos
const storage = multer.memoryStorage();
const upload = multer({
  storage: storage,
  limits: {
    fileSize: 10 * 1024 * 1024, // 10MB max
  },
  fileFilter: (req, file, cb) => {
    const allowedTypes = ['.gcode', '.nc', '.g', '.txt'];
    const ext = path.extname(file.originalname).toLowerCase();
    if (allowedTypes.includes(ext) || file.mimetype === 'text/plain') {
      cb(null, true);
    } else {
      cb(new Error('Solo se permiten archivos G-code (.gcode, .nc, .g, .txt)'));
    }
  }
});

// GET - Listar rutinas del usuario
router.get("/routines", requireLogin, async (req, res) => {
  try {
    const username = req.session.user.username;
    console.log("Listando rutinas para usuario:", username);
    
    logSystem(req, 'routines_list', `Listando rutinas para usuario: ${username}`);
    
    const result = await rpc.methodCall('routineList', [req.session.token]);
    
    if (!result.success) {
      throw new Error(result.message || 'Error listando rutinas');
    }
    
    const routines = result.routines || [];
    
    logSystem(req, 'routines_list_success', `Lista de rutinas obtenida exitosamente para ${username}`, 
              `Cantidad de rutinas: ${routines.length}`);
    
    res.render("routines/index", { 
      routines,
      user: req.session.user,
      success: req.query.success || null,
      error: req.query.error || null
    });
    
  } catch (error) {
    console.error("Error listando rutinas:", error);
    logError(req, 'routines_list', `Error listando rutinas: ${error?.message || String(error)}`);
    res.render("routines/index", { 
      routines: [],
      user: req.session.user,
      error: "Error al cargar las rutinas: " + error.message,
      success: null
    });
  }
});

// GET - Formulario para crear nueva rutina
router.get("/routines/new", requireLogin, (req, res) => {
  res.render("routines/new", { 
    user: req.session.user 
  });
});

// POST - Crear nueva rutina
router.post("/routines", requireLogin, upload.single('gcodeFile'), async (req, res) => {
  try {
    const { filename, description, gcodeContent } = req.body;
    let finalContent = gcodeContent;
    let finalFilename = filename;
    
    // Si se subió un archivo, usar su contenido
    if (req.file) {
      finalContent = req.file.buffer.toString('utf8');
      finalFilename = req.file.originalname;
    }
    
    if (!finalContent || !finalFilename) {
      return res.redirect('/routines/new?error=' + encodeURIComponent('Se requiere un archivo o contenido G-code'));
    }
    
    console.log("Creando rutina:", {
      filename: finalFilename,
      description,
      contentLength: finalContent.length
    });
    
    const result = await rpc.methodCall('routineCreate', [
      req.session.token,
      finalFilename,
      finalFilename,
      description || 'Sin descripción',
      finalContent
    ]);
    
    if (!result.success) {
      throw new Error(result.message || 'Error creando rutina');
    }
    
    res.redirect('/routines?success=' + encodeURIComponent('Rutina creada exitosamente'));
    
  } catch (error) {
    console.error("Error creando rutina:", error);
    res.redirect('/routines/new?error=' + encodeURIComponent(error.message));
  }
});

// GET - Ver detalles de una rutina
router.get("/routines/:id", requireLogin, async (req, res) => {
  try {
    const routineId = parseInt(req.params.id);
    
    if (isNaN(routineId)) {
      return res.redirect('/routines?error=' + encodeURIComponent('ID de rutina inválido'));
    }
    
    const result = await rpc.methodCall('routineGet', [req.session.token, routineId]);
    
    if (!result.success) {
      throw new Error(result.message || 'Rutina no encontrada');
    }
    
    res.render("routines/view", { 
      routine: result.routine,
      user: req.session.user,
      success: req.query.success || null,
      error: req.query.error || null
    });
    
  } catch (error) {
    console.error("Error obteniendo rutina:", error);
    res.redirect('/routines?error=' + encodeURIComponent(error.message));
  }
});

// GET - Formulario para editar rutina
router.get("/routines/:id/edit", requireLogin, async (req, res) => {
  try {
    const routineId = parseInt(req.params.id);
    
    if (isNaN(routineId)) {
      return res.redirect('/routines?error=' + encodeURIComponent('ID de rutina inválido'));
    }
    
    const result = await rpc.methodCall('routineGet', [req.session.token, routineId]);
    
    if (!result.success) {
      throw new Error(result.message || 'Rutina no encontrada');
    }
    
    res.render("routines/edit", { 
      routine: result.routine,
      user: req.session.user,
      error: req.query.error || null
    });
    
  } catch (error) {
    console.error("Error obteniendo rutina para editar:", error);
    res.redirect('/routines?error=' + encodeURIComponent(error.message));
  }
});

// POST - Actualizar rutina existente
router.post("/routines/:id", requireLogin, upload.single('gcodeFile'), async (req, res) => {
  try {
    const routineId = parseInt(req.params.id);
    const { filename, description, gcodeContent } = req.body;
    let finalContent = gcodeContent;
    
    if (isNaN(routineId)) {
      return res.redirect('/routines?error=' + encodeURIComponent('ID de rutina inválido'));
    }
    
    // Si se subió un archivo nuevo, usar su contenido
    if (req.file) {
      finalContent = req.file.buffer.toString('utf8');
    }
    
    if (!finalContent || !filename) {
      return res.redirect(`/routines/${routineId}/edit?error=` + encodeURIComponent('Se requiere nombre y contenido'));
    }
    
    console.log("Actualizando rutina:", routineId, {
      filename,
      description,
      contentLength: finalContent.length
    });
    
    const result = await rpc.methodCall('routineUpdate', [
      req.session.token,
      routineId,
      filename,
      description || 'Sin descripción',
      finalContent
    ]);
    
    if (!result.success) {
      throw new Error(result.message || 'Error actualizando rutina');
    }
    
    res.redirect(`/routines/${routineId}?success=` + encodeURIComponent('Rutina actualizada exitosamente'));
    
  } catch (error) {
    console.error("Error actualizando rutina:", error);
    res.redirect(`/routines/${req.params.id}/edit?error=` + encodeURIComponent(error.message));
  }
});

// POST - Eliminar rutina
router.post("/routines/:id/delete", requireLogin, async (req, res) => {
  try {
    const routineId = parseInt(req.params.id);
    
    if (isNaN(routineId)) {
      return res.redirect('/routines?error=' + encodeURIComponent('ID de rutina inválido'));
    }
    
    console.log("Eliminando rutina:", routineId);
    
    const result = await rpc.methodCall('routineDelete', [req.session.token, routineId]);
    
    if (!result.success) {
      throw new Error(result.message || 'Error eliminando rutina');
    }
    
    res.redirect('/routines?success=' + encodeURIComponent('Rutina eliminada exitosamente'));
    
  } catch (error) {
    console.error("Error eliminando rutina:", error);
    res.redirect('/routines?error=' + encodeURIComponent(error.message));
  }
});

// POST - Usar/ejecutar rutina (enviar al robot)
router.post("/routines/:id/execute", requireLogin, async (req, res) => {
  try {
    const routineId = parseInt(req.params.id);
    
    if (isNaN(routineId)) {
      return res.redirect('/routines?error=' + encodeURIComponent('ID de rutina inválido'));
    }
    
    // Primero obtener la rutina
    const routineResult = await rpc.methodCall('routineGet', [req.session.token, routineId]);
    
    if (!routineResult.success) {
      throw new Error('No se pudo obtener la rutina');
    }
    
    const routine = routineResult.routine;
    
    // Ejecutar el G-code en el robot usando el servidor
    console.log("Ejecutando rutina:", routine.filename);
    console.log("G-code content:", routine.gcodeContent);
    
    try {
      const executeResult = await rpc.methodCall('executeGcode', [routine.gcodeContent]);
      
      if (!executeResult.success) {
        throw new Error(executeResult.message || 'Error ejecutando G-code en el robot');
      }
      
      const message = `Rutina "${routine.filename}" ejecutada exitosamente. ${executeResult.linesProcessed || 0} líneas procesadas.`;
      res.redirect(`/routines/${routineId}?success=` + encodeURIComponent(message));
      
    } catch (robotError) {
      console.error("Error ejecutando en robot:", robotError);
      throw new Error(`Error ejecutando en robot: ${robotError.message}`);
    }
    
  } catch (error) {
    console.error("Error ejecutando rutina:", error);
    res.redirect(`/routines/${req.params.id}?error=` + encodeURIComponent(error.message));
  }
});

// GET - Descargar rutina como archivo
router.get("/routines/:id/download", requireLogin, async (req, res) => {
  try {
    const routineId = parseInt(req.params.id);
    
    if (isNaN(routineId)) {
      return res.status(400).send('ID de rutina inválido');
    }
    
    const result = await rpc.methodCall('routineGet', [req.session.token, routineId]);
    
    if (!result.success) {
      throw new Error(result.message || 'Rutina no encontrada');
    }
    
    const routine = result.routine;
    
    res.setHeader('Content-Type', 'text/plain');
    res.setHeader('Content-Disposition', `attachment; filename="${routine.filename}"`);
    res.send(routine.gcodeContent);
    
  } catch (error) {
    console.error("Error descargando rutina:", error);
    res.status(500).send('Error descargando rutina: ' + error.message);
  }
});

export default router;