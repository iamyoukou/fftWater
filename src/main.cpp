#include "common.h"
#include "ocean.h"
#include "skybox.h"

using namespace glm;
using namespace std;

void initGL();
void initOther();
void initShader();
void initMatrix();
void initUniform();
void initSkybox();
void initScreenQuad();

void computeMatricesFromInputs();
void keyCallback(GLFWwindow *, int, int, int, int);

GLFWwindow *window;
Skybox skybox;

bool saveTrigger = false;
int frameNumber = 0;
bool simulation = true;

float verticalAngle = -1.71874;
float horizontalAngle = 2.4934;
float initialFoV = 45.0f;
float speed = 25.0f;
float mouseSpeed = 0.005f;
float nearPlane = 0.01f, farPlane = 2000.f;

vec3 eyePoint = vec3(-27.000000, 15, -16.000000);
vec3 eyeDirection =
    vec3(sin(verticalAngle) * cos(horizontalAngle), cos(verticalAngle),
         sin(verticalAngle) * sin(horizontalAngle));
vec3 up = vec3(0.f, 1.f, 0.f);

mat4 oceanM, oceanV, oceanP;

/* for underwater effect */
GLuint fboScreenQuad, tboScreenQuad, vaoScreenQuad;
GLuint vboScreenQuad, rboDepthScreenQuad;
GLuint shaderScreenQuad;
GLint uniScreenQuadTex, uniAlpha, uniBaseColor;

GLfloat vtxsScreenQuad[] = {
    -1, -1, 1, -1, -1, 1, 1, 1,
};

vec4 underwaterColor(0.0, 0.65, 0.75, 1.0);

