add_library(tg_fuse telegram_ops.cpp filesystem.cpp)
target_link_libraries(tg_fuse PRIVATE fuse3 telegram)
target_include_directories(tg_fuse PRIVATE ${PROJECT_SOURCE_DIR}/lib)