#include "common.h"

std::string readFile(const std::string filename) {
  std::ifstream in;
  in.open(filename.c_str());

  if (!in) {
    std::cout << "Fail to read file " << filename << '\n';

    return "";
  }

  std::stringstream ss;
  ss << in.rdbuf();
  std::string sOut = ss.str();
  in.close();

  return sOut;
}

// return a shader executable
GLuint buildShader(string vsDir, string fsDir, string tcsDir, string tesDir,
                   string geoDir) {
  GLuint vs, fs, tcs = 0, tes = 0, geo = 0;
  GLint linkOk;
  GLuint exeShader;

  // compile
  vs = compileShader(vsDir, GL_VERTEX_SHADER);
  fs = compileShader(fsDir, GL_FRAGMENT_SHADER);

  // TCS, TES
  if (tcsDir != "" && tesDir != "") {
    tcs = compileShader(tcsDir, GL_TESS_CONTROL_SHADER);
    tes = compileShader(tesDir, GL_TESS_EVALUATION_SHADER);
  }

  // GS
  if (geoDir != "") {
    geo = compileShader(geoDir, GL_GEOMETRY_SHADER);
  }

  // link
  exeShader = linkShader(vs, fs, tcs, tes, geo);

  return exeShader;
}

GLuint compileShader(string filename, GLenum type) {
  /* read source code */
  string sTemp = readFile(filename);
  string info;
  const GLchar *source = sTemp.c_str();

  switch (type) {
  case GL_VERTEX_SHADER:
    info = "Vertex";
    break;
  case GL_FRAGMENT_SHADER:
    info = "Fragment";
    break;
  }

  if (source == NULL) {
    std::cout << info << " Shader : Can't read shader source file."
              << std::endl;
    return 0;
  }

  const GLchar *sources[] = {source};
  GLuint objShader = glCreateShader(type);
  glShaderSource(objShader, 1, sources, NULL);
  glCompileShader(objShader);

  GLint compile_ok;
  glGetShaderiv(objShader, GL_COMPILE_STATUS, &compile_ok);
  if (compile_ok == GL_FALSE) {
    std::cout << info << " Shader : Fail to compile." << std::endl;
    printLog(objShader);
    glDeleteShader(objShader);
    return 0;
  }

  return objShader;
}

GLuint linkShader(GLuint vsObj, GLuint fsObj, GLuint tcsObj, GLuint tesObj,
                  GLuint geoObj) {
  GLuint exe;
  GLint linkOk;

  exe = glCreateProgram();
  glAttachShader(exe, vsObj);
  glAttachShader(exe, fsObj);

  if (tcsObj != 0 && tesObj != 0) {
    glAttachShader(exe, tcsObj);
    glAttachShader(exe, tesObj);
  }

  if (geoObj != 0) {
    glAttachShader(exe, geoObj);
  }

  glLinkProgram(exe);

  // check result
  glGetProgramiv(exe, GL_LINK_STATUS, &linkOk);

  if (linkOk == GL_FALSE) {
    std::cout << "Failed to link shader program." << std::endl;
    printLog(exe);
    glDeleteProgram(exe);

    return 0;
  }

  return exe;
}

void printLog(GLuint &object) {
  GLint log_length = 0;
  if (glIsShader(object)) {
    glGetShaderiv(object, GL_INFO_LOG_LENGTH, &log_length);
  } else if (glIsProgram(object)) {
    glGetProgramiv(object, GL_INFO_LOG_LENGTH, &log_length);
  } else {
    cerr << "printlog: Not a shader or a program" << endl;
    return;
  }

  char *log = (char *)malloc(log_length);

  if (glIsShader(object))
    glGetShaderInfoLog(object, log_length, NULL, log);
  else if (glIsProgram(object))
    glGetProgramInfoLog(object, log_length, NULL, log);

  cerr << log << endl;
  free(log);
}

GLint myGetUniformLocation(GLuint &prog, string name) {
  GLint location;
  location = glGetUniformLocation(prog, name.c_str());
  if (location == -1) {
    cerr << "Could not bind uniform : " << name << ". "
         << "Did you set the right name? "
         << "Or is " << name << " not used?" << endl;
  }

  return location;
}
