// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: filosofos-cam.cpp
// Implementación del problema de los filósofos (con camarero).

// PORTAFOLIOS:
/*
La solución para el problema de los filósofos con camarero consiste básicamente en añadir una solicitud en la función de los filósofos para sentarse
Luego desde la función del camarero se gestiona si los camareros se pueden sentar o no en base al número de camareros que haya sentados.
Existe una variable que cuenta el número de filósofos que se encuentran sentados en cada momento.
Los procesos de los tenedores siguen igual.
Se usan dos etiquetas nuevas para indicar cuando el mensaje del filósofo es para solicitar sentarse o levantarse.
Si está lleno, solo pueden levantarse, si está vacío, sentarse y en cualquier otro caso ambas.
*/
#include <mpi.h>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempoo
#include <iostream>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

const int
   num_filosofos = 5 ,              // número de filósofos
   num_filo_ten  = 2*num_filosofos, // número de filósofos y tenedores = 10
   num_procesos  = num_filo_ten + 1 ,   // número de procesos total (filo,ten y camarero = 11)
   // etiquetas
   etiq_coger = 1,
   etiq_soltar = 2,
   etiq_sentarse = 3,
   etiq_levantarse = 4,
   // Número máximo de filósofos sentados en la mesa
   max_filo_sentados = 4,
   id_camarero = num_procesos - 1; // 10

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
// Sentarse
   cout << "Filósofo " << id << " solicita sentarse" << endl;
   MPI_Ssend(&id, 1, MPI_INT, id_camarero, etiq_sentarse, MPI_COMM_WORLD);
// Coger tenedores
    cout <<"Filósofo " <<id << " solicita ten. izq." <<id_ten_izq <<endl;
   MPI_Ssend(&id, 1, MPI_INT, id_ten_izq, etiq_coger, MPI_COMM_WORLD);
    cout <<"Filósofo " <<id <<" solicita ten. der." <<id_ten_der <<endl;
    MPI_Ssend(&id, 1, MPI_INT, id_ten_der, etiq_coger, MPI_COMM_WORLD);
// Comer
    cout <<"Filósofo " <<id <<" comienza a comer" <<endl ;
    sleep_for( milliseconds( aleatorio<10,100>() ) );
// Soltar tenedores
    cout <<"Filósofo " <<id <<" suelta ten. izq. " <<id_ten_izq <<endl;
     MPI_Ssend(&id, 1, MPI_INT, id_ten_izq, etiq_soltar, MPI_COMM_WORLD);
    cout<< "Filósofo " <<id <<" suelta ten. der. " <<id_ten_der <<endl;
     MPI_Ssend(&id, 1, MPI_INT, id_ten_der, etiq_soltar, MPI_COMM_WORLD);
// Levantarse de la mesa
     cout << "Filósofo " << id << " solicita levantarse" << endl;
     MPI_Ssend(&id, 1 , MPI_INT, id_camarero, etiq_levantarse, MPI_COMM_WORLD);
// Pensar
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

void funcion_camarero()
{
   int valor, // valor recibido
       etiq_aceptada,
       filosofos_sentados = 0;
   MPI_Status estado;

   while (true)
   {
      if (filosofos_sentados == max_filo_sentados){
      etiq_aceptada = etiq_levantarse;
      } else {
         etiq_aceptada = MPI_ANY_TAG;
             }
      MPI_Recv(&valor,1,MPI_INT,MPI_ANY_SOURCE, etiq_aceptada, MPI_COMM_WORLD, &estado);

      if (estado.MPI_TAG == etiq_sentarse){
         filosofos_sentados++;
         cout << "Camarero permite al filosofo " << estado.MPI_SOURCE << " sentarse en la mesa." << endl;
      }
      if (estado.MPI_TAG == etiq_levantarse){
         filosofos_sentados--;
         cout << "Camarero permite al filosofo " << estado.MPI_SOURCE << " levantarse de la mesa." << endl;
      }
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
      if (id_propio == id_camarero)
            funcion_camarero();
      // ejecutar la función correspondiente a 'id_propio'
      else if ( id_propio % 2 == 0 )          // si es par
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
/*
mpirun -oversubscribe  -np 11 ./filosofos-cam_mpi_exe
Filósofo 8 solicita sentarse
Filósofo 0 solicita sentarse
Filósofo 2 solicita sentarse
Filósofo 4 solicita sentarse
Filósofo 6 solicita sentarse
Filósofo 0 solicita ten. izq.1
Camarero permite al filosofo 0 sentarse en la mesa.
Camarero permite al filosofo 2 sentarse en la mesa.
Filósofo 2 solicita ten. izq.3
Filósofo 2 solicita ten. der.1
Ten. 3 ha sido cogido por filo. 2
Camarero permite al filosofo 4 sentarse en la mesa.
Filósofo 4 solicita ten. izq.5
Filósofo 6 solicita ten. izq.7
Camarero permite al filosofo 6 sentarse en la mesa.
Filósofo 6 solicita ten. der.5
Ten. 7 ha sido cogido por filo. 6
Filósofo 0 solicita ten. der.9
Filósofo 0 comienza a comer
 */