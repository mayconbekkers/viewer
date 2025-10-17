set(LLCOMMON_DOCTEST_SOURCES
    # Core migrated suites (green)
    ${CMAKE_CURRENT_SOURCE_DIR}/apply_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bitpack_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lazyeventapi_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llbase64_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llcond_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llerror_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llframetimer_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llprocess_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llrand_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llsdserialize_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llsingleton_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llstring_test_doctest.cpp

    # Helper validation and shims
    ${CMAKE_SOURCE_DIR}/test/ll_doctest_helpers_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/win_wstring_shim.cpp
)

add_executable(llcommon_doctest
    ${LLCOMMON_DOCTEST_SOURCES}
    ${CMAKE_SOURCE_DIR}/test/doctest_main.cpp
)

target_include_directories(llcommon_doctest
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/..
        ${DOCTEST_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/test
        ${CMAKE_SOURCE_DIR}/indra/test
        ${CMAKE_SOURCE_DIR}/indra/llcommon
        ${CMAKE_SOURCE_DIR}/indra/llcommon/tests
        ${CMAKE_SOURCE_DIR}/llcommon
        ${CMAKE_SOURCE_DIR}/llcommon/tests
)

target_link_libraries(llcommon_doctest
    llcommon
)

add_test(NAME llcommon_doctest COMMAND llcommon_doctest)
