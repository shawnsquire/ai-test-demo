#pragma once
#include <string>
#include "core/Model.h"

namespace loaders {
    bool loadModel(const std::string& path, Model& out);
}

