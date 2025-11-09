import Router from "express-promise-router";
import { rpc } from "../services/xmlrpc.js";
import { logAuth, logSystem, logError } from "../services/logger.js";


const router = new Router();


router.get("/login", (req, res) => {
    logSystem(req, 'page_access', 'Acceso a página de login');
    if (req.session?.token) return res.redirect("/panel");
    res.render("auth/login", { error: null });
});


router.post("/login", async (req, res) => {
const { username, password } = req.body;
try {
    logAuth(req, 'login_attempt', false, `Intento de login para usuario: ${username}`);
    
    const meta = { ua: req.headers["user-agent"], ip: req.ip };
    const result = await rpc.login(username, password, meta);
    
    // Expected: { token, user: {id, username}, roles: ['OPERATOR'] }
    req.session.token = result.token;
    req.session.user = result.user;
    req.session.roles = result.roles || [];
    
    logAuth(req, 'login_success', true, `Login exitoso para usuario: ${username}`, 
            `Roles: ${JSON.stringify(result.roles || [])}, SessionID: ${req.sessionID}`);
    
    res.redirect("/panel");
} catch (err) {
    console.error("Login failed:", err?.message || err);
    
    logAuth(req, 'login_failed', false, `Login fallido para usuario: ${username}`, 
            `Error: ${err?.message || err}`);
    
    res.status(401).render("auth/login", { error: "Invalid credentials" });
}
});


router.post("/logout", async (req, res) => {
try {
    const username = req.session?.user?.username || 'unknown';
    
    logAuth(req, 'logout_attempt', false, `Intento de logout para usuario: ${username}`);
    
    if (req.session?.token) {
        await rpc.logout(req.session.token);
    }
    
    req.session.destroy((err) => {
        if (err) {
            console.error("Session destroy error:", err);
            logError(req, 'logout', `Error al destruir sesión: ${err?.message || err}`);
        } else {
            logAuth(req, 'logout_success', true, `Logout exitoso para usuario: ${username}`);
        }
        res.redirect("/login");
    });
} catch (err) {
    console.error("Logout error:", err?.message || err);
    
    logError(req, 'logout', `Error en logout: ${err?.message || err}`);
    
    req.session.destroy(() => {
        res.redirect("/login");
    });
}
});


export default router;