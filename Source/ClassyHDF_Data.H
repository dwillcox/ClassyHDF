#ifndef CLASSY_HDF_DATA_H_
#define CLASSY_HDF_DATA_H_

#include <string>
#include <vector>
#include "hdf5.h"

#include "ClassyHDF_Dimensions.H"
#include "ClassyHDF_Types.H"
#include "ClassyHDF_Dataspace.H"

namespace ClassyHDF {

template<typename T>
class Data : public Dimensions, public DataType {
    private:
        std::string m_name;
        Dataspace m_dataspace;
        void* m_data_ptr;

        // in cases where we initialize Data with a temporary vector
        // we take ownership of the data internally
        std::vector<T> m_datavec;
        const bool m_owns_data;
    public:
        Data(const std::string& name,
             const std::vector<int>& dimensions,
             void* data_ptr,
             const hid_t hdf_data_type = CppTypeToHDF<T>())
             : m_dataspace(Dataspace(dimensions)),
               m_owns_data(false)
        {
            m_name = name;
            set_dimensions(dimensions);
            set_datatype(hdf_data_type);
            m_data_ptr = data_ptr;
        }

        Data(const std::string& name,
             std::vector<T>& datavec,
             const hid_t hdf_data_type = CppTypeToHDF<T>())
             : m_dataspace(Dataspace({static_cast<int>(datavec.size())})),
               m_owns_data(false)
        {
            m_name = name;
            set_dimensions({datavec.size()});
            set_datatype(hdf_data_type);
            m_data_ptr = datavec.data();
        }

        Data(const std::string& name,
             std::vector<T>&& datavec,
             const hid_t hdf_data_type = CppTypeToHDF<T>())
             : m_dataspace(Dataspace({static_cast<int>(datavec.size())})),
               m_owns_data(true)
        {
            m_name = name;
            set_dimensions({static_cast<int>(datavec.size())});
            set_datatype(hdf_data_type);

            // the datavec passed was an rvalue, i.e. a temporary
            // so we should take ownership of the vector
            m_datavec = datavec;
            m_data_ptr = m_datavec.data();
        }

        std::string name() const { return m_name; }

        const Dataspace& dataspace() const { return m_dataspace; }

        void* data() const { return m_data_ptr; }

        bool owns_data() const { return m_owns_data; }
};

}

#endif