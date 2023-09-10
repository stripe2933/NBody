//
// Created by gomkyung2 on 2023/08/30.
//

#pragma once

#include <GL/glew.h>
#include <OpenGLApp/Program.hpp>
#include <NBodyExecutor/Executor.hpp>

#include "Utils/DirtyProperty.hpp"

struct Colorizer{
    struct Uniform{
        DirtyProperty<glm::vec3> body_color = glm::vec3 { 0.2f, 0.5f, 1.f };
    };

    struct SpeedDependent{
        DirtyProperty<float> speed_low = 0.f;
        DirtyProperty<float> speed_high = 1.f;
        DirtyProperty<glm::vec3> color_low = glm::vec3 { 0.f, 0.f, 1.f };
        DirtyProperty<glm::vec3> color_high = glm::vec3 { 1.f, 0.f, 0.f };
    };

    struct DirectionDependent{
        DirtyProperty<float> offset = 0.f;
    };

    using Type = std::variant<Uniform, SpeedDependent, DirectionDependent>;
};

class SimulationView{
protected:
    std::vector<NBodyExecutor::Body> bodies;
    std::unique_ptr<NBodyExecutor::Executor> executor;

    struct {
        GLuint vao, vbo;
    } pointcloud;

public:
    static struct{
        std::unique_ptr<OpenGL::Program> pointcloud_uniform,
                                         pointcloud_speed_dependent,
                                         pointcloud_direction_dependent;
        std::unique_ptr<OpenGL::Program> nodebox;
    } programs;

    const std::string name;
    Colorizer::Type colorizer = Colorizer::Uniform {};

    SimulationView(std::string name, std::vector<NBodyExecutor::Body> bodies, std::unique_ptr<NBodyExecutor::Executor> executor);
    virtual ~SimulationView() noexcept;

    virtual void update(float time_delta);
    virtual void draw() const;

    /**
     * @brief Initialize OpenGL programs.
     * @note OpenGL context must be created before calling this function. This function must be called before use of
     * any programs, e.g. uniform setting, drawing.
     */
    static void initPrograms();
};