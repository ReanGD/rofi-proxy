#!/bin/python

import sys
import json
import math
import types
import subprocess


def send(lines):
    sys.stdout.write(json.dumps({"lines": lines}) + "\n")
    sys.stdout.flush()


def on_input(text: str):
    if len(text) == 0:
        send([])
        return

    answer = eval(text.replace("^", "**"), {"__builtins__": {}}, vars(math))
    if not isinstance(answer, types.BuiltinFunctionType):
        help_msg = "Enter to copy to the clipboard"
        user_text = f'{text} = <span foreground="red">{answer}</span> <span size="smaller">({help_msg})</span>'
        select_msg_text = f"{answer}"
        send([{"text": user_text, "id": select_msg_text, "filtering": False, "markup": True}])


def on_select_line(text: str):
    subprocess.run(["xclip", "-selection", "c"], input=text, encoding="utf-8", check=True)
    sys.exit(0)


sys.stdout.write(json.dumps({"overlay": "calc"}) + "\n")
sys.stdout.flush()

for line in sys.stdin:
    try:
        j = json.loads(line)
        if j["name"] == "input":
            on_input(j["value"])
        elif j["name"] == "select_line":
            on_select_line(j["value"]["id"])
    except Exception:
        send([])
