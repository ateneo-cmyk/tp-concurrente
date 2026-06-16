#include "semaforo.h"
/// INICIALIZACION DEL SEMAFORO
void init(Semaforo& s, int n) {

    s.contador = n;
}
//si el contador es igual a 0 el hilo queda bloqueado
//esto evita "busy waiting"
void wait(Semaforo& s) {
    std::unique_lock<std::mutex> lock(s.mtx);
    while (s.contador == 0) {
        s.cv.wait(lock);
    }
    s.contador--;
}
/// Libera
/// contador++
/// Luego despierta un hilo bloqueado.
void signal(Semaforo& s) {
    std::unique_lock<std::mutex> lock(s.mtx);
    s.contador++;
    s.cv.notify_one();
}