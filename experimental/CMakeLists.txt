find_package(Threads REQUIRED)

add_executable(canard_tx canard_tx.cpp)
target_link_libraries(canard_tx PRIVATE canard Threads::Threads)

add_executable(canard_rx canard_rx.cpp)
target_link_libraries(canard_rx PRIVATE canard Threads::Threads)

add_executable(canard_server canard_server.cpp)
target_link_libraries(canard_server PRIVATE canard Threads::Threads)

add_executable(canard_client canard_client.cpp)
target_link_libraries(canard_client PRIVATE canard Threads::Threads)
