# ruff: noqa: INP001 The Sphinx config file is a standalone script rather than an importable module.
"""Sphinx configuration file for CLP documentation."""

from datetime import datetime, timezone

from sphinx.application import Sphinx

# Constants
CLP_GIT_REF = "main"


# -- Project information -------------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = "CLP"
# NOTE: We don't include a period after "Inc" since the theme adds one already.
# Ignore A001: Shadows built-in name 'copyright' for Sphinx config.
copyright = f"2021-{datetime.now(tz=timezone.utc).year} YScope Inc"  # noqa: A001

# -- General configuration -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    "myst_parser",
    "sphinx_copybutton",
    "sphinx_design",
    "sphinx.ext.autodoc",
    "sphinx.ext.viewcode",
    "sphinxcontrib.mermaid",
]

# -- MyST extensions -----------------------------------------------------------
# https://myst-parser.readthedocs.io/en/stable/syntax/optional.html
myst_enable_extensions = [
    "attrs_block",
    "colon_fence",
    "dollarmath",
]

myst_heading_anchors = 4

# -- Sphinx autodoc options ----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/extensions/autodoc.html#configuration

autoclass_content = "class"
autodoc_class_signature = "separated"
autodoc_typehints = "description"

# -- HTML output options -------------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_favicon = "https://docs.yscope.com/_static/favicon.ico"
html_logo = "../src/clp-logo.png"
html_title = "CLP"
html_show_copyright = True

html_static_path = ["../src/_static"]

html_theme = "pydata_sphinx_theme"

# -- sphinxcontrib-mermaid options ---------------------------------------------
# https://sphinxcontrib-mermaid-demo.readthedocs.io/en/latest/

mermaid_include_elk = True
mermaid_version = "11.5.0"

# -- Theme options -------------------------------------------------------------
# https://pydata-sphinx-theme.readthedocs.io/en/stable/user_guide/layout.html

html_theme_options = {
    "icon_links": [
        {
            "icon": (
                "https://raw.githubusercontent.com/zulip/zulip/main/static/images/logo"
                "/zulip-icon-circle.svg"
            ),
            "name": "CLP on Zulip",
            "url": "https://yscope-clp.zulipchat.com/",
            "type": "url",
        },
    ],
    "footer_start": ["copyright"],
    "footer_center": [],
    "footer_end": ["theme-version"],
    "navbar_start": ["navbar-logo", "version-switcher"],
    "navbar_end": ["navbar-icon-links", "theme-switcher"],
    "primary_sidebar_end": [],
    "secondary_sidebar_items": ["page-toc", "edit-this-page"],
    "show_prev_next": False,
    "switcher": {
        "json_url": "https://docs.yscope.com/_static/clp-versions.json",
        "version_match": CLP_GIT_REF,
    },
    "use_edit_page_button": True,
}

# -- Theme source buttons ------------------------------------------------------
# https://pydata-sphinx-theme.readthedocs.io/en/stable/user_guide/source-buttons.html

html_context = {
    "github_user": "y-scope",
    "github_repo": "clp",
    "github_version": CLP_GIT_REF,
    "doc_path": "docs/src",
}


# -- Theme custom CSS and JS ---------------------------------------------------
# https://pydata-sphinx-theme.readthedocs.io/en/stable/user_guide/static_assets.html


def setup(app: Sphinx) -> None:
    """
    Sets up Sphinx app with custom CSS.

    :param app:
    """
    app.add_css_file("custom.css")

    app.connect("source-read", _replace_variable_placeholders)


def _replace_variable_placeholders(_app: Sphinx, _docname: str, content: list[str]) -> None:
    """
    Replaces each variable placeholder in the docs with the relevant value.

    :param _app:
    :param _docname:
    :param content: The content of the doc in a list with a single element.
    """
    placeholder_to_value = {
        "DOCS_VAR_CLP_GIT_REF": CLP_GIT_REF,
    }
    for placeholder, value in placeholder_to_value.items():
        content[0] = content[0].replace(placeholder, value)
