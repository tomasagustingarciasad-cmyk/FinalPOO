import unittest

from ex3 import Coordenada, SistemaLuminoso, PuntoDeTrayectoria, Trayectoria

# Pruebas unitarias usando unittest

class TestTrayectoria(unittest.TestCase):

    def test_agregar_punto_y_verificar_atributos(self):
        trayectoria = Trayectoria()
        c1 = Coordenada(10.5, 20.3, 30.1)
        s1 = SistemaLuminoso(True, 50, 'R')
        p1 = PuntoDeTrayectoria(c1, 100, s1)

        self.assertTrue(trayectoria.agregar_punto(p1))
        self.assertEqual(trayectoria.obtener_cantidad_puntos(), 1)
    
        # Verificar atributos del punto agregado
        punto_obtenido = trayectoria.obtener_punto(0)
        self.assertEqual(punto_obtenido.coordenada.x, 10.5)
        self.assertEqual(punto_obtenido.tiempo, 100)
        self.assertEqual(punto_obtenido.sistema_luminoso.estado, True)
        self.assertEqual(punto_obtenido.sistema_luminoso.frecuencia, 50)
        self.assertEqual(punto_obtenido.sistema_luminoso.color, 'R')

    def test_agregar_multiples_puntos_y_verificar_limite(self):
        trayectoria = Trayectoria()

        c2 = Coordenada(15.0, 25.4, 35.2)
        s2 = SistemaLuminoso(False, 60, 'G')
        p2 = PuntoDeTrayectoria(c2, 150, s2)

        for _ in range(10):
            self.assertTrue(trayectoria.agregar_punto(p2))
        self.assertEqual(trayectoria.obtener_cantidad_puntos(), 10)

        # Intentar agregar un punto adicional y verificar que falla
        self.assertFalse(trayectoria.agregar_punto(p2))

    def test_verificar_manejo_correcto_tipos_de_datos(self):
        trayectoria = Trayectoria()

        c3 = Coordenada(0.0, -1.5, 3.14)
        s3 = SistemaLuminoso(True, 100, 'B')
        p3 = PuntoDeTrayectoria(c3, -50, s3)

        self.assertTrue(trayectoria.agregar_punto(p3))
        punto_obtenido = trayectoria.obtener_punto(0)
        self.assertAlmostEqual(punto_obtenido.coordenada.z, 3.14)
        self.assertEqual(punto_obtenido.sistema_luminoso.estado, True)
        self.assertEqual(punto_obtenido.sistema_luminoso.frecuencia, 50)
        self.assertEqual(punto_obtenido.sistema_luminoso.color, 'B')

if __name__ == "__main__":
    unittest.main()
