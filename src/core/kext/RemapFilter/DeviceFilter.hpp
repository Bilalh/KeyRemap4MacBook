#ifndef DEVICEFILTER_HPP
#define DEVICEFILTER_HPP

#include "RemapFilterBase.hpp"

namespace org_pqrs_KeyRemap4MacBook {
  namespace RemapFilter {
    class DeviceFilter {
    public:
      DeviceFilter(unsigned int t);
      ~DeviceFilter(void);

      void add(unsigned int vendorID, unsigned int productID, unsigned int locationID);

      bool isblocked(void);

    private:
      unsigned int type_;
      Vector_DeviceIdentifier targets_;
    };
  }
}

#endif
