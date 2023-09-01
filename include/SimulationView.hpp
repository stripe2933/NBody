//
// Created by gomkyung2 on 2023/08/30.
//

#pragma once

#include <GL/glew.h>
#include <NBodyExecutor/Executor.hpp>

struct UniformColorizer {
    glm::vec3 body_color;
};

struct SpeedDependentColorizer {
    float speed_low;
    float speed_high;
    glm::vec3 color_low;
    glm::vec3 color_high;
};

struct DirectionDependentColorizer {
    float offset;
};

class SimulationView{
private:
    std::vector<NBodyExecutor::Body> bodies;
    std::unique_ptr<NBodyExecutor::Executor> executor;
    std::variant<UniformColorizer, SpeedDependentColorizer, DirectionDependentColorizer> colorizer;

    struct {
        GLuint vao, vbo;
    } pointcloud;

public:
    SimulationView(std::vector<NBodyExecutor::Body> &&bodies, std::unique_ptr<NBodyExecutor::Executor> executor);
    SimulationView(SimulationView&&) noexcept = default;
    ~SimulationView() noexcept;

    void updateImGui();
    void update(float time_delta);
    void draw() const;
};