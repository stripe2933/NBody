//
// Created by gomkyung2 on 2023/09/11.
//

#include "NaiveSimulationData.hpp"

#include <imgui.h>

NaiveSimulationData::NaiveSimulationData(std::string name,
                                         std::vector<NBodyExecutor::Body> bodies,
                                         std::unique_ptr<NBodyExecutor::NaiveExecutor> executor)
        : SimulationData { ExecutorType::Naive, std::move(name), std::move(bodies), std::move(executor)}
{

}

void NaiveSimulationData::update(float time_step) {
    SimulationData::update(time_step);
}

void NaiveSimulationData::updateImGui(float time_step) {
    SimulationData::updateImGui(time_step);

    ImGui::Text("Executor: Naive (%u threads)", executor->thread_pool ? executor->thread_pool->get_thread_count() : 1);
}