// Disable compiler warnings in third-party code (which we cannot change).
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glad/glad.h>
// Include glad before glfw3.
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
DISABLE_WARNINGS_POP()
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <framework/mesh.h>
#include <framework/trackball.h>
#include <framework/window.h>
#include <imgui/imgui.h>
#include <iostream>
#include <memory>
#include <numeric>
#include <span>
#include <string>
#include <vector>

// START READING HERE!!!

//////Predefined global variables

// Use the enum values to define different rendering modes
// The mode is used by the function display and the mode is
// chosen during execution with the keys 1-9
enum class DisplayModeType {
	TRIANGLE,
	FACE,
	CUBE,
	ARM,
	MESH,
};

bool show_imgui = true;
bool lighting_enabled = false;
DisplayModeType displayMode = DisplayModeType::TRIANGLE;
constexpr glm::ivec2 resolution{ 800, 800 };
std::unique_ptr<Window> pWindow;
std::unique_ptr<Trackball> pTrackball;

glm::vec4 lightPos{ 1.0f, 1.0f, 0.4f, 1.0f };
Mesh mesh;


// Declare your own global variables here:
float drawTriangle_vertex1_x = 0;

// scale vector of each part of the arm determines the arm shape
const std::array armScale{
	glm::vec3(0.5f, 0.5f, 1.0f),
	glm::vec3(0.3f, 0.3f, 0.8f),
	glm::vec3(0.6f, 0.6f, 0.6f)
};

// rotation of each part of the arm, determines the arm pose
std::array arm_joint{ 0.5f, -0.3f, -0.4f };

////////// Draw Functions

// function to draw coordinate axes with a certain length (1 as a default)
void drawCoordSystem(const float length = 1) {
	// draw simply colored axes

	// remember all states of the GPU
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	// deactivate the lighting state
	glDisable(GL_LIGHTING);
	// draw axes
	glBegin(GL_LINES);
	glColor3f(1, 0, 0);
	glVertex3f(0, 0, 0);
	glVertex3f(length, 0, 0);

	glColor3f(0, 1, 0);
	glVertex3f(0, 0, 0);
	glVertex3f(0, length, 0);

	glColor3f(0, 0, 1);
	glVertex3f(0, 0, 0);
	glVertex3f(0, 0, length);
	glEnd();

	// reset to previous state
	glPopAttrib();
}

/**
 * Several drawing functions for you to work on
 */

void drawTriangle() {
	// A simple example of a drawing function for a triangle
	// 1) Try changing its color to red
	// 2) Try changing its vertex positions
	// 3) Add a second triangle in blue
	// 4) Add a global variable (initialized at 0), which represents the
	//   x-coordinate of the first vertex of each triangle
	// 5) Go to the function animate and increment this variable
	//   by a small value - observe the animation.

	/*glColor3f(1, 1, 1);
	glNormal3f(0, 0, 1);
	glBegin(GL_TRIANGLES);
	glVertex3f(0, 0, 0);
	glVertex3f(1, 0, 0);
	glVertex3f(1, 1, 0);
	glEnd();*/

	glColor3f(1, 0, 0);
	//glNormal3f(0, 0, 1);
	glBegin(GL_TRIANGLES);
	glVertex3f(drawTriangle_vertex1_x, 0, 0);
	glVertex3f(-1, -1, 0);
	glVertex3f(0, 1, 1);

	glColor3f(0, 0, 1);
	glVertex3f(drawTriangle_vertex1_x, 0, 0);
	glVertex3f(1, -1, 0);
	glVertex3f(-1, 0, 1);
	glEnd();
}

