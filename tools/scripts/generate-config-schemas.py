#!/usr/bin/env -S uv run --script
#
# /// script
# dependencies = [
#   "clp_py_utils",
#   "pydantic",
# ]
# [tool.uv.sources]
# clp-py-utils = { path = "../../components/clp-py-utils", editable = true }
# ///

"""Generate JSON schemas for CLP configs."""

import argparse
import json
from pathlib import Path
from typing import Any

from clp_py_utils.clp_config import ClpConfig
from pydantic.json_schema import GenerateJsonSchema


class GenerateJsonSchemaWithSchemaKey(GenerateJsonSchema):
    """Adds the `$schema` key to the JSON Schema produced by the parent generator."""

    def generate(self, *args: Any, **kwargs: Any) -> Any:
        """
        Generates a JSON Schema dictionary with the `$schema` field.

        :return: The JSON Schema dictionary with an added `$schema` key.
        """
        json_schema = super().generate(*args, **kwargs)
        json_schema["$schema"] = self.schema_dialect
        return json_schema


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate JSON schemas for CLP configs.")
    parser.add_argument("output_dir", type=Path, help="Directory to write the schema files to.")
    args = parser.parse_args()

    output_dir = args.output_dir
    output_dir.mkdir(exist_ok=True)

    schema = ClpConfig.model_json_schema(schema_generator=GenerateJsonSchemaWithSchemaKey)
    with Path.open(output_dir / "clp-config.schema.json", "w") as f:
        json.dump(schema, f, indent=2)
