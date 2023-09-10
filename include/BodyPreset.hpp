//
// Created by gomkyung2 on 2023/08/30.
//

#pragma once

#include <vector>
#include <random>

#include <NBodyExecutor/Body.hpp>

namespace BodyPreset{
    static std::random_device rd;

    std::vector<NBodyExecutor::Body> galaxy(std::size_t num_bodies, unsigned int seed);
    std::vector<NBodyExecutor::Body> explosion(std::size_t num_bodies, unsigned int seed);
};