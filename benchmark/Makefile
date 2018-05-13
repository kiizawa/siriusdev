all: benchmark analyser replayer replayer_single_reader data_placer data_placer_hint

benchmark: benchmark.cpp
	g++ -g -I ../lib benchmark.cpp ../lib/object_mover.cpp -lboost_system -lboost_thread -lrados -o benchmark.exe

analyser:
	g++ -g analyser.cpp -o analyser.exe

replayer: replayer.cpp
	g++ -g -I ../lib replayer.cpp ../lib/object_mover.cpp -lboost_system -lboost_thread -lrados -o replayer.exe

replayer_single_reader: replayer_single_reader.cpp
	g++ -g -I ../lib replayer_single_reader.cpp ../lib/object_mover.cpp -lboost_system -lboost_thread -lrados -o replayer_single_reader.exe

data_placer: data_placer.cpp
	g++ -g data_placer.cpp -o data_placer.exe

data_placer_hint: data_placer_hint.cpp
	g++ -g data_placer_hint.cpp -o data_placer_hint.exe
