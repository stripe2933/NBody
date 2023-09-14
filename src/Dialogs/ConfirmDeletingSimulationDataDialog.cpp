//
// Created by gomkyung2 on 2023/09/12.
//

#include "Dialogs/ConfirmDeletingSimulationDataDialog.hpp"

#include <imgui.h>

#include "SimulationView.hpp"

std::optional<ConfirmDeletingSimulationDataDialog::result_t> ConfirmDeletingSimulationDataDialog::inner(SimulationData *simulation_data) {
    std::optional<result_t> result;

    ImGui::TextUnformatted(
        "Are you sure you want to delete this simulation data? All simulation views that are using this "
        "simulation data will be deleted as well.");
    ImGui::Separator();

    ImGui::Text("There %s %zu view(s) associated with this simulation data:",
                simulation_data->associated_views.size() == 1 ? "is" : "are",
                simulation_data->associated_views.size());
    for (const auto &associated_view : simulation_data->associated_views){
        if (auto valid_view = associated_view.lock()){
            ImGui::BulletText("%s", valid_view->name.c_str());
        }
    }
    ImGui::Separator();

    if (ImGui::Button("Yes")){
        return closeAndPresentResult(true);
    }
    ImGui::SetItemDefaultFocus(); // For yes button.

    ImGui::SameLine(); // Two button are in the same line.
    if (ImGui::Button("Cancel")){
        return closeAndPresentResult(false);
    }

    return result;
}
