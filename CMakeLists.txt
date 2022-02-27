cmake_minimum_required(VERSION 3.20)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)

project(headtingrod CXX)

add_subdirectory(test)

include(FetchContent)
FetchContent_Declare(
    PowerManagement
    GIT_REPOSITORY https://github.com/HomeMaintenance/PowerManagement.git
    GIT_TAG main
)
FetchContent_MakeAvailable(PowerManagement)

add_library(
    HeatingRod
    HeatingRod.h
    HeatingRod.cpp
)
target_include_directories(HeatingRod PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(HeatingRod PUBLIC PowerManagement)

# target_compile_definitions(HeatingRod PRIVATE HEATINGROD_DEBUG=1)

if(PROJECT_IS_TOP_LEVEL)
    find_package(Doxygen
        REQUIRED dot
        OPTIONAL_COMPONENTS mscgen dia
    )
    set(DOXYGEN_EXTRACT_PRIVATE TRUE)
    set(DOXYGEN_WARN_IF_UNDOCUMENTED FALSE)
    set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C TRUE)
    set(DOXYGEN_EXCLUDE_PATTERNS */build/*)

    doxygen_add_docs(doxygen_target
        ${CMAKE_SOURCE_DIR}
        COMMENT "Run doxygen_target"
    )
endif()
