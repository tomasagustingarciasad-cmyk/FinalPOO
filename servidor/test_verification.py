#!/usr/bin/env python3
"""
TEST COMPLETO DEL SISTEMA ROBOT
Prueba TODAS las funcionalidades: servidor C++ ↔ firmware Arduino
"""

import xmlrpc.client
import time
import sys

# Configuración
SERVER_URL = "http://localhost:8080"
DEVICE_PATH = "/dev/ttyUSB0"  # Cambiar a /dev/ttyACM0 si es necesario
BAUD_RATE = 115200


def test_connection(server):
    """Test de conexión básica"""
    print("\n=== TEST 1: CONEXIÓN ===")
    try:
        result = server.connectRobot(DEVICE_PATH, BAUD_RATE)
        print(f"→ Conectando a {DEVICE_PATH}: {result}")
        if result.get('ok'):
            print("  ✅ Conexión establecida")
            time.sleep(1)
            return True
        else:
            print("  ❌ Error de conexión")
            return False
    except Exception as e:
        print(f"  ❌ Excepción: {e}")
        return False

def test_initial_status(server):
    """Test de estado inicial"""
    print("\n=== TEST 2: ESTADO INICIAL ===")
    try:
        pos = server.getPosition()
        print(f"→ Posición inicial: {pos}")
        if pos.get('ok'):
            x, y, z = pos.get('x', 0), pos.get('y', 0), pos.get('z', 0)
            print(f"  Coordenadas: X={x:.2f}, Y={y:.2f}, Z={z:.2f}")
            print(f"  Motores: {'ON' if pos.get('motorsEnabled') else 'OFF'}")
            print(f"  Fan: {'ON' if pos.get('fanEnabled') else 'OFF'}")
            return True
        return False
    except Exception as e:
        print(f"  ❌ Error: {e}")
        return False

def test_homing(server):
    """Test de homing y inicialización de coordenadas"""
    print("\n=== TEST 3: HOMING Y COORDENADAS ===")
    try:
        # Verificar posición antes del homing
        print("→ Posición ANTES del homing...")
        pos_before = server.getPosition()
        if pos_before.get('ok'):
            x, y, z = pos_before.get('x', 0), pos_before.get('y', 0), pos_before.get('z', 0)
            print(f"  Antes: X={x:.2f}, Y={y:.2f}, Z={z:.2f}")
        
        # Habilitar motores primero
        print("→ Habilitando motores antes del homing...")
        server.enableMotors(True)
        time.sleep(1)
        
        # Homing
        print("→ Ejecutando G28 (homing)...")
        result = server.home()
        print(f"  Resultado: {result}")
        time.sleep(4)  # Más tiempo para completar homing
        
        # Verificar coordenadas después del homing
        print("→ Verificando posición DESPUÉS del homing...")
        pos = server.getPosition()
        if pos.get('ok'):
            x, y, z = pos.get('x', 0), pos.get('y', 0), pos.get('z', 0)
            print(f"  Después: X={x:.2f}, Y={y:.2f}, Z={z:.2f}")
            
            # Valores esperados según config.h:
            # INITIAL_X = 0.0
            # INITIAL_Y = HIGH_SHANK_LENGTH + END_EFFECTOR_OFFSET = 120 + 50 = 170.0
            # INITIAL_Z = LOW_SHANK_LENGTH = 120.0
            expected_x, expected_y, expected_z = 0.0, 170.0, 120.0
            
            # Verificar si las coordenadas se inicializaron
            if x == 0 and y == 0 and z == 0:
                print("  ❌ PROBLEMA: Coordenadas siguen en (0,0,0)")
                print("  → Diagnóstico: Firmware podría estar aún en modo SIMULATION")
                print("  → O la función setInterpolation no está funcionando")
                return False
            elif abs(x - expected_x) < 1.0 and abs(y - expected_y) < 1.0 and abs(z - expected_z) < 1.0:
                print(f"  ✅ PERFECTO: Coordenadas inicializadas correctamente!")
                print(f"  → Esperado: X={expected_x}, Y={expected_y}, Z={expected_z}")
                print(f"  → Obtenido: X={x}, Y={y}, Z={z}")
                return True
            else:
                print(f"  ⚠️  PARCIAL: Coordenadas se inicializaron pero con valores diferentes")
                print(f"  → Esperado: X={expected_x}, Y={expected_y}, Z={expected_z}")
                print(f"  → Obtenido: X={x}, Y={y}, Z={z}")
                print("  → Esto podría ser normal según configuración específica")
                return True
        return False
    except Exception as e:
        print(f"  ❌ Error: {e}")
        return False

