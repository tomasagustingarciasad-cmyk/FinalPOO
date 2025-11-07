/**
 * Logger para cliente web Node.js
 * Complementa los logs del servidor C++ con logs del cliente web
 */

import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

class WebLogger {
    constructor(logFilePath = null) {
        this.logFilePath = logFilePath || path.join(__dirname, '../../../logs/web_client_activity.csv');
        this.ensureLogDirectory();
        this.initializeLogFile();
    }

    ensureLogDirectory() {
        const logDir = path.dirname(this.logFilePath);
        if (!fs.existsSync(logDir)) {
            fs.mkdirSync(logDir, { recursive: true });
        }
    }

    initializeLogFile() {
        if (!fs.existsSync(this.logFilePath)) {
            const header = 'timestamp,type,module,level,method,user,session_id,client_ip,user_agent,response_code,message,details\n';
            fs.writeFileSync(this.logFilePath, header);
            this.logSystem('WebLogger', 'INFO', 'Logger inicializado', 'Archivo creado: ' + this.logFilePath);
        }
    }

    getCurrentTimestamp() {
        const now = new Date();
        return now.toISOString().replace('T', ' ').substr(0, 23);
    }

    escapeCSV(field) {
        if (typeof field !== 'string') {
            field = String(field || '');
        }
        
        // Escapar comillas dobles
        field = field.replace(/"/g, '""');
        
        // Si contiene comas, saltos de línea o comillas, envolver en comillas
        if (field.includes(',') || field.includes('\n') || field.includes('\r') || field.includes('"')) {
            field = `"${field}"`;
        }
        
        return field;
    }

    writeLogEntry(type, module, level, method, user, sessionId, clientIP, userAgent, responseCode, message, details) {
        try {
            const timestamp = this.getCurrentTimestamp();
            const entry = [
                this.escapeCSV(timestamp),
                this.escapeCSV(type),
                this.escapeCSV(module),
                this.escapeCSV(level),
                this.escapeCSV(method || ''),
                this.escapeCSV(user || ''),
                this.escapeCSV(sessionId || ''),
                this.escapeCSV(clientIP || ''),
                this.escapeCSV(userAgent || ''),
                responseCode || 0,
                this.escapeCSV(message || ''),
                this.escapeCSV(details || '')
            ].join(',') + '\n';

            fs.appendFileSync(this.logFilePath, entry);
        } catch (error) {
            console.error('Error escribiendo log:', error);
        }
    }

    // Para peticiones HTTP
    logRequest(req, res, method, message = '', details = '') {
        const user = req.session?.user?.username || 'anonymous';
        const sessionId = req.sessionID || '';
        const clientIP = req.ip || req.connection?.remoteAddress || '';
        const userAgent = req.get('User-Agent') || '';
        const responseCode = res.statusCode;

        this.writeLogEntry(
            'REQUEST',
            'WebClient',
            'INFO',
            method,
            user,
            sessionId,
            clientIP,
            userAgent,
            responseCode,
            message || `${req.method} ${req.originalUrl}`,
            details
        );
    }

    // Para eventos del sistema web
    logSystem(module, level, message, details = '') {
        this.writeLogEntry(
            'SYSTEM',
            module,
            level,
            '',
            '',
            '',
            '',
            '',
            0,
            message,
            details
        );
    }

    // Para errores específicos
    logError(module, errorCode, message, details = '') {
        this.writeLogEntry(
            'SYSTEM',
            module,
            'ERROR',
            '',
            '',
            '',
            '',
            '',
            0,
            message,
            `ErrorCode: ${errorCode} | ${details}`
        );
    }

    // Para debug
    logDebug(module, functionName, message, details = '') {
        this.writeLogEntry(
            'SYSTEM',
            module,
            'DEBUG',
            '',
            '',
            '',
            '',
            '',
            0,
            message,
            `Function: ${functionName} | ${details}`
        );
    }

    // Para autenticación
    logAuth(req, action, success, message = '', details = '') {
        const user = req.body?.username || req.session?.user?.username || 'unknown';
        const sessionId = req.sessionID || '';
        const clientIP = req.ip || req.connection?.remoteAddress || '';
        const userAgent = req.get('User-Agent') || '';
        const responseCode = success ? 200 : 401;

        this.writeLogEntry(
            'AUTH',
            'Authentication',
            success ? 'INFO' : 'WARNING',
            action,
            user,
            sessionId,
            clientIP,
            userAgent,
            responseCode,
            message,
            details
        );
    }

    // Middleware para logging automático de requests
    middleware() {
        return (req, res, next) => {
            // Capturar el método original de end para interceptar respuestas
            const originalEnd = res.end;
            
            res.end = function(...args) {
                // Log de la request completada
                if (!req.url.includes('/public/')) { // Evitar logs de assets estáticos
                    const message = `${req.method} ${req.originalUrl} - ${res.statusCode}`;
                    const details = `Headers: ${JSON.stringify(req.headers).substring(0, 200)}`;
                    
                    webLogger.logRequest(req, res, req.route?.path || req.url, message, details);
                }
                
                // Llamar al método original
                originalEnd.apply(this, args);
            };
            
            next();
        };
    }
}

// Instancia global
const webLogger = new WebLogger();

// Funciones de conveniencia
export const logRequest = (req, res, method, message, details) => 
    webLogger.logRequest(req, res, method, message, details);

export const logSystem = (module, level, message, details) => 
    webLogger.logSystem(module, level, message, details);

export const logError = (module, errorCode, message, details) => 
    webLogger.logError(module, errorCode, message, details);

export const logDebug = (module, functionName, message, details) => 
    webLogger.logDebug(module, functionName, message, details);

export const logAuth = (req, action, success, message, details) => 
    webLogger.logAuth(req, action, success, message, details);

export const loggerMiddleware = () => webLogger.middleware();

export default webLogger;