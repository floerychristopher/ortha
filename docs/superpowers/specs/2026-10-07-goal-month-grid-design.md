# ortha – Monatsraster für Goals – Design-Spec

## Kontext

Die Goals (Tagesziele) zeigen ihren Verlauf bisher als 7-Tage-Leiste in der Goals-Ansicht und als Liste pro Ziel im History. Der Nutzer will zusätzlich ein **Monatsraster im Stil eines Habit-Tracker-Blatts**: oben die Ziele als Spalten, links die Tage des Monats als Zeilen, in jeder Zelle der Status. So sieht man auf einen Blick, an welchen Tagen man welches Ziel geschafft hat, und kann direkt im Raster bewerten und nachtragen.

### Anforderungen (vom Nutzer festgelegt)
- Raster: Spalten = Ziele, Zeilen = Tage des Monats (`Mo 01`, `Tu 02`, …)
- **Bedienbar**: Zellen per Cursor auswählen, Enter wechselt den Status (wie überall)
- Erreichbar über einen Button **`Month`** in der Goals-Ansicht; Today und History bleiben unverändert
- Spaltenköpfe sind ein **vom Nutzer gewähltes Kürzel** pro Ziel (beliebige Emojis/Zeichen, gemischt erlaubt), aufgelöst durch eine **Legende**
- Kürzel höchstens **4 Terminal-Spalten** breit (= 2 Emojis oder 4 Zeichen oder gemischt, z.B. `📚4h`); was nicht passt, verhindert eine Fehlermeldung

### Annahmen
- Das Kürzel ist optional. Ohne Kürzel zeigt das Raster die ersten 4 Spalten des Titels (`Gym`, `4h l`).
- Monate lassen sich frei vor- und zurückblättern, ohne Begrenzung.
- Gelöschte Ziele erscheinen nicht im Raster.

### Nicht enthalten
Monatsstatistiken (Quote, Summen pro Spalte/Zeile), Wochen- oder Jahresansicht, Sortieren/Ausblenden von Spalten, Kürzel-Vorschläge oder Emoji-Auswahl.

## Verhalten

### Monatsraster
```
╭ October 2026 ─────────────────────╮
│ Back  < Sep  Nov >                │
├───────────────────────────────────┤
│        🏋️   📚    Lese            │
│ Th 01  ·    ✓    ✓                │
│ Fr 02  ✓    ✓    ✗                │
│ Sa 03  ·    ✗    ✓                │
│ ...                               │
│ We 07  ✗   [○]   ○                │
│ Th 08                             │
│ ...                               │
├───────────────────────────────────┤
│ 📚 4h lernen · We 07.10.2026      │
│ 🏋️ Gym  📚 4h lernen  Lese Lesen  │
╰───────────────────────────────────╯
```
(Beispiel für heute = Mittwoch 07.10.2026, Cursor auf „4h lernen“ / heute.)

- **Titel** des Fensters: Monat und Jahr auf Englisch (`October 2026`).
- **Toolbar**: `Back`, `< Sep` (Vormonat), `Nov >` (Folgemonat).
- **Spalten**: alle nicht gelöschten Ziele in Anlagereihenfolge; Kopf = Kürzel; Spaltenbreite 4 plus Abstand.
- **Zeilen**: alle Tage des Monats, Wochentagskürzel englisch wie in `weekdays_label` (`Mo Tu We Th Fr Sa Su`) plus zweistelliger Tag. Die heutige Zeile ist fett.
- **Zellen**:
  - `✓` grün = done, `✗` rot = missed, `○` = offen (geplant, heute oder vergangen)
  - `·` gedimmt = nicht geplant (Wochentag nicht gewählt oder vor dem Anlagedatum) – auch in der Zukunft
  - leer = geplanter Tag, der nach heute liegt
  - die Cursor-Zelle ist invertiert
- **Bedienung**:
  - Pfeiltasten bewegen den Cursor über die Zellen.
  - **Enter** wechselt den Status der Cursor-Zelle `open` → `done` → `missed` → `open`; auf `·` und zukünftigen Tagen passiert nichts. Streaks in Today und Goals ergeben sich daraus automatisch.
  - **PgUp / PgDn** oder die Toolbar-Buttons wechseln den Monat; der Cursor behält Spalte und Tag (begrenzt auf die Länge des neuen Monats).
  - **Esc** oder `Back` → Goals-Ansicht.
- **Start**: aktueller Monat, Cursor auf heute in der ersten Spalte.
- **Fußzeile**: erste Zeile voller Titel + Datum der Cursor-Zelle (`📚 4h lernen · We 07.10.2026`), zweite Zeile Legende aller Kürzel (`<Kürzel> <Titel>`), bricht bei Bedarf um.
- **Scrollen**: passt das Raster nicht ins Terminal, scrollt es senkrecht und waagrecht mit dem Cursor mit.
- **Ohne Ziele**: „no goals yet.“

### Goals-Ansicht
- Toolbar: `Back  New goal  Month`.
- Das Kürzel (bzw. Ersatzkürzel) steht vor dem Titel jeder Zeile.

