# ortha – Routinen und Habits – Design-Spec

## Kontext

ortha soll jeden Tag auf minimalistische Weise zeigen, ob man „am richtigen Weg“ ist – mit Betonung auf dem christlichen Glauben. Die bisherigen Goals (einzelne Tagesziele) werden zu **Habits in Routinen**: geordnete Abfolgen mit Zeitfenster, z.B. Morgenroutine (aufstehen → Wasser → Bett machen → beten). Grundlage sind *Atomic Habits* (James Clear) und *The Power of Habit* (Charles Duhigg):

- **Habit Stacking / Auslöser:** Jeder Schritt einer Routine ist der Auslöser für den nächsten – die Reihenfolge ist Teil der Disziplin.
- **Offensichtlich machen:** Today zeigt, welche Routine gerade dran ist und welcher Schritt als Nächstes kommt.
- **Never miss twice:** Ein Aussetzer ist ein Ausrutscher, erst zwei hintereinander brechen die Kette.
- **Identität:** Pro Routine ein optionaler Satz, wer man sein will („Jede Handlung ist eine Stimme“).

### Anforderungen (vom Nutzer festgelegt)
- Routinen mit Name, Startzeit, optionaler Endzeit, optionalem Identitätssatz und Habits in **fester Reihenfolge**
- **Jedes Habit gehört zu einer Routine** – freie Goals gibt es nicht mehr
- Wochentage **pro Habit** (wie bisher); eine Routine erscheint an Tagen, an denen mindestens eines ihrer Habits geplant ist
- **Spontaner Ruhetag:** ein Habit für einen Tag auf „nicht geplant“ (`·`) setzen – kein neues Symbol; eine ganze Routine pausiert man im Monatsraster (jede Zelle per Enter auf `·`)
- **Tageswechsel um 4:00** für Routinen und Habits; Routinen dürfen über Mitternacht gehen (z.B. 23:30–0:30)
- Today: aktuelle Routine und frühere Routinen mit offenen Habits ausgeklappt, alle anderen eine Zeile
- „Never miss twice“ für Habit- und Routine-Streaks
- Benennung: Menüpunkt **„Routines“**, Einträge **„Habits“**
- Nutzung: morgens und nachmittags/abends am PC

### Nicht enthalten (bewusst, ortha bleibt minimalistisch)
Erinnerungen/Benachrichtigungen, Zeitmessung, Punkte/Level/Abzeichen, Statistik-Dashboards, „attraktiv machen“-Features, Limit für Ruhetage. Für eine spätere Runde vorgemerkt: Mini-Version pro Habit (Zwei-Minuten-Regel), Bestätigung „Routine abgeschlossen“.

## Verhalten

### Status
- Pro (Habit, Tag): `open` (Standard), `done` ✓, `missed` ✗, **`skipped`** – angezeigt als `·` („heute nicht geplant“, Ruhetag).
- Enter wechselt überall (Today, History, Monatsraster): `open` → `done` → `missed` → `skipped` → `open`.
- Ein `skipped`-Tag zählt wie ein nicht geplanter Tag: bricht keine Kette, zählt nicht mit.

### Streaks („Never miss twice“)
**Habit-Streak** (Stand heute), rückwärts über die geplanten, nicht übersprungenen Tage ab dem Anlagedatum:
- `done` zählt +1.
- Ein **Aussetzer** (`missed`, oder ein vergangener Tag, der `open` geblieben ist) zählt nicht, bricht aber nicht – außer der nächstältere geplante Tag ist ebenfalls ein Aussetzer: **zwei Aussetzer hintereinander beenden die Zählung**.
- Heute: `open` wird übersprungen (der Tag läuft noch); `missed` heute ist ein Aussetzer.
- Nicht geplante und `skipped`-Tage werden übersprungen.

**Routine-Streak:** gleiche Regel, angewandt auf Routine-Tage. Ein Routine-Tag ist ein Tag, an dem mindestens ein Habit der Routine geplant und nicht `skipped` ist. Er zählt als geschafft, wenn alle diese Habits `done` sind; heute zählt er erst, wenn alles `done` ist, und ist ein Aussetzer, sobald eines `missed` ist, sonst wird heute übersprungen.

**Hinweis „missed last time“:** Ist ein Habit heute noch `open` und war sein letzter geplanter, nicht übersprungener Tag vor heute ein Aussetzer, steht gedimmt `missed last time` dahinter. (Bezug auf den letzten geplanten Tag statt „gestern“, damit z.B. Gym Mo/Mi/Fr am Mittwoch korrekt auf Montag schaut.)

