//
// Created by gomkyung2 on 2023/08/30.
//

#include "App.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glm/gtc/type_ptr.hpp>

#include "ImGui/VariantSelector.hpp"
#include "Utils/VisitorHelper.hpp"

IMGUI_LABEL(Options::TimeStep::Fixed, "Fixed##time_step_fixed");
IMGUI_LABEL(Options::TimeStep::Automatic, "Automatic##time_step_automatic");

IMGUI_LABEL(Options::PointSize::Fixed, "Fixed##point_size_fixed");
IMGUI_LABEL(Options::PointSize::MassProportional, "Proportional to mass##point_size_mass_proportional");

IMGUI_LABEL(Colorizer::Uniform, "Uniform");
IMGUI_LABEL(Colorizer::SpeedDependent, "Speed-dependent");
IMGUI_LABEL(Colorizer::DirectionDependent, "Direction-dependent");

constexpr auto simple_colorpicker_flags = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoInputs;

void App::update(float time_delta) {
    if (is_running) {
        // Update all notnull SimulationViews.
        const float time_step = std::visit(
            Utils::overload {
                [](const Options::TimeStep::Fixed &fixed) { return fixed.time_step; },
                [=](const Options::TimeStep::Automatic &) { return time_delta; }
            },
            time_step_method
        );
        simulation_grid.update(time_step);
    }

    if (point_size_uniform.is_dirty){
        glBindBuffer(GL_UNIFORM_BUFFER, point_size_ubo);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(PointSizeUniform), &point_size_uniform.value);
        point_size_uniform.is_dirty = false;
    }

    if (projection_view_uniform.is_dirty){
        glBindBuffer(GL_UNIFORM_BUFFER, projection_view_ubo);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ProjectionViewUniform), &projection_view_uniform.value);
        projection_view_uniform.is_dirty = false;
    }

    updateImGui(time_delta);
}

