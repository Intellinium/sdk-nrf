# Copyright (c) 2019-2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

# Builds combined documentation for all documentation sets: nRF (including
# Doxygen documentation), Zephyr, MCUboot, etc.
#
# We use our own Sphinx configuration files when building the documentation set
# for each repository, instead of reusing configuration files. See e.g.
# doc/nrf/conf.py and doc/zephyr/conf.py.
#
# Intersphinx (http://www.sphinx-doc.org/en/master/usage/extensions/intersphinx.html)
# is used to link documentation sets together. It is configured in the Sphinx
# configuration files.

cmake_minimum_required(VERSION 3.15)
project(nrf-connect-sdk-doc LANGUAGES NONE)

set(NO_BOILERPLATE TRUE)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE} ..)

#-------------------------------------------------------------------------------
# Options

set(SPHINXOPTS_DEFAULT -j auto CACHE INTERNAL "Default Sphinx Options")
set(SPHINXOPTS_EXTRA "" CACHE STRING "Extra Sphinx Options")

#-------------------------------------------------------------------------------
# Dependencies

find_package(PythonInterp 3.4)
set(DOXYGEN_SKIP_DOT True)
find_package(Doxygen REQUIRED)

find_program(SPHINXBUILD sphinx-build)
if(NOT SPHINXBUILD)
  message(FATAL_ERROR "The 'sphinx-build' command was not found")
endif()

find_program(WEST west)
if(NOT WEST)
  message(FATAL_ERROR "The 'west' command was not found")
endif()

set(KCONFIG_BINARY_DIR ${CMAKE_BINARY_DIR}/kconfig)
list(INSERT MODULE_EXT_ROOT 0 ${ZEPHYR_BASE})
file(MAKE_DIRECTORY ${KCONFIG_BINARY_DIR})

include(${ZEPHYR_BASE}/cmake/extensions.cmake)
include(${ZEPHYR_BASE}/cmake/zephyr_module.cmake)
# -- Intellinium addition
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/GitUtils.cmake)

#-------------------------------------------------------------------------------
# Functions

# -- Intellinium addition
# Add an external docset
#
# Args:
# - name: Docset name.
# - env: Environment passed to the Sphinx build.
# - repo: The repository of the project
# - branch: The branch of the repo to clone
#
# This function adds a docset whose documentation content is located on a
# separate git repository.
# The content of the repository will be cloned or updated on the configuration
# of the cmake project.
#
# The cloned repository will be located under the {NRF_BASE}/_external/ folder
# which is excluded from git.
#
function(add_external_docset name env repo branch)
  file(MAKE_DIRECTORY ${DOCSET_EXT_DIR})

  git_clone(
    PROJECT_NAME    ${name}
    GIT_URL         ${repo}
    GIT_BRANCH      ${branch}
    DIRECTORY       ${DOCSET_EXT_DIR}
  )

  add_docset(${name} "${env}")
endfunction()

