#include "common.h"

std::string readFile(const std::string filename) {
  std::ifstream in;
  in.open(filename.c_str());
  std::stringstream ss;
  ss << in.rdbuf();
  std::string sOut = ss.str();
  in.close();

  return sOut;
}

Mesh loadObj(std::string filename) {
  Mesh outMesh;

  std::ifstream fin;
  fin.open(filename.c_str());

  if (!(fin.good())) {
    std::cout << "failed to open file : " << filename << std::endl;
  }

  while (fin.peek() != EOF) { // read obj loop
    std::string s;
    fin >> s;

    // vertex coordinate
    if ("v" == s) {
      float x, y, z;
      fin >> x;
      fin >> y;
      fin >> z;
      outMesh.vertices.push_back(glm::vec3(x, y, z));
    }
    // texture coordinate
    else if ("vt" == s) {
      float u, v;
      fin >> u;
      fin >> v;
      outMesh.uvs.push_back(glm::vec2(u, v));
    }
    // face normal (recorded as vn in obj file)
    else if ("vn" == s) {
      float x, y, z;
      fin >> x;
      fin >> y;
      fin >> z;
      outMesh.faceNormals.push_back(glm::vec3(x, y, z));
    }
    // vertices contained in face, and face normal
    else if ("f" == s) {
      Face f;

      // v1/vt1/vn1
      fin >> f.v1;
      fin.ignore(1);
      fin >> f.vt1;
      fin.ignore(1);
      fin >> f.vn1;

      // v2/vt2/vn2
      fin >> f.v2;
      fin.ignore(1);
      fin >> f.vt2;
      fin.ignore(1);
      fin >> f.vn2;

      // v3/vt3/vn3
      fin >> f.v3;
      fin.ignore(1);
      fin >> f.vt3;
      fin.ignore(1);
      fin >> f.vn3;

      // Note:
      //  v, vt, vn in "v/vt/vn" start from 1,
      //  but indices of std::vector start from 0,
      //  so we need minus 1 for all elements
      f.v1 -= 1;
      f.vt1 -= 1;
      f.vn1 -= 1;

      f.v2 -= 1;
      f.vt2 -= 1;
      f.vn2 -= 1;

      f.v3 -= 1;
      f.vt3 -= 1;
      f.vn3 -= 1;

      outMesh.faces.push_back(f);
    } else {
      continue;
    }
  } // end read obj loop

  fin.close();

  return outMesh;
}

