//
// Created by gomkyung2 on 2023/09/01.
//

#include "SimulationGridView.hpp"

#include <range/v3/view/filter.hpp>
#include <range/v3/view/move.hpp>
#include <range/v3/range/conversion.hpp>
#include <visitor_helper.hpp>
#include <imgui.h>

#include "ImGui/ScopedId.hpp"
#include "ImGui/ScopedDisabled.hpp"

std::span<const std::shared_ptr<SimulationView>> SimulationGridView::views() const noexcept{
    return std::visit([](const auto &x) {
        return std::span { x.children.data(), x.children.size() };
    }, split_grid);
}

void SimulationGridView::add(std::shared_ptr<SimulationView> simulation_view) {
    /* State changing strategy:
     *
     * 1. If current split method is NoSplit (currently no simulation or only one simulation is present),
     *    - If there is no simulation, then set the given simulation as the child.
     *    - If there is one simulation, then change the split method to HorizontalSplit and set previous simulation as the
     *      left child and given simulation as the right child.
     * 2. If current split method is HorizontalSplit (currently two simulations are present), then change the split method
     *    to QuadrantSplitGrid and set previous simulations as top-left and top-right children and given simulation as the
     *    bottom-left child.
     * 3. If current split method is QuadrantSplitGrid (currently three or four simulations are present), then find the first
     *    empty simulation (nullptr) in the children array and set the given simulation as the child. If there is no empty
     *    simulation (which means 4 views are fully presented), then throw std::out_of_range exception.
     */

    std::visit(overload {
        [&](NoSplitGrid &no_split){
            if (auto &[child] = no_split.children; child){
                split_grid = HorizontalSplitGrid { std::move(child), std::move(simulation_view) };
            }
            else{
                child = std::move(simulation_view);
            }
        },
        [&](HorizontalSplitGrid &horizontal_split){
            auto &[left, right] = horizontal_split.children;
            split_grid = QuadrantSplitGrid { std::move(left), std::move(right), std::move(simulation_view), nullptr };
        },
        [&](QuadrantSplitGrid &quadrant_split){
            const auto insert_position = std::find(quadrant_split.children.begin(), quadrant_split.children.end(), nullptr);
            if (insert_position == quadrant_split.children.cend()){
                throw std::out_of_range { "No available space to add simulation view." };
            }

            *insert_position = std::move(simulation_view);
        }
    }, split_grid);
}

void SimulationGridView::removeAt(std::size_t idx) {
    /*
     * State changing strategy:
     *
     * 1. If current split method is NoSplit (currently no simulation or only one simulation is present),
     *    - If no simulation is presented, then throw \p std::invalid_argument exception.
     *    - If one simulation is presented, then set it to nullptr.
     * 2. If current split method is HorizontalSplit (currently two simulations are present), since they are both notnull,
     *    set the given index to nullptr and change the split method to NoSplit (its child is the remained simulation).
     * 3. If current split method is QuadrantSplitGrid (currently three or four simulations are present),
     *    - If simulation corresponds to the given index is nullptr, then throw \p std::invalid_argument exception.
     *    - If simulation corresponds to the given index is notnull, then set it to nullptr.
     *      - If there are two simulations, then change the split method to HorizontalSplit.
     *      - If there are three views, then do nothing.
     */

    std::visit(overload {
        [&](NoSplitGrid &no_split){
            if (idx == 0){
                if (auto &[child] = no_split.children; child){
                    child.reset();
                }
                else{
                    throw std::invalid_argument { "Given simulation is already empty." };
                }
            }
            else{
                throw std::out_of_range { "Index must be zero." };
            }
        },
        [&](HorizontalSplitGrid &horizontal_split){
            auto &[left, right] = horizontal_split.children;
            if (idx == 0){
                split_grid = NoSplitGrid { std::move(right) };
            }
            else if (idx == 1){
                split_grid = NoSplitGrid { std::move(left) };
            }
            else{
                throw std::out_of_range { "Index must be zero or one." };
            }

        },
        [&](QuadrantSplitGrid &quadrant_split){
            const std::size_t notnull_simulation_count = notNullViewCount();

            if (notnull_simulation_count == 3){
                if (quadrant_split.children[idx]){
                    quadrant_split.children[idx].reset();

                    auto notnull_views = quadrant_split.children
                           | ranges::views::filter([](const auto &x) { return x != nullptr; })
                           | ranges::views::move
                           | ranges::to_vector;
                    split_grid = HorizontalSplitGrid { std::move(notnull_views[0]), std::move(notnull_views[1]) };
                }
                else{
                    throw std::invalid_argument { "Given simulation is already empty." };
                }
            }
            else if (notnull_simulation_count == 4){
                if (quadrant_split.children[idx]){
                    quadrant_split.children[idx].reset();
                }
                else{
                    throw std::invalid_argument { "Given simulation is already empty." };
                }
            }
            else{
                throw std::logic_error { "Invalid state." };
            }
        }
    }, split_grid);
}

void SimulationGridView::swap(std::size_t idx1, std::size_t idx2) {
    auto at = [this](std::size_t idx) -> std::shared_ptr<SimulationView>& {
        return std::visit([=](auto &x) -> std::shared_ptr<SimulationView>& { return x.children[idx]; }, split_grid);
    };

    std::swap(at(idx1), at(idx2));
}

void SimulationGridView::update(float time_delta) {
    auto notnull_simulation_views = views()
        | ranges::views::filter([](const auto &x) { return x != nullptr; });
    for (auto &simulation_view : notnull_simulation_views){
        simulation_view->update(time_delta);
    }
}

