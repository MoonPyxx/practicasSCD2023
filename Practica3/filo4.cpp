	// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: prodcons2.cpp
// Implementación del problema del productor-consumidor con
// un proceso intermedio que gestiona un buffer finito y recibe peticiones
// en orden arbitrario
// (versión con un único productor y un único consumidor)
//
// Historial:
// Actualizado a C++11 en Septiembre de 2017
// -----------------------------------------------------------------------------

#include <iostream>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <mpi.h>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

const int
	num_productores	= 4 ,
	num_productores_fals	= 2 ,
	num_consumidores	= 8,
	num_buffers		= 2 ,	
   	num_procesos_esperado	= num_productores + num_consumidores + num_productores_fals + num_buffers ,
   	num_items		= 80,
   	id_buffer		= num_productores + num_productores_fals,
   	id_buf_fals		= num_productores + num_productores_fals + 1,
   	tam_vector		= 10,
   	tag_productor		= 0 ,
   	tag_buffer		= 1 ,
   	tag_consumidor		= 2 ,
   	tag_productor_fals	= 3 ;

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
int producir(int orden)
{
   static int contador = 0 + orden*(num_items/num_productores) ;
   sleep_for( milliseconds( aleatorio<10,100>()) );
   contador++ ;
   cout << "Productor " << orden << " ha producido valor " << contador /*<< " y tag "<< tag_productor */<< endl << flush;
   return contador ;
}
// ---------------------------------------------------------------------

void funcion_productor(int orden)
{
   for ( unsigned int i= 0 ; i < num_items/num_productores ; i++ )
   {
      // producir valor
      int valor_prod = producir(orden);
      // enviar valor
      //cout << "Mi orden es: " << orden << " y mi valor prod es " << valor_prod << endl;
      cout << "Productor " << orden << " va a enviar valor " << valor_prod << endl;
      MPI_Ssend( &valor_prod,  1 , MPI_INT, id_buffer , tag_productor , MPI_COMM_WORLD);
   }
}
// ---------------------------------------------------------------------

void funcion_productor_fals(int orden)
{
	const int valor_falso = 44;
	for (unsigned int i = 0 ; i < num_items/num_productores_fals ; i ++)
	{
	cout << "Productor "<< orden << " fals ha producido su valor" << endl;
	sleep_for( milliseconds( aleatorio<110,200>()) );
	int valor_prod = valor_falso;
	MPI_Ssend( &valor_prod,  1 , MPI_INT, id_buf_fals , tag_productor_fals , MPI_COMM_WORLD);
	cout << "Productor "<< orden << " fals ha enviado su valor" << endl;
	}
}	

// ---------------------------------------------------------------------

void consumir( int valor_cons, int orden )
{
   // espera bloqueada
   sleep_for( milliseconds( aleatorio<110,200>()) );
   cout << "Consumidor " << orden << " ha consumido valor " << valor_cons << endl << flush ;
}
// ---------------------------------------------------------------------

void funcion_consumidor(int orden)
{
   int         peticion,
               valor_rec = 1 ;
   MPI_Status  estado ;

   for( unsigned int i=0 ; i < num_items/num_consumidores; i++ )
   {
   	//cout << "Peticion con id: " <<orden<< " y tag " << tag_consumidor <<endl;
   	if (orden == 0){
   		MPI_Ssend( &peticion,  1 , MPI_INT, id_buf_fals , tag_consumidor , MPI_COMM_WORLD);
      		MPI_Recv ( &valor_rec, 1 , MPI_INT, id_buf_fals , tag_consumidor , MPI_COMM_WORLD,&estado );
      		cout << "El consumidor " << orden << " ha consumido un valor falsificado " << valor_rec << endl;
   	}	
   	
      	MPI_Ssend( &peticion,  1 , MPI_INT, id_buffer , tag_consumidor , MPI_COMM_WORLD);
      	MPI_Recv ( &valor_rec, 1 , MPI_INT, id_buffer , tag_consumidor , MPI_COMM_WORLD,&estado );
      	cout << "Consumidor " << orden << " ha recibido valor " << valor_rec << endl << flush ;
      	consumir( valor_rec , orden );
   }
   
   //EL ENUNCIADO ES TERRIBLEMENTE AMBIGUO Y POR ELLO PARA LA CORRECTA FINALIZACION DEL PROBLEMA TUVIMOS QUE HACER ESTE BUCLE
   
   /*Esta parte no seria necesaria, pero como el enunciado dice que se producen 80 de cada tipo y como el consumidor de orden 0 debe consumir alternativamente,
   si este tuviera que consumir los 80 falsificados tambien tendria que hacer los 80 no falsificados y ahi habria un error, por lo tanto se decidio que se comportara
   como el resto, pero tras consumir su regimen normal de los no falsificados terminaria de consumir los otros 70 falsificados*/
   
   if (orden == 0){ 
   	for (unsigned int i = 0; i < (num_items - num_items/num_consumidores); i++)
   	{
   		MPI_Ssend( &peticion,  1 , MPI_INT, id_buf_fals , tag_consumidor , MPI_COMM_WORLD);
      		MPI_Recv ( &valor_rec, 1 , MPI_INT, id_buf_fals , tag_consumidor , MPI_COMM_WORLD,&estado );
      		cout << "El consumidor " << orden << " ha consumido un valor falsificado " << valor_rec << endl;	
   	}
   }
}
// ---------------------------------------------------------------------