# Add a new docset.
#
# Args:
# - name: Docset name.
# - env: Environment passed to the Sphinx build.
#
# This function configures multiple targets which can be used to build a docset.
# The docset configuration (conf.py) is expected to be at the ${name} folder
# (relative to the current directory). The sources are taken from the
# ${name}/src folder (relative to the build directory). This means that docsets
# need to make use of the external_content extension in order to gather all
# docset sources into that folder.
#
# Configured targets:
# - ${name}: Alias for ${name}-html.
# - ${name}-inventory: Run Sphinx "inventory" build. It requires to enable
#   the "inventory" builder on the docset conf.py. This target can be used
#   to solve circular dependencies between docsets.
# - ${name}-html: Run Sphinx "html" build.
# - ${name}-linkcheck: Run Sphinx "linkcheck" target.
# - ${name}-clean: Clean build artifacts.
#
function(add_docset name env)
  set(DOCSET_CFG_DIR ${CMAKE_CURRENT_LIST_DIR}/${name})
  set(DOCSET_BUILD_DIR ${CMAKE_BINARY_DIR}/${name})
  set(DOCSET_SRC_DIR ${CMAKE_BINARY_DIR}/${name}/src)
  set(DOCSET_HTML_DIR ${CMAKE_BINARY_DIR}/html/${name})
  set(DOCSET_MAKE_DIRS ${DOCSET_BUILD_DIR};${DOCSET_SRC_DIR})
  set(DOCSET_CLEAN_DIRS ${DOCSET_BUILD_DIR};${DOCSET_HTML_DIR})

  add_doc_target(
    ${name}-inventory
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DOCSET_MAKE_DIRS}
    COMMAND ${CMAKE_COMMAND} -E env ${env}
    ${SPHINXBUILD}
      -b inventory
      -c ${DOCSET_CFG_DIR}
      -w ${DOCSET_BUILD_DIR}/inventory.log
      ${SPHINXOPTS_DEFAULT}
      ${DOCSET_SRC_DIR}
      ${DOCSET_HTML_DIR}
    USES_TERMINAL
  )

  add_doc_target(
    ${name}-html
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DOCSET_MAKE_DIRS}
    COMMAND ${CMAKE_COMMAND} -E env ${env}
    ${SPHINXBUILD}
      -b html
      -c ${DOCSET_CFG_DIR}
      -w ${DOCSET_BUILD_DIR}/html.log
      ${SPHINXOPTS_DEFAULT}
      ${SPHINXOPTS_EXTRA}
      ${DOCSET_SRC_DIR}
      ${DOCSET_HTML_DIR}
    USES_TERMINAL
  )

  add_custom_target(
    ${name}-linkcheck
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DOCSET_MAKE_DIRS}
    COMMAND ${CMAKE_COMMAND} -E env ${env}
    ${SPHINXBUILD}
      -b linkcheck
      -c ${DOCSET_CFG_DIR}
      -w ${DOCSET_BUILD_DIR}/linkcheck.log
      ${SPHINXOPTS_DEFAULT}
      ${SPHINXOPTS_EXTRA}
      ${DOCSET_SRC_DIR}
      ${DOCSET_BUILD_DIR}
    USES_TERMINAL
  )

  set_target_properties(
    ${name}-inventory ${name}-inventory-all
    ${name}-html ${name}-html-all
    ${name}-linkcheck
    PROPERTIES
      ADDITIONAL_CLEAN_FILES "${DOCSET_CLEAN_DIRS}"
  )

  if(${CMAKE_VERSION} VERSION_LESS 3.17)
    add_custom_target(
      ${name}-clean
      COMMAND ${CMAKE_COMMAND} -E remove_directory ${DOCSET_CLEAN_DIRS}
    )
  else()
    add_custom_target(
      ${name}-clean
      COMMAND ${CMAKE_COMMAND} -E rm -rf ${DOCSET_CLEAN_DIRS}
    )
  endif()

  add_custom_target(${name})
  add_dependencies(${name} ${name}-html)
endfunction()

# Create a custom doc target.
#
# This function has the same signature as `add_custom_target()`
#
# The function will create two targets for the doc build system.
# - Target 1 named: `<name>`
# - Target 2 named: `<name>-all`
#
# Both targets will produce same result, but target 1 is useful when only
# wanting to build a subset of the docs and missing references to other targets
# are acceptable (warnings will be generated).
#
# Target 2 is used for complete docset builds where it is important that build
# order of each target is under full control.
#
function(add_doc_target name)
  add_custom_target(${name} ${ARGN})
  add_custom_target(${name}-all ${ARGN})
endfunction()

#-------------------------------------------------------------------------------
# Environment & Paths

# Set various *_BASE variables pointing to the nrf/, zephyr/, etc.,
# directories. Derive them automatically if they're not set in the environment,
# by assuming that e.g. nrfxlib can be found at ../../nrfxlib/. Also add them
# to the environment if they're not there.
#

get_filename_component(NRF_BASE ${CMAKE_CURRENT_LIST_DIR}../ DIRECTORY)
set(ENV{NRF_BASE} ${NRF_BASE})

# -- Intellinium addition
set(DOCSET_EXT_DIR ${CMAKE_CURRENT_LIST_DIR}/_external)

if(NOT DEFINED ENV{MCUBOOT_BASE})
  get_filename_component(MCUBOOT_BASE ${CMAKE_CURRENT_LIST_DIR}/../../bootloader/mcuboot/ REALPATH)
  set(ENV{MCUBOOT_BASE} ${MCUBOOT_BASE})
