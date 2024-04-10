#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "scd.h"

using namespace std ;
using namespace scd ;

// Portafolios:
/*
 Variables/variables permanentes:
 num_fumadores: const int, valores desde 1 pero usamos 3, es el numero de fumadores y por lo tanto, de hebras que fuman en el programa.
 min_ms : const int, valor 5 pero puede ser desde 1 hasta max_ms, tiempo mínimo de espera en sleep_for
 max_ms : const int, valor 20 pero puede ser desde min_ms hasta infinito, tiempo máximo de espera en sleep_for
 mutx: mutex, variable mutex para coordinar mensajes en pantalla.
 ingrediente_en_mostrador: int, valores desde -1 a 3, -1 si no hay ninguno en mostrador, indica que ingrediente está disponible para que el fumador lo tome.
 Colas condición:
 cola_fumadores[num_fumadores]: CondVar, condición: ingrediente_en_mostrador = i(numero del fumador)
 cola_estanquero: CondVar, condición: ingrediente_en_mostrador == -1, no hay ingrediente en el mostrador.

 Pseudocódigo procedimientos:
    procedure obtenerIngrediente(i:integer)
    begin
        if ingrediente_en_mostrador!=i then
        cola_fumadores[i].wait();
        end
        ingrediente_en_mostrador= -1;
        cout << "Fumador ha tomado";
        cola_estanquero.signal();
    end
    procedure ponerIngrediente(i:integer)
    begin
        ingrediente_en_mostrador= i;
        cout << "Estanquero ha puesto";
        cola_fumadores[i].signal();
    end
    procedure esperarRecogidaIngrediente();
    begin
        if ingrediente_en_mostrador != -1 then
        cola_estanquero.wait();
        end
    end
  */



// numero de fumadores 

const int num_fumadores = 3,
          min_ms = 5,
          max_ms = 20;
mutex mutx;

//-------------------------------------------------------------------------
// Función que simula la acción de producir un ingrediente, como un retardo
// aleatorio de la hebra (devuelve número de ingrediente producido)

int producir_ingrediente()
{
    // calcular milisegundos aleatorios de duración de la acción de fumar)
    chrono::milliseconds duracion_produ( aleatorio<min_ms,max_ms>() );

    // informa de que comienza a producir
    mutx.lock();
    cout << "Estanquero : empieza a producir ingrediente (" << duracion_produ.count() << " milisegundos)" << endl;
    mutx.unlock();
    // espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
    this_thread::sleep_for( duracion_produ );

    const int num_ingrediente = aleatorio<0,num_fumadores-1>() ;

    // informa de que ha terminado de producir
    mutx.lock();
    cout << "Estanquero : termina de producir ingrediente " << num_ingrediente << endl;
    mutx.unlock();
    return num_ingrediente ;
}


//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

    // calcular milisegundos aleatorios de duración de la acción de fumar)
    chrono::milliseconds duracion_fumar( aleatorio<min_ms,max_ms>() );

    // informa de que comienza a fumar
    mutx.lock();
    cout << "Fumador " << num_fumador<< "  :"
         << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;
    mutx.unlock();
    // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
    this_thread::sleep_for( duracion_fumar );

    // informa de que ha terminado de fumar
    mutx.lock();
    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;
    mutx.unlock();
}


class Estanco : public HoareMonitor {
private:
    int ingrediente_en_mostrador;
    CondVar cola_fumadores[num_fumadores];
    CondVar cola_estanquero;
public:
    Estanco();
    void obtenerIngrediente(int i);
    void ponerIngrediente(int i);
    void esperarRecogidaIngrediente();
};

Estanco::Estanco(){
    ingrediente_en_mostrador  = -1;
    for (int i = 0; i < 3; i++) {
        cola_fumadores[i] = newCondVar();
    }
   cola_estanquero = newCondVar();
}

void Estanco::obtenerIngrediente(int i){
        if (ingrediente_en_mostrador != i)
            cola_fumadores[i].wait();

    ingrediente_en_mostrador = -1;
    cout << "Fumador " << i << " ha tomado ingrediente " << i << endl;
    cola_estanquero.signal();
    }
void Estanco::ponerIngrediente(int i) {
    ingrediente_en_mostrador = i;
    cout << "Estanquero ha puesto ingrediente " << i << "en el mostrador" << endl;
    cola_fumadores[i].signal();
    }

void Estanco::esperarRecogidaIngrediente() {
    if (ingrediente_en_mostrador != -1)
        cola_estanquero.wait();
}
//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero(MRef<Estanco> monitor)
{
    while(true) {
        int i = producir_ingrediente();
        monitor->ponerIngrediente(i);
        monitor->esperarRecogidaIngrediente();

    }
}
//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( int num_fumador, MRef<Estanco> monitor )
{
    while( true )
    {
        monitor->obtenerIngrediente(num_fumador);
         fumar(num_fumador);
    }
}

//----------------------------------------------------------------------

int main()
{
    MRef<Estanco> monitor = Create<Estanco>();
    thread hebra_estanquero(funcion_hebra_estanquero,monitor);
    thread hebra_fumadores[num_fumadores];

    for(int i = 0; i < num_fumadores; ++i)
        hebra_fumadores[i] = thread(funcion_hebra_fumador, i, monitor);

    hebra_estanquero.join();

    for(int i = 0; i < num_fumadores; ++i)
        hebra_fumadores[i].join();
    return 0;
}

/* Portafolio: Listado de la salida:
Estanquero : empieza a producir ingrediente (19 milisegundos)
Estanquero : termina de producir ingrediente 2
Estanquero ha puesto ingrediente 2
Fumador 2 ha tomado ingrediente 2
Fumador 2  : empieza a fumar (10 milisegundos)
Estanquero : empieza a producir ingrediente (6 milisegundos)
Estanquero : termina de producir ingrediente 0
Estanquero ha puesto ingrediente 0
Fumador 2  : termina de fumar, comienza espera de ingrediente.
Fumador 0 ha tomado ingrediente 0
Fumador 0  : empieza a fumar (9 milisegundos)
Estanquero : empieza a producir ingrediente (14 milisegundos)
Estanquero : termina de producir ingrediente 2
Estanquero ha puesto ingrediente 2
Fumador 2 ha tomado ingrediente 2
Fumador 2  : empieza a fumar (12 milisegundos)
Estanquero : empieza a producir ingrediente (17 milisegundos)
Fumador 2  : termina de fumar, comienza espera de ingrediente.
Estanquero : termina de producir ingrediente 1
Estanquero ha puesto ingrediente 1
Fumador 1 ha tomado ingrediente 1
Fumador 1  : empieza a fumar (16 milisegundos)
 */