//
// Created by gomkyung2 on 2023/09/03.
//

#pragma once

#include <list>

#include "Dialog.hpp"
#include "SimulationView.hpp"

class NewSimulationViewDialog : public Dialog<std::shared_ptr<SimulationView>, const std::list<std::shared_ptr<SimulationData>>&>{
private:
    static int unnamed_index;

    std::string name;
    std::shared_ptr<SimulationData> simulation_data = nullptr;

    std::optional<result_t> inner(const std::list<std::shared_ptr<SimulationData>> &simulation_list) override;

public:
    NewSimulationViewDialog() : Dialog { "Create new simulation view" } { }
};