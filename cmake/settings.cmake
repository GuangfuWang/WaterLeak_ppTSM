## Author: Guangfu WANG.
## Date: 2023-08-20.
#set cpp version used in this project.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
#this is equivalently to -fPIC in cxx_flags.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#define options for custom build targets.
option(FIGHT_TEST "Build fight test program." ON)
option(FIGHT_PREPROCESS_GPU "Use GPU version of preprocessing pipeline" ON)
set(FIGHT_INPUT_NAME "data_batch_0" CACHE STRING "Input layer name for tensorrt deploy.")
set(FIGHT_OUTPUT_NAMES "linear_2.tmp_1" CACHE STRING "Output layer names for tensorrt deploy, seperated with comma or colon")
set(FIGHT_DEPLOY_MODEL "../models/waterleak20231113.engine" CACHE STRING "Used deploy AI model file (/path/to/*.engine)")

# generate config.h in src folder.
configure_file(
        "${PROJECT_SOURCE_DIR}/src/macro.h.in"
        "${PROJECT_SOURCE_DIR}/src/macro.h"
        @ONLY
)

set(DEPLOY_LIB_NAME "waterleak_ppTSM")
set(DEPLOY_MAIN_NAME "waterleak_ppTSM_main")
set(DEPLOY_TEST_MAIN_NAME "waterleak_ppTSM_test")

set(CMAKE_INSTALL_RPATH "\$ORIGIN")
set(CMAKE_INSTALL_PREFIX "install")
add_link_options("-Wl,--as-needed")
