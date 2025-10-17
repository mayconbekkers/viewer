add_executable(llcorehttp_doctest
    ${CMAKE_SOURCE_DIR}/test/doctest_main.cpp
    ${CMAKE_SOURCE_DIR}/llcommon/tests_doctest/win_wstring_shim.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/http_header_norm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/http_fakes.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bufferarray_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/httpheaders_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/httpoperation_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/httpstatus_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/httprequest_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/httprequestqueue_test_doctest.cpp
)

target_include_directories(llcorehttp_doctest
    PRIVATE
        ${DOCTEST_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR}/../test
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/../llcommon
        ${PROJECT_SOURCE_DIR}/../llmessage
        ${PROJECT_SOURCE_DIR}/../extern/doctest
)

target_link_libraries(llcorehttp_doctest
    PRIVATE
        llcorehttp
        llmessage
        llcommon
)

add_test(NAME llcorehttp_doctest COMMAND llcorehttp_doctest)
