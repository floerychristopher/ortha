# Monatsraster für Goals Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Habit-Tracker-Monatsraster (Spalten = Ziele mit Nutzer-Kürzel, Zeilen = Tage), bedienbar per Cursor + Enter, erreichbar über `Month` in der Goals-Ansicht.

**Architecture:** Schema v3 ergänzt `goals.symbol`. `core/goal` bekommt reine Funktionen für Monatstage und Zellzustand. In der TUI: Kürzel-Feld im Zielformular (Breite per `ftxui::string_width`), `goal_symbol()`-Helfer, Button `Month` in der Goals-Ansicht und die neue Komponente `goal_month_view`. `App` verdrahtet alles.

**Tech Stack:** wie bisher – C++20, FTXUI v6.1.9, SQLite, Catch2.

**Spec:** `docs/superpowers/specs/2026-10-07-goal-month-grid-design.md`

## Global Constraints

- Alle Global Constraints aus `docs/superpowers/plans/2026-10-07-ortha-v1.md` und `docs/superpowers/plans/2026-10-07-goals.md` gelten (C++20, Namespace `ortha`, Warnungen, UI-Texte Englisch, Status-Zyklus `open` → `done` → `missed` → `open`, Wochentage Montag-zuerst).
- **Ausnahme Tasten:** Im Monatsraster zusätzlich **PgUp / PgDn** (Monat wechseln).
- **Commits nur auf ausdrückliche Anweisung des Nutzers**, ohne `Co-Authored-By`. Die Commit-Schritte unten sind Vorschläge.
- Kürzel: max. **4** Spalten laut `ftxui::string_width`; Fehlertext exakt `Symbol is too wide (max. 2 emoji or 4 characters)`; Ersatzkürzel = erste 4 Spalten des Titels.
- Zellsymbole exakt: `✓` grün, `✗` rot, `○` offen, `·` gedimmt (nicht geplant), leer (Zukunft). Zeilenkopf `We 07`. Fenstertitel `October 2026`. Toolbar `Back`, `< Sep`, `Nov >`. Fußzeile `<Kürzel> <Titel> · <Wd> <TT.MM.JJJJ>`, Legende `<Kürzel> <Titel>` pro Ziel.
- Vor jedem Task `git status` + betroffene Dateien neu lesen (der Nutzer editiert evtl. selbst); nach jedem Task C++-Konzepte kurz im Chat erklären.
- Build/Test: `cmake --build build && ctest --test-dir build --output-on-failure`

## Review Focus

1. **Bestehende v2-Datenbank mit Zielen und Einträgen** → wird auf v3 migriert, alles bleibt, `symbol` ist leer, Raster zeigt das Ersatzkürzel. Tests in Task 1 und Task 3.
2. **Ersatzkürzel bei breitem Zeichen an der Grenze** (Titel `abc📚`) → `abc` (Breite 3), nie Breite 5. Test in Task 3.
3. **Monatswechsel vom 31. in einen kürzeren Monat** → Cursor landet auf dem letzten Tag (31.10. + PgDn → 30.11.). Test in Task 5.
4. **Cursor verlässt das Raster nach oben** → Fokus geht in die Toolbar; Enter löst dort den Button aus, nicht `on_cycle`. Test in Task 5.
5. **Ziel mitten im Monat angelegt** → Tage davor `·`, Enter dort wirkungslos. Tests in Task 2 und Task 5.

---

### Task 1: Schema v3 und `Goal::symbol`

**Files:**
- Modify: `src/core/schema.hpp` (`kSchemaVersion = 3`, Kommentar „Version 3: goal symbols“), `src/core/schema.cpp` (`kSchemaV3`, `kSteps` erweitern), `src/core/goal.hpp` (Feld), `src/core/goal_repository.cpp` (`kSelectGoal`, `read_goal`, `insert`, `update`)
- Test: `tests/core/schema_test.cpp`, `tests/core/goal_repository_test.cpp`, `tests/core/task_repository_test.cpp:161` (`== 3`)

