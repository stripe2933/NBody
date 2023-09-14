//
// Created by gomkyung2 on 2023/09/11.
//

#pragma once

#include <NBodyExecutor/NaiveExecutor.hpp>

#include "SimulationData.hpp"

struct NaiveSimulationData : public SimulationData {
    NaiveSimulationData(std::string name, std::vector<NBodyExecutor::Body> bodies, std::unique_ptr<NBodyExecutor::NaiveExecutor> executor);

    void update(float time_step) override;
    void updateImGui(float time_step) override;
};