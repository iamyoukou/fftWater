#include "common.h"
#include "fft.h"

typedef vector<vector<vec3>> Array2D3v;
typedef vector<vector<float>> Array2D1f;
typedef vector<vector<vec4>> Array2D4v;

GLFWwindow *window;
unsigned int frameNumber = 0;

float verticalAngle = -1.95411;
float horizontalAngle = 3.14729;
float initialFoV = 45.0f;
float speed = 5.0f;
float mouseSpeed = 0.005f;
float farPlane = 2000.f;

vec3 eyePoint = vec3(-35.364811, 30.604717, 27.372036);
vec3 eyeDirection =
    vec3(sin(verticalAngle) * cos(horizontalAngle), cos(verticalAngle),
         sin(verticalAngle) * sin(horizontalAngle));
vec3 up = vec3(0.f, 1.f, 0.f);

vec3 lightPos = vec3(0.f, 10.f, 0.f);
vec3 lightColor = vec3(1.f, 1.f, 1.f);
float lightPower = 12.f;

vec3 materialDiffuse = vec3(0.f, 1.f, 0.f);
vec3 materialAmbient = vec3(0.f, 0.05f, 0.f);
vec3 materialSpecular = vec3(1.f, 1.f, 1.f);

const float SIZE = 500.f;
GLfloat vtxsSkybox[] = {
    // right
    SIZE, -SIZE, -SIZE, SIZE, -SIZE, SIZE, SIZE, SIZE, SIZE,
    //
    SIZE, SIZE, SIZE, SIZE, SIZE, -SIZE, SIZE, -SIZE, -SIZE,

    // left
    -SIZE, -SIZE, SIZE, -SIZE, -SIZE, -SIZE, -SIZE, SIZE, -SIZE,
    //
    -SIZE, SIZE, -SIZE, -SIZE, SIZE, SIZE, -SIZE, -SIZE, SIZE,

    // top
    -SIZE, SIZE, -SIZE, SIZE, SIZE, -SIZE, SIZE, SIZE, SIZE,
    //
    SIZE, SIZE, SIZE, -SIZE, SIZE, SIZE, -SIZE, SIZE, -SIZE,

    // bottom
    -SIZE, -SIZE, -SIZE, -SIZE, -SIZE, SIZE, SIZE, -SIZE, -SIZE,
    //
    SIZE, -SIZE, -SIZE, -SIZE, -SIZE, SIZE, SIZE, -SIZE, SIZE,

    // front
    -SIZE, -SIZE, SIZE, -SIZE, SIZE, SIZE, SIZE, SIZE, SIZE,
    //
    SIZE, SIZE, SIZE, SIZE, -SIZE, SIZE, -SIZE, -SIZE, SIZE,

    // back
    -SIZE, SIZE, -SIZE, -SIZE, -SIZE, -SIZE, SIZE, -SIZE, -SIZE,
    //
    SIZE, -SIZE, -SIZE, SIZE, SIZE, -SIZE, -SIZE, SIZE, -SIZE};

/* Water */
int N, M;
float cellSize;
float Lx, Lz;
vec3 wind = vec3(10.f, 0, 5.f);
float A = 0.0001f; // constant in Phillips Spectrum
float G = 9.8f;
float t = 0.f;
float dt = 0.01f;
vector<vec3> vWaterVtxsOri, vWaterVtxs, vWaterNs;
GLfloat *aWaterVtxs, *aWaterNs;
int nOfQuads;

Array2D3v waterPos;
Array2D3v waterN;
Array2D1f tableDisp;
Array2D4v tableH0;

GLuint vaoWater, vboWaterVtx, vboWaterN;

FFT fft;

// GLuint vboWaterPos, vboWaterN, vaoWater;
GLint uniWaterM, uniWaterV, uniWaterP;
GLint uniLightColor, uniLightPos, uniLightPower;
GLint uniDiffuse, uniAmbient, uniSpecular;
GLint vsWater, fsWater;
mat4 waterM, waterV, waterP;
GLuint shaderWater;