**Interfaces:**
- Produces: `struct Goal { …; std::string symbol; … }` – direkt nach `title`, leer = kein Kürzel. SQL: `ALTER TABLE goals ADD COLUMN symbol TEXT NOT NULL DEFAULT '';`

- [ ] **Step 1: Failing tests**
  ```cpp
  // schema_test.cpp: bestehende Checks auf user_version() == 3 umstellen,
  // "newer database is rejected" auf set_user_version(4).
  TEST_CASE("v2 database keeps its goals when upgraded") {   // Review Focus 1
      Database db(":memory:");
      migrate(db, 2);
      db.exec("INSERT INTO goals (id, title, weekdays, start_date, created_at, updated_at) "
              "VALUES ('g1', 'Gym', 21, '2026-10-05', 'x', 'x')");
      db.exec("INSERT INTO goal_entries VALUES ('g1', '2026-10-05', 'done', 'x')");
      GoalRepository repo(db);                                 // migrates to v3
      CHECK(db.user_version() == 3);
      REQUIRE(repo.find("g1"));
      CHECK(repo.find("g1")->symbol.empty());
      CHECK(repo.status("g1", 2026y / October / 5d) == GoalStatus::Done);
  }
  // goal_repository_test.cpp
  TEST_CASE("goal symbol is stored and updated") {
      /* insert mit symbol "💪" → find().symbol == "💪"; symbol = "📚4h", update → find().symbol == "📚4h" */
  }
  ```
- [ ] **Step 2: Run** — Expected: FAIL (`user_version` 2 statt 3, `symbol` kein Member).
- [ ] **Step 3: Implementieren** – Spalte in `kSelectGoal` hinten anhängen (Index 6), `insert`/`update` binden `symbol`.
- [ ] **Step 4: Run** — Expected: alle Tests PASS.
- [ ] **Step 5: Commit-Vorschlag** — `feat(core): add goal symbols (schema v3)`

### Task 2: Monatstage und Zellzustand

**Files:**
- Modify: `src/core/goal.hpp`, `src/core/goal.cpp`
- Test: `tests/core/goal_test.cpp`

**Interfaces:**
- Produces:
  ```cpp
  // All days of the month, first to last.
  std::vector<Date> days_of_month(std::chrono::year_month month);

  enum class CellState { Future, NotScheduled, Open, Done, Missed };
  // Future if day > today, else NotScheduled if !is_scheduled(goal, day), else the status.
  CellState month_cell(const Goal& goal, Date day, Date today, GoalStatus status);
  ```

- [ ] **Step 1: Failing tests** (07.10.2026 = Mittwoch; vorhandene Helfer `daily`/`mwf` aus `goal_test.cpp` nutzen)
  ```cpp
  TEST_CASE("days_of_month") {
      auto oct = days_of_month(2026y / October);
      CHECK(oct.size() == 31);
      CHECK(oct.front() == Date{2026y, October, 1d});
      CHECK(oct.back() == Date{2026y, October, 31d});
      CHECK(days_of_month(2026y / November).size() == 30);
      CHECK(days_of_month(2026y / February).size() == 28);
      CHECK(days_of_month(2028y / February).size() == 29);
  }
  TEST_CASE("month_cell") {                                      // Review Focus 5
      const Date today = 2026y / October / 7d;
      auto g = mwf(2026y / October / 5d);
      CHECK(month_cell(g, 2026y / October / 9d, today, GoalStatus::Done) == CellState::Future);
      CHECK(month_cell(g, 2026y / October / 2d, today, GoalStatus::Open) == CellState::NotScheduled); // before start
      CHECK(month_cell(g, 2026y / October / 6d, today, GoalStatus::Open) == CellState::NotScheduled); // Tuesday
      CHECK(month_cell(g, today, today, GoalStatus::Open) == CellState::Open);
      CHECK(month_cell(g, 2026y / October / 5d, today, GoalStatus::Done) == CellState::Done);
      CHECK(month_cell(g, today, today, GoalStatus::Missed) == CellState::Missed);
  }
  ```
