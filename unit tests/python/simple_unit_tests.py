class Coordenada:
    def __init__(self, x, y, z):
        self.x = x
        self.y = y
        self.z = z

class SistemaLuminoso:
    def __init__(self, estado, frecuencia, color):
        self.estado = estado
        self.frecuencia = frecuencia
        self.color = color

class PuntoDeTrayectoria:
    def __init__(self, coordenada, tiempo, sistema_luminoso):
        self.coordenada = coordenada
        self.tiempo = tiempo
        self.sistema_luminoso = sistema_luminoso

class Trayectoria:
    def __init__(self):
        self.puntos = []
        self.MAX_PUNTOS = 10

    def agregar_punto(self, punto):
        if len(self.puntos) < self.MAX_PUNTOS:
            self.puntos.append(punto)
            return True
        return False

    def mostrar_datos(self):
        print("X\tY\tZ\tTiempo\tEstado\tFrecuencia\tColor")
        for punto in self.puntos:
            estado = "Activo" if punto.sistema_luminoso.estado else "Inactivo"
            print(f"{punto.coordenada.x}\t{punto.coordenada.y}\t{punto.coordenada.z}\t"
                  f"{punto.tiempo}\t{estado}\t{punto.sistema_luminoso.frecuencia}\t"
                  f"{punto.sistema_luminoso.color}")

    def obtener_cantidad_puntos(self):
        return len(self.puntos)

# Pruebas unitarias
def pruebas_unitarias():
    trayectoria = Trayectoria()

    c1 = Coordenada(10.5, 20.3, 30.1)
    s1 = SistemaLuminoso(True, 50, 'R')
    p1 = PuntoDeTrayectoria(c1, 100, s1)

    assert trayectoria.agregar_punto(p1) is True
    assert trayectoria.obtener_cantidad_puntos() == 1

    c2 = Coordenada(15.0, 25.4, 35.2)
    s2 = SistemaLuminoso(False, 60, 'G')
    p2 = PuntoDeTrayectoria(c2, 150, s2)

    assert trayectoria.agregar_punto(p2) is True
    assert trayectoria.obtener_cantidad_puntos() == 2

    # Intento de agregar más puntos hasta superar el límite
    for _ in range(8):
        assert trayectoria.agregar_punto(p2) is True
    assert trayectoria.agregar_punto(p2) is False  # Debería fallar al superar el límite

if __name__ == "__main__":
    pruebas_unitarias()
    print("Todas las pruebas pasaron exitosamente!")
