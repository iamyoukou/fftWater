#include "common.h"
#include "ocean.h"
#include "skybox.h"
#include "screenQuad.h"

using namespace glm;
using namespace std;

void initGL();
void initOther();
void initMatrix();
void releaseResource();

void computeMatricesFromInputs();
void keyCallback(GLFWwindow *, int, int, int, int);

void saveHeightMap();
void saveNormalMap();

GLFWwindow *window;
Skybox *skybox;
cOcean *ocean;
ScreenQuad *screenQuad;

bool saveTrigger = false;
int frameNumber = 0;
bool resume = true;
bool saveMap = true;

float verticalAngle = -1.8238;
float horizontalAngle = 1.56569;
float initialFoV = 45.0f;
float speed = 5.0f;
float mouseSpeed = 0.005f;
float nearPlane = 0.01f, farPlane = 2000.f;

vec3 eyePoint = vec3(0.097707, 0.681555, 2.229438);
vec3 eyeDirection =
    vec3(sin(verticalAngle) * cos(horizontalAngle), cos(verticalAngle),
         sin(verticalAngle) * sin(horizontalAngle));
vec3 up = vec3(0.f, 1.f, 0.f);

mat4 model, view, projection;
bool isRising = false, isDiving = false;

vec3 lightPos = vec3(0.f, 5.f, 0.f);
vec3 lightColor = vec3(1.f, 1.f, 1.f);
float lightPower = 12.f;

int N = 64;
float t = 0.f;

int main(int argc, char *argv[]) {
  initGL();
  initOther();
  initMatrix();

  skybox = new Skybox();

  screenQuad = new ScreenQuad();

  cTimer timer;

  // ocean simulator
  ocean = new cOcean(N, 0.005f, vec2(16.0f, 0.0f), 16);

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
    // glBindFramebuffer(GL_FRAMEBUFFER, screenQuad->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // clear framebuffer
    glClearColor(97 / 256.f, 175 / 256.f, 239 / 256.f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // skybox
    // glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_CULL_FACE);
    skybox->draw(model, view, projection, eyePoint);

    // ocean
    glDisable(GL_CULL_FACE);
    ocean->render(t, model, view, projection, eyePoint, lightColor, lightPos,
                  resume, frameNumber);

    if (resume) {
      t += 0.01f;
    }

    // save height map
    // if (saveMap) {
    //   saveHeightMap();
    //   saveNormalMap();
    //
    //   saveMap = false;
    // }

    /* render to main screen */
    // glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // clear framebuffer
    // glClearColor(97 / 256.f, 175 / 256.f, 239 / 256.f, 1.0f);
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //
    // screenQuad->draw(eyePoint);

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
  releaseResource();

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

  glPatchParameteri(GL_PATCH_VERTICES, 4);
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
  // verticalAngle = glm::clamp(verticalAngle, -2.0f, -0.75f);

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
  // vec3 forwardDir = normalize(vec3(direction.x, 0, direction.z));
  // vec3 rightDir = normalize(vec3(right.x, 0, right.z));
  vec3 forwardDir = normalize(direction);
  vec3 rightDir = normalize(right);

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

  // update transform matrix
  view = lookAt(eyePoint, eyePoint + direction, newUp);
  projection = perspective(initialFoV, 1.f * WINDOW_WIDTH / WINDOW_HEIGHT,
                           nearPlane, farPlane);

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
      resume = !resume;
      break;
    }
    case GLFW_KEY_M: {
      saveMap = true;
      break;
    }
    default:
      break;
    }
  }
}

void initMatrix() {
  model = translate(mat4(1.f), vec3(0.f, 0.f, 0.f));
  view = lookAt(eyePoint, eyePoint + eyeDirection, up);
  projection = perspective(initialFoV, 1.f * WINDOW_WIDTH / WINDOW_HEIGHT,
                           nearPlane, farPlane);
}

void initOther() {
  // initialize random seed
  // this makes the ocean geometry different in every execution
  srand(clock());

  FreeImage_Initialise(true);
}

void releaseResource() {
  delete ocean;
  delete skybox;
  delete screenQuad;

  glfwTerminate();
  FreeImage_DeInitialise();
}

