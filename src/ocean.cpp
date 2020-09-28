#include "ocean.h"

const float cOcean::BASELINE = 0.f;

cOcean::cOcean(const int N, const float A, const vec2 w, const float length)
    : g(9.81), N(N), Nplus1(N + 1), A(A), w(w), length(length), vertices(0),
      h_tilde(0), h_tilde_slopex(0), h_tilde_slopez(0), h_tilde_dx(0),
      h_tilde_dz(0), fft(0) {
  h_tilde = new Complex[N * N];
  h_tilde_slopex = new Complex[N * N];
  h_tilde_slopez = new Complex[N * N];
  h_tilde_dx = new Complex[N * N];
  h_tilde_dz = new Complex[N * N];
  fft = new cFFT(N);
  vertices = new vertex_ocean[Nplus1 * Nplus1];

  // heightMap = FreeImage_Allocate(N * 10, N * 10, 24);

  int index;
  Complex htilde0, htilde0mk_conj;

  for (int m_prime = 0; m_prime < Nplus1; m_prime++) {
    for (int n_prime = 0; n_prime < Nplus1; n_prime++) {
      index = m_prime * Nplus1 + n_prime;

      htilde0 = hTilde_0(n_prime, m_prime);
      htilde0mk_conj = conj(hTilde_0(-n_prime, -m_prime));

      vertices[index].a = htilde0.real();
      vertices[index].b = htilde0.imag();
      vertices[index]._a = htilde0mk_conj.real();
      vertices[index]._b = htilde0mk_conj.imag();

      vertices[index].ox = vertices[index].x =
          (n_prime - N / 2.0f) * length / N;
      vertices[index].oy = vertices[index].y = 0.0f;
      vertices[index].oz = vertices[index].z =
          (m_prime - N / 2.0f) * length / N;

      vertices[index].nx = 0.0f;
      vertices[index].ny = 1.0f;
      vertices[index].nz = 0.0f;
    }
  }

  // import water mesh
  scene = importer.ReadFile("./mesh/gridQuad.obj", aiProcess_CalcTangentSpace);

  initShader();
  initBuffers();
  initTexture();
  initUniform();
  initReflect();
  initRefract();

  FIBITMAP *heightMap = FreeImage_Allocate(N, N, 24);
}

cOcean::~cOcean() {
  if (h_tilde)
    delete[] h_tilde;
  if (h_tilde_slopex)
    delete[] h_tilde_slopex;
  if (h_tilde_slopez)
    delete[] h_tilde_slopez;
  if (h_tilde_dx)
    delete[] h_tilde_dx;
  if (h_tilde_dz)
    delete[] h_tilde_dz;
  if (fft)
    delete fft;
  if (vertices)
    delete[] vertices;
}

void cOcean::initShader() {
  shader = buildShader("./shader/vsOcean.glsl", "./shader/fsOcean.glsl",
                       "./shader/tcsQuad.glsl", "./shader/tesQuad.glsl");
}

