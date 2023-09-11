//
// Created by gomkyung2 on 2023/09/10.
//

#pragma once

#include <list>

#include <GL/glew.h>
#include <NBodyExecutor/Executor.hpp>

struct SimulationData{
    const std::string name;
    std::vector<NBodyExecutor::Body> bodies;
    std::unique_ptr<NBodyExecutor::Executor> executor;

    struct Pointcloud {
        GLuint vao;
        GLuint vbo;
    } pointcloud;

    SimulationData(std::string name, std::vector<NBodyExecutor::Body> bodies, std::unique_ptr<NBodyExecutor::Executor> executor);
    virtual ~SimulationData() noexcept;

    virtual void update(float time_step);
};