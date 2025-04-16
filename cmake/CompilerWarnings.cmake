# from here:
#
# https://github.com/lefticus/cppbestpractices/blob/master/02-Use_the_Tools_Avai
# lable.md
# Courtesy of Jason Turner

function(set_project_warnings PROJECT_NAME)

  set(BOTH_LANGS_WARNINGS
      -Wall
      -Wextra  # reasonable and standard
      #-Wshadow # warn the user if a variable declaration shadows one from a
               # parent context
      #-Wcast-align     # warn for potential performance problem casts
      -Wunused         # warn on anything being unused
      #-Wpedantic   # warn if non-standard C++ is used
      #-Wconversion # warn on type conversions that may lose data
      #-Wsign-conversion  # warn on sign conversions
      #-Wnull-dereference # warn if a null dereference is detected
      -Wdouble-promotion # warn if float is implicit promoted to double
      -Wformat=2 # warn on security issues around functions that format output
                 # (ie printf)
      -Wno-sign-compare
      -Wno-unused-parameter 
      -Wno-missing-field-initializers 
      -Wno-main      
      #-Wno-unused-but-set-variable      
  )

  if (${PROJECT_NAME}_WARNINGS_AS_ERRORS)
    set(BOTH_LANGS_WARNINGS ${BOTH_LANGS_WARNINGS} -Werror)
  endif()

  set(GCC_WARNINGS
      ${BOTH_LANGS_WARNINGS}
      -Wmisleading-indentation # warn if indentation implies blocks where blocks
                               # do not exist
      -Wduplicated-cond # warn if if / else chain has duplicated conditions
      #-Wduplicated-branches # warn if if / else branches have duplicated code
      -Wlogical-op   # warn about logical operations being used where bitwise were
                     # probably wanted
  )

  set(CXX_WARNINGS
      #-Wold-style-cast # warn for c-style casts
      -Wnon-virtual-dtor # warn the user if a class with virtual functions has a
                         # non-virtual destructor. This helps catch hard to
                         # track down memory errors

      -Woverloaded-virtual # warn if you overload (not override) a virtual
                           # function
      #-Wuseless-cast # warn if you perform a cast to the same type
      -Wno-register
      -Wno-double-promotion
  )

  if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    set(PROJECT_WARNINGS ${CLANG_WARNINGS})
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(PROJECT_WARNINGS ${GCC_WARNINGS})
  else()
    message(AUTHOR_WARNING "No compiler warnings set for '${CMAKE_CXX_COMPILER_ID}' compiler.")
  endif()

  if(${PROJECT_NAME}_BUILD_HEADERS_ONLY)
        target_compile_options(${PROJECT_NAME} INTERFACE 
        $<$<COMPILE_LANGUAGE:C>: 
                ${PROJECT_WARNINGS}
        >
        $<$<COMPILE_LANGUAGE:CXX>:
                ${PROJECT_WARNINGS}
                ${CXX_WARNINGS}
        >
        $<$<CONFIG:Debug>:-Wnull-dereference>      # Only for Debug build, else gives false positives
        )
  else()
        target_compile_options(${PROJECT_NAME} PUBLIC
        $<$<COMPILE_LANGUAGE:C>: 
                ${PROJECT_WARNINGS}
        >
        $<$<COMPILE_LANGUAGE:CXX>:
                ${PROJECT_WARNINGS}
                ${CXX_WARNINGS}
        >
        $<$<CONFIG:Debug>:-Wnull-dereference>      # Only for Debug build, else gives false positives
        )
  endif()  

  if(NOT TARGET ${PROJECT_NAME})
    message(AUTHOR_WARNING "${PROJECT_NAME} is not a target, thus no compiler warning were added.")
  endif()
endfunction()
