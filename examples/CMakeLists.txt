add_executable(network_client network_client.c ../network.c)
add_executable(network_server network_server.c ../network.c)

target_include_directories(network_client PUBLIC ..)
target_include_directories(network_server PUBLIC ..)
