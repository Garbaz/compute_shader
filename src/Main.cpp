#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstdlib>

#include "Camera.hpp"
#include "Main.hpp"

#define DBG(s) std::cerr << "DEBUG @" <<  __LINE__ << " : " << s << std::endl

#define PRINT_DEBUG true

/** KEEP IN SYNC WITH SHADER **/
#define WORKGROUP_SIZE (1024)
#define BINDING_INDEX_POS (4)
#define BINDING_INDEX_VEL (5)
/** KEEP IN SYNC WITH SHADER**/


#define NUM_WORKGROUPS (1024)

#define NUM_PARTICLES (NUM_WORKGROUPS * WORKGROUP_SIZE)

GLFWwindow *window;
glm::ivec2 viewport_size;

bool mouse_captured = false;
glm::ivec2 prev_cursor_pos;

Camera camera(glm::vec3(0,0,-30));

int main() {
    srand(time(0));

    init_glfw();
    init_glew();

    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_DEPTH_TEST);

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint ssbo_pos;
    glGenBuffers(1, &ssbo_pos);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_pos);
    glBufferData(GL_SHADER_STORAGE_BUFFER, NUM_PARTICLES * sizeof(glm::vec3), NULL, GL_STATIC_DRAW);
    {
        glm::vec3 *poss = (glm::vec3 *)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
        for (size_t i = 0; i < NUM_PARTICLES; i++) {
            poss[i] = glm::linearRand(glm::vec3(-1.0),glm::vec3(-1.0));
        }
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_INDEX_POS, ssbo_pos);

    GLuint ssbo_vel;
    glGenBuffers(1, &ssbo_vel);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_vel);
    glBufferData(GL_SHADER_STORAGE_BUFFER, NUM_PARTICLES * sizeof(glm::vec3), NULL, GL_STATIC_DRAW);
    {
        glm::vec3 *vels = (glm::vec3 *)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
        for (size_t i = 0; i < NUM_PARTICLES; i++) {
            vels[i] = glm::ballRand(5.0);
        }
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BINDING_INDEX_VEL, ssbo_vel);

    GLuint compute_shader = compile_shader("shaders/compute.comp", GL_COMPUTE_SHADER);
    GLuint compute_program = glCreateProgram();
    glAttachShader(compute_program, compute_shader);
    glLinkProgram(compute_program);

    GLuint vertex_shader = compile_shader("shaders/point.vert", GL_VERTEX_SHADER);
    GLuint fragment_shader = compile_shader("shaders/point.frag", GL_FRAGMENT_SHADER);
    GLuint render_program = glCreateProgram();
    glAttachShader(render_program, vertex_shader);
    glAttachShader(render_program, fragment_shader);
    glBindFragDataLocation(render_program, 0, "out_color");
    glLinkProgram(render_program);

    GLint attrib_loc_position = glGetAttribLocation(render_program, "position");
    glEnableVertexAttribArray(attrib_loc_position);
    glBindBuffer(GL_ARRAY_BUFFER, ssbo_pos);
    glVertexAttribPointer(attrib_loc_position, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
    // glEnableVertexAttribArray(attrib_loc_position);
    // glVertexAttribPointer(attrib_loc_position, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (void *)offsetof(Particle, pos));

    float aspect = float(viewport_size.x) / float(viewport_size.y);
    float fovy = FIELD_OF_VIEW_HORIZONTAL / aspect;
    glm::mat4 proj = glm::perspective(glm::radians(fovy), aspect, 0.01f, 100.0f);

    GLint uniform_time = glGetUniformLocation(render_program, "time");
    GLint uniform_deltatime = glGetUniformLocation(render_program, "deltatime");
    GLint uniform_view = glGetUniformLocation(render_program, "view");
    GLint uniform_proj = glGetUniformLocation(render_program, "proj");
    glUseProgram(render_program);
    glUniformMatrix4fv(uniform_proj, 1, GL_FALSE, glm::value_ptr(proj));

    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_callback);

    double start_time = glfwGetTime();
    double last_frame_time = start_time;

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update time & deltatime
        double frame_time = glfwGetTime();
        double time = frame_time - start_time;
        double deltatime = frame_time - last_frame_time;
        last_frame_time = frame_time;

        // --- COMPUTE ---
        glUseProgram(compute_program);

        glDispatchCompute(NUM_WORKGROUPS, 1, 1);

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // --- RENDER ---
        glUseProgram(render_program);

        // Update time uniforms
        glUniform1f(uniform_time, time);
        glUniform1f(uniform_deltatime, deltatime);

        // Update camera
        camera.update(deltatime);
        if (camera.dirty) {
            glUniformMatrix4fv(uniform_view, 1, GL_FALSE, glm::value_ptr(camera.get_view_matrix()));
        }

        glDrawArrays(GL_POINTS, 0, NUM_PARTICLES);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        exit(0);
    } else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        if (mouse_captured) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            mouse_captured = false;
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            mouse_captured = true;
        }
    } else if (key == GLFW_KEY_W) {
        if (action == GLFW_PRESS) {
            camera.input_movement.y++;
        } else if (action == GLFW_RELEASE) {
            camera.input_movement.y--;
        }
    } else if (key == GLFW_KEY_S) {
        if (action == GLFW_PRESS) {
            camera.input_movement.y--;
        } else if (action == GLFW_RELEASE) {
            camera.input_movement.y++;
        }
    } else if (key == GLFW_KEY_A) {
        if (action == GLFW_PRESS) {
            camera.input_movement.x++;
        } else if (action == GLFW_RELEASE) {
            camera.input_movement.x--;
        }
    } else if (key == GLFW_KEY_D) {
        if (action == GLFW_PRESS) {
            camera.input_movement.x--;
        } else if (action == GLFW_RELEASE) {
            camera.input_movement.x++;
        }
    }
}

