mfs:
	g++ -o mfs mfs.c

class:
	gcc class.c -o class

clean:
	rm mfs

compile:
	make clean && make mfs