//
// Created by gomkyung2 on 2023/09/05.
//

#include "BarnesHutSimulationView.hpp"

BarnesHutSimulationView::BarnesHutSimulationView(std::string name, std::vector<NBodyExecutor::Body> bodies, std::unique_ptr<NBodyExecutor::BarnesHutExecutor> executor)
        : SimulationView(std::move(name), std::move(bodies), std::move(executor))
{
    constexpr std::array<float, 24> unit_box_vertices {
        0.f, 0.f, 0.f,
        0.f, 0.f, 1.f,
        0.f, 1.f, 0.f,
        0.f, 1.f, 1.f,
        1.f, 0.f, 0.f,
        1.f, 0.f, 1.f,
        1.f, 1.f, 0.f,
        1.f, 1.f, 1.f
    };

    constexpr std::array<unsigned char, 24> unit_box_indices {
        0, 1,
        0, 2,
        0, 4,
        1, 3,
        1, 5,
        2, 3,
        2, 6,
        3, 7,
        4, 5,
        4, 6,
        5, 7,
        6, 7
    };

    glGenVertexArrays(1, &nodebox.vao);
    glBindVertexArray(nodebox.vao);

    glGenBuffers(1, &nodebox.base_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, nodebox.base_vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(unit_box_vertices.size() * sizeof(float)), unit_box_vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glGenBuffers(1, &nodebox.base_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, nodebox.base_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(unit_box_indices.size() * sizeof(unsigned char)), unit_box_indices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &nodebox.instance_vbo);

    // Every attributes in vertex shader is used.
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    // Set attribute divisor for instancing.
    glVertexAttribDivisor(0, 0); // unit box vbo is used repeatedly for each instance.
    glVertexAttribDivisor(1, 1); // box position and size vbo must be changed for every instance.
}

BarnesHutSimulationView::~BarnesHutSimulationView() noexcept {
    glDeleteVertexArrays(1, &nodebox.vao);
    glDeleteBuffers(1, &nodebox.base_vbo);
    glDeleteBuffers(1, &nodebox.base_ebo);
    glDeleteBuffers(1, &nodebox.instance_vbo);
}

void BarnesHutSimulationView::update(float time_delta) {
    SimulationView::update(time_delta);
}

void BarnesHutSimulationView::draw() const {
    SimulationView::draw();
}
