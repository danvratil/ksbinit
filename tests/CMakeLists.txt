include_directories(${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})

set(CMAKE_EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
set(ksandworldTest_SRCS ksandworldTest.cpp)

qt4_automoc(${ksandworldTest_SRCS})
add_executable(ksandworldTest ${ksandworldTest_SRCS})
add_test(ksandworldTest ksandworldTest)
target_link_libraries(ksandworldTest ${QT_QTCORE_LIBRARY} ${QT_QTTEST_LIBRARY})
