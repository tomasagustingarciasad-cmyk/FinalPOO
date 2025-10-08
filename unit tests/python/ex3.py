
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
        self.MAX_PUNTOS = 10  # Límite máximo de puntos permitidos

    def agregar_punto(self, punto):
        if len(self.puntos) < self.MAX_PUNTOS:
            self.puntos.append(punto)
            return True
        return False  # No se agrega si se supera el límite

    def obtener_cantidad_puntos(self):
        return len(self.puntos)

    def obtener_punto(self, indice):
        if 0 <= indice < len(self.puntos):
            return self.puntos[indice]
        raise IndexError("Índice fuera de rango")