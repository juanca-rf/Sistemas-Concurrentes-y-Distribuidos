.SUFFIXES:
.PHONY: pc
.PRECIOUS: %_exe

compilador:=g++
Semaphore:= Semaphore.h Semaphore.cpp

t: test-semaforos_exe
	./$< | more

pc: prodcons_exe
	./$<

fu: fumadores_exe
	./$< 

ex: examen_exe
	./$<

prodcons_exe: prodcons-plantilla.cpp $(Semaphore)
	$(compilador) -std=c++11 -pthread -o $@ $< Semaphore.cpp

fumadores_exe: fumadores-plantilla.cpp $(Semaphore)
	$(compilador) -std=c++11 -pthread -o $@ $< Semaphore.cpp

examen_exe: ejercicio1.cpp $(Semaphore)
	$(compilador) -std=c++11 -pthread -o $@ $< Semaphore.cpp
	
%_exe: %.cpp Semaphore.cpp Semaphore.h
		$(compilador) -std=c++11 -pthread -o $@ $< Semaphore.cpp

clean:
	rm -f *_exe
