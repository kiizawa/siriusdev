all: test

test: test.cpp
	g++ -g -I ../lib test.cpp ../lib/object_mover.cpp -lboost_system -lboost_thread -lrados -o test.exe
