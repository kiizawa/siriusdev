all: sample test benchmark

sample: sample.cpp
	g++ -g sample.cpp object_mover.cpp -lboost_system -lboost_thread -lrados -o sample.exe

test: test.cpp
	g++ -g test.cpp object_mover.cpp -lboost_system -lboost_thread -lrados -o test.exe

benchmark: benchmark.cpp
	g++ -g benchmark.cpp object_mover.cpp -lboost_system -lboost_thread -lrados -o benchmark.exe
