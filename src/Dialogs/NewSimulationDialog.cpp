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
#include "NaiveSimulationView.hpp"
#include "BarnesHutSimulationView.hpp"
#include "ImGui/VariantSelector.hpp"
#include "Utils/VisitorHelper.hpp"

IMGUI_LABEL(NewSimulationDialogOption::Executor::Naive, "Naive")
IMGUI_LABEL(NewSimulationDialogOption::Executor::BarnesHut, "Barnes-Hut")

IMGUI_LABEL(NewSimulationDialogOption::BodyPreset::Galaxy, "Galaxy")
IMGUI_LABEL(NewSimulationDialogOption::BodyPreset::Explosion, "Explosion")

IMGUI_LABEL(NewSimulationDialogOption::Seed::Fixed, "Fixed")
IMGUI_LABEL(NewSimulationDialogOption::Seed::Random, "Random")

int Dialog::NewSimulationDialog::unnamed_index = 1;

Dialog::NewSimulationDialog::result_t Dialog::NewSimulationDialog::constructSimulationView() {
    const auto seed = std::visit<std::uint32_t>(
        Utils::overload{
            [](const NewSimulationDialogOption::Seed::Fixed &fixed_seed){
                return fixed_seed.seed;
            },
            [](const NewSimulationDialogOption::Seed::Random&){
                return BodyPreset::rd();
            },
        },
        options.seed
    );

    auto bodies = std::visit(
        Utils::overload{
            [&](const NewSimulationDialogOption::BodyPreset::Galaxy &galaxy){
                return BodyPreset::galaxy(galaxy.num_bodies, seed);
            },
            [&](const NewSimulationDialogOption::BodyPreset::Explosion &explosion){
                return BodyPreset::explosion(explosion.num_bodies, seed);
            },
            [](std::monostate) -> std::vector<NBodyExecutor::Body> {
                throw std::logic_error { "Body preset is not set." };
            },
        },
        options.body_preset
    );

    auto result = std::visit<std::unique_ptr<SimulationView>>(
        Utils::overload{
            [&](const NewSimulationDialogOption::Executor::Naive &naive){
                auto thread_pool = naive.num_threads == 1 ? nullptr : std::make_unique<BS::thread_pool>(naive.num_threads);
                auto executor = std::make_unique<NBodyExecutor::NaiveExecutor>(std::move(thread_pool));
                return std::make_unique<NaiveSimulationView>(std::move(options.name), std::move(bodies), std::move(executor));
            },
            [&](const NewSimulationDialogOption::Executor::BarnesHut &barnes_hut) {
                auto thread_pool = barnes_hut.num_threads == 1 ? nullptr : std::make_unique<BS::thread_pool>(barnes_hut.num_threads);
                auto executor = std::make_unique<NBodyExecutor::BarnesHutExecutor>(std::move(thread_pool));
                return std::make_unique<BarnesHutSimulationView>(std::move(options.name), std::move(bodies), std::move(executor));
            },
            [](std::monostate) {
                throw std::logic_error { "Executor type is not set." };
            },
        },
        options.executor
    );
    options.name.clear();
    return result;
}

std::optional<Dialog::NewSimulationDialog::result_t> Dialog::NewSimulationDialog::open(const char *name) {
    static const auto name_hint_generator = []{
        return fmt::format("Unnamed #{}", unnamed_index++);
    };
    static std::string name_hint = name_hint_generator();

    result_t result = nullptr;
    if (ImGui::BeginPopupModal(name, &is_open, ImGuiWindowFlags_AlwaysAutoResize)){
        // Simulation name.
        ImGui::InputTextWithHint("Simulation name (optional)", name_hint.c_str(), &this->options.name);

        // Select n-body executor.
        ImGui::Text("Executor");
        ImGui::VariantSelector::radio(
            options.executor,
            std::pair {
                [](NewSimulationDialogOption::Executor::Naive &naive) {
                    ImGui::SliderInt("Number of threads", &naive.num_threads, 1, static_cast<int>(std::thread::hardware_concurrency()));
                },
                [] { return NewSimulationDialogOption::Executor::Naive { .num_threads = 4 }; },
            },
            std::pair {
                [](NewSimulationDialogOption::Executor::BarnesHut &barnes_hut) {
                    ImGui::SliderInt("Number of threads", &barnes_hut.num_threads, 1, static_cast<int>(std::thread::hardware_concurrency()));
                },
                [] { return NewSimulationDialogOption::Executor::BarnesHut { .num_threads = 4 }; },
            }
        );

        // Select body preset.
        ImGui::VariantSelector::combo(
            "Body preset",
            options.body_preset,
            "Select body preset",
            std::pair {
                [](NewSimulationDialogOption::BodyPreset::Galaxy &galaxy) {
                    ImGui::DragInt("Number of bodies", &galaxy.num_bodies, 1.f, 1, 65536);
                },
                [] {
                    return NewSimulationDialogOption::BodyPreset::Galaxy { .num_bodies = 256 };
                },
            },
            std::pair {
                [](NewSimulationDialogOption::BodyPreset::Explosion &explosion) {
                    ImGui::DragInt("Number of bodies", &explosion.num_bodies, 1.f, 1, 65536);
                },
                [] {
                    return NewSimulationDialogOption::BodyPreset::Explosion { .num_bodies = 256 };
                },
            }
        );

        if (!std::holds_alternative<std::monostate>(options.body_preset)){
            ImGui::VariantSelector::radio(
                options.seed,
                std::pair {
                    [](NewSimulationDialogOption::Seed::Fixed &fixed_seed) {
                        ImGui::InputInt("Seed", &fixed_seed.seed);
                    },
                    [] { return NewSimulationDialogOption::Seed::Fixed { .seed = 0 }; },
                },
                std::pair {
                    [](NewSimulationDialogOption::Seed::Random &) { },
                    [] { return NewSimulationDialogOption::Seed::Random {}; },
                }
            );
        }

        // If executor and body preset are property selected, enable add button.
        ImGui::BeginDisabled(std::holds_alternative<std::monostate>(options.body_preset));
        if (ImGui::Button("Add")){
            if (options.name.empty()){
                options.name = std::move(name_hint);
                name_hint = name_hint_generator();
            }

            result = constructSimulationView();
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
