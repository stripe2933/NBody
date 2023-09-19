//
// Created by gomkyung2 on 2023/09/10.
//

#include "Dialogs/NewSimulationDataDialog.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <fmt/core.h>
#include <NBodyExecutor/NaiveExecutor.hpp>
#include <NBodyExecutor/BarnesHutExecutor.hpp>
#include <imgui_variant_selector.hpp>
#include <visitor_helper.hpp>

#include "BodyPreset.hpp"
#include "NaiveSimulationData.hpp"
#include "BarnesHutSimulationData.hpp"
#include "ImGui/ScopedDisabled.hpp"

IMGUI_LABEL(NewSimulationDataDialog::Options::Executor::Naive, "Naive");
IMGUI_LABEL(NewSimulationDataDialog::Options::Executor::BarnesHut, "Barnes-Hut");

IMGUI_LABEL(NewSimulationDataDialog::Options::BodyPreset::Galaxy, "Galaxy");
IMGUI_LABEL(NewSimulationDataDialog::Options::BodyPreset::Explosion, "Explosion");

IMGUI_LABEL(NewSimulationDataDialog::Options::Seed::Fixed, "Fixed");
IMGUI_LABEL(NewSimulationDataDialog::Options::Seed::Random, "Random");

int NewSimulationDataDialog::unnamed_index = 1;

NewSimulationDataDialog::result_t NewSimulationDataDialog::constructResult() {
    const auto seed = std::visit<std::uint32_t>(
        overload{
            [](const Options::Seed::Fixed &fixed_seed){
                return fixed_seed.seed;
            },
            [](const Options::Seed::Random&){
                return BodyPreset::rd();
            },
        },
        options.seed
    );

    auto bodies = std::visit(
        overload{
            [&](const Options::BodyPreset::Galaxy &galaxy){
                return BodyPreset::galaxy(galaxy.num_bodies, seed);
            },
            [&](const Options::BodyPreset::Explosion &explosion){
                return BodyPreset::explosion(explosion.num_bodies, seed);
            },
            [](std::monostate) -> std::vector<NBodyExecutor::Body> {
                throw std::logic_error { "Body preset is not set." };
            },
        },
        options.body_preset
    );

    return std::visit<std::shared_ptr<SimulationData>>(
        overload{
            [&](const Options::Executor::Naive &naive){
                auto thread_pool = naive.num_threads == 1 ? nullptr : std::make_unique<BS::thread_pool>(naive.num_threads);
                auto executor = std::make_unique<NBodyExecutor::NaiveExecutor>(std::move(thread_pool));
                return std::make_shared<NaiveSimulationData>(std::move(options.name), std::move(bodies), std::move(executor));
            },
            [&](const Options::Executor::BarnesHut &barnes_hut) {
                auto thread_pool = barnes_hut.num_threads == 1 ? nullptr : std::make_unique<BS::thread_pool>(barnes_hut.num_threads);
                auto executor = std::make_unique<NBodyExecutor::BarnesHutExecutor>(std::move(thread_pool));
                return std::make_shared<BarnesHutSimulationData>(std::move(options.name), std::move(bodies), std::move(executor));
            }
        },
        options.executor
    );
}

std::optional<NewSimulationDataDialog::result_t> NewSimulationDataDialog::inner(){
    static const auto name_hint_generator = []{
        return fmt::format("Unnamed simulation #{}", unnamed_index++);
    };
    static std::string name_hint = name_hint_generator();

    // Simulation name.
    ImGui::InputTextWithHint("Name (optional)", name_hint.c_str(), &this->options.name);

    // Select n-body executor.
    if (ImGui::TreeNode("Executor")){
        static const int max_threads = static_cast<int>(std::thread::hardware_concurrency());
        ImGui::VariantSelector::radio(
            options.executor,
            std::pair {
                [](Options::Executor::Naive &executor) {
                    ImGui::SliderInt("Number of threads", &executor.num_threads, 1, max_threads);
                },
                [] { return Options::Executor::Naive { .num_threads = 4 }; },
            },
            std::pair {
                [](Options::Executor::BarnesHut &executor) {
                    ImGui::SliderInt("Number of threads", &executor.num_threads, 1, max_threads);
                },
                [] { return Options::Executor::BarnesHut { .num_threads = 4 }; },
            }
        );

        ImGui::TreePop();
    }

    // Select body preset.
    if (ImGui::TreeNode("Body setting")){
        ImGui::VariantSelector::combo(
            "##body_preset_combo",
            options.body_preset,
            "Select body preset",
            std::pair {
                [](Options::BodyPreset::Galaxy &galaxy) {
                    ImGui::DragInt("Body count", &galaxy.num_bodies, 1.f, 1, 65536);
                },
                [] {
                    return Options::BodyPreset::Galaxy { .num_bodies = 256 };
                },
            },
            std::pair {
                [](Options::BodyPreset::Explosion &explosion) {
                    ImGui::DragInt("Body count", &explosion.num_bodies, 1.f, 1, 65536);
                },
                [] {
                    return Options::BodyPreset::Explosion { .num_bodies = 256 };
                },
            }
        );

        if (!std::holds_alternative<std::monostate>(options.body_preset)){
            ImGui::TextUnformatted("Seed");
            ImGui::VariantSelector::radio(
                options.seed,
                std::pair {
                    [](Options::Seed::Fixed &fixed_seed) {
                        ImGui::InputInt("##fixed_seed_input", &fixed_seed.seed);
                    },
                    [] { return Options::Seed::Fixed { .seed = 0 }; },
                },
                std::pair {
                    [](Options::Seed::Random &) { },
                    [] { return Options::Seed::Random {}; },
                }
            );
        }

        ImGui::TreePop();
    }

    // If executor and body preset are property selected, enable add button.
    ImGui::ScopedDisabled scoped_disabled { std::holds_alternative<std::monostate>(options.body_preset) };
    if (ImGui::Button("Add")) {
        if (options.name.empty()) {
            options.name = std::move(name_hint);
            name_hint = name_hint_generator();
        }

        return closeAndPresentResult(constructResult());
    }
    ImGui::SetItemDefaultFocus(); // For add button.

    return std::nullopt;
}