/* Other */
GLuint vboSkybox, tboSkybox, vaoSkybox;
GLint uniSkyboxM, uniSkyboxV, uniSkyboxP;
mat4 oriSkyboxM, skyboxM, skyboxV, skyboxP;
GLuint vsSkybox, fsSkybox;
GLuint shaderSkybox;

void computeMatricesFromInputs();
void keyCallback(GLFWwindow *, int, int, int, int);
float randf();
float phillipsSpectrum(int, int);
Complex freqForHeight(int, int);
float dispersion(int, int);

void initGL();
void initOther();
void initShader();
void initMatrix();
void initSkybox();
void initUniform();
void initWater();
void release();
void computeWaterGeometry();
void updateWaterGeometry();

void step();
vec2 gaussianRandom(float, float);
vec4 h0(int, int);

int main(int argc, char **argv) {
  initGL();
  initOther();
  initShader();
  initMatrix();
  initUniform();

  initSkybox();
  initWater();

  // for (size_t i = 0; i < waterPos.size(); i++) {
  //   for (size_t j = 0; j < waterPos[0].size(); j++) {
  //     std::cout << to_string(waterPos[i][j]) << ", ";
  //   }
  //   std::cout << '\n';
  // }

  // a rough way to solve cursor position initialization problem
  // must call glfwPollEvents once to activate glfwSetCursorPos
  // this is a glfw mechanism problem
  glfwPollEvents();
  glfwSetCursorPos(window, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);

  /* Loop until the user closes the window */
  while (!glfwWindowShouldClose(window)) {
    /* Render here */
    glClearColor(97 / 256.f, 175 / 256.f, 239 / 256.f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // update water
    step();

    // view control
    computeMatricesFromInputs();

    // draw skybox
    // glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glUseProgram(shaderSkybox);
    glBindVertexArray(vaoSkybox);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    // draw water
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glUseProgram(shaderWater);
    glBindVertexArray(vaoWater);
    glDrawArrays(GL_TRIANGLES, 0, nOfQuads * 2 * 3);

    // glUseProgram(shaderWater);
    // glBindVertexArray(vaoWater);
    // glDrawArrays(GL_POINTS, 0, N * N);

    // string dir = "./result/output";
    // // zero padding
    // // e.g. "output0001.bmp"
    // string num = to_string(frameNumber);
    // num = string(4 - num.length(), '0') + num;
    // string output = dir + num + ".bmp";
    //
    // FIBITMAP *outputImage =
    //     FreeImage_AllocateT(FIT_UINT32, WINDOW_WIDTH * 2, WINDOW_HEIGHT * 2);
    // glReadPixels(0, 0, WINDOW_WIDTH * 2, WINDOW_HEIGHT * 2, GL_BGRA,
    //              GL_UNSIGNED_INT_8_8_8_8_REV,
    //              (GLvoid *)FreeImage_GetBits(outputImage));
    // FreeImage_Save(FIF_BMP, outputImage, output.c_str(), 0);
    // std::cout << output << " saved." << '\n';
    // frameNumber++;

    // update frame
    glfwSwapBuffers(window);

    /* Poll for and process events */
    glfwPollEvents();
  }

  release();

  return EXIT_SUCCESS;
}

void computeMatricesFromInputs() {
  // glfwGetTime is called only once, the first time this function is called
  static float lastTime = glfwGetTime();

  // Compute time difference between current and last frame
  float currentTime = glfwGetTime();
  float deltaTime = float(currentTime - lastTime);

  // Get mouse position
  double xpos, ypos;
  glfwGetCursorPos(window, &xpos, &ypos);

  // Reset mouse position for next frame
  glfwSetCursorPos(window, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);

  // Compute new orientation
  // The cursor is set to the center of the screen last frame,
  // so (currentCursorPos - center) is the offset of this frame
  horizontalAngle += mouseSpeed * float(xpos - WINDOW_WIDTH / 2.f);
  verticalAngle += mouseSpeed * float(-ypos + WINDOW_HEIGHT / 2.f);

  // Direction : Spherical coordinates to Cartesian coordinates conversion
  vec3 direction =
      vec3(sin(verticalAngle) * cos(horizontalAngle), cos(verticalAngle),
           sin(verticalAngle) * sin(horizontalAngle));

  // Right vector
  vec3 right = vec3(cos(horizontalAngle - 3.14 / 2.f), 0.f,
                    sin(horizontalAngle - 3.14 / 2.f));

  // new up vector
  vec3 newUp = cross(right, direction);

  // Move forward
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
    eyePoint += direction * deltaTime * speed;
  }
  // Move backward
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    eyePoint -= direction * deltaTime * speed;
  }
  // Strafe right
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    eyePoint += right * deltaTime * speed;
  }
  // Strafe left
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    eyePoint -= right * deltaTime * speed;
  }

  // float FoV = initialFoV;
  mat4 newP = perspective(initialFoV, 1.f * WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f,
                          farPlane);
  // Camera matrix
  mat4 newV = lookAt(eyePoint, eyePoint + direction, newUp);

  // update for skybox
  glUseProgram(shaderSkybox);
  skyboxV = newV;
  skyboxP = newP;
  glUniformMatrix4fv(uniSkyboxV, 1, GL_FALSE, value_ptr(skyboxV));
  glUniformMatrix4fv(uniSkyboxP, 1, GL_FALSE, value_ptr(skyboxP));

  // make sure that the center of skybox is always at eyePoint
  // the GLM matrix is column major
  skyboxM[3][0] = oriSkyboxM[0][3] + eyePoint.x;
  skyboxM[3][1] = oriSkyboxM[1][3] + eyePoint.y;
  skyboxM[3][2] = oriSkyboxM[2][3] + eyePoint.z;
  glUniformMatrix4fv(uniSkyboxM, 1, GL_FALSE, value_ptr(skyboxM));

  // update for mesh
  glUseProgram(shaderWater);
  waterV = newV;
  waterP = newP;
  glUniformMatrix4fv(uniWaterV, 1, GL_FALSE, value_ptr(waterV));
  glUniformMatrix4fv(uniWaterP, 1, GL_FALSE, value_ptr(waterP));

  // For the next frame, the "last time" will be "now"
  lastTime = currentTime;
}

