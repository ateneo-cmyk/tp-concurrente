#include "logger.h"
#include <fstream>
#include <mutex>
#include <chrono>
#include <thread>
#include <iostream>
#include <ctime>


std::mutex mtx_log;


void logEvent(int jobId, int prioridad, const char* evento) {
    std::unique_lock<std::mutex> lock(mtx_log);
    
    std::time_t ahora = std::time(nullptr);
    std::tm tiempo;
#if defined(_MSC_VER)
    localtime_s(&tiempo, &ahora);
#else
    localtime_r(&ahora, &tiempo);
#endif
    char hora[20];
    std::strftime(hora, sizeof(hora), "%H:%M:%S", &tiempo);
    
    std::cout << "[" << hora << "] - Job " << jobId << " - Prio " << prioridad << " - " << evento << "\n";
    
    std::ofstream archivo("sistema.log", std::ios::app);
    archivo << "[" << hora << "] - Job " << jobId << " - Prio " << prioridad << " - " << evento << "\n";
    archivo.close();
}