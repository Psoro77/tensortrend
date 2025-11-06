cmake_minimum_required(VERSION 3.16)

project(src VERSION 0.1 LANGUAGES CXX)

set(CMAKE_PREFIX_PATH "C:/Qt/6.10.0/msvc2022_64")

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Trouver Qt avec Widgets et Charts
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Charts)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Charts)

# Chemins XGBoost
set(XGBOOST_INCLUDE_DIR "C:/Users/DELL/xgboost-src-3.1.0/xgboost/include")
set(XGBOOST_LIB_DIR "C:/Users/DELL/xgboost-src-3.1.0/xgboost/lib")

# Fichiers sources du projet
set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    ../predictor/StockPredictor.cpp
    ../predictor/StockPredictor.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(src
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(src SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(src
            ${PROJECT_SOURCES}
        )
    endif()
endif()

# Include directories pour XGBoost
target_include_directories(src PRIVATE
    ${XGBOOST_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../predictor
)

# Lier les bibliothèques Qt
target_link_libraries(src PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Charts
)

# Lier XGBoost
target_link_directories(src PRIVATE ${XGBOOST_LIB_DIR})
target_link_libraries(src PRIVATE xgboost)

# Copier xgboost.dll à côté de l'exe après la compilation
add_custom_command(TARGET src POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/../predictor/xgboost.dll"
    $<TARGET_FILE_DIR:src>
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.src)
endif()

set_target_properties(src PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS src
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(src)
endif()
