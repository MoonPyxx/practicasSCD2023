// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 4. Implementación de sistemas de tiempo real.
//
// Archivo: ejecutivo2.cpp
// Implementación del segundo ejemplo de ejecutivo cíclico:
//
//   Datos de las tareas:
//   ------------
//   Ta.  T    C
//   ------------
//   A  500  100
//   B  500   150
//   C  1000   200
//   D  2000   240
//  -------------
//
//  Planificación (con Ts == 500 ms)
//  *---------*----------*---------*--------*
//  | A B C  | A B D  | A B C  | A B |
//  *---------*----------*---------*--------*
// Portafolio:
// 1. Minimo tiempo de espera: 10ms (en el ciclo 2, ya que Ts = 500 y A = 100ms, B = 150ms y D = 240ms).
// Por esto, 100+150+240 = 490ms, 500ms- 490ms = 10ms
// 2. Si sería planificable, aunque al cambiar el tiempo cómputo a 250ms, el ciclo 2 pasaría a tener 0ms de tiempo de espera ya que 100+150+250ms = 500ms.
// Aunque al no tener ningún tiempo de espera es probable que ocurra algún error debido a algún posible retraso.

#include <string>
#include <iostream> // cout, cerr
#include <thread>
#include <chrono>   // utilidades de tiempo
#include <ratio>    // std::ratio_divide

using namespace std ;
using namespace std::chrono ;
using namespace std::this_thread ;

// tipo para duraciones en segundos y milisegundos, en coma flotante:
//typedef duration<float,ratio<1,1>>    seconds_f ;
typedef duration<float,ratio<1,1000>> milliseconds_f ;

// -----------------------------------------------------------------------------
// tarea genérica: duerme durante un intervalo de tiempo (de determinada duración)

void Tarea( const std::string & nombre, milliseconds tcomputo )
{
    cout << "   Comienza tarea " << nombre << " (C == " << tcomputo.count() << " ms.) ... " ;
    sleep_for( tcomputo );
    cout << "fin." << endl ;
}

// -----------------------------------------------------------------------------
// tareas concretas del problema:

void TareaA() { Tarea( "A", milliseconds(100) );  }
void TareaB() { Tarea( "B", milliseconds( 150) );  }
void TareaC() { Tarea( "C", milliseconds( 200) );  }
void TareaD() { Tarea( "D", milliseconds( 240) );  }

// -----------------------------------------------------------------------------
// implementación del ejecutivo cíclico:

int main( int argc, char *argv[] )
{
    // Ts = duración del ciclo secundario (en unidades de milisegundos, enteros)
    const milliseconds Ts_ms( 500 );

    // ini_sec = instante de inicio de la iteración actual del ciclo secundario
    time_point<steady_clock> ini_sec = steady_clock::now();

    while( true ) // ciclo principal
    {
        cout << endl
             << "---------------------------------------" << endl
             << "Comienza iteración del ciclo principal." << endl ;

        for( int i = 1 ; i <= 4 ; i++ ) // ciclo secundario (4 iteraciones)
        {
            cout << endl << "Comienza iteración " << i << " del ciclo secundario." << endl ;

            switch( i )
            {
                case 1 : TareaA(); TareaB(); TareaC();           break ;
                case 2 : TareaA(); TareaB(); TareaD();           break ;
                case 3 : TareaA(); TareaB(); TareaC();           break ;
                case 4 : TareaA(); TareaB();                     break ;
            }

            // calcular el siguiente instante de inicio del ciclo secundario
            ini_sec += Ts_ms ;

            // esperar hasta el inicio de la siguiente iteración del ciclo secundario
            sleep_until( ini_sec );
            // calcular y mostrar retraso
            time_point<steady_clock> tiempo_actual = steady_clock::now();
            steady_clock::duration retraso = tiempo_actual - ini_sec;
            cout << "Retraso al final del ciclo " << i << ": "<< milliseconds_f(retraso).count() << " ms." << endl;
        }
    }
}
