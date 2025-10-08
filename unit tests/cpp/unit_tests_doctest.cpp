#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <vector>
#include <iostream>
using namespace std;

// Clases: Coordenada, SistemaLuminoso, PuntoDeTrayectoria, Trayectoria (sin cambios)
class Coordenada
{
public:
    float x, y, z;

    Coordenada(float x, float y, float z) : x(x), y(y), z(z) {}
};

class SistemaLuminoso
{
public:
    bool estado;
    int frecuencia;
    char color;

    SistemaLuminoso(bool estado, int frecuencia, char color)
        : estado(estado), frecuencia(frecuencia), color(color) {}
};

class PuntoDeTrayectoria
{
public:
    Coordenada coordenada;
    int tiempo;
    SistemaLuminoso sistemaLuminoso;

    PuntoDeTrayectoria(Coordenada coord, int tiempo, SistemaLuminoso sistema)
        : coordenada(coord), tiempo(tiempo), sistemaLuminoso(sistema) {}
};

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

    void mostrarDatos() const
    {
        cout << "X\tY\tZ\tTiempo\tEstado\tFrecuencia\tColor\n";
        for (const auto &punto : puntos)
        {
            cout << punto.coordenada.x << "\t"
                 << punto.coordenada.y << "\t"
                 << punto.coordenada.z << "\t"
                 << punto.tiempo << "\t"
                 << (punto.sistemaLuminoso.estado ? "Activo" : "Inactivo") << "\t"
                 << punto.sistemaLuminoso.frecuencia << "\t\t"
                 << punto.sistemaLuminoso.color << endl;
        }
    }

    PuntoDeTrayectoria obtenerPunto(int indice) const
    {
        if (indice >= 0 && indice < puntos.size())
        {
            return puntos[indice];
        }
        throw out_of_range("Índice fuera de rango");
    }
};

TEST_CASE("Pruebas de agregación y eliminación de puntos en Trayectoria")
{
    Trayectoria trayectoria;

    Coordenada c1(10.5, 20.3, 30.1);
    SistemaLuminoso s1(true, 50, 'R');
    PuntoDeTrayectoria p1(c1, 100, s1);

    SUBCASE("Agregar y eliminar un punto")
    {
        CHECK(trayectoria.agregarPunto(p1) == true);
        CHECK(trayectoria.obtenerCantidadPuntos() == 1);
        CHECK(trayectoria.eliminarUltimoPunto() == true);
        CHECK(trayectoria.obtenerCantidadPuntos() == 0);
        CHECK(trayectoria.eliminarUltimoPunto() == false); // No hay puntos para eliminar
    }
}

TEST_CASE("Pruebas del límite de puntos en Trayectoria")
{
    Trayectoria trayectoria;

    Coordenada c2(15.0, 25.4, 35.2);
    SistemaLuminoso s2(false, 60, 'G');
    PuntoDeTrayectoria p2(c2, 150, s2);

    SUBCASE("Agregar puntos hasta el límite")
    {
        for (int i = 0; i < 10; ++i)
        {
            CHECK(trayectoria.agregarPunto(p2) == true);
        }
        CHECK(trayectoria.agregarPunto(p2) == false);
    }
}

TEST_CASE("Verificación de atributos y manejo de datos en Trayectoria")
{
    Trayectoria trayectoria;

    Coordenada c3(0.0, -1.5, 3.14);
    SistemaLuminoso s3(true, 100, 'B');
    PuntoDeTrayectoria p3(c3, -50, s3);

    SUBCASE("Verificar atributos de punto agregado")
    {
        CHECK(trayectoria.agregarPunto(p3) == true);

        auto puntoObtenido = trayectoria.obtenerPunto(0);
        CHECK(puntoObtenido.coordenada.x == doctest::Approx(0.0f));
        CHECK(puntoObtenido.coordenada.y == doctest::Approx(-1.5f));
        CHECK(puntoObtenido.coordenada.z == doctest::Approx(3.14f));
        CHECK(puntoObtenido.tiempo == -50);
        CHECK(puntoObtenido.sistemaLuminoso.estado == true);
        CHECK(puntoObtenido.sistemaLuminoso.frecuencia == 100);
        CHECK(puntoObtenido.sistemaLuminoso.color == 'B');
    }
}

TEST_CASE("Verificación del manejo de excepciones en Trayectoria")
{
    Trayectoria trayectoria;

    SUBCASE("Acceso a índice fuera de rango")
    {
        CHECK_THROWS_AS(trayectoria.obtenerPunto(-1), out_of_range);
        CHECK_THROWS_AS(trayectoria.obtenerPunto(1), out_of_range); // Asumiendo que solo hay 0 puntos
    }
}