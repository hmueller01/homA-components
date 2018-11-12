cmake_minimum_required(VERSION 3.6)

project(SML2MQTT
    LANGUAGES C CXX)
	
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    # CMAKE_INSTALL_PREFIX defaults to /usr/local, but it should go to /usr
    set(CMAKE_INSTALL_PREFIX /usr/local)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

# build types: None, Debug, Release, RelWithDebInfo, MinSizeRel
set(CMAKE_BUILD_TYPE Release)

# parts to build
option(OPTION_WITH_SYSTEMD "systemd support" ON)

# directories
include(GNUInstallDirs)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

# dependencies
find_package(LIBSML)

# install
install(
    FILES
        README.md
    DESTINATION ${CMAKE_INSTALL_DOCDIR})

# sub directories
add_subdirectory(src)