int main(int argc, char *argv[]) {
  initGL();
  initOther();
  initShader();
  initMatrix();
  initUniform();
  initScreenQuad();

  skybox.init();

  cTimer timer;

  // ocean simulator
  cOcean ocean = cOcean(64, 0.0005f, vec2(0.0f, 32.0f), 64);

  // a rough way to solve cursor position initialization problem
  // must call glfwPollEvents once to activate glfwSetCursorPos
  // this is a glfw mechanism problem
  glfwPollEvents();
  glfwSetCursorPos(window, WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2);

  /* Loop until the user closes the window */
  while (!glfwWindowShouldClose(window)) {
    // view control
    computeMatricesFromInputs();

    /* render to framebuffer */
    glBindFramebuffer(GL_FRAMEBUFFER, fboScreenQuad);

    // clear framebuffer
    glClearColor(97 / 256.f, 175 / 256.f, 239 / 256.f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // skybox
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_CULL_FACE);
    skybox.draw();

    // ocean
    glDisable(GL_CULL_FACE);
    ocean.render(timer.elapsed(false), vec3(20, 20, 20), oceanP, oceanV, oceanM,
                 simulation);

    /* render to main screen */
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // clear framebuffer
    glClearColor(97 / 256.f, 175 / 256.f, 239 / 256.f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderScreenQuad);

    /* calculate the threshold for the condition of underwater */
    // project the eyePoint onto the ocean surface
    // calculate which ocean cell it is in
    vec3 temp = eyePoint;
    // vec3 tempDir = normalize(vec3(eyeDirection.x, 0, eyeDirection.z));
    // temp += eyeDirection * 2.f;

    int ix = floor(temp.x / ocean.cellSize);
    int iz = floor(temp.z / ocean.cellSize);

    vec3 ref(ix * ocean.cellSize, 0, iz * ocean.cellSize);

    // std::cout << "cell index (before): " << to_string(vec2(ix, iz)) << '\n';

    // if eyePoint is over the duplicated area,
    // transform the indices back to the original area
    while (ix > ocean.N / 2)
      ix -= ocean.N;
    while (ix < -ocean.N / 2)
      ix += ocean.N;
    while (iz > ocean.N / 2)
      iz -= ocean.N;
    while (iz < -ocean.N / 2)
      iz += ocean.N;

    // std::cout << "cell index (after): " << to_string(vec2(ix, iz)) << '\n';

    // ix, iz is [- N/2, N/2]
    // change to [0, N] to access ocean vertices
    ix += ocean.N / 2;
    iz += ocean.N / 2;
    //
    vec3 A, B, C, D;
    A = ocean.getVertex(ix, iz);
    B = ocean.getVertex(ix + 1, iz);
    C = ocean.getVertex(ix + 1, iz + 1);
    D = ocean.getVertex(ix, iz + 1);
    //
    // vec3 whichVtx = ocean.getVertex(ix, iz);
    //
    float alpha = (temp.z - ref.z) / ocean.cellSize;
    float beta = (temp.x - ref.x) / ocean.cellSize;
    //
    float Z1 = (1.f - alpha) * A.y + alpha * D.y;
    float Z2 = (1.f - alpha) * B.y + alpha * C.y;
    float Z = (1.f - beta) * Z1 + beta * Z2;

    float threshold = Z;
    float everage = (A.y + B.y + C.y + D.y) * 0.25;

    // if (simulation) {
    //   std::cout << "alpha = " << alpha << '\n';
    //   std::cout << "beta = " << beta << '\n';
    //
    //   // std::cout << "whichVtx: " << to_string(whichVtx) << '\n';
    //   std::cout << "temp: " << to_string(eyePoint) << '\n';
    //   std::cout << "belongs to: " << to_string(ocean.getVertex(ix, iz)) <<
    //   '\n'; std::cout << "A: " << to_string(A) << '\n'; std::cout << "B: " <<
    //   to_string(B) << '\n'; std::cout << "C: " << to_string(C) << '\n';
    //   std::cout << "D: " << to_string(D) << '\n';
    //   std::cout << "ref: " << to_string(ref) << '\n';
    //   // std::cout << '\n';
    //   // std::cout << "cell index (access): " << to_string(vec2(ix, iz)) <<
    //   // '\n';
    //   // std::cout << to_string(ocean.getVertex(ix + 1, iz)) << '\n';
    //   // std::cout << to_string(ocean.getVertex(ix, iz + 1)) << '\n';
    //   // std::cout << to_string(ocean.getVertex(ix + 1, iz + 1)) << '\n';
    //   std::cout << "threshold: " << threshold << '\n';
    //   std::cout << "everage: " << everage << '\n';
    //   // std::cout << to_string(ocean.getVertex(5, 17)) << '\n';
    //   std::cout << '\n';
    // }

    // for underwater scene
    if (eyePoint.y < threshold) {
      glUniform1f(uniAlpha, 0.5f);
    } else {
      glUniform1f(uniAlpha, 1.f);
    }

    glBindVertexArray(vaoScreenQuad);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // refresh frame
    glfwSwapBuffers(window);

    /* Poll for and process events */
    glfwPollEvents();

    // save frame
    if (saveTrigger) {
      string dir = "./result/output";
      // zero padding
      // e.g. "output0001.bmp"
      string num = to_string(frameNumber);
      num = string(4 - num.length(), '0') + num;
      string output = dir + num + ".bmp";

      FIBITMAP *outputImage =
          FreeImage_AllocateT(FIT_UINT32, WINDOW_WIDTH * 2, WINDOW_HEIGHT * 2);
      glReadPixels(0, 0, WINDOW_WIDTH * 2, WINDOW_HEIGHT * 2, GL_BGRA,
                   GL_UNSIGNED_INT_8_8_8_8_REV,
                   (GLvoid *)FreeImage_GetBits(outputImage));
      FreeImage_Save(FIF_BMP, outputImage, output.c_str(), 0);
      std::cout << output << " saved." << '\n';
      frameNumber++;
    }
  }

  ocean.release();

  glfwTerminate();

  return EXIT_SUCCESS;
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
  window =
      glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "FFT ocean", NULL, NULL);

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
  glEnable(GL_DEPTH_TEST); // must enable depth test!!

  // glEnable(GL_PROGRAM_POINT_SIZE);
  // glPointSize(15);
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
  // As the cursor is put at the center of the screen,
  // (WINDOW_WIDTH/2.f - xpos) and (WINDOW_HEIGHT/2.f - ypos) are offsets
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
  // dive
  if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
    eyePoint -= newUp * deltaTime * speed;
  }
  // rise
  if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
    eyePoint += newUp * deltaTime * speed;
  }

  mat4 newV = lookAt(eyePoint, eyePoint + direction, newUp);
  mat4 newP = perspective(initialFoV, 1.f * WINDOW_WIDTH / WINDOW_HEIGHT,
                          nearPlane, farPlane);

  // update for skybox
  glUseProgram(skybox.shader);
  skybox.V = newV;
  skybox.P = newP;
  glUniformMatrix4fv(skybox.uniV, 1, GL_FALSE, value_ptr(skybox.V));
  glUniformMatrix4fv(skybox.uniP, 1, GL_FALSE, value_ptr(skybox.P));

  // Let the center of the skybox always at eyePoint
  // CAUTION: the matrix of GLM is column major
  skybox.M[3][0] = skybox.oriM[0][3] + eyePoint.x;
  skybox.M[3][1] = skybox.oriM[1][3] + eyePoint.y;
  skybox.M[3][2] = skybox.oriM[2][3] + eyePoint.z;
  glUniformMatrix4fv(skybox.uniM, 1, GL_FALSE, value_ptr(skybox.M));

  // for ocean
  oceanV = newV;
  oceanP = newP;

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
    case GLFW_KEY_Y: {
      saveTrigger = !saveTrigger;
      frameNumber = 0;
      break;
    }
    case GLFW_KEY_X: {
      simulation = !simulation;
      break;
    }
    default:
      break;
    }
  }
}

