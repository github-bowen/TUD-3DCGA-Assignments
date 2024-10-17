#pragma once

#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()


struct Config {
    // Particle simulation parameters
    uint32_t numParticles       = 25;
    float particleSimTimestep   = 0.014f;
    float particleRadius        = 0.45f;
    bool particleInterCollision = true;

    // Particle simulation flags
    bool doSingleStep           = false;
    bool doContinuousSimulation = true;
    bool doResetSimulation      = false;

    // Container sphere parameters
    glm::vec3 sphereCenter          = glm::vec3(0.0f);
    float sphereRadius              = 3.0f;
    glm::vec3 sphereColor           = glm::vec3(1.0f);

    // ===== Part 2: Drawing =====

    // Task 2.1: Speed-based Colors
    glm::vec3 minSpeedColor = glm::vec3(0.0f, 0.0f, 1.0f); // Blue for min speed £¨stationary)
    glm::vec3 maxSpeedColor = glm::vec3(1.0f, 0.0f, 0.0f); // Red for max speed
    float maxSpeedThreshold = 7.5f;
    bool useSpeedBasedColoring = true;

    // Task 2.2: Shading
    bool enableShading = true; // For shading toggle
    float ambientCoefficient = 0.1f;
};
