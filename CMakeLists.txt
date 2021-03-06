cmake_minimum_required(VERSION 3.10)

project(FlatBuffers)

# NOTE: Code coverage only works on Linux & OSX.
option(FLATBUFFERS_CODE_COVERAGE "Enable the code coverage build option." OFF)
option(FLATBUFFERS_BUILD_TESTS "Enable the build of tests and samples." OFF)
option(FLATBUFFERS_INSTALL "Enable the installation of targets." OFF)
option(FLATBUFFERS_BUILD_FLATLIB "Enable the build of the flatbuffers library" ON)
option(FLATBUFFERS_BUILD_FLATC "Enable the build of the flatbuffers compiler" ON)
option(FLATBUFFERS_BUILD_FLATHASH "Enable the build of flathash" OFF)

if(NOT FLATBUFFERS_BUILD_FLATC AND FLATBUFFERS_BUILD_TESTS)
    message(WARNING
    "Cannot build tests without building the compiler. Tests will be disabled.")
    set(FLATBUFFERS_BUILD_TESTS OFF)
endif()

set(FlatBuffers_Library_SRCS
  include/flatbuffers/code_generators.h
  include/flatbuffers/flatbuffers.h
  include/flatbuffers/hash.h
  include/flatbuffers/idl.h
  include/flatbuffers/util.h
  include/flatbuffers/reflection.h
  include/flatbuffers/reflection_generated.h
  src/idl_parser.cpp
  src/idl_gen_text.cpp
  src/reflection.cpp
  src/util.cpp
)

set(FlatBuffers_Compiler_SRCS
  ${FlatBuffers_Library_SRCS}
  src/idl_gen_cpp.cpp
  src/idl_gen_general.cpp
  src/idl_gen_go.cpp
  src/idl_gen_js.cpp
  src/idl_gen_php.cpp
  src/idl_gen_python.cpp
  src/idl_gen_fbs.cpp
  src/flatc.cpp
)

set(FlatHash_SRCS
  include/flatbuffers/hash.h
  src/flathash.cpp
)

set(FlatBuffers_Tests_SRCS
  ${FlatBuffers_Library_SRCS}
  src/idl_gen_fbs.cpp
  src/idl_gen_general.cpp
  tests/test.cpp
  # file generate by running compiler on tests/monster_test.fbs
  ${CMAKE_CURRENT_BINARY_DIR}/tests/monster_test_generated.h
)

set(FlatBuffers_Sample_Binary_SRCS
  include/flatbuffers/flatbuffers.h
  samples/sample_binary.cpp
  # file generated by running compiler on samples/monster.fbs
  ${CMAKE_CURRENT_BINARY_DIR}/samples/monster_generated.h
)

set(FlatBuffers_Sample_Text_SRCS
  include/flatbuffers/flatbuffers.h
  include/flatbuffers/hash.h
  include/flatbuffers/idl.h
  include/flatbuffers/util.h
  src/idl_parser.cpp
  src/idl_gen_text.cpp
  src/util.cpp
  samples/sample_text.cpp
  # file generated by running compiler on samples/monster.fbs
  ${CMAKE_CURRENT_BINARY_DIR}/samples/monster_generated.h
)

# source_group(Compiler FILES ${FlatBuffers_Compiler_SRCS})
# source_group(Tests FILES ${FlatBuffers_Tests_SRCS})

if(APPLE)
  set(fbs-compiler-flags
    ${fbs-compiler-flags} -std=c++17 -Wall -pedantic -Werror -Wextra)
elseif(CMAKE_COMPILER_IS_GNUCXX)
  if(CYGWIN)
    set(fbs-compiler-flags
      ${fbs-compiler-flags} -std=gnu++17)
  else(CYGWIN)
    set(fbs-compiler-flags
      ${fbs-compiler-flags} -std=c++17)
  endif(CYGWIN)
  set(fbs-compiler-flags
    ${fbs-compiler-flags} -Wall -pedantic -Werror -Wextra -Werror=shadow -Wno-implicit-fallthrough)
  if (GCC_VERSION VERSION_GREATER 4.4)
    set(fbs-compiler-flags
      ${fbs-compiler-flags} -Wunused-result -Werror=unused-result)
  endif()

  # Certain platforms such as ARM do not use signed chars by default
  # which causes issues with certain bounds checks.
  set(fbs-compiler-flags
    ${fbs-compiler-flags} -fsigned-char)

elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  set(fbs-compiler-flags
      ${fbs-compiler-flags} -std=c++17 -Wall -pedantic -Werror -Wextra)

  # Certain platforms such as ARM do not use signed chars by default
  # which causes issues with certain bounds checks.
  set(fbs-compiler-flags
    ${fbs-compiler-flags} -fsigned-char)

endif()

if(FLATBUFFERS_CODE_COVERAGE)
  set(fbs-compiler-flags ${fbs-compiler-flags} -g -fprofile-arcs -ftest-coverage)
  set(CMAKE_EXE_LINKER_FLAGS
      "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs -ftest-coverage")
endif()

if(BIICODE)
  include(biicode/cmake/biicode.cmake)
  return()
endif()

