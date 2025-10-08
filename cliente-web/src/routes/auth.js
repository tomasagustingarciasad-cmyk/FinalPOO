import Router from "express-promise-router";
import { rpc } from "../services/xmlrpc.js";


const router = new Router();


router.get("/login", (req, res) => {
if (req.session?.token) return res.redirect("/panel");
res.render("auth/login", { error: null });
});


router.post("/login", async (req, res) => {
const { username, password } = req.body;
try {
const meta = { ua: req.headers["user-agent"], ip: req.ip };
const result = await rpc.login(username, password, meta);
// Expected: { token, user: {id, username}, roles: ['OPERATOR'] }
req.session.token = result.token;
req.session.user = result.user;
req.session.roles = result.roles || [];
res.redirect("/panel");
} catch (err) {
console.error("Login failed:", err?.message || err);
res.status(401).render("auth/login", { error: "Invalid credentials" });
}
});


router.post("/logout", (req, res) => {
req.session.destroy(() => res.redirect("/login"));
});


export default router;