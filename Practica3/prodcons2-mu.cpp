// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: prodcons2-mu.cpp
// Implementación del problema del productor-consumidor con
// un proceso intermedio que gestiona un buffer finito y recibe peticiones
// en orden arbitrario
// (versión con un múltiples productores y consumidores)

// mpicxx  -std=c++11 -g  -o prodconsmu prodcons2-mu.cpp
// mpirun -oversubscribe -np  10 ./prodconsmu
/* PORTAFOLIOS: Cambios añadidos al código.
// Añadidas variables num_productores y num_consumidores
    Añadidas etiquetas para diferenciar entre productores y consumidores
    Creadas variables items_prod e items_cons para saber cuantos items por productor y por consumidor
    Ahora producir, y las funciones del productor y del consumidor llevan como argumento el valor a consumir/producir
    Se pasan como parametros en el main el id del productor o consumidor
    Las funciones productoras y consumidoras usan las nuevas variables para los bucles como items_prod
    Se usan las nuevas etiquetas para enviar y recibir mensajes correctamente.
*/

#include <iostream>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <mpi.h>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

const int
   id_productor          = 0 ,
   id_consumidor         = 1 ,
   num_procesos_esperado = 10 ,
   num_items             = 20,
   tam_vector            = 10,
   num_productores       = 4 ,
   num_consumidores       = 5 ,
   id_buffer             = num_productores;

//etiquetas
const int etiq_prod =1, etiq_cons = 2;
// items por productor/consumidor
unsigned int items_prod = num_items/num_productores,
items_cons = num_items/num_consumidores;

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
// ptoducir produce los numeros en secuencia (1,2,3,....)
// y lleva espera aleatorio
int producir(int num)
{
   static int contador = num * items_prod;
   sleep_for( milliseconds( aleatorio<10,100>()) );
   contador++;
   cout << "Productor " << num << " ha producido valor " << contador << endl << flush;
   return contador ;
}
// ---------------------------------------------------------------------

void funcion_productor(int num)
{
   for ( unsigned int i= 0 ; i < items_prod ; i++ )
   {
      // producir valor
      int valor_prod = producir(num);
      // enviar valor
      cout << "Productor "<< num << " va a enviar valor " << valor_prod << endl << flush;
      MPI_Ssend( &valor_prod, 1, MPI_INT, id_buffer, etiq_prod, MPI_COMM_WORLD );
   }
}
// ---------------------------------------------------------------------

void consumir( int valor_cons )
{
   // espera bloqueada
   sleep_for( milliseconds( aleatorio<10,200>()) );
   cout << "Consumidor ha consumido valor " << valor_cons << endl << flush ;
}
// ---------------------------------------------------------------------

void funcion_consumidor(int num)
{
   int         peticion,
               valor_rec = 1 ;
   MPI_Status  estado ;

   for( unsigned int i=0 ; i < num_items / num_consumidores; i++ )
   {
      MPI_Ssend( &peticion,  1, MPI_INT, id_buffer, etiq_cons, MPI_COMM_WORLD);
      MPI_Recv ( &valor_rec, 1, MPI_INT, id_buffer, etiq_cons, MPI_COMM_WORLD,&estado );
      cout << "Consumidor "<< num <<" ha recibido valor " << valor_rec << endl << flush ;
      consumir( valor_rec );
   }
}
// ---------------------------------------------------------------------

void funcion_buffer()
{
   int        buffer[tam_vector],      // buffer con celdas ocupadas y vacías
              valor,                   // valor recibido o enviado
              primera_libre       = 0, // índice de primera celda libre
              primera_ocupada     = 0, // índice de primera celda ocupada
              num_celdas_ocupadas = 0, // número de celdas ocupadas
              id_emisor_aceptable ;    // identificador de emisor aceptable
   MPI_Status estado ;                 // metadatos del mensaje recibido

   for( unsigned int i=0 ; i < num_items*2 ; i++ )
   {
      // 1. determinar si puede enviar solo prod., solo cons, o todos

      if ( num_celdas_ocupadas == 0 )               // si buffer vacío
         id_emisor_aceptable = etiq_prod ;       // $~~~$ solo prod.
      else if ( num_celdas_ocupadas == tam_vector ) // si buffer lleno
         id_emisor_aceptable = etiq_cons ;      // $~~~$ solo cons.
      else                                          // si no vacío ni lleno
         id_emisor_aceptable = MPI_ANY_TAG ;     // $~~~$ cualquiera

      // 2. recibir un mensaje del emisor o emisores aceptables

      MPI_Recv( &valor, 1, MPI_INT, MPI_ANY_SOURCE, id_emisor_aceptable, MPI_COMM_WORLD, &estado );

      // 3. procesar el mensaje recibido

      switch( estado.MPI_TAG ) // leer emisor del mensaje en metadatos
      {
         case etiq_prod: // si ha sido el productor: insertar en buffer
            buffer[primera_libre] = valor ;
         primera_libre = (primera_libre+1) % tam_vector ;
         num_celdas_ocupadas++ ;
         cout << "Buffer ha recibido valor " << valor << endl ;
         break;

         case etiq_cons: // si ha sido el consumidor: extraer y enviarle
            valor = buffer[primera_ocupada] ;
         primera_ocupada = (primera_ocupada+1) % tam_vector ;
         num_celdas_ocupadas-- ;
         cout << "Buffer va a enviar valor " << valor << endl ;
         MPI_Ssend( &valor, 1, MPI_INT, estado.MPI_SOURCE,etiq_cons , MPI_COMM_WORLD);
         break;
      }
   }
}

// ---------------------------------------------------------------------

int main( int argc, char *argv[] )
{
   int id_propio, num_procesos_actual;

   // inicializar MPI, leer identif. de proceso y número de procesos
   MPI_Init( &argc, &argv );
   MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
   MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );

   if ( num_procesos_esperado == num_procesos_actual )
   {
      // ejecutar la operación apropiada a 'id_propio'
      if ( id_propio < id_buffer ) // Procesos de productores
         funcion_productor(id_propio);
      else if ( id_propio == id_buffer ) // procesos de buffer
         funcion_buffer();
      else  // Procesos de consumidores
         funcion_consumidor(id_propio-id_buffer);
   }
   else
   {
      if ( id_propio == 0 ) // solo el primero escribe error, indep. del rol
      { cout << "el número de procesos esperados es:    " << num_procesos_esperado << endl
             << "el número de procesos en ejecución es: " << num_procesos_actual << endl
             << "(programa abortado)" << endl ;
      }
   }

   // al terminar el proceso, finalizar MPI
   MPI_Finalize( );
   return 0;
}
/* PORTAFOLIOS: Ejemplo salida
mpicxx  -std=c++11 -g -Wall -o prodcons2-mu_mpi_exe prodcons2-mu.cpp
mpirun -oversubscribe  -np  10 ./prodcons2-mu_mpi_exe
Productor 0 ha producido valor 1
Productor 0 va a enviar valor 1
Buffer ha recibido valor 1
Buffer va a enviar valor 1
Consumidor 1 ha recibido valor 1
Productor 1 ha producido valor 6
Productor 1 va a enviar valor 6
Buffer ha recibido valor 6
Buffer va a enviar valor 6
Consumidor 2 ha recibido valor 6
Productor 0 ha producido valor 2
Productor 0 va a enviar valor 2
Buffer ha recibido valor 2
Buffer va a enviar valor 2
Consumidor 3 ha recibido valor 2
Consumidor ha consumido valor 1
Consumidor ha consumido valor 2
  */
