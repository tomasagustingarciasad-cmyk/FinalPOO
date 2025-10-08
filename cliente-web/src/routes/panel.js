import Router from "express-promise-router";
import { requireLogin } from "../middleware/auth.js";
import { rpc } from "../services/xmlrpc.js";


const router = new Router();


router.get("/", (req, res) => res.redirect("/panel"));


router.get("/panel", requireLogin, async (req, res) => {
let status = null, error = null;
try {
status = await rpc.myStatus(req.session.token);
} catch (e) {
error = e?.message || String(e);
}
res.render("panel/index", { status, error });
});


export default router;