- [ ] **Step 2: Run** — Expected: Build-Fehler (unbekannte Funktionen).
- [ ] **Step 3: Implementieren** – Monatslänge über `year_month_day_last`.
- [ ] **Step 4: Run** — Expected: PASS.
- [ ] **Step 5: Commit-Vorschlag** — `feat(core): add month days and grid cell state`

### Task 3: Kürzel im Zielformular und `goal_symbol()`

**Files:**
- Modify: `src/tui/goal_form.hpp/.cpp`, `src/tui/goal_widgets.hpp`
- Create: `tests/tui/goal_widgets_test.cpp` (in `tests/CMakeLists.txt` eintragen)
- Test: `tests/tui/goal_form_test.cpp`

**Interfaces:**
- Consumes: `Goal::symbol` (Task 1).
- Produces:
  ```cpp
  // goal_form.hpp – symbol am Ende, damit bestehende Aggregat-Initialisierungen gültig bleiben
  struct GoalFormState { std::string title; Weekdays weekdays{…}; std::string symbol; };

  // goal_widgets.hpp
  inline constexpr int kMaxSymbolWidth = 4;
  // The goal's symbol, or the first kMaxSymbolWidth columns of its title.
  std::string goal_symbol(const Goal& goal);
  ```
  `apply_goal_form`: Kürzel trimmen, Breite > 4 → Fehlertext aus Global Constraints (geprüft nach Titel und Tagen). Formular: Feld `Symbol` (Placeholder `e.g. 💪`) zwischen `Title` und `Days`, Enter speichert wie im Titel.
  `goal_symbol`: Glyphen aus `ftxui::Utf8ToGlyphs` anhängen, solange `ftxui::string_width` ≤ 4 bleibt; erstes Glyph, das überschreitet, beendet.

- [ ] **Step 1: Failing tests**
  ```cpp
  // goal_form_test.cpp
  TEST_CASE("apply_goal_form checks the symbol width") {
      GoalFormState s;
      s.title = "Gym";
      for (auto ok : {"", "💪", "📚💪", "GYM", "LESE", "📚4h"}) {
          INFO(ok); s.symbol = ok;
          CHECK(std::holds_alternative<Goal>(apply_goal_form(s, Goal{})));
      }
      for (auto bad : {"LESEN", "📚💪🏃", "📚4h!"}) {
          INFO(bad); s.symbol = bad;
          CHECK(std::get<std::string>(apply_goal_form(s, Goal{})) ==
                "Symbol is too wide (max. 2 emoji or 4 characters)");
      }
      s.symbol = " 💪 ";
      CHECK(std::get<Goal>(apply_goal_form(s, Goal{})).symbol == "💪");
  }
  // bestehende Tests: goal_form_state_from kopiert symbol; Render enthält "Symbol".
  // goal_widgets_test.cpp
  TEST_CASE("goal_symbol") {
      Goal g; g.title = "4h lernen";
      CHECK(goal_symbol(g) == "4h l");
      g.title = "Gym";        CHECK(goal_symbol(g) == "Gym");
      g.title = "abc📚";      CHECK(goal_symbol(g) == "abc");   // Review Focus 2
      g.symbol = "💪";        CHECK(goal_symbol(g) == "💪");
  }
  ```
- [ ] **Step 2: Run** — Expected: Build-Fehler (`symbol` kein Member von `GoalFormState`, `goal_symbol` fehlt).
- [ ] **Step 3: Implementieren.**
- [ ] **Step 4: Run** — Expected: PASS.
- [ ] **Step 5: Commit-Vorschlag** — `feat(tui): let goals have a symbol`

### Task 4: Goals-Ansicht – Button `Month` und Kürzel

**Files:**
- Modify: `src/tui/goals_view.hpp/.cpp`
- Test: `tests/tui/goals_view_test.cpp`

