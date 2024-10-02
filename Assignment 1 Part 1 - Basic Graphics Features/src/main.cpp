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
#include <cassert>
#include <cstdlib> // EXIT_FAILURE
#include <framework/mesh.h>
#include <framework/shader.h>
#include <framework/trackball.h>
#include <framework/window.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl2.h>
#include <iostream>
#include <numeric>
#include <optional>
#include <span>
#include <toml/toml.hpp>
#include <vector>
#include <array>

#include <filesystem>
#include <algorithm>

// Configuration
const int WIDTH = 1200;
const int HEIGHT = 800;

bool show_imgui = true;
bool showShadows = false;
bool usePCF = false;  // only take effects when showShadows is set to true

const std::array diffuseModes { "debug", "lambert", "toon", "x-toon" };  // common one: "toon"
const std::array specularModes{ "none", "phong", "blinn-phong", "toon" };

int diffuseMode = 0;
int specularMode = 0;

struct {
    // Diffuse (Lambert)
    glm::vec3 kd { 0.5f };
    // Specular (Phong/Blinn Phong)
    glm::vec3 ks { 0.5f };
    float shininess = 3.0f;
    // Toon
    int toonDiscretize = 4;
    float toonSpecularThreshold = 0.49f;
} shadingData;

struct Texture {
    int width;
    int height;
    int channels;
    stbi_uc* texture_data;
};

// Lights
struct Light {
    glm::vec3 position;
    glm::vec3 color;
    bool is_spotlight;
    glm::vec3 direction;
    bool has_texture;
    Texture texture;

};

std::vector<Light> lights {};
size_t selectedLightIndex = 0;

void addLights() {
    lights.push_back(Light{ glm::vec3(0, 0, 3), glm::vec3(1) });
    //selectedLightIndex = lights.size() - 1;
}

void removeLights() {
    if (!lights.empty() && selectedLightIndex < lights.size()) {
        lights.erase(lights.begin() + selectedLightIndex);
        selectedLightIndex = std::min(lights.size() - 1, selectedLightIndex);
    }
}

void resetLights() {
    lights.clear();
    lights.push_back(Light{ glm::vec3(0, 0, 3), glm::vec3(1) });
    selectedLightIndex = 0;
}

void selectNextLight() {
    selectedLightIndex = (selectedLightIndex + 1) % lights.size();
}

void selectPreviousLight() {
    if (selectedLightIndex == 0)
        selectedLightIndex = lights.size() - 1;
    else
        --selectedLightIndex;
}

void imgui()
{

    // Define UI here
    if (!show_imgui)
        return;

    ImGui::Begin("Final project part 1 : Modern Shading");
    ImGui::Text("Press \\ to show/hide this menu");

    /* NOTE: Materials */

    ImGui::Separator();
    ImGui::Text("Material parameters");
    ImGui::SliderFloat("Shininess", &shadingData.shininess, 0.0f, 100.f);

    // Color pickers for Kd and Ks
    ImGui::ColorEdit3("Kd", &shadingData.kd[0]);
    ImGui::ColorEdit3("Ks", &shadingData.ks[0]);

    ImGui::SliderInt("Toon Discretization", &shadingData.toonDiscretize, 1, 10);
    ImGui::SliderFloat("Toon Specular Threshold", &shadingData.toonSpecularThreshold, 0.0f, 1.0f);

    /* NOTE: Lights */

    ImGui::Separator();
    ImGui::Text("Lights");

    // Display lights in scene
    std::vector<std::string> itemStrings = {};
    for (size_t i = 0; i < lights.size(); i++) {
        auto string = "Light " + std::to_string(i);
        itemStrings.push_back(string);
    }

    std::vector<const char*> itemCStrings = {};
    for (const auto& string : itemStrings) {
        itemCStrings.push_back(string.c_str());
    }

    int tempSelectedItem = static_cast<int>(selectedLightIndex);
    if (ImGui::ListBox("Lights", &tempSelectedItem, itemCStrings.data(), (int)itemCStrings.size(), 4)) {
        selectedLightIndex = static_cast<size_t>(tempSelectedItem);
    }

    if (!lights.empty()) {
        Light& selectedLight = lights[selectedLightIndex];
        ImGui::DragFloat3("Position", &selectedLight.position[0], 0.1f);
        ImGui::ColorEdit3("Color", &selectedLight.color[0]);
    }

    if (ImGui::Button("Add Lights")) addLights();  ImGui::SameLine();
    if (ImGui::Button("Remove Lights")) removeLights();  ImGui::SameLine();
    if (ImGui::Button("Reset Lights")) resetLights();

    /* NOTE: Render Settings */

    ImGui::Separator();
    ImGui::Text("Rendering");
    ImGui::Combo("Diffuse Mode", &diffuseMode, diffuseModes.data(), (int)diffuseModes.size());
    ImGui::Combo("Specular Mode", &specularMode, specularModes.data(), (int)specularModes.size());

    /* NOTE: Shadows Setting*/
    
    ImGui::Text("Shawdows Setting");
    ImGui::Checkbox("Shadows", &showShadows);

    ImGui::BeginDisabled(!showShadows);
    ImGui::Checkbox("PCF", &usePCF);
    ImGui::EndDisabled();

    ImGui::End();
    ImGui::Render();
}