endif()

if(NOT DEFINED ENV{NRFXLIB_BASE})
  get_filename_component(NRFXLIB_BASE ${CMAKE_CURRENT_LIST_DIR}/../../nrfxlib/ REALPATH)
  set(ENV{NRFXLIB_BASE} ${NRFXLIB_BASE})
endif()

if(NOT DEFINED ENV{NRFX_BASE})
  get_filename_component(NRFX_BASE ${CMAKE_CURRENT_LIST_DIR}/../../modules/hal/nordic/nrfx/ REALPATH)
  set(ENV{NRFX_BASE} ${NRFX_BASE})
endif()

# -- Intellinium addition
if(NOT DEFINED ENV{ITL_CAR_POD_BASE})
  get_filename_component(ITL_CAR_POD_BASE ${DOCSET_EXT_DIR}/itl_car_pod/ REALPATH)
  set(ENV{ITL_CAR_POD_BASE} ${ITL_CAR_POD_BASE})
endif()

# -- Intellinium addition
if(NOT DEFINED ENV{ITL_LIB_BASE})
  get_filename_component(ITL_LIB_BASE ${CMAKE_CURRENT_LIST_DIR}/../../modules/itl-lib/ REALPATH)
  set(ENV{ITL_LIB_BASE} ${ITL_LIB_BASE})
endif()

# -- Intellinium addition
if(NOT DEFINED ENV{ITL_MAT_POD_BASE})
  get_filename_component(ITL_MAT_POD_BASE ${DOCSET_EXT_DIR}/itl_mat_pod/ REALPATH)
  set(ENV{ITL_MAT_POD_BASE} ${ITL_MAT_POD_BASE})
endif()

# -- Intellinium addition
if(NOT DEFINED ENV{ITL_TFL_BASE})
  get_filename_component(ITL_TFL_BASE ${DOCSET_EXT_DIR}/itl_tfl/ REALPATH)
  set(ENV{ITL_TFL_BASE} ${ITL_TFL_BASE})
endif()

message(STATUS "ZEPHYR_BASE: ${ZEPHYR_BASE}")
message(STATUS "NRF_BASE: $ENV{NRF_BASE}")
message(STATUS "MCUBOOT_BASE: $ENV{MCUBOOT_BASE}")
message(STATUS "NRFXLIB_BASE: $ENV{NRFXLIB_BASE}")
message(STATUS "NRFX_BASE: $ENV{NRFX_BASE}")
message(STATUS "ITL_CAR_POD_BASE: $ENV{ITL_CAR_POD_BASE}") # -- Intellinium addition
message(STATUS "ITL_LIB_BASE: $ENV{ITL_LIB_BASE}") # -- Intellinium addition
message(STATUS "ITL_MAT_POD_BASE: $ENV{ITL_MAT_POD_BASE}") # -- Intellinium addition
message(STATUS "ITL_TFL_BASE: $ENV{ITL_TFL_BASE}") # -- Intellinium addition

set(ZEPHYR_BINARY_DIR ${CMAKE_BINARY_DIR}/zephyr)
set(NRF_BINARY_DIR ${CMAKE_BINARY_DIR}/nrf)
set(NRFXLIB_BINARY_DIR ${CMAKE_BINARY_DIR}/nrfxlib)
set(NRFX_BINARY_DIR ${CMAKE_BINARY_DIR}/nrfx)
set(ITL_CAR_POD_BINARY_DIR ${CMAKE_BINARY_DIR}/itl_car_pod) # -- Intellinium addition
set(ITL_LIB_BINARY_DIR ${CMAKE_BINARY_DIR}/itl_lib) # -- Intellinium addition
set(ITL_MAT_POD_BINARY_DIR ${CMAKE_BINARY_DIR}/itl_mat_pod) # -- Intellinium addition
set(ITL_TFL_BINARY_DIR ${CMAKE_BINARY_DIR}/itl_tfl) # -- Intellinium addition

# HTML output directory
set(HTML_DIR ${CMAKE_BINARY_DIR}/html)
file(MAKE_DIRECTORY ${HTML_DIR})

#-------------------------------------------------------------------------------
# docset: Zephyr

