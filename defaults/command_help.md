*Text commands*

A button name is matched first. Parentheses are required. Command names ignore case. Separate calls with `;` or a new line. Coordinates are screen pixels; omit them to use the cursor.

*Cursor*
`setMousePos(x, y)` — both coordinates required

*Left button*
`leftClick()` or `leftClick(x, y)` — press and release
`leftDown()` or `leftDown(x, y)` — press
`leftUp()` or `leftUp(x, y)` — release

*Right button*
`rightClick()` or `rightClick(x, y)`
`rightDown()` or `rightDown(x, y)`
`rightUp()` or `rightUp(x, y)`

*Wheel*
`scroll(3)` — 3 notches up at the cursor. `scroll(-3)` — down. Range -120..120.

*Text*
`type("text")` — focused window. Escapes: `\"` `\\` `\n` Enter, `\t` Tab

*Run*
`run("notepad")` or `run("d:\file.txt")` — like Win+R. Name, path, document, or URL. Arguments: `run("notepad.exe d:\buttons.json")`

*Media*
`prev()` `next()` `stop()`
`pause()` — play/pause key
`mute()` — toggle speaker mute
`volumeUp()` `volumeDown()` — one press of the volume key
`volume(10)` — set master volume to 10%. `volume(+10)` raises by 10 points, `volume(-10)` lowers. `volume()` replies with the level.

*Keys*
`keys("alt+f4")`

*System*
`lock()` `sleep()` `signout()`
`displayOff()` — turn the monitor off
`micMute()` — toggle microphone mute
`beep()` — play the find-PC sound
`shot()` — send a screenshot
`shutdown()` `restart()` — reply Yes within 30s

*Link*
`ping()` — replies pong
