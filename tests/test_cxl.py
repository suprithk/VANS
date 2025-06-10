import subprocess
from pathlib import Path
import unittest

class CXLTest(unittest.TestCase):
    def test_cxl_latency(self):
        build = Path('build')
        build.mkdir(exist_ok=True)
        subprocess.check_call(['cmake', '..'], cwd=build)
        subprocess.check_call(['make', '-j2'], cwd=build)
        trace = Path('tests/sample_traces/cxl_read.trace').resolve()
        cfg = Path('config/cxl.cfg').resolve()
        out = subprocess.check_output(['./bin/vans', '-c', str(cfg), '-t', str(trace)])
        self.assertIn(b'Total clock', out)

if __name__ == '__main__':
    unittest.main()
