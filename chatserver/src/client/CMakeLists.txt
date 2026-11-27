add_executable(ChatClient main.cpp)
target_include_directories(ChatClient PRIVATE
    "${PROJECT_SOURCE_DIR}/include" "${PROJECT_SOURCE_DIR}/thirdparty"
)
target_link_libraries(ChatClient PRIVATE Threads::Threads)
