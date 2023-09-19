//
// Created by gomkyung2 on 2023/08/30.
//

#include "SimulationView.hpp"
#include "BarnesHutSimulationData.hpp"

#include <visitor_helper.hpp>
#include <imgui.h>
#include <imgui_variant_selector.hpp>
#include <glm/gtc/type_ptr.hpp>

IMGUI_LABEL(Colorizer::Uniform, "Uniform");
IMGUI_LABEL(Colorizer::SpeedDependent, "Speed-dependent");
IMGUI_LABEL(Colorizer::DirectionDependent, "Direction-dependent");

decltype(SimulationView::programs) SimulationView::programs {
    .pointcloud_uniform = nullptr,
    .pointcloud_speed_dependent = nullptr,
    .pointcloud_direction_dependent = nullptr,
    .node_box = nullptr
};

SimulationView::SimulationView(std::string name, std::weak_ptr<SimulationData> data)
        : name { std::move(name) }, simulation_data { std::move(data) }
{

}

SimulationView::~SimulationView() noexcept{

}

void SimulationView::update(float time_delta) {

}

void SimulationView::draw() const{
    auto valid_simulation_data = simulation_data.lock();
    if (!valid_simulation_data){
        return;
    }

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

    glBindVertexArray(valid_simulation_data->pointcloud.vao);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(valid_simulation_data->bodies.size()));

    if (const auto barnes_hut_simulation_data = dynamic_cast<const BarnesHutSimulationData*>(valid_simulation_data.get())){
        if (show_node_boxes){
            programs.node_box->use();
            programs.node_box->setUniform("octtree_color", octtree_node_color);

            glBindVertexArray(barnes_hut_simulation_data->node_box.vao);
            glDrawElementsInstanced(
                    GL_LINES,
                    24,
                    GL_UNSIGNED_BYTE,
                    nullptr,
                    static_cast<GLsizei>(barnes_hut_simulation_data->node_box.num_boxes));
        }
    }
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

std::shared_ptr<SimulationView> SimulationView::fromSimulationData(std::string name, std::weak_ptr<SimulationData> data) {
    auto simulation_view = std::make_shared<SimulationView>(std::move(name), std::move(data));
    simulation_view->simulation_data.lock()->associated_views.emplace_back(simulation_view);
    return simulation_view;
}

void SimulationView::updateImGui(float time_delta) {
    if (ImGui::CollapsingHeader(name.c_str())) {
        if (ImGui::TreeNode("Common settings")){
            ImGui::VariantSelector::radio(
                colorizer,
                std::pair {
                    [&](Colorizer::Uniform &colorizer) {
                        ImGui::ColorEdit4("Color", glm::value_ptr(colorizer.body_color));
                    },
                    [] { return Colorizer::Uniform { }; }
                },
                std::pair {
                    [&](Colorizer::SpeedDependent &colorizer) {
                        auto &[low, high] = colorizer.speed_range;
                        ImGui::DragFloatRange2("Speed range",
                                               &low,
                                               &high,
                                               0.01f,
                                               0.f,
                                               std::numeric_limits<float>::max());
                        ImGui::ColorEdit4("Low color", glm::value_ptr(colorizer.color_low));
                        ImGui::ColorEdit4("High color", glm::value_ptr(colorizer.color_high));
                    },
                    [] { return Colorizer::SpeedDependent { }; }
                },
                std::pair {
                    [&](Colorizer::DirectionDependent &colorizer) {
                        ImGui::SliderFloat("Offset (rad)", &colorizer.offset, 0.f, glm::two_pi<float>());
                    },
                    [] { return Colorizer::DirectionDependent { }; }
                }
            );
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Executor specific settings")){
            switch (simulation_data.lock()->executor_type){
                case ExecutorType::Naive:
                    break;
                case ExecutorType::BarnesHut: {
                    ImGui::Checkbox("Show octtree nodes", &show_node_boxes);
                    if (show_node_boxes){
                        ImGui::ColorEdit4("Octtree node color", glm::value_ptr(octtree_node_color));
                    }
                    break;
                }
            }

            ImGui::TreePop();
        }
    }
}

bool SimulationView::isNodeBoxVisible() const {
    return show_node_boxes;
}
