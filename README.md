# ESPipes

<details>
<summary>
Inhaltsverzeichnis

---

</summary>

[[_TOC_]]

</details>

## Lernziele

Das Arbeiten mit dynamischem Speicher, File I/O mit Binärdateien
sowie die Verwendung von Bitwise-Operationen und Bit-Masken sollen anhand der
Implementation eines *Pipe*-Minigames geübt werden.

## Beschreibung

*ESPipes* ist eine Variation der *Pipe*-Minigames. Das Ziel des Spiels ist es,
durch das Drehen von Rohren eine Verbindung zwischen dem Start- und Zielrohr
herzustellen. Die Anzahl der benötigten Züge ist hierbei das Punkteergebnis,
wobei weniger Punkte (also weniger Züge) ein besseres Ergebnis bedeuten.
Jede Konfigurationsdatei enthält hierzu auch eine Highscore-Liste, die
entsprechend aktualisiert werden soll.

## Dateiformat

Die Konfigurationsdatei verwendet ein Binärformat. Es kann davon ausgegangen
werden, dass die Datei korrekt formatiert ist, wenn sie mit der korrekten
*Magic-Number* beginnt.
Das Format, also Reihenfolge und Länge der Felder, wird durch folgende
Tabellen spezifiziert (alle Felder mit numerischen Werten sind `unsigned`):

### Gesamt

| Länge  | Inhalt                                                                       |
| ------ | ---------------------------------------------------------------------------- |
| 7 Byte | *Magic-Number* (Muss der ASCII-Text "ESPipes", ohne String-Terminator, sein) |
| 1 Byte | Breite des Spielfelds                                                        |
| 1 Byte | Höhe des Spielfelds                                                          |
| 1 Byte | Zeile des Startrohrs                                                         |
| 1 Byte | Spalte des Startrohrs                                                        |
| 1 Byte | Zeile des Zielrohrs                                                          |
| 1 Byte | Spalte des Zielrohrs                                                         |
| 1 Byte | Anzahl der Einträge in der Highscore-Liste                                   |
| *N/A*  | Highscore-Liste                                                              |
| *N/A*  | Spielfeld                                                                    |

> **Hinweis**: Die Koordinaten in der Konfigurationsdatei verwenden den
> Basis-Index `0`, die Koordinaten in der Eingabe/Ausgabe den Basis-Index `1`.

### Eintrag in Highscore-Liste

| Länge  | Inhalt                                                   |
| ------ | -------------------------------------------------------- |
| 1 Byte | Punktezahl                                               |
| 3 Byte | Name (in ASCII-Zeichenkodierung, ohne String-Terminator) |

### Eintrag im Spielfeld

| Länge  | Inhalt    |
| ------ | --------- |
| 1 Byte | Rohr-Feld |

### Rohr-Feld

Jedes Feld besteht aus einem Byte (programmintern durch ein `uint8_t`
repräsentiert, andere programminterne Repräsentationsformen wie *structs*
sind explizit **nicht** erlaubt). Das Spielfeld wird programmintern durch ein
2-dimensionales Array dieser Felder dargestellt, wobei
die 1. Dimension die Reihen, und die 2. Dimension die Spalten sind.

Ein Feld enthält folgende Informationen:
- Richtungen, in welche das Rohr Öffnungen hat
- Richtungen, in welches das Rohr zusätzlich mit umgebenden Rohren verbunden ist

Diese Informationen sind wie folgt kodiert:

```lang-none
Feld (in binär):  10 00 00 11
Richtungen:       ^^          TOP
                     ^^       LEFT
                        ^^    BOTTOM
                           ^^ RIGHT

Feld (in binär):  10 00 00 11
Werte:            ^  ^  ^  ^  Feld ist in entsprechende Richtung offen
                   ^  ^  ^  ^ Feld hat in entsprechende Richtung Verbindung zu anderem Rohr
```

> **Tipp**: Man benötigt nur 6 "Basis"-Bit-Masken, alle anderen benötigten
> Bit-Masken lassen sich aus diesen durch Bitwise-Operationen generieren.

