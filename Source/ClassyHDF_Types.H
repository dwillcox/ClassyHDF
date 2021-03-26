#ifndef CLASSY_HDF_TYPES_H_
#define CLASSY_HDF_TYPES_H_

#include <vector>
#include "hdf5.h"

namespace ClassyHDF {

    /* make a templated C++/HDF type mapping */

    // generate a compile-time error for nonspecialized
    // C++ types to avoid using an unintended HDF5 type
    // relying on the fact that sizeof is never 0 for a
    // pointer to any type
    template<typename T>
    hid_t CppTypeToHDF() { static_assert(!sizeof(T*), "No HDF type mapped to this C++ type."); return 0; };

    // map char to H5T_NATIVE_CHAR
    template<>
    hid_t CppTypeToHDF<char>() { return H5T_NATIVE_CHAR; };

    // map int to H5T_NATIVE_INT
    template<>
    hid_t CppTypeToHDF<int>() { return H5T_NATIVE_INT; };

    // map float to H5T_NATIVE_FLOAT
    template<>
    hid_t CppTypeToHDF<float>() { return H5T_NATIVE_FLOAT; };

    // map double to H5T_NATIVE_DOUBLE
    template<>
    hid_t CppTypeToHDF<double>() { return H5T_NATIVE_DOUBLE; };

}

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

        bool is_equal_datatype(const hid_t& other_type) {
            return static_cast<bool>(H5Tequal(datatype(), other_type));
        }
};

}

#endif