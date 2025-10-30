import Router from "express-promise-router";
import { requireLogin } from "../middleware/auth.js";
import { rpc } from "../services/xmlrpc.js";


const router = new Router();


router.get("/move", requireLogin, (req, res) => {
res.render("robot/move", { result: null, error: null, last: null });
});


router.post("/move", requireLogin, async (req, res) => {
const { x, y, z, feed } = req.body;
try {
const result = await rpc.move(req.session.token, Number(x), Number(y), Number(z), feed ? Number(feed) : 100);
res.render("robot/move", { result, error: null, last: { x, y, z, feed } });
} catch (e) {
res.status(400).render("robot/move", { result: null, error: e?.message || String(e), last: { x, y, z, feed } });
}
});


router.post("/homing", requireLogin, async (req, res) => {
try {
const result = await rpc.homing(req.session.token);
res.redirect("/panel");
} catch (e) {
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


export default router;