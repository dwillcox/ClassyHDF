#ifndef CLASSY_HDF_GROUP_H_
#define CLASSY_HDF_GROUP_H_

#include <string>
#include <vector>
#include "hdf5.h"

#include "ClassyHDF_Location.H"

namespace ClassyHDF {

class Group : public Location<Group> {
    public:
        // declare a move constructor so we ensure the destructor is
        // not called when we return an object of this class by value
        Group(Group&&) = default;

        Group(const Location& location, const std::string& group_name) {
            // set our name
            set_name(group_name);

            // try to open the group in the given location, otherwise create it
            set_existed(true);
            H5E_BEGIN_TRY
                set_id(H5Gopen(location.id(), name().c_str(), H5P_DEFAULT));
            H5E_END_TRY

            if (id() < 0) {
                set_existed(false);
                set_id(H5Gcreate(location.id(), name().c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT));
            }
        }

        ~Group() {
            herr_t status = H5Gclose(id());
            assert(status >= 0);
        }
};

}

#endif