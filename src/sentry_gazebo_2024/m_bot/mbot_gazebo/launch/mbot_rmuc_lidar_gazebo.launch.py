import runpy
from pathlib import Path
_helper = runpy.run_path(str(Path(__file__).with_name('_multi_robot.py')))
generate_single = _helper['generate_single']
def generate_launch_description():
    return generate_single()
