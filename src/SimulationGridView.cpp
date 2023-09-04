//
// Created by gomkyung2 on 2023/09/01.
//

#include "SimulationGridView.hpp"

#include <range/v3/view/filter.hpp>
#include <range/v3/view/move.hpp>
#include <range/v3/range/conversion.hpp>

std::span<std::unique_ptr<SimulationView>> SimulationGridView::simulations() noexcept{
    return std::visit([](auto &x) -> std::span<std::unique_ptr<SimulationView>> {
        return { x.children.data(), x.children.size() };
    }, split_grid);
}

std::span<const std::unique_ptr<SimulationView>> SimulationGridView::simulations() const noexcept{
    return std::visit([](const auto &x) -> std::span<const std::unique_ptr<SimulationView>> {
        return { x.children.data(), x.children.size() };
    }, split_grid);
}

const std::unique_ptr<SimulationView> &SimulationGridView::operator[](std::size_t idx) const NOEXCEPT_IF_RELEASE{
    return simulations()[idx];
}

void SimulationGridView::add(std::unique_ptr<SimulationView> &&simulation_view) {
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
     *    simulation (which means 4 simulations are fully presented), then throw std::out_of_range exception.
     */

    if (auto *no_split = std::get_if<NoSplitGrid>(&split_grid)){
        if (auto &[child] = no_split->children; child){
            split_grid = HorizontalSplitGrid { std::move(child), std::move(simulation_view) };
        }
        else{
            child = std::move(simulation_view);
        }
    }
    else if (auto *horizontal_split = std::get_if<HorizontalSplitGrid>(&split_grid)){
        auto &[left, right] = horizontal_split->children;
        split_grid = QuadrantSplitGrid { std::move(left), std::move(right), std::move(simulation_view), nullptr };
    }
    else{
        auto &quadrant_split = std::get<QuadrantSplitGrid>(split_grid);
        const auto insert_position = std::find(quadrant_split.children.begin(), quadrant_split.children.end(), nullptr);
        if (insert_position == quadrant_split.children.cend()){
            throw std::out_of_range { "No available space to add simulation view." };
        }

        *insert_position = std::move(simulation_view);
    }
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
     *      - If there are three simulations, then do nothing.
     */

    if (auto *no_split = std::get_if<NoSplitGrid>(&split_grid)){
        if (idx == 0){
            if (auto &[child] = no_split->children; child){
                child.reset();
            }
            else{
                throw std::invalid_argument { "Given simulation is already empty." };
            }
        }
        else{
            throw std::out_of_range { "Index must be zero." };
        }
    }
    else if (auto *horizontal_split = std::get_if<HorizontalSplitGrid>(&split_grid)){
        auto &[left, right] = horizontal_split->children;
        if (idx == 0){
            split_grid = NoSplitGrid { std::move(right) };
        }
        else if (idx == 1){
            split_grid = NoSplitGrid { std::move(left) };
        }
        else{
            throw std::out_of_range { "Index must be zero or one." };
        }
    }
    else{
        auto &quadrant_split = std::get<QuadrantSplitGrid>(split_grid);
        const std::size_t notnull_simulation_count = notNullSimulationCount();

        if (notnull_simulation_count == 3){
            if (quadrant_split.children[idx]){
                quadrant_split.children[idx].reset();

                auto notnull_simulations_vec = quadrant_split.children
                        | ranges::views::filter([](const auto &x) { return x != nullptr; })
                        | ranges::views::move
                        | ranges::to_vector;
                split_grid = HorizontalSplitGrid { std::move(notnull_simulations_vec[0]), std::move(notnull_simulations_vec[1]) };
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
            assert(false); // should not reach to here.
        }
    }
}

void SimulationGridView::swap(std::size_t idx1, std::size_t idx2) {
    auto at = [this](std::size_t idx) -> std::unique_ptr<SimulationView>& {
        return std::visit([=](auto &x) -> std::unique_ptr<SimulationView>& { return x.children[idx]; }, split_grid);
    };

    std::swap(at(idx1), at(idx2));
}

void SimulationGridView::update(float time_delta) {
    auto notnull_simulations = simulations()
        | ranges::views::filter([](const auto &x) { return x != nullptr; });
    for (auto &simulation : notnull_simulations){
        simulation->update(time_delta);
    }
}

void SimulationGridView::draw() const {
    if (auto *no_split = std::get_if<NoSplitGrid>(&split_grid)){
        if (const auto &[child] = no_split->children; child){
            glViewport(position.x, position.y, size.x, size.y);
            child->draw();
        }
    }
    else if (auto *horizontal_split = std::get_if<HorizontalSplitGrid>(&split_grid)){
        const auto &[left, right] = horizontal_split->children;
        glViewport(position.x, position.y, size.x / 2, size.y);
        left->draw();
        glViewport(position.x + size.x / 2, position.y, size.x / 2, size.y);
        right->draw();
    }
    else{
        const auto &[top_left, top_right, bottom_left, bottom_right] = std::get<QuadrantSplitGrid>(split_grid).children;
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
}

SimulationGridView::SplitMethod SimulationGridView::getSplitMethod() const noexcept {
    if (std::holds_alternative<NoSplitGrid>(split_grid)){
        return SplitMethod::NoSplit;
    }
    else if (std::holds_alternative<HorizontalSplitGrid>(split_grid)){
        return SplitMethod::HorizontalSplit;
    }
    else{
        return SplitMethod::QuadrantSplit;
    }
}

SimulationGridView::SimulationGridView(glm::vec<2, GLint> position, glm::vec<2, GLsizei> size)
        : position { position }, size { size }
{

}

[[nodiscard]] std::size_t SimulationGridView::notNullSimulationCount() const noexcept{
    switch (getSplitMethod()){
        case SplitMethod::NoSplit:
            return (*this)[0] != nullptr;
        case SplitMethod::HorizontalSplit:
            return ((*this)[0] != nullptr) + ((*this)[1] != nullptr);
        case SplitMethod::QuadrantSplit:
            return ((*this)[0] != nullptr) + ((*this)[1] != nullptr) + ((*this)[2] != nullptr) + ((*this)[3] != nullptr);
    }
}

[[nodiscard]] float SimulationGridView::getViewportAspectRatio() const noexcept{
    switch (getSplitMethod()){
        case SplitMethod::NoSplit: case SplitMethod::QuadrantSplit:
            return static_cast<float>(size.x) / static_cast<float>(size.y);
        case SplitMethod::HorizontalSplit:
            return static_cast<float>(size.x) / static_cast<float>(size.y) / 2.f;
    }

    assert(false); // Unreachable.
}
