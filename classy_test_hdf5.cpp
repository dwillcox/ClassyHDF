#include <iostream>
#include <string>
#include <assert.h>
#include "ClassyHDF.H"

using namespace ClassyHDF;

/* 
 * We're going to create a "file.h5" HDF5 file
 * and work with an "Indices" data array in it.
 */

int main() {
    const std::string filename = "file.h5";
    bool restart = false;

    File file(filename);
    restart = file.existed();
    Group group1 = file.get_group("Data");
    Group group2 = group1.get_group("GroupA");
    Group group3 = group2.get_group("GroupB");
    
    // specify the size and rank of the data we want to work with
    const int batch_size = 15;

    // make some data to write ...
    std::vector<int> xdata_dimensions = {batch_size};
    int xdata[batch_size];
    for(int i = 0; i < batch_size; ++i) xdata[i] = i;

    // store that data in a Data object
    Data data("Indices", H5T_NATIVE_INT, xdata_dimensions, xdata);

    if (restart) {
        /* PRINT OUT LAST N ELEMENTS */
        {
            // print out the last 5 elements in the dataset
            std::vector<int> xlast = group2.read_dataset<int>("Indices", {-5}, {5});

            for (const auto& i : xlast) {
                std::cout << i << std::endl;
            }
        }

        /* TRIM DATASET TO REMOVE LAST MATCHING ENTRY */
        {
            // we'll want to search & resize the dataset, so use the Dataset object
            Dataset dataset = group2.open_dataset("Indices");

            // find the latest entry matching 7
            auto search_fun = [](int i) -> bool {return (i==7);};
            const bool search_backwards = true;
            int loc = dataset.search1D<int>(search_fun, search_backwards);

            // search1D returns an index, and since in C++ the data is 0-indexed,
            // we can remove the found element and anything past it by setting extent to loc.
            // (set_extent takes a number of elements, not an index)
            if (loc >= 0) dataset.set_extent({loc});
        }

        /* APPEND OUR DATA */
        {
            group2.append(data);
        }
    } else {
        // create a new chunked dataset in this group and write data
        const int chunk_size = 5;
        group2.create_dataset(data, {chunk_size});
    }

    return 0;
}