void drawUnitFace(const glm::mat4& transformMatrix) {
	// 1) Draw a unit quad in the x,y plane oriented along the z axis
	// 2) Make sure the orientation of the vertices is positive (counterclock wise)
	// 3) What happens if the order is inversed?
	// 4) Transform the quad by the given transformation matrix.
	//
	//  For more information on how to use glm (OpenGL Mathematics Library), see:
	//  https://github.com/g-truc/glm/blob/master/manual.md#section1
	//
	//  The documentation of the various matrix transforms can be found here:
	//  https://glm.g-truc.net/0.9.9/api/a00247.html
	//
	//  Please note that the glm matrix operations transform an existing matrix.
	//  To get just a rotation/translation/scale matrix you can pass an identity
	//  matrix (glm::mat4(1.0f)). Also, keep in mind that the rotation angle is
	//  specified in radians. You can use glm::degrees(angleInRadians) to convert
	//  from radians to degrees, and glm::radians(angleInDegrees) for the reverse
	//  operation. Be carefull! these functions only take floating point based types.
	//
	//  For example (rotate 90 degrees around the x axis):
	//  glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0));


	glm::vec4 v1(0, 0, 0, 1), v2(1, 0, 0, 1), v3(1, 1, 0, 1), v4(0, 1, 0, 1);

	v1 = transformMatrix * v1;
	v2 = transformMatrix * v2;
	v3 = transformMatrix * v3;
	v4 = transformMatrix * v4;

	glm::vec3 edge1 = v2 - v1;
	glm::vec3 edge2 = v3 - v1;

	glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

	glNormal3f(normal.x, normal.y, normal.z);

	glBegin(GL_QUADS);
	glVertex3f(v1.x, v1.y, v1.z);
	glVertex3f(v2.x, v2.y, v2.z);
	glVertex3f(v3.x, v3.y, v3.z);
	glVertex3f(v4.x, v4.y, v4.z);
	glEnd();
}

void drawUnitCube(const glm::mat4& transformMatrix) {
	// 1) Draw a cube using your function drawUnitFace. Use glm::translate(Matrix, Vector)
	//    and glm::rotate(Matrix, Angle, Vector) to create the transformation matrices
	//    passed to drawUnitFace.
	// 2) Transform your cube by the given transformation matrix.

	glm::mat4 i(1);
	constexpr float angle90 = glm::radians(90.0f), angle180 = glm::radians(180.0f);
	glm::vec3 x_axis(1, 0, 0), y_axis(0, 1, 0), z_axis(0, 0, 1);

	// z-
	drawUnitFace(
		transformMatrix * glm::rotate(i, angle180, y_axis) * glm::rotate(i, angle90, z_axis)
	);

	// z+
	drawUnitFace(
		transformMatrix * glm::translate(i, z_axis)
	);

	// x-
	drawUnitFace(
		transformMatrix * glm::rotate(i, -angle90, y_axis)
	);

	// x+
	drawUnitFace(
		transformMatrix * glm::translate(i, x_axis) * glm::rotate(i, angle90, z_axis) * glm::rotate(i, angle90, x_axis)
	);

	// y-
	drawUnitFace(
		transformMatrix * glm::rotate(i, angle90, x_axis)
	);

	// y+
	drawUnitFace(
		transformMatrix * glm::translate(i, y_axis) * glm::translate(i, z_axis) * glm::rotate(i, -angle90, x_axis)
	);
}

void drawArm() {
	// Produce a three-unit arm (upperarm, forearm, hand) making use of your function
	// drawUnitCube to define each of them
	// 1) Define 3 global variables that control the angles between the arm parts and add
	//   cases to the keyboard function to control these values

	// 2) Use these variables to define your arm.
	//    Use glm::scale(Matrix, Vector) to achieve different arm lengths.
	//    Use glm::rotate(Matrix, Angle, Vector) to correctly place the elements

	// 3 Optional) make an animated snake out of these boxes
	//(an arm with 10 joints that moves using the animate function)
	glm::mat4 transform(1.0f), i(1.0f);
	glm::vec3 z_axis(0, 0, 1);  // rotation axis

	//glColor3f(1, 0, 0);
	transform = glm::rotate(i, arm_joint[0], z_axis) * transform;
	// NOTE: don't multiply "scale" to transformation maxtrix
	glm::mat4 upperarmTransform = transform * glm::scale(i, armScale[0]);
	drawUnitCube(upperarmTransform);

	//glColor3f(0, 1, 0);
	glm::vec3 translate1(0, 0, armScale[0].z);
	transform = glm::rotate(i, arm_joint[1], z_axis) *
		glm::translate(i, translate1) * transform;
	glm::mat4 forearmTransformation = transform * glm::scale(i, armScale[1]);
	drawUnitCube(forearmTransformation);

	//glColor3f(0, 0, 1);
	glm::vec3 translate2(0, 0, armScale[1].z);
	transform = glm::rotate(i, arm_joint[2], z_axis) *
		glm::translate(i, translate2) * transform;
	glm::mat4 handTransformation = transform * glm::scale(i, armScale[2]);
	drawUnitCube(handTransformation);
}