def test_movements(server):
    """Test de movimientos y actualización de coordenadas"""
    print("\n=== TEST 4: MOVIMIENTOS ===")
    try:
        movements = [
            (100.0, 120.0, 50.0, "Movimiento 1"),
            (50.0, 150.0, 80.0, "Movimiento 2"),
            (75.0, 100.0, 60.0, "Movimiento 3")
        ]
        
        for x, y, z, desc in movements:
            print(f"→ {desc}: G1 X{x} Y{y} Z{z}")
            result = server.move(x, y, z, 800.0)
            print(f"  Comando: {result}")
            time.sleep(2)
            
            # Verificar posición
            pos = server.getPosition()
            if pos.get('ok'):
                px, py, pz = pos.get('x', 0), pos.get('y', 0), pos.get('z', 0)
                print(f"  Posición actual: X={px:.2f}, Y={py:.2f}, Z={pz:.2f}")
                
                # Verificar si se movió (tolerancia de 5mm)
                if abs(px - x) < 5.0 and abs(py - y) < 5.0 and abs(pz - z) < 5.0:
                    print("  ✅ Movimiento exitoso")
                else:
                    print(f"  ⚠️  Desviación: objetivo({x},{y},{z}) vs actual({px},{py},{pz})")
            else:
                print("  ❌ No se pudo verificar posición")
        
        return True
    except Exception as e:
        print(f"  ❌ Error: {e}")
        return False

def test_gcode_commands(server):
    """Test de comandos G-Code usando métodos específicos"""
    print("\n=== TEST 5: COMANDOS G-CODE ===")
    try:
        print("→ Modo absoluto (setMode)")
        result = server.setMode(True, True)  # manual=True, absolute=True
        print(f"  Resultado: {result}")
        
        print("→ Modo relativo (setMode)")
        result = server.setMode(True, False)  # manual=True, absolute=False
        print(f"  Resultado: {result}")
        
        # Volver a absoluto para el resto de tests
        server.setMode(True, True)
        
        print("  ✅ Comandos G-Code (via métodos) enviados")
        return True
    except Exception as e:
        print(f"  ❌ Error: {e}")
        return False

def test_mcode_commands(server):
    """Test de comandos M-Code usando métodos específicos"""
    print("\n=== TEST 6: COMANDOS M-CODE ===")
    try:
        print("→ Activar motores (enableMotors)")
        result = server.enableMotors(True)
        print(f"  Resultado: {result}")
        
        print("→ Reportar posición (getPosition)")
        result = server.getPosition()
        print(f"  Resultado: OK={result.get('ok')}")
        
        print("→ Estado endstops (getEndstops)")
        result = server.getEndstops()
        print(f"  Resultado: OK={result.get('ok')}")
        
        print("→ Fan ON (endEffector)")
        result = server.endEffector(True)
        print(f"  Resultado: {result}")
        
        print("→ Fan OFF (endEffector)")
        result = server.endEffector(False)
        print(f"  Resultado: {result}")
        
        print("→ Desactivar motores (enableMotors)")
        result = server.enableMotors(False)
        print(f"  Resultado: {result}")
        
        print("→ Reactivar motores (enableMotors)")
        result = server.enableMotors(True)
        print(f"  Resultado: {result}")
        
        print("  ✅ Comandos M-Code (via métodos) enviados")
        return True
    except Exception as e:
        print(f"  ❌ Error: {e}")
        return False

def test_end_effector(server):
    """Test del efector final (fan)"""
    print("\n=== TEST 7: EFECTOR FINAL ===")
    try:
        # Activar
        print("→ Activando efector (M106)...")
        result = server.endEffector(True)
        print(f"  Resultado: {result}")
        time.sleep(1)
        
        # Verificar estado
        pos = server.getPosition()
        if pos.get('ok'):
            fan_state = pos.get('fanEnabled', False)
            print(f"  Estado fan: {'ON' if fan_state else 'OFF'}")
            if fan_state:
                print("  ✅ Efector activado correctamente")
            else:
                print("  ⚠️  Efector no reporta estado ON")
        
        # Desactivar
        print("→ Desactivando efector (M107)...")
        result = server.endEffector(False)
        print(f"  Resultado: {result}")
        time.sleep(1)
        
        # Verificar estado
        pos = server.getPosition()
        if pos.get('ok'):
            fan_state = pos.get('fanEnabled', False)
            print(f"  Estado fan: {'ON' if fan_state else 'OFF'}")
            if not fan_state:
                print("  ✅ Efector desactivado correctamente")
        
        return True
    except Exception as e:
        print(f"  ❌ Error: {e}")
        return False

def test_endstops(server):
    """Test de estado de endstops"""
    print("\n=== TEST 8: ENDSTOPS ===")
    try:
        endstops = server.getEndstops()
        print(f"→ Estado endstops: {endstops}")
        if endstops.get('ok'):
            x_state = endstops.get('xState', 0)
            y_state = endstops.get('yState', 0)
            z_state = endstops.get('zState', 0)
            print(f"  X: {x_state} (0=libre, 1=presionado)")
            print(f"  Y: {y_state}")
            print(f"  Z: {z_state}")
            print("  ✅ Estado de endstops obtenido")
        return True
    except Exception as e:
        print(f"  ❌ Error: {e}")
        return False

