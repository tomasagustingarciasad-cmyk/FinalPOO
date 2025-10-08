import path from "path";
import express from "express";
import session from "express-session";
import cookieParser from "cookie-parser";
import morgan from "morgan";
import dotenv from "dotenv";
import { fileURLToPath } from "url";
import expressLayouts from "express-ejs-layouts";


import authRoutes from "./routes/auth.js";
import panelRoutes from "./routes/panel.js";
import robotRoutes from "./routes/robot.js";


dotenv.config();
const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);


const app = express();


// View engine
app.set("views", path.join(__dirname, "views"));
app.set("view engine", "ejs");
app.use(expressLayouts);
app.set("layout", "layout");


// Middlewares
app.use(morgan("dev"));
app.use(express.urlencoded({ extended: true }));
app.use(express.json());
app.use(cookieParser());
app.use(
session({
name: "sid",
secret: process.env.SESSION_SECRET || "dev-secret",
resave: false,
saveUninitialized: false,
cookie: {
httpOnly: true,
sameSite: "lax",
secure: false // set true behind HTTPS/proxy
}
})
);
app.use("/public", express.static(path.join(__dirname, "public")));


// Locals for templates
app.use((req, res, next) => {
res.locals.user = req.session.user || null;
res.locals.roles = req.session.roles || [];
next();
});


// Routes
app.use("/", authRoutes);
app.use("/", panelRoutes);
app.use("/robot", robotRoutes);


// 404
app.use((req, res) => res.status(404).render("404"));


const port = process.env.PORT || 3000;
app.listen(port, '0.0.0.0', () => console.log(`http://0.0.0.0:${port}`));

// app.listen(port, () => {
// console.log(`Web client listening on http://localhost:${port}`);
// });