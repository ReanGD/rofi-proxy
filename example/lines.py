#!/bin/python

import sys
import json


req = {"lines": [
    {"text": "normal", "filtering": False},
    {"text": "urgent", "filtering": False, "urgent": True},
    {"text": "active", "filtering": False, "active": True},
    {"text": "with icon", "filtering": False, "icon": "applications-internet"},
    ]}
sys.stdout.write(json.dumps(req) + "\n")
sys.stdout.flush()

for line in sys.stdin:
    pass
