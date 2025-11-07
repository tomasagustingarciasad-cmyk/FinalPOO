#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Herramientas de análisis y reporte de logs del servidor Robot XML-RPC
Trabajo Integrador - POO
"""

import csv
import sys
import argparse
from datetime import datetime, timedelta
import re
from collections import defaultdict, Counter
from typing import List, Dict, Any
import os

class LogEntry:
    """Representa una entrada del log CSV"""
    def __init__(self, row: Dict[str, str]):
        self.timestamp = row.get('timestamp', '')
        self.type = row.get('type', '')
        self.module = row.get('module', '')
        self.level = row.get('level', '')
        self.method = row.get('method', '')
        self.user = row.get('user', '')
        self.client_ip = row.get('client_ip', '')
        self.response_code = int(row.get('response_code', '0')) if row.get('response_code', '0').isdigit() else 0
        self.message = row.get('message', '')
        self.details = row.get('details', '')
        
        # Parsear timestamp
        self.datetime = None
        try:
            self.datetime = datetime.strptime(self.timestamp, '%Y-%m-%d %H:%M:%S.%f')
        except ValueError:
            try:
                self.datetime = datetime.strptime(self.timestamp, '%Y-%m-%d %H:%M:%S')
            except ValueError:
                pass
    
    def __str__(self):
        return f"{self.timestamp} [{self.type}] {self.module} - {self.message}"

class LogAnalyzer:
    """Analizador de logs del servidor"""
    
    def __init__(self, log_file_path: str):
        self.log_file_path = log_file_path
        self.entries: List[LogEntry] = []
        self.load_logs()
    
    def load_logs(self):
        """Carga los logs desde el archivo CSV"""
        if not os.path.exists(self.log_file_path):
            print(f"❌ Error: Archivo de log no encontrado: {self.log_file_path}")
            sys.exit(1)
        
        try:
            with open(self.log_file_path, 'r', encoding='utf-8') as file:
                reader = csv.DictReader(file)
                for row in reader:
                    entry = LogEntry(row)
                    self.entries.append(entry)
            
            print(f"✅ Cargadas {len(self.entries)} entradas del log")
        except Exception as e:
            print(f"❌ Error cargando logs: {e}")
            sys.exit(1)
    
    def filter_by_user(self, username: str) -> List[LogEntry]:
        """Filtra entradas por usuario"""
        return [entry for entry in self.entries if entry.user == username]
    
    def filter_by_date_range(self, start_date: datetime, end_date: datetime) -> List[LogEntry]:
        """Filtra entradas por rango de fechas"""
        return [entry for entry in self.entries 
                if entry.datetime and start_date <= entry.datetime <= end_date]
    
    def filter_by_method(self, method: str) -> List[LogEntry]:
        """Filtra entradas por método XML-RPC"""
        return [entry for entry in self.entries if entry.method == method]
    
    def filter_by_level(self, level: str) -> List[LogEntry]:
        """Filtra entradas por nivel de log"""
        return [entry for entry in self.entries if entry.level.upper() == level.upper()]
    
    def filter_by_type(self, log_type: str) -> List[LogEntry]:
        """Filtra entradas por tipo (REQUEST, SYSTEM, etc.)"""
        return [entry for entry in self.entries if entry.type.upper() == log_type.upper()]
    
    def generate_summary_report(self) -> str:
        """Genera un reporte resumen del log"""
        if not self.entries:
            return "📊 No hay entradas en el log"
        
        # Estadísticas básicas
        total_entries = len(self.entries)
        request_entries = len(self.filter_by_type('REQUEST'))
        system_entries = len(self.filter_by_type('SYSTEM'))
        
        # Contar por nivel
        level_counts = Counter(entry.level for entry in self.entries if entry.level)
        
        # Contar por usuario
        user_counts = Counter(entry.user for entry in self.entries if entry.user and entry.user != '')
        
        # Contar por método
        method_counts = Counter(entry.method for entry in self.entries if entry.method and entry.method != '')
        
        # Contar errores (códigos 4xx y 5xx)
        error_codes = [entry for entry in self.entries if entry.response_code >= 400]
        
        # Rango de fechas
        dates = [entry.datetime for entry in self.entries if entry.datetime]
        date_range = ""
        if dates:
            min_date = min(dates)
            max_date = max(dates)
            date_range = f"Desde: {min_date.strftime('%Y-%m-%d %H:%M:%S')} - Hasta: {max_date.strftime('%Y-%m-%d %H:%M:%S')}"
        
        report = f"""