add_custom_target(
  zephyr-devicetree
  COMMAND ${CMAKE_COMMAND} -E env
  PYTHONPATH=${ZEPHYR_BASE}/scripts/dts/python-devicetree/src
  ZEPHYR_BASE=${ZEPHYR_BASE}
  ${PYTHON_EXECUTABLE}
    ${ZEPHYR_BASE}/doc/_scripts/gen_devicetree_rest.py
    --vendor-prefixes ${ZEPHYR_BASE}/dts/bindings/vendor-prefixes.txt
    --dts-root ${ZEPHYR_BASE}
    ${ZEPHYR_BINARY_DIR}/src/reference/devicetree
  VERBATIM
  USES_TERMINAL
)

set(zephyr_env
  ZEPHYR_BASE=${ZEPHYR_BASE}
  ZEPHYR_BUILD=${ZEPHYR_BINARY_DIR}
  DOXYGEN_EXECUTABLE=${DOXYGEN_EXECUTABLE}
)

add_docset(zephyr "${zephyr_env}")
add_dependencies(zephyr-html zephyr-devicetree)
add_dependencies(zephyr-html-all zephyr-devicetree)

#-------------------------------------------------------------------------------
# docset: nrf

set(tools_version_files
    ${NRF_BASE}/scripts/tools-versions-minimum.txt
    ${NRF_BASE}/scripts/tools-versions-darwin.txt
    ${NRF_BASE}/scripts/tools-versions-win10.txt
    ${NRF_BASE}/scripts/tools-versions-linux.txt
)

set(pip_requirements_files
    ${ZEPHYR_BASE}/scripts/requirements-base.txt
    ${ZEPHYR_BASE}/scripts/requirements-doc.txt
    ${MCUBOOT_BASE}/scripts/requirements.txt
    ${NRF_BASE}/scripts/requirements-base.txt
    ${NRF_BASE}/scripts/requirements-doc.txt
    ${NRF_BASE}/scripts/requirements-build.txt
)

string(REPLACE ";" "\\;" tools_files_escaped "${tools_version_files}")
string(REPLACE ";" "\\;" pip_files_escaped "${pip_requirements_files}")

set(NRF_DOC_DIR ${NRF_BASE}/doc/nrf)

add_custom_command(
  OUTPUT ${NRF_BINARY_DIR}/src/versions.txt
  COMMAND ${CMAKE_COMMAND}
    -DTOOLS_VERSION_FILES="${tools_files_escaped}"
    -DPIP_REQUIREMENTS_FILES="${pip_files_escaped}"
    -DVERSION_IN=${NRF_DOC_DIR}/versions.txt.in
    -DVERSION_OUT=${NRF_BINARY_DIR}/src/versions.txt
    -P ${NRF_BASE}/doc/update_versions.cmake
  DEPENDS
    ${NRF_DOC_DIR}/versions.txt.in
    ${tools_version_files}
    ${pip_requirements_files}
)

set(nrf_env
  ZEPHYR_BASE=${ZEPHYR_BASE}
  NRF_BASE=${NRF_BASE}
  NRF_BUILD=${NRF_BINARY_DIR}
  DOXYGEN_EXECUTABLE=${DOXYGEN_EXECUTABLE}
)

add_custom_target(
  nrf-versions
  DEPENDS
    ${NRF_BINARY_DIR}/src/versions.txt
)

add_docset(nrf "${nrf_env}")
add_dependencies(nrf-html nrf-versions)
add_dependencies(nrf-html-all nrf-versions)

#-------------------------------------------------------------------------------
# docset: mcuboot

set(mcuboot_env
  NRF_BASE=${NRF_BASE}
  MCUBOOT_BASE=${MCUBOOT_BASE}
)

add_docset(mcuboot "${mcuboot_env}")

#-------------------------------------------------------------------------------
# docset: nrfx

set(nrfx_env
  NRF_BASE=${NRF_BASE}
  NRFX_BASE=${NRFX_BASE}
  NRFX_BUILD=${NRFX_BINARY_DIR}
)

add_docset(nrfx "${nrfx_env}")

#-------------------------------------------------------------------------------
# docset: nrfxlib

