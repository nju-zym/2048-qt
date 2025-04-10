cmake_minimum_required(VERSION 3.16)

project(2048-qt VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Concurrent)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Concurrent)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
)

set(CORE_SOURCES
        core/GameBoard.cpp
        core/GameBoard.h
        core/GameState.cpp
        core/GameState.h
)

set(UI_SOURCES
        ui/GameView.cpp
        ui/GameView.h
        ui/TileView.cpp
        ui/TileView.h
        ui/AIConfigDialog.cpp
        ui/AIConfigDialog.h
)

set(UTILS_SOURCES
        utils/StyleManager.cpp
        utils/StyleManager.h
        utils/AnimationManager.cpp
        utils/AnimationManager.h
        utils/GameController.cpp
        utils/GameController.h
)

# 检查是否存在预编译的查找表
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/ai/BitBoardTables.cpp")
    message(STATUS "使用预编译的BitBoard查找表")
    set(USE_PRECOMPILED_TABLES ON)
    add_compile_definitions(USE_PRECOMPILED_TABLES)
else()
    message(STATUS "预编译的BitBoard查找表不存在，将在运行时计算")
    set(USE_PRECOMPILED_TABLES OFF)
endif()

set(AI_SOURCES
        ai/AIInterface.h
        ai/BitBoard.cpp
        ai/BitBoard.h
        ai/BitBoardInitializer.cpp
        ai/BitBoardInitializer.h
        ai/GenerateTables.cpp
        ai/ExpectimaxAI.cpp
        ai/ExpectimaxAI.h
        ai/ExpectimaxWorker.cpp
        ai/ExpectimaxWorker.h
        ai/ParallelExpectimaxAI.cpp
        ai/ParallelExpectimaxAI.h
        ai/ParallelExpectimaxWorker.cpp
        ai/ParallelExpectimaxWorker.h
        ai/evaluation/MonotonicityEval.cpp
        ai/evaluation/MonotonicityEval.h
        ai/evaluation/SmoothnessEval.cpp
        ai/evaluation/SmoothnessEval.h
        ai/evaluation/FreeTilesEval.cpp
        ai/evaluation/FreeTilesEval.h
        ai/evaluation/MergeEval.cpp
        ai/evaluation/MergeEval.h
        ai/evaluation/TilePlacementEval.cpp
        ai/evaluation/TilePlacementEval.h
        ai/evaluation/EnhancedEval.cpp
        ai/evaluation/EnhancedEval.h
        ai/evaluation/CornerStrategyEval.cpp
        ai/evaluation/CornerStrategyEval.h
        ai/evaluation/LargeNumbersConnectionEval.cpp
        ai/evaluation/LargeNumbersConnectionEval.h
        ai/evaluation/RiskEval.cpp
        ai/evaluation/RiskEval.h
)

# 如果存在预编译的查找表，添加到源文件列表中
if(USE_PRECOMPILED_TABLES)
    set(AI_SOURCES ${AI_SOURCES}
        ai/BitBoardTables.h
        ai/BitBoardTables.cpp
    )
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(2048-qt
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${CORE_SOURCES}
        ${UI_SOURCES}
        ${UTILS_SOURCES}
        ${AI_SOURCES}
    )

# Define target properties for Android with Qt 6 as:
# set_property(TARGET 2048-qt APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
# ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(2048-qt SHARED
            ${PROJECT_SOURCES}
            ${CORE_SOURCES}
            ${UI_SOURCES}
            ${UTILS_SOURCES}
            ${AI_SOURCES}
        )

    # Define properties for Android with Qt 5 after find_package() calls as:
    # set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(2048-qt
            ${PROJECT_SOURCES}
            ${CORE_SOURCES}
            ${UI_SOURCES}
            ${UTILS_SOURCES}
            ${AI_SOURCES}
        )
    endif()
endif()

target_link_libraries(2048-qt PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Concurrent)

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
