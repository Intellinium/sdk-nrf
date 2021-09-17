# itl_lib documentation build configuration file

from pathlib import Path
import os
from sphinx.config import eval_config_file


# Paths ------------------------------------------------------------------------

ITL_LIB_BASE = os.environ.get("ITL_LIB_BASE")
if not ITL_LIB_BASE:
    raise FileNotFoundError("ITL_LIB_BASE not defined")
ITL_LIB_BASE = Path(ITL_LIB_BASE)

# General ----------------------------------------------------------------------

# Import itl-lib configuration
conf = eval_config_file(str(ITL_LIB_BASE / "doc" / "conf.py"), tags)
locals().update(conf)


def setup(app):
    app.add_css_file("css/common.css")
    app.add_css_file("css/itl_lib.css")
