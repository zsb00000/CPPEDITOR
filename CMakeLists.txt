cmake_minimum_required(VERSION 3.16)
project(CPPEDITOR VERSION 1.0.0 LANGUAGES CXX)

# ────────── C++ 标准 ──────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ────────── Qt6 配置 ──────────
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)

qt_standard_project_setup()

# ────────── AUTOMOC / AUTOUIC / AUTORCC ──────────
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ────────── 源文件 ──────────
file(GLOB_RECURSE SOURCES
    src/*.cpp
    src/*.h
)

# 排除可能混入的非源码
list(FILTER SOURCES EXCLUDE REGEX "build/")

# ────────── 可执行文件 ──────────
qt_add_executable(CPPEDITOR ${SOURCES})

# ────────── 包含路径 ──────────
target_include_directories(CPPEDITOR PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/filemanager
    ${CMAKE_SOURCE_DIR}/src/compilemanager
    ${CMAKE_SOURCE_DIR}/src/formatter
    ${CMAKE_SOURCE_DIR}/src/linter
    ${CMAKE_SOURCE_DIR}/src/judge
    ${CMAKE_SOURCE_DIR}/src/GDBmanager
    ${CMAKE_SOURCE_DIR}/src/ui
)

# ────────── 链接 Qt ──────────
target_link_libraries(CPPEDITOR PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
)

# ────────── 属性 ──────────
set_target_properties(CPPEDITOR PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

# ────────── 复制 DATA 目录到构建输出 ──────────
add_custom_command(TARGET CPPEDITOR POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/DATA"
    "$<TARGET_FILE_DIR:CPPEDITOR>/DATA"
    COMMENT "Copying DATA/ to output directory"
)

# ────────── 安装规则（可选） ──────────
install(TARGETS CPPEDITOR
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY DATA/
    DESTINATION ${CMAKE_INSTALL_BINDIR}/DATA
)