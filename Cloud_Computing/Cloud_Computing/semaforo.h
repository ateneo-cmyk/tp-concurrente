#pragma once
#include <mutex>
#include <condition_variable>

struct Semaforo {

    int contador=0;

    std::mutex mtx;

    std::condition_variable cv;
};
void init(Semaforo& s, int n);
void wait(Semaforo& s);
void signal(Semaforo& s);

