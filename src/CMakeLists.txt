# 定义一个SRC_LIST变量，存放该目录下所有的源文件
aux_source_directory(. SRC_LIST)
aux_source_directory(./db SRC_LIST)
aux_source_directory(./redis SRC_LIST)
aux_source_directory(./protobuf SRC_LIST)

# 指定生成可执行文件
add_executable(PickServer ${SRC_LIST})

# 指定可执行文件生成时需要链接的外部库
target_link_libraries(PickServer muduo_net muduo_base pthread mysqlclient hiredis protobuf)

# 指定可执行文件存放位置
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)