void keyCallback(GLFWwindow *keyWnd, int key, int scancode, int action,
                 int mods) {
  if (action == GLFW_PRESS) {
    switch (key) {
    case GLFW_KEY_ESCAPE: {
      glfwSetWindowShouldClose(keyWnd, GLFW_TRUE);
      break;
    }
    case GLFW_KEY_F: {
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      break;
    }
    case GLFW_KEY_L: {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      break;
    }
    case GLFW_KEY_I: {
      std::cout << "eyePoint: " << to_string(eyePoint) << '\n';
      std::cout << "verticleAngle: " << fmod(verticalAngle, 6.28f) << ", "
                << "horizontalAngle: " << fmod(horizontalAngle, 6.28f) << endl;
      break;
    }
    default:
      break;
    } // end switch
  }   // end if
}

void initGL() {
  // Initialise GLFW
  if (!glfwInit()) {
    fprintf(stderr, "Failed to initialize GLFW\n");
    getchar();
    exit(EXIT_FAILURE);
  }

  // without setting GLFW_CONTEXT_VERSION_MAJOR and _MINOR，
  // OpenGL 1.x will be used
  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

  // must be used if OpenGL version >= 3.0
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Open a window and create its OpenGL context
  window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
                            "GLFW window with AntTweakBar", NULL, NULL);

  if (window == NULL) {
    std::cout << "Failed to open GLFW window." << std::endl;
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwMakeContextCurrent(window);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwSetKeyCallback(window, keyCallback);

  /* Initialize GLEW */
  // without this, glGenVertexArrays will report ERROR!
  glewExperimental = GL_TRUE;

  if (glewInit() != GLEW_OK) {
    fprintf(stderr, "Failed to initialize GLEW\n");
    getchar();
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);

  // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  // glEnable(GL_PROGRAM_POINT_SIZE);
  // glPointSize(15);
}

void initOther() {
  FreeImage_Initialise(true); // FreeImage library
  srand(clock());             // initialize random seed
}

void initShader() {
  shaderSkybox =
      buildShader("./shader/vsSkybox.glsl", "./shader/fsSkybox.glsl");

  shaderWater = buildShader("./shader/vsWater.glsl", "./shader/fsWater.glsl");
  // shaderWater = buildShader("./shader/vsPoint.glsl",
  // "./shader/fsPoint.glsl");
}

