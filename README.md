# rofi-proxy

`Rofi-proxy` is a [rofi](https://github.com/davatorium/rofi) plugin that allows you to manage rofi state with an external application.

Running rofi with the plugin:

```bash
rofi -modi proxy -show proxy -proxy-cmd "path_to_app"
```

There is also full support for `combi` mode:

```bash
rofi -modi combi -show combi -combi-modi "proxy,drun" -proxy-cmd "path_to_app"
```

The "-proxy-log" option causes the `rofi-proxy` to log to `$XDG_DATA_HOME/rofi/proxy.log` (or to `$HOME/.local/share/rofi/proxy.log` if the environment variable `XDG_DATA_HOME` not set):

```bash
rofi -modi proxy -show proxy -proxy-log -proxy-cmd "path_to_app"
```

## Communication protocol

Examples of using the protocol [here](https://github.com/ReanGD/rofi-proxy/tree/master/example).

After starting the plugin, it launches the `application` specified in "-proxy-cmd". And it sends json messages to its stdin when rofi state changes. The `application` can send (at any time) messages describing the desired state of rofi in json format to its stdout. All messages must be in single-line json format and end with "\n".

### Messages from `rofi-proxy` to `application`

- Each time the input line changes, the plugin sends a message named "input", for example:

```json
{
    "name": "input",
    "value": "full_input_string"
}
```

- When you select a line (press enter), the plugin will send a "select_line" message with the data for this line, for example:

```json
{
    "name": "select_line",
    "value": {
        "id": "id_string",
        "text": "line_text",
        "group": "droup_string"
    }
}
```

- When you delete a line (press Shift+Delete by default), the plugin will send a "delete_line" message with the data for this line, for example:

```json
{
    "name": "delete_line",
    "value": {
        "id": "id_string",
        "text": "line_text",
        "group": "droup_string"
    }
}
```

- When the hotkey is pressed (see the rofi documentation for options "-kb-custom-1" ... "-kb-custom-19"), the plugin sends a "key_press" message with the code "custom_1" .. "custom_19" (if interception for pressing `Escape` is allowed  then rofi will not close, but will send a message about the pressed key with the code" cancel "). Also in the value of the "line" field, the data of the currently displayed line will be sent, for example:

```json
{
    "name": "key_press",
    "value": {
        "key": "custom_1",
        "line": {
            "id": "id_string",
            "text": "line_text",
            "group": "droup_string"
        }
    }
}
```

### Messages from `application` to `rofi-proxy`

At any time, `application` can send a single line json to the plugin describing the state of the rofi. Here's an example json with a full set of fields:

```json
{
    "prompt": "prompt_text",
    "input": "user_input_text",
    "overlay": "overlay_text",
    "help": "help_text",
    "hide_combi_lines": false,
    "exit_by_cancel": true,
    "lines": [
        {
            "id": "id_text",
            "text": "display_text",
            "group": "group_text",
            "icon": "applications-internet",
            "filtering": true,
            "urgent": false,
            "active": false,
            "markup": false
        },
        ...
    ],
}
```

All fields in the root json are optional. Here is a description of the fields in the root json:

| Name             | Type   | Start value | Description                                                              |
|------------------|--------|-------------|--------------------------------------------------------------------------|
| prompt           | string | ""          | Sets prompt text. If null or not set, `prompt` remains the same.         |
| input            | string | ""          | Sets user input text. If null or not set, `user input` remains the same. |
| overlay          | string | ""          | Sets overlay text. If null or not set, `overlay` remains the same.       |
| help             | string | ""          | Sets help text. If null or not set, `help` remains the same.             |
| hide_combi_lines | bool   | false       | If the value is true, then in combi mode, all lines are hidden except those described in `lines`.</br>If null or not set, `hide_combi_lines` remains the same. |
| exit_by_cancel   | bool   | true        | If the value is false, then when you press Escape, rofi does not exit, but sends the "key_press" message with the "cancel" key.</br>If null or not set, `exit_by_cancel` remains the same. |
| lines            | array  | []          | An array for the contents of the rofi list, see description below.</br>If null or not set, `lines` remains the same. |

Description of fields in array `lines`:

| Name      | Type   | Default  | Description                                                                                                                 |
|-----------|--------|----------|-----------------------------------------------------------------------------------------------------------------------------|
| id        | string | ""       | Sent as is in `select_line`, `delete_line` or `key_press messages`.                                                         |
| text      | string | required | Text displayed in line.                                                                                                     |
| group     | string | ""       | Sent as is in `select_line`, `delete_line`  or `key_press messages`.                                                        |
| icon      | string | none     | Icon name or full path to it (to use it, you need to run rofi with the "-show-icons" flag).                                 |
| filtering | bool   | true     | If the value is false, then this line is always displayed, regardless of filtering.                                         |
| urgent    | bool   | false    | Mark line as urgent.                                                                                                        |
| active    | bool   | false    | Mark line as active.                                                                                                        |
| markup    | bool   | false    | Allow the [pango markup language](https://developer.gnome.org/pygtk/stable/pango-markup-language.html) in the `text` field. |

## Compiling and installing from source

```bash
git clone https://github.com/ReanGD/rofi-proxy.git
cd rofi-proxy
cmake -B build
sudo cmake --build build --config Release --target install
```
