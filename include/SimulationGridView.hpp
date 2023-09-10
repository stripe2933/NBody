//
// Created by gomkyung2 on 2023/09/01.
//

#pragma once

#include <glm/vec2.hpp>

#include "SimulationView.hpp"

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
        std::array<std::unique_ptr<SimulationView>, N> children;

    public:
        explicit SplitGrid(auto &&...args);

        friend class SimulationGridView;
    };


    using NoSplitGrid = SplitGrid<1>;
    using HorizontalSplitGrid = SplitGrid<2>;
    using QuadrantSplitGrid = SplitGrid<4>;

    std::variant<NoSplitGrid, HorizontalSplitGrid, QuadrantSplitGrid> split_grid = NoSplitGrid { nullptr };

public:
    glm::vec<2, GLint> position;
    glm::vec<2, GLsizei> size;

    SimulationGridView(glm::vec<2, GLint> position, glm::vec<2, GLsizei> size);

    /**
     * Access to the simulation with index.
     * @param idx Index of simulation.
     * @return unique_ptr of SimulationView.
     * @note
     * <tt>getSplitMethod() == NoSplit</tt>, only \p idx = 0 valid.<br>
     * <tt>getSplitMethod() == HorizontalSplit</tt>, \p idx = 0 or 1 valid.<br>
     * <tt>getSplitMethod() == QuadrantSplit</tt>, 0 < \p idx < 4 valid.<br>
     * In debug mode, index assertion is executed.
     */
    const std::unique_ptr<SimulationView> &operator[](std::size_t idx) const;

    [[nodiscard]] std::span<std::unique_ptr<SimulationView>> simulations() noexcept;
    [[nodiscard]] std::span<const std::unique_ptr<SimulationView>> simulations() const noexcept;

    [[nodiscard]] SplitMethod getSplitMethod() const noexcept;
    [[nodiscard]] std::size_t notNullSimulationCount() const noexcept;
    [[nodiscard]] float getViewportAspectRatio() const noexcept;

    /**
     * @brief Add simulation to the simulation grid.
     * @param simulation_view \p unique_ptr of SimulationView.
     * @throws std::out_of_range if it already has 4 simulations.
     * @note It automatically change the internal \p split_grid to rearrange the views. The spilt method and state
     * can be only changed via \p add, \p removeAt, \p swap.
     */
    void add(std::unique_ptr<SimulationView> &&simulation_view);

    /**
     * @brief Remove the simulation at the given index.
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
     * @brief Update all notnull simulations in the grid.
     * @param time_delta Time delta to update.
     */
    void update(float time_delta);

    /**
     * @brief Draw all notnull simulations in the grid.
     *
     * Since each viewport of \p SimulationView is varied by the current split method, it depends on the position and the
     * size of the region.
     *
     * @note
     * It does not restore the original OpenGL viewport, so you should call \p glViewport after calling this function.
     */
    void draw() const;
};

template <std::size_t N> requires (N == 1 || N == 2 || N == 4)
SimulationGridView::SplitGrid<N>::SplitGrid(auto &&...args) : children { std::forward<decltype(args)>(args)... } {
#ifndef NDEBUG
    std::size_t notnull_count = [this]{
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
        (N == 2 && notnull_count == 2) || // HorizontalSplitGrid must have 2 notnull simulations.
        (N == 4 && (notnull_count >= 0 && notnull_count <= 3)) // QuadrantSplitGrid should have at least 3 notnull simulations.
    );
#endif
}