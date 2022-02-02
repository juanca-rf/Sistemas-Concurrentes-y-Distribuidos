#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "Semaphore.h"

using namespace std ;
using namespace SEM ;  

const int num_fumadores = 3;

int buff_ingredientes[5] = {};
int i_buff = 0;

Semaphore mostr_vacio = 1,
          buff_lleno = 0,
          buff_vacio = 1,
          ingr_disp[num_fumadores] = {0,0,0};

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

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar

    cout << "   Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar

    cout << "     Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;

}

//-------------------------------------------------------------------------
// Función que simula la acción de producir un ingrediente, como un retardo
// aleatorio de la hebra (devuelve número de ingrediente producido)

int producir_ingrediente()
{
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_produ( aleatorio<10,100>() );

   // informa de que comienza a producir
   cout << "Suministradora : empieza a producir ingrediente (" << duracion_produ.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
   this_thread::sleep_for( duracion_produ );

   const int num_ingrediente = aleatorio<0,num_fumadores-1>() ;

   // informa de que ha terminado de producir
   cout << "Suministradora : termina de producir ingrediente " << num_ingrediente << endl;

   return num_ingrediente ;
}

//----------------------------------------------------------------------
// función que ejecuta la hebra productora

void funcion_suministradora( )
{
   int i;
   while(true){

      if(i_buff>=5)
         sem_wait(buff_lleno);

      i = producir_ingrediente();         //Se adapta para que solo produzca cuando este libre el mostrador
      buff_ingredientes[i_buff] = i;
      i_buff++;
      cout << "Ingrediente producido: " << i << endl;

      sem_signal(buff_vacio);
   }
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero(  )
{
   int i;
   while(true){
      sem_wait(mostr_vacio);
      sem_wait(buff_vacio);

      i = buff_ingredientes[i_buff]; //Se adapta para que solo produzca cuando este libre el mostrador
      i_buff--;  
      sem_signal(buff_lleno); 

      cout << "Ingrediente colocado: " << i << endl;
      sem_signal(ingr_disp[i]);
   }
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( int num_fumador )
{
   while( true )
   {
      sem_wait( ingr_disp[num_fumador] );
      cout << "   Ingrendiente retirado: " << num_fumador << endl;
      sem_signal( mostr_vacio );
      fumar(num_fumador);
   }
}

//----------------------------------------------------------------------

int main()
{
   cout << "--------------------------------------------------------" << endl
        << "Problema de los Fumadores." << endl
        << "--------------------------------------------------------" << endl
        << flush ;

   thread Estanquero ( funcion_hebra_estanquero ),
          Suministradoras[2],
          Fumadores[num_fumadores];

   for(int i = 0; i < 2; i++)
      Suministradoras[i] = thread(funcion_suministradora);

   for(int i = 0; i < num_fumadores; i++)
      Fumadores[i] = thread(funcion_hebra_fumador,i);

   //Nunca termina
   Estanquero.join() ;
    for(int i = 0; i < 2; i++)
      Suministradoras[i].join();
   for(int i = 0; i < num_fumadores; i++)
      Fumadores[i].join();
}
