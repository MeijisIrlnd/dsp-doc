# List all subdirs within a given directory
# USAGE: list_subdirs(output dir_to_list)
function(list_subdirs result dirtolist)
    file(GLOB children RELATIVE ${dirtolist} ${dirtolist}/*)
    set(DIR_LIST "")
    foreach(child ${children})
        if(IS_DIRECTORY ${dirtolist}/${child})
            list(APPEND dirlist ${child})
        endif()
    endforeach ()
    # a function creates a new scope, so use PARENT_SCOPE to circumvent that..
    set(${result} ${dirlist} PARENT_SCOPE)
endfunction()

# Find all cpp files within a given directory (non recursively)
# USAGE: find_sources(source_list dir_to_list)
function(find_sources result dirtolist)
    file(GLOB sources ${dirtolist}/*.cpp)
    set(${result} ${sources} PARENT_SCOPE)
endfunction()

# Only enabled on macos
if(APPLE)
    # Find the system's architecture
    # USAGE: find_macos_arch(detected_arch)
    function(find_macos_arch result)
        EXECUTE_PROCESS(COMMAND uname -m COMMAND tr -d '\n' OUTPUT_VARIABLE detected_arch)
        set(${result} ${detected_arch} PARENT_SCOPE)
    endfunction()
endif()