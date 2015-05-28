cmake_minimum_required(VERSION 2.8.12)

project(ksbinit)

find_package(ECM 1.9.0 REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_MODULE_PATH})

include(KDEInstallDirs)
include(KDECompilerSettings)
include(KDECMakeSettings)

set(QT5_VERSION "5.3.0")
find_package(Qt5 ${QT5_VERSION} REQUIRED COMPONENTS Core DBus)

add_subdirectory(src)