**Interfaces:**
- Consumes: `goal_symbol()` (Task 3).
- Produces: `GoalsCallbacks::on_month` (`std::function<void()>`), Toolbar `Back  New goal  Month`; Zeile beginnt mit `goal_symbol(goal) + " " + title`.

- [ ] **Step 1: Failing tests**
  ```cpp
  TEST_CASE("goals view shows symbols and opens the month grid") {
      auto goals = two_goals(); goals[0].goal.symbol = "💪";
      bool month = false;
      auto view = make_goals_view([&] { return goals; }, {.on_month = [&] { month = true; }});
      auto out = render(view);
      CHECK(contains(out, "Month"));
      CHECK(contains(out, "💪 Gym"));
      CHECK(contains(out, "Read Read"));    // Ersatzkürzel
      /* Fokus in die Toolbar (ArrowUp), ArrowRight bis "Month", Return */
      CHECK(month);
  }
  ```
- [ ] **Step 2: Run** — Expected: Build-Fehler (`on_month` fehlt).
- [ ] **Step 3: Implementieren.**
- [ ] **Step 4: Run** — Expected: PASS (bestehende Goals-Tests weiterhin grün).
- [ ] **Step 5: Commit-Vorschlag** — `feat(tui): add month button and symbols to goals view`

### Task 5: Monatsraster

**Files:**
- Create: `src/tui/goal_month_view.hpp/.cpp`, `tests/tui/goal_month_view_test.cpp` (beide CMakeLists)

**Interfaces:**
- Consumes: `days_of_month`, `month_cell`, `CellState` (Task 2), `goal_symbol` (Task 3), `next_status` nicht (das macht `App`).
- Produces:
  ```cpp
  struct MonthColumn {
      Goal goal;
      std::map<Date, GoalStatus> entries; // days without an entry are Open
  };
  // Habit-tracker grid for one month: goals as columns, days as rows.
  // Enter cycles the focused cell (only Open/Done/Missed cells), PgUp/PgDn change the month,
  // Esc goes back.
  ftxui::Component make_goal_month(std::function<std::vector<MonthColumn>(std::chrono::year_month)> load,
                                   Date today,
                                   std::function<void(const std::string& goal_id, Date day)> on_cycle,
                                   std::function<void()> on_back);
  ```
  Aufbau: `Container::Vertical({toolbar, grid})`, Fokus startet im Grid. Grid = eigene `ComponentBase` (`Focusable() == true`) mit Cursor (Tag-Index, Spalte); Pfeiltasten bewegen, ArrowUp in Zeile 0 gibt `false` zurück (Container wechselt in die Toolbar). Toolbar-Labels `< Sep` / `Nov >` ändern sich mit dem Monat (Button-Label als `const std::string*`). Zeichnen mit `gridbox`; Kopfspalte = `goal_symbol`, Breite 4 + Abstand; Cursor-Zelle `inverted | focus`; Body in `frame` (scrollt beide Richtungen); heutige Zeile `bold`. Nach wirksamem Enter und nach Monatswechsel `load` neu aufrufen; beim Monatswechsel Tag auf Monatslänge begrenzen.

