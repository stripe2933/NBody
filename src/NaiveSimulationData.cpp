//
// Created by gomkyung2 on 2023/09/11.
//

#include "NaiveSimulationData.hpp"

NaiveSimulationData::NaiveSimulationData(std::string name,
                                         std::vector<NBodyExecutor::Body> bodies,
                                         std::unique_ptr<NBodyExecutor::NaiveExecutor> executor)
        : SimulationData { std::move(name), std::move(bodies), std::move(executor)}
{

}

void NaiveSimulationData::update(float time_step) {
    SimulationData::update(time_step);
}