# 设置当前目录为头文件包含路径，这样在编译时可以直接包含当前目录下的头文件
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

# 递归查找当前目录及其子目录下所有的 .cpp 文件，并将它们的路径存储在变量 ALL_SRC 中
FILE(GLOB_RECURSE ALL_SRC  *.cpp)

# 注释说明库的类型
# SHARED：表示创建动态库
# STATIC：表示创建静态库
# 创建一个名为 common 的静态库，使用之前收集到的所有 .cpp 文件作为源文件
ADD_LIBRARY(common STATIC ${ALL_SRC} )

# 检查是否使用 musl libc，如果使用则需要手动链接 libexecinfo 库
IF(USE_MUSL_LIBC)
    # 输出提示信息，表明 musl libc 需要手动链接 libexecinfo 库
    MESSAGE(STATUS "musl libc need manually link libexecinfo")
    # 将 execinfo 库链接到 common 库上
    TARGET_LINK_LIBRARIES(common execinfo)
ENDIF(USE_MUSL_LIBC)

# 将 replxx 库链接到 common 库上
TARGET_LINK_LIBRARIES(common replxx::replxx)
# 输出提示信息，表明 common 库使用了 replxx 库
MESSAGE(STATUS "common uses replxx")

# 编译静态库时，自动会把同名的动态库给删除， 因此需要临时设置一下
# 设置 common 库的属性，CLEAN_DIRECT_OUTPUT 1 表示在清理构建时，保留不同类型库的输出文件
SET_TARGET_PROPERTIES(common PROPERTIES CLEAN_DIRECT_OUTPUT 1)

# 设置库的输出路径为项目构建目录下的 lib 子目录
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