void saveHeightMap() {
  int w, h;
  w = N;
  h = w;

  // find max, min
  float maxHeight = 0, minHeight = 0;
  // float maxDispX = 0, minDispX = 0;
  // float maxDispZ = 0, minDispZ = 0;

  // find max
  for (int i = 0; i < w; i++) {
    for (int j = 0; j < h; j++) {
      int idx = i * N + j;

      // ox, oy, oz: original position
      // x, y, z: position after simulation
      float y = ocean->vertices[idx].oy - ocean->vertices[idx].y;
      // float x = ocean->vertices[idx].ox - ocean->vertices[idx].x;
      // float z = ocean->vertices[idx].oz - ocean->vertices[idx].z;

      if (y > maxHeight) {
        maxHeight = y;
      }
      // if (x > maxDispX) {
      //   maxDispX = x;
      // }
      // if (z > maxDispZ) {
      //   maxDispZ = z;
      // }
    }
  }

  // find min
  for (int i = 0; i < w; i++) {
    for (int j = 0; j < h; j++) {
      int idx = i * N + j;

      // ox, oy, oz: original position
      // x, y, z: position after simulation
      float y = ocean->vertices[idx].oy - ocean->vertices[idx].y;
      // float x = ocean->vertices[idx].ox - ocean->vertices[idx].x;
      // float z = ocean->vertices[idx].oz - ocean->vertices[idx].z;

      if (y < minHeight) {
        minHeight = y;
      }
      // if (x < minDispX) {
      //   minDispX = x;
      // }
      // if (z < minDispZ) {
      //   minDispZ = z;
      // }
    }
  }

  // height range
  float heightRange = maxHeight - minHeight;
  // float xRange = maxDispX - minDispX;
  // float zRange = maxDispZ - minDispZ;

  // std::cout << "maxHeight: " << maxHeight << '\n';
  // std::cout << "minHeight: " << minHeight << '\n';

  FIBITMAP *bitmap = FreeImage_Allocate(w, h, 24);
  RGBQUAD color;

  if (!bitmap) {
    std::cout << "FreeImage: Cannot allocate image." << '\n';
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < w; i++) {
    for (int j = 0; j < h; j++) {
      int idx = i * N + j;

      // ox, oy, oz: original position
      // x, y, z: position after simulation
      // float x = ocean->vertices[idx].ox - ocean->vertices[idx].x;
      // x += abs(minDispX);
      // x /= xRange;
      // int ix = int(x * 255.0);

      float scale = 2.0;
      float y = ocean->vertices[idx].oy - ocean->vertices[idx].y;
      // y += abs(minHeight);     // to non-negative
      // y /= heightRange;        // normalize
      // int iy = int(y * 255.0); // to [0, 255]
      int sign = (y < 0) ? 0 : 255;
      int iy = int(abs(y) / scale * 255.0);
      // std::cout << iy << '\n';

      // float z = ocean->vertices[idx].oz - ocean->vertices[idx].z;
      // z += abs(minDispZ);
      // z /= zRange;
      // int iz = int(z * 255.0);

      color.rgbRed = iy;          // value
      color.rgbGreen = sign;      // sign
      color.rgbBlue = int(scale); // scale
      FreeImage_SetPixelColor(bitmap, i, j, &color);
    }
  }

  if (FreeImage_Save(FIF_PNG, bitmap, "./image/height.png", 0))
    cout << "Height map successfully saved!" << endl;
}

void saveNormalMap() {
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

      vec3 normal(ocean->vertices[idx].nx, ocean->vertices[idx].ny,
                  ocean->vertices[idx].nz);
      normal = normalize(normal);      // to [-1, 1]
      normal = (normal + 1.0f) / 2.0f; // to [0, 1]

      // to [0, 255]
      int ix = int(normal.x * 255.0);
      int iy = int(normal.y * 255.0);
      int iz = int(normal.z * 255.0);

      color.rgbRed = ix;
      color.rgbGreen = iy;
      color.rgbBlue = iz;

      // std::cout << ix << ", " << iy << ", " << iz << '\n';

      FreeImage_SetPixelColor(bitmap, i, j, &color);
    }
  }

  if (FreeImage_Save(FIF_PNG, bitmap, "./image/normal.png", 0))
    cout << "Normal map successfully saved!" << endl;
}
