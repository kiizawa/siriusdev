all: benchmark analyser replayer

benchmark: benchmark.cpp
	g++ -g -I ../lib benchmark.cpp ../lib/object_mover.cpp -lboost_system -lboost_thread -lrados -o benchmark.exe

analyser:
	g++ -g analyser.cpp -o analyser.exe

replayer: replayer.cpp
	g++ -g -I ../lib replayer.cpp ../lib/object_mover.cpp -lboost_system -lboost_thread -lrados -o replayer.exe

data_placer: data_placer.cpp
	g++ -g data_placer.cpp -o data_placer.exe