if(FLATBUFFERS_BUILD_FLATLIB)
  add_library(flatbuffers64 STATIC ${FlatBuffers_Library_SRCS})
  target_compile_features(flatbuffers64 PUBLIC cxx_std_17)
  target_compile_definitions(flatbuffers64 PRIVATE FLATBUFFERS_64=1)
  target_compile_options(flatbuffers64 PRIVATE ${fbs-compiler-flags})
  target_include_directories(flatbuffers64 PUBLIC include)

  add_library(flatbuffers32 STATIC ${FlatBuffers_Library_SRCS})
  target_compile_features(flatbuffers32 PUBLIC cxx_std_17)
  target_compile_definitions(flatbuffers32 PRIVATE FLATBUFFERS_32=1)
  target_compile_options(flatbuffers32 PRIVATE ${fbs-compiler-flags})
  target_include_directories(flatbuffers32 PUBLIC include)
endif()

if(FLATBUFFERS_BUILD_FLATC)
  add_executable(flatc64 ${FlatBuffers_Compiler_SRCS})
  target_compile_features(flatc64 PUBLIC cxx_std_17)
  target_compile_definitions(flatc64 PRIVATE FLATBUFFERS_64=1)
  target_compile_options(flatc64 PRIVATE ${fbs-compiler-flags})
  target_include_directories(flatc64 PUBLIC include)

  add_executable(flatc32 ${FlatBuffers_Compiler_SRCS})
  target_compile_features(flatc32 PUBLIC cxx_std_17)
  target_compile_definitions(flatc32 PRIVATE FLATBUFFERS_32=1)
  target_compile_options(flatc32 PRIVATE ${fbs-compiler-flags})
  target_include_directories(flatc32 PUBLIC include)

  if(NOT FLATBUFFERS_FLATC_32_EXECUTABLE)
    set(FLATBUFFERS_FLATC_32_EXECUTABLE $<TARGET_FILE:flatc32>)
  endif()

  if(NOT FLATBUFFERS_FLATC_64_EXECUTABLE)
    set(FLATBUFFERS_FLATC_64_EXECUTABLE $<TARGET_FILE:flatc64>)
  endif()
endif()

if(FLATBUFFERS_BUILD_FLATHASH)
  add_executable(flathash ${FlatHash_SRCS})
  target_compile_features(flathash PUBLIC cxx_std_17)
endif()

add_executable(resource-generator EXCLUDE_FROM_ALL ${CMAKE_CURRENT_SOURCE_DIR}/helpers/resource_generator.cpp)
target_compile_features(resource-generator PUBLIC cxx_std_17)
target_compile_definitions(resource-generator PRIVATE FLATBUFFERS_32=1)
target_compile_options(resource-generator PRIVATE ${fbs-compiler-flags})
target_include_directories(resource-generator PUBLIC include)

function(compile_flatbuffers_to_resource_header SRC_FBS TARGET_HEADER NAMESPACE DEPS)
  get_filename_component(TARGET_DIR ${TARGET_HEADER} PATH)
  file(MAKE_DIRECTORY ${TARGET_DIR})
  add_custom_command(
    OUTPUT ${TARGET_HEADER}
    COMMAND $<TARGET_FILE:resource-generator> "${SRC_FBS}" "${TARGET_HEADER}" "${NAMESPACE}"
    DEPENDS resource-generator ${DEPS})
endfunction()

get_directory_property(HAS_PARENT PARENT_DIRECTORY)
if(HAS_PARENT)
  set(resource-generator-command "resource-generator" PARENT_SCOPE)
  set(flatc-command "flatc" PARENT_SCOPE)
endif()

if(FLATBUFFERS_BUILD_TESTS)
  compile_flatbuffers_schema_to_cpp(tests/monster_test.fbs)
  include_directories(${CMAKE_CURRENT_BINARY_DIR}/tests)
  add_executable(flattests ${FlatBuffers_Tests_SRCS})
  target_compile_features(flattests PUBLIC cxx_std_17)

  compile_flatbuffers_schema_to_cpp(samples/monster.fbs)
  include_directories(${CMAKE_CURRENT_BINARY_DIR}/samples)

  add_executable(flatsamplebinary ${FlatBuffers_Sample_Binary_SRCS})
  target_compile_features(flatsamplebinary PUBLIC cxx_std_17)

  add_executable(flatsampletext ${FlatBuffers_Sample_Text_SRCS})
  target_compile_features(flatsampletext PUBLIC cxx_std_17)
endif()

if(FLATBUFFERS_INSTALL)
  install(DIRECTORY include/flatbuffers DESTINATION include)
  if(FLATBUFFERS_BUILD_FLATLIB)
    install(TARGETS flatbuffers DESTINATION lib)
  endif()
  if(FLATBUFFERS_BUILD_FLATC)
    install(TARGETS flatc DESTINATION bin)
  endif()
endif()

if(FLATBUFFERS_BUILD_TESTS)
  enable_testing()

  file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/tests" DESTINATION
       "${CMAKE_CURRENT_BINARY_DIR}")
  add_test(NAME flattests COMMAND flattests)
endif()

include(CMake/BuildFlatBuffers.cmake)
