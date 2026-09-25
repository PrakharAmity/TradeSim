cmake_minimum_required(VERSION 3.16)
project(TradeSim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)
include_directories(include)

add_library(tradesim_core
    src/market_data.cpp
    src/portfolio.cpp
    src/single_trade_strategy.cpp
    src/unlimited_strategy.cpp
    src/limited_transaction_strategy.cpp
    src/fee_strategy.cpp
    src/cooldown_strategy.cpp
    src/policy_engine.cpp)
target_link_libraries(tradesim_core PUBLIC Threads::Threads)

add_executable(tradesim src/main.cpp)
target_link_libraries(tradesim PRIVATE tradesim_core Threads::Threads)
if(WIN32)
    target_link_libraries(tradesim PRIVATE ws2_32)
endif()

enable_testing()
add_subdirectory(tests)
