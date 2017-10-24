output: ./main.o
	g++ ./main.o -o ./timesync -lrt

./bin/main.o: main.cpp
	g++ -O2 -c main.cpp -o ./bin/main.o

clean: rm ./*.o

