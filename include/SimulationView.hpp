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

/**
 * This class should be linked in its internal simulation data's \p associated_views , therefore you should use
 * \p fromSimulationData() to automatically hold this, not constructors or \p std::make_shared() .
 */
class SimulationView final {
private:
    Colorizer::Type colorizer = Colorizer::Uniform {};

    // Only applied for barnes-hut simulation.
    bool show_node_boxes = true;
    glm::vec4 octtree_node_color { 0.0, 1.0, 0.0, 0.2 };

public:
    static struct{
        std::unique_ptr<OpenGL::Program> pointcloud_uniform,
                                         pointcloud_speed_dependent,
                                         pointcloud_direction_dependent;
        std::unique_ptr<OpenGL::Program> node_box;
    } programs;

    const std::string name;
    const std::weak_ptr<SimulationData> simulation_data;

    SimulationView(std::string name, std::weak_ptr<SimulationData> data); // TODO: this must be in private.
    ~SimulationView() noexcept;

    [[nodiscard]] bool isNodeBoxVisible() const;

    void update(float time_delta);
    void updateImGui(float time_delta);
    void draw() const;

    /**
     * @brief Initialize OpenGL programs.
     * @note OpenGL context must be created before calling this function. This function must be called before use of
     * any programs, e.g. uniform setting, drawing.
     */
    static void initPrograms();

    /**
     * Create \p shared_ptr of SimulationView with given name and simulation data.
     * @param name Name of the simulation view.
     * @param data Simulation data to holds. This should not be null.
     * @return \p std::shared_ptr of SimulationView.
     */
    static std::shared_ptr<SimulationView> fromSimulationData(std::string name, std::weak_ptr<SimulationData> data);
};