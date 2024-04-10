// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: filosofos.cpp
// Implementación del problema de los filósofos (sin camarero).

/* PORTAFOLIOS:
Aspectos más importantes de la solución: Uso de Ssend y rcev junto a etiquetas para coordinar correctamente los procesos
 tenedor y filósofos correctamente.
 - Los filósofos piden su tenedor izquierdo y luego el derecho.
 - El tenedor primero pide ser cogido por un filósofo cualquiera y luego tienen que recibir el mensaje para ser liberados por el mismo filósofo que los había ocupado.
 La situación que conduce al interbloque ocurre si todos los filósofos envían el mensaje para pedir su tenedor de la izquierda, provocando que el de la derecha
 siempre estuviese ocupado y dando una situación de interbloqueo.
 Para solucionarlo, una de las soluciones es hacer que el primer filósofo solicite primero el tenedor de su derecha y luego el de la izquierda, lo cual soluciona el problema.

*/


#include <mpi.h>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <iostream>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

const int
   num_filosofos = 5 ,              // número de filósofos
   num_filo_ten  = 2*num_filosofos, // número de filósofos y tenedores
   num_procesos  = num_filo_ten ,   // número de procesos total (por ahora solo hay filo y ten)
   // etiquetas
   etiq_coger = 1,
   etiq_soltar = 2;

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

// ---------------------------------------------------------------------

void funcion_filosofos( int id )
{
  int id_ten_izq = (id+1)              % num_filo_ten, //id. tenedor izq.
      id_ten_der = (id+num_filo_ten-1) % num_filo_ten; //id. tenedor der.

  while ( true )
  {
if (id == 0){ // El primer filósofo cambia de orden para evitar interbloqueo
   cout <<"Filósofo " <<id <<" solicita ten. der." <<id_ten_der <<endl;
   MPI_Ssend(&id, 1, MPI_INT, id_ten_der, etiq_coger, MPI_COMM_WORLD);
   cout <<"Filósofo " <<id << " solicita ten. izq." <<id_ten_izq <<endl;
   MPI_Ssend(&id, 1, MPI_INT, id_ten_izq, etiq_coger, MPI_COMM_WORLD);
} else {
   cout <<"Filósofo " <<id << " solicita ten. izq." <<id_ten_izq <<endl;
   MPI_Ssend(&id, 1, MPI_INT, id_ten_izq, etiq_coger, MPI_COMM_WORLD);
   cout <<"Filósofo " <<id <<" solicita ten. der." <<id_ten_der <<endl;
   MPI_Ssend(&id, 1, MPI_INT, id_ten_der, etiq_coger, MPI_COMM_WORLD);
}
    cout <<"Filósofo " <<id <<" comienza a comer" <<endl ;
    sleep_for( milliseconds( aleatorio<10,100>() ) );

    cout <<"Filósofo " <<id <<" suelta ten. izq. " <<id_ten_izq <<endl;
     MPI_Ssend(&id, 1, MPI_INT, id_ten_izq, etiq_soltar, MPI_COMM_WORLD);

    cout<< "Filósofo " <<id <<" suelta ten. der. " <<id_ten_der <<endl;
     MPI_Ssend(&id, 1, MPI_INT, id_ten_der, etiq_soltar, MPI_COMM_WORLD);

    cout << "Filosofo " << id << " comienza a pensar" << endl;
    sleep_for( milliseconds( aleatorio<10,100>() ) );
 }
}
// ---------------------------------------------------------------------

void funcion_tenedores( int id )
{
  int valor, id_filosofo ;  // valor recibido, identificador del filósofo
  MPI_Status estado ;       // metadatos de las dos recepciones

  while ( true )
  {
     MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_coger, MPI_COMM_WORLD, &estado);
   id_filosofo = estado.MPI_SOURCE;
      cout <<"Ten. " <<id <<" ha sido cogido por filo. " <<id_filosofo <<endl;
MPI_Recv(&valor,1,MPI_INT,id_filosofo, etiq_soltar, MPI_COMM_WORLD, &estado);
     cout <<"Ten. "<< id<< " ha sido liberado por filo. " <<id_filosofo <<endl ;
  }
}
// ---------------------------------------------------------------------

int main( int argc, char** argv )
{
   int id_propio, num_procesos_actual ;

   MPI_Init( &argc, &argv );
   MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
   MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );


   if ( num_procesos == num_procesos_actual )
   {
      // ejecutar la función correspondiente a 'id_propio'
      if ( id_propio % 2 == 0 )          // si es par
         funcion_filosofos( id_propio ); //   es un filósofo
      else                               // si es impar
         funcion_tenedores( id_propio ); //   es un tenedor
   }
   else
   {
      if ( id_propio == 0 ) // solo el primero escribe error, indep. del rol
      { cout << "el número de procesos esperados es:    " << num_procesos << endl
             << "el número de procesos en ejecución es: " << num_procesos_actual << endl
             << "(programa abortado)" << endl ;
      }
   }

   MPI_Finalize( );
   return 0;
}
// PORTAFOLIOS: Salida del programa
/* mpicxx  -std=c++11 -g -Wall -o filosofos-interb_mpi_exe filosofos-interb.cpp
mpirun -oversubscribe  -np 10 ./filosofos-interb_mpi_exe
Filósofo 2 solicita ten. izq.3
Filósofo 4 solicita ten. izq.5
Filósofo 8 solicita ten. izq.9
Filósofo 6 solicita ten. izq.7
Ten. 3 ha sido cogido por filo. 2
Filósofo 2 solicita ten. der.1
Ten. 7 ha sido cogido por filo. 6
Filósofo 6 solicita ten. der.5
Ten. 5 ha sido cogido por filo. 4
Filósofo 8 solicita ten. der.7
Filósofo 4 solicita ten. der.3
Ten. 9 ha sido cogido por filo. 8
Filósofo 0 solicita ten. izq.1
Ten. 1 ha sido cogido por filo. 2
Filósofo 2 comienza a comer
*/