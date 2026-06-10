import json
from pathlib import Path

import gridlabd


def test_glm_to_json_package_api(tmp_path, capsys):
    glm_file = tmp_path / "minimal.glm"
    glm_file.write_text(
        """
        module powerflow;
        object node {
            name source;
            phases ABCN;
            nominal_voltage 7200;
        }
        """,
        encoding="utf-8",
    )

    output_file = gridlabd.glm_to_json(glm_file, tmp_path / "converted.json")

    assert output_file == tmp_path / "converted.json"
    assert output_file.exists()

    data = json.loads(output_file.read_text(encoding="utf-8"))
    assert "objects" in data
    assert "node" in data["objects"]
    assert capsys.readouterr().out == ""


def test_gridlabd_instance_glm_to_json_package_api(tmp_path):
    glm_file = tmp_path / "minimal.glm"
    glm_file.write_text(
        """
        module powerflow;
        object node {
            name source;
        }
        """,
        encoding="utf-8",
    )

    gld = gridlabd.DirectGridLabD()
    output_file = gld.convert_glm_to_json(glm_file)

    assert output_file == glm_file.with_suffix(".json")
    assert output_file.exists()


def test_nested_object_with_explicit_name_keeps_name(tmp_path):
    glm_file = tmp_path / "nested_named.glm"
    glm_file.write_text(
        """
        object house {
            name H_1;
            object controller {
                name DC_1;
            };
        }
        """,
        encoding="utf-8",
    )

    output_file = gridlabd.glm_to_json(glm_file)
    data = json.loads(output_file.read_text(encoding="utf-8"))

    controller = data["objects"]["controller"]["instances"][0]
    assert controller["name"] == "DC_1"
    assert controller["parent"] == "H_1"


def test_nested_object_under_object_declaration_uses_resolvable_parent_names(tmp_path):
    glm_file = tmp_path / "nested_oid.glm"
    glm_file.write_text(
        """
        object house:23 {
            floor_area 2500.0;
            object double_assert {
                target floor_area;
                within 0.01;
                object player {
                    property value;
                    file floorArea.player;
                };
            };
        }
        """,
        encoding="utf-8",
    )

    output_file = gridlabd.glm_to_json(glm_file)
    data = json.loads(output_file.read_text(encoding="utf-8"))

    double_assert = data["objects"]["double_assert"]["instances"][0]
    player = data["objects"]["player"]["instances"][0]

    assert double_assert["name"].startswith("house:23double_assert_")
    assert double_assert["parent"] == "house:23"
    assert player["parent"] == double_assert["name"]


def test_nested_object_generated_names_are_unique_across_anonymous_parents(tmp_path):
    glm_file = tmp_path / "nested_oids.glm"
    glm_file.write_text(
        """
        object house:23 {
            object double_assert {
                target floor_area;
                object player {
                    property value;
                };
            };
        }
        object house:24 {
            object double_assert {
                target floor_area;
                object player {
                    property value;
                };
            };
        }
        """,
        encoding="utf-8",
    )

    output_file = gridlabd.glm_to_json(glm_file)
    data = json.loads(output_file.read_text(encoding="utf-8"))

    double_asserts = data["objects"]["double_assert"]["instances"]
    names = {entry["name"] for entry in double_asserts}
    parents = {entry["parent"] for entry in double_asserts}
    player_parents = {entry["parent"] for entry in data["objects"]["player"]["instances"]}

    assert len(names) == 2
    assert any(name.startswith("house:23double_assert_") for name in names)
    assert any(name.startswith("house:24double_assert_") for name in names)
    assert parents == {"house:23", "house:24"}
    assert player_parents == names
