#define USE_OPENGL2
#include "OpenGLWindow/OpenGLInclude.h"
#ifdef _WIN32
#include "OpenGLWindow/Win32OpenGLWindow.h"
#elif defined __APPLE__
#include "OpenGLWindow/MacOpenGLWindow.h"
#else
// assume linux
#include "OpenGLWindow/X11OpenGLWindow.h"
#endif

#ifdef _WIN32
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#endif

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_btgui.h"

#include "imgui/ImGuizmo.h"

b3gDefaultOpenGLWindow *window = 0;
int gWidth = 512;
int gHeight = 512;
int gMousePosX = -1, gMousePosY = -1;
bool gMouseLeftDown = false;
bool gTabPressed = false;
bool gShiftPressed = false;

// Ident matrix
float gGizmoMatrix[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

void checkErrors(std::string desc) {
  GLenum e = glGetError();
  if (e != GL_NO_ERROR) {
    fprintf(stderr, "OpenGL error in \"%s\": %d (%d)\n", desc.c_str(), e, e);
    exit(20);
  }
}

void keyboardCallback(int keycode, int state) {
  printf("hello key %d, state %d(ctrl %d)\n", keycode, state,
         window->isModifierKeyPressed(B3G_CONTROL));
  // if (keycode == 'q' && window && window->isModifierKeyPressed(B3G_SHIFT)) {
  if (keycode == 27) {
    if (window)
      window->setRequestExit();
  } else if (keycode == 9) {
    gTabPressed = (state == 1);
  } else if (keycode == B3G_SHIFT) {
    gShiftPressed = (state == 1);
  }

  ImGui_ImplBtGui_SetKeyState(keycode, (state == 1));

  if (keycode >= 32 && keycode <= 126) {
    if (state == 1) {
      ImGui_ImplBtGui_SetChar(keycode);
    }
  }
}

void mouseMoveCallback(float x, float y) {

  if (gMouseLeftDown) {
  }

  gMousePosX = (int)x;
  gMousePosY = (int)y;
}

void mouseButtonCallback(int button, int state, float x, float y) {
  ImGui_ImplBtGui_SetMouseButtonState(button, (state == 1));

  ImGuiIO &io = ImGui::GetIO();
  if (io.WantCaptureMouse || io.WantCaptureKeyboard) {
    return;
  }

  // left button
  if (button == 0) {
    if (state) {
      gMouseLeftDown = true;
    } else
      gMouseLeftDown = false;
  }
}

// http://stackoverflow.com/questions/2417697/gluperspective-was-removed-in-opengl-3-1-any-replacements
void buildPerspProjMat(GLfloat *m, GLfloat fov, GLfloat aspect, GLfloat znear,
                       GLfloat zfar) {

  GLfloat h = tan(fov);
  GLfloat w = h / aspect;
  GLfloat depth = znear - zfar;
  GLfloat q = (zfar + znear) / depth;
  GLfloat qn = 2 * zfar * znear / depth;

  m[0] = w;
  m[1] = 0;
  m[2] = 0;
  m[3] = 0;
  m[4] = 0;
  m[5] = h;
  m[6] = 0;
  m[7] = 0;
  m[8] = 0;
  m[9] = 0;
  m[10] = q;
  m[11] = -1;
  m[12] = 0;
  m[13] = 0;
  m[14] = qn;
  m[15] = 0;
}

void resizeCallback(float width, float height) {
  GLfloat h = (GLfloat)height / (GLfloat)width;
  GLfloat xmax, znear, zfar;

  znear = 1.0f;
  zfar = 100.0f;
  xmax = znear * 0.5f;

  GLfloat m[16];
  buildPerspProjMat(m, 45.0f, (float)width / (float)height, znear, zfar);

  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf(m);

  gWidth = width;
  gHeight = height;
}

void Gizmo(const float *modelview_matrix, const float *projection_matrix) {
  static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
  static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::LOCAL);

  // Maya shortcut keys
  if (ImGui::IsKeyPressed(90)) // w Key
    mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
  if (ImGui::IsKeyPressed(69)) // e Key
    mCurrentGizmoOperation = ImGuizmo::ROTATE;
  if (ImGui::IsKeyPressed(82)) // r Key
    mCurrentGizmoOperation = ImGuizmo::SCALE;

  if (ImGui::RadioButton("Translate",
                         mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
    mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
  ImGui::SameLine();
  if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
    mCurrentGizmoOperation = ImGuizmo::ROTATE;
  ImGui::SameLine();
  if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
    mCurrentGizmoOperation = ImGuizmo::SCALE;

  float matrixTranslation[3], matrixRotation[3], matrixScale[3];
  ImGuizmo::DecomposeMatrixToComponents(gGizmoMatrix, matrixTranslation,
                                        matrixRotation, matrixScale);
  ImGui::InputFloat3("Tr", matrixTranslation, 3);
  ImGui::InputFloat3("Rt", matrixRotation, 3);
  ImGui::InputFloat3("Sc", matrixScale, 3);
  ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation,
                                          matrixScale, gGizmoMatrix);

  if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
    mCurrentGizmoMode = ImGuizmo::LOCAL;
  ImGui::SameLine();
  if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
    mCurrentGizmoMode = ImGuizmo::WORLD;

  ImGuizmo::Manipulate(modelview_matrix, projection_matrix,
                       mCurrentGizmoOperation, mCurrentGizmoMode, gGizmoMatrix);
}

int main(int argc, char **argv) {

  window = new b3gDefaultOpenGLWindow;
  b3gWindowConstructionInfo ci;
#ifdef USE_OPENGL2
  ci.m_openglVersion = 2;
#endif
  ci.m_width = 640;
  ci.m_height = 480;
  window->createWindow(ci);

  window->setWindowTitle("view");

#ifndef __APPLE__
#ifndef _WIN32
  // some Linux implementations need the 'glewExperimental' to be true
  glewExperimental = GL_TRUE;
#endif
  if (glewInit() != GLEW_OK) {
    fprintf(stderr, "Failed to initialize GLEW\n");
    exit(-1);
  }

  if (!GLEW_VERSION_2_1) {
    fprintf(stderr, "OpenGL 2.1 is not available\n");
    exit(-1);
  }
#endif

  checkErrors("init");

  window->setMouseButtonCallback(mouseButtonCallback);
  window->setMouseMoveCallback(mouseMoveCallback);
  window->setKeyboardCallback(keyboardCallback);
  window->setResizeCallback(resizeCallback);

  ImGui_ImplBtGui_Init(window);

  ImGuiIO &io = ImGui::GetIO();
  io.Fonts->AddFontDefault();
  // io.Fonts->AddFontFromFileTTF("./DroidSans.ttf", 15.0f);

  float modelview_matrix[16];
  float projection_matrix[16];

  while (!window->requestedExit()) {
    window->startRendering();

    checkErrors("begin frame");

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    // @fixme { Something is wrong in matrix so ImGuizmo is not rendererd
    // correctly. }
    glTranslatef(0, 0, 5);

    glGetFloatv(GL_MODELVIEW_MATRIX, modelview_matrix);
    glGetFloatv(GL_PROJECTION_MATRIX, projection_matrix);

    ImGui_ImplBtGui_NewFrame(gMousePosX, gMousePosY);
    // ImGuizmo
    ImGuizmo::BeginFrame();
    ImGuizmo::Enable(true);
    Gizmo(modelview_matrix, projection_matrix);

    ImGui::Begin("UI");
    {
      static float col[3] = {0, 0, 0};
      static float f = 0.0f;
      ImGui::ColorEdit3("color", col);
      ImGui::InputFloat("intensity", &f);
    }
    ImGui::End();
    checkErrors("imgui");

    glViewport(0, 0, window->getWidth(), window->getHeight());
    glClearColor(0, 0.1, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    checkErrors("clear");

    ImGui::Render();

    checkErrors("im render");

    window->endRendering();
  }

  ImGui_ImplBtGui_Shutdown();
  delete window;

  return EXIT_SUCCESS;
}
