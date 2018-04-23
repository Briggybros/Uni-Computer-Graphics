#include <iostream>
#include <glm/glm.hpp>
#include <stdint.h>
#include <assert.h>

using namespace std;
using glm::ivec2;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat3;
using glm::mat4;

struct Pixel {
  int x;
  int y;
  float zinv;
  vec4 homogenous;
  vec4 worldPos;
  vec4 normal;
  vec3 reflectance;

  Pixel() {};
  Pixel(ivec2 vec): x(vec.x), y(vec.y) {};
  Pixel(ivec2 vec, float zinv, vec4 homogenous, vec4 worldPos, vec4 normal, vec3 reflectance): x(vec.x), y(vec.y), zinv(zinv), homogenous(homogenous), worldPos(worldPos), normal(normal), reflectance(reflectance) {};
};

struct Vertex {
  vec4 position;
  vec4 normal;
  vec3 reflectance;

  Vertex() {};
  Vertex(vec4 position): position(position) {};
  Vertex(vec4 position, vec4 normal, vec3 reflectance): position(position), normal(normal), reflectance(reflectance) {};
};

mat4 CalcRotationMatrix(vec3 rotation) {
  float x = rotation.x;
  float y = rotation.y;
  float z = rotation.z;
  mat4 Rx = mat4(vec4(1, 0, 0, 0), vec4(0, cosf(x), sinf(x), 0), vec4(0, -sinf(x), cosf(x), 0), vec4(0, 0, 0, 1));
  mat4 Ry = mat4(vec4(cosf(y), 0, -sinf(y), 0), vec4(0, 1, 0, 0), vec4(sinf(y), 0, cosf(y), 0), vec4(0 , 0, 0, 1));
  mat4 Rz = mat4(vec4(cosf(z), sinf(z), 0, 0), vec4(-sinf(z), cosf(z), 0, 0), vec4(0, 0, 1, 0), vec4(0, 0, 0, 1));
  return Rz * Ry * Rx;
}
