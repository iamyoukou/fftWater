#ifndef OCEAN_H
#define OCEAN_H

#include <complex>

#include "timer.h"
#include "fft.h"
#include "common.h"

typedef complex<double> Complex;

float uniformRandomVariable();
Complex gaussianRandomVariable();

struct vertex_ocean {
  GLfloat x, y, z;    // vertex
  GLfloat nx, ny, nz; // normal
  GLfloat a, b, c;    // htilde0
  GLfloat _a, _b, _c; // htilde0mk conjugate
  GLfloat ox, oy, oz; // original position
};

struct complex_vector_normal { // structure used with discrete fourier transform
  Complex h;                   // wave height
  vec2 D;                      // displacement
  vec3 n;                      // normal
};

class cOcean {
public:
  bool geometry; // flag to render geometry or surface

  float g;          // gravity constant
  int N, Nplus1;    // dimension -- N should be a power of 2
  float A;          // phillips spectrum parameter -- affects heights of waves
  vec2 w;           // wind parameter
  float length;     // length parameter
  Complex *h_tilde, // for fast fourier transform
      *h_tilde_slopex, *h_tilde_slopez, *h_tilde_dx, *h_tilde_dz;
  cFFT *fft; // fast fourier transform

  vertex_ocean *vertices;                  // vertices for vertex buffer object
  unsigned int *indices;                   // indicies for vertex buffer object
  unsigned int indices_count;              // number of indices to render
  GLuint vboVtxs, vboNs, vbo_indices, vao; // vertex buffer objects

  GLuint glProgram, glShaderV, glShaderF; // shaders
  GLint vertex, normal, texture, light_position, projection, view,
      model; // attributes and uniforms

  GLfloat *aWaterVtxs, *aWaterNs;
  int nOfQuads;
  vector<Point> points;

protected:
public:
  cOcean(const int N, const float A, const vec2 w, const float length,
         bool geometry);
  ~cOcean();
  void release();

  float dispersion(int n_prime, int m_prime); // deep water
  float phillips(int n_prime, int m_prime);   // phillips spectrum
  Complex hTilde_0(int n_prime, int m_prime);
  Complex hTilde(float t, int n_prime, int m_prime);
  complex_vector_normal h_D_and_n(vec2 x, float t);
  void evaluateWaves(float t);
  void evaluateWavesFFT(float t);
  void render(float t, glm::vec3 light_pos, glm::mat4 Projection,
              glm::mat4 View, glm::mat4 Model, bool use_fft);
  void computeWaterGeometry();
  void updateWaterGeometry();
  void drawPoints();
  void drawMesh();
};

#endif