void initMatrix() {
  // common
  mat4 M, V, P;

  M = translate(mat4(1.f), vec3(0.f, 0.f, -4.f));
  V = lookAt(eyePoint, eyePoint + eyeDirection, up);
  P = perspective(initialFoV, 1.f * WINDOW_WIDTH / WINDOW_HEIGHT, 0.01f,
                  farPlane);

  // mesh
  glUseProgram(shaderWater);

  waterM = M;
  waterV = V;
  waterP = P;

  // skybox
  glUseProgram(shaderSkybox);

  skyboxM = M;
  oriSkyboxM = skyboxM;
  skyboxV = V;
  skyboxP = P;
}

void initSkybox() {
  // texture
  glActiveTexture(GL_TEXTURE0);
  glGenTextures(1, &tboSkybox);
  glBindTexture(GL_TEXTURE_CUBE_MAP, tboSkybox);

  // necessary parameter setting
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  // read images into cubemap
  vector<string> texImages;
  texImages.push_back("./image/left.png");
  texImages.push_back("./image/right.png");
  texImages.push_back("./image/bottom.png");
  texImages.push_back("./image/top.png");
  texImages.push_back("./image/front.png");
  texImages.push_back("./image/back.png");

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

  // vbo
  // if put these code before setting texture,
  // no skybox will be rendered
  // don't know why
  glGenVertexArrays(1, &vaoSkybox);
  glBindVertexArray(vaoSkybox);

  glGenBuffers(1, &vboSkybox);
  glBindBuffer(GL_ARRAY_BUFFER, vboSkybox);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 6 * 3, vtxsSkybox,
               GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(0);
}

void initUniform() {
  /* Water */
  glUseProgram(shaderWater);

  uniWaterM = myGetUniformLocation(shaderWater, "M");
  uniWaterV = myGetUniformLocation(shaderWater, "V");
  uniWaterP = myGetUniformLocation(shaderWater, "P");

  glUniformMatrix4fv(uniWaterM, 1, GL_FALSE, value_ptr(waterM));
  glUniformMatrix4fv(uniWaterV, 1, GL_FALSE, value_ptr(waterV));
  glUniformMatrix4fv(uniWaterP, 1, GL_FALSE, value_ptr(waterP));

  // light
  // uniLightColor = myGetUniformLocation(shaderWater, "lightColor");
  // glUniform3fv(uniLightColor, 1, value_ptr(lightColor));

  uniLightPos = myGetUniformLocation(shaderWater, "lightPos");
  glUniform3fv(uniLightPos, 1, value_ptr(lightPos));

  // uniLightPower = myGetUniformLocation(shaderWater, "lightPower");
  // glUniform1f(uniLightPower, lightPower);

  // uniDiffuse = myGetUniformLocation(shaderWater, "diffuseColor");
  // glUniform3fv(uniDiffuse, 1, value_ptr(materialDiffuse));
  //
  // uniAmbient = myGetUniformLocation(shaderWater, "ambientColor");
  // glUniform3fv(uniAmbient, 1, value_ptr(materialAmbient));
  //
  // uniSpecular = myGetUniformLocation(shaderWater, "specularColor");
  // glUniform3fv(uniSpecular, 1, value_ptr(materialSpecular));

  /* Skybox */
  glUseProgram(shaderSkybox);

  uniSkyboxM = myGetUniformLocation(shaderSkybox, "M");
  uniSkyboxV = myGetUniformLocation(shaderSkybox, "V");
  uniSkyboxP = myGetUniformLocation(shaderSkybox, "P");

  glUniformMatrix4fv(uniSkyboxM, 1, GL_FALSE, value_ptr(skyboxM));
  glUniformMatrix4fv(uniSkyboxV, 1, GL_FALSE, value_ptr(skyboxV));
  glUniformMatrix4fv(uniSkyboxP, 1, GL_FALSE, value_ptr(skyboxP));
}