void cOcean::initBuffers() {
  // for each mesh
  for (size_t i = 0; i < scene->mNumMeshes; i++) {
    const aiMesh *mesh = scene->mMeshes[i];
    int numVtxs = mesh->mNumVertices;

    // numVertices * numComponents
    GLfloat *aVtxCoords = new GLfloat[numVtxs * 3];
    GLfloat *aUvs = new GLfloat[numVtxs * 2];
    GLfloat *aNormals = new GLfloat[numVtxs * 3];

    for (size_t j = 0; j < numVtxs; j++) {
      aiVector3D &vtx = mesh->mVertices[j];
      aVtxCoords[j * 3 + 0] = vtx.x;
      aVtxCoords[j * 3 + 1] = vtx.y;
      aVtxCoords[j * 3 + 2] = vtx.z;

      aiVector3D &nml = mesh->mNormals[j];
      aNormals[j * 3 + 0] = nml.x;
      aNormals[j * 3 + 1] = nml.y;
      aNormals[j * 3 + 2] = nml.z;

      aiVector3D &uv = mesh->mTextureCoords[0][j];
      aUvs[j * 2 + 0] = uv.x;
      aUvs[j * 2 + 1] = uv.y;
    }

    // vao
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    vaos.push_back(vao);

    // vbo for vertex
    GLuint vboVtx;
    glGenBuffers(1, &vboVtx);
    glBindBuffer(GL_ARRAY_BUFFER, vboVtx);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * numVtxs * 3, aVtxCoords,
                 GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    vboVtxs.push_back(vboVtx);

    // vbo for uv
    GLuint vboUv;
    glGenBuffers(1, &vboUv);
    glBindBuffer(GL_ARRAY_BUFFER, vboUv);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * numVtxs * 2, aUvs,
                 GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);
    vboUvs.push_back(vboUv);

    // vbo for normal
    GLuint vboNml;
    glGenBuffers(1, &vboNml);
    glBindBuffer(GL_ARRAY_BUFFER, vboNml);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * numVtxs * 3, aNormals,
                 GL_STATIC_DRAW);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(2);
    vboNmls.push_back(vboNml);

    // delete client data
    delete[] aVtxCoords;
    delete[] aUvs;
    delete[] aNormals;
  } // end for each mesh
}

void cOcean::initTexture() {
  setTexture(tboHeight, 11, "./image/height.png", FIF_PNG);
  setTexture(tboNormal, 12, "./image/normal.png", FIF_PNG);
  setTexture(tboDispX, 13, "./image/xDisp.png", FIF_PNG);
  setTexture(tboDispZ, 14, "./image/zDisp.png", FIF_PNG);
  setTexture(tboFresnel, 15, "./image/fresnel.png", FIF_PNG);
}

void cOcean::initUniform() {
  glUseProgram(shader);

  // transform
  uniM = myGetUniformLocation(shader, "M");
  uniV = myGetUniformLocation(shader, "V");
  uniP = myGetUniformLocation(shader, "P");

  // texture
  uniTexReflect = myGetUniformLocation(shader, "texReflect");
  uniTexRefract = myGetUniformLocation(shader, "texRefract");
  uniTexHeight = myGetUniformLocation(shader, "texHeight");
  uniTexNormal = myGetUniformLocation(shader, "texNormal");
  uniTexDispX = myGetUniformLocation(shader, "texDispX");
  uniTexDispZ = myGetUniformLocation(shader, "texDispZ");
  uniTexSkybox = myGetUniformLocation(shader, "texSkybox");
  uniTexFresnel = myGetUniformLocation(shader, "texFresnel");

  glUniform1i(uniTexHeight, 11);
  glUniform1i(uniTexNormal, 12);
  glUniform1i(uniTexDispX, 13);
  glUniform1i(uniTexDispZ, 14);
  glUniform1i(uniTexFresnel, 15);
  glUniform1i(uniTexReflect, 3);
  glUniform1i(uniTexRefract, 2);

  // light
  uniLightColor = myGetUniformLocation(shader, "lightColor");
  uniLightPos = myGetUniformLocation(shader, "lightPos");

  // other
  uniEyePoint = myGetUniformLocation(shader, "eyePoint");
}

void cOcean::setTexture(GLuint &tbo, int texUnit, const string texDir,
                        FREE_IMAGE_FORMAT imgType) {
  glActiveTexture(GL_TEXTURE0 + texUnit);

  FIBITMAP *texImage =
      FreeImage_ConvertTo24Bits(FreeImage_Load(imgType, texDir.c_str()));

  glGenTextures(1, &tbo);
  glBindTexture(GL_TEXTURE_2D, tbo);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, FreeImage_GetWidth(texImage),
               FreeImage_GetHeight(texImage), 0, GL_BGR, GL_UNSIGNED_BYTE,
               (void *)FreeImage_GetBits(texImage));
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  // release
  FreeImage_Unload(texImage);
}

