add_executable(ChatServer
    main.cpp chatserver.cpp chatservice.cpp usermodel.cpp
    offlinemessagemodel.cpp friendmodel.cpp groupmodel.cpp db/db.cpp redis.cpp
)
target_include_directories(ChatServer PRIVATE
    "${MUDUO_INCLUDE_DIR}" "${MYSQL_INCLUDE_DIR}" "${HIREDIS_INCLUDE_DIR}"
    "${PROJECT_SOURCE_DIR}/include" "${PROJECT_SOURCE_DIR}/thirdparty"
)
target_link_libraries(ChatServer PRIVATE
    "${HIREDIS_LIBRARY}" "${MYSQL_LIBRARY}" "${MUDUO_NET_LIBRARY}" "${MUDUO_BASE_LIBRARY}" Threads::Threads
)
