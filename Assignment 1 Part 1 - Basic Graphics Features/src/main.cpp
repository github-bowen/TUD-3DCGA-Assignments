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

#define debug (diffuseMode == 0)
#define diffuseLighting (diffuseMode == 1)
#define toonLightingDiffuse (diffuseMode == 2)
#define toonxLighting (diffuseMode == 3)

#define phongSpecularLighting (specularMode == 1)
#define blinnPhongSpecularLighting (specularMode == 2)
#define toonLightingSpecular (specularMode == 3)

#define DEBUG_MODE 1

#define ENABLE_AUTOPLAY 1
#define FRAME_DURATION 0.1

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
    lights.push_back(Light{ glm::vec3(0.4, 1.2, 0.2), glm::vec3(1), false, glm::vec3(0.707, 0.0, 0.707), false});
    //selectedLightIndex = lights.size() - 1;
}

void removeLights() {
    if (!lights.empty() && selectedLightIndex < lights.size() && lights.size() > 1) {
        lights.erase(lights.begin() + selectedLightIndex);
        selectedLightIndex = std::min(lights.size() - 1, selectedLightIndex);
    }
}

void resetLights() {
    lights.clear();
    lights.push_back(Light{ glm::vec3(0.4, 1.2, 0.2), glm::vec3(1), false, glm::vec3(0.707, 0.0, 0.707), false});
    selectedLightIndex = 0;
}

void selectNextLight() {
    selectedLightIndex = (selectedLightIndex + 1) % lights.size();
}

void selectPreviousLight() {
    if (selectedLightIndex == 0)
        selectedLightIndex = std::max((int)lights.size() - 1, 0);
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
    //ImGui::BeginDisabled(debug || toonxLighting);
    ImGui::Checkbox("Shadows", &showShadows);
    //ImGui::EndDisabled();

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
    //std::string config_filename = argc == 2 ? std::string(argv[1]) : "resources/default_scene.toml";
    std::string config_filename = "resources/test_scene.toml";
    //std::string config_filename = "resources/test_scene2.toml";
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
                const Mesh mesh = mergeMeshes(loadMesh(entry.path().string()));
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
        meshes.push_back(mergeMeshes(loadMesh(mesh_path)));
    }

    //auto mesh_path = std::string(RESOURCE_ROOT) + config["mesh"]["path"].value_or("resources/dragon.obj");
    //std::cout << mesh_path << std::endl;
    //const Mesh mesh = loadMesh(mesh_path)[0];
    bool frameChanged = true;

    window.registerKeyCallback([&](int key, int /* scancode */, int action, int /* mods */) {
        if (key == '\\' && action == GLFW_PRESS) {
            show_imgui = !show_imgui;
        }

        if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
            // Advance to next frame in animation
            if (animated) {
                currentFrame = (currentFrame + 1) % meshes.size();
                std::cout << "Current Frame: " << currentFrame << std::endl;
                frameChanged = true;
            }
        }

        if (action != GLFW_RELEASE)
            return;
    });

    // Shader setup - this should happen only once, not in the loop
    const Shader lightShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/light_vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/light_frag.glsl").build();
    
    const Shader debugShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/debug_frag.glsl").build();
    const Shader lambertShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/lambert_frag.glsl").build();
    const Shader phongShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/phong_frag.glsl").build();
    const Shader blinnPhongShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/blinn_phong_frag.glsl").build();
    const Shader toonDiffuseShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/toon_diffuse_frag.glsl").build();
    const Shader toonSpecularShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/toon_specular_frag.glsl").build();
    const Shader xToonShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/vertex.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/xtoon_frag.glsl").build();

    //const Shader mainShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shader_frag.glsl").build();
    const Shader shadowShader = ShaderBuilder().addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shadow_vert.glsl").addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shadow_frag.glsl").build();

    /* NOTE: Texture of Lights */

    // Assuming you have a list of lights and each light may have a texture
    //std::vector<GLuint> lightTextures; 

    //// Initialize textures for each light
    //for (Light& light : lights) {

    //    if (light.has_texture) {
    //        Texture& texture = light.texture;
    //        int texWidth = texture.width, texHeight = texture.height, texChannels = texture.channels;
    //        stbi_uc* pixels = texture.texture_data;  // Texture data already loaded

    //        GLuint texID;
    //        glGenTextures(1, &texID);  // Generate a texture ID
    //        glBindTexture(GL_TEXTURE_2D, texID);
    //        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    //        // Set texture parameters
    //        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    //        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    //        glBindTexture(GL_TEXTURE_2D, 0);  // Unbind texture

    //        // Associate the generated texture ID with the current light in the map
    //        lightTextures.push_back(texID);
    //    }
    //}

    Light& light = lights[selectedLightIndex];
    Texture& texture = light.texture;
    int texWidth = texture.width, texHeight = texture.height, texChannels = texture.channels;
    stbi_uc* pixels = texture.texture_data;  // Texture data already loaded

    GLuint texLight;
    glGenTextures(1, &texLight);  // Generate a texture ID
    glBindTexture(GL_TEXTURE_2D, texLight);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(pixels);


    /* NOTE: Shadow Texture */

    // === Create Shadow Texture ===
    
