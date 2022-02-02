#include <mpi.h>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <iostream>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

int etiq_pago = 0,
    etiq_compra = 1,
    etiq_caja = 2,
    id_cajero = 10;

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

void funcion(){ sleep_for( milliseconds( aleatorio<10,200>()) ); }

void intermedio(){
    int cajas_ocupadas = 0;
    int valor;
    MPI_Status  estado ;
    while(true){

        if(cajas_ocupadas < 2) //no haya mas de tres
            MPI_Recv ( &valor, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &estado );
        else
            MPI_Recv ( &valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_compra, MPI_COMM_WORLD, &estado );
                
        if( estado.MPI_TAG == etiq_pago ){
            cajas_ocupadas++;
            MPI_Ssend( &valor, 1, MPI_INT, estado.MPI_SOURCE, etiq_caja, MPI_COMM_WORLD);
        }    
        if( estado.MPI_TAG == etiq_compra ){
            cajas_ocupadas--;
        }
        

        
    }
}

void clientes(int id){
    int valor;
    MPI_Status  estado ;
    while(true){
        funcion(); //compra

        cout << "Cliente " << valor << " solicita pago " << endl ;
        MPI_Ssend( &valor, 1, MPI_INT, id_cajero, etiq_pago, MPI_COMM_WORLD);
        MPI_Recv ( &valor, 1, MPI_INT, id_cajero, etiq_caja, MPI_COMM_WORLD, &estado );

        funcion(); //pago en la caja
        MPI_Ssend( &valor, 1, MPI_INT, id_cajero, etiq_compra, MPI_COMM_WORLD); //compra completa
        
        funcion(); //gastar comprado
    }
}



int main(){
    int id_propio, num_procesos_actual; // ident. propio, núm. de procesos

    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
    MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );


}