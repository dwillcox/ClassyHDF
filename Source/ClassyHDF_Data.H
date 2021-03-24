#ifndef CLASSY_HDF_DATA_H_
#define CLASSY_HDF_DATA_H_

#include <string>
#include <vector>
#include "hdf5.h"

#include "ClassyHDF_Dimensions.H"
#include "ClassyHDF_Types.H"
#include "ClassyHDF_Dataspace.H"

namespace ClassyHDF {

class Data : public Dimensions, public DataType {
    private:
        std::string m_name;
        Dataspace m_dataspace;
        void* m_data_ptr;
    public:
        Data(const std::string& name, hid_t datatype,
             const std::vector<int>& dimensions,
             void* data_ptr) : m_dataspace(Dataspace(dimensions)) {
            set_dimensions(dimensions);
            set_datatype(datatype);
            m_data_ptr = data_ptr;
            m_name = name;
        }

        std::string name() const { return m_name; }

        const Dataspace& dataspace() const { return m_dataspace; }

        void* data() const { return m_data_ptr; }
};

}

#endif