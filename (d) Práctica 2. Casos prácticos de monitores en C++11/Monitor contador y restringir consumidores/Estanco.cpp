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

mutex
   mtx ;                 // mutex de escritura en pantalla

const int numFumadores = 3;

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

int producirIngrediente()
{
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   return aleatorio<0,2>();
}
//----------------------------------------------------------------------

void fumar( int i)
{
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
}

/**
 * @brief Clase para Monitor que contabiliza el numero de cigarros que hace cada fumador
 * 
 */
class Contador : public HoareMonitor{
   public:
   static const int           // constantes:
      num_celdas_total = 20;   //  núm. de entradas del buffer
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
      Contador(  ) ;           // constructor
      int  leer();                // extraer un valor (sentencia L) (consumidor)
      void escribir( int valor ); // insertar un valor (sentencia E) (productor)
      void print_valor();
};

Contador::Contador(){
   producir     = 0 ;
   consumir     = 0 ;
   num_ocupadas = 0;
   ocupadas     = newCondVar();
   libres       = newCondVar();
}

void Contador::escribir(int i){
   // ganar la exclusión mutua del monitor con una guarda
   //unique_lock<mutex> guarda( cerrojo_monitor );

   // esperar bloqueado hasta que num_celdas_ocupadas < num_celdas_total
   if ( num_ocupadas == num_celdas_total )
      libres.wait( /*guarda*/ );

   //cout << "escribir: ocup == " << num_celdas_ocupadas << ", total == " << num_celdas_total << endl ;
   assert( producir < num_celdas_total );

   // hacer la operación de inserción, actualizando estado del monitor
   buffer[producir] = i;
   producir++ ;
   producir %= num_celdas_total;
   num_ocupadas++;

   // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
   ocupadas.signal();
}

int Contador::leer(){
   // ganar la exclusión mutua del monitor con una guarda
   //unique_lock<mutex> guarda( cerrojo_monitor );

   // esperar bloqueado hasta que consumir < producir
   if ( num_ocupadas == 0  )
      ocupadas.wait( );

   // hacer la operación de lectura, actualizando estado del monitor
   //assert( consumir < producir  );
   const int valor = buffer[consumir] ;
   num_ocupadas--;
   consumir++ ;
   consumir %= num_celdas_total;

   // señalar al productor que hay un hueco libre, por si está esperando
   libres.signal();

   // devolver valor
   return valor ;
}

// *****************************************************************************
// clase para monitor buffer, version LIFO, semántica SC, un prod. y un cons.

class Estanco : public HoareMonitor
{
 public:
   int mostrador;
 mutex
   cerrojo_monitor ;        
 CondVar                    
   mostradorVacio,          
   ingrediente[numFumadores];           

 public:                    // constructor y métodos públicos
   Estanco( ) ;           // constructor
   int  obtenerIngrediente( int i);                // extraer un valor (sentencia L) (consumidor)
   void ponerIngrediente( int i ); // insertar un valor (sentencia E) (productor)
   void esperarRecogidaIngrediente();
} ;
// -----------------------------------------------------------------------------

Estanco::Estanco(  )
{
   mostrador         = -1 ;
   mostradorVacio    = newCondVar();
   for( int i = 0; i < numFumadores; i++  ) 
      ingrediente[i] = newCondVar();
}
// -----------------------------------------------------------------------------

// función llamada por el fumador para extraer un ingrediente
int Estanco::obtenerIngrediente( int i )
{
   if(mostrador != i)
      ingrediente[i].wait();
   //mostrador == i
   mostrador = -1; //Saca el ingrediente
   mostradorVacio.signal();
   return i;
}
// -----------------------------------------------------------------------------

//Funcion del estanquero para poner ingrediente en el mostrador
void Estanco::ponerIngrediente( int i )
{
   mostrador = i;
   ingrediente[i].signal();
}

//Funcion para que el estanquero espere a que el mostrador este vacio
void Estanco::esperarRecogidaIngrediente(){
   if(mostrador != -1)
      mostradorVacio.wait();
}

// *****************************************************************************
// funciones de hebras

void funcionEstanquero( MRef<Estanco> monitor )
{
   int ingrediente;
   while(true){
      ingrediente = producirIngrediente();
      cout << "Ingrediente producido:" << ingrediente << endl;
      monitor->ponerIngrediente(ingrediente);
      monitor->esperarRecogidaIngrediente();
   }
}
// -----------------------------------------------------------------------------

void funcionFumador( MRef<Estanco> monitor, int fumador, MRef<Contador> contador )
{
   int ingrediente;
   while(true){
      ingrediente = monitor->obtenerIngrediente(fumador);
      cout << "El fumador "<<fumador<< " consume: " << ingrediente << endl <<endl;
      contador->escribir(fumador);
      fumar(ingrediente);
   }
   
}
// -----------------------------------------------------------------------------
void funcionContadorCigarrillos(MRef<Contador> contador){
   int cuentacigarros[numFumadores] = {};
   while (true){
      for(int i = 0; i < 5; i++)
         cuentacigarros[contador->leer()]++;
         
      cout << "\nContador ##############################################"<<endl;
      for(int i = 0; i < numFumadores; i ++)
         cout << "El fumador " << i << " ha fumado " << cuentacigarros[i] << " cigarros hasta ahora"<< endl;
      cout << "#######################################################\n"<< endl;
   }
}

// -----------------------------------------------------------------------------

int main()
{
   cout << "-------------------------------------------------------------------------------" << endl
        << "Problema Estanquero Fumadores (Monitor SU). " << endl
        << "-------------------------------------------------------------------------------" << endl
        << flush ;

   MRef<Estanco> monitor = Create<Estanco>() ;
   MRef<Contador> contador = Create<Contador>();
   
   thread Estanquero,
          Fumadores[numFumadores],
          contadorCigarrillos;
          //Añadir hebra contador de cigarrillos que cuenta los cigarrillos de CADA fumador (bucle infinito)
          //Comunicacion fumadores - contador con monitor (escribir y leer de ProdCons1SC)
          //El contador cada cinco iteraciones imprime por pantalla cuantos los cigarrillos que lleva cada uno

   Estanquero = thread(funcionEstanquero, monitor);
   for(int i = 0; i < numFumadores; i++  )
      Fumadores[i] = thread(funcionFumador, monitor, i, contador);
   contadorCigarrillos = thread(funcionContadorCigarrillos, contador);

   //Nunca termina
   Estanquero.join();
   for(int i = 0; i < numFumadores; i++)
      Fumadores[i].join();
   contadorCigarrillos.join();
}