"""Python package API for converting GLM files to JSON."""

from __future__ import annotations

from contextlib import redirect_stdout
import importlib.util
import io
from pathlib import Path
from types import ModuleType


def _candidate_converter_dirs() -> list[Path]:
    package_dir = Path(__file__).resolve().parent
    repo_root = package_dir.parents[2]

    return [
        package_dir / "json_schema",
        repo_root / "utilities" / "json_schema",
    ]


def _load_converter_module() -> ModuleType:
    for converter_dir in _candidate_converter_dirs():
        converter = converter_dir / "glm_to_json.py"
        if converter.exists():
            spec = importlib.util.spec_from_file_location("gridlabd_glm_to_json", converter)
            if spec is None or spec.loader is None:
                continue
            module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(module)
            return module

    searched = ", ".join(str(path) for path in _candidate_converter_dirs())
    raise ImportError("Unable to locate GridLAB-D GLM-to-JSON converter. Searched: {0}".format(searched))


def glm_to_json(glm_path, output_path=None):
    """Convert a GridLAB-D ``.glm`` file to JSON.

    Args:
        glm_path: Path to the input ``.glm`` file.
        output_path: Optional output JSON file path or output directory. When
            omitted, the JSON file is written next to the input file.

    Returns:
        pathlib.Path: Path to the generated JSON file.

    Raises:
        FileNotFoundError: If ``glm_path`` does not exist.
        RuntimeError: If the existing converter reports failure.
    """

    glm_path = Path(glm_path)
    if not glm_path.exists():
        raise FileNotFoundError(str(glm_path))
    if glm_path.suffix.lower() != ".glm":
        raise ValueError("expected a .glm file: {0}".format(glm_path))

    output_dir = None
    output_name = None
    expected_output = glm_path.with_suffix(".json")

    if output_path is not None:
        output_path = Path(output_path)
        if output_path.suffix.lower() == ".json":
            output_dir = str(output_path.parent)
            output_name = output_path.stem
            expected_output = output_path
        else:
            output_dir = str(output_path)
            output_name = glm_path.stem
            expected_output = output_path / "{0}.json".format(glm_path.stem)

    converter = _load_converter_module()
    converter_output = io.StringIO()
    with redirect_stdout(converter_output):
        success = converter.glm_to_json(
            glm_path.stem,
            input_dir=str(glm_path.parent),
            output_dir=output_dir,
            output_name=output_name,
        )

    if not success:
        details = converter_output.getvalue().strip()
        message = "failed to convert GLM file to JSON: {0}".format(glm_path)
        if details:
            message = "{0}\n{1}".format(message, details)
        raise RuntimeError(message)

    return expected_output


convert_glm_to_json = glm_to_json
