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
    herr_t status = H5LTset_attribute_string(group1.id(), "GroupA", "TestAttribute", "Hello World!");
    assert(status >= 0);

    int* ibuf = new int[1];
    ibuf[0] = 5;
    status = H5LTset_attribute_int(group1.id(), "GroupA", "TestAttInt", ibuf, 1);
    delete ibuf;

    float* fbuf = new float[1];
    fbuf[0] = -1.0;
    status = H5LTset_attribute_float(group1.id(), "GroupA", "TestAttFloat", fbuf, 1);
    delete fbuf;

    double* dbuf = new double[1];
    dbuf[0] = -2.0;
    status = H5LTset_attribute_double(group1.id(), "GroupA", "TestAttDouble", dbuf, 1);
    delete dbuf;
}

bool do_test(const std::string& filename) {
    File file(filename);
    Group group1 = file.get_group("Data");
    Group group2 = group1.get_group("GroupA");

    bool success = true;

    // get some attributes from the "GroupA" group
    herr_t status;

    char* buffer;
    int size_buffer;
    status = H5LTget_attribute_ndims(group1.id(), "GroupA", "TestAttribute", &size_buffer);
    buffer = new char[size_buffer];
    status = H5LTget_attribute_string(group1.id(), "GroupA", "TestAttribute", buffer);
    std::string sbuffer = buffer;
    success = success && (sbuffer == "Hello World!");
    delete buffer;

    int* ibuf = new int[1];
    ibuf[0] = 0;
    status = H5LTget_attribute_int(group1.id(), "GroupA", "TestAttInt", ibuf);
    success = success && (ibuf[0] == 5);
    delete ibuf;

    float* fbuf = new float[1];
    fbuf[0] = 0.0;
    status = H5LTget_attribute_float(group1.id(), "GroupA", "TestAttFloat", fbuf);
    success = success && (fbuf[0] == -1.0);
    delete fbuf;

    double* dbuf = new double[1];
    dbuf[0] = 0.0;
    status = H5LTget_attribute_double(group1.id(), "GroupA", "TestAttDouble", dbuf);
    success = success && (dbuf[0] == -2.0);
    delete dbuf;

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