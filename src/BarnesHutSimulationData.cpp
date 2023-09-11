//
// Created by gomkyung2 on 2023/09/11.
//

#include "BarnesHutSimulationData.hpp"

BarnesHutSimulationData::BarnesHutSimulationData(std::string name,
                                                 std::vector <NBodyExecutor::Body> bodies,
                                                 std::unique_ptr <NBodyExecutor::BarnesHutExecutor> executor)
        : SimulationData { std::move(name), std::move(bodies), std::move(executor) }
{
    // Pointcloud VAO is set in base class.
    // Set node box VAO.
    auto barnes_hut_executor = dynamic_cast<NBodyExecutor::BarnesHutExecutor*>(this->executor.get());
    barnes_hut_executor->record_node_boxes = true;

    constexpr std::array<glm::vec3, 8> base_vertices {
        glm::vec3 { 0.f, 0.f, 0.f },
        glm::vec3 { 0.f, 0.f, 1.f },
        glm::vec3 { 0.f, 1.f, 0.f },
        glm::vec3 { 0.f, 1.f, 1.f },
        glm::vec3 { 1.f, 0.f, 0.f },
        glm::vec3 { 1.f, 0.f, 1.f },
        glm::vec3 { 1.f, 1.f, 0.f },
        glm::vec3 { 1.f, 1.f, 1.f }
    };

    constexpr std::array<unsigned char, 24> base_indices {
        0, 1,
        0, 2,
        0, 4,
        1, 3,
        1, 5,
        2, 3,
        2, 6,
        3, 7,
        4, 5,
        4, 6,
        5, 7,
        6, 7
    };

    glGenVertexArrays(1, &node_box.vao);
    glBindVertexArray(node_box.vao);

    glGenBuffers(1, &node_box.base_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, node_box.base_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(base_vertices.size() * sizeof(glm::vec3)),
                 base_vertices.data(),
                 GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribDivisor(0, 0); // base box vbo is used repeatedly for each instance.

    glGenBuffers(1, &node_box.instance_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, node_box.instance_vbo);
    glVertexAttribPointer(1,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(NBodyExecutor::Cube),
                          reinterpret_cast<GLint*>(offsetof(NBodyExecutor::Cube, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1); // cube position must be changed for every instance.
    glVertexAttribPointer(2,
                          1,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(NBodyExecutor::Cube),
                          reinterpret_cast<GLint*>(offsetof(NBodyExecutor::Cube, size)));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1); // cube size must be changed for every instance.

    glGenBuffers(1, &node_box.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, node_box.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(base_indices.size() * sizeof(unsigned char)),
                 base_indices.data(),
                 GL_STATIC_DRAW);
}

BarnesHutSimulationData::~BarnesHutSimulationData() noexcept{
    glDeleteVertexArrays(1, &node_box.vao);
    glDeleteBuffers(1, &node_box.base_vbo);
    glDeleteBuffers(1, &node_box.instance_vbo);
    glDeleteBuffers(1, &node_box.ebo);
}

void BarnesHutSimulationData::update(float time_step) {
    SimulationData::update(time_step);

    auto barnes_hut_executor = dynamic_cast<const NBodyExecutor::BarnesHutExecutor*>(executor.get());
    const auto node_boxes = barnes_hut_executor->getNodeBoxes();
    node_box.num_boxes = node_boxes.size();

    if (node_box.num_boxes <= node_box.allocated_boxes){
        // No need to reallocate the vbo; just rewrite to the existing buffer.
        glBindBuffer(GL_ARRAY_BUFFER, node_box.instance_vbo);
        glBufferSubData(
                GL_ARRAY_BUFFER,
                0,
                static_cast<GLsizeiptr>(node_box.num_boxes * sizeof(NBodyExecutor::Cube)),
                node_boxes.data());
    }
    else{
        // Current box count exceeds the existing buffer size; reallocate the vbo.
        glBindBuffer(GL_ARRAY_BUFFER, node_box.instance_vbo);
        glBufferData(
                GL_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(node_box.num_boxes * sizeof(NBodyExecutor::Cube)),
                node_boxes.data(),
                GL_STREAM_DRAW);
        node_box.allocated_boxes = node_box.num_boxes;
    }
}
