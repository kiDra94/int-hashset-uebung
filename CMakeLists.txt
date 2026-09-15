cmake_minimum_required(VERSION 3.18)
project(int_hash_set VERSION 1.0)

include(CMakeLists.config)

set(CPP_SRCS  # all non-main source files
  ${PROJECT_NAME}.cpp
)

foreach(sourcefile ${CPP_SRCS})
  string(REPLACE ".cpp" "_test.cpp" testfile ${sourcefile})
  string(REPLACE ".cpp" "" testexecutable ${testfile})
  message(STATUS "${sourcefile} + ${testfile} => ${testexecutable}")
  add_executable(${testexecutable} ${sourcefile} ${testfile})
  target_link_libraries(${testexecutable} Catch2::Catch2WithMain)

  if(EXISTS "./${testexecutable}")
    # create ctest definitions (required by KDevelop's unit tests tool view)
    execute_process(COMMAND ./${testexecutable} --list-tests -v quiet
                    OUTPUT_VARIABLE test_case_names)
    string(REPLACE "\n" ";" test_cases "${test_case_names}")
    foreach(test_case ${test_cases})
      message(STATUS "TEST_CASE: \"${testexecutable} ${test_case}\"")
      add_test(NAME ${test_case} COMMAND ${testexecutable} ${test_case})
    endforeach()
  else()
    message(STATUS "if you want to run per testcase `ctest`s in you IDE")
    message(STATUS "rerun `cmake ..` after building the test executables")
    add_test(NAME ${testexecutable} COMMAND ${testexecutable})
  endif()

endforeach()
