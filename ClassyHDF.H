#ifndef CLASSY_HDF_H_
#define CLASSY_HDF_H_

#include <vector>
#include "hdf5.h"

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

class Identity {
    private:
        hid_t m_id;
        bool m_initialized;
    protected:
        void set_id(hid_t id) { m_id = id; m_initialized = true; }
    public:
        Identity(Identity&&) = default;
        Identity() : m_initialized(false) {}
        Identity(hid_t id) { set_id(id); }
        hid_t id() const { return m_id; }
        bool initialized() const { return m_initialized; }
};

class NamedIdentity : public Identity {
    private:
        std::string m_name;
    protected:
        void set_name(std::string name) { m_name = name; }
    public:
        NamedIdentity(NamedIdentity&&) = default;
        NamedIdentity() : m_name("") {}
        NamedIdentity(const std::string& name, hid_t id) { set_name(name); set_id(id); }
        std::string name() const { return m_name; }
};

class Dimensions {
    private:
        int m_rank;
        std::vector<int> m_dimensions;
    protected:
        void set_dimensions(const std::vector<int>& dimensions) {
            m_rank = dimensions.size();
            m_dimensions = dimensions;
        }
    public:
        int rank() const { return m_rank; }
        int length(int i) const { return m_dimensions[i]; }
        std::vector<int> dimensions() const { return m_dimensions; }
};

class Dataspace : public Identity, public Dimensions {
    private:
        std::vector<hsize_t> m_max_dimensions;
    public:
        // declare a move constructor so we ensure the destructor is
        // not called when we return an object of this class by value
        Dataspace(Dataspace&&) = default;

        Dataspace() {}

        Dataspace(const std::vector<int>& dimensions) {
            create(dimensions);
        }

        Dataspace(const std::vector<int>& dimensions, hid_t ds_id) {
            // the dataspace has already been created, so here we set dimensions & ID
            set_dimensions(dimensions);
            m_max_dimensions.resize(rank(), H5S_UNLIMITED);
            set_id(ds_id);
        }

        ~Dataspace() {
            herr_t status = H5Sclose(id());
            assert(status >= 0);
        }

        void create(const std::vector<int>& ds_dimensions) {
            // we can only create this dataspace if it is currently uninitialized
            // to avoid HDF5 library resource leaks
            assert(!initialized());

            // create an unlimited-max-size dataspace with the given dimensions
            set_dimensions(ds_dimensions);
            m_max_dimensions.resize(rank(), H5S_UNLIMITED);

            // create the dataspace and record that we own this resource
            // so we will need to free it in the destructor.
            const auto h_dimensions = getSizeVectorH(dimensions());
            set_id(H5Screate_simple(rank(), h_dimensions.data(), m_max_dimensions.data()));
        }

        void select_hyperslab(std::vector<int> offsets = {},
                              const std::vector<int>& strides = {},
                              const std::vector<int>& block_counts = {},
                              const std::vector<int>& block_sizes = {})
        {
            // transforms the current dataspace into a hyperslab using H5S_SELECT_SET
            // with the desired number of blocks [block_counts] each shaped like [block_sizes]
            // starting from the hyperslab [offsets], striding by the number of elements
            // (not blocks) specified by [strides].
            //
            // if the arguments [offsets], [strides] and [block_sizes] are
            // empty, the offsets will be 0, the stride will be 1,
            // and each block will simply be a single element of data.
            //
            // if any of [offsets] are negative, they are intepreted as offsets from the end
            // of that dimensions, just like in Python

            // if we haven't selected anything, do nothing
            if (block_counts.size() == 0) return;

            // if offsets is empty, then set the offsets to 0
            if (offsets.size() == 0) offsets.resize(block_counts.size(), 0);

            // interpret any negative offset as an offset backwards from
            // the end of the dataset in the respective dimension
            for (int i = 0; i < rank(); ++i) {
                if (offsets[i] < 0) {
                    offsets[i] = length(i) + offsets[i] % length(i);
                }
            }

            const auto h_offsets = getSizeVectorH(offsets);
            const auto h_block_counts = getSizeVectorH(block_counts);
            const auto h_strides = getSizeVectorH(strides);
            const auto h_block_sizes = getSizeVectorH(block_sizes);

            // if strides or block_sizes is empty, then set their pointers to NULL for the HDF5 defaults
            const hsize_t* h_strides_p = h_strides.size() > 0 ? h_strides.data() : NULL;
            const hsize_t* h_block_sizes_p = h_block_sizes.size() > 0 ? h_block_sizes.data() : NULL;

            herr_t status = H5Sselect_hyperslab(id(), H5S_SELECT_SET,
                                                h_offsets.data(), h_strides_p,
                                                h_block_counts.data(), h_block_sizes_p);

            assert(status >= 0);
        }