void cursor_callback(GLFWwindow *window, double xpos, double ypos) {
    if (mouse_captured) {
        camera.update_direction(xpos - prev_cursor_pos.x, ypos - prev_cursor_pos.y);
    }
    prev_cursor_pos.x = xpos;
    prev_cursor_pos.y = ypos;
}

void init_glfw() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        exit(-1);
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *vidmode = glfwGetVideoMode(monitor);
    glfwWindowHint(GLFW_RED_BITS, vidmode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, vidmode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, vidmode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, vidmode->refreshRate);
    // window = glfwCreateWindow(vidmode->width, vidmode->height, "RaytraceGL", monitor, NULL);
    window = glfwCreateWindow(1200, 675, "GL Points", NULL, NULL);

    glfwGetFramebufferSize(window, &viewport_size.x, &viewport_size.y);

    glfwMakeContextCurrent(window);
}

void init_glew() {
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        std::cerr << "glew init error: " << glewGetErrorString(err) << std::endl;
        exit(-1);
    }

#if PRINT_DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(message_callback, 0);
#endif
}

GLuint compile_shader(std::string filename, GLenum shader_type) {
    std::string source;
    try {
        source = read_file(filename);
    } catch (std::string err) {
        std::cerr << "ERROR: " << err << std::endl;
        exit(-1);
    }
    const char *source_cstr = source.c_str();

    GLuint shader = glCreateShader(shader_type);
    glShaderSource(shader, 1, &source_cstr, NULL);
    glCompileShader(shader);
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
#if PRINT_DEBUG
    std::cerr << "Shader \"" << filename << "\" compiled " << (status == GL_TRUE ? "successful" : "unsuccessful") << "." << std::endl;

    GLint info_log_length;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_log_length);
    if (info_log_length > 0) {
        char info_log[info_log_length];
        glGetShaderInfoLog(shader, info_log_length, NULL, info_log);
        std::cerr << "Shader compolation log:" << std::endl;
        std::cerr << info_log << std::endl;
    }
#endif
    if (status != GL_TRUE) {
        exit(-1);
    }
    return shader;
}

std::string read_file(std::string filename) {
    std::ifstream f(filename);
    if (f) {
        std::ostringstream ss;
        ss << f.rdbuf();
        return ss.str();
    } else {
        throw "Could not read file \"" + filename + "\"";
    }
}

void GLAPIENTRY message_callback(GLenum source,
                                 GLenum type,
                                 GLuint id,
                                 GLenum severity,
                                 GLsizei length,
                                 const GLchar *message,
                                 const void *user_param) {
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
            (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
            type, severity, message);
}

glm::vec3 random_saturated_color() {
    float m = glm::linearRand(0, 1);
    switch (rand() % 6) {
        case 0:
            return glm::vec3(1, 0, m);
        case 1:
            return glm::vec3(1, m, 0);
        case 2:
            return glm::vec3(0, m, 1);
        case 3:
            return glm::vec3(0, 1, m);
        case 4:
            return glm::vec3(m, 1, 0);
        case 5:
            return glm::vec3(m, 0, 1);
    }
}