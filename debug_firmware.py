#!/usr/bin/env python3
import serial
import time
import sys

def monitor_position_continuously(ser, times=5):
    """Función para monitorear posición múltiples veces"""
    print(f"\n=== MONITOREO CONTINUO DE POSICIÓN ({times} veces) ===")
    
    for i in range(times):
        print(f"\nConsulta #{i+1}:")
        ser.write(b'M114\r')
        ser.flush()
        
        time.sleep(1)
        while ser.in_waiting > 0:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line:
                if 'CURRENT POSITION' in line:
                    print(f"  POSICIÓN: {line}")
                elif 'MOTORS' in line or 'FAN' in line:
                    print(f"  ESTADO: {line}")
                elif 'OK' in line:
                    print(f"  {line}")
                    break
        
        time.sleep(2)  # Pausa entre consultas

def debug_firmware_communication():
    try:
        print("=== DEBUG COMPLETO: Comunicación con Firmware Arduino ===")
        print("Puerto: /dev/ttyUSB0, Baudios: 115200")
        
        ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=5)
        time.sleep(2)
        
        print("\n1. Enviando línea vacía para despertar firmware...")
        ser.write(b'\n')
        ser.flush()
        time.sleep(0.5)
        
        # Leer cualquier banner inicial
        while ser.in_waiting > 0:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line:
                print(f"BANNER: '{line}'")
        
        print("\n2. Probando comando M114...")
        ser.write(b'M114\r')  # ¡Cambio \n por \r!
        ser.flush()
        
        print("Esperando respuesta...")
        start_time = time.time()
        lines_received = []
        
        while time.time() - start_time < 10:  # 10 segundos máximo
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    lines_received.append(line)
                    print(f"RECV: '{line}'")
                    if 'OK' in line:
                        break
            time.sleep(0.1)
        
        print(f"\n3. Total de líneas recibidas: {len(lines_received)}")
        
        # Probar comandos más completos
        commands = [
            ('G90', 'Modo absoluto', 2),
            ('M17', 'Habilitar motores', 2), 
            ('G28', 'Calibración/Homing', 8),  # Más tiempo para homing
            ('M114', 'Consultar posición después de homing', 3),
            ('G0 X10 Y10 Z10', 'Mover a posición 10,10,10', 5),
            ('M114', 'Consultar posición después de move', 3),
            ('G0 X0 Y0 Z0', 'Mover a origen', 5),
            ('M114', 'Consultar posición final', 3)
        ]
        
        for cmd, description, wait_time in commands:
            print(f"\n4. {description}: {cmd}")
            ser.write(f"{cmd}\r".encode())
            ser.flush()
            
            print(f"   Esperando {wait_time} segundos...")
            time.sleep(wait_time)
            
            response_lines = []
            while ser.in_waiting > 0:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    response_lines.append(line)
                    print(f"  RESP: '{line}'")
            
            if not response_lines:
                print("  (Sin respuesta)")
            
            # Pausa entre comandos
            time.sleep(0.5)
        
        # Monitoreo continuo de posición al final
        monitor_position_continuously(ser, 3)
        
        ser.close()
        
        print("\n=== ANÁLISIS COMPLETO ===")
        print("COMANDOS PROBADOS:")
        print("✓ G90 - Modo absoluto")  
        print("✓ M17 - Habilitar motores")
        print("✓ G28 - Calibración/Homing")
        print("✓ M114 - Consultar posición (múltiples veces)")
        print("✓ G0 - Comandos de movimiento")
        
        print("\nRESUMEN DE RESPUESTA M114:")
        if not lines_received:
            print("❌ El firmware no responde al primer M114")
        elif any('CURRENT POSITION' in line for line in lines_received):
            print("✅ El firmware envía línea CURRENT POSITION correctamente")
        else:
            print("⚠️  El firmware responde pero la línea CURRENT POSITION está vacía")
            print("   Líneas del primer M114:")
            for line in lines_received:
                print(f"   - '{line}'")
                
        print("\nPARA OBTENER LA POSICIÓN:")
        print("1. Envía: M114\\r")
        print("2. Espera respuesta con formato:")
        print("   INFO: ABSOLUTE MODE")
        print("   INFO: CURRENT POSITION: [X:### Y:### Z:### E:###]") 
        print("   INFO: MOTORS ENABLED/DISABLED")
        print("   INFO: FAN ENABLED/DISABLED")
        print("   OK")
        
    except Exception as e:
        print(f"ERROR: {e}")

if __name__ == "__main__":
    debug_firmware_communication()