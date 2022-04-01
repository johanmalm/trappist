# Trappist

A [wip] independent wayland menu

## Usage

`trappist <menu.xml>`

Use [labwc-menu-gnome3] or hand-craft your own menu file using
[openbox menu syntax].

## Goals

- [ ] Openbox menu syntax
- [ ] Pipemenus
- [ ] Type to search
- [ ] Built-in support for system applications
- [ ] Internationalization
- [ ] Icons

## Design

- No GTK or X11
- No parsing of XDG menu spec .directory or .menu files
- Only one binary
- [ ] Long-running application
- [ ] Wake up using unix-socket

## Dependencies

- [x] https://gitlab.freedesktop.org/wlroots/wlr-protocols.git
- [x] https://github.com/johanmalm/sway-client-helpers.git
- [ ] https://gitlab.freedesktop.org/ddevault/fdicons.git

[labwc-menu-gnome3]: https://github.com/labwc/labwc-menu-gnome3
[openbox menu syntax]: http://openbox.org/wiki/Help:Menus

