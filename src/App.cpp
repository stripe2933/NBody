//
// Created by gomkyung2 on 2023/08/30.
//

#include "App.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_variant_selector.hpp>
#include <visitor_helper.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/reverse.hpp>

#include "ImGui/ScopedId.hpp"

IMGUI_LABEL(Options::TimeStep::Fixed, "Fixed##time_step_fixed");
IMGUI_LABEL(Options::TimeStep::Automatic, "Automatic##time_step_automatic");

IMGUI_LABEL(Options::PointSize::Fixed, "Fixed##point_size_fixed");
IMGUI_LABEL(Options::PointSize::MassProportional, "Proportional to mass##point_size_mass_proportional");

void App::update(float time_delta) {
    if (is_running) {
        // Update all notnull SimulationViews.
        const float time_step = std::visit(
            overload {
                [](const Options::TimeStep::Fixed &fixed) { return fixed.time_step; },
                [=](const Options::TimeStep::Automatic &) { return time_delta; }
            },
            time_step_method
        );

        for (auto &simulation : simulations){
            simulation->update(time_step);
        }
    }

    simulation_grid.update(time_delta);

    DirtyPropertyHelper::clean([&](const glm::mat4 &view, const glm::mat4 &projection){
        projection_view_uniform.mutableValue() = ProjectionViewUniform { .projection_view = projection * view };
        projection_view_uniform.makeDirty();
    }, view_matrix, projection_matrix);

    point_size_uniform.clean([&](const PointSizeUniform &value){
        glBindBuffer(GL_UNIFORM_BUFFER, point_size_ubo);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PointSizeUniform), &value);
    });

    projection_view_uniform.clean([&](const ProjectionViewUniform &value){
        glBindBuffer(GL_UNIFORM_BUFFER, projection_view_ubo);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ProjectionViewUniform), &value);
    });

    updateImGui(time_delta);
}

void App::draw() const {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    simulation_grid.draw();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void App::initImGui() {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

App::App() : OpenGL::Window { initial_window_size.x, initial_window_size.y, "N-Body Simulation" },
             simulation_grid { simulations, { 0, 0 }, { 0, 0 } } // At this point, we do not know the framebuffer size of the window.
{
    initImGui();

    // Now window have correct framebuffer size, so initialize simulation grid with correct size.
    simulation_grid.size = getFramebufferSize();
    simulation_grid.on_layout_changed = [&](){
        projection_matrix = camera.getProjection(simulation_grid.getViewportAspectRatio());
    };

    camera.addPitch(-0.2f);
    camera.addYaw(0.2f);
    camera.distance = 5.f;

    view_matrix = camera.getView();
    projection_matrix = camera.getProjection(simulation_grid.getViewportAspectRatio());

    SimulationView::initPrograms();

    // Set Uniform Buffer Objects.
    glGenBuffers(1, &projection_view_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, projection_view_ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ProjectionViewUniform), nullptr, GL_DYNAMIC_DRAW);

    OpenGL::Program::setUniformBlockBindings(
        "ProjectionView", 0,
        *SimulationView::programs.pointcloud_uniform,
        *SimulationView::programs.pointcloud_speed_dependent,
        *SimulationView::programs.pointcloud_direction_dependent,
        *SimulationView::programs.node_box
    );
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, projection_view_ubo);

    glGenBuffers(1, &point_size_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, point_size_ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(PointSizeUniform), nullptr, GL_DYNAMIC_DRAW);

    OpenGL::Program::setUniformBlockBindings(
        "PointSize", 1,
        *SimulationView::programs.pointcloud_uniform,
        *SimulationView::programs.pointcloud_speed_dependent,
        *SimulationView::programs.pointcloud_direction_dependent
    );
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, point_size_ubo);

    glEnable(GL_PROGRAM_POINT_SIZE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_BUFFER);
}

