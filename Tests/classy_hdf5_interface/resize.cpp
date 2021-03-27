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

void do_resize(const std::string& filename) {
    File file(filename);
    Group group1 = file.get_group("Data");
    Group group2 = group1.get_group("GroupA");

    // specify the size and rank of the data we want to work with
    const int new_size = 10;

    // open the dataset
    Dataset dataset = group2.open_dataset("Indices");

    // resize the dataset to the new size
    dataset.set_extent({new_size});
}

bool do_test(const std::string& filename) {
    File file(filename);
    Group group1 = file.get_group("Data");
    Group group2 = group1.get_group("GroupA");

    Dataset data = group2.open_dataset("Indices");

    const auto xdata = data.read<int>();

    bool success = true;
    for (int i = 0; i < 10; ++i) {
        success = success && (xdata[i] == i);
    }
    success = success && (xdata.size() == 10);

    return success;
}

int main() {
    const std::string filename = "file_resize.h5";

    write_test_file(filename);

    do_resize(filename);

    if (do_test(filename)) {
        std::cout << "success" << std::endl;
        return 0;
    } else {
        std::cout << "failure" << std::endl;
        return -1;
    }
}