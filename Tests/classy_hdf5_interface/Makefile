HDF5_HOME=/home/eugene/dev/hdf5/hdf5-1.12.0-cxx

all:
	g++ -o append.exe append.cpp -I../../Source -I$(HDF5_HOME)/include -L$(HDF5_HOME)/lib -lhdf5 -lhdf5_cpp -lsz -lz -lm
	g++ -o get_last_N.exe get_last_N.cpp -I../../Source -I$(HDF5_HOME)/include -L$(HDF5_HOME)/lib -lhdf5 -lhdf5_cpp -lsz -lz -lm
	g++ -o resize.exe resize.cpp -I../../Source -I$(HDF5_HOME)/include -L$(HDF5_HOME)/lib -lhdf5 -lhdf5_cpp -lsz -lz -lm
	g++ -o search.exe search.cpp -I../../Source -I$(HDF5_HOME)/include -L$(HDF5_HOME)/lib -lhdf5 -lhdf5_cpp -lsz -lz -lm

test: all
	./append.exe && ./get_last_N.exe && ./resize.exe && ./search.exe

clean:
	rm *.exe *.h5
