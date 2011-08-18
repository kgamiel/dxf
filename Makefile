all: doc

doc::
	cd doc;doxygen

clean::
	rm -rf doc/html
	cd src;make clean