void drawLight() {
	// 1) Draw a cube at the light's position lightPos using your drawUnitCube function.
	//    To make the light source bright, follow the drawCoordSystem function
	//    To deactivate the lighting temporarily and draw it in yellow.

	// 2) Make the light position controllable via the keyboard function

	// 3) Add normal information to all your faces of the previous functions
	//    and observe the shading after pressing 'L' to activate the lighting.
	//    You can use 'l' to turn it off again.

	// 4) OPTIONAL
	//    Draw a sphere (consisting of triangles) instead of a cube.

	if (lighting_enabled) {
		glDisable(GL_LIGHTING);

		glColor3f(1.0f, 1.0f, 0.0f);  // yellow

		drawUnitCube(
			glm::scale(
				glm::translate(glm::mat4(1.0f), glm::vec3(lightPos)),
				glm::vec3(0.05, 0.05, 0.05)
			)
		);

		glEnable(GL_LIGHTING);

		//GLfloat lightingPos[] = {lightPos.x, lightPos.y, lightPos.z, lightPos.w};
		//glLightfv(GL_LIGHT0, GL_POSITION, lightingPos);
	}
}

void drawMesh() {
	// 1) Use the mesh data structure;
	//    Each triangle is defined with 3 consecutive indices in the meshTriangles table.
	//    These indices correspond to vertices stored in the meshVertices table.
	//    Provide a function that draws these triangles.

	// 2) Compute the normals of these triangles

	// 3) Try computing a normal per vertex as the average of the adjacent face normals.
	//    Call glNormal3f with the corresponding values before each vertex.
	//    What do you observe with respect to the lighting?

	// 4) Try loading your own model (export it from Blender as a Wavefront obj) and replace the provided mesh file.

	for (const auto& triangle : mesh.triangles) {
		Vertex& v1 = mesh.vertices[triangle.x], 
			v2 = mesh.vertices[triangle.y], 
			v3 = mesh.vertices[triangle.z];

		/*glVertex3f(v1.position.x, v1.position.y, v1.position.z);
		glVertex3f(v2.position.x, v2.position.y, v2.position.z);
		glVertex3f(v3.position.x, v3.position.y, v3.position.z);*/

		glm::vec3 edge1 = v1.position - v2.position, edge2 = v1.position - v3.position;
		glm::vec3 triangleNormal = glm::normalize(glm::cross(edge1, edge2));
		
		//glNormal3f(triangleNormal.x, triangleNormal.y, triangleNormal.z);

		v1.normal += triangleNormal;
		v2.normal += triangleNormal;
		v3.normal += triangleNormal;
	}

	for (Vertex& v : mesh.vertices) {
		v.normal = glm::normalize(v.normal);
	}

	glBegin(GL_TRIANGLES);
	for (const auto& triangle : mesh.triangles) {
		Vertex& v1 = mesh.vertices[triangle.x],
			v2 = mesh.vertices[triangle.y],
			v3 = mesh.vertices[triangle.z];

		glNormal3f(v1.normal.x, v1.normal.y, v1.normal.z);
		glVertex3f(v1.position.x, v1.position.y, v1.position.z);
		glNormal3f(v2.normal.x, v2.normal.y, v2.normal.z);
		glVertex3f(v2.position.x, v2.position.y, v2.position.z);
		glNormal3f(v3.normal.x, v3.normal.y, v3.normal.z);
		glVertex3f(v3.position.x, v3.position.y, v3.position.z);
	}
	glEnd();
}

