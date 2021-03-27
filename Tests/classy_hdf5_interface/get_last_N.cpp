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
    File file(filename, FileMode::trunc);
    Group group1 = file.get_group("Data");
    Group group2 = group1.get_group("GroupA");
    
    // specify the size and rank of the data we want to work with
    const int batch_size = 15;

    // make some data to write ...
    std::vector<int> xdata_dimensions = {batch_size};
    int xdata[batch_size];
    for(int i = 0; i < batch_size; ++i) xdata[i] = i;

    // store that data in a Data object
    Data<int> data("Indices", xdata_dimensions, xdata, H5T_NATIVE_INT);

    // create a new chunked dataset in this group and write data
    const int chunk_size = 5;
    group2.create_dataset(data, {chunk_size});
}

bool do_test(const std::string& filename) {
    File file(filename);
    Group group1 = file.get_group("Data");
    Group group2 = group1.get_group("GroupA");

    // get the last 5 elements in the dataset
    std::vector<int> xlast = group2.read_dataset<int>("Indices", {-5}, {5});

    bool success = true;
    success = success && (xlast[0] == 10);
    success = success && (xlast[1] == 21);
    success = success && (xlast[2] == 12);
    success = success && (xlast[3] == 13);
    success = success && (xlast[4] == 14);
    return success;
}

int main() {
    const std::string filename = "file_get_last_N.h5";

    write_test_file(filename);

    if (do_test(filename)) {
        std::cout << "success" << std::endl;
        return 0;
    } else {
        std::cout << "failure" << std::endl;
        return -1;
    }
}