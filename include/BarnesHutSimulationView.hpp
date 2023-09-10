//
// Created by gomkyung2 on 2023/09/05.
//

#pragma once

#include "SimulationView.hpp"

#include <NBodyExecutor/BarnesHutExecutor.hpp>

class BarnesHutSimulationView : public SimulationView{
private:
    struct{
        GLuint vao;
        GLuint base_vbo, base_ebo, instance_vbo;
    } nodebox;

public:
    BarnesHutSimulationView(std::string name, std::vector<NBodyExecutor::Body> bodies, std::unique_ptr<NBodyExecutor::BarnesHutExecutor> executor);
    ~BarnesHutSimulationView() noexcept override;

    void update(float time_delta) override;
    void draw() const override;
};