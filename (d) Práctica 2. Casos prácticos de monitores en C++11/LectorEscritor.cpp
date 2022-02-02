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

const int numLectores = 5;
int lectoresPermitidosEscribiendo = 2;
const int numEscritores = 3;

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

void leer()
{
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

}
//----------------------------------------------------------------------

void escribir()
{
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
}

// *****************************************************************************
// clase para monitor buffer, version LIFO, semántica SC, un prod. y un cons.

class EstrucDatos : public HoareMonitor      //MODIFICACION posible solo tres lectores a la vez cuando hay alguien escribiendo
{
 public:
   int n_lec;
   int bloc;
   bool escrib;
 mutex
   cerrojo_monitor ;        
 CondVar                    
   lectura,          
   escritura;           

 public:                   
   EstrucDatos( ) ;        
   void ini_lectura( );    
   void ini_escritura(  ); 
   void fin_lectura();
   void fin_escritura();
} ;
// -----------------------------------------------------------------------------

EstrucDatos::EstrucDatos(  )
{
   n_lec     = 0;
   bloc      = 0;
   escrib    = false;
   lectura   = newCondVar();
   escritura = newCondVar();
}
// -----------------------------------------------------------------------------

void EstrucDatos::ini_lectura()
{  
   if( escritura.get_nwt()>0 || escrib)
      lectura.wait();
  
   n_lec++;
   lectura.signal();
}

void EstrucDatos::ini_escritura()
{
   if(n_lec>0 || escrib)
      escritura.wait();
   escrib = true;
}

void EstrucDatos::fin_lectura()
{
   n_lec--;
   if(!n_lec) // n_lec == 0
      escritura.signal();
}

void EstrucDatos::fin_escritura()
{
   escrib = false;
   if(!lectura.empty())
      lectura.signal();
   else
      escritura.signal();
}

// *****************************************************************************
// funciones de hebras

void Lector( MRef<EstrucDatos> monitor, int lector )
{
   while(true){
      monitor->ini_lectura();
      mtx.lock();
      cout << "Lector " << lector << " esta leyendo" << endl;
      mtx.unlock();
      leer();
      monitor->fin_lectura();
   }
}
// -----------------------------------------------------------------------------

void Escritor( MRef<EstrucDatos> monitor, int escritor )
{
   while(true){
      monitor->ini_escritura();
      mtx.lock();
      cout << "Escritor " << escritor << " esta escribiendo" << endl;
      mtx.unlock();
      escribir();
      monitor->fin_escritura();
   }
}

// -----------------------------------------------------------------------------

int main()
{
   cout << "-------------------------------------------------------------------------------" << endl
        << "Problema LectorEscritor. " << endl
        << "-------------------------------------------------------------------------------" << endl
        << flush ;

   MRef<EstrucDatos> monitor = Create<EstrucDatos>() ;
   
   thread Escritores[numEscritores],
          Lectores[numLectores];

   for(int i = 0; i < numEscritores; i++  )
      Escritores[i] = thread(Escritor, monitor, i);

   for(int i = 0; i < numLectores; i++  )
      Lectores[i] = thread(Lector, monitor, i);

   //Nunca termina
   for(int i = 0; i < numEscritores; i++)
      Escritores[i].join();
   for(int i = 0; i < numLectores; i++)
      Lectores[i].join();
}