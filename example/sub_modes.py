#!/bin/python

import sys
import json


def send(lines, input_text=None, prompt_text=None, help_text=None, hide_combi_lines=False, exit_by_cancel=True):
    msg = {
        "input": input_text,
        "prompt": prompt_text,
        "help": help_text,
        "hide_combi_lines": hide_combi_lines,
        "exit_by_cancel": exit_by_cancel,
        "lines": lines}
    sys.stdout.write(json.dumps(msg) + "\n")
    sys.stdout.flush()


def on_input(text: str):
    help_text = "Press <span foreground=\"red\">ESCAPE</span> for exit from mode"
    if text == "kill ":
        lines = [{"text": "process1"}, {"text": "process2"}]
        send(lines, input_text="", prompt_text="kill", help_text=help_text, hide_combi_lines=True,
             exit_by_cancel=False)
    if text == "clipboard ":
        lines = [{"text": "bla"}, {"text": "bla bla bla"}]
        send(lines, input_text="", prompt_text="clipboard", help_text=help_text, hide_combi_lines=True,
             exit_by_cancel=False)


def on_key_press(text: str):
    if text == "cancel":
        lines = [{"text": "kill"}, {"text": "clipboard"}]
        send(lines, input_text="", prompt_text="combi", hide_combi_lines=False, exit_by_cancel=True)


send([{"text": "kill"}, {"text": "clipboard"}])
for line in sys.stdin:
    try:
        j = json.loads(line)
        if j["name"] == "input":
            on_input(j["value"])
        elif j["name"] == "key_press":
            on_key_press(j["value"]["key"])
    except Exception:
        send([])
