//
// Created by gomkyung2 on 2023/08/30.
//

#include "App.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <NBodyExecutor/BarnesHutExecutor.hpp>
#include <NBodyExecutor/NaiveExecutor.hpp>

#include "BodyPreset.hpp"

void App::update(float time_delta) {
    // Update all notnull SimulationViews.
    std::visit([=](auto &region) {
        auto notnull_simulations = region | std::views::filter([](auto &view) { return view != nullptr; });
        for (auto &simulation : notnull_simulations) {
            simulation->update(time_delta);
        }
    }, regions);

    // If region is HalfSplitRegion, projection matrix should be calculated with half aspect ratio.
    if (std::holds_alternative<HalfSplitRegion>(regions)){
        const glm::mat4 view = camera.getView();
        const glm::mat4 projection = camera.getProjection(getAspectRatio() / 2.f);
        pointcloud_program.setUniform("projection_view", projection * view);
    }

    // Update ImGui frame.
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::ShowDemoWindow();

    ImGui::Render();
}

void App::draw() const {
    const auto framebuffer_size = getFramebufferSize();

    glClear(GL_COLOR_BUFFER_BIT);

    pointcloud_program.use();
    if (auto *full_region = std::get_if<FullRegion>(&regions)){
        if (std::get<0>(*full_region)){
            std::get<0>(*full_region)->draw();
        }
    }
    else if (auto *half_split_region = std::get_if<HalfSplitRegion>(&regions)){
        if (const auto &left = std::get<0>(*half_split_region); left){
            glViewport(0, 0, framebuffer_size.x / 2, framebuffer_size.y);
            left->draw();
        }
        if (const auto &right = std::get<1>(*half_split_region); right){
            glViewport(framebuffer_size.x / 2, 0, framebuffer_size.x / 2, framebuffer_size.y);
            right->draw();
        }
    }
    else{
        assert(std::holds_alternative<QuarterSplitRegion>(regions));
        auto &quarter_split_region = std::get<QuarterSplitRegion>(regions);

        // Order: top-left, top-right, bottom-left, bottom-right
        for (const auto row : { 0, 1 }){
            const GLint offset_y = row == 1 ? 0 : (framebuffer_size.y / 2);
            for (const auto col : { 0, 1 }){
                const GLint offset_x = col == 1 ? (framebuffer_size.x / 2) : 0;

                if (const auto& simulation = quarter_split_region[2 * row + col]; simulation){
                    glViewport(offset_x, offset_y, framebuffer_size.x / 2, framebuffer_size.y / 2);
                    simulation->draw();
                }
            }
        }
    }

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

App::App() : OpenGL::Window { 1280, 720, "N-Body Simulation" },
             camera { .pitch = 0.2f, .yaw = 0.2f, .distance = 5.f },
             pointcloud_program { "shaders/pointcloud_direction_dependent.vert", "shaders/pointcloud.frag" }
{
    initImGui();

    auto default_bodies = BodyPreset::explosion(256);
    regions = HalfSplitRegion {
        std::make_unique<SimulationView>(std::vector { default_bodies }, std::make_unique<NBodyExecutor::NaiveExecutor>()),
        std::make_unique<SimulationView>(std::move(default_bodies), std::make_unique<NBodyExecutor::BarnesHutExecutor>())
    };

    const glm::mat4 view = camera.getView();
    const glm::mat4 projection = camera.getProjection(getAspectRatio());

    pointcloud_program.use();
    pointcloud_program.setUniform("projection_view", projection * view);
    pointcloud_program.setUniform("mass_factor", 10.f);
    pointcloud_program.setUniform("mass_constant", 0.f);
//    pointcloud_program.setUniform("speed_low", 0.f);
//    pointcloud_program.setUniform("speed_high", 0.3f);
//    pointcloud_program.setUniform("color_low", glm::vec3(1.f, 0.f, 0.f));
//    pointcloud_program.setUniform("color_high", glm::vec3(0.f, 1.f, 0.f));
    pointcloud_program.setUniform("offset", 0.f);

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
    onCameraChanged();
}

void App::onScrollChanged(double xoffset, double yoffset) {
    ImGuiIO &io = ImGui::GetIO();
    io.AddMouseWheelEvent(static_cast<float>(xoffset), static_cast<float>(yoffset));
    if (io.WantCaptureMouse){
        return;
    }

    camera.distance = std::fmax(
            std::exp(camera_properties.scroll_sensitivity * static_cast<float>(-yoffset)) * camera.distance,
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
        const glm::vec2 offset = camera_properties.pan_sensitivity * (position - previous_mouse_position.value());
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
