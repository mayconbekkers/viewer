include(Doctest)

add_executable(llkdu_doctest
    ${CMAKE_SOURCE_DIR}/test/doctest_main.cpp
    ${CMAKE_SOURCE_DIR}/test/lltest_harness.cpp
    ${CMAKE_SOURCE_DIR}/llkdu/llimagej2ckdu.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llimagej2ckdu_test_doctest.cpp
)

target_include_directories(llkdu_doctest
    PRIVATE
        ${CMAKE_SOURCE_DIR}/extern/doctest
        ${CMAKE_SOURCE_DIR}/..
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/test
        ${CMAKE_SOURCE_DIR}/llcommon
        ${CMAKE_SOURCE_DIR}/llimage
        ${CMAKE_SOURCE_DIR}/llkdu
)

target_link_libraries(llkdu_doctest
    PRIVATE
        ll::kdu
        llcommon
)

add_test(NAME llkdu_doctest COMMAND llkdu_doctest)
