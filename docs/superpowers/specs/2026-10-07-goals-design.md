# ortha – Goals (Tagesziele) – Design-Spec

> **Abgelöst** durch `2026-10-10-routines-design.md`: Goals sind jetzt Habits in Routinen, der Streak folgt „never miss twice“, der Menüpunkt heißt „Routines“.

## Kontext

Neben konkreten Tasks („Mathe Hausübung machen“) will der Nutzer **Tagesziele** im Stil eines Habit-Trackers: „Ins Gym gehen“, „4h lernen“. Ein Ziel gilt an bestimmten Wochentagen jeweils neu; am Abend wird bewertet, ob es geschafft wurde. Ziele sind eine **eigene Art von Eintrag** – sie erscheinen nicht in All tasks und belegen keine Today-Task-Plätze.

### Anforderungen (vom Nutzer festgelegt)
- Tagesziele (keine Meilensteine), Habit-Tracker-Prinzip: pro Tag „geschafft“ / „nicht geschafft“
- Pro Ziel **wählbare Wochentage** (z.B. Gym Mo/Mi/Fr); an anderen Tagen erscheint es nicht und bricht keinen Streak
- **Expliziter Status** pro Tag: offen / geschafft ✓ / nicht geschafft ✗
- **Verlauf + Streak**
- **Nachtragen beliebiger vergangener Tage**
- Ziele erscheinen **in Today** (Abschnitt oben) und haben einen **eigenen Menüpunkt „Goals“**

### Annahmen
- Ein Ziel hat nur Titel und Wochentage (keine Menge/Einheit, keine Priorität, keine Tags).
- Ziele gelten ab ihrem Anlagedatum; Tage davor zählen nicht.
- Löschen = Soft-Delete; der Verlauf bleibt in der Datenbank, das Ziel verschwindet überall.
- Streak-Symbol `🔥`.

### Nicht enthalten
Meilensteine, Mengen/Fortschritt („2,5/4 h“), Statistiken über den Streak hinaus, Erinnerungen, Pausieren/Archivieren von Zielen, Sortieren der Ziele.

## Verhalten

### Status und Streak
- Status pro (Ziel, Tag): `open` (Standard), `done` ✓, `missed` ✗.
- **Geplanter Tag** = Wochentag ist beim Ziel gewählt **und** Tag ≥ Anlagedatum.
- **Streak** (Stand „heute“): Anzahl aufeinanderfolgender geplanter Tage mit `done`, rückwärts gezählt.
  - Heute `open` oder heute nicht geplant → wird übersprungen, bricht nichts.
  - Heute `missed` → Streak 0.
  - Ein vergangener geplanter Tag mit `open` (nie bewertet) oder `missed` beendet die Zählung.
  - Nicht geplante Tage werden übersprungen.
- Zustandswechsel per Enter (überall gleich): `open` → `done` → `missed` → `open`.

### Today
Abschnitt **Goals** über den Tasks, nur wenn heute mindestens ein Ziel geplant ist:
```
╭ Today ──────────────────────────────────────╮
│ Goals                                       │
│ [✓] Gym                               🔥 5  │
│ [ ] 4h lernen                         🔥 2  │
├─────────────────────────────────────────────┤
│ [ ] !!! Steuer abgeben          06.10.2026  │
│ ...                                         │
├─────────────────────────────────────────────┤
│ > New task                                  │
│   All tasks (12)                            │
│   Goals                                     │
│   Quit                                      │
╰─────────────────────────────────────────────╯
```
- Zeile: Status `[ ]` / `[✓]` (grün) / `[✗]` (rot), Titel, rechts `🔥 N` (nur wenn N > 0).
- Reihenfolge: nach Anlagezeit.
- Enter auf einem Ziel wechselt den heutigen Status; der Streak aktualisiert sich sofort.
- Menü: `New task`, `All tasks (N)`, `Goals`, `Quit`; der Fokus startet weiterhin auf `New task`.