std::optional<glm::vec3> tomlArrayToVec3(const toml::array* array)
{
    glm::vec3 output {};

    if (array) {
        int i = 0;
        array->for_each([&](auto&& elem) {
            if (elem.is_number()) {
                if (i > 2)
                    return;
                output[i] = static_cast<float>(elem.as_floating_point()->get());
                i += 1;
            } else {
                std::cerr << "Error: Expected a number in array, got " << elem.type() << std::endl;
                return;
            }
        });
    }

    return output;
}

// Program entry point. Everything starts here.
int main(int argc, char** argv)
{

    // read toml file from argument line (otherwise use default file)
    std::string config_filename = argc == 2 ? std::string(argv[1]) : "resources/default_scene.toml";
    //std::string config_filename = "resources/test_scene.toml";
    //std::string config_filename = "resources/scene2.toml";

    // parse initial scene config
    toml::table config;
    try {
        config = toml::parse_file(std::string(RESOURCE_ROOT) + config_filename);
    } catch (const toml::parse_error& ) {
        std::cerr << "parsing failed" << std::endl;
    }

    // read material data
    shadingData.kd = tomlArrayToVec3(config["material"]["kd"].as_array()).value();
    shadingData.ks = tomlArrayToVec3(config["material"]["ks"].as_array()).value();
    shadingData.shininess = config["material"]["shininess"].value_or(0.0f);
    shadingData.toonDiscretize = (int) config["material"]["toonDiscretize"].value_or(0);
    shadingData.toonSpecularThreshold = config["material"]["toonSpecularThreshold"].value_or(0.0f);

    // read lights
    lights = std::vector<Light> {};
    size_t num_lights = config["lights"]["positions"].as_array()->size();
    std::cout << "number of lights: " << num_lights << std::endl;

    for (size_t i = 0; i < num_lights; ++i) {
        auto pos = tomlArrayToVec3(config["lights"]["positions"][i].as_array()).value();
        auto color = tomlArrayToVec3(config["lights"]["colors"][i].as_array()).value();
        bool is_spotlight = config["lights"]["is_spotlight"][i].value<bool>().value();
        auto direction = tomlArrayToVec3(config["lights"]["direction"][i].as_array()).value();
        bool has_texture = config["lights"]["has_texture"][i].value<bool>().value();

        auto tex_path = std::string(RESOURCE_ROOT) + config["mesh"]["path"].value_or("resources/dragon.obj");
        int width = 0, height = 0, sourceNumChannels = 0;// Number of channels in source image. pixels will always be the requested number of channels (3).
        stbi_uc* pixels = nullptr;        
        if (has_texture) {
            pixels = stbi_load(tex_path.c_str(), &width, &height, &sourceNumChannels, STBI_rgb);
        }

        lights.emplace_back(Light { pos, color, is_spotlight, direction, has_texture, { width, height, sourceNumChannels, pixels } });
    }

    // Create window
    Window window { "Shading", glm::ivec2(WIDTH, HEIGHT), OpenGLVersion::GL41 };


    // read camera settings
    auto look_at = tomlArrayToVec3(config["camera"]["lookAt"].as_array()).value();
    auto rotations = tomlArrayToVec3(config["camera"]["rotations"].as_array()).value();
    float fovY = config["camera"]["fovy"].value_or(50.0f);
    float dist = config["camera"]["dist"].value_or(1.0f);

    auto diffuse_model = config["render_settings"]["diffuse_model"].value<std::string>();
    auto specular_model = config["render_settings"]["specular_model"].value<std::string>();
    bool do_pcf = config["render_settings"]["pcf"].value<bool>().value();
    bool do_shadows = config["render_settings"]["shadows"].value<bool>().value();

    Trackball trackball { &window, glm::radians(fovY) };
    trackball.setCamera(look_at, rotations, dist);

    // read mesh
    std::vector<Mesh> meshes;
    size_t currentFrame = 0;
    bool animated = config["mesh"]["animated"].value_or(false);

    if (animated) {
        // Read all .obj files from the directory
        std::string folderPath = std::string(RESOURCE_ROOT) + config["mesh"]["path"].value_or("resources/meshes");

        for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
            if (entry.path().extension() == ".obj") {
                // Load the .obj mesh and store in the meshes vector
                const Mesh mesh = loadMesh(entry.path().string())[0];
                meshes.push_back(mesh);
            }
        }

        if (meshes.empty()) {
            std::cerr << "No meshes found in the directory for animation." << std::endl;
            return EXIT_FAILURE;
        }
    } else {
        // Load a single static .obj file
        std::string mesh_path = std::string(RESOURCE_ROOT) + config["mesh"]["path"].value_or("resources/dragon.obj");
        meshes.push_back(loadMesh(mesh_path)[0]);
    }

    //auto mesh_path = std::string(RESOURCE_ROOT) + config["mesh"]["path"].value_or("resources/dragon.obj");
    //std::cout << mesh_path << std::endl;
    //const Mesh mesh = loadMesh(mesh_path)[0];

    window.registerKeyCallback([&](int key, int /* scancode */, int action, int /* mods */) {
        if (key == '\\' && action == GLFW_PRESS) {
            show_imgui = !show_imgui;
        }

        if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
            // Advance to next frame in animation
            if (animated) {
                currentFrame = (currentFrame + 1) % meshes.size();
                std::cout << "Current Frame: " << currentFrame << std::endl;
            }
        }

        if (action != GLFW_RELEASE)
            return;
    });

    // Shader setup - this should happen only once, not in the loop
    const Shader debugShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl")
        .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/debug_frag.glsl")
        .build();

    // Store VBO, IBO, VAO for each mesh
    std::vector<GLuint> vbos(meshes.size());
    std::vector<GLuint> ibos(meshes.size());
    std::vector<GLuint> vaos(meshes.size());

    // Initialize the buffers for all meshes
    for (size_t i = 0; i < meshes.size(); ++i) {
        const Mesh& mesh = meshes[i];

        // Create and bind buffers
        glGenBuffers(1, &vbos[i]);
        glBindBuffer(GL_ARRAY_BUFFER, vbos[i]);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(Vertex)), mesh.vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glGenBuffers(1, &ibos[i]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibos[i]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.triangles.size() * sizeof(decltype(Mesh::triangles)::value_type)), mesh.triangles.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        glGenVertexArrays(1, &vaos[i]);
        glBindVertexArray(vaos[i]);
        glBindBuffer(GL_ARRAY_BUFFER, vbos[i]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibos[i]);

        // Set vertex attribute pointers for position and normal
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        glBindVertexArray(0); // Unbind the VAO for now
    }

    // Main render loop
    while (!window.shouldClose()) {
        window.updateInput();

        imgui();

        // Clear the framebuffer and depth buffer
        glViewport(0, 0, window.getWindowSize().x, window.getWindowSize().y);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Camera/view/projection settings
        const glm::vec3 cameraPos = trackball.position();
        const glm::mat4 model{ 1.0f };
        const glm::mat4 view = trackball.viewMatrix();
        const glm::mat4 projection = trackball.projectionMatrix();
        const glm::mat4 mvp = projection * view * model;

        // Use the shader
        debugShader.bind();
        glUniformMatrix4fv(debugShader.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));

        auto render = [&](const Shader& shader) {
            const Mesh& mesh = meshes[currentFrame];  // Select current frame

            // Set the MVP matrix for the current frame
            glUniformMatrix4fv(shader.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));

            // Bind the VAO corresponding to the current frame
            glBindVertexArray(vaos[currentFrame]);

            // Draw the mesh
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.triangles.size()) * 3, GL_UNSIGNED_INT, nullptr);

            // Unbind the VAO
            glBindVertexArray(0);
            };


        render(debugShader);

        // Present result to the screen
        window.swapBuffers();
    }

    // Cleanup after rendering
    for (size_t i = 0; i < meshes.size(); ++i) {
        glDeleteBuffers(1, &vbos[i]);
        glDeleteBuffers(1, &ibos[i]);
        glDeleteVertexArrays(1, &vaos[i]);
    }

    return 0;

}
