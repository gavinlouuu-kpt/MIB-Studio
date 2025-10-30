#pragma once

#include <string>
#include "mib/api/Observer.h"

namespace mib {

struct IMibController {
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void setParam(const std::string& key, const std::string& value) = 0;
    virtual void onKey(int keyCode) = 0;
    virtual void subscribe(Observer* observer) = 0;
    virtual void unsubscribe(Observer* observer) = 0;
    virtual ~IMibController() = default;
};

} // namespace mib


