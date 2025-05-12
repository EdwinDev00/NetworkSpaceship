
#get_filename_component(flatcFolderName "flatc.exe" DIRECTORY)
get_filename_component(FLATC flatc.exe REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
if ("${FLATC}" STREQUAL "")
	message(FATAL_ERROR "Could not find flatc.exe!")
else()
	message(STATUS "Using flatc located at ${FLATC}")
endif()
set_property(GLOBAL PROPERTY FLAT_COMPILER "${FLATC}")

function(flat_compile)
	get_property(FLATC GLOBAL PROPERTY FLAT_COMPILER)
	set(files)
	set(fbs "")
    foreach(fb ${ARGN})
        get_filename_component(filename ${fb} NAME)
        get_filename_component(foldername ${fb} DIRECTORY)
        string(REPLACE ".fbs" ".h" out_header ${filename})
       
        set(abs_output_folder "${CMAKE_BINARY_DIR}/generated/flat${foldername}")
        list(APPEND fbs ${fb})
        set(output ${abs_output_folder}/${out_header})
		add_custom_command(OUTPUT ${output}
                PRE_BUILD COMMAND ${FLATC} -c --gen-object-api --gen-mutable --include-prefix "" --keep-prefix --cpp-str-flex-ctor -I ${CMAKE_CURRENT_SOURCE_DIR} --filename-suffix "" -o ${abs_output_folder} "${CMAKE_CURRENT_SOURCE_DIR}/${fb}"
                MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/${fb}"
                DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${fb}"
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Compiling ${fb}"
                VERBATIM
			)
        list(APPEND files ${output})
		
		endforeach()
	set(files_proto ${files} PARENT_SCOPE)
	source_group("generated" FILES ${files})
	source_group("protocol" FILES ${fbs})

endfunction()