╔══════════════════════════════════════════════════════════════════════════════════
║ 📊 REPORTE RESUMEN DEL LOG DEL SERVIDOR ROBOT
╠══════════════════════════════════════════════════════════════════════════════════
║ 
║ 📈 ESTADÍSTICAS GENERALES:
║    • Total de entradas: {total_entries:,}
║    • Peticiones (REQUEST): {request_entries:,}
║    • Eventos de sistema (SYSTEM): {system_entries:,}
║    • Errores (códigos 4xx/5xx): {len(error_codes):,}
║
║ 📅 RANGO TEMPORAL:
║    {date_range}
║
║ 🏷️ DISTRIBUCIÓN POR NIVEL:"""
        
        for level, count in level_counts.most_common():
            percentage = (count / total_entries) * 100
            report += f"\n║    • {level}: {count:,} ({percentage:.1f}%)"
        
        report += f"\n║\n║ 👥 TOP USUARIOS ACTIVOS:"
        for user, count in user_counts.most_common(10):
            if user and user != "unknown":
                percentage = (count / total_entries) * 100
                report += f"\n║    • {user}: {count:,} actividades ({percentage:.1f}%)"
        
        report += f"\n║\n║ 🔧 MÉTODOS MÁS UTILIZADOS:"
        for method, count in method_counts.most_common(10):
            if method:
                percentage = (count / total_entries) * 100
                report += f"\n║    • {method}: {count:,} llamadas ({percentage:.1f}%)"
        
        if error_codes:
            report += f"\n║\n║ ⚠️ ERRORES RECIENTES:"
            recent_errors = sorted(error_codes, key=lambda x: x.datetime or datetime.min, reverse=True)[:5]
            for error in recent_errors:
                report += f"\n║    • [{error.timestamp}] {error.method or 'N/A'} - Código {error.response_code}: {error.message}"
        
        report += "\n╚══════════════════════════════════════════════════════════════════════════════════"
        
        return report
    
    def generate_user_report(self, username: str) -> str:
        """Genera un reporte específico por usuario"""
        user_entries = self.filter_by_user(username)
        
        if not user_entries:
            return f"📋 No se encontraron actividades para el usuario: {username}"
        
        # Estadísticas del usuario
        total_activities = len(user_entries)
        requests = len([e for e in user_entries if e.type == 'REQUEST'])
        successful_requests = len([e for e in user_entries if e.type == 'REQUEST' and e.response_code < 400])
        failed_requests = len([e for e in user_entries if e.type == 'REQUEST' and e.response_code >= 400])
        
        # Métodos más usados por el usuario
        method_counts = Counter(e.method for e in user_entries if e.method)
        
        # Actividad por día
        daily_activity = defaultdict(int)
        for entry in user_entries:
            if entry.datetime:
                day = entry.datetime.strftime('%Y-%m-%d')
                daily_activity[day] += 1
        
        report = f"""
