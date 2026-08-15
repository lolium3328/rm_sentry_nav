from pathlib import Path
from ament_index_python.packages import get_package_share_directory
import runpy
from pathlib import Path
_helper = runpy.run_path(str(Path(__file__).with_name('_multi_robot.py')))
generate_multi_world = _helper['generate_multi_world']
def generate_launch_description():
    return generate_multi_world()
