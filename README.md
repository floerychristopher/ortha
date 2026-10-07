# ortha

A simple, keyboard-driven to-do app for the terminal. Runs on Linux and Windows.

## Features

- **Today** (start screen): your next three open tasks, the soonest first (overdue dates in red,
  today in yellow), with the menu below: *New task*, *All Tasks (N open)*, *Quit*
- **All Tasks**: every open task, sorted by due date and priority; done tasks can be shown with *Show done*
- Tasks have a title, due date (`DD.MM.YYYY`), priority, tags, notes and a checklist of subtasks
- Repeating tasks: daily, weekly (on chosen weekdays), monthly or yearly, every N intervals,
  counted from the due date or from the day you complete it.
  Completing a repeating task creates the next one automatically (marked with ↻).

## Goals

Goals are daily habits like "Go to the gym" or "Study 4h" – separate from tasks.

- Each goal applies on chosen weekdays (daily, or e.g. Mo We Fr).
- The goals planned for today appear at the top of **Today**. Press Enter to rate the day:
  `[ ]` open → `[✓]` done → `[✗]` missed.
- 🔥 shows your streak: consecutive planned days marked done. An unrated past day breaks it.
- **Goals** in the menu lists all goals with the last seven days (`✓` done, `✗` missed,
  `○` open, `·` not planned). Enter on a goal: *History* (rate any past day), *Edit*, *Delete*.

## Usage

```sh
ortha
```

| Key | Action |
|---|---|
| ↑ ↓ ← → | move between entries and fields |
| Tab / Shift+Tab | next / previous field in the task form |
| Enter | open / select; in a task list: open the actions (done, edit, delete); in a one-line form field: save |
| Esc | back / close popup |

## Build

Requirements: a C++20 compiler (GCC 14+ or Visual Studio 2022), CMake 3.25+, Ninja.
All libraries (FTXUI, SQLite, Catch2) are downloaded automatically by CMake.

```sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
./build/src/tui/ortha
```

On Windows, run the same commands from a *Developer PowerShell for VS 2022*
(it provides the compiler and Ninja).

## Data

Tasks are stored in a SQLite database:

- Linux: `$XDG_DATA_HOME/ortha/ortha.db` (usually `~/.local/share/ortha/ortha.db`)
- Windows: `%APPDATA%\ortha\ortha.db`

Set the environment variable `ORTHA_DB` to use a different file.
