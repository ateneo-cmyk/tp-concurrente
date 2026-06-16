#include "sistema_render.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
int main() {
    ///inicializar los semaforos
    inicializarSistema();

    int opcion;

    std::cout << "=== SIMULADOR DE RENDERIZADO ===\n";
    std::cout << "1. Prueba Config C (3 Productores, 3 Consumidores - 15 tareas c/u)\n";
    std::cout << "2. Prueba Carga Masiva (1 Productor inyectando 1500 jobs, 3 Consumidores)\n";
    std::cout << "3. Prueba de Vacuidad (0 Jobs)\n";
    std::cout << "Ingrese la prueba a ejecutar: ";
    std::cin >> opcion;

    switch (opcion) {
    case 1:
        std::cout << "\nEjecutando Configuracion C...\n";
        //ejecutarPrueba(1, 2, 15);
        //ejecutarPrueba(3, 3, 15);
        ejecutarPrueba(3, 1, 15);

        break;
    case 2:
        std::cout << "\nEjecutando Carga Masiva...\n";
        ejecutarPrueba(1, 3, 1500);
        break;
    case 3:
        std::cout << "\nEjecutando Vacuidad...\n";
        ejecutarPrueba(3, 3, 0);
        break;
    default:
        std::cout << "\nOpcion invalida.\n";
        return 1;
    }

    std::cout << "\n>>> SIMULACION FINALIZADA <<<\n";
    std::cout << "Trabajos finalizados con exito: " << obtenerTrabajosFinalizados() << "\n";
    std::cout << "Revise la consola o el archivo 'sistema.log' para la auditoria.\n";

    return 0;
}

