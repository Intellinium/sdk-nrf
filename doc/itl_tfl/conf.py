# TFLite app documentation build configuration file

from pathlib import Path
import os
import sys
from sphinx.config import eval_config_file


# Paths ------------------------------------------------------------------------

NRF_BASE = Path(__file__).absolute().parents[2]

sys.path.insert(0, str(NRF_BASE / "doc" / "_utils"))
import utils

ZEPHYR_BASE = utils.get_projdir("zephyr")
ITL_LIB_BASE = utils.get_projdir("itl_lib")
ITL_TFL_BASE = NRF_BASE / "doc" / "_external" / "itl_tfl"

os.environ["ZEPHYR_BASE"] = str(ZEPHYR_BASE)
os.environ["NRF_BASE"] = str(NRF_BASE)
os.environ["ITL_LIB_BASE"] = str(ITL_LIB_BASE)
os.environ["ITL_TFL_BASE"] = str(ITL_TFL_BASE)
os.environ["ITL_TFL_BUILD"] = str(utils.get_builddir() / "itl_tfl")

# General ----------------------------------------------------------------------

# Import itl-lib configuration
conf = eval_config_file(str(ITL_TFL_BASE / "doc" / "conf.py"), tags)
locals().update(conf)


def setup(app):
    app.add_css_file("css/common.css")
    app.add_css_file("css/itl_tfl.css")