float cOcean::dispersion(int n_prime, int m_prime) {
  float w_0 = 2.0f * M_PI / 200.0f;
  float kx = M_PI * (2 * n_prime - N) / length;
  float kz = M_PI * (2 * m_prime - N) / length;
  return floor(sqrt(g * sqrt(kx * kx + kz * kz)) / w_0) * w_0;
}

float cOcean::phillips(int n_prime, int m_prime) {
  vec2 k(M_PI * (2 * n_prime - N) / length, M_PI * (2 * m_prime - N) / length);
  float k_length = glm::length(k);
  if (k_length < 0.000001)
    return 0.0;

  float k_length2 = k_length * k_length;
  float k_length4 = k_length2 * k_length2;

  float k_dot_w = dot(normalize(k), normalize(w));
  float k_dot_w2 = k_dot_w * k_dot_w;

  float w_length = glm::length(w);
  float L = w_length * w_length / g;
  float L2 = L * L;

  float damping = 0.001;
  float l2 = L2 * damping * damping;

  return A * exp(-1.0f / (k_length2 * L2)) / k_length4 * k_dot_w2 *
         exp(-k_length2 * l2);
}

Complex cOcean::hTilde_0(int n_prime, int m_prime) {
  Complex r = gaussianRandomVariable();
  return r * sqrt(phillips(n_prime, m_prime) / 2.0);
}

Complex cOcean::hTilde(float t, int n_prime, int m_prime) {
  int index = m_prime * Nplus1 + n_prime;

  Complex htilde0(vertices[index].a, vertices[index].b);
  Complex htilde0mkconj(vertices[index]._a, vertices[index]._b);

  float omegat = dispersion(n_prime, m_prime) * t;

  float cos_ = cos(omegat);
  float sin_ = sin(omegat);

  Complex c0(cos_, sin_);
  Complex c1(cos_, -sin_);

  Complex res = htilde0 * c0 + htilde0mkconj * c1;

  return htilde0 * c0 + htilde0mkconj * c1;
}

