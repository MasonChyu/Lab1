TARGET=my_malloc-driver
CFLAGS=-std=gnu99 -Wno-deprecated-declarations -g

.PHONY: cleanall cleanobj

all: $(TARGET)

my_malloc-driver: my_malloc.o my_malloc-driver.o

my_malloc-driver-instr: ~cs350001/share/Labs/Lab1/my_malloc.o my_malloc-driver.o
	gcc $(CFLAGS) -o my_malloc-driver-instr ~cs350001/share/Labs/Lab1/my_malloc.o my_malloc-driver.o

cleanobj:
	rm -rf my_malloc.o my_malloc-driver.o

clean: cleanobj
	rm -rf $(TARGET) my_malloc-driver-instr
