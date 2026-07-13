GameHQ {VERSION}
================

Capture screenshots and replay clips of your games, browse them in a
couch-friendly gallery, and pull them up in-game with the overlay.

Getting started
---------------
1. Run GameHQ.exe. The app lives in the system tray (double-click the tray
   icon to open the gallery).
2. In a game: press the pad Share button (tap = screenshot, hold = save
   replay clip) or Ctrl+Shift+S / Ctrl+Shift+E on the keyboard.
3. Press PS / Guide (or Ctrl+Shift+G) to open the in-game overlay.

Folder layout
-------------
GameHQ.exe      starts the app (the real program is in app\)
app\            program files - nothing user-serviceable inside
Captures\       your screenshots and clips, one folder per game
gamehq-data\    settings, database, thumbnails, logs
portable.flag   keeps everything in this folder; delete it and GameHQ
                stores data in your Windows user profile instead

Notes
-----
- GameHQ never modifies your games and never injects into game processes.
- Logs are at gamehq-data\logs\gamehq.log if you need to report a problem.
- Source code and licenses: https://github.com/underfusion/GameHQ