void display() {
	// set the light to the right position
	drawLight();
	glLightfv(GL_LIGHT0, GL_POSITION, glm::value_ptr(lightPos));

	switch (displayMode) {
	case DisplayModeType::TRIANGLE:
		drawCoordSystem();
		drawTriangle();
		break;
	case DisplayModeType::FACE:
		drawCoordSystem();
		drawUnitFace(glm::mat4(1.0f)); // mat4(1.0f) = identity matrix
		break;
	case DisplayModeType::CUBE:
		drawCoordSystem();
		drawUnitCube(glm::mat4(1.0f));
		break;
	case DisplayModeType::ARM:
		drawCoordSystem();
		drawArm();
		break;
	case DisplayModeType::MESH:
		drawMesh();
		break;
	default:
		break;
	}
}

/**
 * Animation
 */
void animate() {
	//drawTriangle_vertex1_x += 1;
}

// Take keyboard input into account.
void keyboard(int key, int /* scancode */, int action, int /* mods */) {
	glm::dvec2 cursorPos = pWindow->getCursorPos();
	std::cout << "Key " << key << " pressed at " << cursorPos.x << ", " << cursorPos.y << "\n";

	if (key == '\\' && action == GLFW_PRESS) {
		show_imgui = !show_imgui;
	}

	switch (key) {
		case GLFW_KEY_1: {
			displayMode = DisplayModeType::TRIANGLE;
			break;
		}
		case GLFW_KEY_2: {
			displayMode = DisplayModeType::FACE;
			break;
		}
		case GLFW_KEY_3: {
			displayMode = DisplayModeType::CUBE;
			break;
		}
		case GLFW_KEY_4: {
			displayMode = DisplayModeType::ARM;
			break;
		}
		case GLFW_KEY_5: {
			displayMode = DisplayModeType::MESH;
			break;
		}
		case GLFW_KEY_ESCAPE: {
			pWindow->close();
			break;
		}
		case GLFW_KEY_L: {
			// Turn lighting on.
			if (pWindow->isKeyPressed(GLFW_KEY_LEFT_SHIFT) || pWindow->isKeyPressed(GLFW_KEY_RIGHT_SHIFT)) {
				lighting_enabled = true;
				glEnable(GL_LIGHTING);
			} else {
				lighting_enabled = false;
				glDisable(GL_LIGHTING);
			}
			break;
		}
		case GLFW_KEY_W: {
			if (lighting_enabled) lightPos.y += 0.1f;
			break;
		}
		case GLFW_KEY_S: {
			if (lighting_enabled) lightPos.y -= 0.1f;
			break;
		}
		case GLFW_KEY_A: {
			if (lighting_enabled) lightPos.x -= 0.1f;
			break;
		}
		case GLFW_KEY_D: {
			if (lighting_enabled) lightPos.x += 0.1f;
			break;
		}
		case GLFW_KEY_Q: {
			if (lighting_enabled) lightPos.z += 0.1f;
			break;
		}
		case GLFW_KEY_E: {
			if (lighting_enabled) lightPos.z -= 0.1f;
			break;
		}
	};
}

