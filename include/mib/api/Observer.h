#pragma once

#include <string>
#include "mib/api/Frame.h"

namespace mib {

struct Observer {
    virtual void onFrame(const Frame& frame) = 0;
    virtual void onStatus(const std::string& message) = 0;
    virtual void onError(int code, const std::string& message) = 0;
    virtual ~Observer() = default;
};

} // namespace mib