set(nrfxlib_env
  NRF_BASE=${NRF_BASE}
  NRFXLIB_BASE=${NRFXLIB_BASE}
  NRFXLIB_BUILD=${NRFXLIB_BINARY_DIR}
  DOXYGEN_EXECUTABLE=${DOXYGEN_EXECUTABLE}
)

add_docset(nrfxlib "${nrfxlib_env}")

#-------------------------------------------------------------------------------
# -- Intellinium addition
# docset: itl_car_pod

set(itl_car_pod_env
  ZEPHYR_BASE=${ZEPHYR_BASE}
  NRF_BASE=${NRF_BASE}
  ITL_LIB_BASE=${ITL_LIB_BASE}
  ITL_CAR_POD_BASE=${ITL_CAR_POD_BASE}
  ITL_CAR_POD_BUILD=${ITL_CAR_POD_BINARY_DIR}
)

add_external_docset(itl_car_pod
  "${itl_car_pod_env}"
  ssh://git@git.jetbrains.space/intellinium/mobility-pod/embedded-car-pod.git
  develop
)

#-------------------------------------------------------------------------------
# -- Intellinium addition
# docset: itl_lib

set(itl_lib_env
  ZEPHYR_BASE=${ZEPHYR_BASE}
  NRF_BASE=${NRF_BASE}
  ITL_LIB_BASE=${ITL_LIB_BASE}
  ITL_LIB_BUILD=${ITL_LIB_BINARY_DIR}
)

add_docset(itl_lib "${itl_lib_env}")

#-------------------------------------------------------------------------------
# -- Intellinium addition
# docset: itl_mat_pod

set(itl_mat_pod_env
  ZEPHYR_BASE=${ZEPHYR_BASE}
  NRF_BASE=${NRF_BASE}
  ITL_LIB_BASE=${ITL_LIB_BASE}
  ITL_MAT_POD_BASE=${ITL_MAT_POD_BASE}
  ITL_MAT_POD_BUILD=${ITL_MAT_POD_BINARY_DIR}
)

add_external_docset(itl_mat_pod
  "${itl_mat_pod_env}"
  ssh://git@git.jetbrains.space/intellinium/mobility-pod/embedded-mat-pod.git
  develop
)

#-------------------------------------------------------------------------------
# -- Intellinium addition
# docset: itl_tfl

set(itl_tfl_env
  ZEPHYR_BASE=${ZEPHYR_BASE}
  NRF_BASE=${NRF_BASE}
  ITL_LIB_BASE=${ITL_LIB_BASE}
  ITL_TFL_BASE=${ITL_TFL_BASE}
  ITL_TFL_BUILD=${ITL_TFL_BINARY_DIR}
)

add_external_docset(itl_tfl
  "${itl_tfl_env}"
  ssh://git@git.jetbrains.space/intellinium/pod-machine-learning/ml-tflite-zephyr-application.git
  develop
)

#-------------------------------------------------------------------------------
# docset: kconfig

file(WRITE ${KCONFIG_BINARY_DIR}/Kconfig.soc.defconfig
     "osource \"${ZEPHYR_BASE}/soc/$(ARCH)/*/Kconfig.defconfig\"\n"
)
file(WRITE ${KCONFIG_BINARY_DIR}/Kconfig.soc
     "osource \"${ZEPHYR_BASE}/soc/$(ARCH)/*/Kconfig.soc\"\n"
)
file(WRITE ${KCONFIG_BINARY_DIR}/Kconfig.shield.defconfig
     "osource \"${ZEPHYR_BASE}/boards/shields/*/Kconfig.defconfig\"\n"
)
file(WRITE ${KCONFIG_BINARY_DIR}/Kconfig.shield
     "osource \"${ZEPHYR_BASE}/boards/shields/*/Kconfig.shield\"\n"
)
file(WRITE ${KCONFIG_BINARY_DIR}/Kconfig.soc.arch
     "osource \"${ZEPHYR_BASE}/soc/$(ARCH)/Kconfig\"\n"
     "osource \"${ZEPHYR_BASE}/soc/$(ARCH)/*/Kconfig\"\n"
)