void initShader() {
  skybox.shader =
      buildShader("./shader/vsSkybox.glsl", "./shader/fsSkybox.glsl");

  shaderScreenQuad =
      buildShader("./shader/vsScreenQuad.glsl", "./shader/fsScreenQuad.glsl");
}

void initUniform() {
  /* Skybox */
  glUseProgram(skybox.shader);

  // transform
  skybox.uniM = myGetUniformLocation(skybox.shader, "M");
  skybox.uniV = myGetUniformLocation(skybox.shader, "V");
  skybox.uniP = myGetUniformLocation(skybox.shader, "P");

  glUniformMatrix4fv(skybox.uniM, 1, GL_FALSE, value_ptr(skybox.M));
  glUniformMatrix4fv(skybox.uniV, 1, GL_FALSE, value_ptr(skybox.V));
  glUniformMatrix4fv(skybox.uniP, 1, GL_FALSE, value_ptr(skybox.P));

  /* Screen quad */
  glUseProgram(shaderScreenQuad);

  uniScreenQuadTex = myGetUniformLocation(shaderScreenQuad, "tex");
  uniAlpha = myGetUniformLocation(shaderScreenQuad, "alpha");
  uniBaseColor = myGetUniformLocation(shaderScreenQuad, "baseColor");

  glUniform1i(uniScreenQuadTex, 10);
  glUniform1f(uniAlpha, 1.f);
  glUniform4fv(uniBaseColor, 1, value_ptr(underwaterColor));
}

void initMatrix() {
  // common
  mat4 M, V, P;

  M = translate(mat4(1.f), vec3(0.f, 0.f, -4.f));
  V = lookAt(eyePoint, eyePoint + eyeDirection, up);
  P = perspective(initialFoV, 1.f * WINDOW_WIDTH / WINDOW_HEIGHT, nearPlane,
                  farPlane);

  // for skybox
  skybox.M = M;
  skybox.oriM = M;
  skybox.V = V;
  skybox.P = P;

  // for ocean
  oceanM = M;
  oceanV = V;
  oceanP = P;
}

void initOther() {
  // initialize random seed
  // this makes the ocean geometry different in every execution
  srand(clock());
}

void initScreenQuad() {
  /* Texture */
  glActiveTexture(GL_TEXTURE0 + 10);
  glGenTextures(1, &tboScreenQuad);
  glBindTexture(GL_TEXTURE_2D, tboScreenQuad);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  // On OSX, must use WINDOW_WIDTH * 2 and WINDOW_HEIGHT * 2, don't know why
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WINDOW_WIDTH * 2, WINDOW_HEIGHT * 2,
               0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

  /* Depth buffer */
  glGenRenderbuffers(1, &tboScreenQuad);
  glBindRenderbuffer(GL_RENDERBUFFER, tboScreenQuad);
  // On OSX, must use WINDOW_WIDTH * 2 and WINDOW_HEIGHT * 2, don't know why
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, WINDOW_WIDTH * 2,
                        WINDOW_HEIGHT * 2);

  /* Framebuffer to link everything together */
  glGenFramebuffers(1, &fboScreenQuad);
  glBindFramebuffer(GL_FRAMEBUFFER, fboScreenQuad);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         tboScreenQuad, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, tboScreenQuad);

  GLenum status;
  if ((status = glCheckFramebufferStatus(GL_FRAMEBUFFER)) !=
      GL_FRAMEBUFFER_COMPLETE) {
    fprintf(stderr, "glCheckFramebufferStatus: error %u", status);
    exit(EXIT_FAILURE);
  }

  // screen quad
  glGenVertexArrays(1, &vaoScreenQuad);
  glBindVertexArray(vaoScreenQuad);

  glGenBuffers(1, &vboScreenQuad);
  glBindBuffer(GL_ARRAY_BUFFER, vboScreenQuad);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vtxsScreenQuad), vtxsScreenQuad,
               GL_STATIC_DRAW);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(0);
}