void cOcean::evaluateWavesFFT(float t) {
  float kx, kz, len, lambda = -1.0f;
  int index, index1;

  for (int m_prime = 0; m_prime < N; m_prime++) {
    kz = M_PI * (2.0f * m_prime - N) / length;
    for (int n_prime = 0; n_prime < N; n_prime++) {
      kx = M_PI * (2 * n_prime - N) / length;
      len = sqrt(kx * kx + kz * kz);
      index = m_prime * N + n_prime;

      h_tilde[index] = hTilde(t, n_prime, m_prime);

      h_tilde_slopex[index] = h_tilde[index] * Complex(0, kx);
      h_tilde_slopez[index] = h_tilde[index] * Complex(0, kz);
      if (len < 0.000001f) {
        h_tilde_dx[index] = Complex(0.0f, 0.0f);
        h_tilde_dz[index] = Complex(0.0f, 0.0f);
      } else {
        h_tilde_dx[index] = h_tilde[index] * Complex(0, -kx / len);
        h_tilde_dz[index] = h_tilde[index] * Complex(0, -kz / len);
      }
    }
  }

  for (int m_prime = 0; m_prime < N; m_prime++) {
    fft->fft(h_tilde, h_tilde, 1, m_prime * N);
    fft->fft(h_tilde_slopex, h_tilde_slopex, 1, m_prime * N);
    fft->fft(h_tilde_slopez, h_tilde_slopez, 1, m_prime * N);
    fft->fft(h_tilde_dx, h_tilde_dx, 1, m_prime * N);
    fft->fft(h_tilde_dz, h_tilde_dz, 1, m_prime * N);
  }
  for (int n_prime = 0; n_prime < N; n_prime++) {
    fft->fft(h_tilde, h_tilde, N, n_prime);
    fft->fft(h_tilde_slopex, h_tilde_slopex, N, n_prime);
    fft->fft(h_tilde_slopez, h_tilde_slopez, N, n_prime);
    fft->fft(h_tilde_dx, h_tilde_dx, N, n_prime);
    fft->fft(h_tilde_dz, h_tilde_dz, N, n_prime);
  }

  int sign;
  float signs[] = {1.0f, -1.0f};
  vec3 n;
  for (int m_prime = 0; m_prime < N; m_prime++) {
    for (int n_prime = 0; n_prime < N; n_prime++) {
      index = m_prime * N + n_prime;       // index into h_tilde..
      index1 = m_prime * Nplus1 + n_prime; // index into vertices

      sign = signs[(n_prime + m_prime) & 1];

      h_tilde[index] = h_tilde[index] * double(sign);

      // height
      vertices[index1].y = h_tilde[index].real();

      // displacement
      h_tilde_dx[index] = h_tilde_dx[index] * double(sign);
      h_tilde_dz[index] = h_tilde_dz[index] * double(sign);
      vertices[index1].x =
          vertices[index1].ox + h_tilde_dx[index].real() * lambda;
      vertices[index1].z =
          vertices[index1].oz + h_tilde_dz[index].real() * lambda;

      // normal
      h_tilde_slopex[index] = h_tilde_slopex[index] * double(sign);
      h_tilde_slopez[index] = h_tilde_slopez[index] * double(sign);
      n = normalize(vec3(0.0f - h_tilde_slopex[index].real(), 1.0f,
                         0.0f - h_tilde_slopez[index].real()));
      vertices[index1].nx = n.x;
      vertices[index1].ny = n.y;
      vertices[index1].nz = n.z;

      // for tiling
      if (n_prime == 0 && m_prime == 0) {
        vertices[index1 + N + Nplus1 * N].y = h_tilde[index].real();

        vertices[index1 + N + Nplus1 * N].x =
            vertices[index1 + N + Nplus1 * N].ox +
            h_tilde_dx[index].real() * lambda;
        vertices[index1 + N + Nplus1 * N].z =
            vertices[index1 + N + Nplus1 * N].oz +
            h_tilde_dz[index].real() * lambda;

        vertices[index1 + N + Nplus1 * N].nx = n.x;
        vertices[index1 + N + Nplus1 * N].ny = n.y;
        vertices[index1 + N + Nplus1 * N].nz = n.z;
      }
      if (n_prime == 0) {
        vertices[index1 + N].y = h_tilde[index].real();

        vertices[index1 + N].x =
            vertices[index1 + N].ox + h_tilde_dx[index].real() * lambda;
        vertices[index1 + N].z =
            vertices[index1 + N].oz + h_tilde_dz[index].real() * lambda;

        vertices[index1 + N].nx = n.x;
        vertices[index1 + N].ny = n.y;
        vertices[index1 + N].nz = n.z;
      }
      if (m_prime == 0) {
        vertices[index1 + Nplus1 * N].y = h_tilde[index].real();

        vertices[index1 + Nplus1 * N].x = vertices[index1 + Nplus1 * N].ox +
                                          h_tilde_dx[index].real() * lambda;
        vertices[index1 + Nplus1 * N].z = vertices[index1 + Nplus1 * N].oz +
                                          h_tilde_dz[index].real() * lambda;

        vertices[index1 + Nplus1 * N].nx = n.x;
        vertices[index1 + Nplus1 * N].ny = n.y;
        vertices[index1 + Nplus1 * N].nz = n.z;
      }
    }
  }
}

