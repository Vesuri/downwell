#!/usr/bin/env python3
"""Keep representation changes in the same trace cost category as their oracle."""
import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location(
    "trace_report", Path(__file__).resolve().parents[1] / "tools/amiga-trace-report.py")
report = importlib.util.module_from_spec(spec)
spec.loader.exec_module(report)


class CostOwners(unittest.TestCase):
    def test_scalar_and_run_owners(self):
        for owner in ("RunnerRoomMembership", "RunnerRunRoomMembership", "RunnerObservedRoomMembership",
                      "RunnerObjectMembership", "RunnerRunMembership", "TerrainOwnershipRuns",
                      "RunnerSpatialLifetimes", "RunnerObservedRegistration"):
            with self.subTest(owner=owner):
                frame = f"downwell::{owner}<3067u, 8u, 2056u>::bind"
                self.assertEqual(report.cause(["runDownwell", frame]), "runner bookkeeping")
                self.assertEqual(report.cause(["runDownwell", frame, "memcpy"]), "runner bookkeeping")

    def test_terrain_store_variants(self):
        for owner in ("GameplayTerrainInstances", "GameplayTerrainInstancesScalar", "GameplayTerrainInstancesRuns"):
            self.assertEqual(report.cause(["runDownwell", f"downwell::{owner}::append"]),
                             "runner bookkeeping")

    def test_innermost_owner_still_wins(self):
        frames = ["runDownwell", "downwell::RunnerRunRoomMembership<64u,8u,56u>::bind"]
        self.assertEqual(report.cause(frames + ["downwell::RunnerImageValue::assignInteger"]),
                         "software float emulation")
        self.assertEqual(report.cause(frames + ["downwell::RunnerSpatialIndex<128u>::insert"]),
                         "R-tree upkeep")
        self.assertEqual(report.cause(["runDownwell", "AmigaPlatform::waitVblank"]),
                         "wait for vertical blank")

    def test_shared_identity_publication(self):
        for method in ("spatialCanAttach", "spatialObject", "spatialHandle", "spatialPublish", "spatialRetire"):
            self.assertEqual(report.cause(["runDownwell", f"downwell::GameplayTerrainInstancesRuns::{method}"]),
                             "runner bookkeeping")


if __name__ == "__main__":
    unittest.main()
