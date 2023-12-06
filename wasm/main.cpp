#include <stdio.h>

#include <emscripten/emscripten.h>

#include <GL/glew.h>

#include <GLFW/glfw3.h>


#define assert(expr) if(!(expr)) { printf("%s:%d %s() %s\n",__FILE__,__LINE__, __func__, #expr); *(volatile int *)0 = 0; }

GLFWwindow* window;
GLuint vert_shader, frag_shader;
GLuint shader_programme;
GLuint vao;
const char* vertex_shader_str =
#ifdef __EMSCRIPTEN__
    "#version 300 es\n"
#else 
    "#version 330\n"
#endif
    "in vec4 a_position;"
    "void main () {"
    "  gl_Position = a_position;"
    "  gl_Position.xy *= 2.0f;"
    "  gl_Position.xy -= 1.0f;"
    "  gl_Position.y  *= -1.0f;"
    "}";

const char* fragment_shader_str =
#ifdef __EMSCRIPTEN__
    "#version 300 es\n"
#else 
    "#version 330\n"
#endif
    "precision highp float;"
    "out vec4 frag_color;"
    "void main() {"
    "  frag_color = vec4(5.0, 0.0, 0.5, 1.0);"
    "}";

GLfloat points[] = { 
    // triangle 1
    0.0f, 1.0f, 0.0f, 
    1.0f, 1.0f, 0.0f, 
    0.5f, 0.0f, 0.0f, 
};

void main_loop() {
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, 1);
    }
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ); 
    glUseProgram( shader_programme );
    glBindVertexArray( vao );
    glDrawArrays( GL_TRIANGLES, 0, 6 );
    glfwPollEvents();
    glfwSwapBuffers( window );
}

int main() {
    // init
    assert(glfwInit());
    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );
    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );

    window = glfwCreateWindow( 640, 480, "Hello Triangle", NULL, NULL );
    assert(window);
    glfwMakeContextCurrent( window );
    glfwWindowHint( GLFW_SAMPLES, 4 );

    glEnable( GL_DEPTH_TEST ); /* enable depth-testing */
    glDepthFunc( GL_LESS );

    // bindings
    GLuint vbo;
    glGenBuffers( 1, &vbo ); 
    glBindBuffer( GL_ARRAY_BUFFER, vbo );
    glBufferData( GL_ARRAY_BUFFER, 6* 3 * sizeof( GLfloat ), points, GL_STATIC_DRAW );
    glGenVertexArrays( 1, &vao );
    glBindVertexArray( vao );
    glBindBuffer( GL_ARRAY_BUFFER, vbo );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, NULL );
    glEnableVertexAttribArray( 0 );

    // shaders
    vert_shader = glCreateShader( GL_VERTEX_SHADER );
    glShaderSource( vert_shader, 1, &vertex_shader_str, NULL );
    glCompileShader( vert_shader );
    GLint shaderCompiled = 0;
    glGetShaderiv(vert_shader, GL_COMPILE_STATUS, &shaderCompiled);
    if (!shaderCompiled) {
        GLint maxLength;
        glGetShaderiv(vert_shader, GL_INFO_LOG_LENGTH, &maxLength);
        char errorLog[maxLength];
        glGetShaderInfoLog(vert_shader, maxLength, &maxLength, &errorLog[0]);
        printf("Vertical Shader failed to compile:\n%s\n", errorLog);
    }
    assert(shaderCompiled && "Vertex Shader Failed to Compile");

    frag_shader = glCreateShader( GL_FRAGMENT_SHADER );
    glShaderSource( frag_shader, 1, &fragment_shader_str, NULL );
    glCompileShader( frag_shader );

    glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &shaderCompiled);
    if (!shaderCompiled) {
        GLint maxLength;
        glGetShaderiv(frag_shader, GL_INFO_LOG_LENGTH, &maxLength);
        char errorLog[maxLength];
        glGetShaderInfoLog(frag_shader, maxLength, &maxLength, &errorLog[0]);
        printf("Fragment Shader failed to compile:\n%s\n", errorLog);
    }
    assert(shaderCompiled && "Fragment Shader Failed to Compile");

    shader_programme = glCreateProgram();
    glAttachShader( shader_programme, frag_shader );
    glAttachShader( shader_programme, vert_shader );
    glLinkProgram( shader_programme );

    glClearColor(0.1,0.2,0,0);

    #ifdef __EMSCRIPTEN__
        emscripten_set_main_loop(main_loop, 0, 1);
    #else
        while (!glfwWindowShouldClose(window)) {
            main_loop();
        }
    #endif

    return 0;
}
