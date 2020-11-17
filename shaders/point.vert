#version 330

uniform mat4 view;
uniform mat4 proj;

in vec3 position;

const vec3 color = vec3(1.0,1.0,1.0);

out vec3 vertex_color;

void main() {
    vertex_color = color;

    vec4 pos = proj * view * vec4(position, 1.0);

    gl_PointSize = 50.0 / pos.z;//TODO: Arbitrary -> use projection and radius
    gl_Position = pos;
}
