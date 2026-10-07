# ortha – Design-Spec (v1)

## Kontext

"ortha" ist eine intuitive To-Do-App für das Terminal (TUI), geschrieben in **C++**. Sie soll unter **Linux und Windows** laufen. Sync ist ein späteres Ziel; v1 bereitet das Datenmodell nur darauf vor.

### Anforderungen (vom Nutzer festgelegt)
- Start im Terminal → Startmenü: **New task, Today, All Tasks, Quit** (nichts weiter)
- In "All Tasks" Tasks bearbeiten
- Task-Felder: Titel, **Fälligkeitsdatum, Priorität, Notizen, Tags**
- **Today** = heute fällig + überfällig (überfällige markiert)
- Erledigte Tasks bleiben sichtbar (abgehakt), **ein-/ausblendbar**, wieder öffnbar
- v1-Extras: **wiederkehrende Tasks (flexibel)** und **Unteraufgaben (einfache Checkliste, eine Ebene)**
- Bedienung: **nur Pfeiltasten/Enter** (Esc = zurück, Tab zwischen Feldern als Ergänzung)
- Datum: festes Format **TT.MM.JJJJ**; UI-Sprache **Englisch**
- Speicher: **SQLite**; Datenmodell **sync-ready** (UUIDs, `updated_at`, Soft-Delete)
- Keine CLI-Befehle, nur TUI
- **FTXUI**, **Catch2**-Unit-Tests, **GitHub Actions CI** (Linux + Windows)
- Arbeitsweise: **Claude schreibt in kleinen Schritten und erklärt die C++-Konzepte**; der Nutzer kann jederzeit selbst Code schreiben.

### Annahmen
- Vorerst ein einzelner Nutzer; keine Accounts/Mehrbenutzer.
- Prioritäten: none / low / medium / high.
- Sortierung fest: überfällig → Fälligkeitsdatum (ohne Datum zuletzt) → Priorität absteigend → Erstellzeit.
- DB-Ort: Linux `$XDG_DATA_HOME/ortha/ortha.db` (Fallback `~/.local/share/ortha/`), Windows `%APPDATA%\ortha\ortha.db`; überschreibbar per Umgebungsvariable `ORTHA_DB` (praktisch für Tests).
- Löschen = Soft-Delete (Flag), mit Bestätigungsdialog.

### Bewusst NICHT in v1
Sync, CLI-Befehle, Suche/Filter, wählbare Sortierung, Upcoming-/Tag-Ansichten, Settings, Mehrsprachigkeit, Maus, Release-Binaries, verschachtelte Subtasks.

## Tech-Stack

| Baustein | Wahl | Warum |
|---|---|---|
| Sprache | **C++20** | `std::chrono`-Kalender (`year_month_day`, `weekday`) ideal für Datum & Wiederholungen |
| Build | **CMake + Ninja**, Dependencies via **FetchContent** | ein Weg für Linux & Windows, kein Paketmanager nötig |
| TUI | **FTXUI** | moderne Komponenten (Menu, Input, Checkbox, Modal), nativ auf Windows |
| DB | **SQLite-Amalgamation** + **eigener dünner RAII-Wrapper** | lehrreich (RAII, `unique_ptr` mit Custom Deleter, Prepared Statements) statt fertiger Lib |
| Tests | **Catch2 v3** | testet Core-Logik; TUI manuell |
| CI | GitHub Actions: `ubuntu-latest` (GCC) + `windows-latest` (MSVC) | Windows-Brüche sofort sichtbar |

Toolchain (Fedora): `sudo dnf install gcc-c++ cmake ninja-build git`

## Architektur

Zwei Schichten, damit die Logik ohne TUI testbar ist:

```
ortha/
├── CMakeLists.txt
├── cmake/Dependencies.cmake        # FetchContent: ftxui, sqlite3, Catch2
├── src/core/   → Bibliothek ortha_core (keine UI-Abhängigkeit)
│   ├── date.hpp/.cpp               # parse/format TT.MM.JJJJ, today(), ISO für DB
│   ├── task.hpp                    # struct Task, Subtask, Priority
│   ├── recurrence.hpp/.cpp         # RecurrenceRule, next_due(), (de)serialize
│   ├── uuid.hpp/.cpp               # UUID v4
│   ├── sqlite.hpp/.cpp             # RAII: Database, Statement
│   ├── task_repository.hpp/.cpp    # CRUD, Queries (today, all), Schema/Migrationen
│   ├── task_service.hpp/.cpp       # complete()/reopen() inkl. Wiederholungslogik
│   └── paths.hpp/.cpp              # DB-Pfad je OS
├── src/tui/    → Executable ortha
│   ├── main.cpp
│   ├── app.hpp/.cpp                # Screen-Navigation (Menü ↔ Listen ↔ Formular)
│   ├── main_menu.cpp
│   ├── task_list_view.cpp          # gemeinsam für Today & All Tasks
│   └── task_form.cpp               # New/Edit inkl. Subtasks & Wiederholung
├── tests/                          # test_date, test_recurrence, test_repository (":memory:"), test_service
├── .github/workflows/ci.yml
├── .clang-format, .gitignore, README.md
└── docs/superpowers/specs/2026-10-07-ortha-design.md   # dieses Design als Spec
```

