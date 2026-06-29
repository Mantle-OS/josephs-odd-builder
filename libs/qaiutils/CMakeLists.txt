project(qt6-ai-utils
    VERSION 0.1
    LANGUAGES CXX
)

set(CMAKE_AUTOMOC ON)

set(QAIUTILS_HEADERS
    qt-ai-utils/pointer-macros.h
    qt-ai-utils/property-macros.h
    qt-ai-utils/qaiutils.h
    qt-ai-utils/qjsonapiclient.h
    qt-ai-utils/qconcurrentprocess.h
    qt-ai-utils/qdownloader.h
)

set(QAIUTILS_CXX
    qt-ai-utils/qaiutils.cpp
    qt-ai-utils/qjsonapiclient.cpp
    qt-ai-utils/qconcurrentprocess.cpp
    qt-ai-utils/qdownloader.cpp
)

qt_add_library(qt-ai-utils STATIC
    ${QAIUTILS_HEADERS}
    ${QAIUTILS_CXX}
)

target_include_directories(qt-ai-utils PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/qt-ai-utils>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qt-ai-utils>
)

target_link_libraries(qt-ai-utils PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Concurrent
)

#################################################################################

set(QMLAIUTILS_HEADERS
    qml-ai-utils/objectmodel.h
    qml-ai-utils/qmlstringlist.h
    qml-ai-utils/qaipath.h
    qml-ai-utils/qaipathing.h
)

set(QMLAIUTILS_CXX
    qml-ai-utils/qmlstringlist.cpp
    qml-ai-utils/qaipath.cpp
    qml-ai-utils/qaipathing.cpp
)

## Elements and what not for later
set(QMLAIUTILS_QML)

qt_add_library(qml-ai-utils STATIC)
qt_add_qml_module(qml-ai-utils
    URI QAi
    VERSION 1.0
    SOURCES
      ${QMLAIUTILS_HEADERS}
      ${QMLAIUTILS_CXX}
    # QML_FILES
    #   ${QMLAIUTILS_QML}
)

target_include_directories(qml-ai-utils PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/qml-ai-utils>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/qml-ai-utils>
)

target_link_libraries(qml-ai-utils PRIVATE
    Qt::Core
    Qt::Network
    Qt::Concurrent
    Qt::Gui
    Qt::Quick
    Qt::Qml
    qt-ai-utils
)
