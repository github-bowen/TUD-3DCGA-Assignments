#version 410
#extension GL_ARB_explicit_uniform_location : enable

#define COLLISION_OFFSET 0.001

uniform sampler2D previousPositions;
uniform sampler2D previousVelocities;
uniform sampler2D previousBounceData;
uniform float timestep;
uniform uint numParticles;
uniform float particleRadius;
uniform vec3 containerCenter;
uniform float containerRadius;
uniform bool interParticleCollision;

// uniform bool enableBounce;
uniform int bounceThreshold;
uniform int bounceFrames;
// uniform vec3 bounceColor;

layout(location = 0) out vec3 finalPosition;
layout(location = 1) out vec3 finalVelocity;
layout(location = 2) out vec3 finalBounceData;

void main() {
    // ===== Task 1.1 Verlet Integration =====

    // Fetch the particle's previous position and velocity
    vec2 uv = gl_FragCoord.xy / textureSize(previousPositions, 0);  // 0: main mipmap
    vec3 prevPos = texture(previousPositions, uv).rgb;
    vec3 prevVel = texture(previousVelocities, uv).rgb;
    vec2 prevBounceData = texture(previousBounceData, uv).xy;

    // Unpack previous collision and frame counters
    int collisionCount = int(prevBounceData.x);
    int frameCounter = int(prevBounceData.y);

    // Define acceleration due to gravity (constant)
    vec3 gravity = vec3(0.0, -9.81, 0.0);

    // Compute new position using Velocity Verlet Integration
    vec3 newPos = prevPos + prevVel * timestep + 0.5 * gravity * timestep * timestep;

    // Compute velocity at the new position (same acceleration: a = g = -9.81)
    vec3 newVel = prevVel + gravity * timestep;

    // ===== Task 1.3 Inter-particle Collision =====

    if (interParticleCollision) {
        uint curr_i = uint(gl_FragCoord.x);
        for (uint i = 0; i < numParticles; ++i) {
            if (i == curr_i) continue;  // skip self

            // Calculate UV coordinates for the other particle
            vec2 otherUV = vec2(float(i + 0.5) / float(numParticles), 0);  // NOTE: plus 0.5 to sample texture center
            vec3 otherPos = texture(previousPositions, otherUV).rgb;
            //vec3 otherVel = texture(previousVelocities, otherUV).rgb;

            // Check for collision
            vec3 toOther = newPos - otherPos;
            float dist = length(toOther);
            if (dist < 2.0 * particleRadius) {
                vec3 normal = normalize(toOther);
                // Push the particle away
                newPos += normal * (2.0 * particleRadius - dist + COLLISION_OFFSET) * 0.5;  // 0.5: half for both particles
                // Reflect velocity
                newVel = reflect(newVel, normal);

                collisionCount++;
            }
        }
    }
    

    // ===== Task 1.2 Container Collision =====

    vec3 toCenter = newPos - containerCenter;
    float distanceToCenter = length(toCenter);

    if (distanceToCenter > containerRadius - particleRadius) {
        vec3 normal = normalize(toCenter);
        // Push the particle back inside
        newPos = containerCenter + normal * (containerRadius - particleRadius - COLLISION_OFFSET);
        // Reflect the velocity about the collision normal
        newVel = reflect(newVel, normal);

        collisionCount++;
    }

    // ===== Task 3: Blink Logic =====

    // If the collision count exceeds the threshold, set the frame counter and reset the collision count
    if (collisionCount >= bounceThreshold) {
        frameCounter = bounceFrames;
        collisionCount = 0; // Reset collision count
    }

    // Decrement the frame counter if it's above zero
    frameCounter = max(frameCounter - 1, 0);


    finalPosition = newPos;
    finalVelocity = newVel;

    // Pack the updated collision count and frame counter into the final bounce data
    finalBounceData = vec3(float(collisionCount), float(frameCounter), 0.0);
}
