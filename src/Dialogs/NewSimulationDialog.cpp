//
// Created by gomkyung2 on 2023/09/03.
//

#include "Dialogs/NewSimulationDialog.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <fmt/core.h>
#include <NBodyExecutor/NaiveExecutor.hpp>
#include <NBodyExecutor/BarnesHutExecutor.hpp>

#include "BodyPreset.hpp"

int Dialog::NewSimulationDialog::unnamed_index = 1;

const std::unordered_map<Dialog::NewSimulationDialog::ExecutorType, const char*> Dialog::NewSimulationDialog::executor_type_name {
        { NewSimulationDialog::ExecutorType::Unset, "Select executor type" },
        { NewSimulationDialog::ExecutorType::Naive, "Naive executor" },
        { NewSimulationDialog::ExecutorType::BarnesHut, "Barnes-Hut executor" },
};

const std::unordered_map<Dialog::NewSimulationDialog::BodyPresetType, const char*> Dialog::NewSimulationDialog::body_preset_name {
        { NewSimulationDialog::BodyPresetType::Unset, "Select body preset" },
        { NewSimulationDialog::BodyPresetType::Galaxy, "Galaxy" },
        { NewSimulationDialog::BodyPresetType::Explosion, "Explosion" },
};

std::optional<std::unique_ptr<SimulationView>> Dialog::NewSimulationDialog::open(const char *name) {
    static const auto name_hint_generator = []{
        return fmt::format("Unnamed #{}", unnamed_index++);
    };
    static std::string name_hint = name_hint_generator();

    std::unique_ptr<SimulationView> result = nullptr;
    if (ImGui::BeginPopupModal(name, &is_open, ImGuiWindowFlags_AlwaysAutoResize)){
        // Simulation name.
        ImGui::InputTextWithHint("Simulation name (optional)", name_hint.c_str(), &this->simulation_name);

        // Select n-body executor.
        if (ImGui::BeginCombo("Executor", executor_type_name.at(executor_type))){
            if (ImGui::Selectable("Naive executor")){
                executor_type = ExecutorType::Naive;
            }
            if (ImGui::Selectable("Barnes-Hut executor")){
                executor_type = ExecutorType::BarnesHut;
            }
            ImGui::EndCombo();
        }
        if (executor_type != ExecutorType::Unset){
            ImGui::SliderInt("Number of threads", &num_threads, 1, static_cast<int>(std::thread::hardware_concurrency()));
        }

        // Select body preset.
        if (ImGui::BeginCombo("Body preset", body_preset_name.at(body_preset))){
            if (ImGui::Selectable("Galaxy")){
                body_preset = BodyPresetType::Galaxy;
            }
            if (ImGui::Selectable("Explosion")){
                body_preset = BodyPresetType::Explosion;
            }
            ImGui::EndCombo();
        }
        if (body_preset != BodyPresetType::Unset){
            ImGui::InputInt("Number of bodies", &num_bodies);

            if (ImGui::RadioButton("Random", seed_type == SeedType::RandomSeed)){
                seed_type = SeedType::RandomSeed;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Fixed seed", seed_type == SeedType::FixedSeed)){
                seed_type = SeedType::FixedSeed;
            }
            ImGui::SameLine();
            ImGui::BeginDisabled(seed_type != SeedType::FixedSeed);
            ImGui::InputScalar("Seed", ImGuiDataType_U32, &seed);
            ImGui::EndDisabled();
        }

        // If executor and body preset are property selected, enable add button.
        ImGui::BeginDisabled(executor_type == ExecutorType::Unset || body_preset == BodyPresetType::Unset);
        if (ImGui::Button("Add")){
            auto executor = [&]() -> std::unique_ptr<NBodyExecutor::Executor>{
                auto thread_pool = (num_threads == 1) ? nullptr /* single-thread */ : std::make_unique<BS::thread_pool>(num_threads);

                switch (executor_type){
                    case ExecutorType::Naive:
                        return std::make_unique<NBodyExecutor::NaiveExecutor>(std::move(thread_pool));
                    case ExecutorType::BarnesHut:
                        return std::make_unique<NBodyExecutor::BarnesHutExecutor>(std::move(thread_pool));
                    default:
                        assert(false); // Unreachable
                }
            }();

            std::vector<NBodyExecutor::Body> bodies = [&]{
                seed = (seed_type == SeedType::FixedSeed) ? seed : BodyPreset::rd() /* new seed */;

                switch (body_preset){
                    case BodyPresetType::Galaxy:
                        return BodyPreset::galaxy(num_bodies, seed);
                    case BodyPresetType::Explosion:
                        return BodyPreset::explosion(num_bodies, seed);
                    default:
                        assert(false); // Unreachable
                }
            }();

            if (simulation_name.empty()){
                simulation_name = std::move(name_hint);
                name_hint = name_hint_generator();
            }
            result = std::make_unique<SimulationView>(std::move(simulation_name), std::move(bodies), std::move(executor));
            is_open = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndDisabled();

        ImGui::EndPopup();
    }

    if (result){
        return std::move(result);
    }
    else{
        return std::nullopt;
    }
}
