#ifndef CLASSY_HDF_IDENTITY_H_
#define CLASSY_HDF_IDENTITY_H_

#include <string>
#include "hdf5.h"

namespace ClassyHDF {

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
        void invalidate() { m_id = -1; m_initialized = false; }
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

}

#endif