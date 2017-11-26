all: mesa-unpacker mesa-packer libmesa.so mesa.o

mesastor-cli: mesator-cli.o mesastor.o
	gcc -std=c99 -ggdb packer.o -o mesa-packer
mesastor-cli.o: mesastor-cli.c
	gcc -std=c99 -ggdb -c mesastor-cli.c -o mesastor-cli.o
unpacker.o: unpacker.c
	gcc -std=c99 -ggdb -c unpacker.c -o unpacker.o
libmesa.so: mesa.o
	ar rc libmesa.so mesa.o
mesa.o: mesa.c
	gcc -std=c99 -fPIC -c mesa.c -o mesa.o
clean:
	rm -f mesa-unpacker mesa-packer libmesa.so packer.o unpacker.o mesa.o
