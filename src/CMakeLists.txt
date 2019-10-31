add_library (OrderBook order_book.cpp)
add_executable (order_book main.cpp)
target_link_libraries (order_book OrderBook ${CMAKE_THREAD_LIBS_INIT})
