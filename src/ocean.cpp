#include "ocean.h"

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

  origin = vec3(vertices[0].x, vertices[0].y, vertices[0].z);
  // border = origin + vec3(length);
  cellSize = length / float(N);

  // 2 triangles per quad
  // 3 vertices per triangle
  // 3 GLfloat per vertex
  nOfQuads = N * N;
  aWaterVtxs = new GLfloat[nOfQuads * 2 * 3 * 3];
  aWaterNs = new GLfloat[nOfQuads * 2 * 3 * 3];

  computeWaterGeometry();

  // buffer object
  // vao
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  // vbo for vertex position
  glGenBuffers(1, &vboVtxs);
  glBindBuffer(GL_ARRAY_BUFFER, vboVtxs);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * nOfQuads * 2 * 3 * 3,
               aWaterVtxs, GL_STREAM_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(0);

  // vbo for vertex normal
  glGenBuffers(1, &vboNs);
  glBindBuffer(GL_ARRAY_BUFFER, vboNs);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * nOfQuads * 2 * 3 * 3,
               aWaterNs, GL_STREAM_DRAW);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(1);

  shader = buildShader("./shader/vsOcean.glsl", "./shader/fsOcean.glsl");
  // shader = buildShader("./shader/vsPoint.glsl", "./shader/fsPoint.glsl");

  // light_position = myGetUniformLocation(shader, "light_position");
  projection = myGetUniformLocation(shader, "P");
  view = myGetUniformLocation(shader, "V");
  model = myGetUniformLocation(shader, "M");
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

