#ifndef SEMAFORO_H_INCLUDED
#define SEMAFORO_H_INCLUDED

#include <mutex>
#include <condition_variable>


/// ESTRUCTURA SEMAFORO

/// implementacion de semaforo utilizando:
/// mutex
/// condition_variable
/// contador:
/// cantidad de recursos disponibles.


struct Semaforo {

    int contador;

    std::mutex mtx;

    std::condition_variable cv;
};

void init(Semaforo& s, int n);

void wait(Semaforo& s);

void signal(Semaforo& s);

#endif // SEMAFORO_H_INCLUDED
