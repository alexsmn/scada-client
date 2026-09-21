# Writes the About dialog's build stamp: the commit this tree is at, and that
# COMMIT's date.
#
# Run with `cmake -P` from a custom target, so the stamp is refreshed on every
# build rather than frozen at the last configure — a stamp that is three days
# stale is worse than none, because nothing about it says so. The write goes
# through `copy_if_different`, so a rebuild that changes no commit recompiles
# nothing.
#
# The date is the commit's, never `string(TIMESTAMP)`: two builds of one commit
# have to describe themselves identically, which is the property that makes the
# stamp worth quoting back in a fault report.
#
# Expects SOURCE_DIR, TEMPLATE and OUTPUT on the command line.

set(SCADA_BUILD_COMMIT "")
set(SCADA_BUILD_DATE "")

find_package(Git QUIET)
if(GIT_EXECUTABLE)
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" rev-parse --short HEAD
    WORKING_DIRECTORY "${SOURCE_DIR}"
    OUTPUT_VARIABLE _commit
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE _commit_result)
  if(_commit_result EQUAL 0)
    set(SCADA_BUILD_COMMIT "${_commit}")
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" log -1 --format=%cI
      WORKING_DIRECTORY "${SOURCE_DIR}"
      OUTPUT_VARIABLE _date
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
      RESULT_VARIABLE _date_result)
    if(_date_result EQUAL 0)
      set(SCADA_BUILD_DATE "${_date}")
    endif()
  endif()
endif()

configure_file("${TEMPLATE}" "${OUTPUT}.tmp" @ONLY)
execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                        "${OUTPUT}.tmp" "${OUTPUT}")
