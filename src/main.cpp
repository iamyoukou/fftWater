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
Skybox *skybox;

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
mat4 model, view, projection;

/* for underwater effect */
GLuint fboScreenQuad, tboScreenQuad, vaoScreenQuad;
GLuint vboScreenQuad, rboDepthScreenQuad;
GLuint shaderScreenQuad;
GLint uniScreenQuadTex, uniAlpha, uniBaseColor;

GLfloat vtxsScreenQuad[] = {
    -1, -1, 1, -1, -1, 1, 1, 1,
};

vec4 underwaterColor(0.0, 0.65, 0.75, 1.0);
bool isRising = false, isDiving = false;

int main(int argc, char *argv[]) {
  initGL();
  initOther();
  initShader();
  initMatrix();
  initUniform();
  initScreenQuad();

  skybox = new Skybox();

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
    skybox->draw(model, view, projection, eyePoint);

    // ocean
    glDisable(GL_CULL_FACE);
    ocean.render(timer.elapsed(false), vec3(20, 20, 20), oceanP, oceanV, oceanM,
                 simulation, frameNumber);

    /* render to main screen */
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // clear framebuffer
    glClearColor(97 / 256.f, 175 / 256.f, 239 / 256.f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderScreenQuad);

    // for underwater scene
    if (eyePoint.y < 0.f) {
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
    }

    frameNumber++;
  }

  // release
  ocean.release();
  delete skybox;

  glfwTerminate();
  FreeImage_DeInitialise();

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

  // restrict viewing angles
  verticalAngle = glm::clamp(verticalAngle, -2.0f, -0.75f);

  // Direction : Spherical coordinates to Cartesian coordinates conversion
  vec3 direction =
      vec3(sin(verticalAngle) * cos(horizontalAngle), cos(verticalAngle),
           sin(verticalAngle) * sin(horizontalAngle));

  // Right vector
  vec3 right = vec3(cos(horizontalAngle - 3.14 / 2.f), 0.f,
                    sin(horizontalAngle - 3.14 / 2.f));

  // new up vector
  vec3 newUp = cross(right, direction);

  // restrict movements, can only move horizontally
  vec3 forwardDir = normalize(vec3(direction.x, 0, direction.z));
  vec3 rightDir = normalize(vec3(right.x, 0, right.z));

  // Move forward
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
    eyePoint += forwardDir * deltaTime * speed;
  }
  // Move backward
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    eyePoint -= forwardDir * deltaTime * speed;
  }
  // Strafe right
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    eyePoint += rightDir * deltaTime * speed;
  }
  // Strafe left
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    eyePoint -= rightDir * deltaTime * speed;
  }
  // dive
  if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS && !isDiving) {
    isDiving = true;
    isRising = false;
  }
  // rise
  if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS && !isRising) {
    isDiving = false;
    isRising = true;
  }

  // smoothly dive
  if (isDiving && eyePoint.y > -20.f) {
    eyePoint.y -= 2.f;

    if (eyePoint.y < -20.f) {
      isDiving = false;
      eyePoint.y = -20.f;
    }
  }
  // smoothly rise
  else if (isRising && eyePoint.y < 15.f) {
    eyePoint.y += 2.f;

    if (eyePoint.y > 15.f) {
      isRising = false;
      eyePoint.y = 15.f;
    }
  }

  mat4 newV = lookAt(eyePoint, eyePoint + direction, newUp);
  mat4 newP = perspective(initialFoV, 1.f * WINDOW_WIDTH / WINDOW_HEIGHT,
                          nearPlane, farPlane);

  view = newV;
  projection = newP;

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
  shaderScreenQuad =
      buildShader("./shader/vsScreenQuad.glsl", "./shader/fsScreenQuad.glsl");
}

void initUniform() {
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
  model = translate(mat4(1.f), vec3(0.f, 0.f, -4.f));
  view = lookAt(eyePoint, eyePoint + eyeDirection, up);
  projection = perspective(initialFoV, 1.f * WINDOW_WIDTH / WINDOW_HEIGHT,
                           nearPlane, farPlane);

  // for ocean
  oceanM = model;
  oceanV = view;
  oceanP = projection;
}

void initOther() {
  // initialize random seed
  // this makes the ocean geometry different in every execution
  srand(clock());

  FreeImage_Initialise(true);
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