### Today
```
╭ Today ─────────────────────────────────────────────────╮
│ Morning routine      8:00            4/4 ✓  🔥 12      │
│ Focus time           10:00–16:00     0/1               │
│   I do deep work before anything else.                 │
│ > [ ] min. 3h quality time                             │
│ Sport                19:30–21:30     0/1               │
│ Night routine        23:00           0/5               │
├────────────────────────────────────────────────────────┤
│ [ ]     Steuer abgeben                  06.10.2026 (Tu)│
│ ...                                                    │
```
- Es erscheinen die Routinen, von denen heute mindestens ein Habit geplant ist (auch wenn es `skipped` ist), sortiert nach Startzeit innerhalb des Routinen-Tages (0:30 kommt nach 23:00; bei Gleichstand nach Anlagezeit). Ohne solche Routinen kein Abschnitt.
- Überschrift: Name, Zeit (`8:00` bzw. `19:30–21:30`), Fortschritt `erledigt/geplant` (ohne `skipped`), `✓` wenn alles erledigt, `🔥 N` wenn N > 0. Stehen alle Habits heute auf `skipped`, zeigt die Überschrift `·` statt des Fortschritts.
- **Aktuelle Routine:** die zuletzt begonnene (Startzeit ≤ jetzt, im Routinen-Tag); ist noch keine begonnen, die erste des Tages.
- **Automatisch ausgeklappt** sind: die aktuelle Routine und jede früher begonnene Routine, die heute noch offene Habits hat – jeweils nur, solange sie offene Habits hat. Alle anderen zeigen nur die Überschrift. Die Routine des zuletzt bewerteten Habits bleibt offen, damit die Markierung darauf bleibt.
- **Enter auf eine Überschrift** klappt die Routine auf bzw. zu – auch eine, die noch nicht begonnen hat oder fertig ist. Diese Wahl gilt, solange ortha läuft, bis zum nächsten Tageswechsel (4:00), und hat Vorrang vor dem automatischen Ein-/Ausklappen.
- **Farben:** die Routine, in deren Zeitfenster jetzt liegt, ist cyan mit `▶` (Fenster: Start bis Ende, ohne Ende bis zum Start der nächsten Routine, bei der letzten bis zum Tageswechsel); begonnene Routinen mit offenen Habits sind gelb; fertige und spätere neutral.
- In einer ausgeklappten Routine: Identitätssatz gedimmt (falls gesetzt), dann die Habits in Reihenfolge: `[ ]`/`[✓]`/`[✗]`/`[·]`, Symbol und Titel, rechts `🔥 N` bzw. `missed last time`. Das erste offene Habit ist mit `>` markiert.
- Enter auf ein Habit: Status-Zyklus (siehe oben); Streaks aktualisieren sich sofort.
- **Tageswechsel um 4:00:** von 0:00 bis 3:59 zählt für Routinen und Habits (Today, Abhaken, Streaks, Raster, Anlagedatum neuer Habits) noch der Vortag. Tasks und ihre Fälligkeiten wechseln weiter um Mitternacht.
- Der Bibelvers passt sich wie bisher an die verbleibende Höhe an.

### Routines-Ansicht (Menüpunkt „Routines“, ersetzt „Goals“)
```
╭ Routines ──────────────────────────────────────────────╮
│ Back  New routine  Month                               │
├────────────────────────────────────────────────────────┤
│ Morning routine    8:00                         🔥 12  │
│   ⏰ Get up at 8      daily      ✓✓✓✓✓✓○   🔥 12      │
│   🙏 Pray             daily      ✓✓✓✓✓✓○   🔥 12      │
│ Sport              19:30–21:30                   🔥 3  │
│   🏋 Gym              Mo We Fr   ·✓·✓·✓·   🔥 3       │
╰────────────────────────────────────────────────────────╯
```
- Routinen nach Startzeit, darunter eingerückt ihre Habits nach Position: Symbol, Titel, Tage, 7-Tage-Leiste (`✓ ✗ ○ ·`, `skipped` als `·`), Streak.
- Enter auf Routine → Popup `Add habit` / `Edit` / `Delete` / `Cancel`; Delete fragt `Delete "<name>" and its N habit(s)?` und löscht Routine samt Habits (Soft-Delete, Verlauf bleibt).
- Enter auf Habit → Popup `History` / `Edit` / `Move up` / `Move down` / `Delete` / `Cancel`; Move up/down tauscht mit dem Nachbarn innerhalb der Routine (am Rand wirkungslos).
- Ohne Routinen: „no routines yet.“ (gedimmt). Esc / Back → Today.

### Formulare
- **Routine** (`New routine` / `Edit routine`): `Name`, `Start` (HH:MM, Pflicht), `End` (HH:MM, optional), `Identity` (optional; Platzhalter `e.g. i start the day with prayer`). Fehler: `Name must not be empty`, `Start must be HH:MM`, `End must be HH:MM`, `End must be after start` (innerhalb des Routinen-Tages: 23:30–0:30 und 22:00–4:00 sind gültig – ein Ende um 4:00 ist der Tageswechsel –, 10:00–9:00 und 3:00–5:00 nicht). Uhrzeiten akzeptieren `8:00`, `08:00`, `23:59`.
- **Habit** (`New habit` / `Edit habit`): `Title`, `Symbol`, `Routine` (Auswahl der Routinen, bei „Add habit“ vorausgewählt), `Days`. Neue oder in eine andere Routine verschobene Habits kommen ans Ende der Routine.

