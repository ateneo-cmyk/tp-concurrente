#include <iostream>
#include <mutex>
#include <condition_variable>
#include <string>
#include <chrono>
#include <ctime>
#include <fstream>
#include <queue>
//STRUCT PARA SEMAFORO Y JOBS
int contador = 0;
struct Semaforo {
    int contador;
    std::mutex mtx;
    std::condition_variable cv;
};

struct Job {
    int id; 
    int prioridad; // 1 = premium 0 = free
	std::chrono::steady_clock::time_point llegada;  // tiempo de llegada del Job
};
//VARIABLES


 std::queue<Job> premiumQueue; // Cola para premium.
 std::queue<Job> freeQueue; // Cola para free.
Semaforo hay_espacio;
 Semaforo hay_datos;
Semaforo vram;
std::mutex mtx_buffer;
std::mutex mtx_log;
std::mutex mtx_asignacionVRAM;
std::mutex mtx_finalizadas;
std::mutex mtx_jobId;
int tareasFinalizadas = 0;
int jobId = 0;
std::ofstream archivoLog("sistema.log");
int NUM_GATEWAYS = 3;
int NUM_WORKERS = 3;

void init(Semaforo& s, int n) {
    s.contador = n;
}
void wait(Semaforo& s) {
    std::unique_lock<std::mutex> lock(s.mtx);

    while (s.contador == 0) {
        s.cv.wait(lock);  // bloquea el hilo
    }

    s.contador--;  // consume un permiso
}
void signal(Semaforo& s) {
    std::unique_lock<std::mutex> lock(s.mtx);

    s.contador++;        // libera un permiso
    s.cv.notify_one();   // despierta UN hilo en espera
}



void workerNode(); 

void logEvento(const Job& tarea, const std::string& evento) {
    std::unique_lock<std::mutex> lock(mtx_log);

    std::time_t ahora = std::time(nullptr);
    std::tm* tiempo = std::localtime(&ahora);

    char hora[20];
    std::strftime(hora, sizeof(hora), "%H:%M:%S", tiempo);

    std::cout << "[" << hora << "] - " << "Job " << tarea.id << " - Prioridad " << tarea.prioridad << " - " << evento << std::endl;
    archivoLog << "[" << hora << "] - " << "Job " << tarea.id << " - Prioridad " << tarea.prioridad << " - " << evento << std::endl;
}
void apiGateway() {
    for (int i = 0; i < tareasGateway; i++) {
        wait(hay_espacio); // Espera a que haya un hueco libre
        mtx_buffer.lock(); // Mutex de la cola

      
        Job tarea;

       
        mtx_jobId.lock();
        tarea.id = jobId++;
        mtx_jobId.unlock();

        tarea.prioridad = rand() % 2; //random numeros
        
        tarea.llegada = std::chrono::steady_clock::now();
        logEvento(tarea, "CREADO");

        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Retardo antes de encolar.

        // If de prioridad que decide donde encolar dependiendo prioridad.
        if (tarea.prioridad == 1) {
            premiumQueue.push(tarea);
        }
        else {
            freeQueue.push(tarea);
        }

        logEvento(tarea, "EN_COLA");
        mtx_buffer.unlock();

        signal(hay_datos); // 3. Avisa que hay un nuevo dato disponible
    }
}

void workerNode() {
    int totalTareas = NUM_GATEWAYS * tareasGateway; 
    int tareasPorWorker = totalTareas / NUM_WORKERS; 

    for (int i = 0; i < tareasPorWorker; i++) {
        wait(hay_datos); // Espera a que haya al menos un dato
        mtx_buffer.lock(); // Mutex de la cola

        Job tarea; // Variable auxiliar a usar en el worker

        // Encolacion segun prioridad
        if (!freeQueue.empty() && agingActivado(freeQueue.front())) {
            // Si la cola de FREE no está vacía
            // Y además el primer job de esa cola ya superó el tiempo de espera (aging >= 5000ms)
            logEvento(freeQueue.front(), "AGING ACTIVADO");

            tarea = freeQueue.front(); // Se copia ese job a la variable local "tarea"
            freeQueue.pop(); // Se elimina de la cola porque ya va a ser procesado

        }
        else if (!premiumQueue.empty()) {
            // Si no se cumplió el caso de aging en FREE
            // pero hay jobs en la cola PREMIUM
            tarea = premiumQueue.front(); // Se toma el primer job premium (alta prioridad)
            premiumQueue.pop();

        }
        else {
            // Caso final: si no hay premium (o ya fueron consumidos).
            tarea = freeQueue.front(); // Se toma un job de la cola FREE sin importar aging
            freeQueue.pop();

        }

        mtx_buffer.unlock();
        signal(hay_espacio); // Avisa que libera un espacio

        // Mutex local (con llaves) para asignacion de Vram (Evitar asignaciones al mismo tiempo).
        {
            std::unique_lock<std::mutex> lock(mtx_asignacionVRAM);
            wait(vram);

            std::this_thread::sleep_for(std::chrono::milliseconds(450)); // Retardo de asignación a VRAM.
            logEvento(tarea, "VRAM_ASIGNADO");
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(600)); // Retardo durante renderizado.
        logEvento(tarea, "FINALIZADO");

        {
            // Mutex interno
            std::unique_lock<std::mutex> lock(mtx_finalizadas);
            tareasFinalizadas++;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(250)); // TIempo al liberar.
        signal(vram);
    }
}


    bool agingActivado(const Job & tarea){
        
        auto espera = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - tarea.llegada).count(); // tarea.llegada guarda el momento en que se encoló el job

        // Si el job lleva esperando 5000 ms (5 segundos) o más
        // se activa el aging -> el job de baja prioridad deja de ser ignorado
        return espera >= 5000;
    }

const int tareasGateway = 10;

// Recursos compartidos.
std::queue<Job> premiumQueue;
std::queue<Job> freeQueue;

// Buffer 1.
Semaforo hay_espacio;
Semaforo hay_datos;

// Buffer 2.
Semaforo vram;
int main(){

    init(hay_espacio, 50); // Cantidad de espacios.
    init(hay_datos, 0); // Inicializacion de espacios.
    init(vram, 5); // Pool de VRAM

    std::vector<std::thread> gateways; 
    std::vector<std::thread> workers; 

 
    for (int i = 0; i < NUM_GATEWAYS; ++i) {
        gateways.emplace_back(apiGateway);
    }

    // Workers
    for (int i = 0; i < NUM_WORKERS; ++i) {
        workers.emplace_back(workerNode);
    }

    for (auto& g : gateways)
    {
        g.join();
    }

    for (auto& w : workers)
    {
        w.join();
    }

    // Contador final
    std::cout << "\nTareas finalizadas: "
        << tareasFinalizadas
        << std::endl;

    return 0;;
}

