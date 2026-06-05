#ifndef SISTEMA_RENDER_H_INCLUDED
#define SISTEMA_RENDER_H_INCLUDED

#include <chrono>

///estructura del Job
struct Job {
    int id;
    int prioridad; // 1: Premium, 0: Free
    std::chrono::time_point<std::chrono::steady_clock> timestamp_creacion;

    ///declaramos el operador para la priority_queue
    bool operator<(const Job& otro) const;
};

///interfaz del sistema
void inicializarSistema();

void productor(int idProductor, int cantidadTareas);
void consumidor(int idConsumidor, int cantidadTareas);
int obtenerTrabajosFinalizados();
void ejecutarPrueba(int numProductores, int numConsumidores, int tareasPorProductor);
#endif // SISTEMA_RENDER_H_INCLUDED
