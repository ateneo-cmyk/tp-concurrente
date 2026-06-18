#pragma once
#include <chrono>
struct Job {
    int id;
    int prioridad; // 1: Premium, 0: Free
    std::chrono::time_point<std::chrono::steady_clock> timestamp_creacion;

    ///declaramos el operador para la priority_queue
    //bool operator<(const Job& otro) const;
};

///interfaz del sistema
void inicializarSistema();

void productor(int idProductor, int cantidadTareas);
void consumidor(int idConsumidor, int cantidadTareas);
int obtenerTrabajosFinalizados();
void ejecutarPrueba(int numProductores, int numConsumidores, int tareasPorProductor);
void productorEspecial(int idProductor, int cantidadTareas);
void ejecutarPruebaEspecial(int numProductores, int numConsumidores, int tareasPorProductor);