void initWater() {
  // initialize parameters
  // change these parameters if water.obj changes
  // N rows, M columns
  N = 64;
  M = N;
  cellSize = 1.f;
  Lx = cellSize * (N - 1);
  Lz = Lx;

  /* for water geometry description */
  nOfQuads = (N - 1) * (M - 1);

  // 2 triangles per quad
  aWaterVtxs = new GLfloat[nOfQuads * 2 * 3 * 3];
  aWaterNs = new GLfloat[nOfQuads * 2 * 3 * 3];

  // create water geometry position data
  for (size_t i = 0; i < N; i++) {
    for (size_t j = 0; j < M; j++) {
      vec3 vtx = vec3(i * cellSize, 0.f, j * cellSize);
      vec3 n = vec3(0, 1, 0);

      vWaterVtxsOri.push_back(vtx);
      vWaterVtxs.push_back(vtx);
      vWaterNs.push_back(n);
    }
  }

  // vector to array
  computeWaterGeometry();

  // buffer object
  // vao
  glGenVertexArrays(1, &vaoWater);
  glBindVertexArray(vaoWater);

  // vbo for vertex position
  glGenBuffers(1, &vboWaterVtx);
  glBindBuffer(GL_ARRAY_BUFFER, vboWaterVtx);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * nOfQuads * 2 * 3 * 3,
               aWaterVtxs, GL_STREAM_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(0);

  // vbo for vertex normal
  glGenBuffers(1, &vboWaterN);
  glBindBuffer(GL_ARRAY_BUFFER, vboWaterN);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * nOfQuads * 2 * 3 * 3,
               aWaterNs, GL_STREAM_DRAW);
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(2);

  /* FFT object */
  // N sampling points along each axis
  // compute the lookup table for complex exponent terms
  fft.N = N;
  fft.Lx = Lx;
  fft.computeWk();

  tableDisp = Array2D1f(N, vector<float>(M));
  tableH0 = Array2D4v(N, vector<vec4>(M));

  for (size_t n = 0; n < N; n++) {
    for (size_t m = 0; m < M; m++) {
      vec3 k(2.f * PI * n / Lx, 0.f, 2.f * PI * m / Lz);

      tableDisp[n][m] = dispersion(n, m);

      tableH0[n][m] = h0(n, m);
    }
  }
}

float randf() {
  // [0, 1]
  float f = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

  return f;
}

