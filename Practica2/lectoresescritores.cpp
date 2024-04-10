#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "scd.h"
#include <chrono>

using namespace std;
using namespace scd;
using namespace std::chrono;
constexpr int num_escritores = 3, num_lectores = 2,
              min_ms = 5, max_ms = 20;
mutex mutx;
class Lec_Esc_Monitor : public HoareMonitor{
private:
    int n_lec;
    bool escrib;
    CondVar lectura,
    escritura;

public:
    Lec_Esc_Monitor();
    void ini_lectura();
    void fin_lectura();
    void ini_escritura();
    void fin_escritura();
};
Lec_Esc_Monitor::Lec_Esc_Monitor(){
    n_lec = 0;
    escrib = false;
    lectura = newCondVar();
    escritura = newCondVar();
}
void Lec_Esc_Monitor::ini_lectura() {
    if (escrib)
        lectura.wait();
    n_lec++;
    lectura.signal();
}
void Lec_Esc_Monitor::fin_lectura() {
    n_lec--;
    if (n_lec == 0){
        escritura.signal();
    }
}
void Lec_Esc_Monitor::ini_escritura() {
    if (n_lec > 0 || escrib)
        escritura.wait();
    escrib = true;
}
void Lec_Esc_Monitor::fin_escritura() {
    escrib = false;
    if (!lectura.empty()){
        lectura.signal();
    } else{
        escritura.signal();
    }
}
void hebra_escritora(MRef<Lec_Esc_Monitor> monitor, int num){
    while (true){
        monitor->ini_escritura();
        mutx.lock();
        cout << "Escritor "<< num << " ha comenzado a escribir." << endl;
        chrono::milliseconds duracion(aleatorio<min_ms,max_ms>());
        this_thread::sleep_for(duracion);
        cout << "Escritor "<< num << " ha terminado de escribir." << endl;
        mutx.unlock();
        monitor->fin_escritura();
        this_thread::sleep_for(duracion);
    }
}
void hebra_lectora(MRef <Lec_Esc_Monitor> monitor, int num){
    while (true){
        monitor->ini_lectura();
        chrono::milliseconds duracion(aleatorio<20,200>());
        mutx.lock();
        cout << "Lector " << num << " comienza a leer." << endl;
        this_thread::sleep_for(duracion);
        cout << "Lector "<< num << " ha terminado de leer." << endl;
         mutx.unlock();
        monitor->fin_lectura();
        this_thread::sleep_for(duracion);
    }
}
int main(){
    MRef<Lec_Esc_Monitor> monitor = Create<Lec_Esc_Monitor>();
    thread lectores[num_lectores];
    thread escritores[num_escritores];


    for (int i= 0; i< num_lectores; i++){
        lectores[i] = thread(hebra_lectora, monitor,i);
    }
    for (int i= 0; i< num_escritores; i++){
        escritores[i] = thread(hebra_escritora, monitor, i);
    }
    for (int i = 0; i < num_lectores; i++) {
        lectores[i].join();
    }

    for (int i = 0; i < num_escritores; i++) {
        escritores[i].join();
    }
    return 0;
}