Hier sind ein paar Beispiele zum besseren Verständnis der Kodierung
(Visualisierung enthält auch die 4 umgebenden Rohre):

```lang-none
Feld (in binär):  10 00 00 11
Beschreibung:     Ein "L"-förmiges Rohr, das oben und rechts offen ist, wobei
                  es rechts auch mit einem anderen Rohr verbunden ist.
Visualisierung:
   ╝ 
  ╗╚═
   ╬ 
```

```lang-none
Feld (in binär):  10 11 11 00
Beschreibung:     Ein "T"-förmiges Rohr, das oben und links und unten offen ist,
                  wobei es links und unten auch mit einem anderen Rohr verbunden ist.
Visualisierung:
   ╝ 
  ═╣╗
   ╬ 
```

```lang-none
Feld (in binär):  00 10 00 11
Beschreibung:     Ein gerades Rohr, das links und rechts offen ist, wobei
                  es rechts auch mit einem anderen Rohr verbunden ist.
Visualisierung:
   ╝ 
  ╗══
   ╬ 
```

```lang-none
Feld (in binär):  10 10 11 11
Beschreibung:     Ein "+"-förmiges Rohr, das in alle Richtungen offen ist,
                  wobei es unten und rechts auch mit einem anderen Rohr
                  verbunden ist.
Visualisierung:
   ╝ 
  ╗╬═
   ╬ 
```

```lang-none
Feld (in binär):  00 00 00 00
Beschreibung:     Eine Blockade. Sie hat in keine Richtung Öffnungen, und kann
                  daher auch nicht mit anderen Rohren verbunden sein.
Visualisierung:
   ╝ 
  ╗█═
   ╬ 

```

## Datentypen

Folgende Datentypen müssen implementiert werden:

### Direction

Der Datentyp *Direction* gibt eine Richtung an. Er kann folgende Werte haben:

| Wert | Bedeutung |
| ---- | --------- |
| 0    | TOP       |
| 1    | LEFT      |
| 2    | BOTTOM    |
| 3    | RIGHT     |

> **Tipp**: Die Werte sind so gewählt, dass mit ihnen auch gerechnet werden
> kann.

