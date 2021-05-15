cmake_minimum_required(VERSION 3.1.3)  # Least supporting CMAKE_CXX_STANDARD
###############################################################################

project(albert VERSION 0.18.0)


###############################################################################

include(GNUInstallDirs)

################################# GENERAL #####################################

add_definitions(-DPROJECT_VERSION="${PROJECT_VERSION}")

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Set standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set additional compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic") #-Wconversion -Wunused
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DQT_QML_DEBUG")

# Color make output
SET(CMAKE_COLOR_MAKEFILE ON)

# Put the binaries in dedicated toplevel directories
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Set the RPATH for the library lookup
set(CMAKE_INSTALL_RPATH "$ORIGIN/../")

################################# LIBRARIES ###################################

add_subdirectory(lib/globalshortcut)
add_subdirectory(lib/xdg)

################################ DEPENDENCIES #################################

find_package(Qt5 5.5.0
    COMPONENTS
        Gui
        Charts
        Concurrent
        Network
        Sql
        Widgets
        X11Extras
)

##################################### APP #####################################

set(TARGET_NAME_LIB ${PROJECT_NAME}-lib)

file(GLOB_RECURSE LIB_SRC include/* src/lib/*)
configure_file(
    "${PROJECT_SOURCE_DIR}/include/albert/config.h.in"
    "${PROJECT_SOURCE_DIR}/include/albert/config.h"
    @ONLY
)

add_library(${TARGET_NAME_LIB} SHARED ${LIB_SRC})
add_library(albert::lib ALIAS ${TARGET_NAME_LIB})

set_target_properties(${TARGET_NAME_LIB} PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}
    INSTALL_RPATH "$ORIGIN"
)

target_compile_definitions(${TARGET_NAME_LIB} PRIVATE -DCORE) #export branch

target_include_directories(${TARGET_NAME_LIB}
    PUBLIC
        include
    PRIVATE
        include/albert
        include/albert/util
        src/lib
)

target_link_libraries(${TARGET_NAME_LIB}
    PRIVATE
        Qt5::Gui
        Qt5::Sql
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

endif()

##################################### LIB #####################################

set(TARGET_NAME_BIN ${PROJECT_NAME}-bin)

file(GLOB_RECURSE BIN_SRC src/app/*)

add_executable(${TARGET_NAME_BIN} ${BIN_SRC} resources.qrc)

set_target_properties(${TARGET_NAME_BIN} PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}
    INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}/albert/"  # Set the RPATH for the library lookup
)

target_include_directories(${TARGET_NAME_BIN}
    PRIVATE
        src/app
        src/app/settingswidget
)

if (DEFINED CMAKE_LIBRARY_ARCHITECTURE)
    target_compile_definitions(${TARGET_NAME_BIN} PRIVATE -DMULTIARCH_TUPLE="${CMAKE_LIBRARY_ARCHITECTURE}")
endif()

target_link_libraries(${TARGET_NAME_BIN}
    PRIVATE
        Qt5::Charts
        Qt5::Concurrent
        Qt5::Network
        Qt5::Sql
        Qt5::Widgets
        albert::lib
        globalshortcut
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    target_link_libraries(${TARGET_NAME_BIN}
        PRIVATE
            Qt5::X11Extras
            xdg
    )
endif()

################################### INSTALL ###################################

install(TARGETS ${TARGET_NAME_LIB} ${TARGET_NAME_BIN}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/albert
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Do platform specific post target stuff
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # Install desktop entry and icon
   install(FILES resources/albert.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications )
   install(FILES resources/icons/albert.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
endif()

############################### FrontendPlugins ###############################

option(BUILD_WIDGETBOXMODEL "Build the frontend" ON)
if (BUILD_WIDGETBOXMODEL)
    add_subdirectory(plugins/widgetboxmodel)
endif()

option(BUILD_QMLBOXMODEL "Build the frontend" ON)
if (BUILD_QMLBOXMODEL)
    add_subdirectory(plugins/qmlboxmodel)
endif()

############################### ExtensionPlugins ##############################

option(BUILD_APPLICATIONS "Build this plugin." ON)
if (BUILD_APPLICATIONS)
    add_subdirectory(plugins/applications)
endif()

option(BUILD_CALCULATOR "Build this plugin." ON)
if (BUILD_CALCULATOR)
    add_subdirectory(plugins/calculator)
endif()

option(BUILD_CHROMEBOOKMARKS "Build this plugin." ON)
if (BUILD_CHROMEBOOKMARKS)
    add_subdirectory(plugins/chromium)
endif()

option(BUILD_FILES "Build this plugin." ON)
if (BUILD_FILES)
    add_subdirectory(plugins/files)
endif()

option(BUILD_FIREFOXBOOKMARKS "Build this plugin." ON)
if (BUILD_FIREFOXBOOKMARKS)
    add_subdirectory(plugins/firefoxbookmarks)
endif()

option(BUILD_HASHGENERATOR "Build this plugin." ON)
if (BUILD_HASHGENERATOR)
    add_subdirectory(plugins/hashgenerator)
endif()

option(BUILD_MPRIS "Build this plugin." ON)
if (BUILD_MPRIS)
    add_subdirectory(plugins/mpris)
endif()

option(BUILD_PYTHON "Build this plugin." ON)
if (BUILD_PYTHON)
    add_subdirectory(plugins/python)
endif()

option(BUILD_SNIPPETS "Build this plugin." ON)
if (BUILD_SNIPPETS)
    add_subdirectory(plugins/snippets)
endif()

option(BUILD_SSH "Build this plugin." ON)
if (BUILD_SSH)
    add_subdirectory(plugins/ssh)
endif()

option(BUILD_SYSTEM "Build this plugin." ON)
if (BUILD_SYSTEM)
    add_subdirectory(plugins/system)
endif()

option(BUILD_TERMINAL "Build this plugin." ON)
if (BUILD_TERMINAL)
    add_subdirectory(plugins/terminal)
endif()

option(BUILD_VIRTUALBOX "Build this plugin." ON)
if (BUILD_VIRTUALBOX)
    add_subdirectory(plugins/virtualbox)
endif()

option(BUILD_WEBSEARCH "Build this plugin." ON)
if (BUILD_WEBSEARCH)
    add_subdirectory(plugins/websearch)
endif()

###############################################################################

option(BUILD_APITEST "Build this plugin." OFF)
if (BUILD_APITEST)
    add_subdirectory(plugins/apitest)
endif()

###############################################################################






