cmake_minimum_required(VERSION 3.0.0)
project (WebcamDrawing)

add_custom_command(OUTPUT vshader.o
  COMMAND objcopy --input binary --output elf64-x86-64 -B i386 vshader.glsl ${CMAKE_CURRENT_BINARY_DIR}/vshader.o
  DEPENDS vshader.glsl
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
add_custom_command(OUTPUT fshader.o
  COMMAND objcopy --input binary --output elf64-x86-64 -B i386 fshader.glsl ${CMAKE_CURRENT_BINARY_DIR}/fshader.o
  DEPENDS fshader.glsl
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )

set_source_files_properties(
  vshader.o
  PROPERTIES
  EXTERNAL_OBJECT true
  GENERATED true
  )
set_source_files_properties(
  fshader.o
  PROPERTIES
  EXTERNAL_OBJECT true
  GENERATED true
  )


find_package(PkgConfig REQUIRED)
pkg_check_modules(GLESv2 glesv2 REQUIRED)
pkg_check_modules(EGL egl REQUIRED)
pkg_check_modules(X11 x11 REQUIRED)
#pkg_check_modules(DRM drm REQUIRED)
#pkg_check_modules(LIBPNG libpng REQUIRED)
include_directories(
    ${GLESv2_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
    ${X11_INCLUDE_DIRS}
    #${LIBPNG_INCLUDE_DIRS}
)

link_directories(
    ${GLESv2_LIBRARY_DIRS}
    ${EGL_LIBRARY_DIRS}
    ${X11_LIBRARY_DIRS}
    #${LIBPNG_LIBRARY_DIRS}
)

set(LIBS ${LIBS}
    ${GLESv2_LIBRARIES}
    ${EGL_LIBRARIES}
    ${X11_LIBRARIES}
    #${LIBPNG_LIBRARIES}
)

file(GLOB SOURCES
  v4l2test2.cpp GLEStest.cpp
)
#GLOBは.oファイル生成前ファイル名を評価するため、.oはadd_executableで指定する
add_executable(${PROJECT_NAME} ${SOURCES} vshader.o fshader.o)
target_link_libraries(${PROJECT_NAME} ${LIBS})

add_definitions("-std=c++14")
add_definitions("-Wall")
add_definitions("-Wextra")
add_definitions("-g")