void cOcean::render(float t, mat4 M, mat4 V, mat4 P, vec3 eyePoint,
                    vec3 lightColor, vec3 lightPos, bool resume, int frameN) {
  if (resume) {
    evaluateWavesFFT(t);
  }

  // write maps
  writeHeightMap();
  writeNormalMap();

  // update maps
  setTexture(tboHeight, 11, "./image/height.png", FIF_PNG);
  setTexture(tboNormal, 12, "./image/normal.png", FIF_PNG);
  setTexture(tboDispX, 13, "./image/xDisp.png", FIF_PNG);
  setTexture(tboDispZ, 14, "./image/zDisp.png", FIF_PNG);

  // update transform matrix
  glUseProgram(shader);

  glUniformMatrix4fv(uniM, 1, GL_FALSE, value_ptr(M));
  glUniformMatrix4fv(uniV, 1, GL_FALSE, value_ptr(V));
  glUniformMatrix4fv(uniP, 1, GL_FALSE, value_ptr(P));

  glUniform3fv(uniEyePoint, 1, value_ptr(eyePoint));
  glUniform3fv(uniLightColor, 1, value_ptr(lightColor));
  glUniform3fv(uniLightPos, 1, value_ptr(lightPos));

  // duplicated
  for (int j = 0; j < 1; j++) {
    for (int i = 0; i < 1; i++) {
      // modify the scale matrix to obtain different visual effect
      // to obtain a stromy ocean,
      // decrease the difference between scaleY and (scaleX, scaleZ)
      // e.g. vec3(10.f, 10.f, 10.f)
      // on the other hand, (10.f, 2.5f, 10.f) gives a relatively calm ocean
      mat4 Model = translate(mat4(1.0f), vec3(2.0 * i, 0, -2.0 * j));
      glUniformMatrix4fv(uniM, 1, GL_FALSE, value_ptr(Model));

      for (size_t i = 0; i < scene->mNumMeshes; i++) {
        int numVtxs = scene->mMeshes[i]->mNumVertices;

        glBindVertexArray(vaos[i]);
        glDrawArrays(GL_PATCHES, 0, numVtxs);
      }
    } // end for i
  }   // end for j
}

vec3 cOcean::getVertex(int ix, int iz) {
  // there are Nplus1 * Nplus1 vertices
  int idx = iz * Nplus1 + ix;

  vec3 vtx(vertices[idx].x, vertices[idx].y, vertices[idx].z);

  return vtx;
}

void cOcean::writeHeightMap() {
  int w, h;
  w = N;
  h = w;

  FIBITMAP *bitmapY = FreeImage_Allocate(w, h, 24);
  RGBQUAD colorY;

  FIBITMAP *bitmapX = FreeImage_Allocate(w, h, 24);
  RGBQUAD colorX;

  FIBITMAP *bitmapZ = FreeImage_Allocate(w, h, 24);
  RGBQUAD colorZ;

  if (!bitmapY) {
    std::cout << "FreeImage: Cannot allocate bitmapY." << '\n';
    exit(EXIT_FAILURE);
  }
  if (!bitmapX) {
    std::cout << "FreeImage: Cannot allocate bitmapX." << '\n';
    exit(EXIT_FAILURE);
  }
  if (!bitmapZ) {
    std::cout << "FreeImage: Cannot allocate bitmapZ." << '\n';
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < w; i++) {
    for (int j = 0; j < h; j++) {
      int idx = i * N + j;

      // height
      float scaleY = 2.0;
      float y = vertices[idx].oy - vertices[idx].y;
      int signY = (y < 0) ? 0 : 255;
      int iy = int(abs(y) / scaleY * 255.0);

      colorY.rgbRed = iy;           // value
      colorY.rgbGreen = signY;      // sign
      colorY.rgbBlue = int(scaleY); // scale
      FreeImage_SetPixelColor(bitmapY, i, j, &colorY);

      // x-displacement
      float scaleX = 2.0;
      float x = vertices[idx].ox - vertices[idx].x;
      int signX = (x < 0) ? 0 : 255;
      int ix = int(abs(x) / scaleX * 255.0);

      colorX.rgbRed = ix;           // value
      colorX.rgbGreen = signX;      // sign
      colorX.rgbBlue = int(scaleX); // scale
      FreeImage_SetPixelColor(bitmapX, i, j, &colorX);

      // z-displacement
      float scaleZ = 2.0;
      float z = vertices[idx].oz - vertices[idx].z;
      int signZ = (z < 0) ? 0 : 255;
      int iz = int(abs(z) / scaleZ * 255.0);

      colorZ.rgbRed = iz;           // value
      colorZ.rgbGreen = signZ;      // sign
      colorZ.rgbBlue = int(scaleZ); // scale
      FreeImage_SetPixelColor(bitmapZ, i, j, &colorZ);
    }
  }

  FreeImage_Save(FIF_PNG, bitmapY, "./image/height.png", 0);
  FreeImage_Save(FIF_PNG, bitmapX, "./image/xDisp.png", 0);
  FreeImage_Save(FIF_PNG, bitmapZ, "./image/zDisp.png", 0);
}

