cmake_minimum_required(VERSION 3.10)
project(test_parser C)

include(CTest)

set(CMAKE_C_STANDARD 11)

# required for fuzzing
set(CMAKE_C_COMPILER /usr/bin/clang)

include_directories(include/ ../src)

add_compile_options(-Wall -fsanitize=address,bounds -g)
if (CODE_COVERAGE)
  add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
endif()

add_library(algorand
  ../src/algo_addr.c
  ../src/algo_asa.c
  ../src/algo_tx_dec.c
  ../src/algo_tx.c
  ../src/base32.c
  ../src/base64.c
  ../src/str.c
  ../src/ui_text.c
  ../src/ui_txn.c
)

add_executable(test_str
  ../src/ui_text.c
  src/test_str.c
)
target_compile_options(test_str PRIVATE -Wall -fsanitize=address,bounds -g)
target_link_libraries(test_str PRIVATE cmocka crypto algorand -fsanitize=address,bounds -g)

add_executable(fuzz_tx
  src/cx.c
  src/fuzz_tx.c
  include/cx.h
  include/os.h
)
target_compile_options(fuzz_tx PRIVATE -Wall -fsanitize=fuzzer)
target_link_libraries(fuzz_tx PRIVATE
  algorand
  crypto
  -fsanitize=fuzzer,address,bounds -g
  -fprofile-instr-generate -fcoverage-mapping
)

if (CODE_COVERAGE)
  target_compile_options(fuzz_tx PRIVATE -fprofile-instr-generate -fcoverage-mapping)
  target_link_libraries(fuzz_tx PRIVATE -fprofile-instr-generate -fcoverage-mapping)
endif()


enable_testing()
add_test(test_str test_str)
