//
// Created by gomkyung2 on 2023/08/30.
//

#include "App.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glm/gtc/type_ptr.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/enumerate.hpp>

void App::update(float time_delta) {
    if (is_running){
        // Update all notnull SimulationViews.
        simulation_grid.update(time_step.value_or(time_delta));
    }

    // If region is half-split, projection matrix should be calculated with half aspect ratio.
    if (simulation_grid.getSplitMethod() == SimulationGridView::SplitMethod::HorizontalSplit){
        const glm::mat4 view = camera.getView();
        const glm::mat4 projection = camera.getProjection(getAspectRatio() / 2.f);
        pointcloud_program.setUniform("projection_view", projection * view);
    }

    // Update ImGui frame.
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Inspection");
    ImGui::Text("FPS: %.0f", ImGui::GetIO().Framerate);
    if (ImGui::Button(is_running ? "Stop" : "Resume")){
        is_running = !is_running;
    }

    if (ImGui::CollapsingHeader("Global parameters")){
        ImGui::SeparatorText("Camera");
        {
            ImGui::SliderFloat("Scroll sensitivity##camera_scroll_sensitivity", &controlling_properties.scroll_sensitivity, 0.01f, 1.f);
            ImGui::SliderFloat("Pan sensitivity##camera_pan_sensitivity", &controlling_properties.pan_sensitivity, 1e-4f, 1e-2f);
        }

        ImGui::SeparatorText("Simulation");
        {
            ImGui::Text("Time step");

            if (ImGui::RadioButton("Automatic##timestep_automatic", !time_step.has_value())){
                time_step = std::nullopt;
            }

            if (ImGui::RadioButton("Fixed##timestep_fixed", time_step.has_value())){
                time_step = time_delta;
            }
            ImGui::SameLine();

            ImGui::BeginDisabled(!time_step.has_value());
            if (float input_value = time_step.value_or(0.f);
                ImGui::InputFloat("##timestep_fixed_value", &input_value, 1e-3f, 5e-3f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue)){
                time_step = input_value;
            }
            ImGui::EndDisabled();
        }
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
                        }
                        ImGui::EndPopup();
                    }
                } else {
                    ImGui::PushID(static_cast<int>(idx));
                    ImGui::BeginDisabled();
                    ImGui::Button("", button_size);
                    ImGui::EndDisabled();
                    ImGui::PopID();
                }

                // Each simulation's region can be changed by drag and drop, even if the simulation is null (the null position
                // should be able to swapped).
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

            // BUG: split_method is NoSplit, but num_simulations == 2
            const auto [num_simulations, button_size] = [&]() -> std::pair<std::size_t, ImVec2> {
                const float viewport_aspect_ratio = simulation_grid.getViewportAspectRatio();
                const float available_content_region_width = ImGui::GetContentRegionAvail().x;

                switch (simulation_grid.getSplitMethod()) {
                    case SimulationGridView::SplitMethod::NoSplit: {
                        const float width = available_content_region_width;
                        return { 1, { width, width / viewport_aspect_ratio }};
                    }
                    case SimulationGridView::SplitMethod::HorizontalSplit: {
                        const float width = available_content_region_width / 2.f;
                        return { 2, { width, width / viewport_aspect_ratio }};
                    }
                    case SimulationGridView::SplitMethod::QuadrantSplit: {
                        const float width = available_content_region_width / 2.f;
                        return { 4, { width, width / viewport_aspect_ratio }};
                    }
                }
            }();

            for (std::size_t idx = 0; idx < num_simulations; ++idx) {
                if (idx % 2 == 1) {
                    ImGui::SameLine();
                }
                set_button(idx, button_size);
            }
        }

        // Add new simulation button.
        {
            // Simulation cannot be added more than 4.
            ImGui::BeginDisabled(simulation_grid.notNullSimulationCount() >= 4);
            if (ImGui::SmallButton("+##btn_add_new_simulation")) {
                new_simulation_dialog.is_open = true;
            }
            ImGui::SetItemTooltip("Add a new simulation");
            ImGui::EndDisabled();

            if (new_simulation_dialog.is_open) {
                ImGui::OpenPopup("Simulation initial properties");
            }

            if (auto result = new_simulation_dialog.open("Simulation initial properties"); result.has_value()) {
                simulation_grid.add(std::move(result.value()));
            }
        }
    }

    // Simulation-specific properties.
    auto enumerated_notnull_simulations = simulation_grid.simulations()
            | ranges::views::filter([](const auto &x) { return x != nullptr; })
            | ranges::views::enumerate;
    for (auto &&[idx, simulation] : enumerated_notnull_simulations){
        if (ImGui::CollapsingHeader(simulation->name.c_str())){
#ifndef NDEBUG
            int current_colorizer = [&]{
                if (std::holds_alternative<UniformColorizer>(simulation->colorizer)){
                    return 0;
                }
                else if (std::holds_alternative<SpeedDependentColorizer>(simulation->colorizer)){
                    return 1;
                }
                else { // std::holds_alternative<DirectionDependentColorizer>(simulation->colorizer)
                    return 2;
                }
            }();

            // In production, the index of variant type should be same as the above code.
            assert(current_colorizer == simulation->colorizer.index());
#else
            int current_colorizer = simulation->colorizer.index();
#endif

            ImGui::RadioButton("Uniform", &current_colorizer, 0); ImGui::SameLine();
            ImGui::RadioButton("Speed-dependent", &current_colorizer, 1); ImGui::SameLine();
            ImGui::RadioButton("Direction-dependent", &current_colorizer, 2);

            switch (current_colorizer){
                case 0:{
                    if (simulation->colorizer.index() != 0){
                        simulation->colorizer = UniformColorizer{};
                    }

                    auto &uniform_colorizer = std::get<UniformColorizer>(simulation->colorizer);
                    ImGui::ColorPicker3("Color", glm::value_ptr(uniform_colorizer.body_color));

                    break;
                }
                case 1:{
                    if (simulation->colorizer.index() != 1){
                        simulation->colorizer = SpeedDependentColorizer{};
                    }

                    auto &speed_dependent_colorizer = std::get<SpeedDependentColorizer>(simulation->colorizer);
                    ImGui::DragFloatRange2("Speed range",
                                           &speed_dependent_colorizer.speed_low,
                                           &speed_dependent_colorizer.speed_high,
                                           0.01f,
                                           0.f, // speed ≥ 0
                                           std::numeric_limits<float>::max());
                    ImGui::ColorPicker3("Low color", glm::value_ptr(speed_dependent_colorizer.color_low));
                    ImGui::ColorPicker3("High color", glm::value_ptr(speed_dependent_colorizer.color_high));

                    break;
                }
                case 2:{
                    if (simulation->colorizer.index() != 2){
                        simulation->colorizer = DirectionDependentColorizer{};
                    }

                    auto &direction_dependent_colorizer = std::get<DirectionDependentColorizer>(simulation->colorizer);
                    ImGui::DragFloat("Offset (rad)", &direction_dependent_colorizer.offset, 0.01f, 0.f, glm::two_pi<float>());

                    break;
                }
                default:
                    assert(false); // Unreachable.
            }
        }
    }

    ImGui::End();
    ImGui::Render();
}

