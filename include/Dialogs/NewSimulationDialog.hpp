//
// Created by gomkyung2 on 2023/09/03.
//

#pragma once

#include "Dialog.hpp"
#include "SimulationView.hpp"

namespace NewSimulationDialogOption{
    namespace Executor{
        struct Naive { int num_threads; };
        struct BarnesHut { int num_threads; };

        using Type = std::variant<Naive, BarnesHut>;
    };

    namespace BodyPreset{
        struct Galaxy { int num_bodies; };
        struct Explosion { int num_bodies; };

        using Type = std::variant<std::monostate, Galaxy, Explosion>;
    };

    namespace Seed{
        struct Fixed { int seed; };
        struct Random { };

        using Type = std::variant<Fixed, Random>;
    };
};

namespace Dialog{
    class NewSimulationDialog : public Dialog<std::unique_ptr<SimulationView>>{
    private:
        static int unnamed_index;

        struct Options{
            std::string name;

            NewSimulationDialogOption::Executor::Type executor = NewSimulationDialogOption::Executor::Naive { .num_threads = 4 };
            NewSimulationDialogOption::BodyPreset::Type body_preset;
            NewSimulationDialogOption::Seed::Type seed = NewSimulationDialogOption::Seed::Random{};
        } options;

        std::unique_ptr<SimulationView> constructSimulationView();

    public:
        std::optional<result_t> open(const char *title) override;
    };
};