//
// Created by gomkyung2 on 2023/08/30.
//

#pragma once

#include <vector>

#include <NBodyExecutor/Body.hpp>

namespace BodyPreset{
    std::vector<NBodyExecutor::Body> galaxy(std::size_t num_bodies);
    std::vector<NBodyExecutor::Body> explosion(std::size_t num_bodies);
};