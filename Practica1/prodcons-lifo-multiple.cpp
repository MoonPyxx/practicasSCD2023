#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "scd.h"

using namespace std ;
using namespace scd ;

//**********************************************************************
// Variables globales

const unsigned
        num_items = 40 ,   // número de items
tam_vec   = 10 ,   // tamaño del buffer
np = 20, // numero hebras productoras
nc = 20; // numero hebras consumidoras
unsigned
        cont_prod[num_items] = {0}, // contadores de verificación: para cada dato, número de veces que se ha producido.
cont_cons[num_items] = {0}, // contadores de verificación: para cada dato, número de veces que se ha consumido.
siguiente_dato       = 0 ,  // siguiente dato a producir en 'producir_dato' (solo se usa ahí)
num_prod[np] ={0}; // cuantos items cada hebra productora
int buffer[tam_vec], primera_libre = 0;

Semaphore
        libres(tam_vec),
        ocupadas(0),
        exclusion(1); // garantiza exclusion mutua

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

unsigned producir_dato(unsigned num_hebra)
{
    this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
    const unsigned dato_producido = num_hebra*(num_items/np) + num_prod[num_hebra] ;
    num_prod[num_hebra]++;
    cont_prod[dato_producido] ++ ;
    cout << "producido: " << dato_producido << " por hebra " << num_hebra << endl << flush ;
    return dato_producido ;
}
//----------------------------------------------------------------------
void consumir_dato( unsigned dato, unsigned num_hebra )
{
    assert( dato < num_items );
    cont_cons[dato] ++ ;
    this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

    cout << "                  consumido: " << dato << " por hebra " << num_hebra<< endl ;

}


//----------------------------------------------------------------------

void test_contadores()
{
    bool ok = true ;
    cout << "comprobando contadores ...." ;
    for( unsigned i = 0 ; i < num_items ; i++ )
    {  if ( cont_prod[i] != 1 )
        {  cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
            ok = false ;
        }
        if ( cont_cons[i] != 1 )
        {  cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
            ok = false ;
        }
    }
    if (ok)
        cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

//----------------------------------------------------------------------

void  funcion_hebra_productora( unsigned num_hebra )
{
    for( unsigned i = 0 ; i < num_items/np ; i++ )
    {
        int dato = producir_dato(num_hebra) ;
        sem_wait(libres);
        sem_wait(exclusion);
        buffer[primera_libre] = dato;
        primera_libre++;
        sem_signal(exclusion);
        sem_signal(ocupadas);
    }
}

//----------------------------------------------------------------------

void funcion_hebra_consumidora( unsigned num_hebra )
{
    for( unsigned i = 0 ; i < num_items/nc ; i++ )
    {
        int dato ;
        sem_wait(ocupadas);
        sem_wait(exclusion);
        primera_libre--;
        dato=buffer[primera_libre];
        sem_signal(exclusion);
        sem_signal(libres);
        consumir_dato( dato, num_hebra ) ;
    }
}
//----------------------------------------------------------------------

int main()
{
    cout << "-----------------------------------------------------------------" << endl
         << "Problema de los productores-consumidores con multiples hebras (solución LIFO)." << endl
         << "------------------------------------------------------------------" << endl
         << flush ;

    thread hebras_productoras[np];
    thread hebras_consumidoras[nc];

    for (unsigned i=0; i<np; i++){
        hebras_productoras[i] = thread(funcion_hebra_productora,i);
    }
    for (unsigned i = 0; i< nc; i++){
        hebras_consumidoras[i] = thread(funcion_hebra_consumidora,i);
    }

    for(unsigned i = 0; i < np; i++)
    {
        hebras_productoras[i].join();
    }

    for(unsigned i = 0; i < nc; i++)
    {
        hebras_consumidoras[i].join();
    }

    test_contadores();
}
