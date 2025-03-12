cmake_minimum_required(VERSION 3.10...3.27)

project(quicknote LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
find_package(ZLIB REQUIRED)

# QHotkey hinzufügen
include(FetchContent)
FetchContent_Declare(
    QHotkey
    GIT_REPOSITORY https://github.com/Skycoder42/QHotkey.git
    GIT_TAG 1.5.0
)
FetchContent_MakeAvailable(QHotkey)

# Quelldateien
set(SOURCES
    main.cpp
    editor.cpp
    translations.cpp
)

set(HEADERS
    editor.h
    translations.h
)

# Erstelle das ausführbare Programm
add_executable(quicknote ${SOURCES} ${HEADERS})
target_link_libraries(quicknote PRIVATE
    Qt6::Widgets
    Qt6::Network
    ZLIB::ZLIB
    QHotkey::QHotkey
)

# Include-Pfad für QHotkey hinzufügen
target_include_directories(quicknote PRIVATE
    ${qhotkey_SOURCE_DIR}
)

# Aktiviere automatische MOC-Verarbeitung
set_target_properties(quicknote PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
) 