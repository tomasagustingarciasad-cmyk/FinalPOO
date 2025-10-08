#include <iostream>
#include <vector>
#include <cassert>
using namespace std;

// Clase Coordenada
class Coordenada
{
public:
    float x, y, z;

    Coordenada(float x, float y, float z) : x(x), y(y), z(z) {}
};

// Clase SistemaLuminoso
class SistemaLuminoso
{
public:
    bool estado;
    int frecuencia;
    char color;

    SistemaLuminoso(bool estado, int frecuencia, char color)
        : estado(estado), frecuencia(frecuencia), color(color)
    {
        if (color != 'R' && color != 'G' && color != 'B')
        {
            throw invalid_argument("Color invalido: debe ser 'R', 'G' o 'B'");
        }
    }
};

// Clase PuntoDeTrayectoria
class PuntoDeTrayectoria
{
public:
    Coordenada coordenada;
    int tiempo;
    SistemaLuminoso sistemaLuminoso;

    PuntoDeTrayectoria(Coordenada coord, int tiempo, SistemaLuminoso sistema)
        : coordenada(coord), tiempo(tiempo), sistemaLuminoso(sistema) {}
};

// Clase Trayectoria que agrega objetos de tipo PuntoDeTrayectoria
class Trayectoria
{
private:
    vector<PuntoDeTrayectoria> puntos;
    const int MAX_PUNTOS = 10;

public:
    bool agregarPunto(const PuntoDeTrayectoria &punto)
    {
        if (puntos.size() < MAX_PUNTOS)
        {
            puntos.push_back(punto);
            return true;
        }
        return false;
    }

    bool eliminarUltimoPunto()
    {
        if (!puntos.empty())
        {
            puntos.pop_back();
            return true;
        }
        return false;
    }

    int obtenerCantidadPuntos() const
    {
        return puntos.size();
    }
};

// Pruebas unitarias
void pruebasUnitarias()
{
    Trayectoria trayectoria;

    Coordenada c1(10.5, 20.3, 30.1);
    SistemaLuminoso s1(true, 50, 'R');
    PuntoDeTrayectoria p1(c1, 100, s1);

    assert(p1.sistemaLuminoso.estado == true);
    assert(p1.sistemaLuminoso.frecuencia == 50);
    assert(p1.sistemaLuminoso.color == 'R');

    assert(trayectoria.agregarPunto(p1) == true);
    assert(trayectoria.obtenerCantidadPuntos() == 1);
    assert(trayectoria.eliminarUltimoPunto() == true);  // Eliminar el punto agregado
    assert(trayectoria.obtenerCantidadPuntos() == 0);   // Verificar cantidad
    assert(trayectoria.eliminarUltimoPunto() == false); // No hay puntos para eliminar

    Coordenada c2(15.0, 25.4, 35.2);
    SistemaLuminoso s2(false, 60, 'G');
    PuntoDeTrayectoria p2(c2, 150, s2);

    // Intento de agregar puntos más allá del límite
    for (int i = 0; i < 10; ++i)
    {
        cout << trayectoria.obtenerCantidadPuntos() << endl;
        assert(trayectoria.agregarPunto(p2) == true);
    }
    assert(trayectoria.agregarPunto(p2) == false);

    try
    {
        SistemaLuminoso s_invalido(true, 50, 'Z');
        assert(false);
    }
    catch (const invalid_argument &e)
    {
        assert(true);
    }

    // Verificar que la cantidad de puntos no exceda el límite
    assert(trayectoria.obtenerCantidadPuntos() <= 10);
}

int main()
{
    pruebasUnitarias();
    cout << "Todas las pruebas pasaron exitosamente!" << endl;

    return 0;
}
