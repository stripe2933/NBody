//
// Created by gomkyung2 on 2023/09/03.
//

#include "Dialogs/NewSimulationViewDialog.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <fmt/core.h>
#include <range/v3/view/enumerate.hpp>

#include "ImGui/ScopedId.hpp"
#include "ImGui/ScopedDisabled.hpp"

int NewSimulationViewDialog::unnamed_index = 1;

std::optional<NewSimulationViewDialog::result_t> NewSimulationViewDialog::inner(const std::list<std::shared_ptr<SimulationData>> &simulation_list) {
    static const auto name_hint_generator = []{
        return fmt::format("Unnamed simulation view #{}", unnamed_index++);
    };
    static std::string name_hint = name_hint_generator();

    result_t result = nullptr;
    // Simulation name.
    ImGui::InputTextWithHint("Name (optional)", name_hint.c_str(), &name);

    if (ImGui::BeginCombo("Simulation to attach", simulation_data ? simulation_data->name.c_str() : "Select a simulation...")){
        for (auto &&[idx, data] : simulation_list | ranges::views::enumerate){
            ImGui::ScopedId scoped_id { static_cast<int>(idx) };
            if (ImGui::Selectable(data->name.c_str(), simulation_data == data)){
                this->simulation_data = data;
            }
        }
        ImGui::EndCombo();
    }

    // If executor and body preset are property selected, enable add button.
    {
        ImGui::ScopedDisabled scoped_disabled { simulation_data == nullptr };
        if (ImGui::Button("Add")) {
            if (name.empty()) {
                name = std::move(name_hint);
                name_hint = name_hint_generator();
            }

            result = SimulationView::fromSimulationData(std::move(name), std::move(simulation_data));
            name.clear();
            simulation_data.reset();

            return closeAndPresentResult(result);
        }
    }
    return std::nullopt;
}
