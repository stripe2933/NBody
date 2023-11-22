//
// Created by gomkyung2 on 2023/08/30.
//

#pragma once

#include <list>

#include <OpenGLApp/Window.hpp>
#include <OpenGLApp/Program.hpp>
#include <OpenGLApp/Camera.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <dirty_property.hpp>

#include "SimulationGridView.hpp"
#include "Dialogs/NewSimulationDataDialog.hpp"
#include "Dialogs/ConfirmDeletingSimulationDataDialog.hpp"

namespace Options{
    struct TimeStep{
        struct Fixed { float time_step; }; // Uses fixed time step for entity update.
        struct Automatic{}; // Use the time interval between each \p glfwSwapBuffers() calls.

        using Type = std::variant<Fixed, Automatic>;
    };

    struct PointSize{
        struct Fixed { float radius = 1.f; };
        struct MassProportional { float coefficient = 1.f; };

        using Type = std::variant<Fixed, MassProportional>;
    };
};

struct ProjectionViewUniform{
    glm::mat4 projection_view = glm::identity<glm::mat4>();
};

struct PointSizeUniform{
    float coefficient;
    float constant;

    constexpr bool operator==(const PointSizeUniform &rhs) const noexcept {
        return coefficient == rhs.coefficient && constant == rhs.constant;
    }

    constexpr bool operator!=(const PointSizeUniform &rhs) const noexcept {
        return !(rhs == *this);
    }
};

class App : public OpenGL::Window{
private:
    static constexpr glm::uvec2 initial_window_size { 1280, 720 };

    // OpenGL uniforms.
    GLuint projection_view_ubo, point_size_ubo;
    DirtyProperty<ProjectionViewUniform> projection_view_uniform { ProjectionViewUniform { .projection_view = glm::identity<glm::mat4>() } };
    DirtyProperty<PointSizeUniform> point_size_uniform { PointSizeUniform { .coefficient = 0.f, .constant = 10.f } };

    // OpenGL properties.
    OpenGL::PerspectiveCamera camera;
    DirtyProperty<glm::mat4> view_matrix, projection_matrix;

    // Simulation properties.
    bool is_running = false;
    Options::TimeStep::Type time_step_method = Options::TimeStep::Automatic{};
    Options::PointSize::Type point_size_method = Options::PointSize::Fixed { .radius = 10.f };
    std::list<std::shared_ptr<SimulationData>> simulations; // 1 simulation data can be used in multiple simulation views -> use shared_ptr.
    SimulationGridView simulation_grid;
    std::weak_ptr<SimulationData> selected_simulation;

    // Controlling properties.
    std::optional<glm::vec2> previous_mouse_position;
    struct{
        float scroll_sensitivity = 0.1f;
        float pan_sensitivity = 3e-3f;
    } control;

    // ImGui controls.
    ConfirmDeletingSimulationDataDialog confirm_deleting_simulation_data_dialog;
    NewSimulationDataDialog new_simulation_data_dialog;

    void onFramebufferSizeChanged(int width, int height) override;
    void onScrollChanged(double xoffset, double yoffset) override;
    void onMouseButtonChanged(int button, int action, int mods) override;
    void onCursorPosChanged(double xpos, double ypos) override;

    void update(float time_delta) override;
    void draw() const override;

    void initImGui();
    void updateImGui(float colorizer);
    void onCameraChanged();

public:
    App();
    ~App() noexcept override;
};