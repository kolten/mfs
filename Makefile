mfs:
	gcc mfs.c -o mfs

class:
	gcc class.c -o class

clean:
	rm mfs

compile:
	make clean && make mfs