void cOcean::writeNormalMap() {
  int w, h;
  w = N;
  h = w;

  FIBITMAP *bitmap = FreeImage_Allocate(w, h, 24);
  RGBQUAD color;

  if (!bitmap) {
    std::cout << "FreeImage: Cannot allocate image." << '\n';
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < w; i++) {
    for (int j = 0; j < h; j++) {
      int idx = i * N + j;

      vec3 normal(vertices[idx].nx, vertices[idx].ny, vertices[idx].nz);
      normal = normalize(normal);      // to [-1, 1]
      normal = (normal + 1.0f) / 2.0f; // to [0, 1]

      // to [0, 255]
      int ix = int(normal.x * 255.0);
      int iy = int(normal.y * 255.0);
      int iz = int(normal.z * 255.0);

      color.rgbRed = ix;
      color.rgbGreen = iy;
      color.rgbBlue = iz;

      FreeImage_SetPixelColor(bitmap, i, j, &color);
    }
  }

  FreeImage_Save(FIF_PNG, bitmap, "./image/normal.png", 0);
}

void cOcean::initReflect() {
  // framebuffer object
  glGenFramebuffers(1, &fboReflect);
  glBindFramebuffer(GL_FRAMEBUFFER, fboReflect);

  glActiveTexture(GL_TEXTURE0 + 3);
  glGenTextures(1, &tboReflect);
  glBindTexture(GL_TEXTURE_2D, tboReflect);

  // On OSX, must use WINDOW_WIDTH * 2 and WINDOW_HEIGHT * 2, don't know why
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WINDOW_WIDTH * 2, WINDOW_HEIGHT * 2, 0,
               GL_RGB, GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, tboReflect, 0);

  // The depth buffer
  // User-defined framebuffer must have a depth buffer to enable depth test
  glGenRenderbuffers(1, &rboDepthReflect);
  glBindRenderbuffer(GL_RENDERBUFFER, rboDepthReflect);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, WINDOW_WIDTH * 2,
                        WINDOW_HEIGHT * 2);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, rboDepthReflect);

  glDrawBuffer(GL_COLOR_ATTACHMENT2);
}

void cOcean::initRefract() {
  // framebuffer object
  glGenFramebuffers(1, &fboRefract);
  glBindFramebuffer(GL_FRAMEBUFFER, fboRefract);

  glActiveTexture(GL_TEXTURE0 + 2);
  glGenTextures(1, &tboRefract);
  glBindTexture(GL_TEXTURE_2D, tboRefract);

  // On OSX, must use WINDOW_WIDTH * 2 and WINDOW_HEIGHT * 2, don't know why
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WINDOW_WIDTH * 2, WINDOW_HEIGHT * 2, 0,
               GL_RGB, GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, tboRefract, 0);

  // The depth buffer
  // User-defined framebuffer must have a depth buffer to enable depth test
  glGenRenderbuffers(1, &rboDepthRefract);
  glBindRenderbuffer(GL_RENDERBUFFER, rboDepthRefract);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, WINDOW_WIDTH * 2,
                        WINDOW_HEIGHT * 2);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, rboDepthRefract);

  glDrawBuffer(GL_COLOR_ATTACHMENT1);
}

float uniformRandomVariable() {
  // [0, 1]
  float f = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

  return f;
}

Complex gaussianRandomVariable() {
  float x1, x2, w;
  do {
    x1 = 2.f * uniformRandomVariable() - 1.f;
    x2 = 2.f * uniformRandomVariable() - 1.f;
    w = x1 * x1 + x2 * x2;
  } while (w >= 1.f);
  w = sqrt((-2.f * log(w)) / w);
  return Complex(x1 * w, x2 * w);
}
