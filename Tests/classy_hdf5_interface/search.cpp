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

    return (loc == 7);
}

int main() {
    const std::string filename = "file_search.h5";

    write_test_file(filename);

    if (do_test(filename)) std::cout << "success" << std::endl;
    else std::cout << "failure" << std::endl;

    return 0;
}