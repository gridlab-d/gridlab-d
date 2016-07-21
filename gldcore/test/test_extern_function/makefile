
../../autotest/extern_test.so: myfunction.o
	libtool -dynamic -o $@ $<

clean:
	rm myfunction.o ../../autotest/extern_test.so

myfunction.o: myfunction.c
