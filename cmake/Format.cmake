function(add_clang_format_target TARGET_NAME)
    find_program(CLANG_FORMAT_EXE 
        NAMES clang-format 
        DOC "Path to clang-format executable"
    )

    if(NOT CLANG_FORMAT_EXE)
        message(WARNING "clang-format not found. Formatting disabled for ${TARGET_NAME}.")
        return()
    endif()

    set(FORMAT_FILES "")

    # 1. Извлекаем обычные исходники (.cpp, .h)
    get_target_property(TARGET_SOURCES ${TARGET_NAME} SOURCES)
    if(TARGET_SOURCES)
        foreach(FILE IN LISTS TARGET_SOURCES)
            if(FILE MATCHES "\\.(c|cc|cpp|cxx|h|hpp)$")
                get_filename_component(ABS_FILE "${FILE}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
                list(APPEND FORMAT_FILES "${ABS_FILE}")
            endif()
        endforeach()
    endif()

    get_target_property(MODULE_SETS ${TARGET_NAME} CXX_MODULE_SETS)
    if(MODULE_SETS)
        foreach(MOD_SET IN LISTS MODULE_SETS)
            get_target_property(MOD_FILES ${TARGET_NAME} CXX_MODULE_SET_${MOD_SET})
            if(MOD_FILES)
                foreach(FILE IN LISTS MOD_FILES)
                    get_filename_component(ABS_FILE "${FILE}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
                    list(APPEND FORMAT_FILES "${ABS_FILE}")
                endforeach()
            endif()
        endforeach()
    endif()

    if(FORMAT_FILES)
        list(REMOVE_DUPLICATES FORMAT_FILES)
        
        add_custom_target(format-${TARGET_NAME}
            COMMAND ${CLANG_FORMAT_EXE} -i -style=file ${FORMAT_FILES}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Formatting target '${TARGET_NAME}'..."
            VERBATIM
        )

        if(NOT TARGET format)
            add_custom_target(format COMMENT "Running clang-format on all targets...")
        endif()
        add_dependencies(format format-${TARGET_NAME})
    endif()
endfunction()