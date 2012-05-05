#include "Device.h"

Device::Device():
m_Type(""), m_Id(""), m_Name("")
{
    //ctor
}

Device::Device(string type, string id, string name):
m_Type(type), m_Id(id), m_Name(name)
{
    //ctor
}

Device::~Device()
{
    //dtor
}

Device::Device(const Device& other)
{
    m_Type = other.m_Type;
    m_Id = other.m_Id;
    m_Name = other.m_Name;
}

Device& Device::operator=(const Device& rhs)
{
    if (this == &rhs) return *this; // handle self assignment

    m_Type = rhs.m_Type;
    m_Id = rhs.m_Id;
    m_Name = rhs.m_Name;

    return *this;
}
