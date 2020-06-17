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

void computeMatricesFromInputs();
void keyCallback(GLFWwindow *, int, int, int, int);

GLFWwindow *window;
Skybox skybox;

bool saveTrigger = false;
int frameNumber = 0;

float verticalAngle = -2.36374;
float horizontalAngle = 6.25339;
float initialFoV = 45.0f;
float speed = 5.0f;
float mouseSpeed = 0.005f;
float farPlane = 2000.f;

vec3 eyePoint = vec3(49.897549, 40.268173, -3.964368);
vec3 eyeDirection =
    vec3(sin(verticalAngle) * cos(horizontalAngle), cos(verticalAngle),
         sin(verticalAngle) * sin(horizontalAngle));
vec3 up = vec3(0.f, 1.f, 0.f);

mat4 oceanM, oceanV, oceanP;

int main(int argc, char *argv[]) {
  initGL();
  initOther();
  initShader();
  initMatrix();
  initUniform();

  skybox.init();

  cTimer timer;

  // ocean simulator
  cOcean ocean = cOcean(64, 0.0005f, vec2(0.0f, 32.0f), 64, true);

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

    // skybox
    skybox.draw();

    // ocean
    ocean.render(timer.elapsed(false), vec3(20, 20, 20), oceanP, oceanV, oceanM,
                 false);

    // refresh frame
    glfwSwapBuffers(window);

    /* Poll for and process events */
    glfwPollEvents();

    // if (saveTrigger) {
    //   string dir = "./result/output";
    //   // zero padding
    //   // e.g. "output0001.bmp"
    //   string num = to_string(frameNumber);
    //   num = string(4 - num.length(), '0') + num;
    //   string output = dir + num + ".bmp";
    //
    //   FIBITMAP *outputImage =
    //       FreeImage_AllocateT(FIT_UINT32, WINDOW_WIDTH * 2, WINDOW_HEIGHT *
    //       2);
    //   glReadPixels(0, 0, WINDOW_WIDTH * 2, WINDOW_HEIGHT * 2, GL_BGRA,
    //                GL_UNSIGNED_INT_8_8_8_8_REV,
    //                (GLvoid *)FreeImage_GetBits(outputImage));
    //   FreeImage_Save(FIF_BMP, outputImage, output.c_str(), 0);
    //   std::cout << output << " saved." << '\n';
    //   frameNumber++;
    // }
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
  window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
                            "Dudv water simulation", NULL, NULL);

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
  // glCullFace(GL_FRONT);
  glEnable(GL_DEPTH_TEST); // must enable depth test!!

  glEnable(GL_PROGRAM_POINT_SIZE);
  glPointSize(15);
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

  mat4 newV = lookAt(eyePoint, eyePoint + direction, newUp);
  mat4 newP = perspective(initialFoV, 1.f * WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f,
                          farPlane);

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
    default:
      break;
    }
  }
}

void initShader() {
  skybox.shader =
      buildShader("./shader/vsSkybox.glsl", "./shader/fsSkybox.glsl");
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
}

void initMatrix() {
  // common
  mat4 M, V, P;

  M = translate(mat4(1.f), vec3(0.f, 0.f, -4.f));
  V = lookAt(eyePoint, eyePoint + eyeDirection, up);
  P = perspective(initialFoV, 1.f * WINDOW_WIDTH / WINDOW_HEIGHT, 0.01f,
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
