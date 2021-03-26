#ifndef CLASSY_HDF_DATASET_H_
#define CLASSY_HDF_DATASET_H_

#include <string>
#include <vector>
#include "hdf5.h"

#include "ClassyHDF_Identity.H"
#include "ClassyHDF_Types.H"
#include "ClassyHDF_Dataspace.H"

namespace ClassyHDF {

class Dataset : public NamedIdentity, public DataType {
    public:
        Dataset() {}

        Dataset(const std::string& ds_name, hid_t ds_id) {
            set_name(ds_name);
            set_id(ds_id);
            lookup_dataset_type(id());
        }

        Dataset(const std::string& ds_name, hid_t ds_id, hid_t datatype) {
            set_name(ds_name);
            set_id(ds_id);
            set_datatype(datatype);
        }

        ~Dataset() {
            if (initialized()) {
                herr_t status = H5Dclose(id());
                assert(status >= 0);
            }
        }

        // declare a move constructor so we ensure the destructor is
        // not called when we return an object of this class by value
        Dataset(Dataset&&) = default;

        // declare a move assignment operator
        Dataset& operator=(Dataset&& other)
        {
            // for move assignments, if we currently own an identity, release it first
            if (initialized() && this != &other) {
                this->~Dataset();
            }

            set_name(other.name());
            set_datatype(other.datatype());

            if (other.initialized()) {
                // take the other's ID & invalidate it in other so
                // it is not freed in other's destructor
                set_id(other.id());
                other.invalidate();
            }

            return *this;
        }

        // now, we must forbid copying any class that derives from Identity
        // because either copy could free its HDF5 library resources and
        // leave the other in an invalid state
        //
        // delete the copy constructor
        Dataset(const Dataset& other) = delete;
        //
        // delete the copy assignment operator
        Dataset& operator=(const Dataset& other) = delete;

        Dataspace get_space() const {
            // return a Dataspace object with the current data space of this dataset
            hid_t dspace_id = H5Dget_space(id());

            int dim = H5Sget_simple_extent_ndims(dspace_id);
            std::vector<hsize_t> h_dimensions(dim);
            std::vector<hsize_t> h_max_dimensions(dim);
            herr_t status = H5Sget_simple_extent_dims(dspace_id, h_dimensions.data(),
                                                      h_max_dimensions.data());
            assert(status >= 0);

            Dataspace dspace(getSizeVectorI(h_dimensions), dspace_id);
            return dspace;
        }

        int rank() const {
            // get the rank of the dataset
            const auto dataspace = get_space();
            return dataspace.rank();
        }

        std::vector<int> dimensions() const {
            // get the current dimensions of the dataset
            const auto dataspace = get_space();
            return dataspace.dimensions();
        }

        void set_extent(const std::vector<int>& new_dimensions) {
            // set the extent of this dataset to the specified dimensions,
            // making the dataset size equal to the size requested ...
            // i.e. we specify the total new size of the dataset,
            // NOT the amount by which to extend!
            const auto h_new_dimensions = getSizeVectorH(new_dimensions);
            herr_t status = H5Dset_extent(id(), h_new_dimensions.data());
            assert(status >= 0);
        }

        void expand_by(const std::vector<int>& delta_dimensions) {
            // expands the dataset by delta in each dimension

            // first off, assert delta is the same rank as our dataset
            Dataspace dataspace = get_space();
            assert(delta_dimensions.size() == dataspace.rank());

            // calculate new dimensions
            const auto old_dimensions = dataspace.dimensions();
            std::vector<int> new_dataset_size;

            for (int i = 0; i < dataspace.rank(); ++i) {
                new_dataset_size.push_back(old_dimensions[i] + delta_dimensions[i]);
            }

            // extend the dataset with our new dimensions
            set_extent(new_dataset_size);
        }

        template<typename T>
        void write(const Dataspace& target_space, const Data<T>& data) {
            // first assert the types match
            assert(is_equal_datatype(data.datatype()));

            // write the data stored in the Data object to the target dataspace
            herr_t status = H5Dwrite(id(), datatype(), data.dataspace().id(),
                                     target_space.id(), H5P_DEFAULT, data.data());
            assert(status >= 0);
        }

