# Fetches the DuskLight source tree pinned by DUSKLIGHT_VERSION.
include_guard(GLOBAL)

set(DUSKLIGHT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dusklight"
    CACHE PATH "Path to the DuskLight source tree")
set(DUSKLIGHT_REPOSITORY "https://github.com/TwilitRealm/dusklight.git"
    CACHE STRING "DuskLight git repository to fetch from")

function(_exec_git)
    execute_process(COMMAND "${GIT_EXECUTABLE}" ${ARGN}
        WORKING_DIRECTORY "${DUSKLIGHT_DIR}"
        RESULT_VARIABLE _result)
    if (NOT _result EQUAL 0)
        message(FATAL_ERROR "Git command failed while fetching DuskLight ${DUSKLIGHT_VERSION}")
    endif ()
endfunction()

set(_stamp "${DUSKLIGHT_DIR}/.stamp")
if (EXISTS "${DUSKLIGHT_DIR}/sdk/CMakeLists.txt" AND NOT EXISTS "${_stamp}")
    message(STATUS "Using existing DuskLight checkout at ${DUSKLIGHT_DIR}")
else ()
    if (NOT DUSKLIGHT_VERSION)
        message(FATAL_ERROR "DUSKLIGHT_VERSION is not set")
    endif ()

    set(_fetched "")
    if (EXISTS "${_stamp}")
        file(READ "${_stamp}" _fetched)
        string(STRIP "${_fetched}" _fetched)
    endif ()

    if (NOT _fetched STREQUAL DUSKLIGHT_VERSION)
        find_package(Git REQUIRED)
        file(MAKE_DIRECTORY "${DUSKLIGHT_DIR}")
        if (NOT EXISTS "${DUSKLIGHT_DIR}/.git")
            _exec_git(init --quiet)
            _exec_git(remote add origin "${DUSKLIGHT_REPOSITORY}")
        endif ()
        _exec_git(fetch --depth 1 "${DUSKLIGHT_REPOSITORY}" "${DUSKLIGHT_VERSION}")
        _exec_git(-c advice.detachedHead=false checkout --force FETCH_HEAD)
        _exec_git(submodule update --init --depth 1 extern/aurora)
        file(WRITE "${_stamp}" "${DUSKLIGHT_VERSION}\n")
    endif ()
endif ()
