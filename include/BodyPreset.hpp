//
// Created by gomkyung2 on 2023/08/30.
//

#pragma once

#include <vector>
#include <random>

#include <NBodyExecutor/Body.hpp>
#include <glm/gtc/constants.hpp>

namespace BodyPreset{
    static std::random_device rd;

    struct BallDistribution{
        std::uniform_real_distribution<float> dis { 0.f, 1.f };

        glm::vec3 operator()(auto &gen){
            const float longitude = std::lerp(0.f, glm::two_pi<float>(), dis(gen));
            const float latitude = std::lerp(-glm::half_pi<float>(), glm::half_pi<float>(), dis(gen));
            return { std::cos(latitude) * std::cos(longitude), std::cos(latitude) * std::sin(longitude), std::sin(latitude) };
        }
    };

    struct Galaxy{
        std::uniform_real_distribution<float> uniform_dis { 0.f, 1.f };
        std::normal_distribution<float> normal_dis { 0.f, 1e-2f };

        glm::vec3 operator()(auto &gen){
            const float radius = uniform_dis(gen);
            const float angle = glm::two_pi<float>() * uniform_dis(gen);
            const glm::vec3 noise { normal_dis(gen), 4.f * normal_dis(gen), normal_dis(gen) };
            return { radius * glm::vec3(std::cos(angle), 0.f, std::sin(angle)) + noise };
        }
    };

    std::vector<NBodyExecutor::Body> galaxy(std::size_t num_bodies, unsigned int seed);
    std::vector<NBodyExecutor::Body> explosion(std::size_t num_bodies, unsigned int seed);
    std::vector<NBodyExecutor::Body> galaxy_pair(
            int num_bodies1, int num_bodies2,
            glm::vec3 cm_position1, glm::vec3 cm_position2,
            glm::vec3 cm_velocity1, glm::vec3 cm_velocity2,
            unsigned int seed);

//    /**
//     * Generate bodies with given distribution and random seed.
//     * @param n Number of bodies to generate.
//     * @param mass_distribution Statistical distribution for mass.
//     * @param shape_distribution Statistical distribution for position and velocity of bodies.
//     * @return Random bodies with given size.
//     * @note The seed will generated with internal \p std::random_device .
//     */
//    std::vector<NBodyExecutor::Body> generate(std::size_t n, auto &&mass_distribution, auto &&shape_distribution);
//
//    /**
//     * Generate bodies with given distribution.
//     * @param n Number of bodies to generate.
//     * @param mass_distribution Statistical distribution for mass.
//     * @param shape_distribution Statistical distribution for position and velocity of bodies.
//     * @param seed Seed for random generation, would be passed to \p std::mt19937 .
//     * @return Random bodies with given size.
//     */
//    std::vector<NBodyExecutor::Body> generate(std::size_t n, auto &&mass_distribution, auto &&shape_distribution, unsigned int seed);
}