App::~App() noexcept {
    glDeleteBuffers(1, &projection_view_ubo);
    glDeleteBuffers(1, &point_size_ubo);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void App::onFramebufferSizeChanged(int width, int height) {
    OpenGL::Window::onFramebufferSizeChanged(width, height);
    simulation_grid.size = { width, height };

    projection_matrix = camera.getProjection(simulation_grid.getViewportAspectRatio());
}

void App::onScrollChanged(double xoffset, double yoffset) {
    ImGuiIO &io = ImGui::GetIO();
    io.AddMouseWheelEvent(static_cast<float>(xoffset), static_cast<float>(yoffset));
    if (io.WantCaptureMouse) {
        return;
    }

    camera.distance = std::fmax(
        std::exp(control.scroll_sensitivity * static_cast<float>(-yoffset)) * camera.distance,
        camera.near_distance
    );
    onCameraChanged();
}

void App::onMouseButtonChanged(int button, int action, int mods) {
    ImGuiIO &io = ImGui::GetIO();
    io.AddMouseButtonEvent(button, action);
    if (io.WantCaptureMouse) {
        return;
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            double mouse_x, mouse_y;
            glfwGetCursorPos(window, &mouse_x, &mouse_y);

            previous_mouse_position = glm::vec2(mouse_x, mouse_y);
        } else if (action == GLFW_RELEASE) {
            previous_mouse_position = std::nullopt;
        }
    }
    onCameraChanged();
}

void App::onCursorPosChanged(double xpos, double ypos) {
    const glm::vec2 position { xpos, ypos };

    ImGuiIO &io = ImGui::GetIO();
    io.AddMousePosEvent(position.x, position.y);
    if (io.WantCaptureMouse) {
        return;
    }

    if (previous_mouse_position) {
        const glm::vec2 offset = control.pan_sensitivity * (position - *previous_mouse_position);
        previous_mouse_position = position;

        camera.addYaw(offset.x);
        camera.addPitch(-offset.y);

        onCameraChanged();
    }
}

void App::onCameraChanged() {
    view_matrix = camera.getView();
}

