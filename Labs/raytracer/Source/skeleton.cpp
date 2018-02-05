#include <iostream>
#include <stdlib.h>
#include <random> 
#include <time.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <algorithm>
#include <fstream>
#include <string>
#include <sstream>
#include <iterator>
#include <glm/glm.hpp>
#include <SDL.h>
#include "SDLauxiliary.h"
#include "TestModelH.h"

using namespace std;
using glm::mat3;
using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;

#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 256
#define FULLSCREEN_MODE false
#define NUM_RAYS 0
#define BOUNCES 3
#define MIN_BOUNCES 4
#define MAX_BOUNCES 10
#define NUM_SAMPLES 2
#define LIVE

float m = numeric_limits<float>::max();
vec4 lightPos(0, -0.5, -0.7, 1.0);
vec3 lightColor = 14.f * vec3(1, 1, 1); 
vec3 indirectLighting = 0.5f * vec3(1, 1, 1);
float apertureSize = 0.1;

/* ----------------------------------------------------------------------------*/
/* STRUCTS                                                                     */
struct Camera {
  float focalLength;
  vec4 position;
  mat3 R;
  vec3 rotation;
  vec3 movement;
  float movementSpeed;
  float rotationSpeed;
};

/* ----------------------------------------------------------------------------*/
/* FUNCTIONS                                                                   */

void Update(Camera &camera, screen *screen);
void Draw(screen *screen, Camera camera);
bool ClosestIntersection(vec4 start, vec4 dir, Intersection &closestIntersection);
vec3 DirectLight(const Intersection &intersection, vec4 dir, bool spec);
vec3 IndirectLight(const Intersection &intersection, vec4 dir, int bounce, bool spec);
mat3 CalcRotationMatrix(float x, float y, float z);
vec3 uniformSampleHemisphere(const float &r1, const float &r2);
void createCoordinateSystem(const vec3 &N, vec3 &Nt, vec3 &Nb);
vec3 Light(const vec4 start, const vec4 dir, int bounce);
float max3(vec3);
void LoadModel(std::vector<Shape *>& shapes, string path);

float samples = 0;
vector<Shape *> shapes;
int main(int argc, char *argv[]) {
  screen *screen = InitializeSDL(SCREEN_WIDTH, SCREEN_HEIGHT, FULLSCREEN_MODE);

  LoadTestModel(shapes);
  LoadModel(shapes, "/home/gregory/pug/model-triangulated.obj");

  Camera camera = {
    SCREEN_HEIGHT,
    vec4(0, 0, -3, 1),
    mat3(1),
    vec3(0, 0, 0),
    vec3(0, 0, 0),
    0.001,
    0.001,
  };

  srand(42);
  memset(screen->buffer, 0, screen->height * screen->width * sizeof(uint32_t));
#ifdef LIVE
  while (NoQuitMessageSDL()) {
    Update(camera, screen);
    Draw(screen, camera);
    SDL_Renderframe(screen);
  }
#else
  Update(camera, screen);
  Draw(screen, camera);
  SDL_Renderframe(screen);
  Update(camera, screen);
#endif

  SDL_SaveImage(screen, "screenshot.bmp");

  KillSDL(screen);
  return 0;
}

/*Place your drawing here*/
void Draw(screen *screen, Camera camera) {
  samples++;

  float depth[SCREEN_WIDTH][SCREEN_HEIGHT];
  float tempDepth;
  float maxDepth = -1;

  Intersection intersection;
  int x, y;
  #pragma omp parallel for private(x, y, intersection, tempDepth, maxDepth)
  for (y = -SCREEN_HEIGHT/2; y < SCREEN_HEIGHT/2; y++) {
    for (x = -SCREEN_WIDTH/2; x < SCREEN_WIDTH/2; x++) {
      vec3 color = vec3(0);
      tempDepth = 0;
#if NUM_RAYS <= 1
      vec4 direction = glm::normalize(vec4(vec3(x, y, camera.focalLength) * camera.R, 1));
      color += Light(camera.position, direction, 0);
      tempDepth += intersection.distance;
#else
      for (int i = -NUM_RAYS/2; i < NUM_RAYS/2; i++) {
        for (int j = -NUM_RAYS/2; j < NUM_RAYS/2; j++) {
          vec4 direction = glm::normalize(vec4(vec3((float) x + apertureSize*i, (float) y + apertureSize*j, camera.focalLength) * camera.R, 1));
          color += Light(camera.position, direction, 0);
          tempDepth += intersection.distance;
        }
      }
      color /= NUM_RAYS * NUM_RAYS;
      tempDepth /= NUM_RAYS * NUM_RAYS;
#endif
      depth[x + SCREEN_WIDTH/2][y + SCREEN_HEIGHT/2] = tempDepth;
      if (tempDepth > maxDepth) {
        maxDepth = tempDepth;
      }
      PutPixelSDL(screen, x + SCREEN_WIDTH/2, y + SCREEN_HEIGHT/2, color, samples);
    }
  }

  // for (int y = 0; y < SCREEN_HEIGHT; y++) {
  //   for (int x = 0; x < SCREEN_WIDTH; x++) {
  //     if (depth[x][y] == 0) PutPixelSDL(screen, x, y, vec3(0));
  //     PutPixelSDL(screen, x, y, vec3(1 - (depth[x][y] / maxDepth)));
  //   }
  // }
}