## Datenmodell (SQLite, Version über `PRAGMA user_version`)

- **tasks**: `id TEXT PK (UUID)`, `title TEXT NOT NULL`, `notes TEXT`, `due_date TEXT NULL` (ISO `YYYY-MM-DD` gespeichert, angezeigt TT.MM.JJJJ), `priority INTEGER 0–3`, `done INTEGER`, `completed_at TEXT`, `recurrence TEXT NULL`, `created_at`, `updated_at`, `deleted INTEGER`
- **task_tags**: `task_id`, `tag TEXT`, PK(task_id, tag) – Eingabe kommagetrennt, normalisiert (trim, lowercase)
- **subtasks**: `id UUID`, `task_id`, `title`, `done`, `position`, `updated_at`, `deleted`

Zeitstempel in UTC ISO-8601. Alle Queries filtern `deleted = 0`.

## Wiederkehrende Tasks

`RecurrenceRule { freq: daily|weekly|monthly|yearly; interval ≥ 1; weekdays (nur weekly, optional); anchor: due|completion }`, serialisiert RRULE-ähnlich, z.B. `FREQ=WEEKLY;INTERVAL=2;BYDAY=MO,WE,FR;FROM=DUE`.

- Wiederholung erfordert ein Fälligkeitsdatum (Validierung im Formular).
- **Abhaken** eines wiederkehrenden Tasks: aktueller Task → done; neuer Task wird angelegt (gleiche Felder/Tags, Subtasks zurückgesetzt) mit `next_due(basis)`, Basis = altes Fälligkeitsdatum bzw. Erledigungsdatum.
- Regeln: monthly klemmt auf Monatsende (31. → 30./28.), yearly 29.02. → 28.02.; weekly mit Wochentagen: nächster gewählter Tag nach Basis, beim Überschreiten der Woche `interval-1` Wochen überspringen.
- Wieder öffnen eines erledigten Tasks löscht die bereits erzeugte Folgeinstanz **nicht** (einfach & vorhersehbar).

## TUI-Ablauf

- **Main Menu**: zentrierte Box „ortha“, Menü New task / Today (mit Anzahl) / All Tasks / Quit.
- **Today / All Tasks** (gleiche View, anderer Query): erste Zeile Aktionen `[Back] [Show done: off]`, darunter Liste: `[ ]/[x]`, Prioritäts-Marker (`!!!`), Titel, Datum (überfällig rot, heute gelb), Tags, Subtask-Fortschritt (`2/5`). Enter auf Task → Aktionsmenü: Toggle done / Edit / Delete / Cancel. Esc = zurück.
- **Task-Formular** (New & Edit): Title, Due (TT.MM.JJJJ, leer erlaubt), Priority (Toggle), Tags, Notes (mehrzeilig), Repeat (none/daily/weekly/monthly/yearly, Interval, Wochentage-Checkboxen, From due/completion), Subtasks (Liste + Feld zum Hinzufügen, Enter togglet/entfernt), Buttons Save / Cancel. Ungültige Eingaben → Fehlerzeile, kein Speichern.
- Fehler beim Öffnen der DB → verständliche Meldung + Exit-Code ≠ 0, kein Absturz.

## Umsetzung in Meilensteinen

Der detaillierte Schritt-für-Schritt-Plan liegt separat unter `docs/superpowers/plans/`. Jeder Schritt mit Erklärung der C++-Konzepte; Core-Logik testgetrieben (Catch2).

0. **Setup**: Toolchain-Check, `git init`, CMake-Skelett, FetchContent, „Hello FTXUI“, ein Dummy-Test, CI-Workflow.
1. **Core**: date, uuid, sqlite-Wrapper, Task-Modell, Repository + Schema – mit Tests.
2. **TUI-Grundgerüst**: Main Menu, New task (Title/Due/Priority/Notes/Tags), All Tasks, Today.
3. **Bearbeiten**: Edit, Delete (bestätigt), Toggle done, Show/hide done.
4. **Subtasks**.
5. **Wiederholungen** (recurrence + task_service, intensiv getestet).
6. **Feinschliff**: README, Farben, Edge Cases, CI grün auf Linux + Windows.

## Verifikation

- `cmake -S . -B build -G Ninja && cmake --build build && ctest --test-dir build --output-on-failure` → alle Tests grün.
- Manueller TUI-Durchlauf mit `ORTHA_DB=/tmp/.../test.db ./build/ortha`: Task anlegen (mit Datum heute, gestern, morgen) → Today zeigt heute + gestern (rot); All Tasks zeigt alle; Edit/Delete/Toggle; Show done; Subtasks abhaken; wiederkehrenden Task abhaken → Folgetask mit korrektem Datum erscheint; App neu starten → Daten persistent.
- Ungültiges Datum (`32.13.2026`) wird abgelehnt.
- CI-Lauf auf GitHub (Linux + Windows) grün, sobald Remote verbunden ist.
