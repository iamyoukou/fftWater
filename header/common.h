#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext.hpp>
#include <assimp/Importer.hpp>  // C++ importer interface
#include <assimp/scene.h>       // Output data structure
#include <assimp/postprocess.h> // Post processing flags

#include <GLFW/glfw3.h>
#include <FreeImage.h>

using namespace std;
using namespace glm;

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

typedef struct {
  vec3 pos;
  vec3 color;
  vec3 v;
  float m;
} Point;

typedef struct {
  // data index
  GLuint v1, v2, v3;
  GLuint vt1, vt2, vt3;
  GLuint vn1, vn2, vn3;
} Face;

class Mesh {
public:
  // mesh data
  std::vector<glm::vec3> vertices;
  std::vector<glm::vec2> uvs;
  std::vector<glm::vec3> faceNormals;
  std::vector<Face> faces;

  // opengl data
  GLuint vboVtxs, vboUvs, vboNormals;
  GLuint vao;

  // aabb
  vec3 min, max;

  /* Constructors */
  Mesh(){};
  ~Mesh() {
    glDeleteBuffers(1, &vboVtxs);
    glDeleteBuffers(1, &vboUvs);
    glDeleteBuffers(1, &vboNormals);
    glDeleteVertexArrays(1, &vao);
  };

  /* Member functions */
  void translate(glm::vec3);
  void scale(glm::vec3);
  void rotate(glm::vec3);
};

std::string readFile(const std::string);
Mesh loadObj(std::string);
void printLog(GLuint &);
GLint myGetUniformLocation(GLuint &, string);
GLuint buildShader(string, string, string = "", string = "", string = "");
GLuint compileShader(string, GLenum);
GLuint linkShader(GLuint, GLuint, GLuint, GLuint, GLuint);

#endif