void App::draw() const {
    glClear(GL_COLOR_BUFFER_BIT);

    simulation_grid.draw();

    // Restore viewport for ImGui rendering.
    const auto framebuffer_size = getFramebufferSize();
    glViewport(0, 0, framebuffer_size.x, framebuffer_size.y);

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
             simulation_grid { { 0, 0 }, { 0, 0 } } // At this point, we do not know the framebuffer size of the window.
{
    initImGui();

    // Now window have correct framebuffer size, so initialize simulation grid with correct size.
    simulation_grid.size = getFramebufferSize();

    camera.addPitch(0.2f);
    camera.addYaw(0.2f);
    camera.distance = 5.f;

    const glm::mat4 view = camera.getView();
    const glm::mat4 projection = camera.getProjection(simulation_grid.getViewportAspectRatio());
    projection_view_uniform = ProjectionViewUniform { projection * view };

    SimulationView::initPrograms();

    // Set Uniform Buffer Objects.
    glGenBuffers(1, &projection_view_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, projection_view_ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(ProjectionViewUniform), &projection_view_uniform, GL_DYNAMIC_DRAW);

    SimulationView::programs.pointcloud_uniform->setUniformBlockBinding("ProjectionView", 0);
    SimulationView::programs.pointcloud_speed_dependent->setUniformBlockBinding("ProjectionView", 0);
    SimulationView::programs.pointcloud_direction_dependent->setUniformBlockBinding("ProjectionView", 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, projection_view_ubo);

    glGenBuffers(1, &point_size_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, point_size_ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(PointSizeUniform), &point_size_uniform, GL_DYNAMIC_DRAW);

    SimulationView::programs.pointcloud_uniform->setUniformBlockBinding("PointSize", 1);
    SimulationView::programs.pointcloud_speed_dependent->setUniformBlockBinding("PointSize", 1);
    SimulationView::programs.pointcloud_direction_dependent->setUniformBlockBinding("PointSize", 1);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, point_size_ubo);

    glEnable(GL_PROGRAM_POINT_SIZE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
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

    onCameraChanged();
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

    if (previous_mouse_position.has_value()) {
        const glm::vec2 offset = control.pan_sensitivity * (position - previous_mouse_position.value());
        previous_mouse_position = position;

        camera.addYaw(offset.x);
        camera.addPitch(-offset.y);

        onCameraChanged();
    }
}

void App::onCameraChanged() {
    const glm::mat4 view = camera.getView();
    const glm::mat4 projection = camera.getProjection(simulation_grid.getViewportAspectRatio());

    projection_view_uniform = ProjectionViewUniform { projection * view };
}

void App::updateImGui(float time_delta) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Inspection");
    ImGui::Text("FPS: %.0f", ImGui::GetIO().Framerate);
    if (ImGui::Button(is_running ? "Stop##btn_running_state" : "Resume##btn_running_state")) {
        is_running = !is_running;
    }

    if (ImGui::CollapsingHeader("Global parameters")) {
        ImGui::SeparatorText("Camera");

        ImGui::SliderFloat("Scroll sensitivity##camera_scroll_sensitivity", &control.scroll_sensitivity, 0.01f, 1.f);
        ImGui::SliderFloat("Pan sensitivity##camera_pan_sensitivity", &control.pan_sensitivity, 1e-4f, 1e-2f);

        ImGui::SeparatorText("Simulation");

        ImGui::Text("Time step");
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

        ImGui::Text("Point size");

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
                            point_size_uniform.is_dirty = true;
                        }
                    }
                },
                [&] {
                    return Options::PointSize::Fixed { .radius = 10.f };
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
                            point_size_uniform.is_dirty = true;
                        }
                    }
                },
                [&] {
                    return Options::PointSize::MassProportional { .coefficient = 1.2f };
                }
            }
        );
    }

    if (ImGui::CollapsingHeader("Simulation arrangement")) {
        // Simulation grid.
        {
            // https://github.com/ocornut/imgui/blob/master/imgui_demo.cpp line 2315
            const auto set_button = [&](std::size_t idx, ImVec2 button_size) {
                // If simulation is null, button has no displayed content and no context menu.
                // Otherwise, button displays its simulation name and context menu, which have options for deletion.
                const auto &simulation = simulation_grid[idx];
                if (simulation) {
                    ImGui::PushID(static_cast<int>(idx));
                    if (ImGui::Button(simulation->name.c_str(), button_size)) {
                        // TODO: click button to go to the simulation property.
                    }
                    ImGui::SetItemTooltip("Click to go to the simulation property, right click to showDialog context menu.");
                    ImGui::PopID();

                    // Button have context menu, which allows user to delete the simulation.
                    if (ImGui::BeginPopupContextItem()) {
                        if (ImGui::Selectable("Delete")) {
                            simulation_grid.removeAt(idx);
                            projection_view_uniform.value = ProjectionViewUniform { camera.getProjection(simulation_grid.getViewportAspectRatio()) * camera.getView() };
                            projection_view_uniform.is_dirty = true;
                        }
                        ImGui::EndPopup();
                    }
                }
                else {
                    // TODO: make button greyed out when disabled.

                    ImGui::PushID(static_cast<int>(idx));
                    ImGui::BeginDisabled();
                    ImGui::Button("", button_size);
                    ImGui::EndDisabled();
                    ImGui::PopID();
                }

                // Each simulation's region can be changed by drag and drop, even if the simulation is null (the null
                // position should be able to swapped).
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                    ImGui::SetDragDropPayload("DND_DEMO_CELL", &idx, sizeof(decltype(idx)));
                    ImGui::Text("Move to here");
                    ImGui::EndDragDropSource();
                }
                if (ImGui::BeginDragDropTarget()) {
                    if (const auto *payload = ImGui::AcceptDragDropPayload("DND_DEMO_CELL")) {
                        assert(payload->DataSize == sizeof(decltype(idx)));
                        const std::size_t payload_idx = *static_cast<const std::size_t *>(payload->Data);
                        simulation_grid.swap(idx, payload_idx);
                    }
                    ImGui::EndDragDropTarget();
                }
            };

            const auto [num_simulations, button_width] = [&]() -> std::pair<std::size_t, float> {
                const float available_content_region_width = ImGui::GetContentRegionAvail().x;
                switch (simulation_grid.getSplitMethod()) {
                    case SimulationGridView::SplitMethod::NoSplit:
                        return { 1, available_content_region_width };
                    case SimulationGridView::SplitMethod::HorizontalSplit:
                        return { 2, available_content_region_width / 2.f };
                    case SimulationGridView::SplitMethod::QuadrantSplit:
                        return { 4, available_content_region_width / 2.f };
                }
            }();
            const ImVec2 button_size { button_width, button_width / simulation_grid.getViewportAspectRatio() };

            for (std::size_t idx = 0; idx < num_simulations; ++idx) {
                if (idx % 2 == 1) {
                    ImGui::SameLine();
                }
                set_button(idx, button_size);
            }
        }

        // Add new simulation button.
        ImGui::BeginDisabled(simulation_grid.notNullSimulationCount() >= 4); // max simulation count is 4.
        if (ImGui::SmallButton("+##btn_add_new_simulation")) {
            new_simulation_dialog.is_open = true;
        }
        ImGui::SetItemTooltip("Add a new simulation");
        ImGui::EndDisabled();

        if (new_simulation_dialog.is_open) {
            ImGui::OpenPopup("Simulation property");
        }

        if (auto result = new_simulation_dialog.open("Simulation property"); result.has_value()) {
            simulation_grid.add(std::move(result.value()));
            projection_view_uniform = ProjectionViewUniform { camera.getProjection(simulation_grid.getViewportAspectRatio()) * camera.getView() };
            projection_view_uniform.is_dirty = true;
        }
    }

    // Simulation-specific properties.
    for (auto &simulation: simulation_grid.simulations()) {
        if (simulation == nullptr){
            continue;
        }

        if (ImGui::CollapsingHeader(simulation->name.c_str())) {
            ImGui::VariantSelector::radio(
                simulation->colorizer,
                std::pair {
                    [](Colorizer::Uniform &uniform) {
                        if (glm::vec body_color_input = uniform.body_color.value;
                            ImGui::ColorPicker3("Color", glm::value_ptr(body_color_input), simple_colorpicker_flags)
                        ){
                            if (body_color_input != uniform.body_color.value){
                                uniform.body_color = body_color_input;
                                uniform.body_color.is_dirty = true;
                            }
                        }
                    },
                    [] { return Colorizer::Uniform { }; }
                },
                std::pair {
                    [&](Colorizer::SpeedDependent &speed_dependent) {
                        if (float speed_low_input = speed_dependent.speed_low.value, speed_high_input = speed_dependent.speed_high.value;
                            ImGui::DragFloatRange2("Speed range", &speed_low_input, &speed_high_input, 0.01f, 0.f /* speed ≥ 0 */, std::numeric_limits<float>::max())
                        ){
                            if (speed_low_input != speed_dependent.speed_low.value){
                                speed_dependent.speed_low = speed_low_input;
                                speed_dependent.speed_low.is_dirty = true;
                            }
                            if (speed_high_input != speed_dependent.speed_high.value){
                                speed_dependent.speed_high = speed_high_input;
                                speed_dependent.speed_high.is_dirty = true;
                            }
                        }
                        if (glm::vec3 color_low_input = speed_dependent.color_low.value;
                            ImGui::ColorPicker3("Low color", glm::value_ptr(color_low_input), simple_colorpicker_flags)
                        ){
                            if (color_low_input != speed_dependent.color_low.value){
                                speed_dependent.color_low = color_low_input;
                                speed_dependent.color_low.is_dirty = true;
                            }
                        }
                        if (glm::vec3 color_high_input = speed_dependent.color_high.value;
                            ImGui::ColorPicker3("High color", glm::value_ptr(color_high_input), simple_colorpicker_flags)
                        ){
                            if (color_high_input != speed_dependent.color_high.value){
                                speed_dependent.color_high = color_high_input;
                                speed_dependent.color_high.is_dirty = true;
                            }
                        }
                    },
                    [] { return Colorizer::SpeedDependent { }; }
                },
                std::pair {
                    [](Colorizer::DirectionDependent &direction_dependent) {
                        if (float offset_input = direction_dependent.offset.value;
                            ImGui::SliderFloat("Offset (rad)", &offset_input, 0.f, glm::two_pi<float>())
                        ){
                            if (offset_input != direction_dependent.offset.value){
                                direction_dependent.offset = offset_input;
                                direction_dependent.offset.is_dirty = true;
                            }
                        }
                    },
                    [] { return Colorizer::DirectionDependent { }; }
                }
            );
        }
    }

    ImGui::End();
    ImGui::Render();
}