void imgui() {

	if (!show_imgui)
		return;

	ImGui::Begin("Practical 1: Intro to OpenGL");
	ImGui::Text("Press \\ to show/hide this menu");

	// Declare display modes and names
	std::array displayModeNames{ "1: TRIANGLE", "2: FACE", "3: CUBE", "4: ARM", "5: MESH" };

	const std::array displayModes{
		DisplayModeType::TRIANGLE,
		DisplayModeType::FACE,
		DisplayModeType::CUBE,
		DisplayModeType::ARM,
		DisplayModeType::MESH
	};

	// get the index of the current display mode, as current mode
	int current_mode = static_cast<int>(displayMode);

	// update current mode based on menu
	ImGui::Combo("Display Mode", &current_mode, displayModeNames.data(), displayModeNames.size());

	// set display mode
	displayMode = displayModes[current_mode];

	ImGui::Checkbox("Lighting Enabled", &lighting_enabled);

	if (lighting_enabled) {
		glEnable(GL_LIGHTING);
	} else {
		glDisable(GL_LIGHTING);
	}

	ImGui::Separator();
	ImGui::Text("Use this UI as an example, feel free to implement custom UI if it is useful");

	// Checkbox
	static bool checkbox;

	ImGui::Checkbox("Example Checkbox", &checkbox);

	// Slider
	static float sliderValue = 0.0f;
	ImGui::SliderFloat("Float Slider", &sliderValue, 0.0f, 1.0f);

	static int intSliderValue = 0.0f;
	ImGui::SliderInt("Int Slider", &intSliderValue, 0, 10);

	// TODO: Added by myself!!!!!!!!!!!!!! for exercise 1/4
	ImGui::SliderFloat("Exercise 1: x of the first vertex", &drawTriangle_vertex1_x, -2.0f, 2.0f);
	ImGui::SliderFloat("Exercise 4: the first arm joint", &arm_joint[0], -1.5f, 2.5f);
	ImGui::SliderFloat("Exercise 4: the second arm joint", &arm_joint[1], -2.3f, 1.7f);

	// Color Picker
	static glm::vec3 color = glm::vec3(0.45f, 0.55f, 0.60f);
	ImGui::ColorEdit3("Color Picker", glm::value_ptr(color)); // Use glm::value_ptr to get the float*

	if (ImGui::Button("Button")) {
		std::cout << "Button Clicked" << std::endl;
	}

	static char text[128] = "Some Text";
	ImGui::InputText("Input Text", text, IM_ARRAYSIZE(text));

	static int intValue = 0;
	ImGui::InputInt("Input Int", &intValue);

	static float floatValue = 0.0f;
	ImGui::InputFloat("Input Float", &floatValue);

	ImGui::End();
	ImGui::Render();
}

// Nothing needed below this point
// STOP READING //STOP READING //STOP READING

void displayInternal(void);
void reshape(const glm::ivec2&);
void init() {
	// Initialize viewpoint
	pTrackball->printHelp();
	reshape(resolution);

	glDisable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_NORMALIZE);

	// int MatSpec [4] = {1,1,1,1};
	//    glMaterialiv(GL_FRONT_AND_BACK,GL_SPECULAR,MatSpec);
	//    glMateriali(GL_FRONT_AND_BACK,GL_SHININESS,10);

	// Enable Depth test
	glEnable(GL_DEPTH_TEST);

	// Draw frontfacing polygons as filled.
	glPolygonMode(GL_FRONT, GL_FILL);
	// Draw backfacing polygons as outlined.
	glPolygonMode(GL_BACK, GL_LINE);
	glShadeModel(GL_SMOOTH);
	mesh = loadMesh("David.obj", true)[0];


}

// Program entry point. Everything starts here.
int main(int /* argc */, char** argv) {
	pWindow = std::make_unique<Window>(argv[0], resolution, OpenGLVersion::GL2);
	pTrackball = std::make_unique<Trackball>(pWindow.get(), glm::radians(50.0f));
	pWindow->registerKeyCallback(keyboard);
	pWindow->registerWindowResizeCallback(reshape);

	init();

	while (!pWindow->shouldClose()) {
		pWindow->updateInput();
		imgui();

		animate();
		displayInternal();

		pWindow->swapBuffers();
	}
}

// OpenGL helpers. You don't need to touch these.
void displayInternal(void) {
	// Clear screen
	glViewport(0, 0, pWindow->getWindowSize().x, pWindow->getWindowSize().y);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Load identity matrix
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Load up view transforms
	const glm::mat4 viewTransform = pTrackball->viewMatrix();
	glMultMatrixf(glm::value_ptr(viewTransform));

	// Your rendering function
	animate();
	display();
}
void reshape(const glm::ivec2& size) {
	// Called when the window is resized.
	// Update the viewport and projection matrix.
	glViewport(0, 0, size.x, size.y);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	const glm::mat4 perspectiveMatrix = pTrackball->projectionMatrix();
	glLoadMatrixf(glm::value_ptr(perspectiveMatrix));
	glMatrixMode(GL_MODELVIEW);
}
