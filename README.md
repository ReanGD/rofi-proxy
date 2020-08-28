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

## Compiling and installing from source

```bash
git clone https://github.com/ReanGD/rofi-proxy.git
cd rofi-proxy
cmake -B build
sudo cmake --build build --config Release --target install
```
