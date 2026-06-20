#include "sistema_render.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <cstdlib>
int main() {
 
    inicializarSistema();

    int opcion;

    std::cout << "=== SIMULADOR DE RENDERIZADO ===\n";
    std::cout << "1. Prueba Config C (3 Productores, 1 Consumidor - 45 tareas c/u)\n";
    std::cout << "2. Prueba Carga Masiva (1 Productor inyectando 1500 jobs, 3 Consumidores)\n";
    std::cout << "3. Prueba de Vacuidad (0 Jobs)\n";
	std::cout << "4. Prueba de Saturacion de Recursos (8 Jobs Premium)\n";
    std::cout << "Ingrese la prueba a ejecutar: ";
    std::cin >> opcion;

    switch (opcion) {
    case 1:
        std::cout << "\nEjecutando Configuracion C...\n";
		
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
    case 4:
		std::cout << "\nEjecutando Prueba de Saturacion de Recursos 8 Jobs Premium\n";
		
        ejecutarPruebaEspecial(1, 3, 8); 
		break;
    default:
        std::cout << "\nOpcion invalida.\n";
        return 1;
    }

    std::cout << "\n>>> SIMULACION FINALIZADA <<<\n";
    std::cout << "Trabajos finalizados con exito: " << obtenerTrabajosFinalizados() << "\n";
    std::cout << "Revise la consola o el archivo 'sistema.log' para la auditoria.\n";

    
    std::system("pause");

    return 0;
}

