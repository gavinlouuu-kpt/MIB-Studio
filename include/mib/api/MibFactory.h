#pragma once

#include <memory>
#include "mib/api/IMibController.h"

namespace mib {

std::unique_ptr<IMibController> createMibController();

} // namespace mib


