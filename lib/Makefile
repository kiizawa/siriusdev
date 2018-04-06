all: sample_c sample_cpp

sample_c: sample.c
	g++ -g sample.c object_mover.cpp -lboost_system -lboost_thread -lrados -o sample_c.exe

sample_cpp: sample.cpp
	g++ -g sample.cpp object_mover.cpp -lboost_system -lboost_thread -lrados -o sample_cpp.exe
