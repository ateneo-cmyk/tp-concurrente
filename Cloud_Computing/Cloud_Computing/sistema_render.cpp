#include "sistema_render.h"
#include "semaforo.h"
#include "logger.h"

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>

int generador_id = 0;
std::mutex mtx_jobId;

std::vector<Job> buffer_mensajes;
std::mutex mtx_cola;
Semaforo hay_tareas;

Semaforo slots_vram;
int memoria_vram[5] = { -1, -1, -1, -1, -1 };

std::mutex mtx_vram;
int trabajosFinalizados = 0;
std::mutex mtx_contador;

static const int prioridadFija = 1; 
std::vector<Job> Trash_Queue;

void inicializarSistema() {
    init(hay_tareas, 0);
    init(slots_vram, 5);
}

void productor(int idProductor, int cantidadTareas) {
    for (int i = 0; i < cantidadTareas; ++i) {
        Job nuevoJob;
        {
           
            std::unique_lock<std::mutex> lockId(mtx_jobId);
            nuevoJob.id = generador_id++;
        }
      
        nuevoJob.prioridad = rand() % 2;
        nuevoJob.timestamp_creacion = std::chrono::steady_clock::now();
        
        {
            
            std::unique_lock<std::mutex> lockCola(mtx_cola);

            
            buffer_mensajes.push_back(nuevoJob);

          
            logEvent(nuevoJob.id, nuevoJob.prioridad, "CREADO");
            logEvent(nuevoJob.id, nuevoJob.prioridad, "EN_COLA");
        } 

        
        signal(hay_tareas);

       
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

}
void productorEspecial(int idProductor, int cantidadTareas) {
    for (int i = 0; i < cantidadTareas; ++i) {
        Job nuevoJob;
        {
           
            std::unique_lock<std::mutex> lockId(mtx_jobId);
            nuevoJob.id = generador_id++;
        }
		
        nuevoJob.prioridad = 1;
        nuevoJob.timestamp_creacion = std::chrono::steady_clock::now();
        
        {
            
            std::unique_lock<std::mutex> lockCola(mtx_cola);

           
            buffer_mensajes.push_back(nuevoJob);

       
            logEvent(nuevoJob.id, nuevoJob.prioridad, "CREADO");
            logEvent(nuevoJob.id, nuevoJob.prioridad, "EN_COLA");
        } 

       
        signal(hay_tareas);

        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }



}


bool sufreInanicion(const Job& tarea) {
    auto ahora = std::chrono::steady_clock::now();
    
    auto tiempoEspera = std::chrono::duration_cast<std::chrono::milliseconds>(ahora - tarea.timestamp_creacion).count();
    
    return tiempoEspera >= 5000;
}


void consumidor(int idConsumidor, int cantidadTareas) {
    for (int iteracion = 0; iteracion < cantidadTareas; ++iteracion) {
        wait(hay_tareas);
        Job jobActual;

       
        {
            
            std::unique_lock<std::mutex> lockCola(mtx_cola);
            int indiceSeleccionado = -1;
			bool estoyRechazado = false;
            
            for (size_t i = 0; i < buffer_mensajes.size(); ++i) {
                if (buffer_mensajes[i].prioridad == 0) {
                    if (sufreInanicion(buffer_mensajes[i])) {
                        auto ahora = std::chrono::steady_clock::now();
                        auto tiempoEspera = std::chrono::duration_cast<std::chrono::milliseconds>(ahora - buffer_mensajes[i].timestamp_creacion).count();
                        if (tiempoEspera >= 10000) {
                            
                            logEvent(buffer_mensajes[i].id, 0, "DESECHADO_TIMEOUT");

                            
                            Trash_Queue.push_back(buffer_mensajes[i]);

                       
                            buffer_mensajes.erase(buffer_mensajes.begin() + i);
                            estoyRechazado = true;
                            break;
                        }
                    }
                }
            }
            if (estoyRechazado) {
            
                continue;
            }
            if (indiceSeleccionado == -1) {
                for (size_t i = 0; i < buffer_mensajes.size(); ++i) {
                    if (buffer_mensajes[i].prioridad == 1) {
                        indiceSeleccionado = i;
                        break;
                    }
                }
            }
            
            if (indiceSeleccionado == -1) {
                indiceSeleccionado = 0;
            }

           
            jobActual = buffer_mensajes[indiceSeleccionado];
            buffer_mensajes.erase(buffer_mensajes.begin() + indiceSeleccionado);

        } 
        wait(slots_vram);
        int mi_slot = -1;
        {
           
            std::unique_lock<std::mutex> lockVram(mtx_vram);
            
            for (int i = 0; i < 5; i++) {
                if (memoria_vram[i] == -1) {
                    memoria_vram[i] = jobActual.id; 
                    mi_slot = i;
                    break;
                }
            }
            logEvent(jobActual.id, jobActual.prioridad, "ASIGNADO_VRAM");
         
            std::this_thread::sleep_for(std::chrono::milliseconds(450));
        } 
        
        std::this_thread::sleep_for(std::chrono::milliseconds(600));
       
        {
            
            std::unique_lock<std::mutex> lockVram(mtx_vram);
            if (mi_slot >= 0 && mi_slot < 5) {
                memoria_vram[mi_slot] = -1;
                logEvent(jobActual.id, jobActual.prioridad, "FINALIZADO");
               
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
            }
        } 
       
        signal(slots_vram);
      

        {
            std::unique_lock<std::mutex> lockContador(mtx_contador);
            trabajosFinalizados++;
        }
    }
}

static std::vector<int> distribuirTareas(int totalTareas, int numConsumidores) {
    std::vector<int> asignaciones;
    if (numConsumidores <= 0) return asignaciones;
    asignaciones.assign(numConsumidores, 0);
    int base = totalTareas / numConsumidores;
    int rem = totalTareas % numConsumidores;
    for (int i = 0; i < numConsumidores; ++i) {
        asignaciones[i] = base + (i < rem ? 1 : 0);
    }
    return asignaciones;
}

void ejecutarPrueba(int numProductores, int numConsumidores, int tareasPorProductor) {
    std::vector<std::thread> productores;
    std::vector<std::thread> consumidores;

    int totalTareas = numProductores * tareasPorProductor;
    auto asignaciones = distribuirTareas(totalTareas, numConsumidores);

  
    for (int i = 0; i < numProductores; ++i) {
        productores.emplace_back(productor, i + 1, tareasPorProductor);
    }
    
    for (int i = 0; i < numConsumidores; ++i) {
        int asignadas = (i < (int)asignaciones.size()) ? asignaciones[i] : 0;
        consumidores.emplace_back(consumidor, i + 1, asignadas);
    }
    
    for (auto& p : productores) p.join();
    for (auto& c : consumidores) c.join();
}

void ejecutarPruebaEspecial(int numProductores, int numConsumidores, int tareasPorProductor) {
    std::vector<std::thread> productores;
    std::vector<std::thread> consumidores;

    int totalTareas = numProductores * tareasPorProductor;
    auto asignaciones = distribuirTareas(totalTareas, numConsumidores);


    for (int i = 0; i < numProductores; ++i) {
        productores.emplace_back(productorEspecial, i + 1, tareasPorProductor);
    }
    
    for (int i = 0; i < numConsumidores; ++i) {
        int asignadas = (i < (int)asignaciones.size()) ? asignaciones[i] : 0;
        consumidores.emplace_back(consumidor, i + 1, asignadas);
    }

    for (auto& p : productores) p.join();
    for (auto& c : consumidores) c.join();
}
void imprimirRechazados() {
	std::unique_lock<std::mutex> lock(mtx_cola);
	if (Trash_Queue.empty()) {
		std::cout << "No hay trabajos rechazados.\n";
		return;
	}
	for (const auto& job : Trash_Queue) {
		std::cout << "Job ID: " << job.id << ", Prioridad: " << job.prioridad << "\n";
	}
}

int obtenerTrabajosFinalizados() {
    std::unique_lock<std::mutex> lock(mtx_contador);
    return trabajosFinalizados;
}
