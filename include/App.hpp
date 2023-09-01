//
// Created by gomkyung2 on 2023/08/30.
//

#pragma once

#include <OpenGLApp/Window.hpp>
#include <OpenGLApp/Program.hpp>
#include <OpenGLApp/Camera.hpp>

#include "SimulationView.hpp"

template <std::size_t N>
using SplitRegion = std::array<std::unique_ptr<SimulationView>, N>;

using FullRegion = SplitRegion<1>;
using HalfSplitRegion = SplitRegion<2>;
using QuarterSplitRegion = SplitRegion<4>;

class App : public OpenGL::Window{
private:
    std::optional<glm::vec2> previous_mouse_position;
    const struct{
        float scroll_sensitivity = 0.1f;
        float pan_sensitivity = 3e-3f;
        float speed = 2.5f;
    } camera_properties;

    std::variant<FullRegion, HalfSplitRegion, QuarterSplitRegion> regions;
    OpenGL::Camera camera;
    OpenGL::Program pointcloud_program;

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