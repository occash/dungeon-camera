cmake_minimum_required(VERSION 3.21.2)

project(dungeon-camera VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 COMPONENTS
    Core
    Gui
    Widgets
    Multimedia
    MultimediaWidgets
    SvgWidgets
    REQUIRED
)

set(HEADERS
    character.h
    image_formats.h
    mainwindow.h
    shared-memory-queue.h
    sharedmemoryqueue.h
    virtual_output.h    
    virtualoutput.h
)

set(SOURCES
    character.cpp
    main.cpp
    mainwindow.cpp
    shared-memory-queue.c
    sharedmemoryqueue.cpp
    virtualoutput.cpp
)

set(FORMS
    mainwindow.ui
    videooverlay.ui
)

set(RESOURCES
    mainwindow.qrc
)

add_executable(dungeon-camera
    ${HEADERS}
    ${SOURCES}
    ${RESOURCES}
)
target_include_directories(dungeon-camera PRIVATE src ../libyuv/include)
target_link_libraries(dungeon-camera
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    Qt6::SvgWidgets
    yuv
)

set_property(TARGET dungeon-camera PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set_property(TARGET dungeon-camera PROPERTY AUTOMOC ON)
set_property(TARGET dungeon-camera PROPERTY AUTORCC ON)
set_property(TARGET dungeon-camera PROPERTY AUTOUIC ON)
set_property(TARGET dungeon-camera PROPERTY AUTOUIC_SEARCH_PATHS "src")
set_property(TARGET dungeon-camera PROPERTY AUTOMOC_SOURCE_GROUP "moc")
set_property(TARGET dungeon-camera PROPERTY AUTORCC_SOURCE_GROUP "rcc")
set_property(TARGET dungeon-camera PROPERTY AUTOUIC_SOURCE_GROUP "uic")

if(MSVC)
    target_compile_options(dungeon-camera PRIVATE "/MP")
endif()