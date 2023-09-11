//
// Created by gomkyung2 on 2023/09/10.
//

#include "SimulationData.hpp"
#include "NBodyExecutor/BarnesHutExecutor.hpp"

#include <glm/gtc/constants.hpp>

SimulationData::SimulationData(std::string name, std::vector<NBodyExecutor::Body> bodies, std::unique_ptr<NBodyExecutor::Executor> executor)
        : name { std::move(name) }, bodies { std::move(bodies) }, executor { std::move(executor) }
{
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

