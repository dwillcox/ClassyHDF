HDF5_HOME ?= /home/eugene/dev/hdf5/hdf5-1.12.0-cxx
EXTRA_LIBS ?= -lsz -lz -lm

all:
	g++ -o append.exe append.cpp -I../../Source -I$(HDF5_HOME)/include -L$(HDF5_HOME)/lib -lhdf5 $(EXTRA_LIBS)
	g++ -o attributes.exe attributes.cpp -I../../Source -I$(HDF5_HOME)/include -L$(HDF5_HOME)/lib -lhdf5 -lhdf5_hl $(EXTRA_LIBS)
	g++ -o get_last_N.exe get_last_N.cpp -I../../Source -I$(HDF5_HOME)/include -L$(HDF5_HOME)/lib -lhdf5 $(EXTRA_LIBS)
	g++ -o resize.exe resize.cpp -I../../Source -I$(HDF5_HOME)/include -L$(HDF5_HOME)/lib -lhdf5 $(EXTRA_LIBS)
	g++ -o search.exe search.cpp -I../../Source -I$(HDF5_HOME)/include -L$(HDF5_HOME)/lib -lhdf5 $(EXTRA_LIBS)

test: all
	./append.exe
	./attributes.exe
	./get_last_N.exe
	./resize.exe
	./search.exe

clean:
	rm *.exe *.h5
