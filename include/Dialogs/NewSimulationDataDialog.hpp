//
// Created by gomkyung2 on 2023/09/10.
//

#pragma once

#include "Dialog.hpp"
#include "SimulationView.hpp"

class NewSimulationDataDialog : public Dialog<std::shared_ptr<SimulationData>>{
private:
    static int unnamed_index;

    struct Options{
        std::string name;

        struct Executor{
            struct Naive { int num_threads; };
            struct BarnesHut { int num_threads; };

            using Type = std::variant<Naive, BarnesHut>;
        };

        struct BodyPreset{
            struct Galaxy { int num_bodies; };
            struct Explosion { int num_bodies; };
            struct GalaxyPair {
                int num_bodies1, num_bodies2;
                glm::vec3 cm_position1, cm_position2;
                glm::vec3 cm_velocity1, cm_velocity2;
            };

            using Type = std::variant<std::monostate, Galaxy, GalaxyPair, Explosion>;
        };

        struct Seed{
            struct Fixed { int seed; };
            struct Random { };

            using Type = std::variant<Fixed, Random>;
        };

        Options::Executor::Type executor = Options::Executor::Naive { .num_threads = 4 };
        Options::BodyPreset::Type body_preset;
        Options::Seed::Type seed = Options::Seed::Random{};
    } options;

    result_t constructResult();
    std::optional<result_t> inner() override;

public:
    NewSimulationDataDialog() : Dialog { "Create new simulation data" } { }
};