╔══════════════════════════════════════════════════════════════════════════════════
║ 👤 REPORTE DE ACTIVIDAD DEL USUARIO: {username.upper()}
╠══════════════════════════════════════════════════════════════════════════════════
║
║ 📊 RESUMEN DE ACTIVIDAD:
║    • Total de actividades: {total_activities:,}
║    • Peticiones totales: {requests:,}
║    • Peticiones exitosas: {successful_requests:,}
║    • Peticiones fallidas: {failed_requests:,}
║    • Tasa de éxito: {(successful_requests/requests*100 if requests > 0 else 0):.1f}%
║
║ 🔧 MÉTODOS MÁS UTILIZADOS:"""
        
        for method, count in method_counts.most_common(10):
            if method:
                percentage = (count / total_activities) * 100
                report += f"\n║    • {method}: {count:,} veces ({percentage:.1f}%)"
        
        report += f"\n║\n║ 📅 ACTIVIDAD DIARIA (últimos 7 días):"
        for day in sorted(daily_activity.keys(), reverse=True)[:7]:
            count = daily_activity[day]
            report += f"\n║    • {day}: {count:,} actividades"
        
        # Últimas actividades
        recent_activities = sorted(user_entries, key=lambda x: x.datetime or datetime.min, reverse=True)[:10]
        report += f"\n║\n║ 🕒 ÚLTIMAS ACTIVIDADES:"
        for activity in recent_activities:
            status = "✅" if activity.response_code < 400 else "❌"
            method = activity.method or "system"
            report += f"\n║    {status} [{activity.timestamp[:19]}] {method}: {activity.message[:50]}..."
        
        report += "\n╚══════════════════════════════════════════════════════════════════════════════════"
        
        return report
    
    def generate_connection_debug_report(self) -> str:
        """Genera un reporte detallado de errores de conexión XML-RPC y Robot"""
        connection_keywords = ['conexi', 'connect', 'xmlrpc', 'robot', 'puerto', 'serial', 'timeout', 'fault', 'error']
        
        connection_entries = []
        for entry in self.entries:
            message_lower = (entry.message + ' ' + entry.details).lower()
            if any(keyword in message_lower for keyword in connection_keywords):
                connection_entries.append(entry)
        
        if not connection_entries:
            return "✅ No se encontraron problemas de conexión en el log"
        
        # Separar por tipos de problema
        xmlrpc_errors = [e for e in connection_entries if 'xmlrpc' in (e.message + ' ' + e.details).lower()]
        robot_errors = [e for e in connection_entries if 'robot' in (e.message + ' ' + e.details).lower()]
        serial_errors = [e for e in connection_entries if any(word in (e.message + ' ' + e.details).lower() for word in ['serial', 'puerto', 'port'])]
        timeout_errors = [e for e in connection_entries if 'timeout' in (e.message + ' ' + e.details).lower()]
        
        report = f"""
╔══════════════════════════════════════════════════════════════════════════════════
║ 🔧 ANÁLISIS DE CONEXIÓN Y COMUNICACIÓN
╠══════════════════════════════════════════════════════════════════════════════════
║
║ 📊 RESUMEN DE PROBLEMAS DE CONEXIÓN:
║    • Total de eventos relacionados: {len(connection_entries):,}
║    • Errores XML-RPC: {len(xmlrpc_errors)}
║    • Problemas de Robot: {len(robot_errors)}  
║    • Errores de Puerto Serie: {len(serial_errors)}
║    • Timeouts: {len(timeout_errors)}
║"""
        
        if xmlrpc_errors:
            report += f"""
║ 🌐 ERRORES XML-RPC ({len(xmlrpc_errors)} eventos):"""
            for entry in xmlrpc_errors[-5:]:  # Últimos 5
                report += f"""
║    [{entry.timestamp[:19]}] {entry.level}: {entry.message}
║       Detalles: {entry.details[:80]}..."""
        
        if robot_errors:
            report += f"""
║
║ 🤖 PROBLEMAS DE ROBOT ({len(robot_errors)} eventos):"""
            for entry in robot_errors[-5:]:  # Últimos 5
                report += f"""
║    [{entry.timestamp[:19]}] {entry.level}: {entry.message}
║       Detalles: {entry.details[:80]}..."""
        
        if serial_errors:
            report += f"""
