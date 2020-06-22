#include "screenQuad.h"

ScreenQuad::ScreenQuad() {
  initShader();
  initTexture();
  initBuffer();
  initUniform();
}

ScreenQuad::~ScreenQuad() {}

void ScreenQuad::initShader() {
  shader =
      buildShader("./shader/vsScreenQuad.glsl", "./shader/fsScreenQuad.glsl");
}

void ScreenQuad::initUniform() {
  /* Screen quad */
  glUseProgram(shader);

  uniTex = myGetUniformLocation(shader, "tex");
  uniAlpha = myGetUniformLocation(shader, "alpha");
  uniBaseColor = myGetUniformLocation(shader, "baseColor");

  vec4 underwaterColor(0.0, 0.65, 0.75, 1.0);

  glUniform1i(uniTex, 10);
  glUniform1f(uniAlpha, 1.f);
  glUniform4fv(uniBaseColor, 1, value_ptr(underwaterColor));
}

void ScreenQuad::initTexture() {
  glActiveTexture(GL_TEXTURE0 + 10);

  glGenTextures(1, &tbo);
  glBindTexture(GL_TEXTURE_2D, tbo);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  // On OSX, must use WINDOW_WIDTH * 2 and WINDOW_HEIGHT * 2, don't know why
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WINDOW_WIDTH * 2, WINDOW_HEIGHT * 2,
               0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
}

void ScreenQuad::initBuffer() {
  /* Depth buffer */
  glGenRenderbuffers(1, &rboDepth);
  glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
  // On OSX, must use WINDOW_WIDTH * 2 and WINDOW_HEIGHT * 2, don't know why
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, WINDOW_WIDTH * 2,
                        WINDOW_HEIGHT * 2);

  /* Framebuffer to link everything together */
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         tbo, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, rboDepth);

  GLenum status;
  if ((status = glCheckFramebufferStatus(GL_FRAMEBUFFER)) !=
      GL_FRAMEBUFFER_COMPLETE) {
    fprintf(stderr, "glCheckFramebufferStatus: error %u", status);
    exit(EXIT_FAILURE);
  }

  // screen quad
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 8 * 2, vtxs, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(0);
}

void ScreenQuad::draw(vec3 eyePoint) {
  glUseProgram(shader);

  // for underwater scene
  if (eyePoint.y < 0.f) {
    glUniform1f(uniAlpha, 0.5f);
  } else {
    glUniform1f(uniAlpha, 1.f);
  }

  glBindVertexArray(vao);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
