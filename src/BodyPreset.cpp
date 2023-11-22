//
// Created by gomkyung2 on 2023/08/30.
//

#include "BodyPreset.hpp"

#include <algorithm>

namespace{
    template <typename Func>
    std::vector<std::invoke_result_t<Func>> generate_n(std::size_t n, Func &&f){
        std::vector<std::invoke_result_t<Func>> result;
        result.reserve(n);
        std::generate_n(std::back_inserter(result), n, f);

        return result;
    }
};

std::vector<NBodyExecutor::Body> BodyPreset::galaxy(std::size_t num_bodies, unsigned int seed) {
    static Galaxy dis;

    std::mt19937 gen { seed };
    std::uniform_real_distribution mass_dis { 1.f, 10.f };
    return generate_n(num_bodies, [&]() -> NBodyExecutor::Body {
        const glm::vec3 position = dis(gen);
        const glm::vec3 velocity = 0.2f * glm::vec3(position.z, dis.normal_dis(gen), -position.x);
        return { mass_dis(gen), position, velocity };
    });
}

std::vector<NBodyExecutor::Body> BodyPreset::explosion(std::size_t num_bodies, unsigned int seed) {
    static BallDistribution dis;

    std::mt19937 gen { seed };
    std::uniform_real_distribution mass_dis { 1.f, 10.f };
    return generate_n(num_bodies, [&]() -> NBodyExecutor::Body {
        const glm::vec3 position = dis(gen);
        return { mass_dis(gen), position, 0.1f * position };
    });
}

std::vector<NBodyExecutor::Body> BodyPreset::galaxy_pair(
        int num_bodies1, int num_bodies2,
        glm::vec3 cm_position1, glm::vec3 cm_position2,
        glm::vec3 cm_velocity1, glm::vec3 cm_velocity2,
        unsigned int seed)
{
    using body_t = NBodyExecutor::Body;

    std::vector<body_t> galaxy1 = galaxy(num_bodies1, seed);
    for (body_t &body : galaxy1){
        body.position += cm_position1;
        body.velocity += cm_velocity1;
    }

    const std::vector<body_t> galaxy2 = galaxy(num_bodies2, seed);
    std::ranges::transform(
        galaxy2,
        std::back_inserter(galaxy1),
        [&](const body_t &body) -> body_t {
            return {
                body.mass,
                body.position + cm_position2,
                body.velocity + cm_velocity2
            };
        }
    );

    return galaxy1;
}

//std::vector<NBodyExecutor::Body> BodyPreset::generate(std::size_t n, auto &&mass_distribution, auto &&shape_distribution){
//    return generate(n, FWD(mass_distribution), FWD(shape_distribution), rd());
//}
//
//std::vector<NBodyExecutor::Body> BodyPreset::generate(std::size_t n, auto &&mass_distribution,
//                                                      auto &&shape_distribution, unsigned int seed) {
//    std::mt19937 gen { seed };
//    return generate_n(n, [&]() -> NBodyExecutor::Body {
//        const float mass = mass_distribution(gen);
//        const auto [position, velocity] = shape_distribution(gen);
//        return { mass, position, velocity }; // Acceleration will be zero.
//    });
//}