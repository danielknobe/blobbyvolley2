#!/usr/bin/env python3
"""
This is a utility script that uses the `botbench` helper to
generate a table of performances of bots against each other.
This can help identify regressions in bot play.
"""

from pathlib import Path
import subprocess
from tabulate import tabulate
import argparse
from multiprocessing import Pool
from functools import partial
from itertools import combinations_with_replacement

def simulate(bot_names, combination):
    left_bot = bot_names[combination[0]]
    right_bot = bot_names[combination[1]]
    print(f"Running {left_bot} vs {right_bot}")

    output = subprocess.check_output(["./botbench", left_bot, right_bot], text=True)  # type: str
    marker = f"{left_bot} vs {right_bot}:"
    for line in output.splitlines():
        if marker in line:
            result = line[len(marker):]
            score, _, time = result.partition("in")
            left, _, right = score.partition("-")
            time = time.split()[0]
            return (combination, int(left), int(right), int(time))


    return (combination, None, None, None)


def writeToTable(results, score_table, time_table):
    for result in results:
        lid = result[0][0]
        rid = result[0][1]
        left = result[1]
        right = result[2]
        time = result[3]

        if left is not None:
            score_table[lid][1 + rid] = left
        else:
            score_table[lid][1 + rid] = "err"

        if right is not None:
            score_table[rid][1 + lid] = right
        else:
            score_table[rid][1 + lid] = "err"

        if time is not None:
            time_table[rid][1 + lid] = time
        else:
            time_table[rid][1 + lid] = "err"



parser = argparse.ArgumentParser(description='Checks all combinations of bots against each other')
parser.add_argument('-j', '--jobs', type=int, default=1)

bots_path = Path() / "data" / "scripts"
bot_names = [str(bot.relative_to(bots_path)) for bot in bots_path.glob("*.lua")]

assert len(bot_names) != 0, "No bots found"


score_table = [[i] + [0] * len(bot_names) for i in bot_names]
time_table = [[i] + [0] * len(bot_names) for i in bot_names]

bot_combinations = list(combinations_with_replacement(range(len(bot_names)), 2))

with Pool(vars(parser.parse_args())["jobs"]) as p:
    results = p.map(partial(simulate, bot_names), bot_combinations)

writeToTable(results, score_table, time_table)

print("Scores:")
print(tabulate(score_table, headers=[""] + bot_names))

print()
print("Durations:")
print(tabulate(time_table, headers=[""] + bot_names))
