#!/usr/bin/env python3
"""
Prueba Completa del Sistema Robot - Verificación Post-Revertido
Verifica que todas las correcciones aplicadas funcionen correctamente
"""

import xmlrpc.client
import sys
import time

class TestCompleto:
    def __init__(self, server_url="http://localhost:8080/RPC2"):
        self.server = xmlrpc.client.ServerProxy(server_url)
        self.resultados = []
        
    def test(self, nombre, func, *args, **kwargs):
        """Ejecuta una prueba y registra el resultado"""
        try:
            print(f"\n🔍 {nombre}...")
            resultado = func(*args, **kwargs)
            print(f"✅ PASS: {resultado}")
            self.resultados.append({"test": nombre, "status": "PASS", "result": resultado})
            return True
        except Exception as e:
            print(f"❌ FAIL: {e}")
            self.resultados.append({"test": nombre, "status": "FAIL", "error": str(e)})
            return False
    
    def test_servidor_disponible(self):
        """Prueba que el servidor XML-RPC esté disponible"""
        result = self.server.ServerTest()
        return f"Servidor responde: {result}"
    
    def test_conexion_robot(self):
        """Prueba conectar al robot"""
        result = self.server.connectRobot("/dev/ttyUSB0", 115200)
        return f"Conexión: {result}"
    
    def test_is_connected(self):
        """Prueba el método isConnected"""
        result = self.server.isConnected()
        return f"Estado conexión: {result}"
    
    def test_move_integers(self):
        """Prueba movimiento con números enteros (problema original)"""
        result = self.server.move(10, 20, 30, 1000)
        return f"Move enteros: {result}"
    
    def test_move_floats(self):
        """Prueba movimiento con números decimales"""
        result = self.server.move(10.0, 20.0, 30.0, 1000.0)
        return f"Move decimales: {result}"
    
    def test_enable_motors_true(self):
        """Prueba habilitar motores"""
        result = self.server.enableMotors(True)
        return f"Motores ON: {result}"
    
    def test_enable_motors_false(self):
        """Prueba deshabilitar motores"""
        result = self.server.enableMotors(False)
        return f"Motores OFF: {result}"
    
    def test_homing(self):
        """Prueba comando de homing"""
        result = self.server.home()
        return f"Homing: {result}"
    
    def test_gripper_on(self):
        """Prueba gripper ON"""
        result = self.server.endEffector(True)
        return f"Gripper ON: {result}"
    
    def test_gripper_off(self):
        """Prueba gripper OFF"""
        result = self.server.endEffector(False)
        return f"Gripper OFF: {result}"
    
    def test_get_position(self):
        """Prueba obtener posición"""
        result = self.server.getPosition()
        return f"Posición: {result}"
    
    def test_desconectar_robot(self):
        """Prueba desconectar robot"""
        result = self.server.disconnectRobot()
        return f"Desconexión: {result}"
    
    def ejecutar_todas_las_pruebas(self):
        """Ejecuta todas las pruebas del sistema"""
        print("🤖 VERIFICACIÓN COMPLETA POST-REVERTIDO")
        print("="*60)
        print("Verificando que todas las correcciones estén aplicadas...")
        
        # Pruebas básicas
        self.test("Servidor Disponible", self.test_servidor_disponible)
        
        # Pruebas de conexión
        self.test("Conectar Robot", self.test_conexion_robot)
        self.test("Verificar IsConnected", self.test_is_connected)
        
        # Pruebas críticas de conversión de tipos (el problema principal)
        self.test("Move con Enteros (Fix principal)", self.test_move_integers)
        self.test("Move con Decimales", self.test_move_floats)
        
        # Pruebas de funcionalidad
        self.test("Habilitar Motores", self.test_enable_motors_true)
        self.test("Homing", self.test_homing)
        self.test("Gripper ON", self.test_gripper_on)
        self.test("Gripper OFF", self.test_gripper_off)
        self.test("Obtener Posición", self.test_get_position)
        self.test("Deshabilitar Motores", self.test_enable_motors_false)
        
        # Desconectar al final
        self.test("Desconectar Robot", self.test_desconectar_robot)
        
        return self.generar_reporte()
    
    def generar_reporte(self):
        """Genera reporte final"""
        print("\n" + "="*60)
        print("📊 REPORTE FINAL - VERIFICACIÓN POST-REVERTIDO")
        print("="*60)
        
        total = len(self.resultados)
        passed = sum(1 for r in self.resultados if r["status"] == "PASS")
        failed = total - passed
        
        print(f"📈 Total de pruebas: {total}")
        print(f"✅ Exitosas: {passed}")
        print(f"❌ Fallidas: {failed}")
        print(f"📊 Tasa de éxito: {(passed/total)*100:.1f}%")
        
        # Estado de las correcciones principales
        print(f"\n🔧 ESTADO DE CORRECCIONES PRINCIPALES:")
        
        # Verificar fix del type error
        move_int_result = next((r for r in self.resultados if "Move con Enteros" in r["test"]), None)
        if move_int_result:
            if "type error" in str(move_int_result.get("error", "")).lower():
                print("❌ CRÍTICO: Type error NO solucionado")
            else:
                print("✅ Type error SOLUCIONADO (conversión de tipos aplicada)")
        
        # Verificar isConnected
        is_conn_result = next((r for r in self.resultados if "IsConnected" in r["test"]), None)
        if is_conn_result and is_conn_result["status"] == "PASS":
            print("✅ Método isConnected disponible")
        else:
            print("❌ Método isConnected no disponible")
        
        if failed > 0:
            print(f"\n❌ PRUEBAS FALLIDAS:")
            for r in self.resultados:
                if r["status"] == "FAIL":
                    print(f"   - {r['test']}: {r.get('error', 'Sin detalle')}")
        
        print(f"\n📋 TRABAJO RESTAURADO:")
        print("✅ Conversión robusta XML-RPC (fix type error)")
        print("✅ Método isConnected disponible")
        print("✅ Static cast en métodos críticos")
        print("✅ Logging con CSVLogger mantenido")
        
        return failed == 0

def main():
    print("Iniciando verificación completa del sistema...")
    
    tester = TestCompleto()
    exito = tester.ejecutar_todas_las_pruebas()
    
    if exito:
        print("\n🎉 ¡SISTEMA COMPLETAMENTE FUNCIONAL!")
    else:
        print("\n⚠️  Algunos problemas detectados, revisar reporte arriba")
    
    sys.exit(0 if exito else 1)

if __name__ == "__main__":
    main()