Der Datentyp dient der programminternen Repräsentation einer Richtung. Dies
kann etwa die Rotations-Richtung beim Drehen eines Feldes oder auch
die Ausrichtung zweier benachbarter Felder zueinander (zB. "Feld rechts vom
aktuellen Feld") sein.

### Highscore

Der Datentyp *Highscore* enthält die Highscore-Liste. Er darf beliebig
implementiert werden, soll jedoch ein eigener, auf die Aufgabe spezialisierter,
Datentyp sein.

## Framework

Ein Framework zur (Befehls-)Eingabe, Text-Ausgabe, und zum Pathfinding wird
bereitgestellt. Weitere Informationen zum Framework sind in Form von
Dokumentation in [framework.h](./framework.h) zu finden.

## Programm-Struktur

Es wird Empfohlen, folgende Hilfs-Funktionen zu implementieren:

```c
bool isDirectionOutOfMap(uint8_t width, uint8_t height, uint8_t coord[2], Direction dir);
bool isPipeOpenInDirection(uint8_t** map, uint8_t width, uint8_t height, uint8_t coord[2], Direction dir);
bool shouldPipeConnectInDirection(uint8_t** map, uint8_t width, uint8_t height, uint8_t coord[2], Direction dir);
uint8_t* getAdjacentPipe(uint8_t** map, uint8_t width, uint8_t height, uint8_t coord[2], Direction dir);
```

## Programm-Ablauf

### Spiel-Start

Das Programm wird mit einem Kommandozeilenparameter aufgerufen. Dieser gibt den
Pfad zur Konfigurationsdatei an, die geladen werden soll. Sollte das Programm

- mit mehr oder weniger Parametern aufgerufen werden oder
- die Konfigurationsdatei nicht geöffnet werden können oder
- die Konfigurationsdatei nicht mit der korrekten *Magic-Number* beginnen,

soll die entsprechende Fehlermeldung ausgegeben werden und das Programm mit dem
entsprechenden Rückgabewert beendet werden (siehe 
[Programm-Rückgabewerte und Fehlermeldungen](#programm-rückgabewerte-und-fehlermeldungen)).

Anschließend beginnt die erste Runde.

### Spiel-Ablauf

Zu Beginn jeder Runde wird das Spielfeld ausgegeben:

```lang-none

 │1234567
─┼───────
1│╞║╗╔╠═║
2│╗█╣╔║╗╔
3│═╠╗║╗█╣
4│╗█╣╔║═╔
5│═╠═║╗█╣
6│╚╝╚╠═║╨


```

> **Hinweis**: Die Zeichen `╨`,`╡`,`╥`,`╞` stellen die Start- bzw. Zielrohre
> dar. Sie sind in nur eine Richtung geöffnet.

Anschließend wird die Befehlszeile mit der Rundennummer (beginnend mit `1`)
ausgegeben:

```lang-none
1 > 
```

Danach wird auf eine Benutzereingabe gewartet.

Wird nichts oder nur *Whitespace* eingegeben, soll die Befehlszeile erneut
ausgegeben werden sowie auf eine neue Benutzereingabe gewartet werden.

Wird ein unbekannter Befehl oder ein bekannter Befehl mit ungültigen Parametern
eingegeben, soll die entsprechende Fehlermeldung (siehe 
[Programm-Rückgabewerte und Fehlermeldungen](#programm-rückgabewerte-und-fehlermeldungen)
) ausgegeben werden, und die Befehlszeile erneut ausgegeben werden sowie auf
eine neue Benutzereingabe gewartet werden.

Wird durch die Drehung eines Rohr-Feldes eine Verbindung zwischen Start-
und Zielrohr hergestellt, ist das Spiel zu Ende.

#### Befehl: rotate

Dieser Befehl erlaubt der/dem Benutzer*in, ein Rohr-Feld in eine Richtung zu
drehen.

Wird versucht, dass Start- oder Zielrohr zu drehen, soll die entsprechende
Fehlermeldung ausgegeben werden.

Der Befehl `rotate` hat 3 Parameter:
- die Dreh-Richtung (`left` oder `right`)
- die Zeilennummer (positive Ganzzahl, kleiner als die Anzahl an Reihen)
- die Spaltennummer (positive Ganzzahl, kleiner als die Anzahl an Spalten)

#### Befehl: help

Der Befehl `help` gibt folgenden Hilfetext aus, wobei etwaige Parameter
ignoriert werden:

```lang-none
Commands:
 - rotate <DIRECTION> <ROW> <COLUMN>
    <DIRECTION> is either `left` or `right`.

 - help
    Prints this help text.

 - quit
    Terminates the game.

- restart
    Restarts the game.
```

#### Befehl: quit

Der Befehl `quit` (alternativ *EOF*, ausgelöst durch zB. `Ctrl-D`) beendet das
Programm. Parameter werden ignoriert.

#### Befehl: restart

Der Befehl `restart` startet das Spiel neu. Parameter werden ignoriert.

### Spiel-Ende

Zu Spielende wird das (gelöste) Spielfeld, sowie das Punkteergebnis ausgegeben:

```lang-none
 │1234
─┼────
1│╞╗╔═
2│█║╬╚
3│╣╚╗║
4│╗╬╚╡

Puzzle solved!
Score: 4
```

Wurde ein Highscore aus der Highscore-Liste geschlagen, wird der Spieler davon
informiert. Danach wird die/der Benutzer*in nach einem 3 Zeichen langen Namen
gefragt:

```lang-none
Beat Highscore!
Please enter 3-letter name: 
```

Werden mehr oder weniger als 3 Zeichen, oder andere Zeichen als Buchstaben
eingegeben, soll die entsprechende Fehlermeldung ausgegeben werden, sowie
erneut nach einem Namen gefragt werden.

Der eingegebene Name wird zu Großbuchstaben konvertiert, und die
Highscore-Liste aktualisiert, wobei die Anzahl an Einträgen gleich bleiben
soll. Ein Score von `0` bedeutet, dass der Platz in der Highscore-Liste frei
ist. Bei gleichem Highscore soll der neue Score weiter unten gereiht werden.

Anschließend muss die Highscore-Liste auch noch in der Konfigurationsdatei
aktualisiert werden. Hier soll **ausschließlich** die neue Highscore-Liste in
die Datei geschrieben werden, und **nicht** die ganze Datei neu geschrieben
werden.

> **Tipp**: Der *file-mode* `rb+` erlaubt es, Teile einer bestehenden Datei
> zu überschreiben.

Zuletzt wird die Highscore-Liste ausgegeben, wobei bei freien Plätzen (also
Einträgen mit  Score `0`) statt des Namens `---` ausgegeben wird.
Anschließend wird das Programm beendet:

```lang-none

Highscore:
   FOO 4
   BAR 5
   BAZ 7
   QUX 11
   --- 0
```


## Programm-Rückgabewerte und Fehlermeldungen

| Wert | Fehlermeldung                              | Bedeutung                                                      |
| ---- | ------------------------------------------ | -------------------------------------------------------------- |
| 0    |                                            | Erfolgsfall                                                    |
| 1    | `Usage: ./a3 CONFIG_FILE\n`              | Falsche Anzahl von Kommandozeilenparametern                    |
| 2    | `Error: Cannot open file: <CONFIG_FILE>\n` | Konfigurationsdatei kann nicht geöffnet werden                 |
| 3    | `Error: Invalid file: <CONFIG_FILE>\n`     | Konfigurationsdatei beginnt nicht mit korrekter *Magic-Number* |
| 4    | `Error: Out of memory\n`                   | Kein Speicher kann mehr angefordert werden                     |
|      | `Error: Unknown command: <COMMAND>\n`      | Unbekannter Befehl eingegeben                                  |
|      | `Usage: rotate ( left \| right ) ROW COLUMN\n`                | Ungültige Parameter für Befehl `rotate`     |
|      | `Error: Rotating start- or end-pipe is not allowed\n`         | Drehung von Start- oder Zielrohr            |
|      | `Error: Invalid name. Only alphabetic letters allowed\n`      | Name enthält ungültige Zeichen              |
|      | `Error: Invalid name. Name must be exactly 3 letters long\n`  | Name kürzer oder länger als 3 Zeichen       |

> **Hinweis**: Platzhalter sind durch  `<>` markiert, und sollen durch den
> entsprechenden Wert ersetzt werden - auch die spitzen Klammern sind nicht
> Teil der Ausgabe.

## Beispiel-Ausgabe

Eine Beispiel-Ausgabe ist in der Datei [EXAMPLE.txt](./EXAMPLE.txt) zu finden.

## Spezifikation

- Keine zusätzlichen Ausgaben
- Alle Ausgaben erfolgen auf *stdout*
   - Keinerlei Ausgaben auf *stderr*
- Der Dateiinhalt, insbesondere das Spielfeld und die Highscore-Liste, müssen
  am Heap gespeichert werden
   - Das Spielfeld wird als 2-dimensionales Array von `uint8_t` gespeichert
   - Die Highscore-Liste wird mit eigenem Datentypen (siehe
     [Datentypen -> Highscore](#highscore)) gespeichert

## Abgabe

- Die Abgabe erfolgt per *git* auf das Repository, das durch *Progpipe*
  erstellt wurde.
  - Hierzu darf **nicht** das Web-Interface von *GitLab* verwendet werden.
- Dateiname: `a3.c`
- Abgabe bis *spätestens*: 09.01.2021 23:59 (in Österreich gültige Zeit)

## Bewertung

Das Assignment wird
[wie im TeachCenter beschrieben](https://tc.tugraz.at/main/mod/page/view.php?id=55761)
bewertet. Machen Sie sich auch mit dem [Beurteilungsschema](https://tc.tugraz.at/main/mod/page/view.php?id=55602) für die Übungen vertraut, insbesondere mit den Regeln zu Plagiaten!

> **Achtung**: Damit die Abgabe als ernsthafter Versuch gewertet wird, muss das
> abgegebene Programm mindestens **6** Test Cases bestehen!

## Verantwortliche Tutoren

- Florian Hager
- Nives Krizanec
- Clemens Oberhauser
- Lukas Pucher
