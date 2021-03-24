HDF5_HOME=/home/eugene/dev/hdf5/hdf5-1.12.0-cxx

all:
	g++ -o test_hdf5.exe test_hdf5.cpp -I../../Source -I$(HDF5_HOME)/include -L$(HDF5_HOME)/lib -lhdf5 -lhdf5_cpp -lsz -lz -lm

clean:
	rm *.exe *.h5
