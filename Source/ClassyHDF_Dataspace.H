#ifndef CLASSY_HDF_DATASPACE_H_
#define CLASSY_HDF_DATASPACE_H_

#include <string>
#include <vector>
#include "hdf5.h"

#include "ClassyHDF_Identity.H"
#include "ClassyHDF_Dimensions.H"

namespace ClassyHDF {

class Dataspace : public Identity, public Dimensions {
    private:
        std::vector<hsize_t> m_max_dimensions;
    public:
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
            if (initialized()) {
                herr_t status = H5Sclose(id());
                assert(status >= 0);
            }
        }

        // declare a move constructor so we ensure the destructor is
        // not called when we return an object of this class by value
        Dataspace(Dataspace&&) = default;

        // declare a move assignment operator
        Dataspace& operator=(Dataspace&& other)
        {
            // for move assignments, if we currently own an identity, release it first
            if (initialized() && this != &other) {
                this->~Dataspace();
            }

            set_dimensions(other.dimensions());
            m_max_dimensions.resize(rank(), H5S_UNLIMITED);

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
        Dataspace(const Dataspace& other) = delete;
        //
        // delete the copy assignment operator
        Dataspace& operator=(const Dataspace& other) = delete;

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

}

#endif