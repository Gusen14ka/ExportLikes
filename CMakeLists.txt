cmake_minimum_required(VERSION 3.16)

# ————————————————————————————————
# 1) Path to your vcpkg toolchain (replace with your real path)
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
      CACHE FILEPATH "Vcpkg toolchain file")
endif()

# 2) Explicitly tell CMake where Zlib lives in vcpkg (x64-windows triplet)
set(ZLIB_ROOT       "C:/vcpkg/installed/x64-windows" CACHE PATH "Zlib root from vcpkg")
set(ZLIB_INCLUDE_DIR "${ZLIB_ROOT}/include"             CACHE PATH "Zlib include directory")
set(ZLIB_LIBRARY     "${ZLIB_ROOT}/lib/zlib.lib"         CACHE FILEPATH "Zlib library file")


set(OPENSSL_ROOT_DIR "C:/vcpkg/installed/x64-windows")

# 3) Standard project setup
project(ExportLikes VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt auto-tools
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

configure_file(
  ${CMAKE_SOURCE_DIR}/data/cacert.pem
  ${CMAKE_BINARY_DIR}/data/cacert.pem
  COPYONLY
)

# ————————————————————————————————
# Find Qt (Widgets + LinguistTools)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)

add_compile_definitions(
    BOOST_ASIO_HAS_CO_AWAIT
    BOOST_ASIO_USE_TS_EXECUTOR_AS_DEFAULT
)

# Find Boost (system, asio)
find_package(Boost REQUIRED COMPONENTS system asio beast serialization)

# Find CURL (will internally depend on ZLIB, using our hints above)
find_package(CURL REQUIRED)

find_package(OpenSSL REQUIRED)

# ————————————————————————————————
# Sources
#set(TS_FILES
#    ExportLikes_ru_RU.ts
#)

set(PROJECT_SOURCES
    main.cpp
    src/exportlikes.cpp
    include/exportlikes.hpp
    forms/exportlikes.ui
    #${TS_FILES}
)

# ————————————————————————————————
# Create executable
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(ExportLikes
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        include/Client.hpp
        src/Client.cpp
        src/HelperPKCE.cpp
        include/SpotifyClient.hpp
        src/SpotifyClient.cpp
        include/HelperPKCE.hpp
        include/QtSpotifyClient.hpp
        src/QtSpotifyClient.cpp
        src/AuthorizationServer.cpp
        include/AuthorizationServer.hpp
        src/SpotifyIoService.cpp
        include/SpotifyIoService.hpp

    )
    target_include_directories(ExportLikes PRIVATE include)
    #qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(ExportLikes SHARED ${PROJECT_SOURCES})
    else()
        add_executable(ExportLikes ${PROJECT_SOURCES})
    endif()
    #qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

set_target_properties(ExportLikes PROPERTIES
    AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/forms"
)


# Link libraries
target_link_libraries(ExportLikes PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Boost::system
    Boost::asio
    Boost::beast
    Boost::serialization
    CURL::libcurl
    OpenSSL::SSL
)

add_custom_command(TARGET ExportLikes POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/scripts"
        "$<TARGET_FILE_DIR:ExportLikes>/scripts"
)

# Добавляем /bigobj только для MSVC
if (MSVC)
    target_compile_options(ExportLikes PRIVATE /bigobj)
endif()

# ————————————————————————————————
# Mac/iOS bundle properties (you can skip on Windows)
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.ExportLikes)
endif()
set_target_properties(ExportLikes PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# Installation rules
include(GNUInstallDirs)
install(TARGETS ExportLikes
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Finalize for Qt6
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(ExportLikes)
endif()

