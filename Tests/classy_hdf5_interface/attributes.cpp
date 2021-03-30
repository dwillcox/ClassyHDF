#include <iostream>
#include <string>
#include <assert.h>
#include "ClassyHDF.H"
#include "hdf5_hl.h"

using namespace ClassyHDF;

/* 
 * We're going to create a "file.h5" HDF5 file
 * and work with an "Indices" data array in it.
 */

void write_test_file(const std::string& filename) {
    File file(filename, FileMode::trunc);
    Group group1 = file.get_group("Data");
    Group group2 = group1.get_group("GroupA");
    
    // put some attributes into the "GroupA" group
    group2.attr<std::string>("TestAttribute", "Hello World!");
    group2.attr<int>("TestAttInt", 5);
    group2.attr<float>("TestAttFloat", -1.0);
    group2.attr<double>("TestAttDouble", -2.0);
}

bool do_test(const std::string& filename) {
    File file(filename);
    Group group1 = file.get_group("Data");
    Group group2 = group1.get_group("GroupA");

    bool success = true;

    // get some attributes from the "GroupA" group
    std::string stest = group2.attr<std::string>("TestAttribute");
    success = success && (stest == "Hello World!");

    int itest = group2.attr<int>("TestAttInt"); 
    success = success && (itest == 5);

    float ftest = group2.attr<float>("TestAttFloat");
    success = success && (ftest == -1.0);

    double dtest = group2.attr<double>("TestAttDouble");
    success = success && (dtest == -2.0);

    return success;
}

int main() {
    const std::string filename = "file_attributes.h5";

    write_test_file(filename);

    if (do_test(filename)) {
        std::cout << "success" << std::endl;
        return 0;
    } else {
        std::cout << "failure" << std::endl;
        return -1;
    }
}