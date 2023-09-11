//
// Created by gomkyung2 on 2023/09/03.
//

#include "Dialogs/NewSimulationViewDialog.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <fmt/core.h>
#include <range/v3/view/enumerate.hpp>

int Dialog::NewSimulationViewDialog::unnamed_index = 1;

Dialog::NewSimulationViewDialog::NewSimulationViewDialog(const std::list<std::shared_ptr<SimulationData>> &simulation_list)
        : simulation_list { simulation_list }
{

}

std::optional<Dialog::NewSimulationViewDialog::result_t> Dialog::NewSimulationViewDialog::open(const char *title) {
    static const auto name_hint_generator = []{
        return fmt::format("Unnamed simulation view #{}", unnamed_index++);
    };
    static std::string name_hint = name_hint_generator();

    result_t result = nullptr;
    if (ImGui::BeginPopupModal(title, &is_open, ImGuiWindowFlags_AlwaysAutoResize)){
        // Simulation name.
        ImGui::InputTextWithHint("Name (optional)", name_hint.c_str(), &name);

        if (ImGui::BeginCombo("Simulation to attach", simulation_data ? simulation_data->name.c_str() : "Select a simulation...")){
            for (auto &&[idx, data] : simulation_list | ranges::views::enumerate){
                ImGui::PushID(static_cast<int>(idx));
                if (ImGui::Selectable(data->name.c_str(), simulation_data == data)){
                    this->simulation_data = data;
                }
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }

        // If executor and body preset are property selected, enable add button.
        ImGui::BeginDisabled(simulation_data == nullptr);
        if (ImGui::Button("Add")){
            if (name.empty()){
                name = std::move(name_hint);
                name_hint = name_hint_generator();
            }

            result = std::make_shared<SimulationView>(std::move(name), simulation_data);
            name.clear();
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
