# Zephyr documentation build configuration file

from pathlib import Path
import sys
import os
from sphinx.config import eval_config_file

import os
import re
from pathlib import Path

import sphinx_rtd_theme
import sys

# Paths ------------------------------------------------------------------------

ZEPHYR_BASE = os.environ.get("ZEPHYR_BASE")
if not ZEPHYR_BASE:
    raise ValueError("ZEPHYR_BASE environment variable undefined")
ZEPHYR_BASE = Path(ZEPHYR_BASE)

NRF_BASE = os.environ.get("NRF_BASE")
if not NRF_BASE:
    raise FileNotFoundError("NRF_BASE not defined")
NRF_BASE = Path(NRF_BASE)

ITL_LIB_BASE = os.environ.get("ITL_LIB_BASE")
if not ITL_LIB_BASE:
    raise FileNotFoundError("ITL_LIB_BASE not defined")
ITL_LIB_BASE = Path(ITL_LIB_BASE)

ITL_LIB_BUILD = os.environ.get("ITL_LIB_BUILD")
if not ITL_LIB_BUILD:
    raise ValueError("ITL_LIB_BUILD environment variable undefined")
ITL_LIB_BUILD = Path(ITL_LIB_BUILD)

sys.path.insert(0, str(NRF_BASE / "doc" / "_utils"))
import utils

# pylint: disable=undefined-variable

# Add the '_extensions' directory to sys.path, to enable finding Sphinx
# extensions within.
sys.path.insert(0, str(ZEPHYR_BASE / "doc" / "_extensions"))

# Add the '_scripts' directory to sys.path, to enable finding utility
# modules.
sys.path.insert(0, str(ZEPHYR_BASE / "doc" / "_scripts"))

# Add the directory which contains the runners package as well,
# for autodoc directives on runners.xyz.
sys.path.insert(0, str(ZEPHYR_BASE / "scripts" / "west_commands"))

sys.path.insert(0, str(NRF_BASE / "doc" / "_extensions"))

import redirects

try:
    import west as west_found
except ImportError:
    west_found = False

# -- Project --------------------------------------------------------------

project = 'Intellinium'
copyright = '2021, Intellinium'
author = 'Giuliano FRANCHETTO, Farès CHATI, Peng GAO'
# NOTE: use blank space as version to preserve space
version = "&nbsp;"

# -- General configuration ------------------------------------------------

sys.path.insert(0, str(ZEPHYR_BASE / "doc" / "_extensions"))
sys.path.insert(0, str(NRF_BASE / "doc" / "_extensions"))

extensions = [
    "breathe",
    "sphinx.ext.todo",
    "sphinx.ext.extlinks",
    "sphinx.ext.autodoc",
    "sphinx.ext.intersphinx",
    "zephyr.application",
    "zephyr.html_redirects",
    "only.eager_only",
    "zephyr.dtcompatible-role",
    "zephyr.link-roles",
    "sphinx_tabs.tabs",
    "zephyr.warnings_filter",
    "zephyr.doxyrunner",
    "external_content",
    "table_from_rows",
    "options_from_kconfig",
]

# Only use SVG converter when it is really needed, e.g. LaTeX.
if tags.has("svgconvert"):  # pylint: disable=undefined-variable
    extensions.append("sphinxcontrib.rsvgconverter")

templates_path = ["_templates"]

exclude_patterns = ["_build"]

if not west_found:
    exclude_patterns.append("**/*west-apis*")
else:
    exclude_patterns.append("**/*west-not-found*")

# This change will allow us to use bare back-tick notation to let
# Sphinx hunt for a reference, starting with normal "document"
# references such as :ref:, but also including :c: and :cpp: domains
# (potentially) helping with API (doxygen) references simply by using
# `name`
default_role = "any"

pygments_style = "sphinx"

todo_include_todos = False

rst_epilog = """
.. include:: /substitutions.txt
.. include:: /links.txt
.. include:: /shortcuts.txt
"""

# -- Options for HTML output ----------------------------------------------

html_theme = "sphinx_itl_theme"
html_theme_path = [str(ITL_LIB_BASE / "doc" / "_themes")]
html_theme_options = {"docsets": utils.get_docsets("itl_lib")}
html_title = "Zephyr Project Documentation"
html_logo = None
html_favicon = None
html_static_path = [str(ITL_LIB_BASE / "doc" / "_static"),
                    str(ZEPHYR_BASE / "doc" / "_static"),
                    str(NRF_BASE / "doc" / "_static")]