### Goals-Ansicht (Menüpunkt „Goals“)
```
╭ Goals ──────────────────────────────────────╮
│ Back  New goal                              │
├─────────────────────────────────────────────┤
│ Gym          Mo We Fr   ·✓··✓·✗             │
│ 4h lernen    daily      ✓✓✗✓✓✓○   🔥 3      │
╰─────────────────────────────────────────────╯
```
(Beispiel für heute = Mittwoch 07.10.: Gym heute ✗ → kein Streak; „4h lernen“ heute noch offen, davor 3× ✓ → 🔥 3.)
- Pro Ziel: Titel, Tage (`daily` oder Kürzel `Mo Tu We Th Fr Sa Su`), Leiste der letzten 7 Tage (älteste links, heute rechts: `✓` done, `✗` missed, `○` offen, `·` nicht geplant), Streak.
- Enter auf Ziel → Popup `History` / `Edit` / `Delete` / `Cancel`; Delete mit Bestätigung `Delete "<title>"?`.
- `New goal` → Zielformular. Esc / Back → Today. Ohne Ziele: „no goals yet.“

### History (pro Ziel)
- Liste aller geplanten Tage vom Anlagedatum bis heute, **neueste oben**: `We 07.10.2026  [✓]`.
- Enter wechselt den Status des Tages (Nachtragen). Esc → Goals-Ansicht.

### Zielformular (New/Edit)
- Felder: `Title`, `Days` (Checkboxen Mo–Su, bei neuen Zielen alle angehakt), Buttons `Save` / `Cancel`.
- Fehler: `Title must not be empty`, `Choose at least one day`. Enter im Titel speichert, Esc bricht ab (wie Task-Formular).

## Datenmodell (Schema v2)

Migration v1 → v2 legt an (bestehende Daten bleiben unverändert):
- **goals**: `id TEXT PK (UUID)`, `title TEXT NOT NULL`, `weekdays INTEGER NOT NULL` (Bitmaske, Bit 0 = Montag … Bit 6 = Sonntag; 127 = täglich), `start_date TEXT NOT NULL` (ISO, lokales Anlagedatum), `created_at`, `updated_at`, `deleted INTEGER NOT NULL DEFAULT 0`
- **goal_entries**: `goal_id TEXT NOT NULL REFERENCES goals(id)`, `day TEXT NOT NULL` (ISO), `status TEXT NOT NULL` (`open`/`done`/`missed`), `updated_at`, PK(goal_id, day). Zurück auf „offen“ setzt `status = 'open'` (Zeile bleibt, sync-freundlich).

Die Migration wandert aus `TaskRepository` in ein eigenes Modul `core/schema` (`migrate(Database&)`), das beide Repositories aufrufen; neue DB → direkt v2; `user_version` > 2 → Fehler wie bisher.

## Architektur

```
src/core/
├── schema.hpp/.cpp          # migrate(): v0→v1→v2, Versionsprüfung
├── goal.hpp/.cpp            # Goal, GoalStatus, is_scheduled(), streak(), next_status()
└── goal_repository.hpp/.cpp # CRUD, scheduled_on(day), status/set_status, statuses(von, bis)
src/tui/
├── goal_form.hpp/.cpp       # New/Edit goal
├── goals_view.hpp/.cpp      # Liste mit 7-Tage-Leiste + Popup
├── goal_history_view.hpp/.cpp
└── home_view.cpp            # + Goals-Abschnitt, + Menüpunkt "Goals"
```
Streak und Planung sind reine Funktionen (ohne DB) und damit direkt testbar.

## Verifikation

- Unit-Tests: `is_scheduled` (Wochentage, vor Anlagedatum), `streak` (nur done, Lücke durch missed, Lücke durch nie bewertet, heute offen, heute missed, nicht geplante Tage überspringen), `next_status`-Zyklus, Repository (CRUD, Soft-Delete, set_status inkl. zurück auf open, scheduled_on), Migration v1-Datenbank → v2 mit erhaltenen Tasks, Views (Today-Abschnitt nur bei geplanten Zielen, Enter wechselt Status, Goals-Leiste, History-Nachtragen, Formular-Validierung).
- tmux-Durchlauf: Ziel „Gym“ Mo/Mi/Fr + „4h lernen“ täglich anlegen → Today zeigt die heute geplanten; abhaken → 🔥 1; im History gestern als ✓ nachtragen → Streak steigt; App neu starten → alles da; alte v1-DB öffnen → Tasks erhalten, Ziele nutzbar.
