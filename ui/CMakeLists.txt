# Builds each frontend with Vite, zips it, and embeds the zip as JUCE binary data.
#
#   giclee_ui_sampler / giclee_ui_player   binary data targets exposing GicleeUI::ui_zip
#
# With GICLEE_BUILD_UI=OFF a placeholder page is embedded instead, so C++-only builds work
# without Node.js.

set(GICLEE_UI_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

if(GICLEE_BUILD_UI)
    find_program(GICLEE_NPM NAMES npm.cmd npm)

    if(NOT GICLEE_NPM)
        message(FATAL_ERROR "npm not found. Install Node.js 22+, or configure with -DGICLEE_BUILD_UI=OFF.")
    endif()

    file(GLOB_RECURSE GICLEE_UI_SOURCES CONFIGURE_DEPENDS "${GICLEE_UI_DIR}/src/*")
    set(GICLEE_UI_CONFIG
        "${GICLEE_UI_DIR}/package.json"
        "${GICLEE_UI_DIR}/package-lock.json"
        "${GICLEE_UI_DIR}/vite.config.ts"
        "${GICLEE_UI_DIR}/svelte.config.js"
        "${GICLEE_UI_DIR}/tsconfig.json")

    set(GICLEE_NPM_STAMP "${CMAKE_CURRENT_BINARY_DIR}/npm-ci.stamp")

    add_custom_command(
        OUTPUT "${GICLEE_NPM_STAMP}"
        COMMAND "${GICLEE_NPM}" ci --no-audit --no-fund
        COMMAND "${CMAKE_COMMAND}" -E touch "${GICLEE_NPM_STAMP}"
        WORKING_DIRECTORY "${GICLEE_UI_DIR}"
        DEPENDS "${GICLEE_UI_DIR}/package.json" "${GICLEE_UI_DIR}/package-lock.json"
        COMMENT "Installing frontend dependencies (npm ci)"
        VERBATIM)
endif()

function(giclee_add_ui app)
    set(out_dir "${CMAKE_CURRENT_BINARY_DIR}/dist/${app}")
    set(zip_dir "${CMAKE_CURRENT_BINARY_DIR}/zip/${app}")
    set(zip "${zip_dir}/ui.zip")

    if(GICLEE_BUILD_UI)
        add_custom_command(
            OUTPUT "${zip}"
            COMMAND "${CMAKE_COMMAND}" -E env "GICLEE_UI_OUT_DIR=${out_dir}" "${GICLEE_NPM}" run "build:${app}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${zip_dir}"
            COMMAND "${CMAKE_COMMAND}" -E rm -f "${zip}"
            COMMAND "${CMAKE_COMMAND}" -E chdir "${out_dir}" "${CMAKE_COMMAND}" -E tar cf "${zip}" --format=zip index.html assets
            WORKING_DIRECTORY "${GICLEE_UI_DIR}"
            DEPENDS "${GICLEE_NPM_STAMP}" ${GICLEE_UI_SOURCES} ${GICLEE_UI_CONFIG}
            COMMENT "Building ${app} frontend"
            VERBATIM)
    else()
        add_custom_command(
            OUTPUT "${zip}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${zip_dir}"
            COMMAND "${CMAKE_COMMAND}" -E rm -f "${zip}"
            COMMAND "${CMAKE_COMMAND}" -E chdir "${GICLEE_UI_DIR}/fallback" "${CMAKE_COMMAND}" -E tar cf "${zip}" --format=zip index.html
            DEPENDS "${GICLEE_UI_DIR}/fallback/index.html"
            COMMENT "Packaging placeholder ${app} frontend"
            VERBATIM)
    endif()

    juce_add_binary_data(giclee_ui_${app}
        NAMESPACE GicleeUI
        HEADER_NAME GicleeUI.h
        SOURCES "${zip}")

    set_target_properties(giclee_ui_${app} PROPERTIES FOLDER "ui")
endfunction()

giclee_add_ui(sampler)
giclee_add_ui(player)
