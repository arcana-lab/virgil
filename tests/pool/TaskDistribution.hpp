#include <iostream>
#include <vector>
#include <math.h>
#include <random>


std::vector<std::uint32_t> getUniformDistribution(std::uint32_t numTasks, std::uint32_t maxIters) {
    srand(1);
    std::vector<std::uint32_t> distribution; 
    for (int i = 0; i < numTasks; i++) {
        distribution.push_back(rand() % maxIters + 1);
    }
    return distribution;
}

std::vector<std::uint32_t> getBimodalDistribution(std::uint32_t numTasks, std::uint32_t low, std::uint32_t high) {
    srand(1);
    std::vector<std::uint32_t> distribution; 
    for (int i = 0; i < numTasks; i++) {
        distribution.push_back((rand() % 2)? low : high);
    }
    return distribution;
}

std::vector<std::uint32_t> getHomogeneousDistribution (std::uint32_t numTasks, std::uint32_t weight) {
    srand(1);
    std::vector<std::uint32_t> distribution; 
    for (int i = 0; i < numTasks; i++) {
        distribution.push_back(weight);
    }
    return distribution;
}

std::vector<std::uint32_t> getNormalDistribution (std::uint32_t numTasks, std::uint32_t mean, std::uint32_t var) {
    srand(1);
    std::default_random_engine generator;
    std::normal_distribution<double> normal(mean, var);
    std::vector<std::uint32_t> distribution; 
    for (int i = 0; i < numTasks; i++) {
        std::uint32_t weight = (int)(normal(generator) + 0.5);
        weight = std::max(weight, (std::uint32_t) 1); //negative weights not allowed, 0 weights useless.
        distribution.push_back(weight);
    }
    return distribution;
}