# Third-Party Notices

GameHQ itself is licensed under the [MIT License](LICENSE). The Windows package
also redistributes shared runtime components under their own licenses.

## Qt 6.8.3

GameHQ dynamically links to Qt libraries and plugins. The open-source Qt build
is available under the GNU Lesser General Public License version 3, with
individual third-party components under the licenses identified by Qt.

- Qt licensing: https://doc.qt.io/qt-6/licensing.html
- Qt source code: https://code.qt.io/cgit/qt/
- LGPL version 3: https://www.gnu.org/licenses/lgpl-3.0.html

The shared libraries remain replaceable in the package's `app` directory.

## FFmpeg 7.1

The Qt Multimedia runtime includes FFmpeg shared libraries. Qt identifies its
FFmpeg build as LGPL 2.1-or-later with additional permissively licensed parts.

- FFmpeg legal information: https://ffmpeg.org/legal.html
- FFmpeg source code: https://ffmpeg.org/download.html#get-sources
- LGPL version 2.1: https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html

## MinGW-w64 and GCC runtime libraries

The Windows package contains runtime libraries supplied by the MinGW-w64/GCC
toolchain. These components remain under their upstream licenses and applicable
runtime library exceptions.

- GCC licenses: https://gcc.gnu.org/onlinedocs/libstdc++/manual/license.html
- MinGW-w64 licensing: https://www.mingw-w64.org/about/

## miniz 3.1.2

`GameHQUpdater.exe` statically links miniz for ZIP staging and validation.
miniz is distributed under the MIT License; the complete license is included
at `licenses/miniz.txt`.

- Source: https://github.com/richgel999/miniz/tree/3.1.2

No ownership of these third-party components is claimed by the GameHQ project.
