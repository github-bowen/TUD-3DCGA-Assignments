// Disable compiler warnings in third-party code (which we cannot change).
#include <framework/disable_all_warnings.h>
#include <framework/opengl_includes.h>
DISABLE_WARNINGS_PUSH()
// Include glad before glfw3
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
// #define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
DISABLE_WARNINGS_POP()
#include <algorithm>
#include <framework/mesh.h>
#include <framework/shader.h>
#include <framework/trackball.h>
#include <framework/window.h>
#include <imgui/imgui.h>
#include <optional>
#include <vector>

// Configuration
const int WIDTH = 800;
const int HEIGHT = 800;

bool show_imgui = true;

void imgui()
{

    // Define UI here
    if (!show_imgui)
        return;

    ImGui::Begin("Tutorial : GLSL");
    ImGui::Text("Press \\ to show/hide this menu");

    ImGui::End();
    ImGui::Render();
}

// Program entry point. Everything starts here.
int main(int argc, char** argv)
{

    // Initialize window
    Window window { "Basic OpenGL", glm::ivec2(WIDTH, HEIGHT), OpenGLVersion::GL41 };
    Trackball trackball { &window, glm::radians(50.0f) };

    // Read Mesh
    const Mesh mesh = loadMesh(argc == 2 ? argv[1] : RESOURCE_ROOT "resources/dragon.obj")[0];

    // Read Keyboard
    window.registerKeyCallback([&](int key, int /* scancode */, int action, int /* mods */) {
        if (key == '\\' && action == GLFW_PRESS) {
            show_imgui = !show_imgui;
        }

        if (action != GLFW_RELEASE)
            return;

    });

    // Construct Shader Pipeline
    const Shader debugShader = ShaderBuilder()
                                   .addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl")
                                   .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/debug_frag.glsl")
                                   .build();

    // Create Vertex Buffer Object and Index Buffer Objects.
    GLuint vbo;

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(Vertex)), mesh.vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLuint ibo;
    // Create index buffer object (IBO)
    glGenBuffers(1, &ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.triangles.size() * sizeof(decltype(Mesh::triangles)::value_type)), mesh.triangles.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Bind vertex data to shader inputs using their index (location).
    // These bindings are stored in the Vertex Array Object.
    GLuint vao;
    // Create VAO and bind it so subsequent creations of VBO and IBO are bound to this VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

    // The position and normal vectors should be retrieved from the specified Vertex Buffer Object.
    // The stride is the distance in bytes between vertices. We use the offset to point to the normals
    // instead of the positions.
    // Tell OpenGL that we will be using vertex attributes 0 and 1.
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    // We tell OpenGL what each vertex looks like and how they are mapped to the shader by quering the driver for its location.
    glVertexAttribPointer(debugShader.getAttributeLocation("pos"), 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glVertexAttribPointer(debugShader.getAttributeLocation("normal"), 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    glBindVertexArray(0);

    // Enable depth testing.
    glEnable(GL_DEPTH_TEST);

    // Main loop.
    while (!window.shouldClose()) {

        window.updateInput();
        imgui();

        // Clear the framebuffer to black and depth to maximum value (ranges from [-1.0 to +1.0]).
        glViewport(0, 0, window.getWindowSize().x, window.getWindowSize().y);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Set model/view/projection matrix.
        const glm::vec3 cameraPos = trackball.position();
        const glm::mat4 model { 1.0f };
        const glm::mat4 view = trackball.viewMatrix();
        const glm::mat4 projection = trackball.projectionMatrix();
        const glm::mat4 mvp = projection * view * model;

        bool renderedSomething = false;
        auto render = [&]() {
            renderedSomething = true;

            // Set the model/view/projection matrix that is used to transform the vertices in the vertex shader.
            glUniformMatrix4fv(debugShader.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));

            // Bind vertex data.
            glBindVertexArray(vao);

            // Execute draw command.
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.triangles.size()) * 3, GL_UNSIGNED_INT, nullptr);

            // Unbind vertex data.
            glBindVertexArray(0);

        };

        if (!renderedSomething) {
            debugShader.bind();
            render();
        }

        // Present result to the screen.
        window.swapBuffers();
    }

    // Be a nice citizen and clean up after yourself.
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ibo);
    glDeleteVertexArrays(1, &vao);

    return 0;
}