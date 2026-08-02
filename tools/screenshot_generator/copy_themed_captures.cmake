# Copies the themed publish pass's captures into the tracked gallery, failing
# loudly when one of them was not produced.
#
# The dark pass renders into a scratch dir rather than straight over the
# gallery for one reason: a capture can decline to render under --theme (see
# CaptureSettingsDialog, which skips by design), and when it does the generator
# exits 0 having written nothing. Rendering in place would then leave the
# legacy PNG from the first pass sitting there, and the manual would ship one
# light image among the dark ones with no error anywhere — the same class of
# silent-overwrite bug that produced a blank series-inspector.png. Copying from
# a scratch dir turns that into a configure-time-visible build failure naming
# the file.

set(required_vars
  SCADA_THEMED_SRC_DIR
  SCADA_THEMED_DEST_DIR
  SCADA_THEMED_FILES
)

foreach(var IN LISTS required_vars)
  if(NOT DEFINED ${var} OR "${${var}}" STREQUAL "")
    message(FATAL_ERROR "${var} must be provided")
  endif()
endforeach()

# Passed as one comma-separated string so it survives -D without list-escaping.
string(REPLACE "," ";" themed_files "${SCADA_THEMED_FILES}")

set(missing)
foreach(filename IN LISTS themed_files)
  if(NOT EXISTS "${SCADA_THEMED_SRC_DIR}/${filename}")
    list(APPEND missing "${filename}")
  endif()
endforeach()

if(missing)
  string(REPLACE ";" ", " missing_text "${missing}")
  message(FATAL_ERROR
    "The themed publish pass produced no output for: ${missing_text}\n"
    "Each of these is in current_generator_owned_subset without "
    "\"publish_theme\": \"legacy\", so it is expected to render under "
    "--theme=dark. Either its capture skips when themed — in which case mark "
    "the manifest entry \"publish_theme\": \"legacy\" and say why — or the "
    "capture failed and the generator log above says how.")
endif()

foreach(filename IN LISTS themed_files)
  file(COPY_FILE
    "${SCADA_THEMED_SRC_DIR}/${filename}"
    "${SCADA_THEMED_DEST_DIR}/${filename}"
    ONLY_IF_DIFFERENT)
  message(STATUS "Themed publish render: ${filename}")
endforeach()
