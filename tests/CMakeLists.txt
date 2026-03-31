add_executable(unit_tests
    unit/test_make_claude_client.cpp
    unit/test_claude_llm_client.cpp
    unit/test_cli_args.cpp
)
target_link_libraries(unit_tests PRIVATE
    cpp_review_lib
    GTest::gtest_main
    GTest::gmock
)
target_include_directories(unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
gtest_discover_tests(unit_tests)

# ── Integration tests (optional) ──────────────────────────────────────────────
option(INTEGRATION_TESTS "Build integration tests (requires ANTHROPIC_API_KEY)" OFF)

if(INTEGRATION_TESTS)
    add_executable(integration_tests
        integration/test_claude_llm_client_integration.cpp
    )
    target_link_libraries(integration_tests PRIVATE
        cpp_review_lib
        GTest::gtest_main
    )
    target_include_directories(integration_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    gtest_discover_tests(integration_tests)
endif()
