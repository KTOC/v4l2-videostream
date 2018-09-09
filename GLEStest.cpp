#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <KHR/khrplatform.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <drm_fourcc.h>
#include <linux/videodev2.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <string>
#include "v4l2.h"

extern char _binary_vshader_glsl_start[];
extern char _binary_vshader_glsl_end[];
extern char _binary_fshader_glsl_start[];
extern char _binary_fshader_glsl_end[];

int initializeEGL(Display *xdisp, Window &xwindow, EGLDisplay &display,
                  EGLContext &context, EGLSurface &surface) {
    display = eglGetDisplay(static_cast<EGLNativeDisplayType>(xdisp));
    if (display == EGL_NO_DISPLAY) {
        std::cerr << "Error eglGetDisplay." << std::endl;
        return -1;
    }
    if (!eglInitialize(display, nullptr, nullptr)) {
        std::cerr << "Error eglInitialize." << std::endl;
        return -1;
    }

    EGLint attr[] = {EGL_BUFFER_SIZE, 16, EGL_RENDERABLE_TYPE,
                     EGL_OPENGL_ES2_BIT, EGL_NONE};
    EGLConfig config = nullptr;
    EGLint numConfigs = 0;
    if (!eglChooseConfig(display, attr, &config, 1, &numConfigs)) {
        std::cerr << "Error eglChooseConfig." << std::endl;
        return -1;
    }
    if (numConfigs != 1) {
        std::cerr << "Error numConfigs." << std::endl;
        return -1;
    }

    surface = eglCreateWindowSurface(display, config, xwindow, nullptr);
    if (surface == EGL_NO_SURFACE) {
        std::cerr << "Error eglCreateWindowSurface. " << eglGetError()
                  << std::endl;
        return -1;
    }

    EGLint ctxattr[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctxattr);
    if (context == EGL_NO_CONTEXT) {
        std::cerr << "Error eglCreateContext. " << eglGetError() << std::endl;
        return -1;
    }
    eglMakeCurrent(display, surface, surface, context);

    return 0;
}

void destroyEGL(EGLDisplay &display, EGLContext &context, EGLSurface &surface) {
    eglDestroyContext(display, context);
    eglDestroySurface(display, surface);
    eglTerminate(display);
}

GLuint loadShader(GLenum shaderType, const char *source) {
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_FALSE) {
        std::cerr << "Error glCompileShader." << std::endl;
        GLint log_length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
        GLsizei max_length;
        GLsizei length;
        GLchar log[max_length];
        glGetShaderInfoLog(shader, max_length, &length, log);
        std::cout << log << std::endl;
        exit(EXIT_FAILURE);
    }
    return shader;
}

GLuint createProgram(const char *vshader, const char *fshader) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vshader);
    GLuint fragShader = loadShader(GL_FRAGMENT_SHADER, fshader);
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    GLint linkStatus = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_FALSE) {
        std::cerr << "Error glLinkProgram." << std::endl;
        exit(EXIT_FAILURE);
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragShader);
    return program;
}

void deleteShaderProgram(GLuint shaderProgram) {
    glDeleteProgram(shaderProgram);
}

std::string imputShadersrc(const char *begin, const char *end) {
    std::string srcdata(begin, end);
    // std::cout << srcdata << "\n";
    return srcdata;
}

void mainloop(Display *xdisplay, EGLDisplay display, EGLSurface surface) {
    std::string vsd =
        imputShadersrc(_binary_vshader_glsl_start, _binary_vshader_glsl_end);
    std::string fsd =
        imputShadersrc(_binary_fshader_glsl_start, _binary_fshader_glsl_end);
    const char *vshader = vsd.c_str();
    const char *fshader = fsd.c_str();

    GLuint program = createProgram(vshader, fshader);
    glUseProgram(program);

    open_device();
    list_formats();
    struct v4l2_format fmt = init_device();
    start_capturing();
    unsigned char *ImageBuffer = NULL;
    ImageBuffer = snapFrame();
    GLuint position = glGetAttribLocation(program, "position");
    glEnableVertexAttribArray(position);
    GLuint texcoord = glGetAttribLocation(program, "texcoord");
    glEnableVertexAttribArray(texcoord);

    GLuint textures = 0;
    glGenTextures(1, &textures);
    glBindTexture(GL_TEXTURE_2D, textures);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    printf("%p\n", ImageBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fmt.fmt.pix.width / 2,
                 fmt.fmt.pix.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, ImageBuffer);
    glActiveTexture(GL_TEXTURE0);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    const float vertices[] = {-1.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f,
                              1.0f,  1.0f, 0.0f, 1.0f,  -1.0f, 0.0f};
    const float texcoords[] = {0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f};
    while (true) {
        if (ImageBuffer == NULL)
            break;
        XPending(xdisplay);
        glClearColor(0.25f, 0.25f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glVertexAttribPointer(texcoord, 2, GL_FLOAT, GL_FALSE, 0, texcoords);
        glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, 0, vertices);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        eglSwapBuffers(display, surface);

        ImageBuffer = snapFrame();
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, fmt.fmt.pix.width / 2,
                        fmt.fmt.pix.height, GL_RGBA, GL_UNSIGNED_BYTE,
                        ImageBuffer);
    }
    stop_capturing();
    uninit_device();
    close_device();
    deleteShaderProgram(program);
}

int main() {
    Display *xdisplay = XOpenDisplay(nullptr);
    if (xdisplay == nullptr) {
        std::cerr << "Error XOpenDisplay." << std::endl;
        exit(EXIT_FAILURE);
    }

    Window xwindow = XCreateSimpleWindow(
        xdisplay, DefaultRootWindow(xdisplay), 100, 100, 640, 480, 1,
        BlackPixel(xdisplay, 0), WhitePixel(xdisplay, 0));

    XMapWindow(xdisplay, xwindow);

    EGLDisplay display = nullptr;
    EGLContext context = nullptr;
    EGLSurface surface = nullptr;
    if (initializeEGL(xdisplay, xwindow, display, context, surface) < 0) {
        std::cerr << "Error initializeEGL." << std::endl;
        exit(EXIT_FAILURE);
    }

    mainloop(xdisplay, display, surface);
    destroyEGL(display, context, surface);
    XDestroyWindow(xdisplay, xwindow);
    XCloseDisplay(xdisplay);

    return 0;
}
