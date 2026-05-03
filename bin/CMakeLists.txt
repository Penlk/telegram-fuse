add_executable(${PROJECT_NAME} main.cpp)
# target_link_libraries(${PROJECT_NAME} PRIVATE fuse3 cpr::cpr)
target_link_libraries(${PROJECT_NAME} PRIVATE fuse3 cpr::cpr telegram tg_fuse)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/lib)