### Zielformular (New/Edit)
- Neues Feld `Symbol` zwischen `Title` und `Days`, Text-Eingabe, leer erlaubt.
- Leerzeichen am Rand werden entfernt.
- Breite wird mit `ftxui::string_width` gemessen (dieselbe Rechnung wie beim Zeichnen). Breite > 4 → Fehler `Symbol is too wide (max. 2 emoji or 4 characters)`.

### Bekannte Grenze
Manche Emojis (mit Variationszeichen wie `🏋️`, Flaggen, ZWJ-Sequenzen wie Familien-Emojis) stellen Terminals unterschiedlich breit dar. ortha prüft und zeichnet nach FTXUIs Breitenberechnung; weicht das Terminal davon ab, kann eine Spalte leicht verrutschen. Gängige Emojis (📚 💪 🏃) sind nicht betroffen. Es gibt keine Sonderbehandlung. (Gemessen: FTXUI rechnet `🏋`/`🏋️` als 1 Spalte, Terminals zeichnen es meist mit 2.)

## Datenmodell (Schema v3)

Migration v2 → v3 (bestehende Daten bleiben unverändert):
```sql
ALTER TABLE goals ADD COLUMN symbol TEXT NOT NULL DEFAULT '';
```
Als dritter Schritt in `kSteps` in `core/schema.cpp`; `kSchemaVersion` = 3. `user_version` > 3 → Fehler wie bisher.

`Goal` bekommt `std::string symbol` (leer = kein Kürzel). `GoalRepository` liest es beim Laden und schreibt es bei `insert` und `update`.

## Architektur

```
src/core/
├── schema.cpp               # + Schritt v2 → v3
├── goal.hpp/.cpp            # + Goal::symbol, days_of_month(), CellState, month_cell()
└── goal_repository.cpp      # symbol in SELECT / INSERT / UPDATE
src/tui/
├── goal_widgets.hpp         # + goal_symbol(): Kürzel oder erste 4 Spalten des Titels
├── goal_form.hpp/.cpp       # + Feld Symbol, Breitenprüfung in apply_goal_form()
├── goals_view.hpp/.cpp      # + Button Month (Callback on_month), Kürzel vor dem Titel
├── goal_month_view.hpp/.cpp # neu: Monatsraster
└── app.hpp/.cpp             # + show_goal_month()
```

### Reine Funktionen in `core/goal`
- `std::vector<Date> days_of_month(std::chrono::year_month month)` – alle Tage des Monats, aufsteigend.
- `enum class CellState { Future, NotScheduled, Open, Done, Missed }`
- `CellState month_cell(const Goal& goal, Date day, Date today, GoalStatus status)` – `NotScheduled` wenn `!is_scheduled(goal, day)` (auch in der Zukunft), sonst `Future` wenn `day > today`, sonst der Status. Nur bei `Open`/`Done`/`Missed` ist Enter wirksam.

### Monatsraster (`tui/goal_month_view`)
- Daten pro Monat über ein Callback, z.B.
  ```cpp
  struct MonthColumn { Goal goal; std::map<Date, GoalStatus> entries; };
  ftxui::Component make_goal_month(std::function<std::vector<MonthColumn>(std::chrono::year_month)> load,
                                   Date today,
                                   std::function<void(const std::string& goal_id, Date)> on_cycle,
                                   std::function<void()> on_back);
  ```
  `App` füllt `entries` über das bestehende `GoalRepository::statuses(id, erster, letzter Tag)`.
- Eigene `ComponentBase` mit Zustand Monat, Cursor-Zeile, Cursor-Spalte; Darstellung mit `gridbox`, Cursor-Zelle mit `focus` in einem `frame`, damit FTXUI mitscrollt.
- Nach Enter (wirksam) und Monatswechsel wird über `load` neu geladen.

## Verifikation

- **Unit-Tests core**: Migration v2-Datenbank → v3 (Ziele und Einträge erhalten, `symbol` leer); Repository speichert/lädt `symbol` (insert, update); `days_of_month` (31/30 Tage, Februar mit und ohne Schaltjahr); `month_cell` (Zukunft, vor Anlagedatum, nicht gewählter Wochentag, open/done/missed).
- **Unit-Tests tui**: Formular (2 Emojis ok, 4 Buchstaben ok, Emoji + 2 Buchstaben ok, leer ok, 5 Buchstaben bzw. 3 Emojis → Fehler, Ränder getrimmt); `goal_symbol` (Kürzel bzw. Ersatz aus Titel, kurzer Titel); Monatsraster (Kopf mit Kürzeln, Zellsymbole, Enter wechselt Status, Enter auf `·`/Zukunft ruft nichts auf, PgUp/PgDn wechselt Monat, Fußzeile mit Titel/Datum und Legende, „no goals yet.“); Goals-Ansicht zeigt Button `Month` und Kürzel.
- **tmux-Durchlauf**: Ziel „Gym“ (Mo/Mi/Fr, Kürzel `💪`) und „4h lernen“ (täglich, Kürzel `📚`) anlegen; zu breites Kürzel → Fehler; Month öffnen → aktueller Monat, Cursor auf heute; Zellen abhaken und gestern nachtragen → Streak in Goals/Today stimmt; Vormonat/Folgemonat blättern; App neu starten → alles da; alte v2-DB öffnen → Ziele erhalten, Kürzel leer, Ersatzkürzel sichtbar.
