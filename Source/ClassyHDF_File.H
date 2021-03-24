#ifndef CLASSY_HDF_FILE_H_
#define CLASSY_HDF_FILE_H_

#include <string>
#include <vector>
#include "hdf5.h"

#include "ClassyHDF_Group.H"

namespace ClassyHDF {

class File : public Location<Group> {
    public:
        // declare a move constructor so we ensure the destructor is
        // not called when we return an object of this class by value
        File(File&&) = default;

        File(const std::string& file_name) {
            // set our name
            set_name(file_name);

            // try to open the file in read/write mode, otherwise create it
            set_existed(true);
            H5E_BEGIN_TRY
                set_id(H5Fopen(name().c_str(), H5F_ACC_RDWR, H5P_DEFAULT));
            H5E_END_TRY

            if (id() < 0) {
                set_existed(false);
                set_id(H5Fcreate(name().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT));
            }
        }

        ~File() {
            herr_t status = H5Fclose(id());
            assert(status >= 0);
        }
};

}

#endif