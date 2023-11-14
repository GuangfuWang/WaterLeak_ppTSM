add_library(${DEPLOY_LIB_NAME} SHARED ${_SRC})
target_include_directories(${DEPLOY_LIB_NAME} PUBLIC ${CUDA_INCLUDE_DIR})
target_link_libraries(${DEPLOY_LIB_NAME} PUBLIC ${DEP_LIBS})

add_executable(${DEPLOY_MAIN_NAME} ${_HEADER} ${_MAIN})
target_link_libraries(${DEPLOY_MAIN_NAME} PUBLIC ${DEP_LIBS} ${DEPLOY_LIB_NAME})

if (WATERLEAK_TEST)
    message(STATUS "Build Test...")
    add_executable(${DEPLOY_TEST_MAIN_NAME} ${_HEADER} ${_TEST_MAIN})
    target_link_libraries(${DEPLOY_TEST_MAIN_NAME} PRIVATE ${DEP_LIBS} ${DEPLOY_LIB_NAME})
endif ()

