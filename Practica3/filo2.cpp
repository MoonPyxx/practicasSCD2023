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

void funcion_filosofos( int id, int filosofo_dos_sitios )
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

void funcion_camarero(int filosofo_dos_sitios)
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
        if (estado.MPI_SOURCE == filosofo_dos_sitios){
            if (estado.MPI_TAG == etiq_sentarse && filosofos_sentados <= max_filo_sentados - 2){
                filosofos_sentados += 2;
                cout << "Camarero permite al filosofo " << estado.MPI_SOURCE << " sentarse en la mesa (ocupa dos sitios)." << endl;
            }
            if (estado.MPI_TAG == etiq_levantarse){
                filosofos_sentados -=2;
                cout << "Camarero permite al filosofo " << estado.MPI_SOURCE << " levantarse de la mesa(ocupaba dos sitios)." << endl;
            }
        } else {
            if (estado.MPI_TAG == etiq_sentarse) {
                filosofos_sentados++;
                cout << "Camarero permite al filosofo " << estado.MPI_SOURCE << " sentarse en la mesa." << endl;
            }
            if (estado.MPI_TAG == etiq_levantarse) {
                filosofos_sentados--;
                cout << "Camarero permite al filosofo " << estado.MPI_SOURCE << " levantarse de la mesa." << endl;
            }
        }
    }
}

// ---------------------------------------------------------------------

int main( int argc, char** argv )
{
    int id_propio, num_procesos_actual ;
    int filosofo_dos_sitios = aleatorio<0,num_filosofos-1>();
    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
    MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );


    if ( num_procesos == num_procesos_actual )
    {
        if (id_propio == id_camarero)
            funcion_camarero(filosofo_dos_sitios);
            // ejecutar la función correspondiente a 'id_propio'
        else if ( id_propio % 2 == 0 )          // si es par
            funcion_filosofos( id_propio, filosofo_dos_sitios ); //   es un filósofo
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
