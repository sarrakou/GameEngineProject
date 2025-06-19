file(GLOB_RECURSE GAME_SRC src/*.cpp)

add_executable(Game ${GAME_SRC})

target_link_libraries(Game PRIVATE Engine)
target_include_directories(Game PRIVATE ../Engine/include)
 