void step() {
  /* update geometry */
  // height
  CArray2D heightFreqs(CArray(M), N);

  // normal
  // CArray2D slopeFreqsX(CArray(M), N);
  // CArray2D slopeFreqsZ(CArray(M), N);

  // horizontal displacement
  CArray2D displaceFreqsX(CArray(M), N);
  CArray2D displaceFreqsZ(CArray(M), N);

  for (size_t n = 0; n < N; n++) {
    for (size_t m = 0; m < M; m++) {
      vec3 k(2.f * PI * n / Lx, 0, 2.f * PI * m / Lz);

      vec3 kNorm;
      if (length(k) < 0.0001f)
        kNorm = vec3(0, 0, 0);
      else
        kNorm = normalize(k);

      // std::cout << "k = " << to_string(k) << '\n';

      // if (testIdx == 0)
      // std::cout << "freqHeight = " << to_string(freqHeight) << '\n';

      // Complex ikX(0, k.x);
      // Complex ikZ(0, k.z);

      // if (testIdx == 0)
      // std::cout << "freqSlopeX = " << to_string(freqSlopeX) << '\n';

      // if (testIdx == 0)
      // std::cout << "freqSlopeZ = " << to_string(freqSlopeZ) << '\n';
      // std::cout << '\n';

      Complex ikNormX(0, -kNorm.x);
      Complex ikNormZ(0, -kNorm.z);

      // std::cout << "freqDisplaceX: " << to_string(freqDisplaceX) << '\n';
      // std::cout << "freqDisplaceZ: " << to_string(freqDisplaceZ) << '\n';

      heightFreqs[n][m] = freqForHeight(n, m);
      // slopeFreqsX[n][m] = ikX * heightFreqs[n][m];
      // slopeFreqsZ[n][m] = ikZ * heightFreqs[n][m];
      displaceFreqsX[n][m] = ikNormX * heightFreqs[n][m];
      displaceFreqsZ[n][m] = ikNormZ * heightFreqs[n][m];
    }
  }

  // perform IFFT
  fft.ifft2(heightFreqs);
  // fft.ifft2(slopeFreqsX);
  // fft.ifft2(slopeFreqsZ);
  fft.ifft2(displaceFreqsX);
  fft.ifft2(displaceFreqsZ);

  // update geometry
  // N rows, M columns
  for (size_t n = 0; n < N; n++) {
    for (size_t m = 0; m < M; m++) {
      // height
      int idx = n * N + m;
      vWaterVtxs[idx].y = heightFreqs[n][m].real();

      // normal
      // vec3 slope;
      // slope.x = slopeFreqsX[n][m].real();
      // slope.y = 0;
      // slope.z = slopeFreqsZ[n][m].real();

      // if (idx == 0)
      //   std::cout << "slope vector: " << to_string(slope) << '\n';

      // vec3 normal = vec3(0, 1, 0) - slope;
      // normal = glm::normalize(normal);

      // std::cout << to_string(normal) << '\n';

      // vWaterNs[idx] = normal;

      // std::cout << "use slope: " << to_string(normal) << '\n';

      // std::cout << "displaceFreqsX: " << displaceFreqsX[n][m].real() << '\n';
      // std::cout << "displaceFreqsZ: " << displaceFreqsZ[n][m].real() << '\n';

      // horizontal displacement
      float scale = 0.5f;
      vWaterVtxs[idx].x =
          vWaterVtxsOri[idx].x + displaceFreqsX[n][m].real() * scale;
      vWaterVtxs[idx].z =
          vWaterVtxsOri[idx].z + displaceFreqsZ[n][m].real() * scale;
    }
  }

  // std::cout << "use slope: " << to_string(vWaterNs[0]) << '\n';

  // update normal using two edges
  for (size_t n = 0; n < N; n++) {
    for (size_t m = 0; m < M; m++) {
      int idx0 = n * N + m;
      int idx1, idx2;

      // if not border
      if (m < M - 1 && n < N - 1) {
        idx1 = idx0 + 1;
        idx2 = idx0 + M;
      }
      // column border
      else if (m == M - 1 && n < N - 1) {
        idx1 = idx0 + M;
        idx2 = idx0 - 1;
      }
      // row border
      else if (n == N - 1 && m < M - 1) {
        idx1 = idx0 - M;
        idx2 = idx0 + 1;
      }
      // the corner one
      else {
        idx1 = idx0 - 1;
        idx2 = idx0 - M;
      }

      vec3 edge0 = vWaterVtxs[idx1] - vWaterVtxs[idx0];
      vec3 edge1 = vWaterVtxs[idx2] - vWaterVtxs[idx0];
      vec3 normal = glm::normalize(glm::cross(edge0, edge1));

      vWaterNs[idx0] = normal;

      // test
      // if (idx0 == 0)
      //   std::cout << "use two edges: " << to_string(normal) << '\n';
    }
  }

  t += dt;

  /* update buffer objects */
  computeWaterGeometry();
  updateWaterGeometry();
}

/* Gaussian random number generator */
// mean m, standard deviation s
vec2 gaussianRandom(float m, float s) {
  float x1, x2, w, y1;
  do {
    x1 = 2.0 * randf() - 1.0; // randf() is uniform in 0..1
    x2 = 2.0 * randf() - 1.0;
    w = x1 * x1 + x2 * x2;
  } while (w >= 1.0);

  w = sqrt((-2.0 * log(w)) / w);

  return vec2(x1 * w, x2 * w);
}

// (Eq.23) Given a wavevector k
float phillipsSpectrum(int n, int m) {
  vec3 k(2.f * PI * n / Lx, 0.f, 2.f * PI * m / Lz);

  float kLen = length(k);

  // when kLen == 0, kw will be (nan, nan)
  if (kLen < 0.00001f)
    return 0.f;

  float kLen2 = kLen * kLen;
  float kLen4 = kLen2 * kLen2;

  vec3 kDir = glm::normalize(k);

  float V = glm::length(wind);
  vec3 windDir = glm::normalize(wind);

  float L = V * V / G;
  float L2 = L * L;

  float damping = 0.001f;
  float l2 = L2 * damping * damping;

  float commonTerm = A * exp(-1.f / (kLen2 * L2));
  commonTerm /= (kLen4);
  commonTerm *= exp(-kLen2 * l2);

  float kw = dot(kDir, windDir);
  float kw2 = kw * kw;

  return (commonTerm * kw2);
}

