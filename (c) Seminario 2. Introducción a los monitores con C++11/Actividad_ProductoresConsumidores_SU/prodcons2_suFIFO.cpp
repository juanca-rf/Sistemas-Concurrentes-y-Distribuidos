// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Seminario 2. Introducción a los monitores en C++11.
//
// archivo: prodcons_2_su.cpp
// Ejemplo de un monitor en C++11 con semántica SU, para el problema
// del productor/consumidor, con varios productores y consumidores.
// Opcion FIFO (stack)
//
// Historial:
// Creado en Julio de 2017
// Editado: @author Juan Carlos en 31/10/2020 1:30 am
// 
// -----------------------------------------------------------------------------


#include <iostream>
#include <iomanip>
#include <cassert>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>
#include "HoareMonitor.h"

using namespace std ;
using namespace HM;

constexpr int
   num_items  = 100 ;     // número de items a producir/consumir

mutex
   mtx ;                 // mutex de escritura en pantalla
unsigned
   cont_prod[num_items], // contadores de verificación: producidos
   cont_cons[num_items]; // contadores de verificación: consumidos

int cont = 0;

unsigned const short int
   num_productores   = 5, // Numero de hebras productoras
   num_consumidores  = 5; // Numero de hebras consumidoras

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

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

int producir_dato()
{
   static int contador = 0 ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   cout << "producido: " << contador << endl << flush ;
   mtx.unlock();
   cont_prod[contador] ++ ;
   return contador++ ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato )
{
   if ( num_items <= dato )
   {
      cout << " dato === " << dato << ", num_items == " << num_items << endl ;
      assert( dato < num_items );
   }
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   cout << "                  consumido: " << dato << endl ;
   mtx.unlock();
}
//----------------------------------------------------------------------

void ini_contadores()
{
   for( unsigned i = 0 ; i < num_items ; i++ )
   {  cont_prod[i] = 0 ;
      cont_cons[i] = 0 ;
   }
}

//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." << flush ;

   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      if ( cont_prod[i] != 1 )
      {
         cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {
         cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

// *****************************************************************************
// clase para monitor buffer, version LIFO, semántica SC, un prod. y un cons.

class ProdCons1SC : public HoareMonitor
{
 public:
 static const int           // constantes:
   num_celdas_total = 10;   //  núm. de entradas del buffer
 int                        // variables permanentes
   buffer[num_celdas_total],//  buffer de tamaño fijo, con los datos
   producir ,               //  indice de celda de la próxima inserción
   consumir ,               //  indice de celda de la próxima consumicion
   num_ocupadas;
 mutex
   cerrojo_monitor ;        // cerrojo del monitor
 CondVar                    // colas condicion SU:
   ocupadas,                //  cola donde espera el consumidor (n>0)
   libres ;                 //  cola donde espera el productor  (n<num_celdas_total)

 public:                    // constructor y métodos públicos
   ProdCons1SC(  ) ;           // constructor
   int  leer();                // extraer un valor (sentencia L) (consumidor)
   void escribir( int valor ); // insertar un valor (sentencia E) (productor)
   void print_valor();
} ;
// -----------------------------------------------------------------------------

ProdCons1SC::ProdCons1SC(  )
{
   producir     = 0 ;
   consumir     = 0 ;
   num_ocupadas = 0;
   ocupadas     = newCondVar();
   libres       = newCondVar();
}
// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

int ProdCons1SC::leer(  )
{
   // ganar la exclusión mutua del monitor con una guarda
   //unique_lock<mutex> guarda( cerrojo_monitor );

   // esperar bloqueado hasta que consumir < producir
   if ( consumir == producir )
      ocupadas.wait( );

   // hacer la operación de lectura, actualizando estado del monitor
   //assert( consumir < producir  );
   const int valor = buffer[consumir] ;
   consumir++ ;
   consumir %= num_celdas_total;


   // señalar al productor que hay un hueco libre, por si está esperando
   libres.signal();

   // devolver valor
   return valor ;
}
// -----------------------------------------------------------------------------

void ProdCons1SC::escribir( int valor )
{
   // ganar la exclusión mutua del monitor con una guarda
   //unique_lock<mutex> guarda( cerrojo_monitor );

   // esperar bloqueado hasta que num_celdas_ocupadas < num_celdas_total
   if ( producir == num_celdas_total )
      libres.wait( /*guarda*/ );

   //cout << "escribir: ocup == " << num_celdas_ocupadas << ", total == " << num_celdas_total << endl ;
   assert( producir < num_celdas_total );

   // hacer la operación de inserción, actualizando estado del monitor
   buffer[producir] = valor ;
   producir++ ;
   producir %= num_celdas_total;

   // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
   ocupadas.signal();
}
void print_valor(ProdCons1SC * monitor);
// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora( MRef<ProdCons1SC> monitor )
{
   for( unsigned i = 0 ; i < (num_items/num_productores) ; i++ ) //Datos compartidos a producir entre todas las hebras productoras
   {  
      int valor = producir_dato() ;
      monitor->escribir( valor );
      //monitor->print_valor();
   }
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora( MRef<ProdCons1SC> monitor )
{
   for( unsigned i = 0 ; i < (num_items/num_consumidores) ; i++ ) //Datos compartidos a consumir entre todas las hebras consumidoras
   {
      int valor = monitor->leer();
      consumir_dato( valor ) ;
      //monitor->print_valor();
   }
}
// -----------------------------------------------------------------------------

// Para propositos de debugging
void ProdCons1SC::print_valor(){
   cout << "\nValor dee Producir "<< producir <<"\n"
        << "\nValor dee Consumir " << consumir << endl;
}

int main()
{
   cout << "-------------------------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (1 prod/cons, Monitor SC, buffer FIFO). " << endl
        << "-------------------------------------------------------------------------------" << endl
        << flush ;

   MRef<ProdCons1SC> monitor = Create<ProdCons1SC>() ;
   
   thread productoras[num_productores],
         consumidoras[num_consumidores];

   for(int i= 0; i< num_productores; i++)
      productoras[i] = thread(funcion_hebra_productora, monitor);
   
   for(int i= 0; i<num_consumidores; i++)
      consumidoras[i] = thread(funcion_hebra_consumidora, monitor);

   for(int i= 0; i< num_productores; i++)
      productoras[i].join();
   
   for(int i= 0; i<num_consumidores; i++)
      consumidoras[i].join();

   // comprobar que cada item se ha producido y consumido exactamente una vez
   test_contadores() ;
}
