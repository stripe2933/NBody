//
// Created by gomkyung2 on 2023/08/30.
//

#include "BodyPreset.hpp"

#include <algorithm>

#include <glm/gtc/constants.hpp>

namespace{
    struct BallDistribution{
        std::uniform_real_distribution<float> dis { 0.f, 1.f };

        glm::vec3 operator()(auto &gen){
            const float longitude = std::lerp(0.f, glm::two_pi<float>(), dis(gen));
            const float latitude = std::lerp(-glm::half_pi<float>(), glm::half_pi<float>(), dis(gen));
            return { std::cos(latitude) * std::cos(longitude), std::cos(latitude) * std::sin(longitude), std::sin(longitude) };
        }
    };

    struct NoiseDisk{
        std::uniform_real_distribution<float> uniform_dis { 0.f, 1.f };
        std::normal_distribution<float> normal_dis { 0.f, 1e-2f };

        glm::vec3 operator()(auto &gen){
            const float radius = uniform_dis(gen);
            const float angle = glm::two_pi<float>() * uniform_dis(gen);
            const glm::vec3 noise { normal_dis(gen), 4.f * normal_dis(gen), normal_dis(gen) };
            return { radius * glm::vec3(std::cos(angle), 0.f, std::sin(angle)) + noise };
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

std::vector<NBodyExecutor::Body> BodyPreset::galaxy(std::size_t num_bodies, unsigned int seed) {
    static NoiseDisk dis;

    std::mt19937 gen { seed };
    return generate_n(num_bodies, [&]() -> NBodyExecutor::Body {
        const glm::vec3 position = dis(gen);
        const glm::vec3 velocity = 0.2f * glm::vec3(position.z, dis.normal_dis(gen), -position.x);
        return { 1.f, position, velocity };
    });
}

std::vector<NBodyExecutor::Body> BodyPreset::explosion(std::size_t num_bodies, unsigned int seed) {
    static BallDistribution dis;

    std::mt19937 gen { seed };
    return generate_n(num_bodies, [&]() -> NBodyExecutor::Body {
        const glm::vec3 position = dis(gen);
        return NBodyExecutor::Body { 1.f, position, 0.1f * position };
    });
}
