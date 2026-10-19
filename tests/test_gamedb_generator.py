#!/usr/bin/env python3
# Copyright (c) 2026 Peter Fors
# SPDX-License-Identifier: MIT

import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location("gamedb", Path(__file__).resolve().parents[1] / "tools/gen_joystick_gamedb.py")
gamedb = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gamedb)


class GeneratorTests(unittest.TestCase):
	def test_sources(self):
		for source, expected in (("b0", 0x40), ("b31", 0x5f), ("a0", 0x80), ("+a3", 0x83), ("-a3", 0xa3), ("a3~", 0x83), ("h0.1", 0xc1), ("h0.8", 0xc8), ("h0.15", 0xcf)):
			with self.subTest(source=source):
				self.assertEqual(gamedb.pack_source(source), expected)

	def test_unrepresentable_sources(self):
		for source in ("", "b32", "b53", "a32", "h1.1", "h0.0", "h0.16", "h0", "unknown", "a-1"):
			with self.subTest(source=source):
				self.assertEqual(gamedb.pack_source(source), 0)

	def test_filter_and_precedence(self):
		text = "\n".join((
			"# source database",
			"03000000341200007856000000000000,Windows,a:b9,platform:Windows,",
			"03000000341200007856000000000000,First,a:b2,leftx:-a1,dpup:h0.1,platform:Linux,",
			"05000000341200007856000001000000,Second,a:b7,platform:Linux,",
			"xinput,XInput,a:b0,platform:Linux,",
			"03000000000000000000000000000000,No IDs,a:b0,platform:Linux,",
		))
		entries = gamedb.parse(text)
		self.assertEqual(len(entries), 1)
		vendor, product, buttons, axes = entries[0]
		self.assertEqual((vendor, product), (0x1234, 0x5678))
		self.assertEqual(buttons, [0x42] + [0] * 10 + [0xc1] + [0] * 3)
		self.assertEqual(axes, [0xa1] + [0] * 5)

	def test_splice_and_repeat(self):
		original = "enums\n" + gamedb.MARK_BEGIN + "\nold\n" + gamedb.MARK_END + "\nfooter\n"
		block = gamedb.emit([(0x1234, 0x5678, [0] * 15, [0] * 6)], "fixture")
		updated = gamedb.splice(original, block)
		self.assertEqual(updated, "enums\n" + block.rstrip() + "\nfooter\n")
		self.assertEqual(gamedb.splice(updated, block), updated)
		self.assertIn("struct mkfw_gamedb_record", block)
		self.assertNotIn("static const char", block)

	def test_missing_markers(self):
		with self.assertRaises(SystemExit):
			gamedb.splice("no generated block", "replacement")


if __name__ == "__main__":
	unittest.main()
