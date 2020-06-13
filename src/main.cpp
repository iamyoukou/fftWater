#include "common.h"
#include "fft.h"

typedef vector<vector<vec3>> Array2D3f;

GLFWwindow *window;

float verticalAngle = -2.15391;
float horizontalAngle = 1.5649;
float initialFoV = 45.0f;
float speed = 5.0f;
float mouseSpeed = 0.005f;
float farPlane = 2000.f;

vec3 eyePoint = vec3(0.115868, 12.963538, 17.811010);
vec3 eyeDirection =
    vec3(sin(verticalAngle) * cos(horizontalAngle), cos(verticalAngle),
         sin(verticalAngle) * sin(horizontalAngle));
vec3 up = vec3(0.f, 1.f, 0.f);

vec3 lightPos = vec3(3.f, 3.f, 3.f);
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
int N;
float cellSize;
float Lx, Lz;

Array2D3f waterPos;
Array2D3f waterN;

GLuint vboWaterPos, vboWaterN, vaoWater;
GLint uniWaterM, uniWaterV, uniWaterP;
GLint uniLightColor, uniLightPos, uniLightPower;
GLint uniDiffuse, uniAmbient, uniSpecular;
GLint vsWater, fsWater;
mat4 waterM, waterV, waterP;
GLuint shaderWater;

Mesh mesh;

/* Other */
GLuint vboSkybox, tboSkybox, vaoSkybox;
GLint uniSkyboxM, uniSkyboxV, uniSkyboxP;
mat4 oriSkyboxM, skyboxM, skyboxV, skyboxP;
GLuint vsSkybox, fsSkybox;
GLuint shaderSkybox;

void computeMatricesFromInputs();
void keyCallback(GLFWwindow *, int, int, int, int);
float randf();

void initGL();
void initOther();
void initShader();
void initMatrix();
void initSkybox();
void initMesh();
void initUniform();
void initWater();

void step();

int main(int argc, char **argv) {
  initGL();
  initOther();
  initShader();
  initMatrix();
  initUniform();

  initSkybox();
  // initMesh();
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
    glUseProgram(shaderSkybox);
    glBindVertexArray(vaoSkybox);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    // draw 3d models
    glUseProgram(shaderWater);
    glBindVertexArray(mesh.vao);
    glDrawArrays(GL_TRIANGLES, 0, mesh.faces.size() * 3);

    // glUseProgram(shaderWater);
    // glBindVertexArray(vaoWater);
    // glDrawArrays(GL_POINTS, 0, N * N);

    // update frame
    glfwSwapBuffers(window);

    /* Poll for and process events */
    glfwPollEvents();
  }

  glfwTerminate();

  // FreeImage library
  FreeImage_DeInitialise();

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
}

void initOther() {
  FreeImage_Initialise(true); // FreeImage library
  srand(clock());             // initialize random seed
}

void initShader() {
  shaderSkybox =
      buildShader("./shader/vsSkybox.glsl", "./shader/fsSkybox.glsl");

  shaderWater = buildShader("./shader/vsWater.glsl", "./shader/fsWater.glsl");
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
  mesh = loadObj("./mesh/water.obj");
  createMesh(mesh);

  // initialize parameters
  // change these parameters if water.obj changes
  N = 8;
  cellSize = 2.5f;
  Lx = 20.f;
  Lz = Lx;
}

float randf() {
  // [0, 1]
  float f = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

  return f;
}

void step() {
  /* update geometry */
  for (size_t i = 0; i < mesh.vertices.size(); i++) {
    vec3 &vtx = mesh.vertices[i];

    vtx.y += randf() * 0.01f;
  }

  /* update buffer objects */
  int nOfFaces = mesh.faces.size();

  glBindVertexArray(mesh.vao);

  // position
  glBindBuffer(GL_ARRAY_BUFFER, mesh.vboVtxs);
  // buffer orphaning
  glBufferData(GL_ARRAY_BUFFER, nOfFaces * 3 * 3 * sizeof(GLfloat), NULL,
               GL_STREAM_DRAW);

  for (size_t i = 0; i < nOfFaces; i++) {

    int vtxIdx = mesh.faces[i].v1;
    vec3 vtx = mesh.vertices[vtxIdx];
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(GLfloat) * (i * 9 + 0),
                    sizeof(GLfloat) * 3, &vtx);

    vtxIdx = mesh.faces[i].v2;
    vtx = mesh.vertices[vtxIdx];
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(GLfloat) * (i * 9 + 3),
                    sizeof(GLfloat) * 3, &vtx);

    vtxIdx = mesh.faces[i].v3;
    vtx = mesh.vertices[vtxIdx];
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(GLfloat) * (i * 9 + 6),
                    sizeof(GLfloat) * 3, &vtx);
  }
}
