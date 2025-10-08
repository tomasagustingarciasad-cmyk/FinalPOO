export function requireLogin(req, res, next) {
if (!req.session?.token) return res.redirect("/login");
next();
}


export function requireRole(role) {
return (req, res, next) => {
const roles = req.session?.roles || [];
if (!roles.includes(role)) return res.status(403).render("403");
next();
};
}