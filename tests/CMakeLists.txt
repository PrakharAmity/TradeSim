add_executable(run_tests test_main.cpp)
target_link_libraries(run_tests PRIVATE tradesim_core Threads::Threads)
add_test(NAME tradesim_challenge_tests COMMAND run_tests)
