if(NOT DEFINED BERZMOW_STAGED_DIR)
    message(FATAL_ERROR "BERZMOW_STAGED_DIR is required")
endif()

set(vst3 "${BERZMOW_STAGED_DIR}/VST3/Berzmow.vst3")
if(BERZMOW_EXPECT_AU)
    set(standalone "${BERZMOW_STAGED_DIR}/Standalone/Berzmow.app")
elseif(WIN32)
    set(standalone "${BERZMOW_STAGED_DIR}/Standalone/Berzmow.exe")
else()
    set(standalone "${BERZMOW_STAGED_DIR}/Standalone/Berzmow")
endif()

if(NOT EXISTS "${vst3}")
    message(FATAL_ERROR "Missing staged VST3: ${vst3}")
endif()

if(NOT EXISTS "${standalone}")
    message(FATAL_ERROR "Missing staged Standalone app: ${standalone}")
endif()

if(BERZMOW_EXPECT_AU)
    set(au "${BERZMOW_STAGED_DIR}/AU/Berzmow.component")
    if(NOT EXISTS "${au}")
        message(FATAL_ERROR "Missing staged AU: ${au}")
    endif()

    foreach(bundle IN ITEMS "${vst3}" "${standalone}" "${au}")
        execute_process(
            COMMAND codesign --verify --deep --strict "${bundle}"
            RESULT_VARIABLE codesign_result
            ERROR_VARIABLE codesign_error)
        if(NOT codesign_result EQUAL 0)
            message(FATAL_ERROR "Invalid signature for ${bundle}: ${codesign_error}")
        endif()
    endforeach()
endif()
