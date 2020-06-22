#include "skybox.h"

Skybox::Skybox() {
  initShader();
  initUniform();
  initTexture();
  initBuffer();
}

Skybox::~Skybox() {}

void Skybox::draw(mat4 model, mat4 view, mat4 projection, vec3 eyePoint) {
  glUseProgram(shader);

  glUniformMatrix4fv(uniV, 1, GL_FALSE, value_ptr(view));
  glUniformMatrix4fv(uniP, 1, GL_FALSE, value_ptr(projection));

  // Let the center of the skybox always at eyePoint
  // CAUTION: the matrix of GLM is column major
  M = model;
  M[3][0] += eyePoint.x;
  M[3][1] += eyePoint.y;
  M[3][2] += eyePoint.z;
  glUniformMatrix4fv(uniM, 1, GL_FALSE, value_ptr(M));

  glUseProgram(shader);
  glBindVertexArray(vao);
  glDrawArrays(GL_TRIANGLES, 0, 36);
}

void Skybox::initTexture() {
  // texture
  glActiveTexture(GL_TEXTURE0);
  glGenTextures(1, &tbo);
  glBindTexture(GL_TEXTURE_CUBE_MAP, tbo);

  // parameter setting
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  // read images into cubemap
  vector<string> texImages;
  texImages.push_back("./image/right.png");
  texImages.push_back("./image/left.png");
  texImages.push_back("./image/bottom.png");
  texImages.push_back("./image/top.png");
  texImages.push_back("./image/back.png");
  texImages.push_back("./image/front.png");

  for (GLuint i = 0; i < texImages.size(); i++) {
    int width, height;
    FIBITMAP *image;

    image = FreeImage_ConvertTo24Bits(
        FreeImage_Load(FIF_PNG, texImages[i].c_str()));

    width = FreeImage_GetWidth(image);
    height = FreeImage_GetHeight(image);

    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height,
                 0, GL_BGR, GL_UNSIGNED_BYTE, (void *)FreeImage_GetBits(image));

    FreeImage_Unload(image);
  }
}

void Skybox::initBuffer() {
  // vbo
  // if put these code before setting texture,
  // no skybox will be rendered
  // don't know why
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 6 * 3, vtxs,
               GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(0);
}

void Skybox::initShader() {
  shader = buildShader("./shader/vsSkybox.glsl", "./shader/fsSkybox.glsl");
}

void Skybox::initUniform() {
  glUseProgram(shader);

  uniM = myGetUniformLocation(shader, "M");
  uniV = myGetUniformLocation(shader, "V");
  uniP = myGetUniformLocation(shader, "P");

  glUniformMatrix4fv(uniM, 1, GL_FALSE, value_ptr(M));
  glUniformMatrix4fv(uniV, 1, GL_FALSE, value_ptr(V));
  glUniformMatrix4fv(uniP, 1, GL_FALSE, value_ptr(P));
}
