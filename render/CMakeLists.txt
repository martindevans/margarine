add_library(render INTERFACE)

target_sources(render INTERFACE
    dda.h
    dda.cpp
    hud.h
    hud.cpp
)

target_include_directories(render INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(render INTERFACE pico_base_headers)