void cOcean::release() {
  glDeleteBuffers(1, &vbo_indices);
  glDeleteBuffers(1, &vboVtxs);
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

void cOcean::render(float t, glm::vec3 light_pos, glm::mat4 Projection,
                    glm::mat4 View, glm::mat4 Model, bool resume, int frameN) {
  if (resume) {
    evaluateWavesFFT(t);
  }

  computeWaterGeometry();
  updateWaterGeometry();

  // update transform matrix
  glUseProgram(shader);
  glUniform3f(light_position, light_pos.x, light_pos.y, light_pos.z);
  glUniformMatrix4fv(projection, 1, GL_FALSE, glm::value_ptr(Projection));
  glUniformMatrix4fv(view, 1, GL_FALSE, glm::value_ptr(View));
  glUniformMatrix4fv(model, 1, GL_FALSE, glm::value_ptr(Model));

  glBindVertexArray(vao);

  // duplicated
  for (int j = 0; j < 5; j++) {
    for (int i = 0; i < 5; i++) {
      // modify the scale matrix to obtain different visual effect
      // to obtain a stromy ocean,
      // decrease the difference between scaleY and (scaleX, scaleZ)
      // e.g. vec3(10.f, 10.f, 10.f)
      // on the other hand, (10.f, 2.5f, 10.f) gives a relatively calm ocean
      Model = glm::scale(glm::mat4(1.0f), glm::vec3(10.f, 2.5f, 10.f));
      Model = glm::translate(Model, glm::vec3(length * i, 0, length * -j));
      glUniformMatrix4fv(model, 1, GL_FALSE, glm::value_ptr(Model));

      glDrawArrays(GL_TRIANGLES, 0, nOfQuads * 2 * 3);
    }
  }

  // single
  // glBindVertexArray(vao);
  // glDrawArrays(GL_TRIANGLES, 0, nOfQuads * 2 * 3);

  // drawPoints();
}

void cOcean::computeWaterGeometry() {
  // geometry discription for opengl
  int idxQuad = 0;

  for (size_t i = 0; i < N; i++) {
    for (size_t j = 0; j < N; j++) {
      // note: there are Nplus1 vertices in a row and column
      // note: be careful, the order of indices
      // may result in a back-facing triangle
      int idx0 = i * Nplus1 + j;
      int idx1 = idx0 + 1;
      int idx2 = idx1 + Nplus1;
      int idx3 = idx2 - 1;

      // the first triangle in a quad
      // vertex position
      aWaterVtxs[idxQuad * 18 + 0] = vertices[idx2].x;
      aWaterVtxs[idxQuad * 18 + 1] = vertices[idx2].y;
      aWaterVtxs[idxQuad * 18 + 2] = vertices[idx2].z;

      aWaterVtxs[idxQuad * 18 + 3] = vertices[idx1].x;
      aWaterVtxs[idxQuad * 18 + 4] = vertices[idx1].y;
      aWaterVtxs[idxQuad * 18 + 5] = vertices[idx1].z;

      aWaterVtxs[idxQuad * 18 + 6] = vertices[idx0].x;
      aWaterVtxs[idxQuad * 18 + 7] = vertices[idx0].y;
      aWaterVtxs[idxQuad * 18 + 8] = vertices[idx0].z;

      // std::cout << "idx0 = " << idx0 << '\n';
      // std::cout << "triangle 1: " << '\n';
      // std::cout << vertices[idx0].x << ", " << vertices[idx0].y << ", "
      //           << vertices[idx0].z << '\n';
      // std::cout << vertices[idx1].x << ", " << vertices[idx1].y << ", "
      //           << vertices[idx1].z << '\n';
      // std::cout << vertices[idx2].x << ", " << vertices[idx2].y << ", "
      //           << vertices[idx2].z << '\n';

      // vertex normal
      aWaterNs[idxQuad * 18 + 0] = vertices[idx2].nx;
      aWaterNs[idxQuad * 18 + 1] = vertices[idx2].ny;
      aWaterNs[idxQuad * 18 + 2] = vertices[idx2].nz;

      aWaterNs[idxQuad * 18 + 3] = vertices[idx1].nx;
      aWaterNs[idxQuad * 18 + 4] = vertices[idx1].ny;
      aWaterNs[idxQuad * 18 + 5] = vertices[idx1].nz;

      aWaterNs[idxQuad * 18 + 6] = vertices[idx0].nx;
      aWaterNs[idxQuad * 18 + 7] = vertices[idx0].ny;
      aWaterNs[idxQuad * 18 + 8] = vertices[idx0].nz;

      // the second triangle in a quad
      // vertex position
      aWaterVtxs[idxQuad * 18 + 9] = vertices[idx3].x;
      aWaterVtxs[idxQuad * 18 + 10] = vertices[idx3].y;
      aWaterVtxs[idxQuad * 18 + 11] = vertices[idx3].z;

      aWaterVtxs[idxQuad * 18 + 12] = vertices[idx2].x;
      aWaterVtxs[idxQuad * 18 + 13] = vertices[idx2].y;
      aWaterVtxs[idxQuad * 18 + 14] = vertices[idx2].z;

      aWaterVtxs[idxQuad * 18 + 15] = vertices[idx0].x;
      aWaterVtxs[idxQuad * 18 + 16] = vertices[idx0].y;
      aWaterVtxs[idxQuad * 18 + 17] = vertices[idx0].z;

      // std::cout << "triangle 2: " << '\n';
      // std::cout << vertices[idx0].x << ", " << vertices[idx0].y << ", "
      //           << vertices[idx0].z << '\n';
      // std::cout << vertices[idx2].x << ", " << vertices[idx2].y << ", "
      //           << vertices[idx2].z << '\n';
      // std::cout << vertices[idx3].x << ", " << vertices[idx3].y << ", "
      //           << vertices[idx3].z << '\n';
      // std::cout << '\n';

      // vertex normal
      aWaterNs[idxQuad * 18 + 9] = vertices[idx3].nx;
      aWaterNs[idxQuad * 18 + 10] = vertices[idx3].ny;
      aWaterNs[idxQuad * 18 + 11] = vertices[idx3].nz;

      aWaterNs[idxQuad * 18 + 12] = vertices[idx2].nx;
      aWaterNs[idxQuad * 18 + 13] = vertices[idx2].ny;
      aWaterNs[idxQuad * 18 + 14] = vertices[idx2].nz;

      aWaterNs[idxQuad * 18 + 15] = vertices[idx0].nx;
      aWaterNs[idxQuad * 18 + 16] = vertices[idx0].ny;
      aWaterNs[idxQuad * 18 + 17] = vertices[idx0].nz;

      idxQuad++;
    }
  }
}

void cOcean::drawPoints() {
  // update data
  points.clear();
  for (size_t i = 0; i < Nplus1 * Nplus1; i++) {
    Point p;
    p.pos.x = vertices[i].x;
    p.pos.y = vertices[i].y;
    p.pos.z = vertices[i].z;
    points.push_back(p);
  }

  // array data
  int nOfPs = points.size();
  GLfloat *aPos = new GLfloat[nOfPs * 3];
  GLfloat *aColor = new GLfloat[nOfPs * 3];

  // implant data
  for (size_t i = 0; i < nOfPs; i++) {
    // positions
    Point &p = points[i];
    aPos[i * 3 + 0] = p.pos.x;
    aPos[i * 3 + 1] = p.pos.y;
    aPos[i * 3 + 2] = p.pos.z;

    // colors
    aColor[i * 3 + 0] = p.color.r;
    aColor[i * 3 + 1] = p.color.g;
    aColor[i * 3 + 2] = p.color.b;
  }

  // selete vao
  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  // position
  GLuint vboPos;
  glGenBuffers(1, &vboPos);
  glBindBuffer(GL_ARRAY_BUFFER, vboPos);
  glBufferData(GL_ARRAY_BUFFER, nOfPs * 3 * sizeof(GLfloat), aPos,
               GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(0);

  // color
  GLuint vboColor;
  glGenBuffers(1, &vboColor);
  glBindBuffer(GL_ARRAY_BUFFER, vboColor);
  glBufferData(GL_ARRAY_BUFFER, nOfPs * 3 * sizeof(GLfloat), aColor,
               GL_STREAM_DRAW);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(1);

  glDrawArrays(GL_POINTS, 0, nOfPs);

  // release
  delete[] aPos;
  delete[] aColor;
  glDeleteBuffers(1, &vboPos);
  glDeleteBuffers(1, &vboColor);
  glDeleteVertexArrays(1, &vao);
}

void cOcean::updateWaterGeometry() {
  glBindVertexArray(vao);

  // vbo for vertex position
  glBindBuffer(GL_ARRAY_BUFFER, vboVtxs);
  // buffer orphaning
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * nOfQuads * 2 * 3 * 3, NULL,
               GL_STREAM_DRAW);
  // write data
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * nOfQuads * 2 * 3 * 3,
               aWaterVtxs, GL_STREAM_DRAW);

  // vbo for vertex normal
  glBindBuffer(GL_ARRAY_BUFFER, vboNs);
  // buffer orphaning
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * nOfQuads * 2 * 3 * 3, NULL,
               GL_STREAM_DRAW);
  // write data
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * nOfQuads * 2 * 3 * 3,
               aWaterNs, GL_STREAM_DRAW);
}

vec3 cOcean::getVertex(int ix, int iz) {
  // there are Nplus1 * Nplus1 vertices
  int idx = iz * Nplus1 + ix;

  vec3 vtx(vertices[idx].x, vertices[idx].y, vertices[idx].z);

  return vtx;
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
