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

void funcionFumador( MRef<Estanco> monitor, int fumador)
{
   int ingrediente;
   while(true){
      ingrediente = monitor->obtenerIngrediente(fumador);
      cout << "El fumador "<<fumador<< " consume: " << ingrediente << endl <<endl;
      fumar(ingrediente);
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
   
   thread Estanquero,
          Fumadores[numFumadores];
          //Añadir hebra contador de cigarrillos que cuenta los cigarrillos de CADA fumador (bucle infinito)

   Estanquero = thread(funcionEstanquero, monitor);
   for(int i = 0; i < numFumadores; i++  )
      Fumadores[i] = thread(funcionFumador, monitor, i);

   //Nunca termina
   Estanquero.join();
   for(int i = 0; i < numFumadores; i++)
      Fumadores[i].join();
}