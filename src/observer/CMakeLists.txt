# 输出当前源码目录路径信息，用于调试和确认路径设置
MESSAGE(STATUS "This is CMAKE_CURRENT_SOURCE_DIR dir " ${CMAKE_CURRENT_SOURCE_DIR})

# 将当前源码目录添加到包含路径，确保头文件可以被正确引用
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

# 查找并加载必要的依赖包：FLEX (词法分析器生成器)和BISON (语法分析器生成器)
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

# 设置解析器工作目录和相关文件路径
set(PARSER_WORKDIR ${CMAKE_CURRENT_SOURCE_DIR}/sql/parser)  # 解析器源码所在目录
set(LEXER_SOURCES ${PARSER_WORKDIR}/lex_sql.cpp)            # 生成的词法分析器源码
set(LEXER_HEADERS ${PARSER_WORKDIR}/lex_sql.h)              # 生成的词法分析器头文件
set(PARSER_SOURCES ${PARSER_WORKDIR}/yacc_sql.cpp)          # 生成的语法分析器源码
set(PARSER_HEADERS ${PARSER_WORKDIR}/yacc_sql.hpp)          # 生成的语法分析器头文件

# 使用FLEX生成词法分析器
flex_target(lex ${PARSER_WORKDIR}/lex_sql.l "${LEXER_SOURCES}" DEFINES_FILE "${LEXER_HEADERS}")

# 使用BISON生成语法分析器
bison_target(yacc ${PARSER_WORKDIR}/yacc_sql.y "${PARSER_SOURCES}" DEFINES_FILE "${PARSER_HEADERS}")

# 添加FLEX和BISON之间的依赖关系，确保正确的构建顺序
add_flex_bison_dependency(lex yacc)

# 递归查找当前目录下所有的.cpp和.c源文件
FILE(GLOB_RECURSE ALL_SRC *.cpp *.c)

# 指定主程序源码文件
SET(MAIN_SRC ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
MESSAGE("MAIN SRC: " ${MAIN_SRC})

# 初始化库源码列表，添加词法分析器和语法分析器的源码
SET(LIB_SRC ${LIB_SRC} ${LEXER_SOURCES} ${PARSER_SOURCES})

# 遍历所有源码文件，将除主程序外的所有文件添加到库源码列表
FOREACH (F ${ALL_SRC})
    # 排除主程序源码
    IF (NOT ${F} STREQUAL ${MAIN_SRC})
        SET(LIB_SRC ${LIB_SRC} ${F})
        MESSAGE("Use " ${F})
    ENDIF()
ENDFOREACH (F)

# 设置Libevent库的静态链接标志
SET(LIBEVENT_STATIC_LINK TRUE)

# 查找并加载Libevent和jsoncpp库
FIND_PACKAGE(Libevent CONFIG REQUIRED)
FIND_PACKAGE(jsoncpp CONFIG REQUIRED)

# 设置需要链接的库列表
SET(LIBRARIES common pthread dl libevent::core libevent::pthreads)

# 指定目标文件位置
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)     # 可执行文件输出目录
MESSAGE("Binary directory:" ${EXECUTABLE_OUTPUT_PATH})
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)        # 静态库输出目录
MESSAGE("Archive directory:" ${LIBRARY_OUTPUT_PATH})

# 创建可执行文件observer，使用主程序源码
ADD_EXECUTABLE(observer ${MAIN_SRC})

# 链接observer可执行文件与静态库observer_static
TARGET_LINK_LIBRARIES(observer observer_static)

# 创建静态库observer_static，包含所有库源码
ADD_LIBRARY(observer_static STATIC ${LIB_SRC})

# 设置静态库的输出名称为observer
SET_TARGET_PROPERTIES(observer_static PROPERTIES OUTPUT_NAME observer)

# 链接静态库与所需的依赖库
TARGET_LINK_LIBRARIES(observer_static ${LIBRARIES} oblsm)

# Target 必须在定义 ADD_EXECUTABLE 之后， programs 不受这个限制
# TARGETS和PROGRAMS 的默认权限是OWNER_EXECUTE, GROUP_EXECUTE, 和WORLD_EXECUTE，即755权限， programs 都是处理脚本类
# 类型分为RUNTIME／LIBRARY／ARCHIVE, prog
# 设置安装规则，指定可执行文件和静态库的安装位置
INSTALL(TARGETS observer observer_static 
    RUNTIME DESTINATION bin     # 可执行文件安装到bin目录
    ARCHIVE DESTINATION lib)    # 静态库安装到lib目录
