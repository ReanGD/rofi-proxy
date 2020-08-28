# rofi-proxy

`Rofi-proxy` is a [rofi](https://github.com/davatorium/rofi) plugin that allows you to manage rofi state with an external application.

Running rofi with the `rofi-proxy` plugin:

```bash
rofi -modi proxy -show proxy -proxy-cmd "path_to_app"
```

There is also full support for `combi` mode:

```bash
rofi -modi combi -show combi -combi-modi "proxy,drun" -proxy-cmd "path_to_app"
```

The "-proxy-log" option causes the `rofi-proxy` plugin to log to `$XDG_DATA_HOME/rofi/proxy.log` (or to `$HOME/.local/share/rofi/proxy.log` if the environment variable `XDG_DATA_HOME` not set):

```bash
rofi -modi proxy -show proxy -proxy-log -proxy-cmd "path_to_app"
```

## Compiling and installing from source

```bash
git clone https://github.com/ReanGD/rofi-proxy.git
cd rofi-proxy
cmake -B build
sudo cmake --build build --config Release --target install
```
