# ortha – Releases und Installation per Scoop – Design-Spec

## Kontext

Das Repo ist öffentlich. Ein Kollege mit Windows soll ortha nutzen können, ohne selbst zu bauen, und Updates bekommen, wenn neue Features dazukommen. Dafür braucht es versionierte Releases mit fertigen Programmdateien und einen Installationsweg mit Updates.

### Anforderungen (vom Nutzer festgelegt)
- Fertige Programmdateien für Windows (und Linux) über **GitHub Releases**
- Installation und Updates unter Windows per **Scoop**; der Bucket liegt im ortha-Repo selbst
- Das Scoop-Manifest wird bei jedem Release **automatisch durch die CI** aktualisiert (Commit auf `main`)
- **Kein** Update-Hinweis in ortha selbst; ortha bleibt offline
- Vorerst **keine Lizenz** („Proprietary“ im Manifest)

### Nicht enthalten
Update-Prüfung in der App, Linux-Paketmanager (Homebrew, Copr), Code-Signing der exe, Installer (MSI), macOS.

## Verhalten

### Version
- Einzige Quelle: `project(VERSION x.y.z)` in `CMakeLists.txt`. CMake erzeugt daraus beim Konfigurieren eine Konstante (`ortha::kVersion`).
- `ortha --version` gibt `ortha x.y.z` aus und beendet sich (Exit-Code 0). Ohne Argumente startet ortha wie bisher. Unbekannte Argumente: `ortha: unknown option '<arg>'` auf stderr, Exit-Code 2.
- Erste Release-Version: **0.2.0**.

### Release-Ablauf (für den Nutzer)
```
# Version in CMakeLists.txt erhöhen, committen, dann:
git tag -a v0.2.0 -m "ortha 0.2.0"
git push --follow-tags
```

### Release-Workflow (`.github/workflows/release.yml`, Auslöser: Push eines Tags `v*`)
1. **check-version:** Tag ohne `v` muss der Version in `CMakeLists.txt` entsprechen, sonst Abbruch mit `Tag v<tag> does not match the version <x.y.z> in CMakeLists.txt`.
2. **build** (Matrix wie die bestehende CI: `windows-latest` mit MSVC, `ubuntu-24.04` mit GCC 14), jeweils `Release`, bauen und alle Tests ausführen, dann packen:
   - Windows: `ortha-<version>-windows-x64.zip` mit `ortha.exe`
   - Linux: `ortha-<version>-linux-x64.tar.gz` mit `ortha`
3. **release:** GitHub-Release `v<version>` mit beiden Archiven und `SHA256SUMS.txt`; Release-Notizen automatisch aus den Commits seit dem letzten Tag.
4. **scoop:** SHA-256 des Windows-Archivs berechnen, `bucket/ortha.json` aktualisieren (`version`, `url`, `hash`), Commit `chore(scoop): ortha <version>` auf `main` pushen (mit `GITHUB_TOKEN`, Recht `contents: write`; solche Pushes lösen keine weiteren Workflows aus).

Der bestehende Workflow `ci.yml` (Build + Test bei jedem Push) bleibt unverändert.

### Scoop-Manifest (`bucket/ortha.json`)
```json
{
  "version": "0.2.0",
  "description": "A minimal terminal to-do and habit tracker",
  "homepage": "https://github.com/floerychristopher/ortha",
  "license": "Proprietary",
  "url": "https://github.com/floerychristopher/ortha/releases/download/v0.2.0/ortha-0.2.0-windows-x64.zip",
  "hash": "<sha256>",
  "bin": "ortha.exe",
  "checkver": "github",
  "autoupdate": {
    "url": "https://github.com/floerychristopher/ortha/releases/download/v$version/ortha-$version-windows-x64.zip"
  }
}
```
Bis zum ersten Release enthält es Platzhalter für `hash`; die CI überschreibt `version`, `url` und `hash`.

### README
- **Install on Windows:** Scoop installieren (Verweis auf https://scoop.sh), `scoop install git`, `scoop bucket add ortha https://github.com/floerychristopher/ortha`, `scoop install ortha`, Start im Windows Terminal empfohlen; Updates `scoop update ortha`, `scoop status` zeigt neue Versionen; Daten unter `%APPDATA%\ortha\ortha.db` bleiben bei Updates erhalten.
- **Releasing:** die drei Befehle oben.
- `ortha --version` in der Usage.

## Architektur

```
CMakeLists.txt                     # VERSION 0.2.0
src/core/version.hpp.in            # neu: kVersion aus PROJECT_VERSION (configure_file)
src/tui/main.cpp                   # --version / unbekannte Argumente
scripts/update-scoop-manifest.py   # neu: setzt version/url/hash in bucket/ortha.json
scripts/check-tag-version.sh       # neu: Tag gegen CMakeLists-Version prüfen
bucket/ortha.json                  # neu
.github/workflows/release.yml      # neu
tests/CMakeLists.txt               # + CTest: ortha --version
README.md
```
Die Shell-/Python-Schritte liegen in eigenen Skripten statt inline im Workflow, damit sie lokal testbar sind.

## Verifikation

- **Lokal:** CTest `ortha --version` (Ausgabe `ortha 0.2.0`, Exit 0) und unbekanntes Argument (Exit 2); `check-tag-version.sh` mit passendem und falschem Tag; `update-scoop-manifest.py` auf einer Kopie des Manifests (Felder gesetzt, gültiges JSON, andere Felder unverändert); Workflow-Datei als gültiges YAML; Manifest als gültiges JSON mit Pflichtfeldern.
- **Echt:** Release `v0.2.0` durch den Nutzer (Tag pushen). Danach per GitHub-API prüfen: Release vorhanden mit beiden Archiven und `SHA256SUMS.txt`, Manifest-Commit auf `main` mit passendem Hash. Die Installation per Scoop unter Windows macht der Kollege bzw. der Nutzer auf einem Windows-Rechner.