/*Place updates of parameters here*/
void Update(Camera &camera, screen *screen) {
  static int t = SDL_GetTicks();
  /* Compute frame time */
  int t2 = SDL_GetTicks();
  float dt = float(t2 - t);
  t = t2;
  cout << "Render time: " << dt << " ms.\n";
  /* Update variables*/

  camera.movement = vec3(0);
  vec3 tempRot = camera.rotation;

  const Uint8 *keystate = SDL_GetKeyboardState(NULL);
  if (keystate[SDL_SCANCODE_W]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      camera.rotation.x -= camera.rotationSpeed * dt;
    } else {
      camera.movement.z += camera.movementSpeed * dt;
    }
  }

  if (keystate[SDL_SCANCODE_S]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      camera.rotation.x += camera.rotationSpeed * dt;
    } else {
      camera.movement.z -= camera.movementSpeed * dt;
    }
  }

  if (keystate[SDL_SCANCODE_A]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      camera.rotation.y += camera.rotationSpeed * dt;
    } else {
      camera.movement.x -= camera.movementSpeed * dt;
    }
  }

  if (keystate[SDL_SCANCODE_D]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      camera.rotation.y -= camera.rotationSpeed * dt;
    } else {
      camera.movement.x += camera.movementSpeed * dt;
    }
  }

  if (keystate[SDL_SCANCODE_Q]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      camera.rotation.z += camera.rotationSpeed * dt;
    } else {
      camera.movement.y += camera.movementSpeed * dt;
    }
  }

  if (keystate[SDL_SCANCODE_E]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      camera.rotation.z -= camera.rotationSpeed * dt;
    } else {
      camera.movement.y -= camera.movementSpeed * dt;
    }
  }

  camera.R = CalcRotationMatrix(camera.rotation.x, camera.rotation.y, camera.rotation.z);
  camera.position += vec4(vec3(camera.movement * camera.R), 0);
  if (camera.movement != vec3(0) || camera.rotation != tempRot) {
    memset(screen->buffer, 0, screen->height * screen->width * sizeof(uint32_t));
    memset(screen->pixels, 0, screen->height * screen->width * sizeof(vec3));
    samples = 0;
  }
}

bool ClosestIntersection(vec4 start, vec4 dir, Intersection &closestIntersection) {
  closestIntersection.distance = m;
  closestIntersection.shapeIndex = -1;

  for (uint index = 0; index < shapes.size(); index++) {
    Shape *shape = shapes[index];
    float dist = shape->intersects(start, dir);
    if (dist > 0 && dist < closestIntersection.distance) {
      closestIntersection.distance = dist;
      closestIntersection.shapeIndex = index;
      closestIntersection.position = start + dist * dir;
    }
  }

  return closestIntersection.shapeIndex != -1;
}

std::default_random_engine generator; 
std::uniform_real_distribution<float> distribution(0, 1);

vec3 IndirectLight(const Intersection &intersection, vec4 dir, int bounce, bool spec) {
#if BOUNCES == 0
    return indirectLighting;
#else
  if (bounce == 0) {
    return vec3(0);
  } else {
    vec4 n = shapes[intersection.shapeIndex]->getNormal(intersection.position);
    vec3 light = vec3(0);
    vec3 Nt, Nb;
    createCoordinateSystem(vec3(n), Nt, Nb);
    int count = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
      if (rand() / RAND_MAX > 0.5) continue;
      else count++;
      Intersection reflectIntersection;
      float r1 = distribution(generator);
      float r2 = distribution(generator);
      vec3 sample = uniformSampleHemisphere(r1, r2); 
      vec3 sampleWorld;
      if (spec) {
        sampleWorld = vec3(mat3(Nb, vec3(glm::reflect(dir, n)), Nt) * sample);
        vec3 P = IndirectLight(reflectIntersection, vec4(sampleWorld, 1), bounce - 1, spec);
        light += (P * max(glm::dot(vec4(sampleWorld, 1), dir), 0.0f));
      } else {
        sampleWorld = vec3(mat3(Nb, vec3(n), Nt) * sample);
        vec4 rayDir = vec4(sampleWorld, 1);
        if (ClosestIntersection(intersection.position + (rayDir * 1e-4f), rayDir, reflectIntersection)) {
          vec3 tint = shapes[reflectIntersection.shapeIndex]->color;
          vec3 P = DirectLight(intersection, dir, false) + IndirectLight(reflectIntersection, rayDir, bounce - 1, spec);
          light += (P * max(glm::dot(rayDir, n), 0.0f)) * tint;
        }
      }
    }
    light /= count;;
    return light;
  }
