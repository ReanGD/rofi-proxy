#!/bin/python

import sys
import json

req = {"lines": [{"text": "HelloПриветPërshëndetje", "filtering": False}]}
sys.stdout.write(json.dumps(req) + "\n")
sys.stdout.flush()

for line in sys.stdin:
    j = json.loads(line)
    if j["name"] == "input":
        sys.stderr.write(j["value"] + "\n")
        sys.stderr.flush()
