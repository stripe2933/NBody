//
// Created by gomkyung2 on 2023/08/30.
//

#pragma once

#include <GL/glew.h>
#include <OpenGLApp/Program.hpp>
#include <dirty_property.hpp>

#include "SimulationData.hpp"

struct Colorizer{
    struct Uniform{
        glm::vec4 body_color { 0.2f, 0.5f, 1.f, 1.f };
    };

    struct SpeedDependent{
        std::array<float, 2> speed_range { 0.f, 1.f };
        glm::vec4 color_low { 0.f, 0.f, 1.f, 1.f };
        glm::vec4 color_high { 1.f, 0.f, 0.f, 1.f };
    };

    struct DirectionDependent{
        float offset { 0.f };
    };

    using Type = std::variant<Uniform, SpeedDependent, DirectionDependent>;
};

class SimulationView final : public std::enable_shared_from_this<SimulationView>{
protected:
    std::shared_ptr<SimulationData> simulation_data;

public:
    static struct{
        std::unique_ptr<OpenGL::Program> pointcloud_uniform,
                                         pointcloud_speed_dependent,
                                         pointcloud_direction_dependent;
        std::unique_ptr<OpenGL::Program> node_box;
    } programs;

    const std::string name;
    Colorizer::Type colorizer = Colorizer::Uniform {};

    SimulationView(std::string name, std::shared_ptr<SimulationData> data);

    void update(float time_delta);
    void draw() const;

    std::shared_ptr<SimulationView> getSharedPtr();

    /**
     * @brief Initialize OpenGL programs.
     * @note OpenGL context must be created before calling this function. This function must be called before use of
     * any programs, e.g. uniform setting, drawing.
     */
    static void initPrograms();
};