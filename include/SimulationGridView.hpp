//
// Created by gomkyung2 on 2023/09/01.
//

#pragma once

#include <glm/vec2.hpp>

#include "SimulationView.hpp"
#include "Dialogs/NewSimulationViewDialog.hpp"

class SimulationGridView{
public:
    enum class SplitMethod{
        NoSplit,         // View is placed in full region.
        HorizontalSplit, // Two views are placed in left and right region. 0 -> left, 1 -> right
        QuadrantSplit,   // Four views are placed in quadrant regions. 0 -> top-left, 1 -> top-right, 2 -> bottom-left, 3 -> bottom-right
    };

private:
    template <std::size_t N> requires (N == 1 || N == 2 || N == 4)
    class SplitGrid {
    private:
        std::array<std::shared_ptr<SimulationView>, N> children;

    public:
        explicit SplitGrid(auto &&...args) : children { std::forward<decltype(args)>(args)... } {
#ifndef NDEBUG
            const std::size_t notnull_count = [this]{
                if constexpr (N == 1){
                    return (std::get<0>(children) != nullptr);
                }
                else if constexpr (N == 2){
                    return (std::get<0>(children) != nullptr) + (std::get<1>(children) != nullptr);
                }
                else if constexpr (N == 4){
                    return (std::get<0>(children) != nullptr) + (std::get<1>(children) != nullptr) + (std::get<2>(children) != nullptr) + (std::get<3>(children) != nullptr);
                }
            }();

            assert(
                (N == 1 && (notnull_count == 0 || notnull_count == 1)) || // NoSplitGrid can have either null or notnull simulation.
                (N == 2 && notnull_count == 2) || // HorizontalSplitGrid must have 2 notnull views.
                (N == 4 && (notnull_count >= 0 && notnull_count <= 3)) // QuadrantSplitGrid should have at least 3 notnull views.
            );
#endif
        }

        friend class SimulationGridView;
    };

    using NoSplitGrid = SplitGrid<1>;
    using HorizontalSplitGrid = SplitGrid<2>;
    using QuadrantSplitGrid = SplitGrid<4>;

    std::variant<NoSplitGrid, HorizontalSplitGrid, QuadrantSplitGrid> split_grid = NoSplitGrid { nullptr };
    const std::list<std::shared_ptr<SimulationData>> &simulations;

//    [[nodiscard]] std::span<std::shared_ptr<SimulationView>> views() noexcept;

public:
    glm::vec<2, GLint> position;
    glm::vec<2, GLsizei> size;

    // A function executed when layout is changed. You can register your own callback to this function.
    std::function<void()> on_layout_changed;

    NewSimulationViewDialog new_simulation_view_dialog;

    SimulationGridView(const std::list<std::shared_ptr<SimulationData>> &simulations, glm::vec<2, GLint> position, glm::vec<2, GLsizei> size);

    [[nodiscard]] std::span<const std::shared_ptr<SimulationView>> views() const noexcept;

    [[nodiscard]] SplitMethod getSplitMethod() const noexcept;
    [[nodiscard]] std::size_t notNullViewCount() const noexcept;
    [[nodiscard]] float getViewportAspectRatio() const noexcept;

    /**
     * @brief Add simulation view to the simulation grid view.
     * @param simulation_view \p unique_ptr of SimulationView.
     * @throws std::out_of_range if it already has 4 views.
     * @note It automatically change the internal \p split_grid to rearrange the views. The spilt method and state
     * can be only changed via \p add, \p removeAt, \p swap.
     */
    void add(std::shared_ptr<SimulationView> simulation_view);

    /**
     * @brief Remove the simulation view at the given index.
     * @param idx Index to remove at.
     * @throws std::out_of_range if the index is out of range. Valid index is different for each split method:
     * (\p NoSplit -> 0, \p HorizontalSplit -> 0, 1, \p QuadrantSplit -> 0, 1, 2, 3).
     * @throws std::invalid_argument if the simulation at the given index is already empty.
     */
    void removeAt(std::size_t idx);

    /**
     * @brief Swap the simulation at the given indices.
     */
    void swap(std::size_t idx1, std::size_t idx2);

    /**
     * @brief Update all notnull simulation views in the grid.
     * @param time_delta Time delta to update.
     */
    void update(float time_delta);

    void updateImGui(float time_delta);

    /**
     * @brief Draw all notnull simulation views in the grid.
     *
     * Since each viewport of \p SimulationView is varied by the current split method, it depends on the position and the
     * size of the region.
     *
     * @note
     * It does not restore the original OpenGL viewport, so you should call \p glViewport after calling this function.
     */
    void draw() const;
};