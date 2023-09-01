//
// Created by gomkyung2 on 2023/08/30.
//

#include "BodyPreset.hpp"

#include <random>
#include <algorithm>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/random.hpp>

namespace{
    struct GalaxyDistribution{
        NBodyExecutor::Body operator()(auto &gen) const{
            std::uniform_real_distribution<float> uniform_dis { 0.f, 1.f };
            std::normal_distribution<float> normal_dis { 0.f, 1e-2f };

            const float radius = uniform_dis(gen);
            const float longitude = glm::two_pi<float>() * uniform_dis(gen);
            const glm::vec3 noise { normal_dis(gen), 4.f * normal_dis(gen), normal_dis(gen) };
            const glm::vec3 position = radius * glm::vec3(std::cos(longitude), 0.f, std::sin(longitude)) + noise;

            const glm::vec3 velocity = 0.2f * glm::vec3(position.z, normal_dis(gen), -position.x);

            return { std::lerp(0.8f, 1.2f, uniform_dis(gen)), position, velocity };
        }
    };

    template <typename Func>
    std::vector<std::invoke_result_t<Func>> generate_n(std::size_t n, Func &&f){
        std::vector<std::invoke_result_t<Func>> result;
        result.reserve(n);
        std::generate_n(std::back_inserter(result), n, f);

        return result;
    }
};

std::vector<NBodyExecutor::Body> BodyPreset::galaxy(std::size_t num_bodies) {
    static std::random_device rd;
    static std::mt19937 gen { rd() };
    const GalaxyDistribution dis;

    return generate_n(num_bodies, [&] { return dis(gen); });
}

std::vector<NBodyExecutor::Body> BodyPreset::explosion(std::size_t num_bodies) {
    return generate_n(num_bodies, []{
        glm::vec3 position = glm::ballRand(0.1f);
        return NBodyExecutor::Body { 1.f, position, 0.1f * position };
    });
}
