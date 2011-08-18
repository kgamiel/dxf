all: doc lib

doc::
	cd doc;doxygen

lib::
	cd src;make

clean::
	rm -rf doc/html
	cd src;make clean

