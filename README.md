# MP3 Normalizer

Ein Multithreaded-Tool zur Normalisierung der Lautstärke von `.mp3`-Dateien unter Windows. Das Programm nutzt `ffmpeg`, um alle Audiodateien in einem Verzeichnis auf eine einheitliche Lautstärke zu bringen (Standard: -14 LUFS).

## 🛠 Funktionen

- Multithreaded Verarbeitung für hohe Geschwindigkeit
- Visuelle Konsolenanzeige mit Fortschrittsüberwachung
- Lautstärke-Normalisierung via `ffmpeg`
- Konfigurierbar über `config.ini`
- Fehlerprotokollierung in `failed_jobs.txt`

---

## ⚙️ Verwendung

```batch
normalize.exe <Input-Ordner> <Output-Ordner> [Threads] [--force] [--target=-14]
```

### Parameter:

| Argument        | Bedeutung |
|----------------|-----------|
| `<Input-Ordner>`  | Pfad zum Ordner mit den MP3-Dateien |
| `<Output-Ordner>` | Zielordner für normalisierte Dateien |
| `[Threads]`       | Anzahl gleichzeitiger Threads (optional) |
| `--force`         | erzwingt erneute Verarbeitung existierender Dateien |
| `--target=-14`    | Ziel-Lautstärke in LUFS (Standard: -14) |

---

## 📁 Beispiel

```batch
normalize.exe "C:\Musik\Originale" "C:\Musik\Normalisiert" 4 --target=-16 --force
```

---

## 🧩 Voraussetzungen

- Windows-Betriebssystem
- [`ffmpeg`](https://ffmpeg.org/download.html) installiert
- Beispiel für `config.ini`:

```ini
# Konfigurationsdatei für MP3 Normalizer
ffmpeg_path=C:\Tools\ffmpeg\ffmpeg.exe
```

---

## 📦 Kompilierung (mit MinGW)

```batch
g++ -std=c++17 -municode -o normalize.exe main.cpp
```

---

## 📄 Lizenz

Dieses Projekt ist **frei verwendbar** (Free License).  
Keine Garantie oder Haftung bei Nutzung.

---

## 👤 Autor

Matthias Stoltze, 2025  
