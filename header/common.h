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

#include <GLFW/glfw3.h>
#include <FreeImage.h>

using namespace std;
using namespace glm;

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

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
GLuint buildShader(string, string);
GLuint compileShader(string, GLenum);
GLuint linkShader(GLuint, GLuint);
void createMesh(Mesh &);
void updateMesh(Mesh &);
void findAABB(Mesh &);
void drawBox(vec3, vec3);
