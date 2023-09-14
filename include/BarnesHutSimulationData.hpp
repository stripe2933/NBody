//
// Created by gomkyung2 on 2023/09/11.
//

#pragma once

#include <NBodyExecutor/BarnesHutExecutor.hpp>

#include "SimulationData.hpp"

struct BarnesHutSimulationData : public SimulationData {
    struct NodeBox {
        std::size_t num_boxes;
        std::size_t allocated_boxes = 0;

        GLuint vao;
        GLuint base_vbo, instance_vbo, ebo;
    } node_box;

    BarnesHutSimulationData(std::string name, std::vector<NBodyExecutor::Body> bodies, std::unique_ptr<NBodyExecutor::BarnesHutExecutor> executor);
    ~BarnesHutSimulationData() noexcept override;

    void update(float time_step) override;
    void updateImGui(float time_step) override;
};