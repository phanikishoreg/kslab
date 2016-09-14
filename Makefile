all:
	make -C src all
	make -C test all

clean:
	make -C src clean
	make -C test clean
