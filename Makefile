CXX=$(shell which g++)


all: bin2elfo nelfo
clean: clean-test
	rm -rf bin2elfo
	rm -rf nelfo

clean-test:
	rm -rf test.o
	rm -rf test.log
	rm -rf test2.o
	rm -rf test2.log

bin2elfo: bin2elfo.cpp
	g++ -o bin2elfo bin2elfo.cpp

nelfo: nelfo.cpp
	g++ -o nelfo nelfo.cpp

test: bin2elfo nelfo
	./bin2elfo bin2elfo.cpp test.o
	./nelfo test.asm test2.o
	
	readelf -a test.o > test.log
	objdump -t test.o >> test.log
	objdump -s test.o >> test.log
	objdump -D test.o >> test.log

	readelf -a test2.o > test2.log
	objdump -t test2.o >> test2.log
	objdump -s test2.o >> test2.log
	objdump -D test2.o >> test2.log

