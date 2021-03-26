#ifndef CLASSY_HDF_LOCATION_H_
#define CLASSY_HDF_LOCATION_H_

#include <string>
#include <vector>
#include "hdf5.h"

#include "ClassyHDF_Identity.H"
#include "ClassyHDF_Data.H"
#include "ClassyHDF_Dataset.H"

namespace ClassyHDF {

// We're going to use the Curiously Recurring Template Pattern to
// allow any derived class from Location to create Group objects,
// even though Group derives from Location.
template<class TGroup>
class Location : public NamedIdentity {
    private:
        bool m_existed;
    protected:
        void set_existed(bool existed) { m_existed = existed; }
    public:
        // this function tells us if the location existed already
        // when we opened it or if we had to create it
        bool existed() const { return m_existed; }

        TGroup get_group(const std::string& group_name) {
            // given a group name, create or open it in the Location and return the Group object
            // warning: does not check to make sure the group is not already open
            TGroup group(*this, group_name);
            return group;
        }

        std::vector<TGroup> get_nested_groups(const std::vector<std::string>& nested_group_names) {
            // given a list of nested groups, either create or open them in the Location
            // and return a list of corresponding Group objects.
            // warning: does not check to make sure any of the group are already open
            std::vector<TGroup> nested_groups;
            Location& super_location = *this;

            for (auto gname : nested_group_names) {
                // open or create each group
                nested_groups.emplace_back(super_location, gname);
                super_location = nested_groups.back();
            }

            return nested_groups;
        }

        Dataset open_dataset(const std::string& dataset_name) {
            // will open the named dataset from this location and
            // return the Dataset object
            hid_t ds_id = H5Dopen(id(), dataset_name.c_str(), H5P_DEFAULT);
            Dataset dataset(dataset_name, ds_id);
            return dataset;
        }

        bool has_dataset(const std::string& dataset_name) {
            bool dataset_exists = false;
            hid_t ds_id = -1;

            H5E_BEGIN_TRY
                ds_id = H5Dopen(id(), dataset_name.c_str(), H5P_DEFAULT);
            H5E_END_TRY

            if (ds_id >= 0) {
                dataset_exists = true;
                herr_t status = H5Dclose(ds_id);
                assert(status >= 0);
            }
            return dataset_exists;
        }

        template<typename T>
        std::vector<T> read_dataset(const std::string& dataset_name,
                                    const std::vector<int>& offsets = {},
                                    const std::vector<int>& read_dims = {})
        {
            // will return contiguous data of dimensions [read_dims] from the
            // [dataset_name] named dataset in this location using the [offsets].
            Dataset dataset = open_dataset(dataset_name);
            return dataset.read<T>(offsets, read_dims);
        }

        Dataset create_dataset(const std::string& dataset_name,
                               const hid_t& hdf5_type_id,
                               const std::vector<int>& dimensions,
                               const std::vector<int>& chunk_dimensions,
                               const int compression_level = 0)
        {
            // will create the specified dataset in this location and
            // return the Dataset object

            // first make a Dataspace for this dataset with the specified dimensions
            Dataspace dataspace(dimensions);

            // a data set property list specifies how our data is written
            // including chunking into contigous subsets and compression
            hid_t dataset_creation_parameters;
            herr_t status;

            dataset_creation_parameters = H5Pcreate(H5P_DATASET_CREATE);

            if (chunk_dimensions.size() > 0) {
                hsize_t data_rank = getSizeH(dimensions.size());
                const auto h_chunk_dimensions = getSizeVectorH(chunk_dimensions);

                status = H5Pset_chunk(dataset_creation_parameters, data_rank, h_chunk_dimensions.data());
                assert(status >= 0);
            }

            if (compression_level > 0) {
                // calling this will still call the gzip filter, even if it isn't doing anything,
                // so we can avoid that by checking if we need it
                status = H5Pset_deflate(dataset_creation_parameters, compression_level);
                assert(status >= 0);
            }

            // create the dataset using both the data space and data set
            // property list to hold a particular data type
            hid_t dataset_id = H5Dcreate(id(), dataset_name.c_str(), hdf5_type_id, dataspace.id(),
                                         H5P_DEFAULT, dataset_creation_parameters, H5P_DEFAULT);

            // close the dataset creation parameters
            status = H5Pclose(dataset_creation_parameters);
            assert(status >= 0);

            // make our Dataset object and return
            Dataset dataset(dataset_name, dataset_id, hdf5_type_id);
            return dataset;
        }

        template<typename T>
        Dataset create_dataset(const std::string& dataset_name,
                               std::vector<int> dimensions = {},
                               std::vector<int> chunk_dimensions = {},
                               const int compression_level = 0)
        {
            // if dimensions & chunk_dimensions are not passed,
            // assume a 1D dataset and guess at a reasonable chunk size
            if (dimensions.size() == 0) dimensions = {1};
            if (chunk_dimensions.size() == 0) chunk_dimensions = {256};

            return create_dataset(dataset_name, CppTypeToHDF<T>(),
                                  dimensions, chunk_dimensions,
                                  compression_level);
        }

        template<typename T>
        Dataset create_dataset(const Data<T>& data,
                               const std::vector<int>& chunk_dimensions,
                               const int compression_level = 0)
        {
            // create dataset and write the data, returning the new Dataset object

            // make the dataset
            Dataset dataset = create_dataset(data.name(), data.datatype(), data.dimensions(),
                                             chunk_dimensions, compression_level);

            // Now, get the data space and select a hyperslab to hold all the data
            Dataspace hyperslab = dataset.get_space();
            hyperslab.select_hyperslab({}, {}, data.dimensions());

            // write our data to the dataset with stride 1 and element-sized blocks
            dataset.write(hyperslab, data);

            // return this new dataset
            return dataset;
        }

        template<typename T>
        Dataset append(const Data<T>& data)
        {
            // open dataset and append the data, returning the Dataset object

            Dataset dataset = open_dataset(data.name());
            dataset.append(data);
            return dataset;
        }
};

}

#endif