#if DEBUG_MODE
    const int SHADOWTEX_WIDTH = 2400;
    const int SHADOWTEX_HEIGHT = 1600;
#else
    const int SHADOWTEX_WIDTH = 1024;
    const int SHADOWTEX_HEIGHT = 1024;
#endif

    GLuint texShadow;
    glGenTextures(1, &texShadow);
    glBindTexture(GL_TEXTURE_2D, texShadow);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, SHADOWTEX_WIDTH, SHADOWTEX_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    // Set behaviour for when texture coordinates are outside the [0, 1] range.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Set interpolation for texture sampling (GL_NEAREST for no interpolation).
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0);

    // === Create framebuffer for extra texture ===
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texShadow, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glEnable(GL_DEPTH_TEST);

    /* NOTE: Toon Texture */

    // Load image from disk to CPU memory.
    int width, height, sourceNumChannels; // Number of channels in source image. pixels will always be the requested number of channels (3).
    stbi_uc* pixels2 = stbi_load(RESOURCE_ROOT "resources/toon_map.png", &width, &height, &sourceNumChannels, STBI_rgb);

    // Create a texture on the GPU with 3 channels with 8 bits each.
    GLuint texToon;
    glGenTextures(1, &texToon);
    glBindTexture(GL_TEXTURE_2D, texToon);

    // Set behavior for when texture coordinates are outside the [0, 1] range.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Set interpolation for texture sampling (GL_NEAREST for no interpolation).
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels2);

    // Free the CPU memory after we copied the image to the GPU.
    stbi_image_free(pixels2);

    // Enable depth testing.
    //glEnable(GL_DEPTH_TEST);



    // Store VBO, IBO, VAO for each mesh
    //std::vector<GLuint> vbos(meshes.size());
    //std::vector<GLuint> ibos(meshes.size());
    //std::vector<GLuint> vaos(meshes.size());

    //// Initialize the buffers for all meshes
    //for (size_t i = 0; i < meshes.size(); ++i) {
    //    const Mesh& mesh = meshes[i];

    //    // Create and bind buffers
    //    glGenBuffers(1, &vbos[i]);
    //    glBindBuffer(GL_ARRAY_BUFFER, vbos[i]);
    //    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(Vertex)), mesh.vertices.data(), GL_STATIC_DRAW);
    //    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //    glGenBuffers(1, &ibos[i]);
    //    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibos[i]);
    //    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.triangles.size() * sizeof(decltype(Mesh::triangles)::value_type)), mesh.triangles.data(), GL_STATIC_DRAW);
    //    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    //    glGenVertexArrays(1, &vaos[i]);
    //    glBindVertexArray(vaos[i]);
    //    glBindBuffer(GL_ARRAY_BUFFER, vbos[i]);
    //    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibos[i]);

    //    // Set vertex attribute pointers for position and normal

    //    glEnableVertexAttribArray(0);
    //    glEnableVertexAttribArray(1);

    //    glBindVertexArray(0); // Unbind the VAO for now
    //}


    double lastTime = glfwGetTime(); // Track the time when the last frame was displayed
    double frameDuration = FRAME_DURATION;      // Duration (in seconds) to display each frame


    GLuint vao = 0, vbo = 0, ibo = 0;

    // Main render loop
    while (!window.shouldClose()) {
        window.updateInput();

        imgui();

#if ENABLE_AUTOPLAY
        // Time-based automatic frame animation
        double currentTime = glfwGetTime();
        if (animated && (currentTime - lastTime >= frameDuration)) {
            currentFrame = (currentFrame + 1) % meshes.size(); // Advance to the next frame
            lastTime = currentTime;  // Reset the timer
            frameChanged = true;     // Indicate that the frame has changed
        }
#endif

        Mesh& mesh = meshes[currentFrame];
        // Assume frameHasChanged() checks if the frame has changed
        if (frameChanged) {
            frameChanged = false;

            // Clear the previous buffers
            if (vao != 0) {
                glDeleteVertexArrays(1, &vao);
                glDeleteBuffers(1, &vbo);
                glDeleteBuffers(1, &ibo);
            }
            

            //    // Create and bind buffers
			glGenBuffers(1, &vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(Vertex)), mesh.vertices.data(), GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);

			glGenBuffers(1, &ibo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.triangles.size() * sizeof(decltype(Mesh::triangles)::value_type)), mesh.triangles.data(), GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);

			//    // Set vertex attribute pointers for position and normal

			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);

			glBindVertexArray(0); // Unbind the VAO for now
        }

        // Camera/view/projection settings
        const glm::vec3 cameraPos = trackball.position();
        const glm::mat4 model{ 1.0f };
        const glm::mat4 view = trackball.viewMatrix();
        const glm::mat4 projection = trackball.projectionMatrix();
        const glm::mat4 mvp = projection * view * model;


        Light light = lights[selectedLightIndex];  // TODO

        //const glm::mat4 lightViewMatrix = glm::lookAt(light.position, light.position + light.direction, glm::vec3(0.0f, 1.0f, 0.0f));
        //const glm::mat4 lightMVP = mvp * lightViewMatrix;
        GLfloat near_plane = 0.5f, far_plane = 30.0f;
        glm::mat4 mainProjectionMatrix = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
        glm::mat4 lightViewMatrix = glm::lookAt(light.position, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::mat4 lightMVP = mainProjectionMatrix * lightViewMatrix * model;

        if (showShadows) {
            // Bind the off-screen framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

            // Clear the shadow map and set needed options
            glClearDepth(1.0);
            glClear(GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);

            // Bind the shader
            shadowShader.bind();

            // Set viewport size
            glViewport(0, 0, SHADOWTEX_WIDTH, SHADOWTEX_HEIGHT);

            // .... HERE YOU MUST ADD THE CORRECT UNIFORMS FOR RENDERING THE SHADOW MAP
            // pass samplingmode as uniform

            glUniformMatrix4fv(shadowShader.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(lightMVP));

            // Bind vertex data
            glBindVertexArray(vao);

            glVertexAttribPointer(shadowShader.getAttributeLocation("pos"), 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

            // Execute draw command
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(
                meshes[currentFrame].triangles.size() * 3), GL_UNSIGNED_INT, nullptr);

            // Unbind the off-screen framebuffer
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // Clear the framebuffer to black and depth to maximum value (ranges from [-1.0 to +1.0]).
        glViewport(0, 0, window.getWindowSize().x, window.getWindowSize().y);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);



        // Use the shader
        //mainShader.bind();
        //glUniformMatrix4fv(mainShader.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));

        auto render = [&](const Shader& shader) {

            const Mesh& mesh = meshes[currentFrame];  // Select current frame

            if (!(debug)) {
                glUniformMatrix4fv(shader.getUniformLocation("lightMVP"), 1, GL_FALSE, glm::value_ptr(lightMVP));
            }

            // Set the MVP matrix for the current frame
            glUniformMatrix4fv(shader.getUniformLocation("mvp"), 1, GL_FALSE, glm::value_ptr(mvp));

            //std::cout << "spotlight mode: " << light.is_spotlight << std::endl;
            //std::cout << "light texture: " << light.has_texture << std::endl;

            if (!(debug)) {
                glUniform1i(shader.getUniformLocation("shadow"), showShadows);
                glUniform1i(shader.getUniformLocation("pcf"), usePCF);
                glUniform1i(shader.getUniformLocation("peelingMode"), 0);
                glUniform1i(shader.getUniformLocation("lightMode"), light.is_spotlight ? 1 : 0);
                glUniform1i(shader.getUniformLocation("lightColorMode"), light.has_texture ? 1 : 0);
            }

            // Bind the VAO corresponding to the current frame
            glBindVertexArray(vao);

            if (!(debug)) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texShadow);
                glUniform1i(shader.getUniformLocation("texShadow"), 0);

				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, texLight);
				glUniform1i(shader.getUniformLocation("texLight"), 1);
            }

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

            glViewport(0, 0, SHADOWTEX_WIDTH, SHADOWTEX_HEIGHT);

            // Clear the framebuffer to black and depth to maximum value
            //glClearDepth(1.0);
            ////glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
            //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            //glDisable(GL_CULL_FACE);
            //glEnable(GL_DEPTH_TEST);

            glClearDepth(1.0);
            glClear(GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);

            // Draw the mesh
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mesh.triangles.size()) * 3, GL_UNSIGNED_INT, nullptr);

            // Unbind the VAO
            glBindVertexArray(0);
            };


        if (!debug) {
            // Draw mesh into depth buffer but disable color writes.
            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LEQUAL);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            debugShader.bind();
            render(debugShader);

            // Draw the mesh again for each light / shading model.
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); // Enable color writes.
            glDepthMask(GL_FALSE); // Disable depth writes.
            glDepthFunc(GL_EQUAL); // Only draw a pixel if it's depth matches the value stored in the depth buffer.
            glEnable(GL_BLEND); // Enable blending.
            glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive blending.

            for (size_t i = 0; i < lights.size(); ++i) {
                Light& light = lights[i];

                if (i != selectedLightIndex) continue;  //TODO

                // Diffuse Part
				if (toonxLighting) {
					xToonShader.bind();

					// === SET YOUR X-TOON UNIFORMS HERE ===
					// Values that you may want to pass to the shader are stored in light, shadingData and cameraPos and texToon.
					glActiveTexture(GL_TEXTURE2);
					glBindTexture(GL_TEXTURE_2D, texToon);
                    glUniform1i(xToonShader.getUniformLocation("texToon"), 2); // Change xxx to the uniform name that you want to use.

					glUniform3fv(xToonShader.getUniformLocation("lightPos"), 1, glm::value_ptr(light.position));
					glUniform3fv(xToonShader.getUniformLocation("cameraPos"), 1, glm::value_ptr(cameraPos));
					glUniform1f(xToonShader.getUniformLocation("shininess"), shadingData.shininess);
					
					render(xToonShader);
				} else if (toonLightingDiffuse) {
					toonDiffuseShader.bind();

					// === SET YOUR DIFFUSE TOON UNIFORMS HERE ===
					// Values that you may want to pass to the shader are stored in light, shadingData.
					glUniform3fv(toonDiffuseShader.getUniformLocation("lightPos"), 1, glm::value_ptr(light.position));
					glUniform3fv(toonDiffuseShader.getUniformLocation("lightColor"), 1, glm::value_ptr(light.color));
					glUniform3fv(toonDiffuseShader.getUniformLocation("kd"), 1, glm::value_ptr(shadingData.kd));
					glUniform1i(toonDiffuseShader.getUniformLocation("toonDiscretize"), shadingData.toonDiscretize);
					render(toonDiffuseShader);
				} else if (diffuseLighting) {
                    lambertShader.bind();
                    // === SET YOUR LAMBERT UNIFORMS HERE ===
                    // Values that you may want to pass to the shader include light.position, light.color and shadingData.kd.
                    glUniform3fv(lambertShader.getUniformLocation("lightPos"), 1, glm::value_ptr(light.position));
                    glUniform3fv(lambertShader.getUniformLocation("lightColor"), 1, glm::value_ptr(light.color));
                    glUniform3fv(lambertShader.getUniformLocation("kd"), 1, glm::value_ptr(shadingData.kd));
                    render(lambertShader);
                }


                // Specular Part
				if (toonLightingSpecular) {
					toonSpecularShader.bind();

					// === SET YOUR SPECULAR TOON UNIFORMS HERE ===
					// Values that you may want to pass to the shader are stored in light, shadingData and cameraPos.
					glUniform3fv(toonSpecularShader.getUniformLocation("lightPos"), 1, glm::value_ptr(light.position));
					//glUniform3fv(toonSpecularShader.getUniformLocation("lightColor"), 1, glm::value_ptr(light.color));
					//glUniform3fv(toonSpecularShader.getUniformLocation("ks"), 1, glm::value_ptr(shadingData.ks));
					glUniform1f(toonSpecularShader.getUniformLocation("shininess"), shadingData.shininess);
					glUniform3fv(toonSpecularShader.getUniformLocation("cameraPos"), 1, glm::value_ptr(cameraPos));
					glUniform1f(toonSpecularShader.getUniformLocation("toonSpecularThreshold"), shadingData.toonSpecularThreshold);
					render(toonSpecularShader);
				} else if (phongSpecularLighting || blinnPhongSpecularLighting) {
					const Shader& shader = phongSpecularLighting ? phongShader : blinnPhongShader;
					shader.bind();

					// === SET YOUR PHONG/BLINN PHONG UNIFORMS HERE ===
					// Values that you may want to pass to the shader are stored in light, shadingData and cameraPos.
					glUniform3fv(shader.getUniformLocation("lightPos"), 1, glm::value_ptr(light.position));
					glUniform3fv(shader.getUniformLocation("lightColor"), 1, glm::value_ptr(light.color));
					glUniform3fv(shader.getUniformLocation("ks"), 1, glm::value_ptr(shadingData.ks));
					glUniform1f(shader.getUniformLocation("shininess"), shadingData.shininess);
					glUniform3fv(shader.getUniformLocation("cameraPos"), 1, glm::value_ptr(cameraPos));
					render(shader);
				}
                
            }

            // Restore default depth test settings and disable blending.
            glDepthFunc(GL_LEQUAL);
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
        } else {
            debugShader.bind();
            // glUniform3fv(debugShader.getUniformLocation("viewPos"), 1, glm::value_ptr(cameraPos)); // viewPos.
            render(debugShader);
        }


        lightShader.bind();
        {
            const glm::vec4 screenPos = mvp * glm::vec4(lights[selectedLightIndex].position, 1.0f);
            //const glm::vec3 color { 1, 1, 0 };
            glm::vec3 color = lights[selectedLightIndex].color;  // FIXME: I changed the light cube color to light color

            glPointSize(40.0f);
            glUniform4fv(lightShader.getUniformLocation("pos"), 1, glm::value_ptr(screenPos));
            glUniform3fv(lightShader.getUniformLocation("color"), 1, glm::value_ptr(color));
            glBindVertexArray(vao);
            glDrawArrays(GL_POINTS, 0, 1);
            glBindVertexArray(0);
        }
        for (const Light& light : lights) {
            const glm::vec4 screenPos = mvp * glm::vec4(light.position, 1.0f);
            // const glm::vec3 color { 1, 0, 0 };

            glPointSize(10.0f);
            glUniform4fv(lightShader.getUniformLocation("pos"), 1, glm::value_ptr(screenPos));
            glUniform3fv(lightShader.getUniformLocation("color"), 1, glm::value_ptr(light.color));
            glBindVertexArray(vao);
            glDrawArrays(GL_POINTS, 0, 1);
            glBindVertexArray(0);

        }

        // Present result to the screen
        window.swapBuffers();
    }

    // Be a nice citizen and clean up after yourself.
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteTextures(1, &texToon);
    glDeleteTextures(1, &texShadow);
    glDeleteTextures(1, &texLight);
    //for (GLuint & texLight : lightTextures) glDeleteTextures(1, &texLight);

    // Cleanup after rendering
    for (size_t i = 0; i < meshes.size(); ++i) {
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ibo);
        glDeleteVertexArrays(1, &vao);
    }

    return 0;

}
