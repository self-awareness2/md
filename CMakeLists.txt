cmake_minimum_required(VERSION 3.25)

project(Marknote VERSION 0.1.0 LANGUAGES CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Quick QuickControls2 Widgets Gui Test)

# cmark-gfm is pinned so parser behavior remains reproducible across platforms.
# Disable its auxiliary targets; Marknote only consumes the two static libraries.
set(CMARK_TESTS OFF CACHE BOOL "" FORCE)
set(CMARK_SHARED OFF CACHE BOOL "" FORCE)
set(CMARK_STATIC ON CACHE BOOL "" FORCE)
set(CMARK_LIB_FUZZER OFF CACHE BOOL "" FORCE)
FetchContent_Declare(cmark_gfm
    GIT_REPOSITORY https://github.com/github/cmark-gfm.git
    GIT_TAG 0.29.0.gfm.13
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(cmark_gfm)

add_library(marknote_markdown STATIC
    src/markdown/markdownrenderer.cpp
    src/markdown/markdownrenderer.h
    src/markdown/markdownsyntaxhighlighter.cpp
    src/markdown/markdownsyntaxhighlighter.h
)

target_include_directories(marknote_markdown
    PUBLIC src
    PRIVATE
        "${cmark_gfm_SOURCE_DIR}/src"
        "${cmark_gfm_SOURCE_DIR}/extensions"
        "${cmark_gfm_BINARY_DIR}/src"
)
target_link_libraries(marknote_markdown PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    libcmark-gfm_static
    libcmark-gfm-extensions_static
)

# A Qt installation must use the same compiler ABI as the application. CMake
# can otherwise accept MinGW package files in an MSVC build and fail much later
# with misleading QtCore linker errors.
if(MSVC AND Qt6_DIR MATCHES "[Mm]in[Gg][Ww]")
    message(FATAL_ERROR
        "The MSVC build selected a MinGW Qt installation: ${Qt6_DIR}. "
        "Set CMAKE_PREFIX_PATH to a Qt msvc2022_64 installation.")
endif()

if(MINGW AND Qt6_DIR MATCHES "[Mm][Ss][Vv][Cc]")
    message(FATAL_ERROR
        "The MinGW build selected an MSVC Qt installation: ${Qt6_DIR}. "
        "Set CMAKE_PREFIX_PATH to a Qt mingw_64 installation.")
endif()

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(marknote
    src/app/main.cpp
    src/app/applicationcontroller.cpp
    src/app/applicationcontroller.h
)

qt_add_qml_module(marknote
    URI Marknote
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
)

target_link_libraries(marknote PRIVATE
    marknote_markdown
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Gui
    Qt6::Widgets
)

target_compile_definitions(marknote PRIVATE
    MARKNOTE_VERSION="${PROJECT_VERSION}"
)

option(MARKNOTE_DEPLOY_QT
    "Copy the Qt runtime beside the application after a Windows build"
    ON)

if(WIN32 AND MARKNOTE_DEPLOY_QT)
    add_custom_command(TARGET marknote POST_BUILD
        COMMAND $<TARGET_FILE:Qt6::windeployqt>
            --qmldir "${CMAKE_CURRENT_SOURCE_DIR}/qml"
            --dir "$<TARGET_FILE_DIR:marknote>"
            "$<TARGET_FILE:marknote>"
        COMMENT "Deploying the Qt runtime beside Marknote"
        VERBATIM
        COMMAND_EXPAND_LISTS
    )
endif()

if(MSVC)
    # Qt checks __cplusplus when compiling generated QML sources. MSVC only
    # reports the selected standard correctly when /Zc:__cplusplus is enabled.
    target_compile_options(marknote PRIVATE /W4 /permissive- /Zc:__cplusplus)
else()
    target_compile_options(marknote PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS marknote
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

enable_testing()

qt_add_executable(marknote_smoke_test
    tests/unit/test_applicationcontroller.cpp
    src/app/applicationcontroller.cpp
    src/app/applicationcontroller.h
)

if(MSVC)
    target_compile_options(marknote_smoke_test PRIVATE /Zc:__cplusplus)
endif()

target_link_libraries(marknote_smoke_test PRIVATE
    marknote_markdown
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test
)

add_test(NAME marknote_smoke_test COMMAND marknote_smoke_test)

qt_add_executable(marknote_markdown_test
    tests/unit/test_markdownrenderer.cpp
)

target_link_libraries(marknote_markdown_test PRIVATE
    marknote_markdown
    Qt6::Test
)

if(MSVC)
    target_compile_options(marknote_markdown_test PRIVATE /Zc:__cplusplus)
endif()

add_test(NAME marknote_markdown_test COMMAND marknote_markdown_test)
