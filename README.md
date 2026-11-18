# iliz pc remote

[Русский](README.ru.md)

A simple, convenient remote for a Windows PC. One tiny tray exe, no installer. On the phone you install nothing — the whole remote already lives in a Telegram bot.

Pause music from the sofa, send hotkeys, lock the screen, glance at the desktop, or play a voice message on the speakers. It fits a machine that stays on in another room, when a full remote desktop is more than you need.

MIT license.

The whole client is the tray icon. Right-click it for Open Telegram, Open control page, Settings, Open config.json, About, and Exit. Double-click opens Settings. There is no other window until you ask for one.

**Two ways to talk to the PC. Use one, or both at once.**

- **Iliz server** — the hosted bot. Works as soon as you start the exe: no setup, no BotFather token on this PC. The client bootstraps MQTT from the API and opens `t.me/{bot}?start={token}`. Tray → **Open Telegram**.
- **Own bot** — your BotFather token and a list of Telegram user IDs. Fully serverless: no cloud of ours, no extra catalog server. Telegram is the catalog. Commands run only for those IDs.

Buttons are yours. Media, hotkeys, programs, system actions, mouse and typed text — up to 64 buttons, up to 10 rows. Edit them in Settings or in `config.json` next to the exe.

![Telegram remote](docs/telegram.jpg)

![Custom buttons in Settings](docs/settings.png)

## Features

