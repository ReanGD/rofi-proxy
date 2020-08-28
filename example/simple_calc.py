#!/bin/python

import sys
import json


for line in sys.stdin:
    j = json.loads(line)
    if j["name"] != "input":
        continue

    try:
        answer = str(eval(j["value"]))
        req = {"lines": [{"text": answer, "filtering": False}]}
        sys.stdout.write(json.dumps(req) + "\n")
        sys.stdout.flush()
    except Exception:
        pass