html_last_updated_fmt = "%b %d, %Y"
html_domain_indices = False
html_split_index = True
html_show_sourcelink = True
html_show_sphinx = False
html_search_scorer = str(ZEPHYR_BASE / "doc" / "_static" / "js" / "scorer.js")

is_release = tags.has("release")  # pylint: disable=undefined-variable
docs_title = "Docs / {}".format(version if is_release else "Latest")
html_context = {
    "show_license": True,
    "docs_title": docs_title,
    "is_release": is_release,
}

# -- Options for zephyr.doxyrunner plugin ---------------------------------

doxyrunner_doxygen = os.environ.get("DOXYGEN_EXECUTABLE", "doxygen")
doxyrunner_doxyfile = ITL_LIB_BASE / "doc" / "itl_lib.doxyfile.in"
doxyrunner_outdir = ITL_LIB_BUILD / "doxygen"
doxyrunner_fmt = True
doxyrunner_fmt_vars = {
    "ZEPHYR_BASE": str(ZEPHYR_BASE),
    "ITL_LIB_BASE": str(ITL_LIB_BASE)
}

# -- Options for Breathe plugin -------------------------------------------

breathe_projects = {"Zephyr": str(doxyrunner_outdir / "xml")}
breathe_default_project = "Zephyr"
breathe_domain_by_extension = {
    "h": "c",
    "c": "c",
}
breathe_show_enumvalue_initializer = True
breathe_default_members = ("members",)

cpp_id_attributes = [
    "__syscall",
    "__deprecated",
    "__may_alias",
    "__used",
    "__unused",
    "__weak",
    "__attribute_const__",
    "__DEPRECATED_MACRO",
    "FUNC_NORETURN",
    "__subsystem",
]
c_id_attributes = cpp_id_attributes

# -- Options for html_redirect plugin -------------------------------------

# html_redirect_pages = redirects.REDIRECTS

# -- Options for zephyr.warnings_filter -----------------------------------

warnings_filter_config = str(ZEPHYR_BASE / "doc" / "known-warnings.txt")
warnings_filter_silent = False

# -- Linkcheck options ----------------------------------------------------

extlinks = {
    "jira": ("https://jira.zephyrproject.org/browse/%s", ""),
    "github": ("https://github.com/zephyrproject-rtos/zephyr/issues/%s", ""),
}

linkcheck_timeout = 30
linkcheck_workers = 10
linkcheck_anchors = False

# Options for intersphinx ------------------------------------------------------

intersphinx_mapping = dict()

nrf_mapping = utils.get_intersphinx_mapping("nrf")
if nrf_mapping:
    intersphinx_mapping["nrf"] = nrf_mapping

zephyr_mapping = utils.get_intersphinx_mapping("zephyr")
if zephyr_mapping:
    intersphinx_mapping["zephyr"] = zephyr_mapping

mcuboot_mapping = utils.get_intersphinx_mapping("mcuboot")
if mcuboot_mapping:
    intersphinx_mapping["mcuboot"] = mcuboot_mapping

nrfxlib_mapping = utils.get_intersphinx_mapping("nrfxlib")
if nrfxlib_mapping:
    intersphinx_mapping["nrfxlib"] = nrfxlib_mapping

kconfig_mapping = utils.get_intersphinx_mapping("kconfig")
if kconfig_mapping:
    intersphinx_mapping["kconfig"] = kconfig_mapping

# Options for external_content -------------------------------------------------

external_content_contents = [
    (ITL_LIB_BASE / "doc", "*"),
    (ITL_LIB_BASE, "samples/**/*.rst"),
]
external_content_keep = [
    "reference/devicetree/bindings.rst",
    "reference/devicetree/bindings/**/*",
    "reference/devicetree/compatibles/**/*",
]

# Options for table_from_rows --------------------------------------------------

table_from_rows_base_dir = ITL_LIB_BASE

# Options for options_from_kconfig ---------------------------------------------

options_from_kconfig_base_dir = ITL_LIB_BASE

# pylint: enable=undefined-variable


def setup(app):
    app.add_css_file("css/common.css")
    app.add_css_file("css/itl_lib.css")