// return a shader executable
GLuint buildShader(string vsDir, string fsDir) {
  GLuint vs, fs;
  GLint linkOk;
  GLuint exeShader;

  // compile
  vs = compileShader(vsDir, GL_VERTEX_SHADER);
  fs = compileShader(fsDir, GL_FRAGMENT_SHADER);

  // link
  exeShader = linkShader(vs, fs);

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

GLuint linkShader(GLuint vsObj, GLuint fsObj) {
  GLuint exe;
  GLint linkOk;

  exe = glCreateProgram();
  glAttachShader(exe, vsObj);
  glAttachShader(exe, fsObj);
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

/* Mesh class */
void Mesh::translate(glm::vec3 xyz) {
  // move each vertex with xyz
  for (size_t i = 0; i < vertices.size(); i++) {
    vertices[i] += xyz;
  }

  // update aabb
  min += xyz;
  max += xyz;
}

void Mesh::scale(glm::vec3 xyz) {
  // scale each vertex with xyz
  for (size_t i = 0; i < vertices.size(); i++) {
    vertices[i].x *= xyz.x;
    vertices[i].y *= xyz.y;
    vertices[i].z *= xyz.z;
  }

  // update aabb
  min.x *= xyz.x;
  min.y *= xyz.y;
  min.z *= xyz.z;

  max.x *= xyz.x;
  max.y *= xyz.y;
  max.z *= xyz.z;
}

// rotate mesh along x, y, z axes
// xyz specifies the rotated angle along each axis
void Mesh::rotate(glm::vec3 xyz) {}

void createMesh(Mesh &mesh) {
  // write vertex coordinate to array
  int nOfFaces = mesh.faces.size();

  // 3 vertices per face, 3 float per vertex coord, 2 float per tex coord
  GLfloat *aVtxCoords = new GLfloat[nOfFaces * 3 * 3];
  GLfloat *aUvs = new GLfloat[nOfFaces * 3 * 2];
  GLfloat *aNormals = new GLfloat[nOfFaces * 3 * 3];

  for (size_t i = 0; i < nOfFaces; i++) {
    // vertex 1
    int vtxIdx = mesh.faces[i].v1;
    aVtxCoords[i * 9 + 0] = mesh.vertices[vtxIdx].x;
    aVtxCoords[i * 9 + 1] = mesh.vertices[vtxIdx].y;
    aVtxCoords[i * 9 + 2] = mesh.vertices[vtxIdx].z;

    // normal for vertex 1
    int nmlIdx = mesh.faces[i].vn1;
    aNormals[i * 9 + 0] = mesh.faceNormals[nmlIdx].x;
    aNormals[i * 9 + 1] = mesh.faceNormals[nmlIdx].y;
    aNormals[i * 9 + 2] = mesh.faceNormals[nmlIdx].z;

    // uv for vertex 1
    int uvIdx = mesh.faces[i].vt1;
    aUvs[i * 6 + 0] = mesh.uvs[uvIdx].x;
    aUvs[i * 6 + 1] = mesh.uvs[uvIdx].y;

    // vertex 2
    vtxIdx = mesh.faces[i].v2;
    aVtxCoords[i * 9 + 3] = mesh.vertices[vtxIdx].x;
    aVtxCoords[i * 9 + 4] = mesh.vertices[vtxIdx].y;
    aVtxCoords[i * 9 + 5] = mesh.vertices[vtxIdx].z;

    // normal for vertex 2
    nmlIdx = mesh.faces[i].vn2;
    aNormals[i * 9 + 3] = mesh.faceNormals[nmlIdx].x;
    aNormals[i * 9 + 4] = mesh.faceNormals[nmlIdx].y;
    aNormals[i * 9 + 5] = mesh.faceNormals[nmlIdx].z;

    // uv for vertex 2
    uvIdx = mesh.faces[i].vt2;
    aUvs[i * 6 + 2] = mesh.uvs[uvIdx].x;
    aUvs[i * 6 + 3] = mesh.uvs[uvIdx].y;

    // vertex 3
    vtxIdx = mesh.faces[i].v3;
    aVtxCoords[i * 9 + 6] = mesh.vertices[vtxIdx].x;
    aVtxCoords[i * 9 + 7] = mesh.vertices[vtxIdx].y;
    aVtxCoords[i * 9 + 8] = mesh.vertices[vtxIdx].z;

    // normal for vertex 3
    nmlIdx = mesh.faces[i].vn3;
    aNormals[i * 9 + 6] = mesh.faceNormals[nmlIdx].x;
    aNormals[i * 9 + 7] = mesh.faceNormals[nmlIdx].y;
    aNormals[i * 9 + 8] = mesh.faceNormals[nmlIdx].z;

    // uv for vertex 3
    uvIdx = mesh.faces[i].vt3;
    aUvs[i * 6 + 4] = mesh.uvs[uvIdx].x;
    aUvs[i * 6 + 5] = mesh.uvs[uvIdx].y;
  }

  // vao
  glGenVertexArrays(1, &mesh.vao);
  glBindVertexArray(mesh.vao);

  // vbo for vertex
  glGenBuffers(1, &mesh.vboVtxs);
  glBindBuffer(GL_ARRAY_BUFFER, mesh.vboVtxs);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * nOfFaces * 3 * 3, aVtxCoords,
               GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(0);

  // vbo for texture
  glGenBuffers(1, &mesh.vboUvs);
  glBindBuffer(GL_ARRAY_BUFFER, mesh.vboUvs);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * nOfFaces * 3 * 2, aUvs,
               GL_STATIC_DRAW);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(1);

  // vbo for normal
  glGenBuffers(1, &mesh.vboNormals);
  glBindBuffer(GL_ARRAY_BUFFER, mesh.vboNormals);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * nOfFaces * 3 * 3, aNormals,
               GL_STATIC_DRAW);
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(2);

  // delete client data
  delete[] aVtxCoords;
  delete[] aUvs;
  delete[] aNormals;
}