        template<typename T>
        void append(const Data<T>& data) {
            // expands the dataset to accomodate the new data and writes it

            // get the current dimensions before expanding the dataset
            const auto old_dimensions = dimensions();

            // expand the dataset by the data size in each dimension
            expand_by(data.dimensions());

            // Now, get the expanded data space
            Dataspace hyperslab = get_space();

            // And turn the new data space into a hyperslab we're going to write into
            // using the old_dimensions as the hyperslab offset
            hyperslab.select_hyperslab(old_dimensions, {}, data.dimensions());

            // write our data to this hyperslab in the dataset
            write(hyperslab, data);
        }

        template<typename T>
        void read(const Dataspace& hyperslab, Data<T>& data) {
            // first assert the types match
            assert(is_equal_datatype(data.datatype()));

            // read from the hyperslab to fill Data
            herr_t status = H5Dread(id(), data.datatype(), data.dataspace().id(),
                                    hyperslab.id(), H5P_DEFAULT, data.data());
            assert(status >= 0);
        }

        template<typename T>
        void read(const std::vector<int>& offsets, Data<T>& data) {
            // read data from this dataset, using the supplied offsets
            // with a stride of 1, block count set to the dimensionality of data,
            // and a block shape of 1 data element.
            Dataspace hyperslab = get_space();
            hyperslab.select_contiguous(offsets, data.dimensions());

            read(hyperslab, data);
        }

        template<typename T>
        std::vector<T> read(std::vector<int> offsets = {},
                            std::vector<int> read_dims = {})
        {
            // takes the supplied offsets and the dimensions of the data to read
            // and returns a std::vector with the requested data, flattened to 1D

            // if [offsets] or [read_dims] were supplied, they should have
            // the same length as the dataset rank.
            if (offsets.size() > 0) assert(offsets.size() == rank());
            if (read_dims.size() > 0) assert(read_dims.size() == rank());

            // if [offsets] is not supplied, by default use offsets of 0.
            // if [read_dims] is not supplied, by default read the entire dataset.
            if (offsets.size() == 0) offsets.resize(rank(), 0);
            if (read_dims.size() == 0) read_dims = dimensions();

            // first, figure out how many data elements to read
            int size_data = 1;
            for (const auto& d : read_dims) {
                size_data *= d;
            }

            // now create a vector to hold them
            std::vector<T> datavector(size_data);

            // and create a data object that knows how to read that data
            Data<T> data("data", read_dims, datavector.data());

            // read the data and return the vector
            read(offsets, data);
            return datavector;
        }

        template<typename T, typename F>
        int search(F test, bool search_from_end = false) {
            // return the first index in the dataset for which test(element) returns true
            // if search_from_end, searches from the end of the dataset
            // F is the type of a test function
            // T is the type contained in the dataset
            //
            // warning: currently only implemented for 1D datasets
            assert(rank() == 1);

            std::vector<int> edims = {1};
            std::vector<T> element(1); 
            Data<T> delement("element", edims, element.data());

            const auto ds_dims = dimensions();
            std::vector<int> index(rank(), 0);
            std::vector<int> index_end(rank(), 0);
            int increment = (search_from_end) ? -1 : 1;

            for (int i = 0; i < rank(); ++i) {
                if (search_from_end) {
                    index[i] = ds_dims[i] - 1;
                } else {
                    index_end[i] = ds_dims[i] - 1;
                }
            }

            // this part is only written for a 1D dataset so far ...
            for (int offset = index[0]; offset != index_end[0]; offset += increment) {
                Dataspace hyperslab = get_space();
                hyperslab.select_hyperslab({offset}, {}, edims, {});
                read(hyperslab, delement);
                if (test(element[0])) {
                    return offset;
                }
            }

            // we didn't find the element
            return -1;
        }
};

}

#endif