║
║ 🔌 ERRORES DE PUERTO SERIE ({len(serial_errors)} eventos):"""
            for entry in serial_errors[-5:]:  # Últimos 5
                report += f"""
║    [{entry.timestamp[:19]}] {entry.level}: {entry.message}
║       Detalles: {entry.details[:80]}..."""
        
        if timeout_errors:
            report += f"""
║
║ ⏱️ TIMEOUTS ({len(timeout_errors)} eventos):"""
            for entry in timeout_errors[-3:]:  # Últimos 3
                report += f"""
║    [{entry.timestamp[:19]}] {entry.level}: {entry.message}
║       Detalles: {entry.details[:80]}..."""
        
        # Recomendaciones
        report += f"""
║
║ 💡 RECOMENDACIONES DE DIAGNÓSTICO:"""
        
        if xmlrpc_errors:
            report += """
║    • Verificar que el servidor XML-RPC esté ejecutándose en puerto 8080
║    • Revisar firewall y conectividad de red
║    • Comprobar formato de parámetros en llamadas XML-RPC"""
        
        if robot_errors or serial_errors:
            report += """
║    • Verificar conexión física del robot al puerto serie
║    • Revisar permisos del puerto serie (/dev/ttyUSB0, /dev/ttyACM0, etc.)
║    • Comprobar velocidad de baudios (típicamente 115200)
║    • Verificar que el firmware del robot esté respondiendo"""
        
        if timeout_errors:
            report += """
║    • Robot puede estar sobrecargado o respondiendo lentamente
║    • Revisar comandos G-code complejos que toman mucho tiempo
║    • Considerar aumentar timeouts en comandos largos"""
        
        report += """
║
║ 🔍 COMANDOS ÚTILES PARA DIAGNÓSTICO:
║    • Filtrar solo errores: --errors --level ERROR
║    • Ver actividad reciente: --from "$(date -d '1 hour ago' '+%Y-%m-%d %H:%M:%S')"
║    • Analizar método específico: --method move --level DEBUG
║
╚══════════════════════════════════════════════════════════════════════════════════"""
        
        return report
    
    def generate_error_report(self) -> str:
        """Genera un reporte de errores"""
        error_entries = [e for e in self.entries if e.response_code >= 400 or e.level in ['ERROR', 'CRITICAL']]
        
        if not error_entries:
            return "✅ No se encontraron errores en el log"
        
        # Agrupar errores por código
        error_by_code = defaultdict(list)
        for entry in error_entries:
            code = entry.response_code if entry.response_code >= 400 else entry.level
            error_by_code[code].append(entry)
        
        report = f"""
