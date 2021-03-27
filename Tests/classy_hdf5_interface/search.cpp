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
    Data<int> data("Indices", xdata_dimensions, xdata);

    // create a new chunked dataset in this group and write data
    const int chunk_size = 5;
    group2.create_dataset(data, {chunk_size});
}

bool do_test(const std::string& filename) {
    File file(filename);
    Group group1 = file.get_group("Data");
    Group group2 = group1.get_group("GroupA");

    // we'll want to search & resize the dataset, so use the Dataset object
    Dataset dataset = group2.open_dataset("Indices");

    // find the latest entry matching 7
    auto search_fun = [](int i) -> bool {return (i==7);};
    const bool search_backwards = true;
    int loc = dataset.search<int>(search_fun, search_backwards);

    // resize the dataset to truncate at the found location
    // this removes the found entry and all following entries
    //
    // Note: set_extent() takes a number of elements in the new dataset,
    // so we can pass loc since it is a 0-based index of the first
    // element to be truncated.
    dataset.set_extent({loc});

    return (loc == 7);
}

int main() {
    const std::string filename = "file_search.h5";

    write_test_file(filename);

    if (do_test(filename)) {
        std::cout << "success" << std::endl;
        return 0;
    } else {
        std::cout << "failure" << std::endl;
        return -1;
    }
}