- On first launch the app creates a device token and opens Settings.
- Tray icon: gray while an enabled transport is offline, normal when every enabled transport is online, a short flash on a command.
- Buttons, up to 64, up to 10 rows:
  - **media** — previous / next track, pause, stop, volume, mute.
  - **keys** — a key combination (`ctrl+shift+s`, `alt+f4`, `win+d`). The button dialog captures it from the keyboard.
  - **run** — open a file, a program, or a URL.
  - **system** — lock, sleep, display off, sign out, restart, shutdown, screenshot, microphone, find-PC beep.
  - **cmd** — a text command (mouse, wheel, typed text). See [Commands](#commands) below.
- Restart and shutdown run only after `Yes` within 30 seconds.
- A voice message or an audio file sent to the bot plays on this PC's speakers. The next clip replaces the one that is playing. Own bot: only from IDs on the allow list. `bass.dll` and `bassopus.dll` sit next to the exe.
- Iliz: a screenshot goes to the hosted bot. Own bot: a screenshot goes to the chat.
- Everything is in the tray menu. No extra control panel to keep open.
- Start with Windows. The checkbox writes `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`, value `IlizPcrc`. Nothing else is stored in the registry.
- Starting the exe again does not open a second copy: it shows Settings of the one already running.
- Own bot: `/start` and `/keyboard` show your user ID and refresh the keyboard. `/help` lists the text commands.

An example layout (media, hotkeys, screenshot, lock, beep) is in [`defaults/config.json`](defaults/config.json). Secrets stay empty. The `config.json` next to your exe is yours — do not publish it.

## Requirements

- Windows 10 or 11, 64-bit
- Own bot: outbound HTTPS to `api.telegram.org`
- Iliz server: your API and an MQTT broker on plain TCP
- To build: Visual Studio Build Tools 2022 (MSVC x64)

## Install

1. Download the zip from this repository's Releases, or build it yourself (below).
2. Put `iliz_pc_remote.exe` in its own folder. The zip already has `config.json`, `bass.dll`, `bassopus.dll`, and `beep.mp3`. A source build copies [`defaults/config.json`](defaults/config.json) to `bin\` on the first run if that file is missing; put the two DLLs next to the exe yourself (`build.bat` does not touch them).
3. Run the exe. It sits in the tray. Settings opens when there is no device token yet.

Right-click the tray icon for the menu: Open Telegram, Open control page (when Iliz is enabled), Settings, Open config.json, About, Exit. Double-click the icon for Settings. That menu is the whole UI.

`config.json` holds the device token, the API address, and the bot token. The MQTT login comes from bootstrap and is not written to this file. Do not commit it or send it.

### Iliz server

Enabled out of the box. Start the exe, wait until the tray icon is online, then tray → **Open Telegram**. No BotFather, no token to paste, no extra fields to fill.

The client posts `{server}/api/device/bootstrap` and takes the MQTT login from the response. The control page is `{server}/{token}`.

The Telegram link is `https://t.me/{username}?start={token}`. The bot username comes from that response and stays in memory until the next start. The page, the link, and the username are not written to the config. The client does not call Telegram `getMe` for this bot: that needs the bot token, which stays on the server.

**New token** rotates the device token. The previous page and deep link no longer match this PC.

A different API address is optional: Settings → **Iliz server**.

### Own bot

No extra app on the phone. No server of your own. Create a bot, paste the token, add your Telegram ID.

1. In Telegram, open [@BotFather](https://t.me/BotFather).
2. Send `/newbot`, then set a name and a username. The username must end with `bot`.
3. BotFather sends a token like `123456789:AA...`. Copy the whole token.
4. Settings → **Own bot**: paste the token, **Enabled**, **Save**.
5. Open your bot and send `/start`. The reply contains `Your ID: ...`.
6. Paste that ID into **Allowed user IDs**. Several people, separated by commas: `111,222`.
7. **Save** again. The bot replies `Ready.` and shows the keyboard.

The token is the bot's password. If it leaks, run `/revoke` in BotFather and paste the new token.

While the ID list is empty, buttons do nothing. Anyone else gets `Not authorized`. A voice or audio file from an allowed user plays on the speakers.

## Buttons

Settings → the **Row / Button / Action / Value** table. **Add**, **Edit**, **Delete**, **Up**, **Down**, then **Save**. **Up** and **Down** swap the selected button with its neighbor. If that neighbor is on another row, the moved button joins that row. Iliz publishes the layout again on **Save**. For the own bot, send `/start` so Telegram refreshes the keyboard.

Tray → **Open config.json** edits the same file by hand (UTF-8). The published example is [`defaults/config.json`](defaults/config.json). Buttons are the `buttons` array. A short shape:

```json
{
  "version": 1,
  "ilizEnabled": true,
  "ownEnabled": false,
  "token": "",
  "server": "http://127.0.0.1:8080",
  "botToken": "",
  "allowedIds": "",
  "buttons": [
    { "row": 1, "label": "⏸", "action": "media", "value": "pause" },
    { "row": 2, "label": "Alt+F4", "action": "keys", "value": "alt+f4" },
    { "row": 3, "label": "Screenshot", "action": "system", "value": "shot" }
  ]
}
```

| action | value |
|--------|--------|
| `media` | `prev` `next` `pause` `stop` `vol_up` `vol_down` `mute` |
| `keys` | `ctrl+shift+s` `alt+f4` `win+d` |
| `run` | `C:\\Tools\\script.bat` or a path / URL |
| `system` | `lock` `sleep` `display` `restart` `shutdown` `signout` `shot` `mic` `beep` |
| `cmd` | a command string, see [Commands](#commands) |

The button caption (`label`) is the text the client matches. Two buttons with the same caption cannot be told apart.

## Commands

Button names are matched first. If none match, the string runs as a command.

Several calls in one message: separate with `;` or a newline. Spaces do not matter. Parentheses are required, even with no arguments. Command names ignore case.

`/help` in the own bot and in the Iliz bot sends [defaults/command_help.md](defaults/command_help.md) (Telegram Markdown). The developer notes are in [docs/dev/commands.md](docs/dev/commands.md).

Coordinates are screen pixels (can be negative on multi-monitor). Omit them to act at the current cursor position.

| command | notes |
|---------|--------|
| `ping()` | replies `pong` |
| `setMousePos(x, y)` | both args required |
| `leftClick()` / `leftClick(x, y)` | press and release |
| `leftDown()` / `leftDown(x, y)` | press |
| `leftUp()` / `leftUp(x, y)` | release |
| `rightClick()` / `rightClick(x, y)` | |
| `rightDown()` / `rightDown(x, y)` | |
| `rightUp()` / `rightUp(x, y)` | |
| `scroll(3)` | wheel notches at the cursor. Positive is up, negative is down, `-120` to `120` |
| `type("text")` | types text into the focused window. `\"` `\\` `\n` (Enter) `\t` (Tab) |
| `run("notepad")` | Win+R: a program name, a path, a document, or a URL. A backslash is a path separator: `run("notepad.exe d:\buttons.json")` |
| `volume(10)` | set master volume to 10%. `volume(+10)` adds 10 points, `volume(-10)` subtracts. `volume()` replies with the level |
| `volumeUp()` / `volumeDown()` | one press of the volume key, not a percent |
| `prev()` `next()` `stop()` | media keys |
| `pause()` | play/pause key |
| `mute()` | toggle speaker mute |
| `keys("alt+f4")` | the same shortcut string as a keys button |
| `lock()` `sleep()` `signout()` | system actions |
| `displayOff()` | turn the monitor off |
| `micMute()` | toggle microphone mute |
| `beep()` | play the find-PC sound |
| `shot()` | send a screenshot |
| `shutdown()` `restart()` | ask `Yes` within 30 seconds |

One argument alone is an error for a click. Zero or a coordinate pair only. `scroll` takes one number. `type` and `run` take one quoted string.

Example drag:

```
setMousePos(100, 200)
leftDown()
setMousePos(400, 200)
leftUp()
```

Own bot: send the text in chat from an allowed user. Success is silent, except `ping()` which replies `pong`. Errors come back as text.

Iliz: the same text in MQTT `type` `cmd`. `ping()` answers `{"ok":true,"result":"pong"}`.

Button: action `cmd`, value is the same string. On save the string is checked. The cursor, wheel, and keyboard are not touched.

## Updates

Releases include iliz_updater (a separate project) (`iliz_updater.exe` + `iliz_updater.json`).
The app checks GitHub Releases about a minute after it starts and then once a day.
If `iliz_updater.exe` is missing (for example, in a dev build), update checks are turned off.

- Tray menu: **Check for updates** opens the updater window, which shows the installed and new versions and what's new, and lets you install. When an update is found, the menu also gets **Update to X**. A tray notification appears once per version. Click it to update.
- The updater downloads the release zip, checks its SHA-256 (GitHub asset digest), closes the app, replaces the files, and starts the app again. If something fails, it rolls back. Your `config.json` is kept.
- `config.json` → `"autoUpdate"`: `"notify"` (default; notification only), `"auto"` (install silently in the background and restart), or `"off"` (no background checks; the menu item still works). `"auto_update"` is accepted too.
- By hand: run `iliz_updater.exe` from the app folder. The log is in `update\updater.log`.

Release: `pack-release.bat` puts the updater into the zip. Upload the zip (named `iliz_pc_remote-X.Y.Z.zip`) to a GitHub release tagged `X.Y.Z`. `dist\SHA256SUMS` is optional (`NO_SUMS=1` skips it).

## Build

From the repository root, with VS Build Tools 2022 installed:

```bat
build.bat
```

If Build Tools are not in the default directory:

```bat
set VSINSTALL=C:\Path\To\BuildTools
build.bat
```

Output: `bin\iliz_pc_remote.exe`.

Release zip (exe, both DLLs, example `config.json`, `command_help.md`, and `beep.mp3` when it sits in `bin\`):

```bat
pack-release.bat
```

Output: `dist\iliz_pc_remote-<version>.zip`. The zip uses [`defaults/config.json`](defaults/config.json) and [`defaults/command_help.md`](defaults/command_help.md), not a `config.json` you edited next to a running exe.

## License

[MIT](LICENSE)
