cmake_minimum_required(VERSION 3.16)

project(qt_protoplanetary_view VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.2 COMPONENTS
    Core
    Gui
    Quick
    Quick3D
    Graphs
  REQUIRED
)

qt_add_executable(appqt_protoplanetary_view
    main.cpp
)

qt_add_qml_module(appqt_protoplanetary_view
    URI ProtoplanetaryQml
    VERSION 1.0
    QML_FILES
      main.qml
      Viewer.qml
    SOURCES
      objectmodel.h
      pointer-macros.h
      property-macros.h
      qppsengine.h
      qppsparticle.h
      qppsvec3f.h
      qppscomposition.h
      qppsdiskmodel.h
      qppszones.h
)


set_target_properties(appqt_protoplanetary_view PROPERTIES
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
)

target_compile_options(appqt_protoplanetary_view PRIVATE
    -freflection
    -fcontracts
)

target_link_options(appqt_protoplanetary_view PRIVATE
    -fcontracts
)

target_link_libraries(appqt_protoplanetary_view PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Quick3D
    Qt6::Graphs
    Threads::Threads
    atomic
    JosephsOddBuilder_Core
    JosephsOddBuilder_Threads
    JosephsOddBuilder_IO
    JosephsOddBuilder_Net
    JosephsOddBuilder_Crypto
    JosephsOddBuilder_Simd
    JosephsOddBuilder_Science
)

include(GNUInstallDirs)
install(TARGETS appqt_protoplanetary_view
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