#endif
}

vec3 DirectLight(const Intersection &intersection, vec4 dir, bool spec) {
  vec3 P = lightColor;
  vec4 n = shapes[intersection.shapeIndex]->getNormal(intersection.position);
  vec4 r = lightPos - intersection.position;
  float rL = glm::length(r);
  vec4 rN = r / rL;
  Intersection shadowIntersection;
  if (ClosestIntersection(intersection.position + (rN * 1e-4f), rN, shadowIntersection)) {
    float distance = glm::distance(intersection.position, shadowIntersection.position);
    if (distance < rL) {
      return vec3(0);
    }
  }
  if (spec) {
    vec4 reflected = glm::reflect(rN, n);
    float shininess = shapes[intersection.shapeIndex]->shininess;
    return (P * max(powf(glm::dot(reflected, dir), shininess), 0.0f)) / (float) (4 * M_PI * powf(rL, 2));
  } else {
    return (P * max(glm::dot(rN, n), 0.0f)) / (float) (4 * M_PI * powf(rL, 2));
  }
}

vec3 Light(const vec4 start, const vec4 dir, int bounce) {
  Intersection intersection;
  if (ClosestIntersection(start, dir, intersection)) {

    Shape *obj = shapes[intersection.shapeIndex];
    // Russian roulette termination
    float U = rand() / (float) RAND_MAX;
    if (bounce > MIN_BOUNCES && (bounce > MAX_BOUNCES || U > max3(obj->color))) {
      // terminate
      return vec3(0);
    }
    
    vec4 hitPos = intersection.position;
    vec4 normal = obj->getNormal(hitPos);

    // Direct Light
    vec3 directDiffuseLight = vec3(0);
    vec3 directSpecularLight = vec3(0);
    for (Shape *light : shapes) {
      if (light->isLight()) {
        vec4 lightPos = light->randomPoint();
        float lightDist = glm::distance(lightPos, hitPos);
        vec4 lightDir = (lightPos - hitPos) / lightDist;
        if (bounce == 0) {
          vec4 reflected = glm::reflect(lightDir, normal);
          directSpecularLight += (light->emit * max(powf(glm::dot(reflected, dir), obj->shininess), 0.0f)) / (float) (4 * M_PI * powf(lightDist, 2));
        }
        Intersection lightIntersection;
        if (ClosestIntersection(hitPos, lightDir, lightIntersection)) {
          if (light == obj) {
            directDiffuseLight += light->emit * max(glm::dot(lightDir, normal), 0.0f) / (float) (4 * M_PI * powf(lightDist, 2));
          }
        }
      }
    }

    // Indirect Light
    vec3 Nt, Nb;
    createCoordinateSystem(vec3(normal), Nt, Nb);
    float r1 = distribution(generator);
    float r2 = distribution(generator);
    vec3 sample = uniformSampleHemisphere(r1, r2); 
    vec3 sampleWorld = vec3(mat3(Nb, vec3(normal), Nt) * sample);
    vec4 rayDir = vec4(sampleWorld, 1);
    vec3 indirectLight = Light(hitPos, rayDir, bounce + 1);


    // if (bounce == 0) {
      return obj->emit + obj->Kd * obj->color * (directDiffuseLight + indirectLight) /*+ obj->Ks * directSpecularLight*/;
    // } else {
      // return obj->color * (directLight + indirectLight);
    // }
  }
  return vec3(0);
}

mat3 CalcRotationMatrix(float x, float y, float z) {
  mat3 Rx = mat3(vec3(1, 0, 0), vec3(0, cosf(x), sinf(x)), vec3(0, -sinf(x), cosf(x)));
  mat3 Ry = mat3(vec3(cosf(y), 0, -sinf(y)), vec3(0, 1, 0), vec3(sinf(y), 0, cosf(y)));
  mat3 Rz = mat3(vec3(cosf(z), sinf(z), 0), vec3(-sinf(z), cosf(z), 0), vec3(0, 0, 1));
  return Rx * Ry * Rz;
}

