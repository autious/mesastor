all: mesa-unpacker mesa-packer libmesa.so mesa.o

mesa-unpacker: unpacker.o mesa.o
	gcc -std=c99 -ggdb unpacker.o mesa.o -o mesa-unpacker
mesa-packer: packer.o mesa.o
	gcc -std=c99 -ggdb packer.o -o mesa-packer
packer.o: packer.c
	gcc -std=c99 -ggdb -c packer.c -o packer.o
unpacker.o: unpacker.c
	gcc -std=c99 -ggdb -c unpacker.c -o unpacker.o
libmesa.so: mesa.o
	ar rc libmesa.so mesa.o
mesa.o: mesa.c
	gcc -std=c99 -fPIC -c mesa.c -o mesa.o
clean:
	rm -f mesa-unpacker mesa-packer libmesa.so packer.o unpacker.o mesa.o
