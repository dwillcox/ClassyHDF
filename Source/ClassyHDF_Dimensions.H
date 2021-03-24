#ifndef CLASSY_HDF_DIMENSIONS_H_
#define CLASSY_HDF_DIMENSIONS_H_

#include <vector>

namespace ClassyHDF {

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

}

#endif