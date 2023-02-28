#!/usr/bin/env python3
"""
This is a utility script that uses the `botbench` helper to
generate a table of performances of bots against each other.
This can help identify regressions in bot play.
"""

from pathlib import Path
import subprocess
from tabulate import tabulate


bots_path = Path() / "data" / "scripts"
bot_names = [str(bot.relative_to(bots_path)) for bot in bots_path.glob("*.lua")]

assert len(bot_names) != 0, "No bots found"


score_table = [[i] + [0] * len(bot_names) for i in bot_names]
time_table = [[i] + [0] * len(bot_names) for i in bot_names]


for lid in range(len(bot_names)):
    left_bot = bot_names[lid]
    for rid in range(lid, len(bot_names)):
        right_bot = bot_names[rid]
        print(f"Running {left_bot} vs {right_bot}")
        output = subprocess.check_output(["./botbench", left_bot, right_bot], text=True)  # type: str
        marker = f"{left_bot} vs {right_bot}:"
        for line in output.splitlines():
            if marker in line:
                result = line[len(marker):]
                score, _, time = result.partition("in")
                left, _, right = score.partition("-")
                score_table[lid][1 + rid] = int(left)
                score_table[rid][1 + lid] = int(right)
                time = time.split()[0]
                time_table[rid][1 + lid] = int(time)

print("Scores:")
print(tabulate(score_table, headers=[""] + bot_names))

print()
print("Durations:")
print(tabulate(time_table, headers=[""] + bot_names))
