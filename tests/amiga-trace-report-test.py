#!/usr/bin/env python3
"""Keep representation changes in the same trace cost category as their oracle."""
import importlib.util
from pathlib import Path
import unittest
from unittest import mock
from collections import Counter

spec = importlib.util.spec_from_file_location(
    "trace_report", Path(__file__).resolve().parents[1] / "tools/amiga-trace-report.py")
report = importlib.util.module_from_spec(spec)
spec.loader.exec_module(report)


class LogicalNames(unittest.TestCase):
    owner = "downwell::RunnerSpatialInstances<" + "VeryLongOwnershipPolicy<3067u>," * 6 + "6134u>"

    def test_long_owner_methods_remain_distinct(self):
        birth = report.short(self.owner + "::attachPreparedTerrainBirth(int)")
        finish = report.short(self.owner + "::finishFreshPreparedTerrain(int)")
        self.assertGreater(len(birth), 110)
        self.assertEqual(birth[:110], finish[:110])
        self.assertNotEqual(birth, finish)
        self.assertTrue(birth.endswith("::attachPreparedTerrainBirth"))
        self.assertTrue(finish.endswith("::finishFreshPreparedTerrain"))
        symbols = report.Symbolizer("unused", {})
        symbols.chains[1] = [(finish, "owner.h:99"), (birth, "owner.h:55")]
        self.assertEqual(symbols.frames(1), [birth, finish])

    def test_display_keeps_method_without_changing_key(self):
        name = report.short(self.owner + "::attachPreparedTerrainBirth<386u, Graph<3u>>(int)")
        self.assertEqual(report.display_name(name),
                         "RunnerSpatialInstances<...>::attachPreparedTerrainBirth<...>")
        self.assertIn("VeryLongOwnershipPolicy", name)
        self.assertEqual(report.display_name("operator<(int)"), "operator<(int)")

    def test_focus_finds_method_after_long_owner(self):
        names = [report.short(self.owner + "::attachPreparedTerrainBirth(int)"),
                 report.short(self.owner + "::finishFreshPreparedTerrain(int)")]
        self.assertEqual(report.matching_names(names, "attachPreparedTerrainBirth"), names[:1])
        self.assertEqual(report.matching_names(names, names[1]), names[1:])
        self.assertEqual(report.matching_names(names, "missing"), [])


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


class FrameBoundaries(unittest.TestCase):
    def test_dma_stall_crossing_field_is_not_lost_or_invented(self):
        pending = report.field_cycle_carry(140718, 142102, 4)
        self.assertEqual(pending, 1384)
        self.assertEqual(report.field_cycle_carry(143480, 142102, 3436, pending), 0)
        self.assertEqual(report.field_cycle_carry(143958, 142102, 3796,
                         report.field_cycle_carry(140242, 142102, 10)), 0)

    def test_unexplained_cycles_still_fail(self):
        for accounted, first, pending in ((143480, 3436, 0), (142102, 3436, 1384),
                                           (143480, 100, 1384), (140718, 3436, 1384),
                                           (70000, 10, 0)):
            with self.subTest(accounted=accounted, first=first, pending=pending):
                with self.assertRaises(ValueError):
                    report.field_cycle_carry(accounted, 142102, first, pending)

    def test_pre_mark_execution_is_not_a_complete_frame(self):
        stacks, entries, cycles = Counter(), Counter(), Counter()
        # Recording starts during frame 843, then sees complete frame 844
        # and only the beginning of 845. The first fragment is not idle.
        instructions = [(0, 10, 100, 4096, 0, 0), (0, 20, 200, 4096, 0, 0),
                        (0, 99, 20, 4096, 0, 0), (0, 103, 16, 4096, 0, 844),
                        (1, 30, 384, 4096, 0, 0),
                        (1, 99, 20, 4096, 0, 0), (1, 103, 16, 4096, 0, 845),
                        (1, 40, 54, 4096, 0, 0)]
        fields, last = self.attribute(instructions, stacks, cycles, entries)
        self.assertEqual((fields, last), (2, 845))
        self.assertEqual(cycles, {843: 300, 844: 420, 845: 90})
        self.assertEqual(report.complete_frame_numbers(cycles, {843, last}), [844])
        self.assertEqual(report.complete_frame_numbers({843: 300}, {843}), [])

    def attribute(self, instructions, stacks=None, cycles=None, entries=None):
        calls = report.CallSites()
        calls.marker = report.marker_reader({99: ('move.l', '4(sp),d0'), 103: ('rts', '')}, 99)
        def records(path, include_registers=False):
            for item in instructions:
                yield item[:5] + ((item[5],) + (0,) * 16,) if include_registers else item[:5]
        with mock.patch.object(report, 'records', side_effect=records):
            return report.attribute('unused', 843, calls, 99,
                                    stacks if stacks is not None else Counter(),
                                    cycles if cycles is not None else Counter(),
                                    entries if entries is not None else Counter())

    def test_capture_includes_requested_marker_and_interrupt(self):
        cycles = Counter()
        self.attribute([(0, 99, 20, 4096, 0, 0),
                        (0, 77, 100, 2048, 0x2000, 999),
                        (0, 103, 16, 4096, 0, 843),
                        (1, 99, 20, 4096, 0, 0),
                        (1, 103, 16, 4096, 0, 844)], cycles=cycles)
        self.assertEqual(cycles, {843: 136, 844: 36})

    def test_invalid_argument_and_truncated_marker_fail(self):
        for instructions in ([(0, 99, 20, 4096, 0, 0)],
                             [(0, 99, 20, 4096, 0, 0), (0, 103, 16, 4096, 0, 845)]):
            with self.assertRaises(ValueError):
                self.attribute(instructions)

    def test_unsupported_abi_fails(self):
        with self.assertRaises(ValueError):
            report.marker_reader({99: ('nop', ''), 103: ('rts', '')}, 99)


if __name__ == "__main__":
    unittest.main()
