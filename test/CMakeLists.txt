find_package (Boost COMPONENTS system filesystem unit_test_framework REQUIRED)
include_directories (${CMAKE_SOURCE_DIR}/src
                     ${Boost_INCLUDE_DIRS}
                     )
add_definitions (-DBOOST_TEST_DYN_LINK)
add_executable (order_book_test order_book_test.cpp)
target_link_libraries (order_book_test
                        OrderBook
                        ${Boost_FILESYSTEM_LIBRARY}
                        ${Boost_SYSTEM_LIBRARY}
                        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
                        ${CMAKE_THREAD_LIBS_INIT}
                       )
ADD_TEST(NAME UnitTests COMMAND order_book_test)