foreach(module_name ${ZEPHYR_MODULE_NAMES})
  zephyr_string(SANITIZE TOUPPER MODULE_NAME_UPPER ${module_name})
  list(APPEND
       ZEPHYR_KCONFIG_MODULES
       "ZEPHYR_${MODULE_NAME_UPPER}_MODULE_DIR=${ZEPHYR_${MODULE_NAME_UPPER}_MODULE_DIR}"
  )

  if(ZEPHYR_${MODULE_NAME_UPPER}_KCONFIG)
    list(APPEND
         ZEPHYR_KCONFIG_MODULES
         "ZEPHYR_${MODULE_NAME_UPPER}_KCONFIG=${ZEPHYR_${MODULE_NAME_UPPER}_KCONFIG}"
  )
  endif()
endforeach()

set(KCONFIG_RST_OUT ${KCONFIG_BINARY_DIR}/src)

add_custom_target(
  kconfig-content
  COMMAND ${CMAKE_COMMAND} -E make_directory ${KCONFIG_RST_OUT}
  COMMAND ${CMAKE_COMMAND} -E env
    PYTHONPATH=${ZEPHYR_BASE}/scripts/kconfig
    ZEPHYR_BASE=${ZEPHYR_BASE}
    srctree=${ZEPHYR_BASE}
    BOARD_DIR=boards/*/*/
    ARCH=*
    ARCH_DIR=arch/
    SOC_DIR=soc/
    KCONFIG_BINARY_DIR=${KCONFIG_BINARY_DIR}
    KCONFIG_WARN_UNDEF=y
    KCONFIG_TURBO_MODE=${KCONFIG_TURBO_MODE}
    KCONFIG_DOC_MODE=1
    ${ZEPHYR_KCONFIG_MODULES}
      ${PYTHON_EXECUTABLE}
        ${ZEPHYR_BASE}/doc/_scripts/gen_kconfig_rest.py ${KCONFIG_RST_OUT}
        --separate-all-index
        --modules Zephyr,zephyr,${ZEPHYR_BASE}
                  nRF,nrf,${NRF_BASE}
                  nrfxlib,nrfxlib,${NRFXLIB_BASE}
                  itl-lib,itl_lib,${ITL_LIB_BASE} # -- Intellinium addition
        --no-index-modules BuildDir,${CMAKE_BINARY_DIR}
                           MCUboot,${MCUBOOT_BASE}
  VERBATIM
)

add_docset(kconfig "")
add_dependencies(kconfig-html kconfig-content)
add_dependencies(kconfig-html-all kconfig-content)

#-------------------------------------------------------------------------------
# Global targets

add_custom_target(
  copy-extra-content
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/_static/html/index.html ${HTML_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy ${NRF_BASE}/doc/versions.json ${HTML_DIR}
)

add_dependencies(zephyr-html-all kconfig-html-all)
add_dependencies(mcuboot-html-all kconfig-html-all)
add_dependencies(nrfxlib-inventory-all kconfig-html-all)
add_dependencies(nrf-html-all zephyr-html-all nrfxlib-inventory-all mcuboot-html-all kconfig-html-all)
add_dependencies(nrfxlib-html-all nrf-html-all)

# -- Intellinium additions
add_dependencies(itl_car_pod-html-all itl_lib-html-all)
add_dependencies(itl_lib-html-all nrf-html-all)
add_dependencies(itl_mat_pod-html-all itl_lib-html-all)
add_dependencies(itl_tfl-html-all itl_lib-html-all)

add_custom_target(build-all ALL)
add_dependencies(build-all
    copy-extra-content
    nrf-html-all
    nrfx-html-all
    nrfxlib-html-all
    zephyr-html-all
    mcuboot-html-all
    kconfig-html-all
    itl_car_pod-html-all # -- Intellinium addition
    itl_lib-html-all # -- Intellinium addition
    itl_mat_pod-html-all # -- Intellinium addition
    itl_tfl-html-all # -- Intellinium addition
)

add_custom_target(linkcheck)
add_dependencies(linkcheck
    nrf-linkcheck
    nrfxlib-linkcheck
    mcuboot-linkcheck
    itl_car_pod-linkcheck # -- Intellinium addition
    itl_lib-linkcheck # -- Intellinium addition
    itl_mat_pod-linkcheck # -- Intellinium addition
    itl_tfl-linkcheck # -- Intellinium addition
)
