# Locate or install Sphinx (Python documentation generator)
#
# Variables defined by this module:
#   SPHINX_FOUND
#   SPHINX_EXECUTABLE
#
# Targets defined by this module:
#   Sphinx - custom pip target, if package can be installed
#
# Usage:
#   find_package(Sphinx)
#
# If Sphinx is not installed in a standard path, add it to the SPHINX_DIRS 
# variable to tell CMake where to find it. If it is not found and 
# OCIO_INSTALL_EXT_PACKAGES is set to MISSING or ALL, Sphinx will be 
# installed via pip at build time.
#

find_package(PythonInterp 2.7 QUIET)

if(NOT TARGET Sphinx)
    add_custom_target(Sphinx)
    set(_SPHINX_TARGET_CREATE TRUE)
endif()

###############################################################################
### Try to find package ###

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
    # Find sphinx-build
    find_program(SPHINX_EXECUTABLE 
        NAMES 
            sphinx-build
        HINTS
            ${SPHINX_DIRS}
    )

    # Override REQUIRED if package can be installed
    if(OCIO_INSTALL_EXT_PACKAGES STREQUAL MISSING)
        set(Sphinx_FIND_REQUIRED FALSE)
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(Sphinx
        REQUIRED_VARS 
            SPHINX_EXECUTABLE
    )
    set(SPHINX_FOUND ${Sphinx_FOUND})
endif()

###############################################################################
### Install package from PyPi ###

if(NOT SPHINX_FOUND)
    set(_EXT_DIST_ROOT "${CMAKE_BINARY_DIR}/ext/dist")

    # Set find_package standard args
    set(SPHINX_FOUND TRUE)
    set(SPHINX_EXECUTABLE "${_EXT_DIST_ROOT}/bin/sphinx-build")

    # Configure install target
    if(_SPHINX_TARGET_CREATE)
        add_custom_command(
            TARGET
                Sphinx
            COMMAND
                pip install --quiet
                            --disable-pip-version-check
                            --install-option="--prefix=${_EXT_DIST_ROOT}"
                            -I Sphinx==${Sphinx_FIND_VERSION}
            WORKING_DIRECTORY
                "${CMAKE_BINARY_DIR}"
        )

        message(STATUS "Installing Sphinx: ${SPHINX_EXECUTABLE} (version ${Sphinx_FIND_VERSION})")
    endif()
endif()

mark_as_advanced(SPHINX_EXECUTABLE)
