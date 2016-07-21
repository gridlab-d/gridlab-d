all: example

glproperty.o: glproperty.cpp glproperty.h
	c++ -I../../.. -g -c glproperty.cpp

glengine.o: glengine.cpp glengine.h glproperty.h
	c++ -I../../.. -g -c glengine.cpp -lpthread
	
example: example.cpp glengine.o glproperty.o
	c++ -I../../.. -g example.cpp glengine.o glproperty.o -lpthread -o example

clean:
	rm -rf example *.o

