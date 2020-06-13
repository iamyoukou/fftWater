#include "common.h"
#include "fft.h"

#include <cstdlib>
#include <ctime>

GLFWwindow *window;

float verticalAngle = -1.775f;
float horizontalAngle = 0.935f;
float initialFoV = 45.0f;
float speed = 5.0f;
float mouseSpeed = 0.005f;
float farPlane = 2000.f;

vec3 eyePoint = vec3(2.f, 1.2f, -0.8f);
vec3 eyeDirection =
    vec3(sin(verticalAngle) * cos(horizontalAngle), cos(verticalAngle),
         sin(verticalAngle) * sin(horizontalAngle));
vec3 up = vec3(0.f, 1.f, 0.f);

vec3 lightPosition = vec3(3.f, 3.f, 3.f);
vec3 lightColor = vec3(1.f, 1.f, 1.f);
float lightPower = 12.f;

vec3 materialDiffuse = vec3(0.f, 1.f, 0.f);
vec3 materialAmbient = vec3(0.f, 0.05f, 0.f);
vec3 materialSpecular = vec3(1.f, 1.f, 1.f);

Mesh mesh;

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

GLuint vboSkybox, tboSkybox, vaoSkybox;
GLint uniSkyboxM, uniSkyboxV, uniSkyboxP;
GLint uniMeshM, uniMeshV, uniMeshP;
GLint uniLightColor, uniLightPos, uniLightPower;
GLint uniDiffuse, uniAmbient, uniSpecular;
mat4 oriSkyboxM, skyboxM, skyboxV, skyboxP;
mat4 meshM, meshV, meshP;
GLuint vsSkybox, fsSkybox, vsModel, fsModel;
GLuint shaderSkybox, shaderMesh;

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

int main(int argc, char **argv) {
  initGL();
  initOther();
  initShader();
  initMatrix();
  initUniform();

  initSkybox();
  initMesh();

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

    // view control
    computeMatricesFromInputs();

    // draw skybox
    glUseProgram(shaderSkybox);
    glBindVertexArray(vaoSkybox);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    // draw 3d models
    glUseProgram(shaderMesh);
    glBindVertexArray(mesh.vao);
    glDrawArrays(GL_TRIANGLES, 0, mesh.faces.size() * 3);

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
  glUseProgram(shaderMesh);
  meshV = newV;
  meshP = newP;
  glUniformMatrix4fv(uniMeshV, 1, GL_FALSE, value_ptr(meshV));
  glUniformMatrix4fv(uniMeshP, 1, GL_FALSE, value_ptr(meshP));

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
}

void initOther() {
  FreeImage_Initialise(true); // FreeImage library
  srand(clock());             // initialize random seed
}

void initShader() {
  shaderSkybox =
      buildShader("./shader/vsSkybox.glsl", "./shader/fsSkybox.glsl");

  shaderMesh = buildShader("./shader/vsModel.glsl", "./shader/fsModel.glsl");
}

void initMatrix() {
  // common
  mat4 M, V, P;

  M = translate(mat4(1.f), vec3(0.f, 0.f, -4.f));
  V = lookAt(eyePoint, eyePoint + eyeDirection, up);
  P = perspective(initialFoV, 1.f * WINDOW_WIDTH / WINDOW_HEIGHT, 0.01f,
                  farPlane);

  // mesh
  glUseProgram(shaderMesh);

  meshM = M;
  meshV = V;
  meshP = P;

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

void initMesh() {
  mesh = loadObj("./mesh/cube.obj");
  createMesh(mesh);
}

void initUniform() {
  /* Mesh */
  glUseProgram(shaderMesh);

  uniMeshM = myGetUniformLocation(shaderMesh, "model");
  uniMeshV = myGetUniformLocation(shaderMesh, "view");
  uniMeshP = myGetUniformLocation(shaderMesh, "projection");

  glUniformMatrix4fv(uniMeshM, 1, GL_FALSE, value_ptr(meshM));
  glUniformMatrix4fv(uniMeshV, 1, GL_FALSE, value_ptr(meshV));
  glUniformMatrix4fv(uniMeshP, 1, GL_FALSE, value_ptr(meshP));

  // light
  uniLightColor = myGetUniformLocation(shaderMesh, "lightColor");
  glUniform3fv(uniLightColor, 1, value_ptr(lightColor));

  uniLightPos = myGetUniformLocation(shaderMesh, "lightPosition");
  glUniform3fv(uniLightPos, 1, value_ptr(lightPosition));

  uniLightPower = myGetUniformLocation(shaderMesh, "lightPower");
  glUniform1f(uniLightPower, lightPower);

  uniDiffuse = myGetUniformLocation(shaderMesh, "diffuseColor");
  glUniform3fv(uniDiffuse, 1, value_ptr(materialDiffuse));

  uniAmbient = myGetUniformLocation(shaderMesh, "ambientColor");
  glUniform3fv(uniAmbient, 1, value_ptr(materialAmbient));

  uniSpecular = myGetUniformLocation(shaderMesh, "specularColor");
  glUniform3fv(uniSpecular, 1, value_ptr(materialSpecular));

  /* Skybox */
  glUseProgram(shaderSkybox);

  uniSkyboxM = myGetUniformLocation(shaderSkybox, "model");
  uniSkyboxV = myGetUniformLocation(shaderSkybox, "view");
  uniSkyboxP = myGetUniformLocation(shaderSkybox, "projection");

  glUniformMatrix4fv(uniSkyboxM, 1, GL_FALSE, value_ptr(skyboxM));
  glUniformMatrix4fv(uniSkyboxV, 1, GL_FALSE, value_ptr(skyboxV));
  glUniformMatrix4fv(uniSkyboxP, 1, GL_FALSE, value_ptr(skyboxP));
}

float randf() {
  // [0, 1]
  float f = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

  return f;
}
