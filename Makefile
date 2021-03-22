HDF5_HOME=/home/eugene/dev/hdf5/hdf5-1.12.0-cxx

all:
	g++ test_hdf5.cpp -I$(HDF5_HOME)/include -L$(HDF5_HOME)/lib -lhdf5 -lhdf5_cpp -lsz -lz -lm

classy:
	g++ classy_test_hdf5.cpp -I$(HDF5_HOME)/include -L$(HDF5_HOME)/lib -lhdf5 -lhdf5_cpp -lsz -lz -lm

resize:
	g++ resize.cpp -I$(HDF5_HOME)/include -L$(HDF5_HOME)/lib -lhdf5 -lhdf5_cpp -lsz -lz -lm

clean:
	rm a.out
