#
# Add a target for formating the project using `clang-format` (i.e: cmake --build build --target clang-format)
#

function(add_clang_format_target)
    if(NOT ${PROJECT_NAME}_CLANG_FORMAT_BINARY)
			find_program(${PROJECT_NAME}_CLANG_FORMAT_BINARY clang-format)
    endif()

    if(${PROJECT_NAME}_CLANG_FORMAT_BINARY)
			if(${PROJECT_NAME}_BUILD_EXECUTABLE)
                add_custom_target(clang-format
                    COMMAND ${${PROJECT_NAME}_CLANG_FORMAT_BINARY}
                      -i
                      --verbose
                      ${SOURCES} ${HEADERS}
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                    COMMENT "Formatting with ${${PROJECT_NAME}_CLANG_FORMAT_BINARY}..."
                  )
			elseif(${PROJECT_NAME}_BUILD_HEADERS_ONLY)
				add_custom_target(clang-format
						COMMAND ${${PROJECT_NAME}_CLANG_FORMAT_BINARY}
						-i ${HEADERS}
                        --verbose
						WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
                        COMMENT "Formatting with ${${PROJECT_NAME}_CLANG_FORMAT_BINARY} ${SOURCES}..."
                 )
			else()
				add_custom_target(clang-format
						COMMAND ${${PROJECT_NAME}_CLANG_FORMAT_BINARY}
						-i ${SOURCES} ${HEADERS}
                        --verbose
          				WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
                        COMMENT "Formatting with ${${PROJECT_NAME}_CLANG_FORMAT_BINARY} ${SOURCES}..."
                 )
			endif()

			message(STATUS "Format the project using the `clang-format` target (i.e: cmake --build build --target clang-format).\n")
    endif()
endfunction()

add_clang_format_target()

