#!/bin/python

import sys
import json


req = {"help": "Press <span foreground=\"red\">Alt+1</span> or <span foreground=\"red\">Alt+2</span>"}
sys.stdout.write(json.dumps(req) + "\n")
sys.stdout.flush()

for line in sys.stdin:
    j = json.loads(line)
    if j["name"] != "key_press":
        continue

    key_code = j["value"]["key"]
    if key_code == "custom_1":
        req = {"lines": [{"text": "You press Alt+1", "filtering": False}]}
    elif key_code == "custom_2":
        req = {"lines": [{"text": "You press Alt+2", "filtering": False}]}
    else:
        req = {"lines": [{"text": "You press unknown key", "filtering": False}]}

    sys.stdout.write(json.dumps(req) + "\n")
    sys.stdout.flush()
