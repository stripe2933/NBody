//
// Created by gomkyung2 on 2023/08/30.
//

#include "SimulationView.hpp"
#include "BarnesHutSimulationData.hpp"

#include <visitor_helper.hpp>

decltype(SimulationView::programs) SimulationView::programs {
    .pointcloud_uniform = nullptr,
    .pointcloud_speed_dependent = nullptr,
    .pointcloud_direction_dependent = nullptr,
    .node_box = nullptr
};

SimulationView::SimulationView(std::string name, std::shared_ptr<SimulationData> data)
        : name { std::move(name) }, simulation_data { std::move(data) }
{

}

void SimulationView::update(float time_delta) {

}

void SimulationView::draw() const{
    // Set program and uniforms.
    // Uniform setting shouldn't be done in update() function, because the same shader for different colorizer
    // is used in draw function.
    std::visit(overload{
        [&](const Colorizer::Uniform &colorizer){
            programs.pointcloud_uniform->use();
            programs.pointcloud_uniform->setUniform("body_color", colorizer.body_color);
        },
        [&](const Colorizer::SpeedDependent &colorizer){
            programs.pointcloud_speed_dependent->use();
            programs.pointcloud_speed_dependent->setUniform("speed_low", std::get<0>(colorizer.speed_range));
            programs.pointcloud_speed_dependent->setUniform("speed_high", std::get<1>(colorizer.speed_range));
            programs.pointcloud_speed_dependent->setUniform("color_low", colorizer.color_low);
            programs.pointcloud_speed_dependent->setUniform("color_high", colorizer.color_high);
        },
        [&](const Colorizer::DirectionDependent &colorizer){
            programs.pointcloud_direction_dependent->use();
            programs.pointcloud_direction_dependent->setUniform("offset", colorizer.offset);
        }
    }, colorizer);

    glBindVertexArray(simulation_data->pointcloud.vao);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(simulation_data->bodies.size()));

    if (auto *barnes_hut_simulation_data = dynamic_cast<const BarnesHutSimulationData*>(simulation_data.get())){
        programs.node_box->use();
        glBindVertexArray(barnes_hut_simulation_data->node_box.vao);
        glDrawElementsInstanced(
                GL_LINES,
                24,
                GL_UNSIGNED_BYTE,
                nullptr,
                static_cast<GLsizei>(barnes_hut_simulation_data->node_box.num_boxes));
    }
}

std::shared_ptr<SimulationView> SimulationView::getSharedPtr() {
    return shared_from_this();
}

void SimulationView::initPrograms() {
    const auto uniform_vertex_shader = OpenGL::Shader::fromFile(
        GL_VERTEX_SHADER,
        "shaders/pointcloud_uniform.vert"
    );
    const auto speed_dependent_vertex_shader = OpenGL::Shader::fromFile(
        GL_VERTEX_SHADER,
        "shaders/pointcloud_speed_dependent.vert"
    );
    const auto direction_dependent_vertex_shader = OpenGL::Shader::fromFile(
        GL_VERTEX_SHADER,
        "shaders/pointcloud_direction_dependent.vert"
    );
    const auto fragment_shader = OpenGL::Shader::fromFile(
        GL_FRAGMENT_SHADER,
        "shaders/pointcloud.frag"
    );

    programs.pointcloud_uniform = std::make_unique<OpenGL::Program>(
        uniform_vertex_shader,
        fragment_shader
    );
    programs.pointcloud_speed_dependent = std::make_unique<OpenGL::Program>(
        speed_dependent_vertex_shader,
        fragment_shader
    );
    programs.pointcloud_direction_dependent = std::make_unique<OpenGL::Program>(
        direction_dependent_vertex_shader,
        fragment_shader
    );

    programs.node_box = std::make_unique<OpenGL::Program>(
        "shaders/node_box.vert",
        "shaders/node_box.frag"
    );
}
