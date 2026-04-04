enable_testing()

function(add_nndeploy_test TEST_NAME TEST_SOURCE)
  add_executable(${TEST_NAME}
    ${TEST_SOURCE}
  )
  target_link_libraries(${TEST_NAME}
    GTest::gtest_main
  )
  if (APPLE)
    set_target_properties(${TEST_NAME} PROPERTIES LINK_FLAGS "-Wl,-undefined,dynamic_lookup")
  elseif (UNIX)
    set_target_properties(${TEST_NAME} PROPERTIES LINK_FLAGS "-Wl,--no-as-needed")
  elseif(WIN32)
    if(MSVC)
      # target_link_options(${TEST_NAME} PRIVATE /WHOLEARCHIVE)
    elseif(MINGW)
      set_target_properties(${TEST_NAME} PROPERTIES LINK_FLAGS "-Wl,--no-as-needed")
    endif()
  endif()
  # DEPEND_LIBRARY
  target_link_libraries(${TEST_NAME} ${DEPEND_LIBRARY}) 
  # SYSTEM_LIBRARY
  target_link_libraries(${TEST_NAME} ${SYSTEM_LIBRARY}) 
  # THIRD_PARTY_LIBRARY
  target_link_libraries(${TEST_NAME} ${THIRD_PARTY_LIBRARY}) 
  # install
  if(SYSTEM_Windows)
    install(TARGETS ${TEST_NAME} RUNTIME DESTINATION ${NNDEPLOY_INSTALL_TEST_PATH})
  else() 
    install(TARGETS ${TEST_NAME} RUNTIME DESTINATION ${NNDEPLOY_INSTALL_TEST_PATH})
  endif()

  add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

set(DIRECTORY test)
set(DEPEND_LIBRARY)
set(SYSTEM_LIBRARY)
set(THIRD_PARTY_LIBRARY)

# DEPEND_LIBRARY
list(APPEND DEPEND_LIBRARY ${NNDEPLOY_FRAMEWORK_BINARY})
list(APPEND DEPEND_LIBRARY ${NNDEPLOY_DEPEND_LIBRARY})
# SYSTEM_LIBRARY
list(APPEND SYSTEM_LIBRARY ${NNDEPLOY_SYSTEM_LIBRARY})
# THIRD_PARTY_LIBRARY
list(APPEND THIRD_PARTY_LIBRARY ${NNDEPLOY_THIRD_PARTY_LIBRARY})
list(APPEND THIRD_PARTY_LIBRARY ${NNDEPLOY_PLUGIN_THIRD_PARTY_LIBRARY})
list(APPEND THIRD_PARTY_LIBRARY ${NNDEPLOY_PLUGIN_LIST})

set(TEST_BUILD_DIR ${CMAKE_BINARY_DIR}/test)

add_nndeploy_test(edge_test ${ROOT_PATH}/test/dag/edge_test.cc)
add_nndeploy_test(graph_test ${ROOT_PATH}/test/dag/graph_test.cc)
add_nndeploy_test(device_buffer_tensor_test
                  ${ROOT_PATH}/test/device/device_buffer_tensor_test.cc)

unset(DIRECTORY)
unset(DEPEND_LIBRARY)
unset(SYSTEM_LIBRARY)
unset(THIRD_PARTY_LIBRARY)