╔══════════════════════════════════════════════════════════════════════════════════
║ ⚠️ REPORTE DE ERRORES DEL SERVIDOR
╠══════════════════════════════════════════════════════════════════════════════════
║
║ 📊 RESUMEN:
║    • Total de errores: {len(error_entries):,}
║    • Códigos de error únicos: {len(error_by_code)}
║
║ 📋 ERRORES POR CÓDIGO:"""
        
        for code, entries in sorted(error_by_code.items(), key=lambda x: len(x[1]), reverse=True):
            count = len(entries)
            latest = max(entries, key=lambda x: x.datetime or datetime.min)
            report += f"\n║    • {code}: {count:,} ocurrencias (última: {latest.timestamp[:19]})"
        
        # Errores más recientes
        recent_errors = sorted(error_entries, key=lambda x: x.datetime or datetime.min, reverse=True)[:15]
        report += f"\n║\n║ 🕒 ERRORES RECIENTES:"
        for error in recent_errors:
            user = error.user or "N/A"
            method = error.method or error.module
            report += f"\n║    [{error.timestamp[:19]}] {user} -> {method}: {error.message[:60]}..."
        
        report += "\n╚══════════════════════════════════════════════════════════════════════════════════"
        
        return report

def main():
    parser = argparse.ArgumentParser(
        description="Analizador de logs del servidor Robot XML-RPC",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Ejemplos de uso:
  %(prog)s -f /ruta/logs/server_activity.csv --summary
  %(prog)s -f logs/server_activity.csv --user admin
  %(prog)s -f logs/server_activity.csv --errors
  %(prog)s -f logs/server_activity.csv --method authLogin
  %(prog)s -f logs/server_activity.csv --level ERROR
  %(prog)s -f logs/server_activity.csv --from "2025-11-07 09:00:00" --to "2025-11-07 18:00:00"
        """)
    
    parser.add_argument('-f', '--file', 
                       default='logs/web_client_activity.csv',
                       help='Ruta al archivo de log CSV (default: logs/web_client_activity.csv)')
    
    parser.add_argument('--summary', action='store_true',
                       help='Genera un reporte resumen general')
    
    parser.add_argument('--user', type=str,
                       help='Genera reporte para un usuario específico')
    
    parser.add_argument('--errors', action='store_true',
                       help='Genera reporte de errores')
    
    parser.add_argument('--connection-debug', action='store_true',
                       help='Análisis detallado de errores de conexión XML-RPC y Robot')
    
    parser.add_argument('--method', type=str,
                       help='Filtra por método XML-RPC específico')
    
    parser.add_argument('--level', type=str,
                       choices=['DEBUG', 'INFO', 'WARNING', 'ERROR', 'CRITICAL'],
                       help='Filtra por nivel de log')
    
    parser.add_argument('--from', dest='from_date', type=str,
                       help='Fecha/hora de inicio (formato: YYYY-MM-DD HH:MM:SS)')
    
    parser.add_argument('--to', dest='to_date', type=str,
                       help='Fecha/hora de fin (formato: YYYY-MM-DD HH:MM:SS)')
    
    parser.add_argument('--count', action='store_true',
                       help='Solo mostrar conteo de entradas filtradas')
    
    args = parser.parse_args()
    
    # Crear analizador
    analyzer = LogAnalyzer(args.file)
    
    if not analyzer.entries:
        print("❌ No hay entradas que analizar")
        sys.exit(1)
    
    # Aplicar filtros
    filtered_entries = analyzer.entries
    
    if args.from_date:
        try:
            start_date = datetime.strptime(args.from_date, '%Y-%m-%d %H:%M:%S')
            filtered_entries = [e for e in filtered_entries 
                              if e.datetime and e.datetime >= start_date]
        except ValueError:
            print("❌ Error: Formato de fecha inválido para --from")
            sys.exit(1)
    
    if args.to_date:
        try:
            end_date = datetime.strptime(args.to_date, '%Y-%m-%d %H:%M:%S')
            filtered_entries = [e for e in filtered_entries 
                              if e.datetime and e.datetime <= end_date]
        except ValueError:
            print("❌ Error: Formato de fecha inválido para --to")
            sys.exit(1)
    
    if args.method:
        filtered_entries = [e for e in filtered_entries if e.method == args.method]
    
    if args.level:
        filtered_entries = [e for e in filtered_entries if e.level.upper() == args.level.upper()]
    
    # Generar reportes
    if args.summary:
        print(analyzer.generate_summary_report())
    elif args.user:
        print(analyzer.generate_user_report(args.user))
    elif args.errors:
        print(analyzer.generate_error_report())
    elif args.connection_debug:
        print(analyzer.generate_connection_debug_report())
    elif args.count:
        print(f"📊 Total de entradas filtradas: {len(filtered_entries):,}")
    else:
        # Mostrar entradas filtradas
        if not filtered_entries:
            print("📋 No se encontraron entradas con los filtros aplicados")
        else:
            print(f"📋 Mostrando {len(filtered_entries):,} entradas:")
            print("="*100)
            for entry in filtered_entries[-50:]:  # Mostrar las últimas 50
                print(f"[{entry.timestamp}] {entry.type:7} | {entry.level:8} | {entry.module:15} | {entry.message}")

if __name__ == '__main__':
    main()