vec3 uniformSampleHemisphere(const float &r1, const float &r2) {
  float sinTheta = sqrtf(1 - r1 * r1); 
  float phi = 2 * M_PI * r2; 
  float x = sinTheta * cosf(phi); 
  float z = sinTheta * sinf(phi); 
  return vec3(x, r1, z); 
}

void createCoordinateSystem(const vec3 &N, vec3 &Nt, vec3 &Nb) { 
  if (fabs(N.x) > fabs(N.y)) 
      Nt = glm::normalize(vec3(N.z, 0, -N.x)); 
  else 
      Nt = glm::normalize(vec3(0, -N.z, N.y)); 
  Nb = glm::cross(N, Nt); 
}

float max3(vec3 v) {
  return max(v.x, max(v.y, v.z));
}

vector<string> tokenize(string str, char delim) {
  std::istringstream ss(str);
	std::string token;

	vector<string> toks;
	while(std::getline(ss, token, delim)) {
		toks.push_back(token);
	}

  return toks;
}
 
vector<Shape *> makeTriangles(vector<string> facePoints, vector<vec3> vertices) {
  vector<int> vertexIds;
  for (uint i = 0; i < facePoints.size(); i++) {
    vector<string> comps = tokenize(facePoints[i].c_str(), '/');
    int parsedIndex = stoi(comps[0]);
    int index = parsedIndex < 0 ? vertices.size() + parsedIndex : parsedIndex;
    vertexIds.push_back(index);
  }

  vector<Shape *> triangles;
  if (vertexIds.size() == 3) {
    triangles.push_back(
      new Triangle(
        vec4(vertices[vertexIds[0]], 1),
        vec4(vertices[vertexIds[2]], 1),
        vec4(vertices[vertexIds[1]], 1),
        vec3(0),
        vec3(1, 0, 1),
        100, 0.25, 0.75
      )
    );
  } else if (vertexIds.size() == 4) {
    triangles.push_back(
      new Triangle(
        vec4(vertices[vertexIds[0]], 1),
        vec4(vertices[vertexIds[2]], 1),
        vec4(vertices[vertexIds[1]], 1),
        vec3(0),
        vec3(1, 0, 1),
        100, 0.25, 0.75
      )
    );
    triangles.push_back(
      new Triangle(
        vec4(vertices[vertexIds[2]], 1),
        vec4(vertices[vertexIds[1]], 1),
        vec4(vertices[vertexIds[3]], 1),
        vec3(0),
        vec3(1, 0, 1),
        100, 0.25, 0.75
      )
    );
  } else {
    throw 5; // 5 or more vertices on face
  }
  return triangles;
}

string trim(const string& str) {
  size_t first = str.find_first_not_of(" \t\r\n");
  size_t last = str.find_last_not_of(" \t\r\n");
  return str.substr(first, (last-first+1));
}

void LoadModel(std::vector<Shape *>& shapes, string path) {

  // See here: https://www.cs.cmu.edu/~mbz/personal/graphics/obj.html
  vector<vec3> vertices;
  vector<vec2> texture;
  vector<vec3> normals;

  ifstream objFile (path);
  if (!objFile) {
    cerr << "Cannot open " << path << std::endl;
    exit(1);
  }

  string line;
  while (getline(objFile, line)) {
    if (line.substr(0, 2) == "v ") {
      istringstream v(trim(line.substr(2)));
      vec3 vert;
      float x, y, z;
      v >> x >> y >> z;
      vert = vec3(x,y,z);
      vertices.push_back(vert);
    } else if (line.substr(0, 2) == "vt") {
      istringstream vt(trim(line.substr(3)));
      vec2 tex;
      int u, v;
      vt >> u >> v;
      tex = vec2(u, v);
      texture.push_back(tex);
    } else if (line.substr(0, 2) == "vn") {
      istringstream vn(trim(line.substr(3)));
      vec3 norm;
      int x, y, z;
      vn >> x >> y >> z;
      norm = vec3(x, y, z);
      normals.push_back(norm);
    } else if(line.substr(0, 2) == "f ") {
      string faceDeets = trim(line.substr(2));
      vector<string> facePoints = tokenize(faceDeets, ' ');
      
      vector<Shape *> tris = makeTriangles(facePoints, vertices);
      shapes.insert(shapes.end(), tris.begin(), tris.end());
    }
  }
}