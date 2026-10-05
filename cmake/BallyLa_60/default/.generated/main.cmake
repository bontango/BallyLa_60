include("${CMAKE_CURRENT_LIST_DIR}/rule.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/file.cmake")

set(BallyLa_60_default_library_list )

# Handle files with suffix (s|as|asm|AS|ASM|As|aS|Asm), for group default-XC8
if(BallyLa_60_default_default_XC8_FILE_TYPE_assemble)
add_library(BallyLa_60_default_default_XC8_assemble OBJECT ${BallyLa_60_default_default_XC8_FILE_TYPE_assemble})
    BallyLa_60_default_default_XC8_assemble_rule(BallyLa_60_default_default_XC8_assemble)
    list(APPEND BallyLa_60_default_library_list "$<TARGET_OBJECTS:BallyLa_60_default_default_XC8_assemble>")

endif()

# Handle files with suffix S, for group default-XC8
if(BallyLa_60_default_default_XC8_FILE_TYPE_assemblePreprocess)
add_library(BallyLa_60_default_default_XC8_assemblePreprocess OBJECT ${BallyLa_60_default_default_XC8_FILE_TYPE_assemblePreprocess})
    BallyLa_60_default_default_XC8_assemblePreprocess_rule(BallyLa_60_default_default_XC8_assemblePreprocess)
    list(APPEND BallyLa_60_default_library_list "$<TARGET_OBJECTS:BallyLa_60_default_default_XC8_assemblePreprocess>")

endif()

# Handle files with suffix [cC], for group default-XC8
if(BallyLa_60_default_default_XC8_FILE_TYPE_compile)
add_library(BallyLa_60_default_default_XC8_compile OBJECT ${BallyLa_60_default_default_XC8_FILE_TYPE_compile})
    BallyLa_60_default_default_XC8_compile_rule(BallyLa_60_default_default_XC8_compile)
    list(APPEND BallyLa_60_default_library_list "$<TARGET_OBJECTS:BallyLa_60_default_default_XC8_compile>")

endif()


# Main target for this project
add_executable(BallyLa_60_default_image_I4zBGaPE ${BallyLa_60_default_library_list})

set_target_properties(BallyLa_60_default_image_I4zBGaPE PROPERTIES
    OUTPUT_NAME "default"
    SUFFIX ".elf"
    ADDITIONAL_CLEAN_FILES "${output_extensions}"
    RUNTIME_OUTPUT_DIRECTORY "${BallyLa_60_default_output_dir}")
target_link_libraries(BallyLa_60_default_image_I4zBGaPE PRIVATE ${BallyLa_60_default_default_XC8_FILE_TYPE_link})

# Add the link options from the rule file.
BallyLa_60_default_link_rule( BallyLa_60_default_image_I4zBGaPE)


