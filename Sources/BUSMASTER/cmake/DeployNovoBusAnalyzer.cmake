if(NOT DEPLOY_DIR OR NOT CONFIG OR NOT RUNTIME_FILES OR NOT CONVERTER_PLUGIN_FILES)
  message(FATAL_ERROR "NovoBusAnalyzer deployment arguments are incomplete")
endif()

file(MAKE_DIRECTORY "${DEPLOY_DIR}")

file(GLOB _old_dlls "${DEPLOY_DIR}/*.dll")
if(_old_dlls)
  file(REMOVE ${_old_dlls})
endif()

foreach(_plugin_dir platforms styles tls networkinformation generic iconengines imageformats)
  file(REMOVE_RECURSE "${DEPLOY_DIR}/${_plugin_dir}")
endforeach()

function(copy_listed_files list_file destination)
  file(STRINGS "${list_file}" _files)
  file(MAKE_DIRECTORY "${destination}")
  foreach(_file IN LISTS _files)
    if(NOT EXISTS "${_file}")
      message(FATAL_ERROR "Runtime file does not exist: ${_file}")
    endif()
    file(COPY "${_file}" DESTINATION "${destination}")
  endforeach()
endfunction()

copy_listed_files("${RUNTIME_FILES}" "${DEPLOY_DIR}")
file(RENAME "${DEPLOY_DIR}/LIN_ETAS_BOA_1_5.dll" "${DEPLOY_DIR}/LIN_ETAS_BOA.dll")

set(_converter_plugin_dir "${DEPLOY_DIR}/ConverterPlugins")
file(REMOVE_RECURSE "${_converter_plugin_dir}")
copy_listed_files("${CONVERTER_PLUGIN_FILES}" "${_converter_plugin_dir}")

if(CONFIG STREQUAL "Debug")
  set(_vcpkg_bin_dir "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/bin")
  set(_qt_config_argument --debug)
else()
  set(_vcpkg_bin_dir "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")
  set(_qt_config_argument --release)
endif()

file(GLOB _vcpkg_dlls "${_vcpkg_bin_dir}/*.dll")
if(NOT _vcpkg_dlls)
  message(FATAL_ERROR "No vcpkg runtime DLLs found in ${_vcpkg_bin_dir}")
endif()
file(COPY ${_vcpkg_dlls} DESTINATION "${DEPLOY_DIR}")

file(STRINGS "${QT_DEPLOY_INPUTS}" _qt_deploy_inputs)
execute_process(
  COMMAND "${WINDEPLOYQT_EXECUTABLE}"
    ${_qt_config_argument}
    --compiler-runtime
    --no-translations
    --dir "${DEPLOY_DIR}"
    ${_qt_deploy_inputs}
  RESULT_VARIABLE _qt_deploy_result
)
if(NOT _qt_deploy_result EQUAL 0)
  message(FATAL_ERROR "windeployqt failed with exit code ${_qt_deploy_result}")
endif()

file(COPY "${SPLASH_BITMAP}" DESTINATION "${DEPLOY_DIR}")
