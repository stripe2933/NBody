//
// Created by gomkyung2 on 2023/08/30.
//

#include "SimulationView.hpp"

#include <glm/gtc/constants.hpp>

SimulationView::SimulationView(std::vector<NBodyExecutor::Body> &&bodies, std::unique_ptr<NBodyExecutor::Executor> executor)
        : bodies { std::move(bodies) }, executor { std::move(executor) }
{
    glGenVertexArrays(1, &pointcloud.vao);
    glBindVertexArray(pointcloud.vao);

    glGenBuffers(1, &pointcloud.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, pointcloud.vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(this->bodies.size() * sizeof(NBodyExecutor::Body)), this->bodies.data(), GL_STREAM_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, sizeof(NBodyExecutor::Body), reinterpret_cast<GLint*>(offsetof(NBodyExecutor::Body, mass)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(NBodyExecutor::Body), reinterpret_cast<GLint*>(offsetof(NBodyExecutor::Body, position)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(NBodyExecutor::Body), reinterpret_cast<GLint*>(offsetof(NBodyExecutor::Body, velocity)));
}

SimulationView::~SimulationView() noexcept {
    glDeleteBuffers(1, &pointcloud.vbo);
    glDeleteVertexArrays(1, &pointcloud.vao);
}

void SimulationView::updateImGui() {

}

void SimulationView::update(float time_delta) {
    // Reset each body's acceleration to zero before n-body execution.
    std::ranges::for_each(bodies, [](NBodyExecutor::Body &body) { body.acceleration = glm::zero<glm::vec3>(); });
    executor->execute(bodies, time_delta);

    // Copy data in bodies to GPU. Since allocated data size in GPU is equal to bodies, use glBufferSubData rather
    // than glBufferData.
    glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(bodies.size() * sizeof(NBodyExecutor::Body)), bodies.data());
}

void SimulationView::draw() const{
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(bodies.size()));
}