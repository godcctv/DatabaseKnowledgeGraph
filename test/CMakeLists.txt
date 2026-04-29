# 让测试程序也能找到 src 目录下的头文件
include_directories(${CMAKE_SOURCE_DIR}/src)

# 定义测试可执行程序
add_executable(UnitTests tst_main.cpp)

# 链接我们在 src 里打包好的 CoreLib，以及 Qt 的 Test 模块
target_link_libraries(UnitTests PRIVATE
        CoreLib
        Qt5::Test
)

# 注册给 CTest
add_test(NAME AllUnitTests COMMAND UnitTests)