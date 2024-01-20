
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <android/log.h>
#include <jni.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../../../../common.h"
#include "../../../../../platform_main.h"
#include "../../../../../android_platform.cpp"
#include "../../../../../common.cpp"
#include "../../../../../main.cpp"

#define LOG_TAG "libgl2jni"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

Arena * arena;
U8 *buffer_str_arr[BUFFER_MAX];


static void printGLString(const char* name, GLenum s) {
  const char* v = (const char*)glGetString(s);
  LOGI("GL %s = %s\n", name, v);
}

static void checkGlError(const char* op) {
  for (GLint error = glGetError(); error; error = glGetError()) {
    LOGI("after %s() glError (0x%x)\n", op, error);
  }
}

auto gVertexShader =
    "attribute vec4 vPosition;\n"
    "void main() {\n"
    "  gl_Position = vPosition;\n"
    "  gl_Position.xy *= 2.0;\n"
    "  gl_Position.x /= 1000.0;\n"
    "  gl_Position.y /= 600.0;\n"
    "  gl_Position.xy -= 1.0;\n"
    "  gl_Position.y *= -1.0;\n"
    "}\n";

auto gFragmentShader =
    "precision mediump float;\n"
    "void main() {\n"
    "  gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
    "}\n";

GLuint loadShader(GLenum shaderType, const char* pSource) {
  GLuint shader = glCreateShader(shaderType);
  if (shader) {
    glShaderSource(shader, 1, &pSource, NULL);
    glCompileShader(shader);
    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
      GLint infoLen = 0;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
      if (infoLen) {
        char* buf = (char*)malloc(infoLen);
        if (buf) {
          glGetShaderInfoLog(shader, infoLen, NULL, buf);
          LOGE("Could not compile shader %d:\n%s\n", shaderType, buf);
          free(buf);
        }
        glDeleteShader(shader);
        shader = 0;
      }
    }
  }
  return shader;
}

GLuint createProgram(const char* pVertexSource, const char* pFragmentSource) {
  GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
  if (!vertexShader) {
    return 0;
  }

  GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
  if (!pixelShader) {
    return 0;
  }

  GLuint program = glCreateProgram();
  if (program) {
    glAttachShader(program, vertexShader);
    checkGlError("glAttachShader");
    glAttachShader(program, pixelShader);
    checkGlError("glAttachShader");
    glLinkProgram(program);
    GLint linkStatus = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE) {
      GLint bufLength = 0;
      glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
      if (bufLength) {
        char* buf = (char*)malloc(bufLength);
        if (buf) {
          glGetProgramInfoLog(program, bufLength, NULL, buf);
          LOGE("Could not link program:\n%s\n", buf);
          free(buf);
        }
      }
      glDeleteProgram(program);
      program = 0;
    }
  }
  return program;
}

GLuint gProgram;
GLuint gvPositionHandle;




extern "C" {
JNIEXPORT void JNICALL Java_com_seth_triangle_GL2JNILib_init(JNIEnv* env,
                                                              jobject obj,
                                                              jint width,
                                                              jint height);
JNIEXPORT void JNICALL Java_com_seth_triangle_GL2JNILib_step(JNIEnv* env,
                                                              jobject obj);
};


JNIEXPORT void JNICALL Java_com_seth_triangle_GL2JNILib_init(JNIEnv* env,
                                                              jobject obj,
                                                              jint width,
                                                              jint height) {

    arena = arena_init();


    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);

    LOGI("setupGraphics(%d, %d)", width, height);
    gProgram = createProgram(gVertexShader, gFragmentShader);
    if (!gProgram) {
      LOGE("Could not create program.");
      return;
    }
    gvPositionHandle = glGetAttribLocation(gProgram, "vPosition");
    checkGlError("glGetAttribLocation");
    LOGI("glGetAttribLocation(\"vPosition\") = %d\n", gvPositionHandle);
    glViewport(0, 0, width, height);
    checkGlError("glViewport");

    return;
}

JNIEXPORT void JNICALL Java_com_seth_triangle_GL2JNILib_step(JNIEnv* env,
                                                              jobject obj) {
  static float grey;
  grey = 0.0f;
  glClearColor(grey, grey, grey, 1.0f);
  checkGlError("glClearColor");
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
  checkGlError("glClear");

  glUseProgram(gProgram);
  checkGlError("glUseProgram");




  game_loop();
}
// seth convert this to andorid
void * _pVertexPositionsBuffer[BUFFER_MAX];

void platform_draw_triangle(String * id_str, V2F32 p1, V2F32 p2, V2F32 p3, V3F32 color) {
    LOGI("test");



    //
    // get hash_idx - seth - this can probably be compressed
    //
    S32 hash_idx = (S32)hash_key(id_str);
    S32 hash_idx_start = hash_idx;
    for (;;) {
        if (!_pVertexPositionsBuffer[hash_idx]) {
            //string_print(id_str);
            debug_hash("id placed in bucket %d\n", hash_idx);
            break;
        }
        if (string_compare((String *)buffer_str_arr[hash_idx], id_str)) {
            //string_print(id_str);
            debug_hash("id found in bucket %d\n", hash_idx);
            break;
        }
        //string_print(id_str);
        debug_hash("id can't be placed in bucket %d, moving on\n",  hash_idx);
        hash_idx++;
        // wrap around
        if (hash_idx >= BUFFER_MAX) {
            debug_hash("at the end of list starting over\n");
            hash_idx = 0;
        }
        if (hash_idx == hash_idx_start) {
            debug_hash("Scanned all possibilities, couldn't find place for buffer, increase BUFFER_MAX\n");

            debug_hash("id: ");
            //string_print(id_str);
            debug_hash("buffer_str_arr:\n");
            for (S32 idx = 0; idx < BUFFER_MAX; ++idx) {
                debug_hash("%d:\t", idx);
                //string_print(buffer_str_arr[idx]);
            }
            // seth andd this bakcassert(0);
            // print contents of buffer_str_arr
        }
    }

    S32 buffer_idx = hash_idx;


    const GLfloat gTriangleVertices[] = {
            p1.x, p1.y,
            p2.x, p2.y,
            p3.x, p3.y};



    glVertexAttribPointer(gvPositionHandle, 2, GL_FLOAT, GL_FALSE, 0,
                          gTriangleVertices);
    checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gvPositionHandle);
    checkGlError("glEnableVertexAttribArray");
    glDrawArrays(GL_TRIANGLES, 0, 3);
    checkGlError("glDrawArrays");


}