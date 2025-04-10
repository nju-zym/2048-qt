cmake_minimum_required(VERSION 3.16)

project(2048-qt VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# 添加include路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# 分类源文件
set(UI_SOURCES
    ui/mainwindow.h
    ui/mainwindow.cpp
    ui/mainwindow.ui
)

set(CORE_SOURCES
    core/board.h
    core/board.cpp
)

set(AI_SOURCES
    ai/autoplayer.h
    ai/autoplayer.cpp
    ai/aiworker.cpp
    ai/bitboard.h
    ai/bitboard.cpp
    ai/expectimax.h
    ai/expectimax.cpp
    ai/evaluation/evaluation.h
    ai/evaluation/evaluation.cpp
    ai/evaluation/monotonicity.h
    ai/evaluation/monotonicity.cpp
    ai/evaluation/smoothness.h
    ai/evaluation/smoothness.cpp
    ai/evaluation/snake.h
    ai/evaluation/snake.cpp
    ai/evaluation/tile.h
    ai/evaluation/tile.cpp
    ai/evaluation/merge.h
    ai/evaluation/merge.cpp
)

set(UTILS_SOURCES
    utils/animation.h
    utils/animation.cpp
)

set(PROJECT_SOURCES
    main.cpp
    ${UI_SOURCES}
    ${CORE_SOURCES}
    ${AI_SOURCES}
    ${UTILS_SOURCES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(2048-qt
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )

# Define target properties for Android with Qt 6 as:
# set_property(TARGET 2048-qt APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
# ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(2048-qt SHARED
            ${PROJECT_SOURCES}
        )

    # Define properties for Android with Qt 5 after find_package() calls as:
    # set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(2048-qt
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(2048-qt PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.2048-qt)
endif()

set_target_properties(2048-qt PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS 2048-qt
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(2048-qt)
endif()
