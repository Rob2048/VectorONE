build_folder := $(shell mkdir -p bin)

output: ./bin/main.o ./bin/precomp.o
	g++ ./bin/main.o ./bin/precomp.o -o ./bin/pixel -lrt

./bin/main.o: main.cpp precomp.cpp
	g++ -O3 -c main.cpp -o ./bin/main.o

./bin/precomp.o: precomp.cpp
	g++ -O3 -c precomp.cpp -o ./bin/precomp.o

clean: rm ./bin/*.o

