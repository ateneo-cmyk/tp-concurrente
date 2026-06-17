#include "sistema_render.h"
#include "semaforo.h"
#include "logger.h"

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
/// RECURSOS COMPARTIDOS
int generador_id = 0;
std::mutex mtx_jobId;
/// Buffer 1
std::vector<Job> buffer_mensajes;
std::mutex mtx_cola;
Semaforo hay_tareas;
///buffer 2 pool de vram
Semaforo slots_vram;
///representación física de la memoria
///un valor de -1 indica que el slot está vacio
int memoria_vram[5] = { -1, -1, -1, -1, -1 };
/// Candado único para acceso a la VRAM (entrada y salida)
std::mutex mtx_vram;
int trabajosFinalizados = 0;
std::mutex mtx_contador;

/// INTERFAZ DEL SISTEMA

void inicializarSistema() {
    init(hay_tareas, 0);
    init(slots_vram, 5);
}
/// PRODUCTOR
void productor(int idProductor, int cantidadTareas) {
    for (int i = 0; i < cantidadTareas; ++i) {
        Job nuevoJob;
        {
            // asignación segura del ID global
            std::unique_lock<std::mutex> lockId(mtx_jobId);
            nuevoJob.id = generador_id++;
        }
        // prioridad aleatoria
        nuevoJob.prioridad = rand() % 2;
        nuevoJob.timestamp_creacion = std::chrono::steady_clock::now();
        // ---ingreso al buffer 1---
        {
            // usamos el mutex para que otros productores no escriban a la vez
            std::unique_lock<std::mutex> lockCola(mtx_cola);

            // insertamos el job al final
            buffer_mensajes.push_back(nuevoJob);

            // registro de evento
            logEvent(nuevoJob.id, nuevoJob.prioridad, "CREADO");
            logEvent(nuevoJob.id, nuevoJob.prioridad, "EN_COLA");
        } // se libera el lock

        // despertamos a un consumidor avisando que hay un nuevo job
        signal(hay_tareas);

        //retardo simulado de ingreso a la cola
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
/// FUNCIoN AUXILIAR DE ENVEJECIMIENTO
// esta funcion evalua si un job ha estado esperando demasiado tiempo
bool sufreInanicion(const Job& tarea) {
    auto ahora = std::chrono::steady_clock::now();
    // calculamos la diferencia de tiempo
    auto tiempoEspera = std::chrono::duration_cast<std::chrono::milliseconds>(ahora - tarea.timestamp_creacion).count();
    /// si esta esperando mas de 5000ms esta sufriendo inanicion
    return tiempoEspera >= 5000;
}

/// CONSUMIDOR (Filtro Procesador)

// representa un worker Node en nuestra granja de renderizado
// corre en un bucle infinito procesando los jobs
void consumidor(int idConsumidor, int cantidadTareas) {
    for (int iteracion = 0; iteracion < cantidadTareas; ++iteracion) {
        wait(hay_tareas);
        Job jobActual;

        ///EXTRACCIÓN DEL BUFFER 1
        {
            // tomamos el candado de la cola usamos unique_lock porque define un
            // ambito (scope) en el cual al cerrar la llave el candado se libera solo.
            std::unique_lock<std::mutex> lockCola(mtx_cola);
            int indiceSeleccionado = -1;

            /// Prevención de Inanición
            // recorremos todo el vector para ver si algun Job estancado
            for (size_t i = 0; i < buffer_mensajes.size(); ++i) {
                if (buffer_mensajes[i].prioridad == 0) {
                    if (sufreInanicion(buffer_mensajes[i])) {
                        indiceSeleccionado = i;
                        logEvent(buffer_mensajes[i].id, 0, "RESCATE_INANICION");
                        break;
                    }
                }
            }
            /// Si ningún free superó los 5000ms, despachamos por prioridad normal.
            /// Buscamos el primer Premium (1) en el vector
            if (indiceSeleccionado == -1) {
                for (size_t i = 0; i < buffer_mensajes.size(); ++i) {
                    if (buffer_mensajes[i].prioridad == 1) {
                        indiceSeleccionado = i;
                        break;
                    }
                }
            }
            /// Si no hubo rescates ni trabajos Premium, simplemente tomamos
            /// el primer trabajo de la lista (que será un comun).
            if (indiceSeleccionado == -1) {
                indiceSeleccionado = 0;
            }

            // copiamos el job a nuestra variable local
            jobActual = buffer_mensajes[indiceSeleccionado];
            buffer_mensajes.erase(buffer_mensajes.begin() + indiceSeleccionado);

        } /// se destruye el lock

        /// INGRESO AL BUFFER 2 (Pool de VRAM)

        //pedimos uno de los 5 "puestos" de la VRAM.
        //si ya hay 5 hilos renderizando, el 6to hilo se duerme en esta línea.
        wait(slots_vram);
        int mi_slot = -1;
        {
            // Tomamos el candado único de VRAM para buscar y ocupar un slot
            std::unique_lock<std::mutex> lockVram(mtx_vram);
            // buscamos cuál de los 5 espacios está libre (-1)
            for (int i = 0; i < 5; i++) {
                if (memoria_vram[i] == -1) {
                    memoria_vram[i] = jobActual.id; // Ocupamos la memoria
                    mi_slot = i;
                    break;
                }
            }
            logEvent(jobActual.id, jobActual.prioridad, "ASIGNADO_VRAM");
            // Retardo de 450ms.
            std::this_thread::sleep_for(std::chrono::milliseconds(450));
        } //Se libera mtx_vram
        /// ZONA NO CRÍTICA: PROCESAMIENTO GPU
        //NO hay candados tomados (solo tenemos el  semaforo).
        //nos da  hasta 5 hilos que pueden estar ejecutando
        std::this_thread::sleep_for(std::chrono::milliseconds(600));
        ///SALIDA DE VRAM
        {
            /// Tomamos el candado único de VRAM para liberar el slot
            std::unique_lock<std::mutex> lockVram(mtx_vram);
            if (mi_slot >= 0 && mi_slot < 5) {
                memoria_vram[mi_slot] = -1;
                logEvent(jobActual.id, jobActual.prioridad, "FINALIZADO");
                // Retardo de 250ms. solo un hilo a la vez puede estar liberando
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
            }
        } //se libera mtx_vram.
        ///devolvemos el "ticket" de la VRAM al semáforo general,
        /// despertando a algún hilo que estuviera esperando en wait(slots_vram).
        signal(slots_vram);
        /// ACTUALIZACIÓN DEL CONTADOR GLOBAL

        {
            std::unique_lock<std::mutex> lockContador(mtx_contador);
            trabajosFinalizados++;
        }//Se libera mtx_contador.
    }
}
void ejecutarPrueba(int numProductores, int numConsumidores, int tareasPorProductor) {
    std::vector<std::thread> productores;
    std::vector<std::thread> consumidores;
    // Calculamos cuánto le toca a cada consumidor
    int totalTareas = numProductores * tareasPorProductor;
    int tareasPorConsumidor = totalTareas > 0 ? (totalTareas / numConsumidores) : 0;
    //lanzar hilos productores
    for (int i = 0; i < numProductores; ++i) {
        productores.emplace_back(productor, i + 1, tareasPorProductor);
    }
    //lanzar hilos consumidores
    for (int i = 0; i < numConsumidores; ++i) {
        consumidores.emplace_back(consumidor, i + 1, tareasPorConsumidor);
    }
    //esperar a que todos terminen su ciclo
    for (auto& p : productores) p.join();
    for (auto& c : consumidores) c.join();
}

int obtenerTrabajosFinalizados() {
    std::unique_lock<std::mutex> lock(mtx_contador);
    return trabajosFinalizados;
}