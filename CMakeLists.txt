cmake_minimum_required(VERSION 3.20)
project(KeystoneDTL C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(keystonedtl STATIC
    src/kdtl_accounts.c
    src/kdtl_amount.c
    src/kdtl_catalog.c
    src/kdtl_codec.c
    src/kdtl_engine.c
    src/kdtl_events.c
    src/kdtl_ids.c
    src/kdtl_limits.c
    src/kdtl_positions.c
    src/kdtl_policy.c
    src/kdtl_reconcile.c
    src/kdtl_reporting.c
    src/kdtl_settlement.c
    src/kdtl_stress.c
)

target_include_directories(keystonedtl PUBLIC include PRIVATE src)

add_executable(kdtl_scenario tools/kdtl_scenario.c)
target_link_libraries(kdtl_scenario PRIVATE keystonedtl)

if(MSVC)
    target_compile_options(keystonedtl PRIVATE /W4 /WX)
    target_compile_options(kdtl_scenario PRIVATE /W4 /WX)
else()
    target_compile_options(keystonedtl PRIVATE -Wall -Wextra -Werror -pedantic)
    target_compile_options(kdtl_scenario PRIVATE -Wall -Wextra -Werror -pedantic)
endif()

enable_testing()
add_test(NAME node_contracts COMMAND node --test tests/node/*.test.js)
