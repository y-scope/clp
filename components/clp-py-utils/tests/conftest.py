import sys
from pathlib import Path

# `clp_py_utils` imports from the sibling `job_orchestration` component.
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "job-orchestration"))