void SimulationGridView::draw() const {
    std::visit(overload {
        [&](const NoSplitGrid &no_split){
            if (const auto &[child] = no_split.children; child){
                glViewport(position.x, position.y, size.x, size.y);
                child->draw();
            }
        },
        [&](const HorizontalSplitGrid &horizontal_split){
            const auto &[left, right] = horizontal_split.children;
            glViewport(position.x, position.y, size.x / 2, size.y);
            left->draw();
            glViewport(position.x + size.x / 2, position.y, size.x / 2, size.y);
            right->draw();
        },
        [&](const QuadrantSplitGrid &quadrant_split){
            const auto &[top_left, top_right, bottom_left, bottom_right] = quadrant_split.children;
            if (top_left){
                glViewport(position.x, position.y + size.y / 2, size.x / 2, size.y / 2);
                top_left->draw();
            }
            if (top_right){
                glViewport(position.x + size.x / 2, position.y + size.y / 2, size.x / 2, size.y / 2);
                top_right->draw();
            }
            if (bottom_left){
                glViewport(position.x, position.y, size.x / 2, size.y / 2);
                bottom_left->draw();
            }
            if (bottom_right){
                glViewport(position.x + size.x / 2, position.y, size.x / 2, size.y / 2);
                bottom_right->draw();
            }
        }
    }, split_grid);
}

SimulationGridView::SplitMethod SimulationGridView::getSplitMethod() const noexcept {
    return std::visit(overload {
        [](const NoSplitGrid&) { return SplitMethod::NoSplit; },
        [](const HorizontalSplitGrid&) { return SplitMethod::HorizontalSplit; },
        [](const QuadrantSplitGrid&) { return SplitMethod::QuadrantSplit; }
    }, split_grid);
}

SimulationGridView::SimulationGridView(const std::list<std::shared_ptr<SimulationData>> &simulations, glm::vec<2, GLint> position, glm::vec<2, GLsizei> size)
        : simulations { simulations }, position { position }, size { size }
{

}

[[nodiscard]] std::size_t SimulationGridView::notNullViewCount() const noexcept{
    return std::ranges::count_if(views(), [](const auto &view) { return view != nullptr; });
}

[[nodiscard]] float SimulationGridView::getViewportAspectRatio() const noexcept{
    switch (getSplitMethod()){
        case SplitMethod::NoSplit: case SplitMethod::QuadrantSplit:
            return static_cast<float>(size.x) / static_cast<float>(size.y);
        case SplitMethod::HorizontalSplit:
            return static_cast<float>(size.x) / static_cast<float>(size.y) / 2.f;
    }
}

void SimulationGridView::updateImGui(float time_delta) {
    if (ImGui::CollapsingHeader("Simulation arrangement")) {
        const auto set_button = [&](std::size_t idx, ImVec2 button_size) {
            // If simulation is null, button has no displayed content and no context menu.
            // Otherwise, button displays its simulation name and context menu, which have options for deletion.
            auto &simulation = views()[idx];
            auto &data = simulation->simulation_data;
            if (simulation) {
                ImGui::Button(simulation->name.c_str(), button_size);
                ImGui::SetItemTooltip("Right click to open context menu.");

                // Button have context menu, which allows user to delete the simulation.
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::Selectable("Delete")) {
                        removeAt(idx);
                        if (auto valid_data = data.lock()){
                            valid_data->refreshAssociatedViews();
                        }
                        on_layout_changed();
                    }
                    ImGui::EndPopup();
                }
            }
            else {
                ImGui::ScopedDisabled scoped_disabled;
                ImGui::Button("-##btn_null_simulation", button_size);
            }

            // Each simulation's region can be changed by drag and drop, even if the simulation is null (the null
            // position should be able to swapped).
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                ImGui::SetDragDropPayload("DND_DEMO_CELL", &idx, sizeof(decltype(idx)));
                ImGui::TextUnformatted("Move to here");
                ImGui::EndDragDropSource();
            }
            if (ImGui::BeginDragDropTarget()) {
                if (const auto *payload = ImGui::AcceptDragDropPayload("DND_DEMO_CELL")) {
                    assert(payload->DataSize == sizeof(decltype(idx)));
                    const std::size_t payload_idx = *static_cast<const std::size_t *>(payload->Data);
                    swap(idx, payload_idx);
                }
                ImGui::EndDragDropTarget();
            }
        };

        const auto [num_simulations, button_width] = [&]() -> std::pair<std::size_t, float> {
            const float available_content_region_width = ImGui::GetContentRegionAvail().x;
            switch (getSplitMethod()) {
                case SimulationGridView::SplitMethod::NoSplit:
                    return { 1, available_content_region_width };
                case SimulationGridView::SplitMethod::HorizontalSplit:
                    return { 2, available_content_region_width / 2.f };
                case SimulationGridView::SplitMethod::QuadrantSplit:
                    return { 4, available_content_region_width / 2.f };
            }
        }();
        const ImVec2 button_size { button_width, button_width / getViewportAspectRatio() };

        for (std::size_t idx = 0; idx < num_simulations; ++idx) {
            if (idx % 2 == 1) {
                ImGui::SameLine();
            }

            ImGui::ScopedId scoped_id { static_cast<int>(idx) };
            set_button(idx, button_size);
        }

        // Add new simulation view button.
        ImGui::ScopedDisabled scoped_disabled { notNullViewCount() >= 4 }; // max simulation count is 4.
        if (ImGui::SmallButton("+##btn_add_new_simulation_view")) {
            new_simulation_view_dialog.open();
        }
        ImGui::SetItemTooltip("Add new simulation view");

        if (auto result = new_simulation_view_dialog.show(simulations)) {
            add(std::move(*result));
            on_layout_changed();
        }
    }
}
