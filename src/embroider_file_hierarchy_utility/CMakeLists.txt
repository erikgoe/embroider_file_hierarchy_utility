cmake_minimum_required(VERSION 3.21)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)

set(ARTIFACT_NAME ${PROJECT_NAME})

# add files
set(TARGET_SOURCE_FILES
    main.cpp
)

add_executable(${ARTIFACT_NAME}
    ${TARGET_SOURCE_FILES}
)

# PCH
target_precompile_headers(${ARTIFACT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include/${PROJECT_DIR_NAME}/pch.hpp
)
set_property(SOURCE main.cpp PROPERTY SKIP_PRECOMPILE_HEADERS ON)

# includes
target_include_directories(${ARTIFACT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

# linking
if(WIN32)
    set(TARGET_LIBS )
else()
    set(TARGET_LIBS )
endif()

target_link_libraries(${ARTIFACT_NAME}
    ${TARGET_LIBS} 
)
