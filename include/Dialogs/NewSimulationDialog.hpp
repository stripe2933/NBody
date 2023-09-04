//
// Created by gomkyung2 on 2023/09/03.
//

#pragma once

#include <unordered_map>

#include "Dialog.hpp"
#include "SimulationView.hpp"

namespace Dialog{
    class NewSimulationDialog : public Dialog<std::unique_ptr<SimulationView>>{
    private:
        enum class ExecutorType{
            Unset,
            Naive,
            BarnesHut,
        };

        enum class BodyPresetType{
            Unset,
            Galaxy,
            Explosion,
        };

        enum class SeedType{
            FixedSeed = 0,
            RandomSeed = 1
        };

        static int unnamed_index;

        std::string simulation_name;

        ExecutorType executor_type = ExecutorType::Unset;
        int num_threads = 1;

        BodyPresetType body_preset = BodyPresetType::Unset;
        int num_bodies = 256;
        SeedType seed_type = SeedType::FixedSeed;
        unsigned seed = 0;

        static const std::unordered_map<ExecutorType, const char*> executor_type_name;
        static const std::unordered_map<BodyPresetType, const char*> body_preset_name;

    public:
        std::optional<std::unique_ptr<SimulationView>> open(const char *title) override;
    };
};