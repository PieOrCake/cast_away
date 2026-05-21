cmake_minimum_required(VERSION 3.20)

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

project(CastAway)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_CXX_FLAGS_RELEASE "-Os -DNDEBUG")

set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")
set(CMAKE_SHARED_LINKER_FLAGS "-static -static-libgcc -static-libstdc++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++ -DWIN32_LEAN_AND_MEAN")

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/lib)
include_directories(${CMAKE_SOURCE_DIR}/lib/imgui)
include_directories(${CMAKE_SOURCE_DIR}/lib/imgui/backends)
include_directories(${CMAKE_SOURCE_DIR}/lib/nlohmann)

set(IMGUI_SOURCES
    lib/imgui/imgui.cpp
    lib/imgui/imgui_draw.cpp
    lib/imgui/imgui_tables.cpp
    lib/imgui/imgui_widgets.cpp
)

set(SOURCES
    src/dllmain.cpp
    src/FishData.cpp
    src/FishRarity.cpp
    src/FishBonus.cpp
    src/FishTank.cpp
    src/HoleLocations.cpp
    src/TileCache.cpp
    src/MapPanel.cpp
    src/IconManager.cpp
    src/PriceCache.cpp
    src/AchievementTracker.cpp
    ${IMGUI_SOURCES}
)

add_library(CastAway SHARED ${SOURCES})
set_target_properties(CastAway PROPERTIES SUFFIX ".dll" PREFIX "")
target_link_libraries(CastAway gdi32 wininet shell32)
target_link_options(CastAway PRIVATE -Wl,--out-implib,CastAway.lib -Wl,--strip-all)
target_sources(CastAway PRIVATE ${CMAKE_SOURCE_DIR}/CastAway.def)
set_target_properties(CastAway PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_custom_command(TARGET CastAway POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:CastAway>/assets
    COMMENT "Copying CastAway assets next to DLL")