- [ ] **Step 1: Failing tests** (render mit `width = 80, height = 50`; heute = Mi 07.10.2026; Daten: `g1` „Gym“, Kürzel `💪`, Mo/Mi/Fr, Start 05.10., Einträge 05.10. Done, 07.10. Missed; `g2` „Read books“, ohne Kürzel, täglich, Start 01.10., Eintrag 06.10. Done; Helfer `line_with(out, "Mo 05")` liefert die Zeile, die den Text enthält)
  ```cpp
  TEST_CASE("month grid renders header, cells, footer and legend") {
      /* out enthält "October 2026", "< Sep", "Nov >", "💪", "Read", "Th 01", "Sa 31",
         "💪 Gym · We 07.10.2026", "Read Read books" */
      CHECK(contains(line_with(out, "Mo 05"), "✓"));
      CHECK(contains(line_with(out, "We 07"), "✗"));
      CHECK(contains(line_with(out, "Tu 06"), "·"));    // Gym nicht geplant
      CHECK(contains(line_with(out, "Th 01"), "·"));    // Gym vor Start
      CHECK(contains(line_with(out, "Th 01"), "○"));    // Read offen
      CHECK_FALSE(contains(line_with(out, "Th 08"), "○")); // Zukunft leer
  }
  TEST_CASE("enter cycles the focused cell") {
      /* Return → on_cycle("g1", 07.10.); ArrowRight, Return → on_cycle("g2", 07.10.); load erneut aufgerufen */
  }
  TEST_CASE("enter does nothing on unscheduled or future cells") {   // Review Focus 5
      /* ArrowUp (06.10., Gym nicht geplant), Return → kein Aufruf;
         ArrowDown ×2 (08.10., Zukunft), Return → kein Aufruf */
  }
  TEST_CASE("page keys change the month") {                          // Review Focus 3
      /* PageUp → "September 2026", load mit 2026y/September; PageDown ×2 → "November 2026";
         zurück auf Oktober, ArrowDown ×24 (31.10.), PageDown → Fußzeile enthält "30.11.2026" */
  }
  TEST_CASE("arrow up from the first row moves into the toolbar") {   // Review Focus 4
      /* ArrowUp bis über Zeile 01 hinaus, Return → on_back aufgerufen (Back ist erster Button),
         on_cycle nicht aufgerufen */
  }
  TEST_CASE("escape leaves the grid") { /* Escape → on_back */ }
  TEST_CASE("month grid without goals") { /* load liefert {} → "No goals yet.", kein "Empty container" */ }
  ```
- [ ] **Step 2: Run** — Expected: Build-Fehler (`tui/goal_month_view.hpp` fehlt).
- [ ] **Step 3: Implementieren.**
- [ ] **Step 4: Run** — Expected: PASS.
- [ ] **Step 5: Commit-Vorschlag** — `feat(tui): add goal month grid`

### Task 6: Verdrahtung, README, Abnahme

**Files:** Modify `src/tui/app.hpp/.cpp` (`void show_goal_month();`; Goals-Callback `.on_month`), `README.md` (Goals-Abschnitt: Kürzel + Monatsraster).

`show_goal_month()`: `load(month)` = für jedes `goals_.all()` ein `MonthColumn` mit `goals_.statuses(id, erster Tag, letzter Tag)`; `on_cycle(id, day)` = `set_status(id, day, next_status(status(id, day)))`; `on_back` = `show_goals()`; `today = today()`.

- [ ] **Step 1: Implementieren** (App hat keine Unit-Tests; Logik steckt in getesteten Teilen).
- [ ] **Step 2: Volltest** — Build/Test-Befehl, plus Release-Build ohne Warnungen. Expected: alles PASS, 0 Warnungen.
- [ ] **Step 3: tmux-Abnahme** (frische `ORTHA_DB`): „Gym“ (Mo/Mi/Fr, Kürzel `💪`) und „4h lernen“ (täglich, Kürzel `📚`) anlegen; Kürzel `LESEN` → Fehlermeldung; Goals zeigt `💪 Gym`; `Month` → aktueller Monat, Cursor auf heute; heute abhaken, gestern bei „4h lernen“ nachtragen → Esc, Goals zeigt passenden Streak; PgUp/PgDn blättern; Neustart → alles da. v2-Datenbank: Commit `2fa35d3` per `git archive | tar -xm` ins Scratchpad entpacken und bauen, damit ein Ziel mit Einträgen anlegen; dieselbe DB mit dem neuen Build öffnen → Ziel und Einträge da, Ersatzkürzel im Raster.
- [ ] **Step 4: Commit-Vorschlag** — `feat: wire goal month grid into the app` und `docs: describe goal symbols and month grid in README`