// (Eq.25)
// compute h0(k) and {h0(-k)}*
// vec4.xy represent the real and imag part of h0(k)
// vec4.zw represent the real and imag part of {h0(-k)}*
vec4 h0(int n, int m) {
  vec3 k(2.f * PI * n / Lx, 0.f, 2.f * PI * m / Lz);

  vec2 gaus = gaussianRandom(0.f, 1.f);

  vec2 gausConj = gaussianRandom(0.f, 1.f);
  gausConj.y = -gausConj.y;

  float philSpec = phillipsSpectrum(n, m);
  float philSpecConj = phillipsSpectrum(-n, -m);
  // std::cout << "philSpec = " << to_string(philSpec) << '\n';

  vec2 h0k = gaus * sqrt(philSpec / 2.f);
  vec2 h0kConj = gausConj * sqrt(philSpecConj / 2.f);

  // std::cout << "h0k = " << to_string(h0k) << '\n';
  // std::cout << "h0kConj = " << to_string(h0kConj) << '\n';

  return vec4(h0k, h0kConj);
}

// frequency term in Eq.19, from Eq.26
// Note: the complex from Eq.25 has a format of (a + ib)
// so be careful to change the complex exponent term
// into (a + ib) before multiplication
// (a + bi)(c + di) = (ac - bd) + (ad + bc)i
Complex freqForHeight(int n, int m) {
  // std::cout << "k = " << to_string(k) << '\n';

  // float omega = dispersion(k);
  float omega = tableDisp[n][m];

  // from complex exponent to the (a + ib) format
  vec2 eTerm = vec2(cos(omega * t), sin(omega * t));
  vec2 eTermConj = vec2(eTerm.x, -eTerm.y);

  // vec4 h0Term = h0(k);
  vec4 h0Term = tableH0[n][m];

  // std::cout << "omega = " << omega << '\n';
  // std::cout << "eTerm = " << to_string(eTerm) << '\n';
  // std::cout << "eTermConj = " << to_string(eTermConj) << '\n';
  // std::cout << "h0Term = " << to_string(h0Term) << '\n';
  // std::cout << '\n';

  // the first part of Eq.26
  vec2 freq;
  freq.x = h0Term.x * eTerm.x - h0Term.y * eTerm.y;
  freq.y = h0Term.x * eTerm.y + h0Term.y * eTerm.x;

  // the second part of Eq.26
  vec2 freqConj;
  freqConj.x = h0Term.z * eTermConj.x - h0Term.w * eTermConj.y;
  freqConj.y = h0Term.z * eTermConj.y + h0Term.w * eTermConj.x;

  return Complex(freq.x + freqConj.x, freq.y + freqConj.y);
}

// (Eq.14) Dispersion relation
float dispersion(int n, int m) {
  vec3 k(2.f * PI * n / Lx, 0.f, 2.f * PI * m / Lz);

  float omega0 = 2.f * PI / 200.f;

  float omega = sqrt(length(k) * G);
  omega = floor(omega / omega0) * omega0;

  return omega;
}

void release() {
  glfwTerminate();
  FreeImage_DeInitialise();

  delete[] aWaterVtxs;
  delete[] aWaterNs;
}

