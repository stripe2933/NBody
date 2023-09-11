//
// Created by gomkyung2 on 2023/09/03.
//

#pragma once

#include <list>

#include "Dialog.hpp"
#include "SimulationView.hpp"

namespace Dialog{
    class NewSimulationViewDialog : public Dialog<std::shared_ptr<SimulationView>>{
    private:
        static int unnamed_index;

        std::string name;
        std::shared_ptr<SimulationData> simulation_data = nullptr;

        const std::list<std::shared_ptr<SimulationData>> &simulation_list;

    public:
        NewSimulationViewDialog(const std::list<std::shared_ptr<SimulationData>> &simulation_list);

        std::optional<result_t> open(const char *title) override;
    };
};