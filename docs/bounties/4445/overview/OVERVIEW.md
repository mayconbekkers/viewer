# Relatório doctest x TUT

## Resumo executivo

- 3 alvos doctest cobrindo 227 casos e 1,401,659 asserts em 3.64s totais (ctest verde).
- 151 arquivos ainda dependem de TUT; principais diretórios remanescentes: indra/llcommon/tests (1,211 hits), indra/test (1,113 hits), indra/newview/tests (913 hits).
- Referências CMake a TUT: 39 ocorrências em 11 arquivos.

## Alvos doctest

| Target | Módulo | Casos | Asserts | Runtime (s) |
| --- | --- | ---:| ---:| ---:|
| llcommon_doctest | llcommon | 100 | 1,400,949 | 2.61 |
| llcorehttp_doctest | llcorehttp | 34 | 184 | 0.56 |
| llmath_doctest | llmath | 93 | 526 | 0.47 |
| **Total** |  | 227 | 1,401,659 | 3.64 |

## Top diretórios com TUT

| Diretório | Arquivos | Hits |
| --- | ---:| ---:|
| indra/llcommon/tests | 42 | 1,211 |
| indra/test | 34 | 1,113 |
| indra/newview/tests | 23 | 913 |
| indra/llmath/tests | 16 | 888 |
| indra/llcorehttp/tests | 9 | 399 |

## Estado por módulo

### llcommon

* Suítes doctest ativas: `llcommon_doctest` (100 casos / 1,400,949 asserts / 2.61s).
* Backlog TUT: 42 arquivos remanescentes (~1,211 hits).
* Top pendências: [indra/llcommon/tests/llstring_test.cpp](../../../../indra/llcommon/tests/llstring_test.cpp) (160 hits), [indra/llcommon/tests/llprocess_test.cpp](../../../../indra/llcommon/tests/llprocess_test.cpp) (115 hits), [indra/llcommon/tests/commonmisc_test.cpp](../../../../indra/llcommon/tests/commonmisc_test.cpp) (112 hits).

### llcorehttp

* Suítes doctest ativas: `llcorehttp_doctest` (34 casos / 184 asserts / 0.56s).
* Backlog TUT: 9 arquivos remanescentes (~399 hits).
* Top pendências: [indra/llcorehttp/tests/test_httprequest.hpp](../../../../indra/llcorehttp/tests/test_httprequest.hpp) (172 hits), [indra/llcorehttp/tests/test_bufferarray.hpp](../../../../indra/llcorehttp/tests/test_bufferarray.hpp) (81 hits), [indra/llcorehttp/tests/test_httpheaders.hpp](../../../../indra/llcorehttp/tests/test_httpheaders.hpp) (50 hits).

### llmath

* Suítes doctest ativas: `llmath_doctest` (93 casos / 526 asserts / 0.47s).
* Backlog TUT: 16 arquivos remanescentes (~888 hits).
* Top pendências: [indra/llmath/tests/llrect_test.cpp](../../../../indra/llmath/tests/llrect_test.cpp) (140 hits), [indra/llmath/tests/llquaternion_test.cpp](../../../../indra/llmath/tests/llquaternion_test.cpp) (85 hits), [indra/llmath/tests/v3math_test.cpp](../../../../indra/llmath/tests/v3math_test.cpp) (76 hits).

## Onde ainda há TUT

- [indra/newview/tests/llmediadataclient_test.cpp](../../../../indra/newview/tests/llmediadataclient_test.cpp) — 201 hits
- [indra/newview/tests/llsechandler_basic_test.cpp](../../../../indra/newview/tests/llsechandler_basic_test.cpp) — 174 hits
- [indra/llcorehttp/tests/test_httprequest.hpp](../../../../indra/llcorehttp/tests/test_httprequest.hpp) — 172 hits
- [indra/llcommon/tests/llstring_test.cpp](../../../../indra/llcommon/tests/llstring_test.cpp) — 160 hits
- [indra/llmath/tests/llrect_test.cpp](../../../../indra/llmath/tests/llrect_test.cpp) — 140 hits
- [indra/test/io.cpp](../../../../indra/test/io.cpp) — 138 hits
- [indra/llmessage/tests/llnamevalue_test.cpp](../../../../indra/llmessage/tests/llnamevalue_test.cpp) — 131 hits
- [indra/llcommon/tests/llprocess_test.cpp](../../../../indra/llcommon/tests/llprocess_test.cpp) — 115 hits
- [indra/llcommon/tests/commonmisc_test.cpp](../../../../indra/llcommon/tests/commonmisc_test.cpp) — 112 hits
- [indra/llinventory/tests/inventorymisc_test.cpp](../../../../indra/llinventory/tests/inventorymisc_test.cpp) — 105 hits

## CMake