void computeWaterGeometry() {

  // geometry discription for opengl
  int idxQuad = 0;

  for (size_t i = 0; i < N - 1; i++) {
    for (size_t j = 0; j < M - 1; j++) {
      int idx0 = i * M + j;
      int idx1 = idx0 + 1;
      int idx2 = idx1 + M;
      int idx3 = idx2 - 1;

      // the first triangle in a quad
      // vertex position
      aWaterVtxs[idxQuad * 18 + 0] = vWaterVtxs[idx0].x;
      aWaterVtxs[idxQuad * 18 + 1] = vWaterVtxs[idx0].y;
      aWaterVtxs[idxQuad * 18 + 2] = vWaterVtxs[idx0].z;

      aWaterVtxs[idxQuad * 18 + 3] = vWaterVtxs[idx1].x;
      aWaterVtxs[idxQuad * 18 + 4] = vWaterVtxs[idx1].y;
      aWaterVtxs[idxQuad * 18 + 5] = vWaterVtxs[idx1].z;

      aWaterVtxs[idxQuad * 18 + 6] = vWaterVtxs[idx2].x;
      aWaterVtxs[idxQuad * 18 + 7] = vWaterVtxs[idx2].y;
      aWaterVtxs[idxQuad * 18 + 8] = vWaterVtxs[idx2].z;

      // vertex normal
      aWaterNs[idxQuad * 18 + 0] = vWaterNs[idx0].x;
      aWaterNs[idxQuad * 18 + 1] = vWaterNs[idx0].y;
      aWaterNs[idxQuad * 18 + 2] = vWaterNs[idx0].z;

      aWaterNs[idxQuad * 18 + 3] = vWaterNs[idx1].x;
      aWaterNs[idxQuad * 18 + 4] = vWaterNs[idx1].y;
      aWaterNs[idxQuad * 18 + 5] = vWaterNs[idx1].z;

      aWaterNs[idxQuad * 18 + 6] = vWaterNs[idx2].x;
      aWaterNs[idxQuad * 18 + 7] = vWaterNs[idx2].y;
      aWaterNs[idxQuad * 18 + 8] = vWaterNs[idx2].z;

      // the second triangle in a quad
      // vertex position
      aWaterVtxs[idxQuad * 18 + 9] = vWaterVtxs[idx0].x;
      aWaterVtxs[idxQuad * 18 + 10] = vWaterVtxs[idx0].y;
      aWaterVtxs[idxQuad * 18 + 11] = vWaterVtxs[idx0].z;

      aWaterVtxs[idxQuad * 18 + 12] = vWaterVtxs[idx2].x;
      aWaterVtxs[idxQuad * 18 + 13] = vWaterVtxs[idx2].y;
      aWaterVtxs[idxQuad * 18 + 14] = vWaterVtxs[idx2].z;

      aWaterVtxs[idxQuad * 18 + 15] = vWaterVtxs[idx3].x;
      aWaterVtxs[idxQuad * 18 + 16] = vWaterVtxs[idx3].y;
      aWaterVtxs[idxQuad * 18 + 17] = vWaterVtxs[idx3].z;

      // vertex normal
      aWaterNs[idxQuad * 18 + 9] = vWaterNs[idx0].x;
      aWaterNs[idxQuad * 18 + 10] = vWaterNs[idx0].y;
      aWaterNs[idxQuad * 18 + 11] = vWaterNs[idx0].z;

      aWaterNs[idxQuad * 18 + 12] = vWaterNs[idx2].x;
      aWaterNs[idxQuad * 18 + 13] = vWaterNs[idx2].y;
      aWaterNs[idxQuad * 18 + 14] = vWaterNs[idx2].z;

      aWaterNs[idxQuad * 18 + 15] = vWaterNs[idx3].x;
      aWaterNs[idxQuad * 18 + 16] = vWaterNs[idx3].y;
      aWaterNs[idxQuad * 18 + 17] = vWaterNs[idx3].z;

      idxQuad++;
    }
  }
}

void updateWaterGeometry() {
  glBindVertexArray(vaoWater);

  // vbo for vertex position
  glBindBuffer(GL_ARRAY_BUFFER, vboWaterVtx);
  // buffer orphaning
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * nOfQuads * 2 * 3 * 3, NULL,
               GL_STREAM_DRAW);
  // write data
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * nOfQuads * 2 * 3 * 3,
               aWaterVtxs, GL_STREAM_DRAW);

  // vbo for vertex normal
  glBindBuffer(GL_ARRAY_BUFFER, vboWaterN);
  // buffer orphaning
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * nOfQuads * 2 * 3 * 3, NULL,
               GL_STREAM_DRAW);
  // write data
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * nOfQuads * 2 * 3 * 3,
               aWaterNs, GL_STREAM_DRAW);
}
