//
// Created by gomkyung2 on 2023/09/12.
//

#pragma once

#include <list>

#include "Dialog.hpp"
#include "SimulationData.hpp"

class ConfirmDeletingSimulationDataDialog : public Dialog<bool, SimulationData*> {
private:
    std::optional<result_t> inner(SimulationData *simulation_data) override;

public:
    ConfirmDeletingSimulationDataDialog() : Dialog { "Data has associated view." } { }
};