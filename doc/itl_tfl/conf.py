# itl_lib documentation build configuration file

from pathlib import Path
import os
from sphinx.config import eval_config_file


# Paths ------------------------------------------------------------------------

ITL_TFL_BASE = os.environ.get("ITL_TFL_BASE")
if not ITL_TFL_BASE:
    raise FileNotFoundError("ITL_TFL_BASE not defined")
ITL_TFL_BASE = Path(ITL_TFL_BASE)

# General ----------------------------------------------------------------------

# Import itl-lib configuration
conf = eval_config_file(str(ITL_TFL_BASE / "doc" / "conf.py"), tags)
locals().update(conf)


def setup(app):
    app.add_css_file("css/common.css")
    app.add_css_file("css/itl_tfl.css")
