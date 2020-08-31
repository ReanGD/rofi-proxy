#!/bin/python

import sys
import json


lines = []


def send_lines():
    req = {"lines": [{"text": line} for line in lines]}
    sys.stdout.write(json.dumps(req) + "\n")
    sys.stdout.flush()


sys.stdout.write(json.dumps({"help": "Press Enter to add text\rPress Shift+Delete to remove the line"}) + "\n")
sys.stdout.flush()
for line in sys.stdin:
    j = json.loads(line)
    if j["name"] == "select_custom_input":
        lines.append(j["value"])
        send_lines()
    elif j["name"] == "delete_line":
        lines.remove(j["value"]["text"])
        send_lines()
    else:
        continue
