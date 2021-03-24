#ifndef CLASSY_HDF_TYPES_H_
#define CLASSY_HDF_TYPES_H_

#include <vector>
#include "hdf5.h"
// todo -- add map for C++/HDF types

namespace ClassyHDF {

template<typename T>
hsize_t getSizeH(T s) { return static_cast<hsize_t>(s); }

template<typename T>
std::vector<hsize_t> getSizeVectorH(const std::vector<T>& svect) {
    std::vector<hsize_t> vect;

    for (const auto& s : svect) {
        vect.push_back(static_cast<hsize_t>(s));
    }

    return vect;
}

template<typename T>
int getSizeI(T s) { return static_cast<int>(s); }

template<typename T>
std::vector<int> getSizeVectorI(const std::vector<T>& svect) {
    std::vector<int> vect;

    for (const auto& s : svect) {
        vect.push_back(static_cast<int>(s));
    }

    return vect;
}

class DataType {
    private:
        hid_t m_datatype;
        bool m_owns_type_resource;
    protected:
        void set_datatype(hid_t datatype) {
            m_datatype = datatype;
            m_owns_type_resource = false;
        }

        void lookup_dataset_type(hid_t dataset_id) {
            m_datatype = H5Dget_type(dataset_id);
            m_owns_type_resource = true;
        }
    public:
        DataType() {}

        ~DataType() {
            if (m_owns_type_resource) {
                herr_t status = H5Tclose(m_datatype);
                assert(status >= 0);
            }
        }

        hid_t datatype() const { return m_datatype; }
};

}

#endif