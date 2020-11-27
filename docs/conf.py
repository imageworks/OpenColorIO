# -*- coding: utf-8 -*-
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

# See:
# http://sphinx.pocoo.org/config.html

import os
import shutil
import subprocess
import sys
import re


HERE = os.path.dirname(os.path.realpath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, os.pardir))

# -- Build options ------------------------------------------------------------

# OCIO API has been modified. Update frozen API docs in source tree. 
# Dependent on CMake build (so not RTD).
BUILD_FROZEN = "@OCIO_BUILD_FROZEN_DOCS@" == "ON"

RTD_BUILD = os.environ.get("READTHEDOCS") == "True"  # Set by Read the Docs
CI_BUILD = os.environ.get("CI") == "true"            # Set by GitHub Actions

# -- Add PyOpenColorIO and local Sphinx extensions ----------------------------

if RTD_BUILD:
    # No CMake configuration
    sys.path.append(os.path.join(ROOT, "share", "docs"))

    import configure_file

else:
    # NOTE: PyOpenColorIO not available on RTD
    sys.path.append(os.path.join(
        "@CMAKE_BINARY_DIR@".replace("\\", "/"), 
        "src", "bindings", "python"
    ))
    sys.path.append(os.path.join(
        "@CMAKE_SOURCE_DIR@".replace("\\", "/"), 
        "share", "docs"
    ))

    configure_file = None

import expandvars
import prettymethods

if BUILD_FROZEN:
    import frozendoc
else:
    frozendoc = None

# -- General configuration ----------------------------------------------------

if RTD_BUILD:
    # Extract project info from root CMakeLists.txt
    project = configure_file.get_project_name()
    description = configure_file.get_project_description()

    version, version_major, version_minor, version_patch \
        = configure_file.get_project_version()
        
    ocio_namespace = configure_file.get_ocio_namespace(
        major=version_major, 
        minor=version_minor
    )

else:
    project = "@CMAKE_PROJECT_NAME@"
    description = "@CMAKE_PROJECT_DESCRIPTION@"
    version = "@CMAKE_PROJECT_VERSION@"
    ocio_namespace = "@OCIO_NAMESPACE@"

author = "Contributors to the {} Project".format(project)
copyright = "Copyright {}.".format(author)

extensions = [
    "sphinx.ext.autodoc",
    "sphinx.ext.napoleon",
    "recommonmark",
    "sphinx_tabs.tabs",
    "breathe",
    "expandvars",
    "prettymethods",
    "frozendoc",
]

source_suffix = {
  ".rst": "restructuredtext",
  ".md": "markdown",
  ".txt": "markdown",
}

master_doc = "index"
exclude_patterns = ["build", "*-prefix"]

rst_prolog = """
.. |OCIO| replace:: *OCIO*
.. |OpenColorIO| replace:: **OpenColorIO**
.. _Academy Software Foundation: https://www.aswf.io/
.. |Academy Software Foundation| replace:: `Academy Software Foundation`_
.. _ocio-user: https://lists.aswf.io/g/ocio-user
.. _ocio-dev: https://lists.aswf.io/g/ocio-dev
""".format(project=project)

# -- Extension Configuration --------------------------------------------------

# frozendoc
frozendoc_build = BUILD_FROZEN

# When enabled, frozendoc_build is also enabled to build and compare new frozen
# RST with frozen RST present in the source tree. This is only necessary in a 
# CI job, where we want to confirm the contributor has built frozen RST along 
# with any public API or documentation changes.
frozendoc_compare = CI_BUILD

# breathe
breathe_projects = {project: "_doxygen/xml"}
breathe_default_project = project

# pygments
pygments_style = "friendly"

# napoleon
napoleon_use_param = False
napoleon_include_init_with_doc = True

# expandvars
expandvars_define = {
    "PYDIR": "frozen" if RTD_BUILD else "src",
    "OCIO_NAMESPACE": ocio_namespace,
}

# -- Options for HTML output --------------------------------------------------

html_theme = "press"
html_logo = "_static/ocio_b.svg"
html_static_path = ["_static"]

html_theme_options = {
  "external_links": [
      ("Github", "https://github.com/AcademySoftwareFoundation/OpenColorIO"),
  ]
}

# -- Options for LaTeX output -------------------------------------------------

latex_documents = [(
    "index", 
    "{}.tex".format(project), 
    "{} Documentation".format(project),
    author, 
    "manual", 
    False
)]

latex_elements = {
    "preamble": "\setcounter{tocdepth}{2}",
}
latex_domain_indices = ["cpp-modindex", "py-modindex"]

# -- Options for manual page output -------------------------------------------

man_pages = [(
    "index", 
    project.lower(), 
    "{} Documentation".format(project),
    [author], 
    1
)]

# -- Options for Texinfo output -----------------------------------------------

texinfo_documents = [(
    "index", 
    project, 
    "{} Documentation".format(project), 
    author,
    project, 
    description, 
    "Miscellaneous",
    True
)]

# -- Options for Epub output --------------------------------------------------

epub_title = project
epub_author = author
epub_publisher = author
epub_copyright = copyright

# -- Run Doxygen --------------------------------------------------------------

# When building docs for Read the Docs, only sphinx-build is called, so we must
# run doxygen first to build XML data for breathe. In all other cases doxygen 
# is run when needed by CMake.

if RTD_BUILD:
    # Configure needed *.in files
    for src_filename, dst_filename in [
        (
            os.path.join(ROOT, "include", "OpenColorIO", "OpenColorABI.h.in"), 
            os.path.join(ROOT, "include", "OpenColorIO", "OpenColorABI.h")
        ),
        (
            os.path.join(ROOT, "docs", "Doxyfile.in"), 
            os.path.join(ROOT, "docs", "Doxyfile")
        ),
    ]:
        configure_file.configure_file(src_filename, dst_filename)

    # Generate Doxygen XML
    subprocess.run(["doxygen", "Doxyfile"], check=True)
