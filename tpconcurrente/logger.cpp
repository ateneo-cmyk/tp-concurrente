#include "logger.h"

#include <fstream>
#include <mutex>
#include <chrono>
#include <thread>
#include <iostream>


///exclusion mutua sobre el archivo sistema.log


std::mutex mtx_log;


///FUNCION DE LOGGING
///Sincroniza el acceso al archivo
///Genera un timestamp
///Guarda el evento del Job




void logEvent(int jobId, int prioridad, const char* evento) {
    std::unique_lock<std::mutex> lock(mtx_log);

    //formato de hora
    std::time_t ahora = std::time(nullptr);
    std::tm* tiempo = std::localtime(&ahora);
    char hora[20];
    std::strftime(hora, sizeof(hora), "%H:%M:%S", tiempo);

    //Imprimir en consola
    std::cout << "[" << hora << "] - Job " << jobId << " - Prio " << prioridad << " - " << evento << "\n";

    //escribir en archivo
    std::ofstream archivo("sistema.log", std::ios::app);
    archivo << "[" << hora << "] - Job " << jobId << " - Prio " << prioridad << " - " << evento << "\n";
    archivo.close();
}
