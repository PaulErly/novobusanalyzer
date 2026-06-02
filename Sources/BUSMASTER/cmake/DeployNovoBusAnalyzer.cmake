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
  set(_vc_runtime_dirs
    "C:/Program Files (x86)/Microsoft Visual Studio/18/BuildTools/VC/Redist/MSVC/14.51.36231/debug_nonredist/x64/Microsoft.VC145.DebugCRT"
    "C:/Program Files (x86)/Microsoft Visual Studio/18/BuildTools/VC/Redist/MSVC/14.51.36231/debug_nonredist/x64/Microsoft.VC145.DebugMFC"
  )
  set(_qt_config_argument --debug)
else()
  set(_vcpkg_bin_dir "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")
  set(_vc_runtime_dirs
    "C:/Program Files (x86)/Microsoft Visual Studio/18/BuildTools/VC/Redist/MSVC/14.51.36231/x64/Microsoft.VC145.CRT"
    "C:/Program Files (x86)/Microsoft Visual Studio/18/BuildTools/VC/Redist/MSVC/14.51.36231/x64/Microsoft.VC145.MFC"
  )
  set(_qt_config_argument --release)
endif()

file(GLOB _vcpkg_dlls "${_vcpkg_bin_dir}/*.dll")
if(NOT _vcpkg_dlls)
  message(FATAL_ERROR "No vcpkg runtime DLLs found in ${_vcpkg_bin_dir}")
endif()
file(COPY ${_vcpkg_dlls} DESTINATION "${DEPLOY_DIR}")

set(_vc_runtime_copied FALSE)
foreach(_vc_runtime_dir IN LISTS _vc_runtime_dirs)
  file(GLOB _vc_runtime_dlls "${_vc_runtime_dir}/*.dll")
  if(_vc_runtime_dlls)
    file(COPY ${_vc_runtime_dlls} DESTINATION "${DEPLOY_DIR}")
    set(_vc_runtime_copied TRUE)
  endif()
endforeach()
if(NOT _vc_runtime_copied)
  message(WARNING "No VC runtime DLLs found in the configured redist folders")
endif()

if(CONFIG STREQUAL "Debug")
  set(_ucrt_debug_dll "C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/ucrt/ucrtbased.dll")
  if(EXISTS "${_ucrt_debug_dll}")
    file(COPY "${_ucrt_debug_dll}" DESTINATION "${DEPLOY_DIR}")
  else()
    message(WARNING "ucrtbased.dll was not found at ${_ucrt_debug_dll}")
  endif()
endif()

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
  message(WARNING "windeployqt failed with exit code ${_qt_deploy_result}; continuing with the copied runtime DLL set")
endif()

file(COPY "${SPLASH_BITMAP}" DESTINATION "${DEPLOY_DIR}")

get_filename_component(_deploy_script_dir "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
if(DEFINED HELP_FILE AND EXISTS "${HELP_FILE}")
  file(COPY "${HELP_FILE}" DESTINATION "${DEPLOY_DIR}")
endif()

set(_help_file "${_deploy_script_dir}/../BIN/Release/BUSMASTER.chm")
if(EXISTS "${_help_file}")
  file(COPY "${_help_file}" DESTINATION "${DEPLOY_DIR}")
endif()

# The CAN simulation adapter activates this ATL local server by CLSID. Register
# the deployed x64 copy per-user so local builds do not require elevation.
set(_bus_emulation_exe "${DEPLOY_DIR}/BusEmulation.exe")
function(register_bus_emulation_value key)
  execute_process(
    COMMAND reg.exe ADD "${key}" ${ARGN} /f
    RESULT_VARIABLE _registration_result
    OUTPUT_QUIET
    ERROR_QUIET
  )
  if(NOT _registration_result EQUAL 0)
    message(WARNING "Failed to register the deployed x64 BusEmulation COM server; continuing without per-user COM registration")
  endif()
endfunction()

set(_bus_emulation_clsid "{5983045C-CA05-47CF-8CD8-86A4445FB48C}")
set(_bus_emulation_iid "{32610836-D66A-4A57-83D5-EA50ECB0B7BB}")
set(_bus_emulation_libid "{1C499499-5718-4CB9-BEE9-6E3FEF24F5C7}")
set(_automation_marshaler_clsid "{00020424-0000-0000-C000-000000000046}")

register_bus_emulation_value(
  "HKCU\\Software\\Classes\\CLSID\\${_bus_emulation_clsid}\\LocalServer32"
  /ve /d "\"${_bus_emulation_exe}\""
)
register_bus_emulation_value(
  "HKCU\\Software\\Classes\\Interface\\${_bus_emulation_iid}"
  /ve /d "ISimENG"
)
register_bus_emulation_value(
  "HKCU\\Software\\Classes\\Interface\\${_bus_emulation_iid}\\ProxyStubClsid32"
  /ve /d "${_automation_marshaler_clsid}"
)
register_bus_emulation_value(
  "HKCU\\Software\\Classes\\Interface\\${_bus_emulation_iid}\\TypeLib"
  /ve /d "${_bus_emulation_libid}"
)
register_bus_emulation_value(
  "HKCU\\Software\\Classes\\Interface\\${_bus_emulation_iid}\\TypeLib"
  /v Version /d "1.1"
)
register_bus_emulation_value(
  "HKCU\\Software\\Classes\\TypeLib\\${_bus_emulation_libid}\\1.1\\0\\win64"
  /ve /d "${_bus_emulation_exe}"
)
register_bus_emulation_value(
  "HKCU\\Software\\Classes\\TypeLib\\${_bus_emulation_libid}\\1.1\\FLAGS"
  /ve /d "0"
)
