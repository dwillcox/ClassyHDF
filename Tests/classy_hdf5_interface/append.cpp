#include <iostream>
#include <string>
#include <assert.h>
#include "ClassyHDF.H"

using namespace ClassyHDF;

/* 
 * We're going to create a "file.h5" HDF5 file
 * and work with an "Indices" data array in it.
 */

void write_test_file(const std::string& filename) {
    File file(filename);
    Group group1 = file.get_group("Data");
    Group group2 = group1.get_group("GroupA");
    
    // specify the size and rank of the data we want to work with
    const int batch_size = 15;

    // make some data to write ...
    std::vector<int> xdata_dimensions = {batch_size};
    int xdata[batch_size];
    for(int i = 0; i < batch_size; ++i) xdata[i] = i;

    // store that data in a Data object
    Data data("Indices", H5T_NATIVE_INT, xdata_dimensions, xdata);

    // create a new chunked dataset in this group and write data
    const int chunk_size = 5;
    group2.create_dataset(data, {chunk_size});
}

void do_append(const std::string& filename) {
    File file(filename);
    Group group1 = file.get_group("Data");
    Group group2 = group1.get_group("GroupA");

    // specify the size and rank of the data we want to work with
    const int batch_size = 15;

    // make some data to write ...
    int xdata[batch_size];
    for(int i = 0; i < batch_size; ++i) xdata[i] = i;

    // store that data in a Data object
    Data data("Indices", H5T_NATIVE_INT, {batch_size}, xdata);

    // append data to the dataset
    group2.append(data);
}

bool do_test(const std::string& filename) {
    File file(filename);
    Group group1 = file.get_group("Data");
    Group group2 = group1.get_group("GroupA");

    // get the data in the dataset
    std::vector<int> xlast = group2.read_dataset<int>("Indices");

    bool success = true;
    for (int i = 0; i < 30; ++i) {
        if (i < 15) {
            success = success && (xlast[i] == i);
        } else {
            success = success && (xlast[i] == i-15);
        }
    }
    return success;
}

int main() {
    const std::string filename = "file_append.h5";

    write_test_file(filename);

    do_append(filename);

    if (do_test(filename)) std::cout << "success" << std::endl;
    else std::cout << "failure" << std::endl;

    return 0;
}