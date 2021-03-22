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
    Data data("Indices", H5T_NATIVE_INT, xdata_dimensions, static_cast<void*>(xdata));

    if (restart) {
        /* PRINT OUT 5 ELEMENTS */
        // print out the last 5 elements in the dataset before writing more data
        std::vector<int> xlast_dim = {5};
        std::vector<int> xlast(5);
        Data dxlast("xlast", H5T_NATIVE_INT, xlast_dim, xlast.data());

        // get a hyperslab for the last 5 elements of this dataset
        Dataset dataset = group2.open_dataset("Indices");
        Dataspace hyperslab = dataset.get_space();
        const int num_entries = hyperslab.dimensions()[0];
        const std::vector<int> offset = {num_entries - xlast_dim[0]};
        hyperslab.select_contiguous(offset, xlast_dim);

        // read the hyperslab from the dataset
        dataset.read(hyperslab, dxlast);

        // print out the data
        for (const auto& i : xlast) {
            std::cout << i << std::endl;
        }

        /* TRIM DATASET TO REMOVE LAST MATCHING ENTRY */
        // find the latest entry matching 7
        auto search_fun = [](int i) -> bool {return (i==7);};
        int loc = dataset.search1D<int>(search_fun, true); 
        if (loc >= 0) dataset.set_extent({loc});

        /* APPEND OUR DATA*/
        group2.append(data);
    } else {
        // create a new chunked dataset in this group and write data
        std::vector<int> chunk_dimensions = {5};
        group2.create_dataset(data, chunk_dimensions);
    }

    return 0;
}