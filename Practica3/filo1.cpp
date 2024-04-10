
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
        num_procesos_esperado = 11   ,
        num_items             = 20,
        tam_vector            = 10,
        num_productores       = 4 ,
        num_consumidores       = 5 ,
        id_primer_buffer       = num_productores,
        id_segundo_buffer = num_productores+1;

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
        // comprobar si es par o impar (ternario)
        int destino_buffer = (valor_prod % 2 == 0) ? id_primer_buffer : id_segundo_buffer;
        // enviar valor
        cout << "Productor "<< num << " va a enviar valor " << valor_prod << " al buffer " << (valor_prod % 2 == 0 ? "par" : "impar") << endl << flush;
        MPI_Ssend( &valor_prod, 1, MPI_INT, destino_buffer, etiq_prod, MPI_COMM_WORLD );
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
    valor_rec = 1;
    MPI_Status  estado ;

    for( unsigned int i=0 ; i < num_items / num_consumidores; i++ )
    {
        int destino_buffer = (i % 2 == 0) ? id_primer_buffer : id_segundo_buffer;
        MPI_Ssend( &peticion,  1, MPI_INT, destino_buffer, etiq_cons, MPI_COMM_WORLD);
        MPI_Recv ( &valor_rec, 1, MPI_INT, destino_buffer, etiq_cons, MPI_COMM_WORLD,&estado );
        cout << "Consumidor " << num << " ha recibido valor " << valor_rec << " del buffer " << (i % 2 == 0 ? "par" : "impar") << endl << flush;
        consumir( valor_rec );
    }
}
// ---------------------------------------------------------------------

void funcion_buffer(int id_buffer)
{
    int        buffer_par[tam_vector], buffer_impar[tam_vector],      // buffer con celdas ocupadas y vacías
    valor,                   // valor recibido o enviado
    primera_libre_impar       = 0, // índice de primera celda libre
    primera_ocupada_impar     = 0, // índice de primera celda ocupada
    primera_libre_par = 0,
    num_celdas_ocupadas_par = 0, // número de celdas ocupadas
    num_celdas_ocupadas_impar = 0,
    id_emisor_aceptable ;    // identificador de emisor aceptable
    MPI_Status estado ;                 // metadatos del mensaje recibido

    for( unsigned int i=0 ; i < num_items*2 ; i++ )
    {
        // 1. determinar si puede enviar solo prod., solo cons, o todos
        if (id_buffer == id_primer_buffer){ // par
            if ( num_celdas_ocupadas_par == 0 )               // si buffer vacío
                id_emisor_aceptable = etiq_prod ;       // $~~~$ solo prod.
            else if ( num_celdas_ocupadas_par == tam_vector ) // si buffer lleno
                id_emisor_aceptable = etiq_cons ;      // $~~~$ solo cons.
            else                                          // si no vacío ni lleno
                id_emisor_aceptable = MPI_ANY_TAG ;     // $~~~$ cualquiera
        }
        else if(id_buffer==id_segundo_buffer){ // impar
                if ( num_celdas_ocupadas_impar == 0 )               // si buffer vacío
                    id_emisor_aceptable = etiq_prod ;       // $~~~$ solo prod.
                else if ( num_celdas_ocupadas_impar == tam_vector ) // si buffer lleno
                    id_emisor_aceptable = etiq_cons ;      // $~~~$ solo cons.
                else                                          // si no vacío ni lleno
                    id_emisor_aceptable = MPI_ANY_TAG ;     // $~~~$ cualquiera
        }


        // 2. recibir un mensaje del emisor o emisores aceptables

        MPI_Recv( &valor, 1, MPI_INT, MPI_ANY_SOURCE, id_emisor_aceptable, MPI_COMM_WORLD, &estado );

        // 3. procesar el mensaje recibido

        switch( estado.MPI_TAG ) // leer emisor del mensaje en metadatos
        {
            case etiq_prod: // si ha sido el productor: insertar en buffer
            if (id_buffer == id_primer_buffer){
                buffer_par[primera_libre_par] = valor ;
                primera_libre_par++;
                num_celdas_ocupadas_par++ ;
                cout << "Buffer par ha recibido valor " << valor << endl ; // Valor par(LIFO)

            }
            else if(id_buffer == id_segundo_buffer){
                buffer_impar[primera_libre_impar] = valor ;
                primera_libre_impar = (primera_libre_impar+1) % tam_vector ;
                num_celdas_ocupadas_impar++ ;
                cout << "Buffer impar ha recibido valor " << valor << endl ; // valor impar(FIFO)
            }
                break;

            case etiq_cons: // si ha sido el consumidor: extraer y enviarle
                if (id_buffer == id_primer_buffer){
                    primera_libre_par--;
                    valor = buffer_par[primera_libre_par];
                    num_celdas_ocupadas_par--;
                    cout << "Buffer par va a enviar valor " << valor << endl ; // valor par(LIFO)
                    MPI_Ssend( &valor, 1, MPI_INT, estado.MPI_SOURCE,etiq_cons , MPI_COMM_WORLD);
                }
                else if(id_buffer == id_segundo_buffer){
                    valor = buffer_impar[primera_ocupada_impar];
                    primera_ocupada_impar = (primera_ocupada_impar+1) % tam_vector ;
                    num_celdas_ocupadas_impar-- ;
                    cout << "Buffer impar va a enviar valor " << valor << endl ; // valor impar (FIFO)
                    MPI_Ssend( &valor, 1, MPI_INT, estado.MPI_SOURCE,etiq_cons , MPI_COMM_WORLD);
                }
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
        if ( id_propio < id_primer_buffer ) // Procesos de productores
            funcion_productor(id_propio);
        else if ( id_propio == id_primer_buffer ) // procesos de buffer
            funcion_buffer(id_primer_buffer); // primer buffer
        else if(id_propio == id_segundo_buffer){
            funcion_buffer(id_segundo_buffer);// segundo buffer
        } else // Procesos de consumidores
            funcion_consumidor(id_propio-id_segundo_buffer-1);
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
