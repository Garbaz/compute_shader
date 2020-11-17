#pragma once

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void cursor_callback(GLFWwindow *window, double xpos, double ypos);
void init_glfw();
void init_glew();
GLuint compile_shader(std::string filename, GLenum shader_type);
std::string read_file(std::string filename);
void GLAPIENTRY message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *user_param);
glm::vec3 random_saturated_color();