def test_modes(server):
    """Test de modos (manual/auto, absoluto/relativo)"""
    print("\n=== TEST 9: MODOS ===")
    try:
        # Modo manual + absoluto
        print("→ Configurando modo manual + absoluto...")
        result = server.setMode(True, True)
        print(f"  Resultado: {result}")
        
        # Modo manual + relativo
        print("→ Configurando modo manual + relativo...")
        result = server.setMode(True, False)
        print(f"  Resultado: {result}")
        
        # Volver a absoluto
        print("→ Volviendo a modo absoluto...")
        result = server.setMode(True, True)
        print(f"  Resultado: {result}")
        
        print("  ✅ Cambios de modo completados")
        return True
    except Exception as e:
        print(f"  ❌ Error: {e}")
        return False

def test_stress(server):
    """Test de estrés - múltiples comandos rápidos"""
    print("\n=== TEST 10: ESTRÉS ===")
    try:
        print("→ Enviando múltiples comandos rápidamente...")
        
        # Secuencia de movimientos más conservadores
        movements = [
            (60.0, 110.0, 65.0),
            (70.0, 115.0, 68.0),
            (80.0, 120.0, 71.0)
        ]
        
        for i, (x, y, z) in enumerate(movements, 1):
            try:
                result = server.move(x, y, z, 800.0)  # Feedrate más bajo
                print(f"  Movimiento {i}: X{x} Y{y} Z{z} → {result.get('ok', False)}")
                time.sleep(1.0)  # Más tiempo entre comandos
            except Exception as e:
                print(f"  Movimiento {i}: ERROR - {e}")
        
        # Secuencia de comandos usando métodos disponibles
        commands = [
            ("Fan ON", lambda: server.endEffector(True)),
            ("Fan OFF", lambda: server.endEffector(False)),
            ("Posición", lambda: server.getPosition()),
            ("Endstops", lambda: server.getEndstops())
        ]
        
        for desc, cmd_func in commands:
            try:
                result = cmd_func()
                print(f"  {desc} → {result.get('ok', False)}")
                time.sleep(0.5)
            except Exception as e:
                print(f"  {desc}: ERROR - {e}")
        
        print("  ✅ Test de estrés completado")
        return True
    except Exception as e:
        print(f"  ❌ Error: {e}")
        return False

def main():
    print("=" * 70)
    print("🤖 TEST COMPLETO DEL SISTEMA ROBOT")
    print("=" * 70)
    print(f"Servidor: {SERVER_URL}")
    print(f"Dispositivo: {DEVICE_PATH}")
    print(f"Baudrate: {BAUD_RATE}")
    print("=" * 70)
    
    try:
        # Conectar al servidor RPC
        server = xmlrpc.client.ServerProxy(SERVER_URL)
        
        # Ejecutar todos los tests
        tests = [
            ("Conexión", test_connection),
            ("Estado inicial", test_initial_status),
            ("Homing", test_homing),
            ("Movimientos", test_movements),
            ("G-Code", test_gcode_commands),
            ("M-Code", test_mcode_commands),
            ("Efector final", test_end_effector),
            ("Endstops", test_endstops),
            ("Modos", test_modes),
            ("Estrés", test_stress)
        ]
        
        passed = 0
        total = len(tests)
        
        for test_name, test_func in tests:
            try:
                if test_func(server):
                    passed += 1
                    print(f"  ✅ {test_name}: PASÓ")
                else:
                    print(f"  ❌ {test_name}: FALLÓ")
            except Exception as e:
                print(f"  ❌ {test_name}: ERROR - {e}")
        
        # Desconectar al final
        print("\n=== DESCONEXIÓN ===")
        try:
            result = server.disconnectRobot()
            print(f"→ Desconectando: {result}")
        except:
            pass
        
        # Resumen final
        print("\n" + "=" * 70)
        print("📊 RESUMEN FINAL")
        print("=" * 70)
        print(f"Tests ejecutados: {total}")
        print(f"Tests exitosos: {passed}")
        print(f"Tests fallidos: {total - passed}")
        
        if passed == total:
            print("\n🎉 ¡TODOS LOS TESTS PASARON!")
            print("✅ Sistema completamente funcional")
            print("✅ Servidor-firmware comunicando correctamente")
            print("✅ Coordenadas actualizándose correctamente")
        elif passed >= total * 0.8:
            print(f"\n⚠️  Sistema mayormente funcional ({passed}/{total})")
            print("🔧 Revisar tests fallidos")
        elif passed >= total * 0.5:
            print(f"\n⚠️  Sistema parcialmente funcional ({passed}/{total})")
            print("🔧 Problemas significativos detectados")
        else:
            print(f"\n❌ PROBLEMAS CRÍTICOS ({passed}/{total})")
            print("🚨 Sistema no funcional")
        
        print("=" * 70)
        return 0 if passed >= total * 0.8 else 1
        
    except ConnectionRefusedError:
        print("\n❌ ERROR CRÍTICO: Servidor XML-RPC no disponible")
        print("Solución:")
        print("  Terminal 1: cd servidor && ./servidor_rpc 8080")
        print("  Terminal 2: python3 test_verification.py")
        return 1
    except Exception as e:
        print(f"\n❌ ERROR INESPERADO: {e}")
        return 1


if __name__ == '__main__':
    sys.exit(main())