        void select_contiguous(std::vector<int> offsets = {}, const std::vector<int>& counts = {})
        {
            select_hyperslab(offsets, {}, counts, {});
        }
};

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

class Dataset : public NamedIdentity, public DataType {
    public:
        // declare a move constructor so we ensure the destructor is
        // not called when we return an object of this class by value
        Dataset(Dataset&&) = default;

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
            herr_t status = H5Dclose(id());
            assert(status >= 0);
        }

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

        void write(const Dataspace& target_space, const Data& data) {
            // write the data stored in the Data object to the target dataspace
            herr_t status = H5Dwrite(id(), datatype(), data.dataspace().id(),
                                     target_space.id(), H5P_DEFAULT, data.data());
            assert(status >= 0);
        }

        void append(const Data& data) {
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

        void read(const Dataspace& hyperslab, Data& data) {
            // read from the hyperslab to fill Data
            herr_t status = H5Dread(id(), data.datatype(), data.dataspace().id(),
                                    hyperslab.id(), H5P_DEFAULT, data.data());
            assert(status >= 0);
        }

        void read(const std::vector<int>& offsets, Data& data) {
            // read data from this dataset, using the supplied offsets
            // with a stride of 1, block count set to the dimensionality of data,
            // and a block shape of 1 data element.
            Dataspace hyperslab = get_space();
            hyperslab.select_contiguous(offsets, data.dimensions());

            read(hyperslab, data);
        }

        template<typename T>
        std::vector<T> read(const std::vector<int>& offsets,
                            const std::vector<int>& read_dims)
        {
            // takes the supplied offsets and the dimensions of the data to read
            // and returns a std::vector with the requested data, flattened to 1D

            // first, figure out how many data elements to read
            int size_data = 1;
            for (const auto& d : read_dims) {
                size_data *= d;
            }

            // now create a vector to hold them
            std::vector<T> datavector(size_data);

            // and create a data object that knows how to read that data
            Data data("data", datatype(), read_dims, datavector.data());

            // read the data and return the vector
            read(offsets, data);
            return datavector;
        }

        template<typename T, typename F>
        int search1D(F test, bool search_from_end = false) {
            // return the first index in the dataset for which test(element) returns true
            // if search_from_end, searches from the end of the dataset
            // F is the type of a test function
            // T is the type contained in the dataset
            std::vector<int> edims = {1};
            std::vector<T> element(1); 
            Data delement("element", datatype(), edims, element.data());

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
                    std::cout << "found search in index: " << offset << std::endl;
                    return offset;
                }
            }

            // we didn't find the element
            return -1;
        }
};

// we are going to add open_dataset() and create_dataset() functions
// to the Location class ...

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

        template<typename T>
        std::vector<T> read_dataset(const std::string& dataset_name,
                                    std::vector<int> offsets = {},
                                    std::vector<int> read_dims = {})
        {
            // will return contiguous data of dimensions [read_dims] from the
            // [dataset_name] named dataset in this location using the [offsets].
            Dataset dataset = open_dataset(dataset_name);

            // if [offsets] or [read_dims] were supplied, they should have
            // the same length as the dataset rank.
            if (offsets.size() > 0) assert(offsets.size() == dataset.rank());
            if (read_dims.size() > 0) assert(read_dims.size() == dataset.rank());

            // if [offsets] is not supplied, by default use offsets of 0.
            // if [read_dims] is not supplied, by default read the entire dataset.
            if (offsets.size() == 0) offsets.resize(dataset.rank(), 0);
            if (read_dims.size() == 0) read_dims = dataset.dimensions();

            // read and return data
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

        Dataset create_dataset(const Data& data,
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

        Dataset append(const Data& data)
        {
            // open dataset and append the data, returning the Dataset object

            Dataset dataset = open_dataset(data.name());
            dataset.append(data);
            return dataset;
        }
};

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