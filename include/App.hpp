//
// Created by gomkyung2 on 2023/08/30.
//

#pragma once

#include <OpenGLApp/Window.hpp>
#include <OpenGLApp/Program.hpp>
#include <OpenGLApp/Camera.hpp>

#include "SimulationGridView.hpp"
#include "Dialogs/NewSimulationDialog.hpp"

class App : public OpenGL::Window{
private:
    static constexpr glm::uvec2 initial_window_size { 1280, 720 };

    // OpenGL properties.
    OpenGL::Camera camera;
    OpenGL::Program pointcloud_program;

    // Simulation properties.
    SimulationGridView simulation_grid;
    bool is_running = false;
    std::optional<float> time_step = std::nullopt; // nullopt -> render loop time step, otherwise -> fixed time step.

    // Controlling properties.
    std::optional<glm::vec2> previous_mouse_position;
    struct{
        float scroll_sensitivity = 0.1f;
        float pan_sensitivity = 3e-3f;
    } controlling_properties;

    // ImGui controls.
    Dialog::NewSimulationDialog new_simulation_dialog;

    void onFramebufferSizeChanged(int width, int height) override;
    void onScrollChanged(double xoffset, double yoffset) override;
    void onMouseButtonChanged(int button, int action, int mods) override;
    void onCursorPosChanged(double xpos, double ypos) override;

    void update(float time_delta) override;
    void draw() const override;

    void initImGui();
    void onCameraChanged();

public:
    App();
    ~App() noexcept override;
};