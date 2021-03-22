#include <iostream>
#include <string>
#include <assert.h>
#include "ClassyHDF.H"

using namespace ClassyHDF;

/* 
 * We're going to create a "file.h5" HDF5 file
 * and save a "Indices" data array into it.
 * 
 * Every time this program is run, we're going to
 * extend this Indices array and then write 15 integers from 0-14.
 */

int main() {
    const std::string filename = "file.h5";

    File file(filename);
    Group group1 = file.get_group("Data");
    Group group2 = group1.get_group("GroupA");
    Group group3 = group2.get_group("GroupB");
    
    // specify the size and rank of the data we want to work with
    const int new_size = 10;

    // open the dataset
    Dataset dataset = group2.open_dataset("Indices");

    // resize the dataset to the new size
    dataset.set_extent({new_size});

    return 0;
}