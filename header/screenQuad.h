#ifndef SCREEN_QUAD_H
#define SCREEN_QUAD_H

#include "common.h"

class ScreenQuad {
public:
  GLuint fbo, tbo, vao;
  GLuint vbo, rboDepth;
  GLuint shader;
  GLint uniTex, uniAlpha, uniBaseColor;

  GLfloat vtxs[8] = {
      -1, -1, 1, -1, -1, 1, 1, 1,
  };

  ScreenQuad();
  ~ScreenQuad();

  void initShader();
  void initUniform();
  void initTexture();
  void initBuffer();
  void draw(vec3);
};

#endif