void funcion_buffer(int orden)
{
   int        buffer[tam_vector],      // buffer con celdas ocupadas y vacías
              valor_e,			//valor enviado
              valor_r,                   // valor recibido 
              primera_libre       	= 0, // índice de primera celda libre
              primera_ocupada     	= 0, // índice de primera celda ocupada
              num_celdas_ocupadas 	= 0, // número de celdas ocupadas
              tag_emisor_aceptable,    // identificador de emisor aceptable
              flag;		
   MPI_Status estado ;                 // metadatos del mensaje recibido

   for( unsigned int i=0 ; i < num_items*2 ; i++ )
   {
      // 1. determinar si puede enviar solo prod., solo cons, o todos 
      	if ( num_celdas_ocupadas == 0 ){               	// si buffer vacío  $~~~$ solo prod.
         	if (orden == 0){
         		cout << "buffer "<< orden << "entra en productor base " << endl;	
         		tag_emisor_aceptable = tag_productor;
         	}else {
         		cout << "buffer "<< orden << "entra en productor falso " << endl;
         		tag_emisor_aceptable = tag_productor_fals;
         	}
      	}      
      	else if ( num_celdas_ocupadas == tam_vector ){ 	// si buffer lleno // $~~~$ solo cons.
      	  	cout << "buffer "<< orden << "entra en consumidor" << endl;
         	tag_emisor_aceptable = tag_consumidor ;
     	}       
      	else{                                         	// si no vacío ni lleno
         	cout << "buffer "<< orden << "entra en cualquiera" << endl;
         	tag_emisor_aceptable = MPI_ANY_TAG ;		// $~~~$ cualquiera
	}
          

      // 2. recibir un mensaje del emisor o emisores aceptables
	//cout << "pendiente de recibir" << endl;
	cout << tag_emisor_aceptable << endl;
      MPI_Recv( &valor_r, 1 , MPI_INT, MPI_ANY_SOURCE , tag_emisor_aceptable , MPI_COMM_WORLD, &estado );
      	cout << "buffer "<< orden << "Ha recibido del proceso " << estado.MPI_SOURCE << endl;
	
	//cout << "Se recibe el valor :" << valor_r << endl;
      // 3. procesar el mensaje recibido

      if (estado.MPI_TAG == tag_productor || estado.MPI_TAG == tag_productor_fals){ // leer emisor del mensaje en metadatos
						 // si ha sido el productor: insertar en buffer
            buffer[primera_libre] = valor_r ;
            primera_libre = (primera_libre+1) % tam_vector ;
            num_celdas_ocupadas++ ;
            cout << "Buffer "<< orden << " ha recibido valor " << valor_r << endl ;
	}else{ // si ha sido el consumidor: extraer y enviarle
            valor_e = buffer[primera_ocupada] ;
            primera_ocupada = (primera_ocupada+1) % tam_vector ;
            num_celdas_ocupadas-- ;
            cout << "Buffer "<< orden << " va a enviar valor " << valor_e << endl ;
            MPI_Ssend( &valor_e, 1, MPI_INT, estado.MPI_SOURCE , tag_consumidor , MPI_COMM_WORLD);
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
      if ( id_propio < num_productores ) 
         funcion_productor(id_propio);
      else if (id_propio < (num_productores + num_productores_fals))
      	 funcion_productor_fals(id_propio % num_productores);
      else if ( id_propio <= id_buf_fals )
         funcion_buffer(id_propio%(num_productores + num_productores_fals));
      else
         funcion_consumidor(id_propio%(num_productores+num_productores_fals+num_buffers));
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
