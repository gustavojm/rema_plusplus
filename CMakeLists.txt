cmake_minimum_required(VERSION 3.15)

set(CMAKE_TOOLCHAIN_FILE "cmake/armgcc.cmake")

# This template attempts to be "fetch_content"-able
# so that it works well with tools like CPM or other
# manual dependency management

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Only set the cxx_standard if it is not set by someone else
if (NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

# specifiy project name
project(
  "REMA_plusplus"
  VERSION 0.1.0
  LANGUAGES C CXX
)

include(cmake/StandardSettings.cmake)
include(cmake/StaticAnalyzers.cmake)
include(cmake/Utils.cmake)
include(cmake/PreventInSourceBuilds.cmake)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()
message(STATUS "Started CMake for ${PROJECT_NAME} v${PROJECT_VERSION}...\n")

set(C_FLAGS
    -mfpu=fpv4-sp-d16
    -mcpu=cortex-m4
    -specs=nano.specs
    -mthumb
    -mfloat-abi=softfp
    -ffunction-sections -fdata-sections -fno-strict-aliasing
    -fsingle-precision-constant
    -fmessage-length=0
    -fno-builtin -fshort-enums
    -fno-common -fmerge-constants
    #-save-temps=obj                            save intermediate files in CMakeFiles/REMA_plusplus.dir "source.c.i"
   )

set(CXX_FLAGS
    ${C_FLAGS}    
    -fno-rtti -fno-exceptions
)

set(ASM_FLAGS -g3 -mcpu=cortex-m4 -mthumb -mfloat-abi=softfp --specs=nano.specs -mfpu=fpv4-sp-d16)

set(LINKER_FILES        
  -T${CMAKE_SOURCE_DIR}/device/lpc4337_memory.ld      # Provided by mcuxpresso ide 
  -T${CMAKE_SOURCE_DIR}/device/lpc4337.ld             # Provided by mcuxpresso ide 
)

list(JOIN C_FLAGS " " C_FLAGS)
list(JOIN CXX_FLAGS " " CXX_FLAGS)
list(JOIN ASM_FLAGS " " ASM_FLAGS)

set(CMAKE_ASM_FLAGS_DEBUG "${ASM_FLAGS}")
set(CMAKE_ASM_FLAGS_RELEASE "${ASM_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG "${C_FLAGS} -g -Og")
set(CMAKE_CXX_FLAGS_DEBUG "${CXX_FLAGS} -g -Og")
set(CMAKE_C_FLAGS_RELEASE "${C_FLAGS} -DNDEBUG -O3")    # -Os -flto
set(CMAKE_CXX_FLAGS_RELEASE "${CXX_FLAGS} -DNDEBUG -O3")  # -Os -g3 -flto
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${LD_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${LD_FLAGS}")   # -g3 -flto

# some definition (-D)
add_compile_definitions(
    DEBUG
    #SIMULATE_ENCODER 
    __CODE_RED 
    __NEWLIB__ 
    CORE_M4     
    __USE_LPCOPEN 
    #CPP_USE_HEAP 
    __LPC43XX__
)

# compile option only for Debug build
#add_compile_options("$<$<CONFIG:Debug>:-finstrument-functions>")


file(GLOB_RECURSE CHIP_SOURCES
    ../lpc_chip_43xx/src/*.c
)

add_library(chip STATIC ${CHIP_SOURCES})

target_include_directories(chip PRIVATE
  ../lpc_chip_43xx/inc
  ../lpc_chip_43xx/inc/config_43xx
)
target_compile_options(chip PRIVATE
  -w
)

file(GLOB_RECURSE BOARD_SOURCES
      "../CIAA_NXP_board/src/*.c"
)
add_library(board STATIC ${BOARD_SOURCES})
target_compile_options(board PRIVATE
  -w
)

target_include_directories(board PRIVATE
  ../lpc_chip_43xx/inc
  ../CIAA_NXP_board/inc
  ../lpc_chip_43xx/inc/config_43xx
)


file(GLOB_RECURSE SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/newlib/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sysinit.c
)

file(GLOB_RECURSE EXTERNAL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/freertos/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/lwip/*.c
)

file(GLOB_RECURSE HEADERS
    inc/*
)

set(HEADERS_DIR
    inc
)

set(EXTERNAL_HEADERS_DIR
    ../CIAA_NXP_board/inc
    lwip/inc
    lwip/inc/ipv4
    freertos/inc
    ../../pico/encoders/inc
    #/usr/local/arm-gnu-toolchain-12.3.rel1-x86_64-arm-none-eabi/arm-none-eabi/include
    ../lpc_chip_43xx/inc
    ../lpc_chip_43xx/inc/config_43xx
)

if(${PROJECT_NAME}_BUILD_EXECUTABLE)
  add_executable(${PROJECT_NAME} ${SOURCES} ${EXTERNAL_SOURCES})

  if(${PROJECT_NAME}_VERBOSE_OUTPUT)
  verbose_message("Found the following HEADERS:")
  foreach(HEADER IN LISTS HEADERS)
    verbose_message("* ${HEADER}")
  endforeach()
  
  verbose_message("Found the following SOURCES:")
    foreach(SOURCE IN LISTS SOURCES)
      verbose_message("* ${SOURCE}")
    endforeach()
  endif()

  if(${PROJECT_NAME}_ENABLE_UNIT_TESTING)
    add_library(${PROJECT_NAME}_LIB ${HEADERS} ${SOURCES})

    if(${PROJECT_NAME}_VERBOSE_OUTPUT)
      verbose_message("Found the following HEADERS:")
      foreach(HEADER IN LISTS HEADERS)
        verbose_message("* ${HEADER}")
      endforeach()
    endif()
  endif()
elseif(${PROJECT_NAME}_BUILD_HEADERS_ONLY)
  add_library(${PROJECT_NAME} INTERFACE)

  if(${PROJECT_NAME}_VERBOSE_OUTPUT)
    verbose_message("Found the following HEADERS:")
    foreach(HEADER IN LIST HEADERS)
      verbose_message("* ${HEADER}")
    endforeach()
  endif()
else()
  add_library(
    ${PROJECT_NAME}
    ${HEADERS}
    ${SOURCES}
  )

  if(${PROJECT_NAME}_VERBOSE_OUTPUT)
    verbose_message("Found the following SOURCES:")
    foreach(SOURCE IN LISTS SOURCES)
      verbose_message("* ${SOURCE}")
    endforeach()
    verbose_message("Found the following HEADERS:")
    foreach(HEADER IN LISTS HEADERS)
      verbose_message("* ${HEADER}")
    endforeach()
  endif()
endif()
  
target_include_directories(${PROJECT_NAME} PRIVATE
  ${HEADERS_DIR} ${EXTERNAL_HEADERS_DIR}
)

target_link_libraries(
  ${PROJECT_NAME}
  board
  chip
)

set(MAIN_TARGET ${PROJECT_NAME})

# Linker options
target_link_options(${PROJECT_NAME} PRIVATE
        ${LINKER_FILES}        
        -mcpu=cortex-m4
        -mthumb
        #-nostdlib        
        -Wl,-Map=${PROJECT_NAME}.map,--cref
        -Wl,--gc-sections
        -Xlinker -print-memory-usage -Xlinker
        -fstack-usage
        -u _printf_float        
)

#
# Set the project standard and warnings
#
include(cmake/CompilerWarnings.cmake)
set_project_warnings(${PROJECT_NAME})

verbose_message("Applied compiler warnings. Using standard ${CMAKE_CXX_STANDARD}.\n")

#
# Enable Doxygen
#
include(cmake/Doxygen.cmake)

#
# Format the project using the `clang-format` target (i.e: cmake --build build --target clang-format)
#
include(cmake/clang-format.cmake)

# Optional: Print executable size as part of the post build process
add_custom_command(TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_SIZE} ${PROJECT_NAME}
)

# Optional: Create hex, bin and S-Record files after the build
add_custom_command(TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O srec --srec-len=64 ${PROJECT_NAME} ${PROJECT_NAME}.s19
        COMMAND ${CMAKE_OBJCOPY} -O ihex ${PROJECT_NAME} ${PROJECT_NAME}.hex
        COMMAND ${CMAKE_OBJCOPY} -O binary ${PROJECT_NAME} ${PROJECT_NAME}.bin
)

# Specify OpenOCD flash command
add_custom_target(flash
    COMMAND openocd -f ${PROJECT_SOURCE_DIR}/openOCD/lpc4337.cfg 
    -c "program ${PROJECT_NAME}"
    -c "init"
    -c "halt 0"
    -c "flash write_image erase ${PROJECT_NAME}"
    -c "reset run"
    -c "shutdown"
    DEPENDS ${PROJECT_NAME}
    COMMENT "Flashing the device with OpenOCD"
)        


