#add_executable(filetransfer_download download.cc)
#target_link_libraries(filetransfer_download tmuduo_net)
#
#add_executable(filetransfer_download2 download2.cc)
#target_link_libraries(filetransfer_download2 tmuduo_net)

add_executable(filetransfer_download3 download3.cc)
target_link_libraries(filetransfer_download3 tmuduo_net)

add_executable(Filetransfer_server Filetransfer_server.cc)
target_link_libraries(Filetransfer_server tmuduo_net)

