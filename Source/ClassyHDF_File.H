#ifndef CLASSY_HDF_FILE_H_
#define CLASSY_HDF_FILE_H_

#include <string>
#include <vector>
#include "hdf5.h"

#include "ClassyHDF_Group.H"

namespace ClassyHDF {

namespace FileMode {
    enum {rw=0, trunc};
}

class File : public Location<Group> {
    public:
        File(const std::string& file_name, const int access_type = FileMode::rw) {
            // set our name
            set_name(file_name);

            // try to open the file in read/write mode, otherwise create it
            if (access_type == FileMode::rw) {
                set_existed(true);
                H5E_BEGIN_TRY
                    set_id(H5Fopen(name().c_str(), H5F_ACC_RDWR, H5P_DEFAULT));
                H5E_END_TRY
            }

            if (access_type == FileMode::trunc ||
                (access_type == FileMode::rw && id() < 0)) {
                set_existed(false);
                set_id(H5Fcreate(name().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT));
            }
        }

        ~File() {
            herr_t status = H5Fclose(id());
            assert(status >= 0);
        }

        // declare a move constructor so we ensure the destructor is
        // not called when we return an object of this class by value
        File(File&&) = default;

        // declare a move assignment operator
        File& operator=(File&& other)
        {
            // for move assignments, if we currently own an identity, release it first
            if (initialized() && this != &other) {
                this->~File();
            }

            set_name(other.name());
            if (other.initialized()) set_id(other.id());
            set_existed(other.existed());

            return *this;
        }

        // now, we must forbid copying any class that derives from Identity
        // because either copy could free its HDF5 library resources and
        // leave the other in an invalid state
        //
        // delete the copy constructor
        File(const File& other) = delete;
        //
        // delete the copy assignment operator
        File& operator=(const File& other) = delete;
};

}

#endif