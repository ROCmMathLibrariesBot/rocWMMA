###############################################################################
 #
 # MIT License
 #
 # Copyright 2021-2022 Advanced Micro Devices, Inc.
 #
 # Permission is hereby granted, free of charge, to any person obtaining a copy
 # of this software and associated documentation files (the "Software"), to deal
 # in the Software without restriction, including without limitation the rights
 # to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 # copies of the Software, and to permit persons to whom the Software is
 # furnished to do so, subject to the following conditions:
 #
 # The above copyright notice and this permission notice shall be included in
 # all copies or substantial portions of the Software.
 #
 # THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 # IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 # FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 # AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 # LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 # OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 # SOFTWARE.
 #
 ###############################################################################

set(ROCWMMA_SAMPLES_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# Regular gtests that don't require any special
# designation for validation or benchmarking
function(add_rocwmma_sample TEST_TARGET TEST_SOURCE)

  list(APPEND TEST_SOURCE ${ARGN})
  add_executable(${TEST_TARGET} ${TEST_SOURCE})
  target_link_libraries(${TEST_TARGET} rocwmma )
  target_include_directories(${TEST_TARGET} PRIVATE
                             ${CMAKE_CURRENT_SOURCE_DIR}
                             ${ROCWMMA_SAMPLES_INCLUDE_DIR})

  # Add support to build the target's assembly files
  if(ROCWMMA_BUILD_ASSEMBLY)
    foreach(file_name ${TEST_SOURCE})

      # Replicate the current source tree in the build output/assembly folder
      file(RELATIVE_PATH relative_file_path "${CMAKE_CURRENT_SOURCE_DIR}" "${file_name}")
      string(REPLACE "../" "__/" relative_file_string "${relative_file_path}")
      add_custom_command(TARGET ${TEST_TARGET}
                         POST_BUILD
                         COMMAND make ARGS ${relative_file_string}.s
                         COMMAND ${CMAKE_COMMAND} -E copy
                           "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${TEST_TARGET}.dir/${relative_file_string}.s"
                           "${CMAKE_CURRENT_BINARY_DIR}/assembly/${relative_file_string}.s"
                          WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    endforeach()
  endif()

  rocm_install_targets(
    TARGETS ${TEST_TARGET}
    COMPONENT samples
  )
endfunction()

# Create sample targets
add_rocwmma_sample(simple_gemm ${CMAKE_CURRENT_SOURCE_DIR}/simple_gemm.cpp)
add_rocwmma_sample(sgemmv ${CMAKE_CURRENT_SOURCE_DIR}/sgemmv.cpp)
add_rocwmma_sample(simple_dlrm ${CMAKE_CURRENT_SOURCE_DIR}/simple_dlrm.cpp)
