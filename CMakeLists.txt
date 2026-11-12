cmake_minimum_required(VERSION 3.20)

project(RichTerm
    VERSION 0.2.0
    DESCRIPTION "A modular C++ terminal presentation library inspired by Rich and ColormaCpp"
    LANGUAGES CXX)

include(GNUInstallDirs)
find_package(Threads REQUIRED)
include(CMakePackageConfigHelpers)

option(RICHTERM_BUILD_EXAMPLES "Build RichTerm examples" ON)
option(RICHTERM_ENABLE_WARNINGS "Enable strict compiler warnings" ON)
option(RICHTERM_BUILD_TESTS "Build and register unit tests" ON)

add_library(richterm INTERFACE)
add_library(RichTerm::RichTerm ALIAS richterm)
set_target_properties(richterm PROPERTIES EXPORT_NAME RichTerm)
target_compile_features(richterm INTERFACE cxx_std_20)
target_link_libraries(richterm INTERFACE Threads::Threads)
if(MSVC)
    # Public headers contain UTF-8 box-drawing glyphs used by Table and Progress.
    target_compile_options(richterm INTERFACE /utf-8)
endif()
target_include_directories(richterm INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

if(RICHTERM_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(RICHTERM_BUILD_TESTS)
    include(CTest)
    if(BUILD_TESTING)
        add_executable(richterm_tests tests/test_richterm.cpp)
        target_link_libraries(richterm_tests PRIVATE RichTerm::RichTerm)
        if(RICHTERM_ENABLE_WARNINGS)
            if(MSVC)
                target_compile_options(richterm_tests PRIVATE /W4 /permissive- /EHsc)
            else()
                target_compile_options(richterm_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
            endif()
        endif()
        add_test(NAME richterm_tests COMMAND richterm_tests)
    endif()
endif()

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS richterm EXPORT RichTermTargets)
install(EXPORT RichTermTargets FILE RichTermTargets.cmake NAMESPACE RichTerm::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/RichTerm)

configure_package_config_file(cmake/RichTermConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/RichTermConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/RichTerm)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/RichTermConfigVersion.cmake
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/RichTermConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/RichTermConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/RichTerm)

message(STATUS "Configuring RichTerm ${PROJECT_VERSION}")
