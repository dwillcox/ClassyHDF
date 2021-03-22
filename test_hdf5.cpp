#include <iostream>
#include <string>
#include <assert.h>
// #include "ClassyHDF5.H"
#include "hdf5.h"

/* 
 * We're going to create a "file.h5" HDF5 file
 * and save a "Indices" data array into it.
 * 
 * Every time this program is run, we're going to
 * extend this Indices array and then write 15 integers from 0-14.
 */

int main() {
    const std::string filename = "file.h5";
    hid_t file_id;
    hid_t group1_id;
    hid_t group2_id;
    hid_t group3_id;
    herr_t status;
    bool restart = false;

    // We wrap the file & group creation in this HDF5 library try/except
    // block to prevent the library from printing errors to the console
    H5E_BEGIN_TRY
        // with H5F_ACC_EXCL we create a new file and fail if it already exists
        file_id = H5Fcreate(filename.c_str(), H5F_ACC_EXCL, H5P_DEFAULT, H5P_DEFAULT);
        group1_id = H5Gcreate(file_id, "Data", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        group2_id = H5Gcreate(group1_id, "GroupA", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        group3_id = H5Gcreate(group2_id, "GroupB", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    H5E_END_TRY

    if (file_id < 0)
    {
        // the file already existed, so we're going to
        // open it read/write to edit the existing dataset
        file_id = H5Fopen(filename.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
        group1_id = H5Gopen(file_id, "Data", H5P_DEFAULT);
        group2_id = H5Gopen(group1_id, "GroupA", H5P_DEFAULT);
        group3_id = H5Gopen(group2_id, "GroupB", H5P_DEFAULT);
        restart = true;
    }

    // specify the batch size and rank of the dataset we want to work with
    const int batch_size = 15;
    const int data_rank = 1;

    // make some data to write ...
    hsize_t batch_dimensions[1] = {batch_size};
    int data[batch_size];
    for(int i = 0; i < batch_size; ++i) data[i] = i;

    // make a data space for the data we are going to write
    // to the hyperslab
    hid_t memory_dataspace, hyperslab;
    memory_dataspace = H5Screate_simple(data_rank, batch_dimensions, batch_dimensions);

    // the data set and the corresponding data space in the file
    hid_t dataset, dataspace;

    if (restart) {
        // If we're restarting, then open the dataset and figure out what dimensionality
        // it is and what the current size of the dataset is ...
        dataset = H5Dopen(group2_id, "Indices", H5P_DEFAULT);
        dataspace = H5Dget_space(dataset);
        int dim = H5Sget_simple_extent_ndims(dataspace);
        assert(dim == 1);

        hsize_t old_dimensions[1];
        hsize_t max_dimensions[1];
        status = H5Sget_simple_extent_dims(dataspace, old_dimensions, max_dimensions);

        // We're going to write a new batch of data, so extend the dataset to contain it
        hsize_t new_dataset_size[1];
        new_dataset_size[0] = old_dimensions[0] + batch_size;
        // extend makes the dataset size equal to the size requested
        // i.e. we specify the total new size of the dataset,
        // NOT the amount by which to extend!
        status = H5Dset_extent(dataset, new_dataset_size);

        // Now, get the new data space
        hyperslab = H5Dget_space(dataset);

        // And get the hyperslab we're going to write into
        // using old_dimensions as the hyperslab offset.
        // The NULL arguments tell HDF5 to use the default stride of 1 and block size of 1,
        // since we're only writing contiguous data at a given offset with specified dimensions.
        status = H5Sselect_hyperslab(hyperslab, H5S_SELECT_SET, old_dimensions, NULL, batch_dimensions, NULL);
    } else {
        // specify the rank of the dataset we want to write and how many
        // elements it can hold (unlimited) as well as the chunk and fill
        // values we will use later.
        hsize_t dimensions[1];
        hsize_t max_dimensions[1] = {H5S_UNLIMITED};
        hsize_t chunk_dimensions[1];

        dimensions[0] = batch_size;
        chunk_dimensions[0] = 5;
        int fill_value = 0;

        // a data space specifies the rank and dimensionality of the data
        // analogously to an AMReX domain box
        dataspace = H5Screate_simple(data_rank, dimensions, max_dimensions);

        // a data set property list specifies how our data is written
        // including chunking into contigous subsets and compression
        hid_t dataset_creation_parameters;
        dataset_creation_parameters = H5Pcreate(H5P_DATASET_CREATE);
        status = H5Pset_chunk(dataset_creation_parameters, data_rank, chunk_dimensions);
        status = H5Pset_deflate(dataset_creation_parameters, 6);

        // create the dataset using both the data space and data set
        // property list to hold a particular data type
        dataset = H5Dcreate(group2_id, "Indices", H5T_NATIVE_INT, dataspace, H5P_DEFAULT, dataset_creation_parameters, H5P_DEFAULT);

        // extend the dataset to hold our first batch of data
        // set the size of the dataset
        hsize_t dataset_size[1];
        dataset_size[0] = batch_size;
        // extend makes the dataset size equal to the size requested
        // i.e. we specify the total new size of the dataset,
        // NOT the amount by which to extend!
        status = H5Dset_extent(dataset, dataset_size);

        // select a hyperslab, i.e. a subset of the dataset that we
        // are going to write to ... a hyperslab is analogous to a box
        // within an AMReX domain, or a tile.
        //
        // a hyperslab is a DataSpace type that just has particular indices
        // first, get the data space from our dataset to initialize our hyperslab
        hyperslab = H5Dget_space(dataset);

        // second, specify the hyperslab offset and select it from the data space
        hsize_t hyperslab_offset[1];
        hyperslab_offset[0] = 0;
        // the NULL arguments tell HDF5 to use the default stride of 1 and block size of 1,
        // since we're only writing contiguous data at a given offset with specified dimensions.
        status = H5Sselect_hyperslab(hyperslab, H5S_SELECT_SET, hyperslab_offset, NULL, dimensions, NULL);

        // close the dataset creation parameters
        status = H5Pclose(dataset_creation_parameters);
    }

    // write our data to the hyperslab
    status = H5Dwrite(dataset, H5T_NATIVE_INT, memory_dataspace, hyperslab, H5P_DEFAULT, data);

    // close HDF5 object resources to avoid memory leaks
    // (e.g. if this was part of a long-running program)
    status = H5Sclose(memory_dataspace);
    status = H5Dclose(dataset);
    status = H5Sclose(dataspace);
    status = H5Gclose(group3_id);
    status = H5Gclose(group2_id);
    status = H5Gclose(group1_id);
    status = H5Fclose(file_id);

    return 0;
}