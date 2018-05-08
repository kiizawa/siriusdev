all: sample_c sample_cpp

sample_c: sample.c
	gcc -g sample.c object_mover_c.cpp object_mover.cpp -lstdc++ -lboost_system -lboost_thread -lrados -o sample_c.exe

sample_cpp: sample.cpp
	g++ -g sample.cpp object_mover.cpp -lboost_system -lboost_thread -lrados -o sample_cpp.exe