void App::updateImGui(float time_delta) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Inspection");
    ImGui::Text("FPS: %.0f", ImGui::GetIO().Framerate);
    if (ImGui::ScopedId scoped_id { "btn_running_state" }; ImGui::Button(is_running ? "Stop" : "Resume")){
        is_running = !is_running;
    }

    if (ImGui::CollapsingHeader("Global parameters")) {
        ImGui::SeparatorText("Camera");

        ImGui::SliderFloat("Scroll sensitivity##camera_scroll_sensitivity", &control.scroll_sensitivity, 0.01f, 1.f);
        ImGui::SliderFloat("Pan sensitivity##camera_pan_sensitivity", &control.pan_sensitivity, 1e-4f, 1e-2f);

        ImGui::SeparatorText("Simulation");

        ImGui::TextUnformatted("Time step");
        ImGui::VariantSelector::radio(
            time_step_method,
            std::pair {
                [](Options::TimeStep::Fixed &fixed) {
                    ImGui::InputFloat("##timestep_fixed_value",
                                      &fixed.time_step,
                                      1e-3f, 5e-3f,
                                      "%.3f",
                                      ImGuiInputTextFlags_EnterReturnsTrue);
                },
                [=] { return Options::TimeStep::Fixed { .time_step = time_delta }; }
            },
            std::pair {
                [](Options::TimeStep::Automatic &) { },
                [] { return Options::TimeStep::Automatic { }; }
            }
        );

        if (ImGui::TreeNode("Which option should I select?")) {
            ImGui::TextWrapped(
                "Automatic time step can provide smooth results at high FPS, but as the number of rigid "
                "bodies increases, the FPS may decrease, leading to an increase in the time step, which "
                "can result in inaccurate simulation results. Additionally, if there is a long period of "
                "time when results are not being drawn (for example, when the screen is off or the window "
                "size is being changed), the increased time step is used for the next drawing, making it "
                "appear as if the rigid bodies are teleporting. For fewer rigid bodies, choose the "
                "Automatic option, but if you need to adjust the size or use a large number of rigid "
                "bodies, select the Fixed option.");
            ImGui::TreePop();
        }

        ImGui::TextUnformatted("Point size");

        ImGui::VariantSelector::radio(
            point_size_method,
            std::pair {
                [&](Options::PointSize::Fixed &fixed) {
                    if (float input_radius = fixed.radius;
                        ImGui::SliderFloat("Radius##point_size_fixed_radius", &input_radius, 1.f, 10.f))
                    {
                        if (input_radius != fixed.radius){
                            fixed.radius = input_radius;
                            point_size_uniform = PointSizeUniform { .coefficient = 0.f, .constant = fixed.radius };
                        }
                    }
                },
                [&] {
                    return Options::PointSize::Fixed{};
                }
            },
            std::pair {
                [&](Options::PointSize::MassProportional &mass_proportional) {
                    if (float input_coefficient = mass_proportional.coefficient;
                        ImGui::InputFloat("Coefficient##point_size_mass_proportional_coefficient", &input_coefficient))
                    {
                        if (input_coefficient != mass_proportional.coefficient){
                            mass_proportional.coefficient = input_coefficient;
                            point_size_uniform = PointSizeUniform { .coefficient = mass_proportional.coefficient, .constant = 0.f };
                        }
                    }
                },
                [&] {
                    return Options::PointSize::MassProportional{};
                }
            }
        );
    }

    if (ImGui::CollapsingHeader("Simulations")){
        // Show simulation data table.
        if (ImGui::BeginTable("Simulation data table", 3)){
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Body count");
            ImGui::TableSetupColumn("Executor");
            ImGui::TableHeadersRow();

            static std::optional<SimulationData*> erase = std::nullopt;
            for (auto &simulation : simulations){
                ImGui::ScopedId scoped_id { simulation.get() };

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (ImGui::Selectable(simulation->name.c_str(),
                                      simulation == selected_simulation.lock(),
                                      ImGuiSelectableFlags_SpanAllColumns)){
                    selected_simulation = simulation;
                }
                if (ImGui::BeginPopupContextItem()){
                    selected_simulation = simulation;
                    if (ImGui::Selectable("Delete")){
                        // Notify to the user that the simulation has visible simulation view and deleting simulation
                        // will also delete the corresponding simulation views.
                        erase = simulation.get();
                    }
                    ImGui::EndPopup();
                }

                ImGui::TableNextColumn();
                ImGui::Text("%zu", simulation->bodies.size());
                ImGui::TableNextColumn();
                const auto executor_type_string = [&]{
                    switch (simulation->executor_type){
                        case ExecutorType::Naive:
                            return "Naive executor";
                        case ExecutorType::BarnesHut:
                            return "Barnes-Hut executor";
                    }
                }();
                const unsigned int thread_count = simulation->executor->thread_pool ?
                                                  simulation->executor->thread_pool->get_thread_count() : 1;
                ImGui::Text("%s (%u threads)", executor_type_string, thread_count);
            }

            if (erase){
                confirm_deleting_simulation_data_dialog.open();
                if (auto result = confirm_deleting_simulation_data_dialog.show(*erase)){
                    if (*result){ // User pressed OK.
                        // Destroy all associated views to the simulation data.
                        for (auto &erase_view : (*erase)->associated_views){
                            const auto it = std::ranges::find(simulation_grid.views(), erase_view.lock());
                            if (it != simulation_grid.views().end()){
                                const std::size_t erase_index = it - simulation_grid.views().begin();
                                simulation_grid.removeAt(erase_index);
                            }
                        }
                        std::erase_if(simulations, [&](const auto &simulation) { return simulation.get() == *erase; });
                    }
                    // The below line should not be in the above scope, since erase must be set to nullopt for when user
                    // clicked cancel button too (otherwise the dialog will be not closed).
                    erase = std::nullopt;
                }
            }

            ImGui::EndTable();
        }

        // Add new simulation button.
        if (ImGui::SmallButton("+##btn_add_new_simulation")) {
            new_simulation_data_dialog.open();
        }
        ImGui::SetItemTooltip("Add new simulation");

        if (auto result = new_simulation_data_dialog.show()) {
            simulations.emplace_back(std::move(*result));
        }

        // Show selected simulation if presented.
        if (auto valid_selected_simulation = selected_simulation.lock()){
            valid_selected_simulation->updateImGui(time_delta);
        }
    }

    // Show simulation arrangement.
    simulation_grid.updateImGui(time_delta);

    // Simulation-specific properties.
    for (auto &simulation_view : simulation_grid.views() | ranges::views::filter([](const auto &p) { return p != nullptr; })) {
        ImGui::ScopedId scoped_id { simulation_view.get() };
        simulation_view->updateImGui(time_delta);
    }

    ImGui::End();
    ImGui::Render();
}