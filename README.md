# Keepnote

A simple, persistent tabbed note pad with Markdown preview. Type, close the window, and reopen it later. Every tab, its name, and its text are saved automatically in the app data directory. An unnamed tab takes its title from its first line.

## Build and run

```sh
./bin/build
./build/keepnote
```

The title bar shows Keepnote and Minimize, Maximize or Restore, and Close buttons. The menu bar sits below it. The **File** menu has New Tab, Open File, Save File, Save File As, Close Tab, and Quit. The **Edit** menu has Undo, Redo, Cut, Copy, Paste, and Select All. The **View** menu switches between Editor, Split, and Preview for Markdown notes. Markdown headings, emphasis, links, lists, and code blocks are rendered in Preview; the source text stays editable in Editor or Split.

The **Language** menu selects highlighting for Plain Text, Markdown, and Log, with Data formats (JSON, YAML, TOML) and Code (Python, JavaScript, Lua, Bash/Shell, TypeScript) submenus. Opened files choose a language from their extension; each tab remembers a manual choice. Highlighting covers common syntax, including multiline strings and comments, and does not change the saved text.

`Ctrl+N` or `Ctrl+T` adds a tab; `Ctrl+O` opens a UTF-8 text file in a tab; `Ctrl+Tab` and `Ctrl+Shift+Tab` switch tabs. Click the pencil, double-click a tab, or press `F2` to name it. `Ctrl+W` asks before deleting the current tab. Press `Ctrl+S` to save the current tab to a file; the first save suggests an extension for the selected language and asks where to put it. `Ctrl+Shift+S` saves it to a different file. Edits continue to be saved automatically for recovery, but the chosen file is updated only when you press `Ctrl+S`. `Ctrl+Q` closes the window. Closing saves pending edits for recovery. If that save fails, the window stays open and shows the error.

The first version's single note is imported automatically when you first open this version. Its original `note.txt` is kept as a backup.

Run `./bin/test` to verify note persistence. On Omarchy, `./bin/install` installs the package and launcher entry.
