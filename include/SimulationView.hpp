//
// Created by gomkyung2 on 2023/08/30.
//

#pragma once

#include <GL/glew.h>
#include <NBodyExecutor/Executor.hpp>

struct UniformColorizer {
    glm::vec3 body_color { 0.2f, 0.5f, 1.f };
};

struct SpeedDependentColorizer {
    float speed_low = 0.f;
    float speed_high = 1.f;
    glm::vec3 color_low { 0.f, 0.f, 1.f }; // blue
    glm::vec3 color_high { 1.f, 0.f, 0.f }; // red
};

struct DirectionDependentColorizer {
    float offset = 0.f;
};

class SimulationView{
private:
    std::vector<NBodyExecutor::Body> bodies;
    std::unique_ptr<NBodyExecutor::Executor> executor;

    struct {
        GLuint vao, vbo;
    } pointcloud;

public:
    const std::string name;
    std::variant<UniformColorizer, SpeedDependentColorizer, DirectionDependentColorizer> colorizer = UniformColorizer {};

    SimulationView(std::string name, std::vector<NBodyExecutor::Body> bodies, std::unique_ptr<NBodyExecutor::Executor> executor);
    ~SimulationView() noexcept;

    void update(float time_delta);
    void draw() const;
};