void App::draw() const {
    const auto framebuffer_size = getFramebufferSize();

    glClear(GL_COLOR_BUFFER_BIT);

    pointcloud_program.use();
    simulation_grid.draw();

    // Restore viewport for ImGui rendering.
    glViewport(0, 0, framebuffer_size.x, framebuffer_size.y);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void App::initImGui() {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

App::App() : OpenGL::Window { initial_window_size.x, initial_window_size.y, "N-Body Simulation" },
             camera { .pitch = 0.2f, .yaw = 0.2f, .distance = 5.f },
             pointcloud_program { "shaders/pointcloud_speed_dependent.vert", "shaders/pointcloud.frag" },
             simulation_grid { { 0, 0 }, { 0, 0 } } // At this point, we do not know the framebuffer size of the window.
                                                    // NOTE. (window size) != (framebuffer size)
{
    initImGui();

    // Now window have correct framebuffer size, so initialize simulation grid with correct size.
    simulation_grid.size = getFramebufferSize();

    const glm::mat4 view = camera.getView();
    const glm::mat4 projection = camera.getProjection(getAspectRatio());

    pointcloud_program.use();
    pointcloud_program.setUniform("projection_view", projection * view);
    pointcloud_program.setUniform("mass_factor", 10.f);
    pointcloud_program.setUniform("mass_constant", 0.f);
    pointcloud_program.setUniform("speed_low", 0.f);
    pointcloud_program.setUniform("speed_high", 0.1f);
    pointcloud_program.setUniform("color_low", glm::vec3(1.f, 0.f, 0.f));
    pointcloud_program.setUniform("color_high", glm::vec3(0.f, 1.f, 0.f));
//    pointcloud_program.setUniform("offset", 0.f);

    glEnable(GL_PROGRAM_POINT_SIZE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
}

App::~App() noexcept {
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
    if (io.WantCaptureMouse){
        return;
    }

    camera.distance = std::fmax(
        std::exp(controlling_properties.scroll_sensitivity * static_cast<float>(-yoffset)) * camera.distance,
        camera.min_distance
    );
    onCameraChanged();
}

void App::onMouseButtonChanged(int button, int action, int mods) {
    ImGuiIO &io = ImGui::GetIO();
    io.AddMouseButtonEvent(button, action);
    if (io.WantCaptureMouse){
        return;
    }

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS){
        double mouse_x, mouse_y;
        glfwGetCursorPos(window, &mouse_x, &mouse_y);

        previous_mouse_position = glm::vec2(mouse_x, mouse_y);
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE){
        previous_mouse_position = std::nullopt;
    }
    onCameraChanged();
}

void App::onCursorPosChanged(double xpos, double ypos) {
    const glm::vec2 position { xpos, ypos };

    ImGuiIO &io = ImGui::GetIO();
    io.AddMousePosEvent(position.x, position.y);
    if (io.WantCaptureMouse){
        return;
    }

    if (previous_mouse_position.has_value()) {
        const glm::vec2 offset = controlling_properties.pan_sensitivity * (position - previous_mouse_position.value());
        previous_mouse_position = position;

        camera.yaw += offset.x;
        camera.addPitch(-offset.y);

        onCameraChanged();
    }
}

void App::onCameraChanged() {
    const auto camera_front = camera.getFront();
    const auto camera_right = glm::normalize(glm::cross(camera_front, OpenGL::Camera::up));
    const auto camera_up = glm::normalize(glm::cross(camera_right, camera_front));
    pointcloud_program.setUniform("camera_front", camera_front);
    pointcloud_program.setUniform("camera_up", camera_up);

    const auto view = camera.getView();
    const auto projection = camera.getProjection(getAspectRatio());
    pointcloud_program.setUniform("projection_view", projection * view);
}
