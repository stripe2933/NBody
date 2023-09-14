//
// Created by gomkyung2 on 2023/09/10.
//

#include "SimulationData.hpp"

#include <glm/gtc/constants.hpp>
#include <imgui.h>

#ifndef NDEBUG
#include <NBodyExecutor/NaiveExecutor.hpp>
#include <NBodyExecutor/BarnesHutExecutor.hpp>
#endif

void SimulationData::refreshAssociatedViews() {
    associated_views.remove_if([](const auto &view) {
        return view.expired();
    });
}

SimulationData::SimulationData(ExecutorType executor_type, std::string name, std::vector<NBodyExecutor::Body> bodies, std::unique_ptr<NBodyExecutor::Executor> executor)
        : executor_type { executor_type }, name { std::move(name) }, bodies { std::move(bodies) }, executor { std::move(executor) }
{
#ifndef NDEBUG
    // Assertion for check if executor matches to the executor_type.
    assert(
        (dynamic_cast<const NBodyExecutor::NaiveExecutor*>(this->executor.get()) && executor_type == ExecutorType::Naive) ||
        (dynamic_cast<const NBodyExecutor::BarnesHutExecutor*>(this->executor.get()) && executor_type == ExecutorType::BarnesHut)
    );
#endif

    // Set pointcloud VAO.

    glGenVertexArrays(1, &pointcloud.vao);
    glBindVertexArray(pointcloud.vao);

    glGenBuffers(1, &pointcloud.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, pointcloud.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(this->bodies.size() * sizeof(NBodyExecutor::Body)),
                 this->bodies.data(),
                 GL_STREAM_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,
                          1,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(NBodyExecutor::Body),
                          reinterpret_cast<GLint*>(offsetof(NBodyExecutor::Body, mass)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(NBodyExecutor::Body),
                          reinterpret_cast<GLint*>(offsetof(NBodyExecutor::Body, position)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(NBodyExecutor::Body),
                          reinterpret_cast<GLint*>(offsetof(NBodyExecutor::Body, velocity)));
}

SimulationData::~SimulationData() noexcept {
    glDeleteVertexArrays(1, &pointcloud.vao);
    glDeleteBuffers(1, &pointcloud.vbo);
}

void SimulationData::update(float time_step) {
    std::ranges::for_each(bodies, [](auto &body){ body.acceleration = glm::zero<glm::vec3>(); });
    executor->execute(bodies, time_step);

    // Copy data in bodies to GPU. Since allocated data size in GPU is equal to bodies, use glBufferSubData rather
    // than glBufferData.
    glBindBuffer(GL_ARRAY_BUFFER, pointcloud.vbo);
    glBufferSubData(GL_ARRAY_BUFFER,
                    0,
                    static_cast<GLsizeiptr>(bodies.size() * sizeof(NBodyExecutor::Body)),
                    bodies.data());
}

void SimulationData::updateImGui(float time_delta) {
    ImGui::Text("Name: %s", name.c_str());
    ImGui::Text("Body count: %zu", bodies.size());
}

