//
// Created by gomkyung2 on 2023/08/30.
//

#include "SimulationView.hpp"

#include <glm/gtc/constants.hpp>

#include "Utils/VisitorHelper.hpp"

decltype(SimulationView::programs) SimulationView::programs { .pointcloud_uniform = nullptr, .pointcloud_speed_dependent = nullptr, .pointcloud_direction_dependent = nullptr, .nodebox = nullptr };

SimulationView::SimulationView(std::string name, std::vector<NBodyExecutor::Body> bodies, std::unique_ptr<NBodyExecutor::Executor> executor)
        : name { std::move(name) }, bodies { std::move(bodies) }, executor { std::move(executor) }
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
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(NBodyExecutor::Body), reinterpret_cast<GLint*>(offsetof(NBodyExecutor::Body, velocity)));
}

SimulationView::~SimulationView() noexcept {
    glDeleteBuffers(1, &pointcloud.vbo);
    glDeleteVertexArrays(1, &pointcloud.vao);
}

void SimulationView::update(float time_delta) {
    // Reset each body's acceleration to zero before n-body execution.
    std::ranges::for_each(bodies, [](NBodyExecutor::Body &body) { body.acceleration = glm::zero<glm::vec3>(); });
    executor->execute(bodies, time_delta);

    // Copy data in bodies to GPU. Since allocated data size in GPU is equal to bodies, use glBufferSubData rather
    // than glBufferData.
    glBindBuffer(GL_ARRAY_BUFFER, pointcloud.vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(bodies.size() * sizeof(NBodyExecutor::Body)), bodies.data());

    std::visit(Utils::overload{
        [&](const Colorizer::Uniform& uniform_colorizer){
            if (uniform_colorizer.body_color.is_dirty){
                programs.pointcloud_uniform->setUniform("body_color", uniform_colorizer.body_color.value);
            }
        },
        [&](const Colorizer::SpeedDependent& speed_dependent_colorizer){
            if (speed_dependent_colorizer.speed_low.is_dirty){
                programs.pointcloud_speed_dependent->setUniform("speed_low", speed_dependent_colorizer.speed_low.value);
            }
            if (speed_dependent_colorizer.speed_high.is_dirty){
                programs.pointcloud_speed_dependent->setUniform("speed_high", speed_dependent_colorizer.speed_high.value);
            }
            if (speed_dependent_colorizer.color_low.is_dirty){
                programs.pointcloud_speed_dependent->setUniform("color_low", speed_dependent_colorizer.color_low.value);
            }
            if (speed_dependent_colorizer.color_high.is_dirty){
                programs.pointcloud_speed_dependent->setUniform("color_high", speed_dependent_colorizer.color_high.value);
            }
        },
        [&](const Colorizer::DirectionDependent& direction_dependent_colorizer){
            if (direction_dependent_colorizer.offset.is_dirty){
                programs.pointcloud_direction_dependent->setUniform("offset", direction_dependent_colorizer.offset.value);
            }
        }
    }, colorizer);
}

void SimulationView::draw() const{
    std::visit(Utils::overload{
        [&](const Colorizer::Uniform&){
            programs.pointcloud_uniform->use();
        },
        [&](const Colorizer::SpeedDependent&){
            programs.pointcloud_speed_dependent->use();
        },
        [&](const Colorizer::DirectionDependent&){
            programs.pointcloud_direction_dependent->use();
        }
    }, colorizer);
    glBindVertexArray(pointcloud.vao);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(bodies.size()));
}

void SimulationView::initPrograms() {
    programs.pointcloud_uniform = std::make_unique<OpenGL::Program>(
            "shaders/pointcloud_uniform.vert",
            "shaders/pointcloud.frag"
    );
    programs.pointcloud_speed_dependent = std::make_unique<OpenGL::Program>(
            "shaders/pointcloud_speed_dependent.vert",
            "shaders/pointcloud.frag"
    );
    programs.pointcloud_direction_dependent = std::make_unique<OpenGL::Program>(
            "shaders/pointcloud_direction_dependent.vert",
            "shaders/pointcloud.frag"
    );
}
