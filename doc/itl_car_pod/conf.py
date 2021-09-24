# Car Pod documentation build configuration file

from pathlib import Path
import os
from sphinx.config import eval_config_file


# Paths ------------------------------------------------------------------------

ITL_CAR_POD_BASE = os.environ.get("ITL_CAR_POD_BASE")
if not ITL_CAR_POD_BASE:
    raise FileNotFoundError("ITL_CAR_POD_BASE not defined")
ITL_CAR_POD_BASE = Path(ITL_CAR_POD_BASE)

# General ----------------------------------------------------------------------

# Import itl-lib configuration
conf = eval_config_file(str(ITL_CAR_POD_BASE / "doc" / "conf.py"), tags)
locals().update(conf)


def setup(app):
    app.add_css_file("css/common.css")
    app.add_css_file("css/itl_car_pod.css")
