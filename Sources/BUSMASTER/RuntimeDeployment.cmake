set(NOVO_RUNTIME_TARGETS
  AdvancedUIPlugin
  BusmasterDriverInterface
  BusmasterKernel
  DIL_J1939
  CAN_ETAS_BOA_1_4
  CAN_ETAS_BOA_1_5
  CAN_ETAS_BOA_2
  CAN_ICS_neoVI
  CAN_ISOLAR_EVE_VCAN
  CAN_i-VIEW
  CAN_IXXAT_VCI
  CAN_Kvaser_CAN
  CAN_STUB
  CAN_Vector_XL
  DMGraph
  Filter
  FrameProcessor
  LIN_ETAS_BOA_1_5
  LIN_ISOLAR_EVE_VLIN
  LIN_Kvaser
  LIN_PEAK_USB
  LIN_Vector_XL
  NodeSimEx
  ProjectConfiguration
  PSDI_CAN
  Replay
  SigGrphWnd
  SignalDefiner
  SignalWatch
  TestSuiteExecutorGUI
  TXWindow
  UDS_Protocol
)

set(NOVO_COMPANION_TARGETS
  BusEmulation
  FormatConverter
  TestSetupEditorGUI
)

if(BUILD_LDFEDITOR)
  list(APPEND NOVO_COMPANION_TARGETS LDFEditor)
endif()

if(BUILD_LDFVIEWER)
  list(APPEND NOVO_COMPANION_TARGETS LDFViewer)
endif()

set(NOVO_CONVERTER_PLUGIN_TARGETS
  AscLogConverter
  BlfLibrary
  BlfLogConverter
  CAPL2CConverter
  DBC2DBFConverter
  DBC2DBFConverterLibrary
  DBF2DBCConverter
  J1939DBC2DBFConverter
  LogAscConverter
  LogToExcelConverter
)

find_program(WINDEPLOYQT_EXECUTABLE
  NAMES windeployqt windeployqt6
  HINTS "${Qt6_DIR}/../../../bin"
  REQUIRED
)

if(NOT VCPKG_INSTALLED_DIR)
  set(VCPKG_INSTALLED_DIR "${_VCPKG_INSTALLED_DIR}")
endif()

set(_runtime_files)
foreach(_target IN LISTS NOVO_RUNTIME_TARGETS NOVO_COMPANION_TARGETS)
  list(APPEND _runtime_files "$<TARGET_FILE:${_target}>")
endforeach()
string(JOIN "\n" _runtime_file_content ${_runtime_files})

set(_converter_plugin_files)
foreach(_target IN LISTS NOVO_CONVERTER_PLUGIN_TARGETS)
  list(APPEND _converter_plugin_files "$<TARGET_FILE:${_target}>")
endforeach()
string(JOIN "\n" _converter_plugin_file_content ${_converter_plugin_files})

set(_qt_deploy_inputs
)
if(BUILD_LDFEDITOR)
  list(APPEND _qt_deploy_inputs "$<TARGET_FILE:LDFEditor>")
endif()
if(BUILD_LDFVIEWER)
  list(APPEND _qt_deploy_inputs "$<TARGET_FILE:LDFViewer>")
endif()
string(JOIN "\n" _qt_deploy_input_content ${_qt_deploy_inputs})

file(GENERATE
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/runtime-files-$<CONFIG>.txt"
  CONTENT "${_runtime_file_content}\n"
)
file(GENERATE
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/converter-plugin-files-$<CONFIG>.txt"
  CONTENT "${_converter_plugin_file_content}\n"
)
file(GENERATE
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/qt-deploy-inputs-$<CONFIG>.txt"
  CONTENT "${_qt_deploy_input_content}\n"
)

set(_powershell_exe "C:/Windows/System32/WindowsPowerShell/v1.0/powershell.exe")

add_custom_target(deploy_runtime ALL
  COMMAND ${CMAKE_COMMAND}
    "-DDEPLOY_DIR=$<TARGET_FILE_DIR:NovoBusAnalyzer>"
    "-DCONFIG=$<CONFIG>"
    "-DRUNTIME_FILES=${CMAKE_CURRENT_BINARY_DIR}/runtime-files-$<CONFIG>.txt"
    "-DCONVERTER_PLUGIN_FILES=${CMAKE_CURRENT_BINARY_DIR}/converter-plugin-files-$<CONFIG>.txt"
    "-DQT_DEPLOY_INPUTS=${CMAKE_CURRENT_BINARY_DIR}/qt-deploy-inputs-$<CONFIG>.txt"
    "-DVCPKG_INSTALLED_DIR=${VCPKG_INSTALLED_DIR}"
    "-DVCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET}"
    "-DWINDEPLOYQT_EXECUTABLE=${WINDEPLOYQT_EXECUTABLE}"
    "-DSPLASH_BITMAP=${CMAKE_CURRENT_SOURCE_DIR}/Application/Res/Splsh16.bmp"
    "-DHELP_FILE=${CMAKE_CURRENT_SOURCE_DIR}/BIN/Release/BUSMASTER.chm"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/DeployNovoBusAnalyzer.cmake"
  COMMAND "${_powershell_exe}" -NoProfile -ExecutionPolicy Bypass
    -File "${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyX64Runtime.ps1"
    "$<TARGET_FILE_DIR:NovoBusAnalyzer>"
  DEPENDS
    NovoBusAnalyzer
    ${NOVO_RUNTIME_TARGETS}
    ${NOVO_COMPANION_TARGETS}
    ${NOVO_CONVERTER_PLUGIN_TARGETS}
  COMMENT "Deploying x64 NovoBusAnalyzer runtime for $<CONFIG>"
  VERBATIM
)