// Whenever the vertex attributes have been changed, call this function
// Otherwise, the vertex data on the server side will not be updated
void updateMesh(Mesh &mesh) {
  // write vertex coordinate to array
  int nOfFaces = mesh.faces.size();

  // 3 vertices per face, 3 float per vertex coord, 2 float per tex coord
  GLfloat *aVtxCoords = new GLfloat[nOfFaces * 3 * 3];
  // GLfloat *aUvs = new GLfloat[nOfFaces * 3 * 2];
  // GLfloat *aNormals = new GLfloat[nOfFaces * 3 * 3];

  for (size_t i = 0; i < nOfFaces; i++) {
    // vertex 1
    int vtxIdx = mesh.faces[i].v1;
    aVtxCoords[i * 9 + 0] = mesh.vertices[vtxIdx].x;
    aVtxCoords[i * 9 + 1] = mesh.vertices[vtxIdx].y;
    aVtxCoords[i * 9 + 2] = mesh.vertices[vtxIdx].z;

    // normal for vertex 1
    // int nmlIdx = mesh.faces[i].vn1;
    // aNormals[i * 9 + 0] = mesh.faceNormals[nmlIdx].x;
    // aNormals[i * 9 + 1] = mesh.faceNormals[nmlIdx].y;
    // aNormals[i * 9 + 2] = mesh.faceNormals[nmlIdx].z;

    // uv for vertex 1
    // int uvIdx = mesh.faces[i].vt1;
    // aUvs[i * 6 + 0] = mesh.uvs[uvIdx].x;
    // aUvs[i * 6 + 1] = mesh.uvs[uvIdx].y;

    // vertex 2
    vtxIdx = mesh.faces[i].v2;
    aVtxCoords[i * 9 + 3] = mesh.vertices[vtxIdx].x;
    aVtxCoords[i * 9 + 4] = mesh.vertices[vtxIdx].y;
    aVtxCoords[i * 9 + 5] = mesh.vertices[vtxIdx].z;

    // normal for vertex 2
    // nmlIdx = mesh.faces[i].vn2;
    // aNormals[i * 9 + 3] = mesh.faceNormals[nmlIdx].x;
    // aNormals[i * 9 + 4] = mesh.faceNormals[nmlIdx].y;
    // aNormals[i * 9 + 5] = mesh.faceNormals[nmlIdx].z;

    // uv for vertex 2
    // uvIdx = mesh.faces[i].vt2;
    // aUvs[i * 6 + 2] = mesh.uvs[uvIdx].x;
    // aUvs[i * 6 + 3] = mesh.uvs[uvIdx].y;

    // vertex 3
    vtxIdx = mesh.faces[i].v3;
    aVtxCoords[i * 9 + 6] = mesh.vertices[vtxIdx].x;
    aVtxCoords[i * 9 + 7] = mesh.vertices[vtxIdx].y;
    aVtxCoords[i * 9 + 8] = mesh.vertices[vtxIdx].z;

    // normal for vertex 3
    // nmlIdx = mesh.faces[i].vn3;
    // aNormals[i * 9 + 6] = mesh.faceNormals[nmlIdx].x;
    // aNormals[i * 9 + 7] = mesh.faceNormals[nmlIdx].y;
    // aNormals[i * 9 + 8] = mesh.faceNormals[nmlIdx].z;

    // uv for vertex 3
    // uvIdx = mesh.faces[i].vt3;
    // aUvs[i * 6 + 4] = mesh.uvs[uvIdx].x;
    // aUvs[i * 6 + 5] = mesh.uvs[uvIdx].y;
  }

  // vao
  glBindVertexArray(mesh.vao);

  // vbo for vertex
  glBindBuffer(GL_ARRAY_BUFFER, mesh.vboVtxs);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * nOfFaces * 3 * 3, aVtxCoords,
               GL_STATIC_DRAW);

  // vbo for texture
  // glBindBuffer(GL_ARRAY_BUFFER, mesh.vboUvs);
  // glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * nOfFaces * 3 * 2, aUvs,
  //              GL_STATIC_DRAW);

  // vbo for normal
  // glBindBuffer(GL_ARRAY_BUFFER, mesh.vboNormals);
  // glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * nOfFaces * 3 * 3, aNormals,
  //              GL_STATIC_DRAW);

  // delete client data
  delete[] aVtxCoords;
  // delete[] aUvs;
  // delete[] aNormals;
}

