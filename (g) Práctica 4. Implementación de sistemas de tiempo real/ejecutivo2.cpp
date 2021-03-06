// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 4. Implementación de sistemas de tiempo real.
//
// Archivo: ejecutivo2.cpp
// Implementación del segundo ejemplo de ejecutivo cíclico:
//
//   Datos de las tareas:
//   ------------
//   Ta.  T    C
//   ------------
//   A  500    100
//   B  500    150
//   C  1000   200
//   D  2000   240
//  -------------
//
//  Planificación (con Ts == 500 ms)
//  *---------*----------*---------*--------*
//  | A B C   | A B D    | A B C   | A B    |
//  *---------*----------*---------*--------*
//  0       500         1000     1500      2000
//
//  Preguntas
//  
//  ** El minimo tiempo de espera sera en el segundo ciclo secundario ( 10 ) siendo nuestro Ts 500 - A+B+D => 500 - 490 = 10 
// 
//  ** Si, teoricamente el tiempo de espera ocupando D 250 seria cero, entraria justo en el tiempo secundario.
//     Probando durante varios ciclos principales ( 2000 ) se adecua al resultado teórico.
//     De hecho, mientras estoy escribiendo esto sigue ejecutandose sin problemas. 
// 
// Historial:
// Creado en Diciembre de 2017
// -----------------------------------------------------------------------------

#include <string>
#include <iostream> // cout, cerr
#include <thread>
#include <chrono>   // utilidades de tiempo
#include <ratio>    // std::ratio_divide

using namespace std ;
using namespace std::chrono ;
using namespace std::this_thread ;

// tipo para duraciones en segundos y milisegundos, en coma flotante:
typedef duration<float,ratio<1,1>>    seconds_f ;
typedef duration<float,ratio<1,1000>> milliseconds_f ;

// -----------------------------------------------------------------------------
// tarea genérica: duerme durante un intervalo de tiempo (de determinada duración)

void Tarea( const std::string & nombre, milliseconds tcomputo )
{
   cout << "   Comienza tarea " << nombre << " (C == " << tcomputo.count() << " ms.) ... " ;
   sleep_for( tcomputo );
   cout << "fin." << endl ;
}

// -----------------------------------------------------------------------------
// tareas concretas del problema:

void TareaA() { Tarea( "A", milliseconds( 100 ) );  }
void TareaB() { Tarea( "B", milliseconds( 150 ) );  }
void TareaC() { Tarea( "C", milliseconds( 200 ) );  }
void TareaD() { Tarea( "D", milliseconds( 240 ) );  }


// -----------------------------------------------------------------------------
// implementación del ejecutivo cíclico:

int main( int argc, char *argv[] )
{
   // Ts = duración del ciclo secundario
   const milliseconds Ts( 500 );

   //limite de retardo
   const milliseconds limite( 20 );

   // ini_sec = instante de inicio de la iteración actual del ciclo secundario
   time_point<steady_clock> ini_sec = steady_clock::now();
   time_point<steady_clock> sec_fin;
   steady_clock::duration seconds_f ;


   while( true ) // ciclo principal
   {
      cout << endl
           << "---------------------------------------" << endl
           << "Comienza iteración del ciclo principal." << endl ;

      for( int i = 1 ; i <= 4 ; i++ ) // ciclo secundario (4 iteraciones)
      {
         cout << endl << "Comienza iteración " << i << " del ciclo secundario." << endl ;

         switch( i )
         {
            case 1 : TareaA(); TareaB(); TareaC();           break ;
            case 2 : TareaA(); TareaB(); TareaD();           break ;
            case 3 : TareaA(); TareaB(); TareaC();           break ;
            case 4 : TareaA(); TareaB();                     break ;
         }

         // calcular el siguiente instante de inicio del ciclo secundario
         ini_sec += Ts ;


         // esperar hasta el inicio de la siguiente iteración del ciclo secundario
         sleep_until( ini_sec );

         //comprobacion
         sec_fin = steady_clock::now();
         seconds_f = sec_fin - ini_sec;
         if( seconds_f > limite ){
            cout << "\t\t\tLIMITE EXCEDIDO" << endl;
            exit(1);
         }
      }
   }
}
