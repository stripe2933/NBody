//
// Created by gomkyung2 on 2023/09/10.
//

#pragma once

#include <list>

#include <GL/glew.h>
#include <NBodyExecutor/Executor.hpp>


class SimulationView;

enum class ExecutorType{
    Naive,
    BarnesHut
};

struct SimulationData{
    const ExecutorType executor_type;
    const std::string name;
    std::vector<NBodyExecutor::Body> bodies;
    std::unique_ptr<NBodyExecutor::Executor> executor;

    std::list<std::weak_ptr<SimulationView>> associated_views; // The views that are using this simulation data.

    struct Pointcloud {
        GLuint vao;
        GLuint vbo;
    } pointcloud;

    SimulationData(ExecutorType executor_type, std::string name, std::vector<NBodyExecutor::Body> bodies, std::unique_ptr<NBodyExecutor::Executor> executor);
    virtual ~SimulationData() noexcept;

    /**
     * Remove expired views from associated_views.
     */
    void refreshAssociatedViews();

    virtual void update(float time_step);
    virtual void updateImGui(float time_delta);
};