#pragma once
#include <chrono>
struct Job {
    int id;
    int prioridad;
    std::chrono::time_point<std::chrono::steady_clock> timestamp_creacion;

   
};


void inicializarSistema();

void productor(int idProductor, int cantidadTareas);
void consumidor(int idConsumidor, int cantidadTareas);
int obtenerTrabajosFinalizados();
void ejecutarPrueba(int numProductores, int numConsumidores, int tareasPorProductor);
void productorEspecial(int idProductor, int cantidadTareas);
void ejecutarPruebaEspecial(int numProductores, int numConsumidores, int tareasPorProductor);

void imprimirRechazados();