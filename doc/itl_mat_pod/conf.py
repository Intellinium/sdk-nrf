# Mat Pod documentation build configuration file

from pathlib import Path
import os
from sphinx.config import eval_config_file


# Paths ------------------------------------------------------------------------

ITL_MAT_POD_BASE = os.environ.get("ITL_MAT_POD_BASE")
if not ITL_MAT_POD_BASE:
    raise FileNotFoundError("ITL_MAT_POD_BASE not defined")
ITL_MAT_POD_BASE = Path(ITL_MAT_POD_BASE)

# General ----------------------------------------------------------------------

# Import itl-lib configuration
conf = eval_config_file(str(ITL_MAT_POD_BASE / "doc" / "conf.py"), tags)
locals().update(conf)


def setup(app):
    app.add_css_file("css/common.css")
    app.add_css_file("css/itl_mat_pod.css")
