# ortha

A simple, keyboard-driven to-do app for the terminal. Runs on Linux and Windows.

## Features

- **Today** (start screen): today's routines and your next four open tasks, the soonest first,
  with the menu below: *New task*, *All tasks (N open)*, *Routines*, *Quit*.
  At the very bottom: a bible verse (Luther 1912), picked at random on every start.
- Due dates read `today`, `tomorrow` or e.g. `12.10.2026 (Mo)`; overdue in red, today in yellow.
- **All tasks**: every open task with its notes, sorted by due date and priority; done tasks can
  be shown with *Show done*
- Tasks have a title, due date (`DD.MM.YYYY`), priority, tags, notes and a checklist of subtasks
- Repeating tasks: daily, weekly (on chosen weekdays), monthly or yearly, every N intervals,
  counted from the due date or from the day you complete it.
  Completing a repeating task creates the next one automatically (marked with ↻).

## Routines

ortha helps you check every day, in a minimal way, whether you are on the right path. Habits
live in **routines** – ordered steps with a time window, e.g. a morning routine at 8:00:
get up → drink water → make the bed → pray. Each step cues the next one.

- A routine has a name, a start time, an optional end time and an optional sentence about who
  you want to be ("I start the day with prayer").
- Its **habits** keep their order (*Move up* / *Move down*) and their own weekdays (daily, or
  e.g. Mo We Fr). A routine shows up on every day one of its habits is planned.
- A habit can have a short **symbol** – up to 2 emoji or 4 characters, e.g. `💪` or `📚4h`.
- **Today** shows the routine that is on now with its habits and marks the next open step
  with `>`. Earlier routines stay open while something is left; the others take one line.
- Enter on a habit rates the day: `[ ]` open → `[✓]` done → `[✗]` missed → `[·]` not planned
  today (a rest day). Enter on a routine heading opens or closes it; ortha keeps that choice
  for the day. To pause a whole routine, set its days to `·` in the month grid.
- The routine on now is cyan (`▶`); routines that have begun and still have open habits are
  yellow.
- For routines the day changes at 4:00, so a night routine may run past midnight
  (e.g. 23:30–0:30) and still counts for its evening.
- 🔥 shows the streak – **never miss twice**: one missed day is bridged, two in a row end it.
  Rest days and days off neither count nor break it. A habit missed last time says so.
- **Routines** in the menu lists all routines and habits with the last seven days (`✓` done,
  `✗` missed, `○` open, `·` not planned). Enter on a routine: *Add habit*, *Edit*, *Delete*;
  on a habit: *History* (rate any past day), *Edit*, *Move up*, *Move down*, *Delete*.
- **Month** in the routines view shows a habit-tracker grid: one column per habit (headed by
  its symbol), one row per day. Move with the arrow keys, Enter rates the selected day,
  PgUp/PgDn switch the month.
- Goals from older ortha versions move into a routine called "My routine" on the first start.

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
| PgUp / PgDn | previous / next month in the goal month grid |

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