- `indra/cmake/CMakeLists.txt:60` — Tut.cmake
- `indra/cmake/LLAddBuildTest.cmake:17` — # * properties for each sourcefile passed in indicate what libs to link that file with (MAKE NO ASSUMPTIONS ASIDE FROM TUT)
- `indra/cmake/LLAddBuildTest.cmake:195` — ${CMAKE_SOURCE_DIR}/test/lltut.cpp
- `indra/cmake/LLAddBuildTest.cmake:30` — ${CMAKE_SOURCE_DIR}/test/lltut.cpp
- `indra/cmake/LLAddBuildTest.cmake:5` — include(Tut)
- `indra/cmake/Tut.cmake:4` — use_prebuilt_binary(tut)
- `indra/cmake/Variables.cmake:161` — # different CMake behavior: it substitutes plain -g. As of 2017-09-19,
- `indra/cmake/Variables.cmake:163` — # no-symbols case. Set -gdwarf, triggering CMake to substitute plain -g --
- `indra/integration_tests/llui_libtest/CMakeLists.txt:23` — # include(Tut)
- `indra/llcorehttp/CMakeLists.txt:13` — include(Tut)
- `indra/llimage/CMakeLists.txt:13` — include(Tut)
- `indra/llkdu/CMakeLists.txt:50` — include(Tut)
- `indra/llkdu/CMakeLists.txt:57` — lltut.h
- `indra/llmessage/CMakeLists.txt:11` — include(Tut)
- `indra/llui/CMakeLists.txt:211` — lltextutil.h
- `indra/llui/CMakeLists.txt:90` — lltextutil.cpp
- `indra/test/CMakeLists.txt:15` — llapp_tut.cpp
- `indra/test/CMakeLists.txt:16` — llblowfish_tut.cpp
- `indra/test/CMakeLists.txt:17` — llbuffer_tut.cpp
- `indra/test/CMakeLists.txt:18` — lldoubledispatch_tut.cpp
- `indra/test/CMakeLists.txt:19` — llevents_tut.cpp
- `indra/test/CMakeLists.txt:20` — llhttpdate_tut.cpp
- `indra/test/CMakeLists.txt:21` — llhttpnode_tut.cpp
- `indra/test/CMakeLists.txt:22` — lliohttpserver_tut.cpp
- `indra/test/CMakeLists.txt:23` — llmessageconfig_tut.cpp
- `indra/test/CMakeLists.txt:24` — llpermissions_tut.cpp
- `indra/test/CMakeLists.txt:26` — llsaleinfo_tut.cpp
- `indra/test/CMakeLists.txt:27` — llsdmessagebuilder_tut.cpp
- `indra/test/CMakeLists.txt:28` — llsdmessagereader_tut.cpp
- `indra/test/CMakeLists.txt:29` — llsd_new_tut.cpp
- `indra/test/CMakeLists.txt:30` — llsdutil_tut.cpp
- `indra/test/CMakeLists.txt:31` — llservicebuilder_tut.cpp
- `indra/test/CMakeLists.txt:32` — llstreamtools_tut.cpp
- `indra/test/CMakeLists.txt:33` — lltemplatemessagebuilder_tut.cpp
- `indra/test/CMakeLists.txt:34` — lltut.cpp
- `indra/test/CMakeLists.txt:35` — message_tut.cpp
- `indra/test/CMakeLists.txt:45` — lltut.h
- `indra/test/CMakeLists.txt:51` — llmessagetemplateparser_tut.cpp
- `indra/test/CMakeLists.txt:9` — include(Tut)

## Plano de fechamento

1. Confirmar portabilidade dos casos remanescentes em `indra/llcommon/tests` e eliminar duplicatas entre TUT e doctest.
2. Transpor suítes críticas de `indra/llmath/tests` (ex.: geometria e limites) para doctest reutilizando fixtures existentes.
3. Migrar `indra/llcorehttp/tests` — começar por `test_httprequest.hpp` e coberturas de retries/fakes.
4. Revisar `indra/newview/tests` e `indra/test` para planejar substituições modulares (mocking/eventos).
5. Limpar referências CMake (`ENABLE_TUT`/`LL_TESTS`) e encerrar últimos harnesses TUT antes do ajuste no CI.

## Comandos de reprodução

```powershell
# llcommon_doctest
autobuild configure -c RelWithDebInfoOS -- -DLL_TESTS=ON
"C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" --build build-vc170-64 --config RelWithDebInfo --target llcommon_doctest -- /p:BuildProjectReferences=false
"C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/ctest.exe" --test-dir build-vc170-64 -C RelWithDebInfo -R llcommon_doctest -V

# llmath_doctest
autobuild configure -c RelWithDebInfoOS -- -DLL_TESTS=ON
"C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" --build build-vc170-64 --config RelWithDebInfo --target llmath_doctest -- /p:BuildProjectReferences=false
"C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/ctest.exe" --test-dir build-vc170-64 -C RelWithDebInfo -R llmath_doctest -V

# llcorehttp_doctest
autobuild configure -c RelWithDebInfoOS -- -DLL_TESTS=ON
"C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" --build build-vc170-64 --config RelWithDebInfo --target llcorehttp_doctest -- /p:BuildProjectReferences=false
"C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/ctest.exe" --test-dir build-vc170-64 -C RelWithDebInfo -R llcorehttp_doctest -V
```

## Apêndice

- `docs/bounties/4445/overview/DOCTEST_TARGETS.csv`
- `docs/bounties/4445/overview/TUT_FILES.csv`
- `docs/bounties/4445/overview/TUT_BY_DIR.csv`
- `docs/bounties/4445/overview/CMAKE_TUT_REFS.csv`