### Monatsraster und History
- Raster-Spalten in derselben Reihenfolge wie die Routines-Ansicht. `skipped` erscheint als `·`, ist aber (anders als nicht geplante Tage) per Enter weiterschaltbar.
- History pro Habit wie bisher; `skipped`-Tage als `[·]`.

### Casing
Beschriftungen in Sentence case („New routine“, „Add habit“, „Move up“); Platzhalter und gedimmte Texte komplett klein.

## Datenmodell (Schema v4)

Migration v3 → v4 (bestehende Daten bleiben erhalten):
```sql
CREATE TABLE routines (
    id           TEXT PRIMARY KEY,
    name         TEXT NOT NULL,
    start_minute INTEGER NOT NULL,          -- Minuten seit Mitternacht, 8:00 = 480
    end_minute   INTEGER,                   -- NULL = keine Endzeit
    identity     TEXT NOT NULL DEFAULT '',
    created_at   TEXT NOT NULL,
    updated_at   TEXT NOT NULL,
    deleted      INTEGER NOT NULL DEFAULT 0
);
ALTER TABLE goals ADD COLUMN routine_id TEXT REFERENCES routines(id);
ALTER TABLE goals ADD COLUMN position INTEGER NOT NULL DEFAULT 0;
```
- Gibt es nicht gelöschte Goals, legt die Migration die Routine **„My routine“** (00:00, ohne Ende, ohne Identität) an und ordnet alle Goals in Anlagereihenfolge zu (`position` 0, 1, 2, …).
- `goal_entries.status` kennt zusätzlich `skipped`.
- Im Code bleibt der Typ `Goal` (Tabelle `goals`); in der Oberfläche heißt er „Habit“. Ein Kommentar am Typ erklärt das.

## Architektur

```
src/core/
├── date.hpp/.cpp             # + now_minutes(): lokale Uhrzeit in Minuten seit Mitternacht
├── goal.hpp/.cpp             # + GoalStatus::Skipped, Goal::routine_id/position,
│                             #   streak() mit "never miss twice", missed_last_time(),
│                             #   next_status() mit Skipped, month_cell() mit Skipped
├── routine.hpp/.cpp          # neu: Routine, parse_time()/format_time(), current_routine(),
│                             #   Fortschritt eines Tages, routine_streak(), ist ausgeklappt?
├── routine_repository.hpp/.cpp # neu: CRUD, Reihenfolge der Habits, Ruhetag für eine Routine
├── goal_repository.cpp       # routine_id/position lesen und schreiben, Reihenfolge
└── schema.cpp                # + Schritt v3 → v4
src/tui/
├── home_view.cpp             # Routinen-Abschnitt statt Goals-Abschnitt
├── routines_view.hpp/.cpp    # ersetzt goals_view
├── routine_form.hpp/.cpp     # neu
├── goal_form.cpp             # + Feld Routine, Überschriften New/Edit habit
├── goal_widgets.hpp          # [·] für Skipped
├── goal_month_view.cpp       # Reihenfolge, Skipped
└── app.cpp                   # Menüpunkt "Routines", Verdrahtung, now_minutes()
```
Alle Regeln (Streaks, aktuelle Routine, Ein-/Ausklappen, Fortschritt) sind reine Funktionen ohne DB und UI und direkt testbar.

## Verifikation

- **Unit-Tests core:** Streak „never miss twice“ (ein Aussetzer überbrückt, zwei beenden, heute offen, heute missed, skipped überspringen, nicht geplante Tage); `missed_last_time` (inkl. Mo/Mi/Fr); Status-Zyklus mit Skipped; Routine-Streak und Tagesfortschritt; `current_routine` (vor der ersten, mitten im Tag, nach der letzten, gleiche Startzeit); `parse_time`/`format_time` (gültig/ungültig); Migration v3-DB mit Goals und Verlauf → alles in „My routine“; Repository (Routinen CRUD + Soft-Delete samt Habits, Move up/down, Ruhetag setzen/zurücknehmen).
- **Unit-Tests tui:** Today (Ein-/Ausklappen, `>`, Enter auf Habit/Überschrift, `missed last time`, `·`-Überschrift), Routines-Ansicht (Gruppierung, Popups, Move up/down, Delete mit Rückfrage), Routinen- und Habit-Formular (Validierung, vorausgewählte Routine), Raster und History mit `·`.
- **Sanitizer-Lauf** aller Tests und ein Durchlauf der App im Sanitizer-Build.
- **tmux-Durchlauf:** Morgen-, Focus-, Sport- und Nachtroutine anlegen; Today zu verschiedenen Zeiten; Habits abhaken, Ruhetag für Sport; Neustart; alte v3-DB öffnen → „My routine“ mit allen Goals und Verlauf.