void findAABB(Mesh &mesh) {
  int nOfVtxs = mesh.vertices.size();
  vec3 min(0, 0, 0), max(0, 0, 0);

  for (size_t i = 0; i < nOfVtxs; i++) {
    vec3 vtx = mesh.vertices[i];

    // x
    if (vtx.x > max.x) {
      max.x = vtx.x;
    }
    if (vtx.x < min.x) {
      min.x = vtx.x;
    }
    // y
    if (vtx.y > max.y) {
      max.y = vtx.y;
    }
    if (vtx.y < min.y) {
      min.y = vtx.y;
    }
    // z
    if (vtx.z > max.z) {
      max.z = vtx.z;
    }
    if (vtx.z < min.z) {
      min.z = vtx.z;
    }
  }

  mesh.min = min;
  mesh.max = max;
}

void drawBox(vec3 min, vec3 max) {
  // 8 corners
  GLfloat aVtxs[]{
      min.x, max.y, min.z, // 0
      min.x, min.y, min.z, // 1
      max.x, min.y, min.z, // 2
      max.x, max.y, min.z, // 3
      min.x, max.y, max.z, // 4
      min.x, min.y, max.z, // 5
      max.x, min.y, max.z, // 6
      max.x, max.y, max.z  // 7
  };

  // vertex color
  // GLfloat colorArray[] = {color.x, color.y, color.z, color.x, color.y,
  // color.z,
  //                         color.x, color.y, color.z, color.x, color.y,
  //                         color.z, color.x, color.y, color.z, color.x,
  //                         color.y, color.z, color.x, color.y, color.z,
  //                         color.x, color.y, color.z};

  // vertex index
  GLushort aIdxs[] = {
      0, 1, 2, 3, // front face
      4, 7, 6, 5, // back
      4, 0, 3, 7, // up
      5, 6, 2, 1, // down
      0, 4, 5, 1, // left
      3, 2, 6, 7  // right
  };

  // prepare buffers to draw
  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  GLuint vboVtx;
  glGenBuffers(1, &vboVtx);
  glBindBuffer(GL_ARRAY_BUFFER, vboVtx);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 8 * 3, aVtxs, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(0);

  // GLuint vboColor;
  // glGenBuffers(1, &vboColor);
  // glBindBuffer(GL_ARRAY_BUFFER, vboColor);
  // glBufferData(GL_ARRAY_BUFFER, sizeof(colorArray), colorArray,
  // GL_STATIC_DRAW); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
  // glEnableVertexAttribArray(1);

  GLuint ibo;
  glGenBuffers(1, &ibo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(aIdxs), aIdxs, GL_STATIC_DRAW);

  // draw box
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
  for (size_t i = 0; i < 6; i++) {
    glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_SHORT,
                   (GLvoid *)(sizeof(GLushort) * 4 * i));
  }

  glDeleteBuffers(1, &vboVtx);
  // glDeleteBuffers(1, &vboColor);
  glDeleteBuffers(1, &ibo);
  glDeleteVertexArrays(1, &vao);
}
