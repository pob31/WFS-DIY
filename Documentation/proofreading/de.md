# Proofreading checklist — German (Deutsch)

Locale: `de`  |  Total keys: 687  |  Source: `Resources/lang/en.json` vs `Resources/lang/de.json`

## How to use this file

This checklist covers the **translated prose only** — help text, tooltips, status / dialog messages, and dialog / help-card titles. UI control labels, buttons, section and tab names, and technical / domain terms are **intentionally English in every language** (the app overlays each locale on an English base, so any key not translated here simply shows English). They are not listed and need no review.

Walk through each section. For every entry:
- If the translation reads naturally and matches the meaning of the English source, leave the `[ ]` checkbox blank or mark `[x] OK`.
- If the translation is wrong, awkward, or has a typo, write the corrected text under `Fix:`.
- **Control and parameter names appear in English inside the translated text on purpose** (e.g. fr `Nombre de Reverb Channels.`, ja `Reverb Channels数。`), so the help matches the English labels on screen. Do **not** translate these back — leave them in English.
- If a whole entry is **identical to English**, that may be intentional: loanwords (Sampler, Tracking, Pre, Post, ON/OFF, LFO, HF, EQ, AI), proper nouns (QLab, Lightpad, Stream Deck, ADM-OSC, MQTT), technical terms (OSC Path:, Localhost). Mark `[x] OK` to confirm intent, or write `Fix:` if it should be translated.
- `(missing — falls back to English)` means the key is untranslated and currently shows English in the UI; write the correct translation under `Fix:`.
- Curly braces `{name}`, `{num}`, `{path}` etc. are runtime placeholders; keep them in the translation.
- `\n` in the value is a literal newline in the rendered UI; preserve it.

---

## `ai.tooltips`

- **`aiToggle`**
  - EN: Master switch for the whole MCP integration. When OFF every AI tool call is refused; when ON, normal tier handling applies (the critical-actions toggle still controls destructive calls separately).
  - DE: Hauptschalter für die gesamte MCP-Integration. Im AUS-Zustand wird jeder KI-Tool-Aufruf abgelehnt; im EIN-Zustand gilt die normale Tier-Behandlung (der Schalter für kritische Aktionen steuert destruktive Aufrufe weiterhin separat).
  - [ ] OK    Fix: 

- **`gate`**
  - EN: Allow destructive AI actions (channel-count changes, port reconfigure, runDSP, etc.) AND skip the per-call confirm handshake for less-destructive tier-2 actions while open. Acts as a superset of the Tier 2 auto-confirm toggle. The red fill drains over 5 minutes, then auto-blocks again.
  - DE: Destruktive KI-Aktionen erlauben (Änderung der Kanalanzahl, Port-Neukonfiguration, runDSP usw.) UND den Bestätigungs-Handshake pro Aufruf für weniger destruktive Tier-2-Aktionen überspringen, solange offen. Übergeordnet zum Tier-2-Auto-Confirm-Schalter. Die rote Füllung läuft über 5 Minuten ab, dann wird automatisch wieder gesperrt.
  - [ ] OK    Fix: 

- **`openHistory`**
  - EN: Open the AI History window: scrollable timeline of every recent AI change with per-row undo / redo and a step-by-step cursor.
  - DE: Fenster KI-Verlauf öffnen: scrollbare Chronik aller jüngsten KI-Änderungen mit Rückgängig/Wiederherstellen pro Zeile und Schritt-für-Schritt-Cursor.
  - [ ] OK    Fix: 

- **`urlButton`**
  - EN: Click to copy the MCP server URL. Useful for Claude Code (claude mcp add wfs-diy <URL> -t http) or any MCP client that takes a URL. Claude Desktop instead uses the JSON config snippet — open the (?) help card.
  - DE: Klicken, um die MCP-Server-URL zu kopieren. Nützlich für Claude Code (claude mcp add wfs-diy <URL> -t http) oder jeden MCP-Client, der eine URL akzeptiert. Claude Desktop nutzt stattdessen den JSON-Konfigurationsausschnitt — öffnen Sie die (?)-Hilfekarte.
  - [ ] OK    Fix: 

## `arrayHelper.status`

- **`applied`**
  - EN: Applied {count} speakers to Array {array}. Ready for next array.
  - DE: {count} Lautsprecher auf Array {array} angewendet. Bereit für nächstes Array.
  - [ ] OK    Fix: 

- **`calculated`**
  - EN: Calculated {count} positions
  - DE: {count} Positionen berechnet
  - [ ] OK    Fix: 

- **`ready`**
  - EN: Ready
  - DE: Bereit
  - [ ] OK    Fix: 

## `audioPatch.dialogs`

- **`unpatchInputsMessage`**
  - EN: Are you sure you want to remove all input patches?
  - DE: Möchten Sie wirklich alle Eingangs-Patches entfernen?
  - [ ] OK    Fix: 

- **`unpatchInputsTitle`**
  - EN: Unpatch All Inputs
  - DE: Alle Eingänge trennen
  - [ ] OK    Fix: 

- **`unpatchOutputsMessage`**
  - EN: Are you sure you want to remove all output patches?
  - DE: Möchten Sie wirklich alle Ausgangs-Patches entfernen?
  - [ ] OK    Fix: 

- **`unpatchOutputsTitle`**
  - EN: Unpatch All Outputs
  - DE: Alle Ausgänge trennen
  - [ ] OK    Fix: 

## `audioPatch.messages`

- **`chooseTestSignal`**
  - EN: Choose a Test Signal to Enable Testing
  - DE: Wählen Sie ein Testsignal zum Aktivieren des Testmodus
  - [ ] OK    Fix: 

## `clusters.help`

- **`attenuationSlider`**
  - EN: Adjust attenuation of all cluster inputs (dB).
  - DE: Dämpfung aller Cluster-Eingänge anpassen (dB).
  - [ ] OK    Fix: 

- **`exportPresets`**
  - EN: Export all 16 LFO presets to an XML file.
  - DE: Alle 16 LFO-Presets in eine XML-Datei exportieren.
  - [ ] OK    Fix: 

- **`importPresets`**
  - EN: Import LFO presets from an XML file.
  - DE: LFO-Presets aus einer XML-Datei importieren.
  - [ ] OK    Fix: 

- **`inputsVisibility`**
  - EN: Show or hide this cluster's inputs on the Map tab. Hide propagates to new members; removing an input restores its visibility.
  - DE: Eingänge dieses Clusters auf der Karte ein- oder ausblenden. Ausblenden gilt auch für neue Mitglieder; Entfernen eines Eingangs stellt seine Sichtbarkeit wieder her.
  - [ ] OK    Fix: 

- **`lfoActiveButton`**
  - EN: Enable or Disable the Periodic Movement of the Cluster (LFO).
  - DE: Periodische Bewegung des Clusters (LFO) aktivieren oder deaktivieren.
  - [ ] OK    Fix: 

- **`lfoAmplitudeRotSlider`**
  - EN: Maximum Rotation Angle (-360 to 360 Degrees).
  - DE: Maximaler Rotationswinkel (-360 bis 360 Grad).
  - [ ] OK    Fix: 

- **`lfoAmplitudeScaleSlider`**
  - EN: Maximum Scale Factor (0.1x to 10x, Logarithmic).
  - DE: Maximaler Skalierungsfaktor (0,1× bis 10×, logarithmisch).
  - [ ] OK    Fix: 

- **`lfoAmplitudeXSlider`**
  - EN: Width of Movement in Relation to Cluster Reference Position.
  - DE: Breite der Bewegung im Verhältnis zur Cluster-Referenzposition.
  - [ ] OK    Fix: 

- **`lfoAmplitudeYSlider`**
  - EN: Depth of Movement in Relation to Cluster Reference Position.
  - DE: Tiefe der Bewegung im Verhältnis zur Cluster-Referenzposition.
  - [ ] OK    Fix: 

- **`lfoAmplitudeZSlider`**
  - EN: Height of Movement in Relation to Cluster Reference Position.
  - DE: Höhe der Bewegung im Verhältnis zur Cluster-Referenzposition.
  - [ ] OK    Fix: 

- **`lfoPeriodDial`**
  - EN: Base Period of the Cluster Movement.
  - DE: Grundperiode der Cluster-Bewegung.
  - [ ] OK    Fix: 

- **`lfoPhaseDial`**
  - EN: Global Phase Offset of the Cluster Movement.
  - DE: Globaler Phasenversatz der Cluster-Bewegung.
  - [ ] OK    Fix: 

- **`lfoPhaseRotDial`**
  - EN: Phase Offset of the Cluster Rotation.
  - DE: Phasenversatz der Cluster-Rotation.
  - [ ] OK    Fix: 

- **`lfoPhaseScaleDial`**
  - EN: Phase Offset of the Cluster Scaling.
  - DE: Phasenversatz der Cluster-Skalierung.
  - [ ] OK    Fix: 

- **`lfoPhaseXDial`**
  - EN: Phase Offset of the Cluster Movement in Width.
  - DE: Phasenversatz der Cluster-Bewegung in der Breite.
  - [ ] OK    Fix: 

- **`lfoPhaseYDial`**
  - EN: Phase Offset of the Cluster Movement in Depth.
  - DE: Phasenversatz der Cluster-Bewegung in der Tiefe.
  - [ ] OK    Fix: 

- **`lfoPhaseZDial`**
  - EN: Phase Offset of the Cluster Movement in Height.
  - DE: Phasenversatz der Cluster-Bewegung in der Höhe.
  - [ ] OK    Fix: 

- **`lfoRateRotSlider`**
  - EN: Faster or Slower Rotation in Relation to Base Period.
  - DE: Schnellere oder langsamere Rotation im Verhältnis zur Grundperiode.
  - [ ] OK    Fix: 

- **`lfoRateScaleSlider`**
  - EN: Faster or Slower Scaling in Relation to Base Period.
  - DE: Schnellere oder langsamere Skalierung im Verhältnis zur Grundperiode.
  - [ ] OK    Fix: 

- **`lfoRateXSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Width.
  - DE: Schnellere oder langsamere Bewegung im Verhältnis zur Grundperiode in der Breite.
  - [ ] OK    Fix: 

- **`lfoRateYSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Depth.
  - DE: Schnellere oder langsamere Bewegung im Verhältnis zur Grundperiode in der Tiefe.
  - [ ] OK    Fix: 

- **`lfoRateZSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Height.
  - DE: Schnellere oder langsamere Bewegung im Verhältnis zur Grundperiode in der Höhe.
  - [ ] OK    Fix: 

- **`lfoShapeRotSelector`**
  - EN: Rotation Behaviour of the Cluster.
  - DE: Rotationsverhalten des Clusters.
  - [ ] OK    Fix: 

- **`lfoShapeScaleSelector`**
  - EN: Scale Behaviour of the Cluster.
  - DE: Skalierungsverhalten des Clusters.
  - [ ] OK    Fix: 

- **`lfoShapeXSelector`**
  - EN: Movement Behaviour of the Cluster in Width.
  - DE: Bewegungsverhalten des Clusters in der Breite.
  - [ ] OK    Fix: 

- **`lfoShapeYSelector`**
  - EN: Movement Behaviour of the Cluster in Depth.
  - DE: Bewegungsverhalten des Clusters in der Tiefe.
  - [ ] OK    Fix: 

- **`lfoShapeZSelector`**
  - EN: Movement Behaviour of the Cluster in Height.
  - DE: Bewegungsverhalten des Clusters in der Höhe.
  - [ ] OK    Fix: 

- **`planeSelector`**
  - EN: Select the plane for rotation and scale operations (XY, XZ, YZ).
  - DE: Ebene für Drehungs- und Skalierungsoperationen auswählen (XY, XZ, YZ).
  - [ ] OK    Fix: 

- **`positionJoystick`**
  - EN: Move all cluster inputs in X/Y. Hold and drag to translate.
  - DE: Alle Cluster-Eingänge in X/Y bewegen. Halten und ziehen zum Verschieben.
  - [ ] OK    Fix: 

- **`presetTile`**
  - EN: Click: recall preset. Double-click: recall + start. Middle/right-click: store current LFO.
  - DE: Klick: Preset abrufen. Doppelklick: abrufen + starten. Mittel-/Rechtsklick: aktuellen LFO speichern.
  - [ ] OK    Fix: 

- **`qlabPreset`**
  - EN: Create a QLab network cue to recall the last selected LFO preset for the current cluster.
  - DE: Eine QLab-Netzwerk-Cue erstellen, um das zuletzt gewählte LFO-Preset für den aktuellen Cluster abzurufen.
  - [ ] OK    Fix: 

- **`referenceMode`**
  - EN: Select the reference point for cluster transforms: First Input, Barycenter, or Shared Position (all members coincide; scale and rotation apply to per-input offsets).
  - DE: Wählen Sie den Referenzpunkt für Cluster-Transformationen: First Input, Schwerpunkt oder Shared Position (alle Mitglieder fallen zusammen; Skalierung und Rotation wirken auf die Einzeloffsets).
  - [ ] OK    Fix: 

- **`rotationDial`**
  - EN: Rotate all cluster inputs around the reference point in the selected plane.
  - DE: Alle Cluster-Eingänge in der gewählten Ebene um den Referenzpunkt drehen.
  - [ ] OK    Fix: 

- **`scaleJoystick`**
  - EN: Scale cluster inputs relative to the reference point in the selected plane.
  - DE: Cluster-Eingänge relativ zum Referenzpunkt in der gewählten Ebene skalieren.
  - [ ] OK    Fix: 

- **`stopAllLFO`**
  - EN: Stop LFO on all 10 clusters.
  - DE: LFO auf allen 10 Clustern stoppen.
  - [ ] OK    Fix: 

- **`zSlider`**
  - EN: Move all cluster inputs along the Z axis (height).
  - DE: Alle Cluster-Eingänge entlang der Z-Achse (Höhe) bewegen.
  - [ ] OK    Fix: 

## `clusters.status`

- **`noInputs`**
  - EN: No inputs assigned
  - DE: Keine Eingänge zugewiesen
  - [ ] OK    Fix: 

- **`trackedMarker`**
  - EN: [T]
  - DE: [T]
  - [ ] OK    Fix: 

- **`tracking`**
  - EN: Tracking: Input {num} (overrides reference)
  - DE: Tracking: Eingang {num} (überschreibt Referenz)
  - [ ] OK    Fix: 

## `common`

- **`add`**
  - EN: Add
  - DE: Hinzufügen
  - [ ] OK    Fix: 

- **`all`**
  - EN: All
  - DE: Alle
  - [ ] OK    Fix: 

- **`apply`**
  - EN: Apply
  - DE: Anwenden
  - [ ] OK    Fix: 

- **`cancel`**
  - EN: Cancel
  - DE: Abbrechen
  - [ ] OK    Fix: 

- **`close`**
  - EN: Close
  - DE: Schließen
  - [ ] OK    Fix: 

- **`delete`**
  - EN: Delete
  - DE: Löschen
  - [ ] OK    Fix: 

- **`disable`**
  - EN: Disable
  - DE: Deaktivieren
  - [ ] OK    Fix: 

- **`edit`**
  - EN: Edit
  - DE: Bearbeiten
  - [ ] OK    Fix: 

- **`enable`**
  - EN: Enable
  - DE: Aktivieren
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - DE: Exportieren
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - DE: Importieren
  - [ ] OK    Fix: 

- **`load`**
  - EN: Load
  - DE: Laden
  - [ ] OK    Fix: 

- **`no`**
  - EN: No
  - DE: Nein
  - [ ] OK    Fix: 

- **`none`**
  - EN: None
  - DE: Keine
  - [ ] OK    Fix: 

- **`off`**
  - EN: OFF
  - DE: AUS
  - [ ] OK    Fix: 

- **`ok`**
  - EN: OK
  - DE: OK
  - [ ] OK    Fix: 

- **`on`**
  - EN: ON
  - DE: EIN
  - [ ] OK    Fix: 

- **`reload`**
  - EN: Reload
  - DE: Neu laden
  - [ ] OK    Fix: 

- **`remove`**
  - EN: Remove
  - DE: Entfernen
  - [ ] OK    Fix: 

- **`reset`**
  - EN: Reset
  - DE: Zurücksetzen
  - [ ] OK    Fix: 

- **`save`**
  - EN: Save
  - DE: Speichern
  - [ ] OK    Fix: 

- **`select`**
  - EN: Select
  - DE: Auswählen
  - [ ] OK    Fix: 

- **`store`**
  - EN: Store
  - DE: Speichern
  - [ ] OK    Fix: 

- **`yes`**
  - EN: Yes
  - DE: Ja
  - [ ] OK    Fix: 

## `eq.status`

- **`off`**
  - EN: EQ OFF
  - DE: EQ AUS
  - [ ] OK    Fix: 

- **`on`**
  - EN: EQ ON
  - DE: EQ EIN
  - [ ] OK    Fix: 

## `fileManager.dialogs`

- **`selectProjectFolder`**
  - EN: Select Project Folder
  - DE: Projektordner auswählen
  - [ ] OK    Fix: 

## `help.admOsc`

- **`body`**
  - EN: ADM-OSC is a protocol aiming to improve interoperability for spatial sound. It sends Cartesian positions (X, Y, Z) or polar values (AED for Azimuth, Elevation, Distance) from the console or from a DAW's automation curves.\nData is sent normalised:\n- between -1.0 and 1.0 for X, Y and Z;\n- between 0.0 to 1.0 for distance,\n- between -180° to 180° for Azimuth\n- between -90° to 90° for elevation.\nThe origin point can be moved and the mapping can also be adjusted in different segments for the inner and outer parts of the stage.\nWhen dragging the handles on the graphs, holding the shift key will apply symmetrical adjustments on the opposite side.
  - DE: ADM-OSC-Zuordnungen\n\nADM-OSC ist ein Protokoll zur Verbesserung der Interoperabilität für räumlichen Klang. Es sendet kartesische Positionen (X, Y, Z) oder polare Werte (AED für Azimut, Elevation, Distanz) von der Konsole oder den Automationskurven einer DAW.\nDie Daten werden normalisiert gesendet:\n- zwischen -1,0 und 1,0 für X, Y und Z;\n- zwischen 0,0 und 1,0 für Distanz,\n- zwischen -180° und 180° für Azimut\n- zwischen -90° und 90° für Elevation.\nDer Ursprungspunkt kann verschoben und das Mapping in verschiedenen Segmenten für den inneren und äußeren Bühnenbereich angepasst werden.\nBeim Ziehen der Griffe in den Diagrammen werden mit gedrückter Umschalttaste symmetrische Anpassungen auf der gegenüberliegenden Seite vorgenommen.
  - [ ] OK    Fix: 

- **`title`**
  - EN: ADM-OSC Mappings
  - DE: ADM-OSC Mappings
  - [ ] OK    Fix: 

## `help.automOtion`

- **`body`**
  - EN: One time movement can also be programmed and triggered manually or triggered by sound level.\nThe coordinates are either relative from the start position or absolute relative to the origin point.\nThe input can either stay at the end position or revert to the starting position.\nInput position cannot be changed while the input is moving this way, but interacting with the input will change the position offset of the input instead.\nFor audio level triggering select the sound level above which the movement will start. When the sound drops below the reset level the movement will be rearmed, ready to be triggered again. New movement automation will not be possible while the input is moving.
  - DE: Einmalige Bewegungen können programmiert und manuell oder durch Schallpegel ausgelöst werden.\nDie Koordinaten sind entweder relativ zur Startposition oder absolut zum Ursprungspunkt.\nDer Eingang kann an der Endposition bleiben oder zur Startposition zurückkehren.\nDie Position kann während der Bewegung nicht geändert werden, aber Interaktion ändert den Positionsversatz.\nFür Pegelauslösung wählen Sie den Schwellwert. Wenn der Pegel unter den Rücksetzpegel fällt, wird die Bewegung wieder bereitgestellt.
  - [ ] OK    Fix: 

- **`title`**
  - EN: AutomOtion
  - DE: AutomOtion
  - [ ] OK    Fix: 

## `help.binaural`

- **`body`**
  - EN: The Binaural renderer is used for:\n- listening to a rough spatial mix on headphones,\n- creating a mix for stereo output,\n- listening to a single soloed track through the spatial processing.\nThis may take the place of your master mix if it's only feeding headphones and media mix.\nThe position of the listening position may be adjusted in depth from the origin point and in orientation. Delay and level settings allow you to eventually match the sound at the FOH position.
  - DE: Binaural-Renderer\n\nDer Binaural Renderer wird verwendet für:\n- das Abhören eines groben räumlichen Mixes über Kopfhörer,\n- das Erstellen eines Mixes für Stereoausgabe,\n- das Abhören einer einzelnen Solo-Spur durch die räumliche Verarbeitung.\nEr kann Ihren Master-Mix ersetzen, wenn dieser nur Kopfhörer und Medienmix versorgt.\nDie Hörposition kann in der Tiefe vom Ursprungspunkt und in der Orientierung angepasst werden. Delay- und Pegeleinstellungen ermöglichen die Anpassung an die FOH-Position.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Binaural Renderer
  - DE: Binaural Renderer
  - [ ] OK    Fix: 

## `help.clusters`

- **`body`**
  - EN: Clusters are groups of inputs that can be manipulated and animated as a whole.\nEach input can only be part of one cluster.\nEach cluster can only have one input with tracking fully enabled. Then this input will become the reference points for the cluster.\nIf no input with tracking is part of the cluster then there are two modes for the reference point of the cluster. Either the first input assigned in the list becomes the reference or the barycentre, in other words the center of gravity or the middle of the shape formed by the assigned inputs.\nAll inputs of the clusters can be moved by dragging the reference point. The individual inputs (other than a first input that would be a reference point) can still be adjusted individually. Dragging an input with tracking activated that is also a reference point for a cluster will affect its position offset and the position of the other inputs of the cluster normally.\nAll inputs in a cluster can be rotated or scaled around the reference point.\nAll clusters can be assigned an animation via an LFO. The positions X, Y and Z, the rotation and scale of the cluster can be controlled. The LFO has a period and a phase setting. Each individual parameter has shape, amplitude, rate and phase. The LFO settings can be assigned to pads for a quick recall. A right click will store the LFO parameters to a pad. Double clicking the top of the pad will allow to edit the name of the preset. Clicking or tapping a pad will recall the settings whether the LFO is running or not, but it will not start it if is isn't. A double click/tap will load and start the LFO.\nAll input clusters share the same set of LFO presets.
  - DE: Cluster sind Gruppen von Eingängen, die als Ganzes manipuliert und animiert werden können.\nJeder Eingang kann nur Teil eines Clusters sein.\nJeder Cluster kann nur einen Eingang mit vollständig aktiviertem Tracking haben. Dieser wird zum Referenzpunkt des Clusters.\nWenn kein Eingang mit Tracking vorhanden ist, gibt es zwei Modi: entweder der erste zugewiesene Eingang oder der Schwerpunkt der zugewiesenen Eingänge.\nAlle Eingänge können durch Ziehen des Referenzpunkts verschoben werden. Einzelne Eingänge können weiterhin individuell angepasst werden. Das Ziehen eines Eingangs mit aktiviertem Tracking, der auch Referenzpunkt ist, beeinflusst seinen Positionsversatz und die Position der anderen Eingänge des Clusters normal.\nAlle Eingänge eines Clusters können um den Referenzpunkt rotiert oder skaliert werden.\nAllen Clustern kann eine Animation über einen LFO zugewiesen werden. Positionen X, Y, Z, Rotation und Skalierung können gesteuert werden. LFO-Einstellungen können Pads zugewiesen werden. Ein Rechtsklick speichert die LFO-Parameter in einem Pad. Doppelklick auf den oberen Bereich des Pads ermöglicht die Bearbeitung des Preset-Namens. Klicken oder Tippen auf ein Pad ruft die Einstellungen ab, ob der LFO läuft oder nicht, startet ihn aber nicht. Ein Doppelklick/Doppeltipp lädt und startet den LFO.\nAlle Cluster teilen sich denselben Satz von LFO-Presets.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Clusters
  - DE: Cluster
  - [ ] OK    Fix: 

## `help.diagnostics`

- **`body`**
  - EN: The diagnostic tools are hidden by default: long-press the Diagnostics button below to show or hide them. They appear automatically when the previous session did not shut down cleanly.\nTo send feedback or report a problem, click Report Issue: it opens the project's GitHub issue tracker in your browser. Describe what happened, what you expected and the steps to reproduce it, then attach the exported diagnostic data.\nExport Logs copies the useful data to a WFS-DIY-logs folder at the location you choose: the logs of the current and up to five previous sessions, plus the application settings file. Attach this folder (or a zip of it) to your report.\nThe session logs contain start-up information (application version, operating system, CPU, channel counts), project loading, network activity and errors. No audio is ever recorded.\nOpen Log Folder shows the raw session logs on disk (the WFS-DIY/logs folder in the user application data directory), useful to find a specific session.\nCopy System Info puts a short summary on the clipboard — application version, operating system, CPU and the current audio device with its sample rate and buffer size — ready to paste into an issue.
  - DE: Die Diagnosewerkzeuge sind standardmäßig ausgeblendet: Ein langer Druck auf die Diagnostics-Schaltfläche darunter blendet sie ein oder aus. Sie erscheinen automatisch, wenn die vorherige Sitzung nicht sauber beendet wurde.\nUm Feedback zu senden oder ein Problem zu melden, klicken Sie auf „Problem melden“ (Report Issue): Der GitHub-Issue-Tracker des Projekts öffnet sich im Browser. Beschreiben Sie, was passiert ist, was Sie erwartet haben und die Schritte zur Reproduktion, und hängen Sie die exportierten Diagnosedaten an.\n„Protokolle exportieren“ (Export Logs) kopiert die nützlichen Daten in einen Ordner WFS-DIY-logs am gewählten Ort: die Protokolle der aktuellen und bis zu fünf vorheriger Sitzungen sowie die Einstellungsdatei der Anwendung. Hängen Sie diesen Ordner (oder ein Zip davon) an Ihren Bericht an.\nDie Sitzungsprotokolle enthalten Startinformationen (Programmversion, Betriebssystem, CPU, Kanalzahlen), Projektladevorgänge, Netzwerkaktivität und Fehler. Es wird niemals Audio aufgezeichnet.\n„Protokollordner öffnen“ (Open Log Folder) zeigt die rohen Sitzungsprotokolle auf der Festplatte (Ordner WFS-DIY/logs im Anwendungsdaten-Verzeichnis des Benutzers), nützlich, um eine bestimmte Sitzung zu finden.\n„Systeminfo kopieren“ (Copy System Info) legt eine kurze Zusammenfassung in die Zwischenablage — Programmversion, Betriebssystem, CPU und das aktuelle Audiogerät mit Abtastrate und Puffergröße — bereit zum Einfügen in ein Issue.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Diagnostics & Feedback
  - DE: Diagnose & Feedback
  - [ ] OK    Fix: 

## `help.floorReflections`

- **`body`**
  - EN: Simulating the floor reflections improves the naturalness of the sound. We don't expect sounds to play in a soundproof anechoic chamber. This setting helps recreate the expected floor reflections.\nThe level of the floor reflections can be adjusted as well as the low cut and the high frequency shelf filters. Diffusion adds a bit of chaos to simulate the unevenness of the floor.
  - DE: Bodenreflexionen\n\nDie Simulation von Bodenreflexionen verbessert die Natürlichkeit des Klangs. Wir erwarten nicht, dass Klänge in einem schalltoten Raum wiedergegeben werden. Diese Einstellung hilft, die erwarteten Bodenreflexionen nachzubilden.\nDer Pegel der Bodenreflexionen kann ebenso angepasst werden wie die Tiefensperre und die Hochfrequenz-Shelf-Filter. Diffusion fügt etwas Chaos hinzu, um die Unebenheiten des Bodens zu simulieren.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Floor Reflections
  - DE: Floor Reflections
  - [ ] OK    Fix: 

## `help.gradientMap`

- **`body`**
  - EN: Gradient maps allow to control attenuation, height and high frequency filtering (shelf with a smooth slope centered at 1kHz) depending on the X, Y position. For example, you can fade out a sound when entering a certain zone, you can have high frequency roll-off when moving away from the front of the stage, you can automatically adjust the height of an actor even when they are standing on elevated platforms without having to control height manually.\nThere are three layers for attenuation, height and HF shelf. They can be toggled on and off and they also can be hidden. The focused layer will look dimmed if disabled. Unfocused layers will look dimmed if active and only the shape outlines will be visible if they are deactivated.\nEach layer has a mapping control for white and black to adjust the range of the effect. The curve setting adjusts the transition.\nEach layer can have editable shapes (rectangle, ellipse or polygon) with either a single shade of grey, a linear gradient or a radial gradient. End points of the gradients can be adjusted.\nWhen creating a polygon click for each corner. Double-clicking will create a last corner and close the shape.\nDouble-clicking an existing point on a rectangle or a polygon will remove this corner. Double-clicking on a side will add a new point.\nThe scale and rotation of each shape can be edited for its center or from the origin point.\nWhen enabled the corner points of the rectangles and polygons can also be edited individually.\nShapes and layers can be copied to another layer on the same input or any other input.\nGradient map settings are stored in the input files.
  - DE: Gradientenkarten ermöglichen die Steuerung von Dämpfung, Höhe und Hochfrequenzfilterung (Shelf mit sanfter Flanke bei 1kHz) abhängig von der X, Y Position. Zum Beispiel können Sie einen Klang beim Betreten einer Zone ausblenden, Höhenabfall beim Entfernen von der Bühnenvorderkante anwenden oder die Höhe eines Schauspielers auf erhöhten Plattformen automatisch anpassen.\nDrei Ebenen stehen zur Verfügung: Dämpfung, Höhe und HF-Shelf. Sie können ein-/ausgeschaltet und ausgeblendet werden.\nJede Ebene hat Mapping-Regler für Weiß und Schwarz zur Bereichsanpassung. Die Kurveneinstellung steuert den Übergang.\nJede Ebene kann editierbare Formen haben (Rechteck, Ellipse oder Polygon) mit einheitlichem Grau, linearem oder radialem Gradient.\nBeim Erstellen eines Polygons klicken Sie für jede Ecke. Doppelklick schließt die Form.\nDoppelklick auf einen Punkt entfernt ihn. Doppelklick auf eine Seite fügt einen neuen Punkt hinzu.\nSkalierung und Rotation können vom Zentrum oder Ursprung aus bearbeitet werden.\nFormen und Ebenen können kopiert werden.\nEinstellungen werden in den Eingangsdateien gespeichert.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Gradient Maps
  - DE: Gradient Maps
  - [ ] OK    Fix: 

## `help.inputAdvanced`

- **`body`**
  - EN: - Sidelines and Fringe enable muting when an input comes close to the limits of a rectangular stage (except downstage).\n- Tracking can be enabled and the ID of the tracker can be selected. The amount of smoothing on the position can also be adjusted.\n- Maximum Speed can be engaged and the speed limit adjusted. The system will apply a gradual acceleration and deceleration when the movement starts and finishes. When Path is enabled, the system will track the path followed by the input and will not go in a straight line to the final position. This is especially handy if movements have to be operated manually.\n- Height Factor lets you work in 2D, when set to 0%, or full 3D, when set at 100%, and anything in between. It's the ratio of the height in the level and delay calculations. If you wish to use floor reflections, set it to 100% and use the parallax correction of the output parameters.
  - DE: - Seitenlinien und Randbereich ermöglichen die Stummschaltung, wenn ein Eingang sich den Grenzen einer rechteckigen Bühne nähert (ausser Publikumsseite).\n- Tracking kann aktiviert und die Tracker-ID ausgewählt werden. Die Positionsglättung kann ebenfalls angepasst werden.\n- Die Höchstgeschwindigkeit kann engagiert und die Geschwindigkeitsgrenze angepasst werden. Das System wendet eine graduelle Beschleunigung und Verzögerung am Anfang und Ende der Bewegung an. Wenn der Pfadmodus aktiviert ist, folgt das System dem vom Eingang genommenen Pfad und geht nicht in gerader Linie zur Endposition. Das ist besonders nützlich, wenn Bewegungen manuell gesteuert werden sollen.\n- Der Höhenfaktor erlaubt das Arbeiten in 2D, wenn auf 0 % gesetzt, oder in vollem 3D, wenn auf 100 % gesetzt, und alles dazwischen. Das ist das Verhältnis der Höhe in den Pegel- und Verzögerungsberechnungen. Wenn Sie Floor Reflections verwenden möchten, stellen Sie es auf 100 % und nutzen Sie die Parallaxenkorrektur in den Ausgangsparametern.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Advanced Controls
  - DE: Erweiterte Steuerung
  - [ ] OK    Fix: 

## `help.inputBasic`

- **`body`**
  - EN: Inputs have a wide variety of settings to account for different situations necessitating realistic sound reinforcement or creative tools for sound design.\n- Input level can be adjusted.\n- Inputs can be delayed or they can try to take into account specific latency (digital processing of wireless transmission or digital effects) and compensate for it to better align the amplification and the acoustic sound.\n- Minimal Latency can be toggled instead of Acoustic Precedence. On the other hand this tries to let the sound out through the system as soon as possible. The system scans this input's feeds to the outputs for lowest delay and subtracts it from all delays and bypasses additional Haas effect. Here the idea would be to beat the acoustic sound on stage to try and place a sound in a slightly different position by altering the location first perceived.\n- The location (position and offset) for any input can be given in Cartesian, Cylindrical or Spherical coordinates independently from the stage shape or other channels.\n- The position can be constrained to the dimensions of the stage in Cartesian coordinates or to a specific distance range in polar coordinates.\n- Flip will take symmetrical position for the given coordinate around the origin point.\n- The joystick and vertical slider allow relative control of the position.\n- Inputs can be assigned to a cluster to group them for coordinated movements.
  - DE: Eingänge verfügen über eine Vielzahl von Einstellungen für verschiedene Situationen, die realistische Beschallung oder kreative Werkzeuge für Sounddesign erfordern.\n- Der Eingangspegel kann angepasst werden.\n- Eingänge können verzögert werden oder versuchen, spezifische Latenz (digitale Verarbeitung von Funkübertragung oder digitalen Effekten) zu berücksichtigen und zu kompensieren, um Verstärkung und akustischen Klang besser auszurichten.\n- Minimal Latency kann anstelle von Acoustic Precedence aktiviert werden. Dies versucht den Klang so schnell wie möglich durch das System zu bringen. Das System scannt die Sends dieses Eingangs zu den Ausgängen auf die niedrigste Verzögerung und zieht sie von allen Verzögerungen ab, wobei der zusätzliche Haas-Effekt umgangen wird.\n- Die Position (Position und Offset) kann in Kartesischen, Zylindrischen oder Sphärischen Koordinaten unabhängig von der Bühnenform oder anderen Kanälen angegeben werden.\n- Die Position kann auf die Bühnendimensionen in Kartesischen Koordinaten oder auf einen bestimmten Entfernungsbereich in Polarkoordinaten beschränkt werden.\n- Flip nimmt die symmetrische Position für die gegebene Koordinate um den Ursprungspunkt.\n- Der Joystick und der vertikale Schieberegler ermöglichen relative Positionssteuerung.\n- Eingänge können einem Cluster zugewiesen werden, um sie für koordinierte Bewegungen zu gruppieren.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Inputs Basic Parameters
  - DE: Grundlegende Eingangsparameter
  - [ ] OK    Fix: 

## `help.inputHF`

- **`body`**
  - EN: When talking away, the timbre of a voice will sound less bright. Reproducing this was the initial goal here even though we usually want to have support for voices when not addressing the audience or in bi-frontal configurations. This can be put to good use for creative effects such as having a brighter reverb on a dampened direct sound.\nThe orientation of the input in azimuth and in pitch can be set as well as the angle where the high frequencies will not be filtered.\nThe HF Shelf will set the maximum attenuation in the back of the input. There is a smooth fade (like a cosine curve) from full brightness in front to damped at the rear.
  - DE: Wenn man sich abwendet, klingt das Timbre einer Stimme weniger brillant. Dies zu reproduzieren war das ursprüngliche Ziel, auch wenn wir normalerweise Unterstützung für Stimmen wünschen, die nicht zum Publikum sprechen oder in bi-frontalen Konfigurationen. Dies kann für kreative Effekte genutzt werden, wie eine brillantere Nachhall auf einem gedämpften Direktsignal.\nDie Orientierung des Eingangs in Azimut und Neigung kann eingestellt werden sowie der Winkel, in dem die Hochfrequenzen nicht gefiltert werden.\nDas HF Shelf bestimmt die maximale Dämpfung hinter dem Eingang. Es gibt einen sanften Übergang (wie eine Kosinuskurve) von voller Brillanz vorne zu gedämpft hinten.
  - [ ] OK    Fix: 

- **`title`**
  - EN: High Frequency Directivity
  - DE: Hochfrequenz-Directivity
  - [ ] OK    Fix: 

## `help.inputLevel`

- **`body`**
  - EN: There are two level attenuation models. One where the level decreases with the distance by a given ratio in dB/m. Otherwise the level is halved each time the distance doubles. This might be more realistic, but it may be too loud close to the source or not give enough focus. The former may be less physically accurate, it offers usually better control to have a more even and stable mix.
  - DE: Es gibt zwei Pegelabschwächungsmodelle. Eines, bei dem der Pegel mit der Entfernung um ein bestimmtes Verhältnis in dB/m abnimmt. Alternativ halbiert sich der Pegel jedes Mal wenn sich die Entfernung verdoppelt. Letzteres mag realistischer sein, kann aber in der Nähe der Quelle zu laut sein oder nicht genug Fokus bieten. Ersteres mag physikalisch weniger genau sein, bietet aber in der Regel bessere Kontrolle für einen gleichmäßigeren und stabileren Mix.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Level Adjustments
  - DE: Pegelanpassungen
  - [ ] OK    Fix: 

## `help.inputMutes`

- **`body`**
  - EN: You can set for each output array a specific attenuation for the selected input.\nYou can mute each send to any output individually. There are macros to speed up the process.
  - DE: Array-Abschwächung und Ausgangs-Mutes\n\nSie können für jedes Ausgangs-Array eine spezifische Abschwächung für den ausgewählten Eingang festlegen.\nSie können jeden Send zu jedem Ausgang einzeln stummschalten. Makros stehen zur Verfügung, um den Prozess zu beschleunigen.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Array Attenuation and Output Mutes
  - DE: Array Attenuation and Output Mutes
  - [ ] OK    Fix: 

## `help.lfo`

- **`body`**
  - EN: Input position can be automated. The LFO can control X, Y and Z position coordinates individually as well as the rotation of HF directivity (gyrophone).\nAdjust the global period and phase for the LFO.\nFor X, Y and Z coordinates select a shape, amplitude, rate and phase as desired. A circle in the XY plane would have sine for shape for X and for Y and an offset of ±90° between the two. A square would be the same but with keystone shapes.\nInput position can be moved while the LFO is running.
  - DE: Die Eingangsposition kann automatisiert werden. Der LFO kann X-, Y- und Z-Koordinaten einzeln steuern sowie die Rotation der HF-Direktionalität (Gyrophon).\nPassen Sie Periode und Phase des LFO global an.\nFür X, Y und Z wählen Sie Form, Amplitude, Rate und Phase. Ein Kreis in der XY-Ebene verwendet Sinusform für X und Y mit ±90° Versatz. Ein Quadrat wäre gleich aber mit Trapezformen.\nDie Eingangsposition kann während des LFO-Betriebs verschoben werden.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Input LFO
  - DE: Eingangs-LFO
  - [ ] OK    Fix: 

## `help.liveSource`

- **`body`**
  - EN: A loud source on stage may not need much reinforcement through the nearby speakers. Imagine an opera singer near the edge of the stage. Normally the level distribution would make the level louder near the input position. But if it is already loud enough we should be able to not over-amplify it. This function manages this.\nThe radius and shape describe how to attenuate the level for a specific source for speakers within the radius of influence of this input. There are various shapes: a V shaped linear effect; a U shape for fast decrease within the radius; a tight V effect or a mix of the previous (sine).\nThe attenuation can be constant or level dependent like a local compression reacting to transients and to average RMS level.
  - DE: Live-Quellen-Dämpfer\n\nEine laute Quelle auf der Bühne braucht möglicherweise keine Verstärkung durch die nahegelegenen Lautsprecher. Stellen Sie sich einen Opernsänger am Bühnenrand vor. Normalerweise würde die Pegelverteilung den Pegel in der Nähe der Eingangsposition erhöhen. Aber wenn es bereits laut genug ist, sollten wir nicht überverstärken. Diese Funktion verwaltet dies.\nRadius und Form beschreiben, wie der Pegel für Lautsprecher innerhalb des Einflussradius dieser Quelle abgeschwächt wird. Es gibt verschiedene Formen: ein V-förmiger linearer Effekt; ein U für schnelle Abnahme; ein enges V oder eine Mischung (Sinus).\nDie Dämpfung kann konstant oder pegelabhängig sein, wie eine lokale Kompression, die auf Transienten und den durchschnittlichen RMS-Pegel reagiert.
  - [ ] OK    Fix: 

- **`legendAttenuation`**
  - EN: attenuation
  - DE: Dämpfung
  - [ ] OK    Fix: 

- **`legendLinear`**
  - EN: linear
  - DE: linear
  - [ ] OK    Fix: 

- **`legendLog`**
  - EN: log
  - DE: log
  - [ ] OK    Fix: 

- **`legendMaxAttenuation`**
  - EN: maximum attenuation
  - DE: maximale Dämpfung
  - [ ] OK    Fix: 

- **`legendNoAttenuation`**
  - EN: no attenuation
  - DE: keine Dämpfung
  - [ ] OK    Fix: 

- **`legendPosition`**
  - EN: position of the source
  - DE: Position der Quelle
  - [ ] OK    Fix: 

- **`legendRadius`**
  - EN: radius
  - DE: Radius
  - [ ] OK    Fix: 

- **`legendSine`**
  - EN: sine
  - DE: sinus
  - [ ] OK    Fix: 

- **`legendSquare`**
  - EN: square x²
  - DE: Quadrat x²
  - [ ] OK    Fix: 

- **`title`**
  - EN: Live Source Tamer
  - DE: Live Source Tamer
  - [ ] OK    Fix: 

## `help.map`

- **`body`**
  - EN: - A left click on an input or a cluster will allow to move it by dragging it. A single finger touch will do the same.\n- A left click with the shift key pressed will add or remove inputs to the selection. A double tap and drag will act the same way.\n- A left click drag will draw a selection rectangle to select multiple inputs and clusters at the same time.\n- A left double-click or tap will reset the position offset of the input.\n- A long left click or press with no movement will switch to the input tab with the focus on the selected input on release.\n- A left click away from any input will clear the selection.\n- A right click and drag will pan the view of the map. A two finger drag with no selected input or cluster will do the same if your operating system supports multitouch.\n- The mouse wheel will zoom in and out. A two finger pinch with no selected input or cluster will also zoom in and out.\n- A middle click will reset the view to fit the stage on the map display. There is also a dedicated set of buttons to reset the view to fit all inputs and to fit the stage respectively.\n- Selected inputs and clusters can also be moved with the arrow keys for X and Y and with the PageUp and PageDown keys for height. Hardware controllers can be used too.\n- When an input is touched, a second finger nearby can rotate the input directivity and adjust the height by pinching if your operating system allows multitouch interaction.\n- When a cluster is touched, a second finger nearby can rotate the cluster and scale it by pinching.\n- Inputs, output arrays and the reverb nodes can be hidden on the map.\n- Inputs can also be locked to prevent selecting and moving them on the map. They will still be moved by clusters, network commands, tracking and hardware controllers.\n- All reverb nodes can be moved on the map if this is enabled on the reverb tab. Holding the Ctrl/Cmd key will move each pair of reverb nodes in symmetry.\n- Inputs with offsets, LFO or with speed regulation will have a temporary position marker. But the point of interaction will remain the normal marker.\n- The Live Source Tamer radius will be displayed around input when activated.\n- There is a toggle to display the audio level for the inputs and outputs on the map tab, that's active when the audio processing is running.
  - DE: - Linksklick auf einen Eingang oder Cluster ermöglicht das Verschieben durch Ziehen.\n- Linksklick mit Umschalttaste fügt Eingänge zur Auswahl hinzu/entfernt sie.\n- Linksklick-Ziehen zeichnet ein Auswahlrechteck.\n- Doppelklick setzt den Positionsversatz zurück.\n- Langer Klick ohne Bewegung wechselt zum Eingangs-Tab.\n- Klick außerhalb löscht die Auswahl.\n- Rechtsklick-Ziehen verschiebt die Kartenansicht. Zwei-Finger-Ziehen ebenso.\n- Mausrad zoomt. Zwei-Finger-Pinch ebenfalls.\n- Mittelklick setzt die Ansicht zurück.\n- Pfeiltasten verschieben X/Y, BildAuf/Ab die Höhe.\n- Ein zweiter Finger kann die Direktionalität drehen und die Höhe anpassen.\n- Bei Clustern kann ein zweiter Finger rotieren und skalieren.\n- Eingänge, Ausgabe-Arrays und Hallknoten können ausgeblendet werden.\n- Eingänge können gesperrt werden.\n- Hallknoten können verschoben werden. Strg/Cmd bewegt Paare symmetrisch.\n- Der Live-Source-Tamer-Radius wird angezeigt.\n- Audiopegel können auf der Karte angezeigt werden.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Map
  - DE: Karte
  - [ ] OK    Fix: 

## `help.mcp`

- **`body`**
  - EN: The MCP server lets an AI assistant (Claude Desktop, Claude Code, ChatGPT with custom connectors) read and write the parameters of this WFS-DIY session over a local network connection.\n\nWhat the AI can do:\n• Read live state: channel counts, names, positions, attenuations, EQs, snapshots, clusters, the full parameter surface.\n• Move sources, rename channels, set cluster assignments, adjust the array layout, place outputs and reverbs.\n• Run guided workflows (system tuning walkthroughs, troubleshooting localization, snapshot management) via prepared prompt templates.\n\nOperator controls on this row:\n• AI: ON / OFF — master switch. When OFF every AI tool call is refused; when ON the AI works under the rules below.\n• AI critical actions: blocked / ALLOWED — the destructive actions (deleting snapshots, resetting DSP, changing channel counts) are blocked by default. Click to allow them for 10 minutes; the red fill drains as the window expires, then they auto-block again.\n• Open AI History — scrollable timeline of every recent AI change with per-row undo / redo and a step-by-step cursor.\n• The MCP URL button copies the server URL to the clipboard for AI clients that take a URL directly.\n\nOperator awareness:\n• Every AI action is recorded with origin tags. The AI History window shows the full timeline; per-row × reverses an action with its dependents.\n• If you manually adjust a parameter the AI just moved, the AI is notified and will not blindly retry. You always have the last word.\n• The Cmd/Ctrl+Alt+Z and Cmd/Ctrl+Alt+Y shortcuts undo and redo the last AI change without affecting your manual edits (which use plain Ctrl+Z as usual).\n\nTo add this server to Claude Desktop:\n  1. Open Settings → Developer → Edit Config.\n  2. Paste the JSON snippet below into claude_desktop_config.json (merge into the existing mcpServers block if you already have one).\n  3. Restart Claude Desktop. The server appears as 'wfs-diy' in the tools menu.\n\nTo add to Claude Code, run:\n  claude mcp add wfs-diy <url> -t http\n\nThe URL changes if you switch network interface or if the server falls back to a different port. The URL button on this row always reflects the live URL.
  - DE: KI / MCP-Server\n\nDer MCP Server ermöglicht es einem KI-Assistenten (Claude Desktop, Claude Code, ChatGPT mit benutzerdefinierten Konnektoren), die Parameter dieser WFS-DIY-Sitzung über eine lokale Netzwerkverbindung zu lesen und zu schreiben.\n\nWas die KI tun kann:\n• Live-Status lesen: Kanalanzahl, Namen, Positionen, Dämpfungen, EQs, Snapshots, Cluster, die gesamte Parameteroberfläche.\n• Quellen verschieben, Kanäle umbenennen, Cluster-Zuweisungen festlegen, Array-Layout anpassen, Ausgänge und Hall platzieren.\n• Geführte Workflows ausführen (Systemabstimmung, Lokalisations-Fehlersuche, Snapshot-Verwaltung) über vorbereitete Prompt-Vorlagen.\n\nBedienelemente in dieser Zeile:\n• KI: EIN / AUS — Hauptschalter. Im AUS-Zustand wird jeder KI-Tool-Aufruf abgelehnt; im EIN-Zustand arbeitet die KI nach den unten stehenden Regeln.\n• Kritische KI-Aktionen: gesperrt / ERLAUBT — destruktive Aktionen (Snapshots löschen, DSP zurücksetzen, Kanalanzahl ändern) sind standardmässig gesperrt. Klicken, um sie 10 Minuten lang zu erlauben; die rote Füllung läuft mit dem Ablauf des Fensters ab und wird dann automatisch wieder gesperrt.\n• KI-Verlauf öffnen — scrollbare Chronik aller jüngsten KI-Änderungen mit Rückgängig/Wiederherstellen pro Zeile.\n• Die MCP-URL-Schaltfläche kopiert die Server-URL in die Zwischenablage für KI-Clients, die eine URL direkt akzeptieren.\n\nBetreiber-Aufmerksamkeit:\n• Jede KI-Aktion wird mit Herkunfts-Tags aufgezeichnet. Das KI-Verlaufsfenster zeigt die vollständige Chronik; das × pro Zeile macht eine Aktion mit ihren Abhängigkeiten rückgängig.\n• Wenn Sie einen Parameter manuell anpassen, den die KI gerade verschoben hat, wird die KI benachrichtigt und versucht es nicht blindlings erneut. Sie haben immer das letzte Wort.\n• Die Tastenkombinationen Cmd/Strg+Alt+Z und Cmd/Strg+Alt+Y machen die letzte KI-Änderung rückgängig bzw. stellen sie wieder her, ohne Ihre manuellen Änderungen zu beeinflussen (die wie üblich Strg+Z verwenden).\n\nUm diesen Server zu Claude Desktop hinzuzufügen:\n  1. Einstellungen → Entwickler → Konfiguration bearbeiten öffnen.\n  2. Den JSON-Ausschnitt unten in claude_desktop_config.json einfügen (in den vorhandenen mcpServers-Block einfügen, falls bereits vorhanden).\n  3. Claude Desktop neu starten. Der Server erscheint als 'wfs-diy' im Tools-Menü.\n\nFür Claude Code ausführen:\n  claude mcp add wfs-diy <url> -t http\n\nDie URL ändert sich, wenn Sie die Netzwerkschnittstelle wechseln oder der Server auf einen anderen Port ausweicht. Die URL-Schaltfläche in dieser Zeile zeigt immer die aktuelle Live-URL.
  - [ ] OK    Fix: 

- **`copyButton`**
  - EN: Copy Config
  - DE: Konfiguration kopieren
  - [ ] OK    Fix: 

- **`copyConfirm`**
  - EN: MCP config JSON copied to clipboard
  - DE: MCP-Konfigurations-JSON in die Zwischenablage kopiert
  - [ ] OK    Fix: 

- **`title`**
  - EN: AI / MCP Server
  - DE: AI / MCP Server
  - [ ] OK    Fix: 

## `help.network`

- **`body`**
  - EN: The system can communicate through several network protocols, (UDP or TCP) using OSC. OSC query can be enabled to allow the clients to discover the possible OSC paths and subscribe to some parameter updates.\nThe IP of the local machine corresponding to the selected network interface is shown. The incoming TCP and UDP ports as well as the OSC Query port.\nThere are a few specialised OSC clients such as:\n- Remote for the Android application for multitouch operation and for remote control.\n- QLab that can send data and that can also be programmed directly from the application.\n- ADM-OSC for control from consoles and DAW (see specific help).\nThe data can be filtered to only allow the data from the recorded IP addresses or to allow any client sending on the correct ports.\nThere is a Log window to see what data comes in or out, filter by the type of protocol, client and so on.\nThere is also a locator function to find a lost remote Android tablet. It will flash and sound the alarm on the missing device.
  - DE: Das System kann über mehrere Netzwerkprotokolle (UDP oder TCP) mittels OSC kommunizieren. OSC Query kann aktiviert werden, damit Clients die möglichen OSC-Pfade entdecken und Parameteraktualisierungen abonnieren können.\nDie IP der lokalen Maschine entsprechend der ausgewählten Netzwerkschnittstelle wird angezeigt. Die eingehenden TCP- und UDP-Ports sowie der OSC Query-Port.\nEs gibt einige spezialisierte OSC-Clients wie:\n- Remote für die Android-Anwendung für Multitouch-Bedienung und Fernsteuerung.\n- QLab zum Senden von Daten und zur direkten Programmierung aus der Anwendung.\n- ADM-OSC zur Steuerung von Konsolen und DAW (siehe spezifische Hilfe).\nDie Daten können gefiltert werden. Ein Log-Fenster zeigt ein- und ausgehende Daten.\nEs gibt auch eine Ortungsfunktion für verlorene Android-Tablets.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Network
  - DE: Netzwerk
  - [ ] OK    Fix: 

## `help.outputAdvanced`

- **`body`**
  - EN: There are a few parameters to help you adjust to the acoustic sound.\nMost of these parameters are set for whole arrays unless the propagation mode is switched to off for this output in the array. Relative change can also be selected after a specific setting.\n- Orientation and On/Off Angles define what inputs each speaker will amplify. By default the speakers are pointing to the audience, away from the stage. Inputs in the green sector will be amplified, but not the ones in front of the speaker, in the red sector. There is a fade between both sectors. For sub-bass speakers which usually come in limited numbers and locations, opening all the way to the maximum will allow you to have all inputs possibly picked up by the subwoofers.\n- HF Damping simulates the loss of high frequency with distance. Speakers close to the listeners can have more than speakers away from the stage and the listeners.\n- Distance Attenuation percentage allows you to define if more or less of the attenuation calculated from the distance and parameters of the input is applied. Again for Sub-bass in case you only have two and don't want to lose too much level or have hot-spots it may be wise to lower this setting to 50%.\n- Minimal Latency allows or excludes this speaker from Minimal Latency processing. When allowed, the output is scanned for the smallest calculated delay and, once the setting is engaged on an input, the delay for that input through this speaker is reduced.\n- Live Source Attenuation allows or excludes this speaker from the level reduction of nearby inputs that have this setting enabled. This may not be necessary for speakers away from the audience or for sub-bass.\n- Floor Reflections allows or excludes this speaker from the reflections applied to the signal, such as sub-bass and flown arrays...
  - DE: Es gibt einige Parameter zur Anpassung an den akustischen Klang.\nDie meisten dieser Parameter gelten für ganze Arrays, es sei denn, der Ausbreitungsmodus ist für diesen Ausgang im Array deaktiviert. Relative Änderung kann auch nach einer bestimmten Einstellung gewählt werden.\n- Orientierung und On/Off-Winkel definieren, welche Eingänge jeder Lautsprecher verstärkt. Standardmäßig zeigen die Lautsprecher zum Publikum, weg von der Bühne. Eingänge im grünen Sektor werden verstärkt, nicht aber die vor dem Lautsprecher im roten Sektor. Es gibt einen Übergang zwischen beiden Sektoren. Für Subbass-Lautsprecher kann das vollständige Öffnen alle Eingänge einbeziehen.\n- HF-Dämpfung simuliert den Hochfrequenzverlust mit der Entfernung.\n- Der Prozentsatz der Distance attenuation definiert, wie viel der berechneten Dämpfung angewendet wird. Für Subbass kann es sinnvoll sein, auf 50% zu senken.\n- Minimal Latency aktiviert das Scannen nach der kleinsten berechneten Verzögerung.\n- Live-Source-Attenuation aktiviert die Pegelreduzierung naher Eingänge.\n- Floor Reflections aktiviert ob Reflexionen für diesen Ausgang angewendet werden, wie Subbass und geflogene Arrays...
  - [ ] OK    Fix: 

- **`title`**
  - EN: Advanced Parameters
  - DE: Erweiterte Parameter
  - [ ] OK    Fix: 

## `help.outputs`

- **`body`**
  - EN: The design of the WFS system has to do with the right choice of equipment and their position. Here is a guide to help you with the design and the tuning of your arrays.\nAn array is a line (straight or curved) of speakers. This is one of the most important concepts in WFS adapted to sound reinforcement and for creative sound design.\nA rule of thumb would be that each listener should hear three speakers of an array to have enough psycho-acoustic cues to feel the direction each sound is coming from. There will be a sweet spot to find between the distance between the speakers and the listeners, their spacing and coverage angle. This is especially true for short range arrays, a.k.a front fills. Speakers with a 120° coverage angle can be spaced by the same distance between the array and the first row of listeners. The number of speakers also depends on the sound pressure level. When placed as a flown array, trapezoidal/asymmetric horns with wide coverage angle (120°) below axis and narrow coverage (60°) on axis will give good coverage for seat rows a bit too far from the ground array and will have enough throw to reach 20m or 30m away while avoiding the walls of the venue where they would create reflections that give away the position of the speakers to the ears of the listeners. Most of the time for larger venues coaxial (elliptical or conical horns) have not enough reach and require one or more delay lines. They are more suited for smaller venues with few rows of seats.\nThe positioning of the speakers in the system can be done through the 'Wizard of OutZ' and the editable presets it has.
  - DE: Das Design des WFS-Systems betrifft die richtige Auswahl der Ausrüstung und deren Positionierung. Hier ist ein Leitfaden für das Design und die Abstimmung Ihrer Arrays.\nEin Array ist eine Linie (gerade oder gekrümmt) von Lautsprechern. Dies ist eines der wichtigsten Konzepte in WFS, angepasst an Beschallung und kreatives Sounddesign.\nAls Faustregel sollte jeder Zuhörer drei Lautsprecher eines Arrays hören, um genügend psychoakustische Hinweise für die Richtungswahrnehmung zu haben. Es gibt einen Sweet Spot zwischen dem Abstand der Lautsprecher zu den Zuhörern, ihrem Abstand zueinander und ihrem Abstrahlwinkel. Lautsprecher mit 120° Abstrahlwinkel können im gleichen Abstand wie zwischen Array und erster Reihe platziert werden. Die Anzahl hängt auch vom Schalldruckpegel ab. Als Hänge-Array bieten trapezförmige/asymmetrische Hörner mit breitem Abstrahlwinkel (120°) unter der Achse und schmalem (60°) in der Achse gute Abdeckung und Reichweite bis 20-30m, während Wandreflexionen vermieden werden. Koaxiale Lautsprecher haben meist nicht genug Reichweite für große Räume und benötigen Delay-Lines.\nDie Positionierung kann über den 'Wizard of OutZ' und seine editierbaren Presets erfolgen.
  - [ ] OK    Fix: 

- **`title`**
  - EN: WFS Array Design
  - DE: WFS-Array-Design
  - [ ] OK    Fix: 

## `help.overview`

- **`body`**
  - EN: This WFS spatial sound processor aims to be a tool for natural sound reinforcement and also a creative tool opening new avenues for writing sound in space.\nSome parameters are straightforward: place sound (Map, Tracking, Speed Limiting, Gradient Maps...), work its shape (Attenuation Profile) and its acoustic presence (Directivity, Floor Reflections), give it a one-off motion (AutomOtion) or a repetitive movement (L.F.O). In some cases amplification should be limited around loud sources on stage (Live Source Tamer). All of these functionalities can be stored and recalled internally or with the help of QLab. On the other side the system allows real-time interaction to trigger and move samples, move large clusters of inputs manually or thanks to easily recallable LFO presets.
  - DE: Dieser räumliche WFS-Klangprozessor soll ein Werkzeug für natürliche Beschallung und zugleich ein kreatives Werkzeug sein, das neue Wege eröffnet, Klang im Raum zu gestalten.\nEinige Parameter sind unkompliziert: Klang platzieren (Karte, Tracking, Geschwindigkeitsbegrenzung, Gradient Maps...), seine Form bearbeiten (Attenuation-Profil) und seine akustische Präsenz (Directivity, Floor Reflections), ihm eine einmalige Bewegung (AutomOtion) oder eine repetitive Bewegung (L.F.O) geben. In manchen Fällen sollte die Verstärkung um laute Quellen auf der Bühne begrenzt werden (Live Source Tamer). Alle diese Funktionen können intern oder mit Hilfe von QLab gespeichert und abgerufen werden. Darüber hinaus ermöglicht das System Echtzeit-Interaktion zum Auslösen und Bewegen von Samples, zum Bewegen großer Eingabe-Cluster manuell oder dank einfach abrufbarer LFO-Presets.
  - [ ] OK    Fix: 

- **`dontShow`**
  - EN: Don't show this again
  - DE: Nicht mehr anzeigen
  - [ ] OK    Fix: 

- **`title`**
  - EN: System Overview
  - DE: Systemübersicht
  - [ ] OK    Fix: 

## `help.parallax`

- **`body`**
  - EN: Each speaker points more or less clearly at a listener. To calculate the delay for an input for each speaker, let's consider the distance from the input to this listener, we can also calculate the distance the sound from the speaker to this listener. To match the time of arrival of both we need to apply the difference in the aforementioned distances as a delay for this input and this speaker. This gives greater stability when inputs are moved on stage and especially when they move upstage, away from the edge of the stage. This also can enable the synthesis of floor reflections. This setting can be tuned for finer adjustments, rather than simply measured. Use your ears!
  - DE: Jeder Lautsprecher zeigt mehr oder weniger deutlich auf einen Zuhörer. Um die Verzögerung für einen Eingang für jeden Lautsprecher zu berechnen, betrachten wir die Entfernung vom Eingang zu diesem Zuhörer, wir können auch die Entfernung des Schalls vom Lautsprecher zu diesem Zuhörer berechnen. Um die Ankunftszeit beider abzugleichen, müssen wir die Differenz der genannten Entfernungen als Verzögerung anwenden. Dies gibt größere Stabilität wenn Eingänge auf der Bühne bewegt werden und besonders wenn sie sich vom Bühnenrand entfernen. Dies kann auch die Synthese von Bodenreflexionen ermöglichen. Diese Einstellung kann fein abgestimmt werden, anstatt nur gemessen zu werden. Vertrauen Sie Ihren Ohren!
  - [ ] OK    Fix: 

- **`title`**
  - EN: Parallax Correction
  - DE: Parallaxenkorrektur
  - [ ] OK    Fix: 

## `help.reverb`

- **`body`**
  - EN: Reverb helps blur the actual reflections of the speakers in the acoustic space. Without some reverb the listener might perceive the sound missing some depth because the reflections of the speakers in the acoustic of the room gives away their positions and all sounds might feel as coming from the plane of the speakers.\nPlace the reverb node according to the number of channels and the geometry of the stage and listening area, and the positions of speakers. If necessary the return position can be offset from the feed position.\nOther parameters are very similar to Outputs' and Inputs'.
  - DE: Hall hilft, die Reflexionen der Lautsprecher zu verschleiern.\nPlatzieren Sie die Hall-Knoten entsprechend der Kanalanzahl und Bühnengeometrie.\nAndere Parameter sind ähnlich wie bei Ausgängen und Eingängen.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb
  - DE: Hall
  - [ ] OK    Fix: 

## `help.reverbAlgo`

- **`body`**
  - EN: There are three built-in reverb types inside this spatial sound processor:\n- SDN (Scattered Delay Network): The sound bounces between every reverb node which act as reflecting surfaces. This reverb algorithm favours an odd number of nodes that should not have too much symmetry. This helps reduce the artifacts of the reverb or any metallic sounding resonance.\n- FDN (Feedback Delay Network): Each reverb node functions as a separate reverb processor with a classical reverb algorithm. Place nodes around the stage and eventually around the audience. Inputs are sent to each node in a similar fashion as with outputs. The return signal from the reverb is then distributed like other inputs to all outputs.\n- IR (Impulse Response): This is a classical convolution reverb. You can load audio samples as impulse responses. Each reverb node can share the same IR or they can have different ones. Place nodes around the stage and eventually around the audience.\nThe node positions can be adjusted directly on the map. The Ctrl/Cmd key will move a reverb node pair in symmetry.
  - DE: Drei eingebaute Halltypen stehen in diesem räumlichen Klangprozessor zur Verfügung:\n- SDN (Scattered Delay Network): Der Klang wird zwischen allen Hall-Knoten reflektiert, die als reflektierende Oberflächen wirken. Dieser Algorithmus bevorzugt eine ungerade Anzahl von Knoten ohne zu viel Symmetrie, um Artefakte oder metallische Resonanzen zu reduzieren.\n- FDN (Feedback Delay Network): Jeder Hall-Knoten funktioniert als separater Hallprozessor mit einem klassischen Algorithmus. Platzieren Sie Knoten um die Bühne und eventuell um das Publikum.\n- IR (Impulsantwort): Klassische Faltungshall. Sie können Audiosamples als Impulsantworten laden. Jeder Knoten kann dieselbe IR teilen oder verschiedene verwenden.\nDie Knotenpositionen können direkt auf der Karte angepasst werden. Die Strg/Cmd-Taste bewegt ein Hall-Knotenpaar symmetrisch.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Algorithms
  - DE: Hallalgorithmen
  - [ ] OK    Fix: 

## `help.reverbFeed`

- **`body`**
  - EN: Pre-processing sending of the input channels to the reverb nodes.\n- Orientation and On/Off Angles define what inputs each reverb node will receive. Inputs in the green sector will be picked-up, but not the ones in front in the red sector. There is a fade between both sectors.\n- HF Damping simulates the loss of high frequency with distance.\n- Distance Attenuation percentage allows you to define if more or less of the attenuation calculated from the distance and parameters of the input is applied.\n- Minimal Latency toggles whether the output is scanned for the smallest calculated delay and also if the setting once engaged on an input will reduce the delay for that input through this reverb node.\n- Live Source Attenuation toggles the reduction of level of nearby input if they have this setting enabled.
  - DE: Hall-Einspeisung\n\nVorverarbeitung der Eingangskanalzuführung zu den Hall-Knoten.\n- Orientierung und On/Off-Winkel bestimmen welche Eingänge empfangen werden.\n- HF-Dämpfung simuliert Hochfrequenzverlust mit Distanz.\n- Distanz-Dämpfung bestimmt die angewandte Abschwächung.\n- Minimal Latency schaltet die Verwendung des kleinsten Delays um.\n- Live-Source-Dämpfung reduziert den Pegel nahegelegener Eingänge.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Feed
  - DE: Reverb Feed
  - [ ] OK    Fix: 

## `help.reverbPostProc`

- **`body`**
  - EN: Includes a 4-band EQ and an Expander that looks at the signal entering the reverb processor to reduce long reverb tails when the inputs are quiet.
  - DE: Enthält einen 4-Band-EQ und einen Expander, der das Signal vor dem Hallprozessor überwacht, um lange Hallfahnen zu reduzieren, wenn die Eingänge leise sind.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Post-Processing
  - DE: Hall-Nachbearbeitung
  - [ ] OK    Fix: 

## `help.reverbPreProc`

- **`body`**
  - EN: Includes a 4-band EQ and a Compressor to remove transients which might excite the reverb processor a bit too much.
  - DE: Enthält einen 4-Band-EQ und einen Kompressor, um Transienten zu entfernen, die den Hallprozessor zu stark anregen könnten.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Pre-Processing
  - DE: Hall-Vorverarbeitung
  - [ ] OK    Fix: 

## `help.reverbReturn`

- **`body`**
  - EN: Post-processing sending to the speakers.\n- Distance Attenuation defines the level drop per meter to the speakers.\n- Common Attenuation will look for the lowest attenuation and keep only a percentage of this level drop and apply this across all other outputs.\n- Mutes and Mute Macros allow to prevent a reverb channel from feeding an output. For instance it may not be necessary to send reverb to sub-bass speakers.
  - DE: Hall-Rückgabe\n\nNachbearbeitung an die Lautsprecher.\n- Distanz-Dämpfung definiert den Pegelabfall pro Meter.\n- Gemeinsame Dämpfung behält einen Prozentsatz der niedrigsten Dämpfung.\n- Mutes verhindern, dass ein Hall-Kanal einen Ausgang speist.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Return
  - DE: Reverb Return
  - [ ] OK    Fix: 

## `help.sampler`

- **`body`**
  - EN: The sampler allows to trigger samples and interact with them in real time.\nThe sampler when enabled on a track will replace the live input at all times.\nSeveral samplers can be assigned to different inputs and triggered individually.\nTo use the sampler:\n- Select a Roli Lightpad or a pad on the connected Android Remote app.\n- Add samples to the different tiles in the grid to the left. Adjust their relative starting position and their level and eventually their in and out points. Several samples can be selected using the shift key while clicking.\n- Create sets of samples: selected samples will be added to new sets. Samples can be added or removed after the creation of a set by holding Ctrl/Cmd while clicking on the tiles. Each set can be renamed. Each set can either have a fixed sequence or a random order (round robin, each sample is played once before a new random order is drawn). Each set has an attenuation setting. Each set has a base position applied to the input when selecting the set. It can be moved on the map or using external control. The sample position offset is added to the set position each time a sample is triggered.\n- Press a Roli Lightpad or a pad on the Android app to trigger a sample. The pressure applied to the pad can be mapped to any of the following controls: level, height and high frequency filtering. The sensitivity can be adjusted for each. The movement of the finger on the pad will cause the sound to move. This acts by measuring the deflection from the initial contact point like a joystick. This can be disabled. All sets have their respective settings for the interaction.\nReleasing the pad will stop the triggered sample.\nSampler settings are stored in the input files.\nFor convenience sample tiles and sets can be copied, exported and imported.
  - DE: Der Sampler ermöglicht das Auslösen von Samples und die Echtzeit-Interaktion mit ihnen.\nWenn der Sampler auf einer Spur aktiviert ist, ersetzt er den Live-Eingang dauerhaft.\nMehrere Sampler können verschiedenen Eingängen zugewiesen und einzeln ausgelöst werden.\nVerwendung des Samplers:\n- Wählen Sie ein Roli Lightpad oder ein Pad in der verbundenen Android Remote App.\n- Fügen Sie Samples zu den verschiedenen Kacheln im Raster hinzu. Passen Sie die relative Startposition, den Pegel und die Ein-/Ausstiegspunkte an. Mehrere Samples können mit Umschalttaste und Klick ausgewählt werden.\n- Erstellen Sie Sample-Sets: Ausgewählte Samples werden neuen Sets hinzugefügt. Samples können nach der Erstellung eines Sets mit Strg/Cmd und Klick hinzugefügt oder entfernt werden. Jedes Set kann umbenannt werden und eine feste Sequenz oder zufällige Reihenfolge haben. Jedes Set hat eine Dämpfungseinstellung und eine Basisposition.\n- Drücken Sie ein Lightpad oder Pad, um ein Sample auszulösen. Der Druck kann auf Pegel, Höhe und Hochfrequenzfilterung gemappt werden. Die Fingerbewegung bewegt den Klang wie ein Joystick.\nLoslassen stoppt das ausgelöste Sample.\nSampler-Einstellungen werden in den Eingangsdateien gespeichert.\nKacheln und Sets können kopiert, exportiert und importiert werden.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Sampler
  - DE: Sampler
  - [ ] OK    Fix: 

## `help.sessionData`

- **`body`**
  - EN: When starting a session select the working folder where the system will place the files and eventual audio files. For new projects create a new folder. When reloading a previous session navigate to the corresponding folder.\nEach section has a separate xml file (System configuration, Network, Outputs, Reverbs, Inputs) and backups. Additionally convolution reverb impulse responses and audio samples will be stored in sub-directories.\nEach section can be stored and recalled individually or as a whole.\nEach section can also export and import files from other projects.
  - DE: Beim Start einer Sitzung wählen Sie den Arbeitsordner, in dem das System die Dateien und eventuellen Audiodateien ablegen wird. Für neue Projekte erstellen Sie einen neuen Ordner. Beim Laden einer vorherigen Sitzung navigieren Sie zum entsprechenden Ordner.\nJeder Abschnitt hat eine separate XML-Datei (Systemkonfiguration, Netzwerk, Ausgänge, Halleffekte, Eingänge) und Backups. Faltungshall-Impulsantworten und Audiosamples werden in Unterverzeichnissen gespeichert.\nJeder Abschnitt kann einzeln oder als Ganzes gespeichert und abgerufen werden.\nJeder Abschnitt kann auch Dateien aus anderen Projekten exportieren und importieren.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Session Data
  - DE: Sitzungsdaten
  - [ ] OK    Fix: 

## `help.shortcuts`

- **`body`**
  - EN: *H* opens the help card closest to the pointer.\n*I*, *O* and *R* open the Input, Output and Reverb tabs respectively; for a few seconds afterwards you can type a channel number to select it (confirm with *Enter*).\n*N* opens the Network tab.\n*C* opens the Clusters tab.\n*M* opens the Map tab.\n*Spacebar* scrolls to the next channel and *Shift+Spacebar* to the previous one in the Input, Output and Reverb tabs. On the Clusters tab they cycle through the clusters.\n*Ctrl/Cmd* while adjusting a parameter of an output channel that is part of an array adjusts the parameter for the selected channel only, temporarily disabling the propagation to the rest of the array.\n*F1* to *F10* assign inputs to the corresponding cluster in the Input and Map tabs, assign outputs to the corresponding array in the Output tab, and select the corresponding cluster in the Clusters tab. *F11* sets the channel back to Single.\n*Shift* while adjusting a parameter of an input that is part of a cluster adjusts this parameter for the other inputs of the cluster in relative mode: the variation affects all inputs of the cluster, but relative offsets are kept. *Ctrl/Cmd+Shift* changes the parameter in absolute mode: the value becomes identical across all inputs of the cluster.\n*Ctrl/Cmd+Z* undoes the last change; *Ctrl/Cmd+Y* or *Ctrl/Cmd+Shift+Z* redoes it.
  - DE: *H* öffnet die Hilfekarte, die dem Mauszeiger am nächsten liegt.\n*I*, *O* und *R* öffnen die Tabs Inputs (Eingänge), Outputs (Ausgänge) bzw. Reverb; einige Sekunden danach kann eine Kanalnummer eingegeben werden, um den Kanal auszuwählen (mit *Enter* bestätigen).\n*N* öffnet den Network-Tab (Netzwerk).\n*C* öffnet den Clusters-Tab.\n*M* öffnet den Map-Tab (Karte).\nDie *Leertaste* springt zum nächsten Kanal, *Umschalt+Leertaste* zum vorherigen — in den Tabs Inputs, Outputs und Reverb. Im Clusters-Tab wird stattdessen durch die Cluster geblättert.\n*Strg/Cmd* während der Änderung eines Parameters eines Ausgangs, der zu einem Array gehört, ändert den Parameter nur für den ausgewählten Kanal und deaktiviert vorübergehend die Übertragung auf den Rest des Arrays.\n*F1* bis *F10* weisen Eingänge im Inputs- und Map-Tab dem entsprechenden Cluster zu, weisen Ausgänge im Outputs-Tab dem entsprechenden Array zu und wählen im Clusters-Tab den entsprechenden Cluster aus. *F11* setzt den Kanal zurück auf Single.\n*Umschalt* während der Änderung eines Parameters eines Eingangs, der zu einem Cluster gehört, ändert diesen Parameter für die anderen Eingänge des Clusters im Relativmodus: Die Änderung wirkt auf alle Eingänge des Clusters, die relativen Abstände bleiben erhalten. *Strg/Cmd+Umschalt* ändert den Parameter im Absolutmodus: Der Wert wird für alle Eingänge des Clusters identisch.\n*Strg/Cmd+Z* macht die letzte Änderung rückgängig; *Strg/Cmd+Y* oder *Strg/Cmd+Umschalt+Z* stellt sie wieder her.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Keyboard Shortcuts
  - DE: Tastaturkürzel
  - [ ] OK    Fix: 

## `help.snapshotScope`

- **`body`**
  - EN: Snapshots store input parameters, but can have a scope to be recalled during a performance. They can have between all parameters for all inputs and only one parameter for a single channel. They can be updated and renamed for convenience.\nThe Scope tells the system what data to store or recall. It's the opposite of 'safe' parameters.\nThere are several ways to do this in this application:\n- Record only the needed data in local files. The scope filter is applied when storing the data. A recall cue can be created in QLab to trigger the reading of the local file.\n- Record all data and a filter in local files. The scope filter is applied when recalling the data. This allows to eventually recall all data not taking into account the scope filter. This may come in handy when a complete configuration should be recalled during rehearsal for example. A recall cue can be created in QLab to trigger the reading of the local file.\n- Record all data in scope in QLab cues. This should not be used to recall all parameters for large configurations since QLab may stall when recalling so much data.\nThe scope can show and automatically pre-select the parameters that have been manually changed (local UI, hardware controllers, remote Android application). Changed parameters are marked with a yellow mark.
  - DE: Snapshots speichern Eingangsparameter, können aber einen Bereich haben, der während einer Aufführung abgerufen wird.\nDer Bereich teilt dem System mit, welche Daten gespeichert oder abgerufen werden sollen.\nMehrere Methoden sind verfügbar:\n- Nur benötigte Daten in lokalen Dateien speichern. Der Filter wird beim Speichern angewandt.\n- Alle Daten und einen Filter in lokalen Dateien speichern. Der Filter wird beim Abrufen angewandt.\n- Alle Daten im Bereich in QLab-Cues speichern. Nicht für große Konfigurationen empfohlen.\nDer Bereich kann manuell geänderte Parameter anzeigen und automatisch vorauswählen. Geänderte Parameter sind gelb markiert.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Input Snapshots and Scope
  - DE: Eingangs-Snapshots und Bereich
  - [ ] OK    Fix: 

## `help.tracking`

- **`body`**
  - EN: Tracking allows to follow the position in 2D or 3D of actors and musicians. There are several tracking solutions based on UWB (UltraWide Band) tags, 3D camera, computer vision systems and Infra-Red LED's followed by IR sensitive cameras.\nThis application allows you to receive tracking data from several protocols: OSC, MQTT, PosiStageNet/PSN, RTTrP.\nYou can select which protocol you're using and enter its settings. You may also adjust the mapping (offset, scaling and orientation).\nEach input has a toggle to enable tracking, an ID to select which tracking marker it should follow and a smoothing algorithm to reduce jittery movements.
  - DE: Tracking ermöglicht die Verfolgung der 2D- oder 3D-Position von Schauspielern und Musikern. Es gibt verschiedene Tracking-Lösungen basierend auf UWB-Tags, 3D-Kameras, Computer-Vision-Systemen und Infrarot-LEDs mit IR-empfindlichen Kameras.\nDiese Anwendung kann Tracking-Daten über mehrere Protokolle empfangen: OSC, MQTT, PosiStageNet/PSN, RTTrP.\nSie können das verwendete Protokoll auswählen und die Einstellungen eingeben. Das Mapping (Versatz, Skalierung und Orientierung) kann ebenfalls angepasst werden.\nJeder Eingang hat einen Schalter zur Aktivierung des Trackings, eine ID zur Auswahl des Markers und einen Glättungsalgorithmus.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Tracking
  - DE: Tracking
  - [ ] OK    Fix: 

## `help.tuning`

- **`body`**
  - EN: System tuning for WFS is different from standard PA tuning. It can proceed as follows:\n- Start with the flown array muted. Set the desired levels for the near-field speakers when listening to them in the first row. Adjust the high frequency shelf attenuation so the near-field speakers are not too bright and draw too much attention.\n- Mute the near-field array and unmute the flown array, find a suitable level towards the back of the house.\n- Unmute both arrays, adjust the delay of the flown array to bring the sound down to the correct height in the lower rows. Adjust the levels, HF shelf/distance ratio and vertical and horizontal parallax for each array to have consistent level wherever your inputs are on stage. This is especially important if your inputs are actors, singers or acoustic music instruments on stage. You may also test adding some Haas effect to delay the whole system if you feel the WFS is too early or adding comb filtering with the acoustic sound.\nYou may follow a different workflow for tuning or go for different cues.
  - DE: Die Systemabstimmung für WFS unterscheidet sich von der Standard-PA-Abstimmung. Sie kann wie folgt vorgehen:\n- Beginnen Sie mit stummgeschaltetem Hänge-Array. Stellen Sie die gewünschten Pegel für die Nahfeld-Lautsprecher ein, wenn Sie sie in der ersten Reihe hören. Passen Sie die HF-Shelf-Dämpfung an, damit die Nahfeld-Lautsprecher nicht zu brilliant sind.\n- Schalten Sie das Nahfeld-Array stumm und das Hänge-Array ein, finden Sie einen geeigneten Pegel zum Ende des Saals hin.\n- Aktivieren Sie beide Arrays, passen Sie die Verzögerung des Hänge-Arrays an. Passen Sie Pegel, HF-Shelf/Entfernungsverhältnis und vertikale und horizontale Parallaxe für jedes Array an, um einen konsistenten Pegel zu erreichen, wo auch immer Ihre Eingänge auf der Bühne sind.\nSie können einen anderen Workflow für die Abstimmung verfolgen oder für verschiedene Situationen unterschiedliche Einstellungen wählen.
  - [ ] OK    Fix: 

- **`title`**
  - EN: System Tuning
  - DE: Systemabstimmung
  - [ ] OK    Fix: 

## `inputs.dialogs`

- **`exportConfig`**
  - EN: Export Input Configuration
  - DE: Eingangskonfiguration exportieren
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Input Configuration
  - DE: Eingangskonfiguration importieren
  - [ ] OK    Fix: 

- **`selectChannel`**
  - EN: Select Channel
  - DE: Kanal auswählen
  - [ ] OK    Fix: 

- **`snapshotNameLabel`**
  - EN: Name:
  - DE: Name:
  - [ ] OK    Fix: 

- **`storeSnapshotMessage`**
  - EN: Enter a name for the new snapshot:
  - DE: Geben Sie einen Namen für den neuen Snapshot ein:
  - [ ] OK    Fix: 

- **`storeSnapshotTitle`**
  - EN: Store New Snapshot
  - DE: Neuen Snapshot speichern
  - [ ] OK    Fix: 

- **`trackingConflictCluster`**
  - EN: Input {current} has tracking enabled, but Input {existing} in Cluster {cluster} is already tracked.\n\nOnly one tracked input per cluster is allowed.
  - DE: Eingang {current} hat Tracking aktiviert, aber Eingang {existing} in Cluster {cluster} wird bereits getrackt.\n\nNur ein getrackter Eingang pro Cluster ist erlaubt.
  - [ ] OK    Fix: 

- **`trackingConflictContinue`**
  - EN: Continue (disable tracking)
  - DE: Fortfahren (Tracking deaktivieren)
  - [ ] OK    Fix: 

- **`trackingConflictSwitch`**
  - EN: Input {existing} in Cluster {cluster} already has tracking enabled.\n\nOnly one tracked input per cluster is allowed.\n\nDo you want to disable tracking on Input {existing} and enable it on Input {to}?
  - DE: Eingang {existing} in Cluster {cluster} hat bereits Tracking aktiviert.\n\nNur ein getrackter Eingang pro Cluster ist erlaubt.\n\nMöchten Sie das Tracking auf Eingang {existing} deaktivieren und auf Eingang {to} aktivieren?
  - [ ] OK    Fix: 

- **`trackingConflictTitle`**
  - EN: Tracking Conflict
  - DE: Tracking-Konflikt
  - [ ] OK    Fix: 

- **`trackingConflictYes`**
  - EN: Yes, switch tracking
  - DE: Ja, Tracking übertragen
  - [ ] OK    Fix: 

## `inputs.gradientMap`

- **`help.blackValue`**
  - EN: Parameter value mapped to black (0.00–1.00)
  - DE: Parameterwert für Schwarz (0.00–1.00)
  - [ ] OK    Fix: 

- **`help.blur`**
  - EN: Edge blur in meters
  - DE: Kantenunschärfe in Metern
  - [ ] OK    Fix: 

- **`help.copy`**
  - EN: Copy selected shape or layer
  - DE: Ausgewählte Form oder Ebene kopieren
  - [ ] OK    Fix: 

- **`help.curve`**
  - EN: Gamma curve (-1 to 1, 0 = linear)
  - DE: Gammakurve (-1 bis 1, 0 = linear)
  - [ ] OK    Fix: 

- **`help.ellipseTool`**
  - EN: Draw ellipse
  - DE: Ellipse zeichnen
  - [ ] OK    Fix: 

- **`help.fillTool`**
  - EN: Apply uniform fill to shape
  - DE: Gleichmäßige Füllung auf Form anwenden
  - [ ] OK    Fix: 

- **`help.fillValue`**
  - EN: Fill brightness (0 = black, 1 = white)
  - DE: Füllhelligkeit (0 = Schwarz, 1 = Weiß)
  - [ ] OK    Fix: 

- **`help.layerEnable`**
  - EN: Enable/disable layer (affects output and OSC)
  - DE: Ebene aktivieren/deaktivieren (beeinflusst Ausgabe und OSC)
  - [ ] OK    Fix: 

- **`help.layerSelect`**
  - EN: Select this layer for editing
  - DE: Diese Ebene zur Bearbeitung auswählen
  - [ ] OK    Fix: 

- **`help.layerVisible`**
  - EN: Show/hide layer preview on canvas
  - DE: Ebenenvorschau auf der Leinwand ein-/ausblenden
  - [ ] OK    Fix: 

- **`help.linGradTool`**
  - EN: Apply linear gradient to shape
  - DE: Linearen Verlauf auf Form anwenden
  - [ ] OK    Fix: 

- **`help.paste`**
  - EN: Paste shape or layer from clipboard
  - DE: Form oder Ebene aus Zwischenablage einfügen
  - [ ] OK    Fix: 

- **`help.polygonTool`**
  - EN: Draw polygon (double-click to close)
  - DE: Polygon zeichnen (Doppelklick zum Schließen)
  - [ ] OK    Fix: 

- **`help.radGradTool`**
  - EN: Apply radial gradient to shape
  - DE: Radialen Verlauf auf Form anwenden
  - [ ] OK    Fix: 

- **`help.rectTool`**
  - EN: Draw rectangle
  - DE: Rechteck zeichnen
  - [ ] OK    Fix: 

- **`help.selectTool`**
  - EN: Select and move shapes
  - DE: Formen auswählen und verschieben
  - [ ] OK    Fix: 

- **`help.shapeDelete`**
  - EN: Delete selected shape(s)
  - DE: Ausgewählte Form(en) löschen
  - [ ] OK    Fix: 

- **`help.shapeEnable`**
  - EN: Enable/disable shape
  - DE: Form aktivieren/deaktivieren
  - [ ] OK    Fix: 

- **`help.shapeLock`**
  - EN: Lock shape position
  - DE: Formposition sperren
  - [ ] OK    Fix: 

- **`help.whiteValue`**
  - EN: Parameter value mapped to white (0.00–1.00)
  - DE: Parameterwert für Weiß (0.00–1.00)
  - [ ] OK    Fix: 

## `inputs.help`

- **`admMapping`**
  - EN: Assign this input to an ADM-OSC mapping for position receive/transmit.
  - DE: Diesen Eingang einem ADM-OSC-Mapping für Positionsempfang/-sendung zuweisen.
  - [ ] OK    Fix: 

- **`arrayAttenDial`**
  - EN: Attenuation for Array {num} (-60 to 0 dB).
  - DE: Dämpfung für Array {num} (-60 bis 0 dB).
  - [ ] OK    Fix: 

- **`attenuationLawButton`**
  - EN: Attenuation Law Model (Linear Decrease of Volume with Distance Between Object and Speaker or Squared).
  - DE: Attenuation Law-Modell (lineare Lautstärkeabnahme mit dem Abstand zwischen Objekt und Lautsprecher oder quadratisch).
  - [ ] OK    Fix: 

- **`attenuationSlider`**
  - EN: Input Channel Attenuation.
  - DE: Eingangskanal-Dämpfung.
  - [ ] OK    Fix: 

- **`channelSelector`**
  - EN: Input Channel Number and Selection.
  - DE: Nummer und Auswahl des Eingangskanals.
  - [ ] OK    Fix: 

- **`clusterSelector`**
  - EN: Object is Part of a Cluster.
  - DE: Objekt ist Teil eines Clusters.
  - [ ] OK    Fix: 

- **`commonAttenDial`**
  - EN: Percentage of the Common Part of the Attenuation for selected Object Relative to All Outputs.
  - DE: Prozentsatz des gemeinsamen Anteils der Dämpfung für das ausgewählte Objekt relativ zu allen Ausgängen.
  - [ ] OK    Fix: 

- **`constraintDistanceButton`**
  - EN: Limit Position to Distance Range from Origin (for Cylindrical/Spherical modes).
  - DE: Position auf einen Distanzbereich vom Ursprung begrenzen (für zylindrische/sphärische Modi).
  - [ ] OK    Fix: 

- **`constraintXButton`**
  - EN: Limit Position to the Bounds of the Stage in Width.
  - DE: Position auf die Bühnenbreite begrenzen.
  - [ ] OK    Fix: 

- **`constraintYButton`**
  - EN: Limit Position to the Bounds of the Stage in Depth.
  - DE: Position auf die Bühnentiefe begrenzen.
  - [ ] OK    Fix: 

- **`constraintZButton`**
  - EN: Limit Position to the Bounds of the Stage in Height.
  - DE: Position auf die Bühnenhöhe begrenzen.
  - [ ] OK    Fix: 

- **`coordModeSelector`**
  - EN: Coordinate display mode: Cartesian (X/Y/Z), Cylindrical (radius/azimuth/height), or Spherical (radius/azimuth/elevation).
  - DE: Koordinatenanzeige-Modus: Kartesisch (X/Y/Z), zylindrisch (Radius/Azimut/Höhe) oder sphärisch (Radius/Azimut/Elevation).
  - [ ] OK    Fix: 

- **`delayLatencySlider`**
  - EN: Input Channel Delay (positive values) or Latency Compensation (negative values).
  - DE: Eingangskanal-Verzögerung (positive Werte) oder Latenzkompensation (negative Werte).
  - [ ] OK    Fix: 

- **`deleteSnapshot`**
  - EN: Delete Selected Input Snapshot With Confirmation.
  - DE: Ausgewählten Eingangs-Snapshot mit Bestätigung löschen.
  - [ ] OK    Fix: 

- **`directivitySlider`**
  - EN: How Wide is the Brightness of The Object.
  - DE: Wie weit der Helligkeitskegel des Objekts ist.
  - [ ] OK    Fix: 

- **`distanceAttenDial`**
  - EN: Attenuation per Meter Between Object and Speaker.
  - DE: Dämpfung pro Meter zwischen Objekt und Lautsprecher.
  - [ ] OK    Fix: 

- **`distanceMaxEditor`**
  - EN: Maximum Distance from Origin in Meters.
  - DE: Maximale Distanz vom Ursprung in Metern.
  - [ ] OK    Fix: 

- **`distanceMinEditor`**
  - EN: Minimum Distance from Origin in Meters.
  - DE: Minimale Distanz vom Ursprung in Metern.
  - [ ] OK    Fix: 

- **`distanceRangeSlider`**
  - EN: Set Minimum and Maximum Distance from Origin.
  - DE: Minimale und maximale Distanz vom Ursprung festlegen.
  - [ ] OK    Fix: 

- **`distanceRatioDial`**
  - EN: Attenuation Ratio for Squared Model.
  - DE: Dämpfungsverhältnis für das quadratische Modell.
  - [ ] OK    Fix: 

- **`editScope`**
  - EN: Open Selected Input Snapshot Scope Window.
  - DE: Filter-Fenster des ausgewählten Eingangs-Snapshots öffnen.
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Input Configuration to file (with file explorer window).
  - DE: Eingangskonfiguration in Datei exportieren (mit Datei-Explorer).
  - [ ] OK    Fix: 

- **`flipXButton`**
  - EN: X will be Symetrical to the Origin. Keyboard Nudging will be Inverted.
  - DE: X wird symmetrisch zum Ursprung. Tastatur-Nudging wird invertiert.
  - [ ] OK    Fix: 

- **`flipYButton`**
  - EN: Y will be Symetrical to the Origin. Keyboard Nudging will be Inverted.
  - DE: Y wird symmetrisch zum Ursprung. Tastatur-Nudging wird invertiert.
  - [ ] OK    Fix: 

- **`flipZButton`**
  - EN: Z will be Symetrical to the Origin. Keyboard Nudging will be Inverted.
  - DE: Z wird symmetrisch zum Ursprung. Tastatur-Nudging wird invertiert.
  - [ ] OK    Fix: 

- **`frActiveButton`**
  - EN: Enable Simulated Floor Reflections for the Object.
  - DE: Simulierte Floor Reflections für das Objekt aktivieren.
  - [ ] OK    Fix: 

- **`frAttenuationSlider`**
  - EN: Attenuation of the Simulated Floor Reflections for the Object.
  - DE: Dämpfung der simulierten Floor Reflections für das Objekt.
  - [ ] OK    Fix: 

- **`frDiffusionDial`**
  - EN: Diffusion Effect of the Simulated Floor Reflections for the Object.
  - DE: Diffusionseffekt der simulierten Floor Reflections für das Objekt.
  - [ ] OK    Fix: 

- **`frHighShelfActiveButton`**
  - EN: Enable High Shelf Filter for Floor Reflections.
  - DE: High Shelf-Filter für Floor Reflections aktivieren.
  - [ ] OK    Fix: 

- **`frHighShelfFreqSlider`**
  - EN: High Shelf Frequency for Floor Reflections.
  - DE: High Shelf-Frequenz für Floor Reflections.
  - [ ] OK    Fix: 

- **`frHighShelfGainSlider`**
  - EN: High Shelf Gain for Floor Reflections.
  - DE: High Shelf-Gain für Floor Reflections.
  - [ ] OK    Fix: 

- **`frHighShelfSlopeSlider`**
  - EN: High Shelf Slope for Floor Reflections.
  - DE: High Shelf-Steilheit für Floor Reflections.
  - [ ] OK    Fix: 

- **`frLowCutActiveButton`**
  - EN: Enable Low Cut Filter for Floor Reflections.
  - DE: Low Cut-Filter für Floor Reflections aktivieren.
  - [ ] OK    Fix: 

- **`frLowCutFreqSlider`**
  - EN: Low Cut Frequency for Floor Reflections.
  - DE: Low Cut-Frequenz für Floor Reflections.
  - [ ] OK    Fix: 

- **`heightFactorDial`**
  - EN: Take Elevation of Object into Account Fully, Partially or Not.
  - DE: Höhe des Objekts vollständig, teilweise oder gar nicht berücksichtigen.
  - [ ] OK    Fix: 

- **`hfShelfSlider`**
  - EN: How Much Brightness is lost in the Back of the Object, Out of its Brightness Cone.
  - DE: Wie viel Höhenenergie hinter dem Objekt verloren geht, ausserhalb seines Helligkeitskegels.
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Input Configuration from file (with file explorer window).
  - DE: Eingangskonfiguration aus Datei importieren (mit Datei-Explorer).
  - [ ] OK    Fix: 

- **`inputDirectivityDial`**
  - EN: Where is the Object pointing to in the Horizontal Plane.
  - DE: Ausrichtung des Objekts in der Horizontalen.
  - [ ] OK    Fix: 

- **`jitterSlider`**
  - EN: Sphere of Rapid Movements of the Object.
  - DE: Sphäre der schnellen Bewegungen des Objekts.
  - [ ] OK    Fix: 

- **`lfoActiveButton`**
  - EN: Enable or Disable the Periodic Movement of the Object (LFO).
  - DE: Periodische Bewegung des Objekts (LFO) aktivieren oder deaktivieren.
  - [ ] OK    Fix: 

- **`lfoAmplitudeXSlider`**
  - EN: Width of Movement in Relation to Base Position of the Object.
  - DE: Breite der Bewegung im Verhältnis zur Basisposition des Objekts.
  - [ ] OK    Fix: 

- **`lfoAmplitudeYSlider`**
  - EN: Depth of Movement in Relation to Base Position of the Object.
  - DE: Tiefe der Bewegung im Verhältnis zur Basisposition des Objekts.
  - [ ] OK    Fix: 

- **`lfoAmplitudeZSlider`**
  - EN: Height of Movement in Relation to Base Position of the Object.
  - DE: Höhe der Bewegung im Verhältnis zur Basisposition des Objekts.
  - [ ] OK    Fix: 

- **`lfoGyrophoneSelector`**
  - EN: Rotation of the Brightness Cone of the Object.
  - DE: Rotation des Helligkeitskegels des Objekts.
  - [ ] OK    Fix: 

- **`lfoPeriodDial`**
  - EN: Base Period of the Movement of the Object.
  - DE: Grundperiode der Bewegung des Objekts.
  - [ ] OK    Fix: 

- **`lfoPhaseDial`**
  - EN: Phase Offset of the Movement of the Object.
  - DE: Phasenversatz der Bewegung des Objekts.
  - [ ] OK    Fix: 

- **`lfoPhaseXDial`**
  - EN: Phase Offset of the Movement of the Object in Width.
  - DE: Phasenversatz der Bewegung des Objekts in der Breite.
  - [ ] OK    Fix: 

- **`lfoPhaseYDial`**
  - EN: Phase Offset of the Movement of the Object in Depth.
  - DE: Phasenversatz der Bewegung des Objekts in der Tiefe.
  - [ ] OK    Fix: 

- **`lfoPhaseZDial`**
  - EN: Phase Offset of the Movement of the Object in Height.
  - DE: Phasenversatz der Bewegung des Objekts in der Höhe.
  - [ ] OK    Fix: 

- **`lfoRateXSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Width.
  - DE: Schnellere oder langsamere Bewegung im Verhältnis zur Grundperiode in der Breite.
  - [ ] OK    Fix: 

- **`lfoRateYSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Depth.
  - DE: Schnellere oder langsamere Bewegung im Verhältnis zur Grundperiode in der Tiefe.
  - [ ] OK    Fix: 

- **`lfoRateZSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Height.
  - DE: Schnellere oder langsamere Bewegung im Verhältnis zur Grundperiode in der Höhe.
  - [ ] OK    Fix: 

- **`lfoShapeXSelector`**
  - EN: Movement Behaviour of the Object in Width.
  - DE: Bewegungsverhalten des Objekts in der Breite.
  - [ ] OK    Fix: 

- **`lfoShapeYSelector`**
  - EN: Movement Behaviour of the Object in Depth.
  - DE: Bewegungsverhalten des Objekts in der Tiefe.
  - [ ] OK    Fix: 

- **`lfoShapeZSelector`**
  - EN: Movement Behaviour of the Object in Height.
  - DE: Bewegungsverhalten des Objekts in der Höhe.
  - [ ] OK    Fix: 

- **`lsActiveButton`**
  - EN: If You Need to Reduce the Level in Speakers Close to the Object. (eg. Loud Source Present on Stage)
  - DE: Wenn der Pegel in Lautsprechern nahe am Objekt reduziert werden muss (z. B. laute Quelle auf der Bühne).
  - [ ] OK    Fix: 

- **`lsAttenuationSlider`**
  - EN: Constant Attenuation of Speakers Around the Object.
  - DE: Konstante Dämpfung der Lautsprecher um das Objekt herum.
  - [ ] OK    Fix: 

- **`lsPeakEnableButton`**
  - EN: Enable or Disable the Fast (Peak) Compressor for the Live Source Tamer.
  - DE: Schnellen (Peak-) Kompressor des Live Source Tamers aktivieren oder deaktivieren.
  - [ ] OK    Fix: 

- **`lsPeakRatioDial`**
  - EN: Ratio to Apply the Fast Compression for Speakers Around the Object.
  - DE: Ratio für die schnelle Kompression der Lautsprecher um das Objekt herum.
  - [ ] OK    Fix: 

- **`lsPeakThresholdSlider`**
  - EN: Fast Compression Threshold for Speakers Around the Object to Control Transients.
  - DE: Schwellwert der schnellen Kompression für Lautsprecher um das Objekt herum, zur Kontrolle der Transienten.
  - [ ] OK    Fix: 

- **`lsRadiusSlider`**
  - EN: How Far does the Attenuation Affect The Speakers.
  - DE: Wie weit die Dämpfung die Lautsprecher beeinflusst.
  - [ ] OK    Fix: 

- **`lsShapeSelector`**
  - EN: Profile of the Attenuation Around the Object.
  - DE: Profil der Dämpfung um das Objekt herum.
  - [ ] OK    Fix: 

- **`lsSlowEnableButton`**
  - EN: Enable or Disable the Slow Compressor for the Live Source Tamer.
  - DE: Langsamen Kompressor des Live Source Tamers aktivieren oder deaktivieren.
  - [ ] OK    Fix: 

- **`lsSlowRatioDial`**
  - EN: Ratio to Apply the Slow Compression for Speakers Around the Object.
  - DE: Ratio für die langsame Kompression der Lautsprecher um das Objekt herum.
  - [ ] OK    Fix: 

- **`lsSlowThresholdSlider`**
  - EN: Slow Compression Threshold for Speakers Around the Object to Control Sustained Level.
  - DE: Schwellwert der langsamen Kompression für Lautsprecher um das Objekt herum, zur Kontrolle des Dauerpegels.
  - [ ] OK    Fix: 

- **`mapLockButton`**
  - EN: Prevent Interaction on the Map Tab
  - DE: Interaktion auf der Karten-Registerkarte verhindern.
  - [ ] OK    Fix: 

- **`mapVisibilityButton`**
  - EN: Make Visible or Hide The Selected Input on the Map
  - DE: Den ausgewählten Eingang auf der Karte anzeigen oder ausblenden.
  - [ ] OK    Fix: 

- **`maxSpeedActiveButton`**
  - EN: Enable or Disable Speed Limiting for Object.
  - DE: Geschwindigkeitsbegrenzung für das Objekt aktivieren oder deaktivieren.
  - [ ] OK    Fix: 

- **`maxSpeedDial`**
  - EN: Maximum Speed Limit for Object.
  - DE: Maximale Geschwindigkeitsgrenze für das Objekt.
  - [ ] OK    Fix: 

- **`minimalLatencyButton`**
  - EN: Select between Acoustic Precedence and Minimal Latency for Amplification Precedence.
  - DE: Wahl zwischen Acoustic Precedence und Minimal Latency für die Verstärkungspräzedenz.
  - [ ] OK    Fix: 

- **`muteButton`**
  - EN: Mute Output {num} for this Object.
  - DE: Ausgang {num} für dieses Objekt stummschalten.
  - [ ] OK    Fix: 

- **`muteMacrosSelector`**
  - EN: Mute Macros for Fast Muting and Unmuting of Arrays.
  - DE: Mute Macros zum schnellen Stummschalten und Aufheben für Arrays.
  - [ ] OK    Fix: 

- **`muteReverbSendsButton`**
  - EN: Mute sends from this input to all reverb channels.
  - DE: Sends von diesem Eingang zu allen Hallkanälen stummschalten.
  - [ ] OK    Fix: 

- **`nameEditor`**
  - EN: Displayed Input Channel Name (editable).
  - DE: Angezeigter Eingangskanal-Name (bearbeitbar).
  - [ ] OK    Fix: 

- **`nudgeHint1`**
  - EN:  Nudge with Left and Right Arrow Keys.
  - DE:  Mit Links- und Rechts-Pfeiltasten anpassen.
  - [ ] OK    Fix: 

- **`nudgeHint2`**
  - EN:  Nudge with Up and Down Arrow Keys.
  - DE:  Mit Auf- und Ab-Pfeiltasten anpassen.
  - [ ] OK    Fix: 

- **`nudgeHint3`**
  - EN:  Nudge with Page Up and Page Down Keys.
  - DE:  Mit Bild-Auf und Bild-Ab anpassen.
  - [ ] OK    Fix: 

- **`offset1`**
  - EN: Object Offset {name} ({unit}). Adjusted when Tracking is Enabled.
  - DE: Objekt-Versatz {name} ({unit}). Wird bei aktiviertem Tracking angepasst.
  - [ ] OK    Fix: 

- **`offset2`**
  - EN: Object Offset {name} ({unit}). Adjusted when Tracking is Enabled.
  - DE: Objekt-Versatz {name} ({unit}). Wird bei aktiviertem Tracking angepasst.
  - [ ] OK    Fix: 

- **`offset3`**
  - EN: Object Offset {name} ({unit}). Adjusted when Tracking is Enabled.
  - DE: Objekt-Versatz {name} ({unit}). Wird bei aktiviertem Tracking angepasst.
  - [ ] OK    Fix: 

- **`otomoAbsRelButton`**
  - EN: Select Relative or Absolute Coordinates of Displacement.
  - DE: Wahl zwischen relativen oder absoluten Verschiebungskoordinaten.
  - [ ] OK    Fix: 

- **`otomoCoordMode`**
  - EN: Coordinate display mode for AutomOtion destinations: Cartesian (X/Y/Z), Cylindrical (r/θ/Z), or Spherical (r/θ/φ).
  - DE: Koordinatenmodus für AutomOtion-Ziele: Kartesisch (X/Y/Z), Zylindrisch (r/θ/Z) oder Sphärisch (r/θ/φ).
  - [ ] OK    Fix: 

- **`otomoCurveDial`**
  - EN: Bend the Path to the Left (Negative) or Right (Positive) of the Direction of Travel.
  - DE: Pfad nach links (negativ) oder rechts (positiv) der Bewegungsrichtung biegen.
  - [ ] OK    Fix: 

- **`otomoDest1`**
  - EN: Relative or Absolute Destination {name} ({unit}).
  - DE: Relatives oder absolutes Ziel {name} ({unit}).
  - [ ] OK    Fix: 

- **`otomoDest2`**
  - EN: Relative or Absolute Destination {name} ({unit}).
  - DE: Relatives oder absolutes Ziel {name} ({unit}).
  - [ ] OK    Fix: 

- **`otomoDest3`**
  - EN: Relative or Absolute Destination {name} ({unit}).
  - DE: Relatives oder absolutes Ziel {name} ({unit}).
  - [ ] OK    Fix: 

- **`otomoDurationDial`**
  - EN: Duration of the Movement in Seconds (0.1s to 1 hour).
  - DE: Dauer der Bewegung in Sekunden (0,1 s bis 1 Stunde).
  - [ ] OK    Fix: 

- **`otomoPauseButton`**
  - EN: Pause and Resume the Movement.
  - DE: Bewegung pausieren und fortsetzen.
  - [ ] OK    Fix: 

- **`otomoPauseResumeAllButton`**
  - EN: Pause or Resume All Active Movements Globally.
  - DE: Alle aktiven Bewegungen global pausieren oder fortsetzen.
  - [ ] OK    Fix: 

- **`otomoResetDial`**
  - EN: Set the Reset Level for the Automatic Trigger.
  - DE: Reset-Pegel für die automatische Auslösung festlegen.
  - [ ] OK    Fix: 

- **`otomoSpeedProfileDial`**
  - EN: Constant Speed or Gradual Acceleration and Slow Down at the Start and the End of the Movement.
  - DE: Konstante Geschwindigkeit oder graduelle Beschleunigung und Verzögerung am Anfang und Ende der Bewegung.
  - [ ] OK    Fix: 

- **`otomoStartButton`**
  - EN: Start the Movement Manually.
  - DE: Bewegung manuell starten.
  - [ ] OK    Fix: 

- **`otomoStayReturnButton`**
  - EN: At the End of the Movement, should the Source Stay or Return to the Original Position.
  - DE: Am Ende der Bewegung soll die Quelle bleiben oder zur Ausgangsposition zurückkehren.
  - [ ] OK    Fix: 

- **`otomoStopAllButton`**
  - EN: Stop All Active Movements Globally.
  - DE: Alle aktiven Bewegungen global anhalten.
  - [ ] OK    Fix: 

- **`otomoStopButton`**
  - EN: Stop the Movement.
  - DE: Bewegung anhalten.
  - [ ] OK    Fix: 

- **`otomoThresholdDial`**
  - EN: Set the Threshold for the Automatic Trigger of the Movement.
  - DE: Schwellwert für die automatische Auslösung der Bewegung festlegen.
  - [ ] OK    Fix: 

- **`otomoTriggerButton`**
  - EN: Manual Start of Displacement or Automatic Trigger on the Audio Level.
  - DE: Manueller Start der Verschiebung oder automatische Auslösung anhand des Audiopegels.
  - [ ] OK    Fix: 

- **`pathModeButton`**
  - EN: Enable Path Mode to Follow Drawn Movement Paths Instead of Direct Lines.
  - DE: Pfadmodus aktivieren, um gezeichneten Bewegungspfaden zu folgen statt direkter Linien.
  - [ ] OK    Fix: 

- **`position1`**
  - EN: Object {name} ({unit}).
  - DE: Objekt {name} ({unit}).
  - [ ] OK    Fix: 

- **`position2`**
  - EN: Object {name} ({unit}).
  - DE: Objekt {name} ({unit}).
  - [ ] OK    Fix: 

- **`position3`**
  - EN: Object {name} ({unit}).
  - DE: Objekt {name} ({unit}).
  - [ ] OK    Fix: 

- **`positionJoystick`**
  - EN: Drag to adjust X/Y position in real-time. Returns to center on release.
  - DE: Ziehen, um die X/Y-Position in Echtzeit anzupassen. Kehrt beim Loslassen zur Mitte zurück.
  - [ ] OK    Fix: 

- **`positionZSlider`**
  - EN: Drag to adjust Z (height) position in real-time. Returns to center on release.
  - DE: Ziehen, um die Z-Position (Höhe) in Echtzeit anzupassen. Kehrt beim Loslassen zur Mitte zurück.
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Input Configuration from backup file.
  - DE: Eingangskonfiguration aus Sicherungsdatei neu laden.
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Input Configuration from file.
  - DE: Eingangskonfiguration aus Datei neu laden.
  - [ ] OK    Fix: 

- **`reloadSnapshot`**
  - EN: Reload Selected Input Snapshot for All Objects Taking the Scope into Account.
  - DE: Ausgewählten Eingangs-Snapshot für alle Objekte unter Berücksichtigung des Filters neu laden.
  - [ ] OK    Fix: 

- **`reloadWithoutScope`**
  - EN: Reload Selected Input Snapshot for All Objects Without the Scope.
  - DE: Ausgewählten Eingangs-Snapshot für alle Objekte ohne Filter neu laden.
  - [ ] OK    Fix: 

- **`sidelinesActiveButton`**
  - EN: Enable Automatic Muting when Source Approaches Stage Edges. Does Not Apply to Downstage (Front) Edge.
  - DE: Automatische Stummschaltung aktivieren, wenn die Quelle sich den Bühnenrändern nähert. Gilt nicht für die vordere (Publikums-) Kante.
  - [ ] OK    Fix: 

- **`sidelinesFringeDial`**
  - EN: Fringe Zone Size in Meters. Outer Half is Full Mute, Inner Half Fades Linearly.
  - DE: Übergangszonengrösse in Metern. Die äussere Hälfte schaltet vollständig stumm, die innere Hälfte blendet linear.
  - [ ] OK    Fix: 

- **`snapshotSelector`**
  - EN: Select Input Snapshot Without Loading.
  - DE: Eingangs-Snapshot auswählen ohne zu laden.
  - [ ] OK    Fix: 

- **`solo`**
  - EN: Listen to Binaural Rendering of this channel.
  - DE: Binaurales Rendering dieses Kanals abhören.
  - [ ] OK    Fix: 

- **`soloMode`**
  - EN: Single: one input at a time. Multi: multiple inputs simultaneously.
  - DE: Einzeln: ein Eingang nach dem anderen. Mehrfach: mehrere Eingänge gleichzeitig.
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Input Configuration to file (with backup).
  - DE: Eingangskonfiguration in Datei speichern (mit Sicherung).
  - [ ] OK    Fix: 

- **`storeSnapshot`**
  - EN: Store new Input Snapshot for All Objects.
  - DE: Neuen Eingangs-Snapshot für alle Objekte speichern.
  - [ ] OK    Fix: 

- **`tiltSlider`**
  - EN: Where is the Object pointing to in the Vertical Plane.
  - DE: Ausrichtung des Objekts in der Vertikalen.
  - [ ] OK    Fix: 

- **`trackingActiveButton`**
  - EN: Enable or Disable Tracking for Object.
  - DE: Tracking für das Objekt aktivieren oder deaktivieren.
  - [ ] OK    Fix: 

- **`trackingIdSelector`**
  - EN: Tracker ID for Object.
  - DE: Tracker-ID für das Objekt.
  - [ ] OK    Fix: 

- **`trackingSmoothDial`**
  - EN: Smoothing of Tracking Data for Object.
  - DE: Glättung der Trackingdaten für das Objekt.
  - [ ] OK    Fix: 

- **`updateSnapshot`**
  - EN: Update Selected Input Snapshot (with backup).
  - DE: Ausgewählten Eingangs-Snapshot aktualisieren (mit Sicherung).
  - [ ] OK    Fix: 

## `inputs.messages`

- **`assignedCluster`**
  - EN: Input {channel} assigned to Cluster {cluster}
  - DE: Eingang {channel} zu Cluster {cluster} zugewiesen
  - [ ] OK    Fix: 

- **`backupLoaded`**
  - EN: Input configuration loaded from backup.
  - DE: Eingangskonfiguration aus Backup geladen.
  - [ ] OK    Fix: 

- **`clusterEditAbsolute`**
  - EN: Ctrl+Shift edit: copying value to {count} other input(s) of Cluster {cluster}
  - DE: Strg+Shift-Bearbeitung: Wert auf {count} weitere Eingänge von Cluster {cluster} kopiert
  - [ ] OK    Fix: 

- **`clusterEditRelative`**
  - EN: Shift edit: applying relative change to {count} other input(s) of Cluster {cluster}
  - DE: Shift-Bearbeitung: relative Änderung auf {count} weitere Eingänge von Cluster {cluster} angewendet
  - [ ] OK    Fix: 

- **`configExported`**
  - EN: Input configuration exported.
  - DE: Eingangskonfiguration exportiert.
  - [ ] OK    Fix: 

- **`configImported`**
  - EN: Input configuration imported.
  - DE: Eingangskonfiguration importiert.
  - [ ] OK    Fix: 

- **`configLoaded`**
  - EN: Input configuration loaded.
  - DE: Eingangskonfiguration geladen.
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Input configuration saved.
  - DE: Eingangskonfiguration gespeichert.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - DE: Fehler: {error}
  - [ ] OK    Fix: 

- **`noSnapshotSelected`**
  - EN: No snapshot selected.
  - DE: Kein Snapshot ausgewählt.
  - [ ] OK    Fix: 

- **`scopeConfigured`**
  - EN: Scope configured for next snapshot.
  - DE: Umfang für nächsten Snapshot konfiguriert.
  - [ ] OK    Fix: 

- **`scopeSaved`**
  - EN: Snapshot scope saved.
  - DE: Snapshot-Umfang gespeichert.
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - DE: Bitte wählen Sie zuerst einen Projektordner in der System Config.
  - [ ] OK    Fix: 

- **`setSingle`**
  - EN: Input {channel} set to Single
  - DE: Eingang {channel} auf Einzeln gesetzt
  - [ ] OK    Fix: 

- **`snapshotDeleted`**
  - EN: Snapshot '{name}' deleted.
  - DE: Snapshot '{name}' gelöscht.
  - [ ] OK    Fix: 

- **`snapshotLoaded`**
  - EN: Snapshot '{name}' loaded.
  - DE: Snapshot '{name}' geladen.
  - [ ] OK    Fix: 

- **`snapshotLoadedWithoutScope`**
  - EN: Snapshot '{name}' loaded (without scope).
  - DE: Snapshot '{name}' geladen (ohne Umfang).
  - [ ] OK    Fix: 

- **`snapshotScopeUpdated`**
  - EN: Snapshot '{name}' scope updated.
  - DE: Umfang von Snapshot '{name}' aktualisiert.
  - [ ] OK    Fix: 

- **`snapshotStored`**
  - EN: Snapshot '{name}' stored.
  - DE: Snapshot '{name}' gespeichert.
  - [ ] OK    Fix: 

- **`snapshotUpdated`**
  - EN: Snapshot '{name}' updated.
  - DE: Snapshot '{name}' aktualisiert.
  - [ ] OK    Fix: 

- **`trackingDisabled`**
  - EN: Tracking disabled for Input {channel}
  - DE: Tracking für Eingang {channel} deaktiviert
  - [ ] OK    Fix: 

- **`trackingSwitched`**
  - EN: Tracking switched from Input {from} to Input {to}
  - DE: Tracking von Eingang {from} zu Eingang {to} gewechselt
  - [ ] OK    Fix: 

## `levelMeter.tooltips`

- **`clearSolo`**
  - EN: Disengage all solo toggles
  - DE: Alle Solos deaktivieren
  - [ ] OK    Fix: 

- **`solo`**
  - EN: Display the contribution of the input to all outputs in the Level Meter display (in Single mode) and play binaural render of soloed inputs
  - DE: Beitrag des Eingangs zu allen Ausgängen in der Pegelanzeige anzeigen (im Einzelmodus) und binaurale Wiedergabe der Solo-Eingänge
  - [ ] OK    Fix: 

- **`soloMode`**
  - EN: Single: one input at a time. Multi: multiple inputs simultaneously.
  - DE: Einzeln: ein Eingang nach dem anderen. Mehrfach: mehrere Eingänge gleichzeitig.
  - [ ] OK    Fix: 

## `map.messages`

- **`assignedCluster`**
  - EN: Input {channel} assigned to Cluster {cluster}
  - DE: Eingang {channel} zu Cluster {cluster} zugewiesen
  - [ ] OK    Fix: 

- **`assignedClusterMulti`**
  - EN: {count} inputs assigned to Cluster {cluster}
  - DE: {count} Eingänge zu Cluster {cluster} zugewiesen
  - [ ] OK    Fix: 

- **`clusterBrokenUp`**
  - EN: Cluster {cluster} broken up
  - DE: Cluster {cluster} aufgelöst
  - [ ] OK    Fix: 

- **`setSingle`**
  - EN: Input {channel} removed from cluster
  - DE: Eingang {channel} aus Cluster entfernt
  - [ ] OK    Fix: 

- **`setSingleMulti`**
  - EN: {count} inputs removed from clusters
  - DE: {count} Eingänge aus Clustern entfernt
  - [ ] OK    Fix: 

## `map.tooltips`

- **`detach`**
  - EN: Detach the map into a separate window for dual-screen setups
  - DE: Karte in ein separates Fenster lösen für Dual-Screen-Setups
  - [ ] OK    Fix: 

- **`fitInputs`**
  - EN: Adjust the zoom and panning to fit all visible inputs on the display
  - DE: Zoom und Schwenken anpassen, um alle sichtbaren Eingänge anzuzeigen
  - [ ] OK    Fix: 

- **`fitStage`**
  - EN: Adjust the zoom and panning to fit the stage on the display
  - DE: Zoom und Schwenken anpassen, um die Bühne anzuzeigen
  - [ ] OK    Fix: 

- **`levels`**
  - EN: Display the levels for inputs and outputs on the map
  - DE: Pegel für Eingänge und Ausgänge auf der Karte anzeigen
  - [ ] OK    Fix: 

## `meta`

- **`author`**
  - EN: WFS-DIY Team
  - DE: WFS-DIY Team
  - [ ] OK    Fix: 

- **`language`**
  - EN: English
  - DE: Deutsch
  - [ ] OK    Fix: 

- **`locale`**
  - EN: en
  - DE: de
  - [ ] OK    Fix: 

- **`version`**
  - EN: 1.0.0
  - DE: 1.0.0
  - [ ] OK    Fix: 

## `network.dialogs`

- **`exportConfig`**
  - EN: Export Network Configuration
  - DE: Netzwerkkonfiguration exportieren
  - [ ] OK    Fix: 

- **`findMyRemoteMessage`**
  - EN: Enter the password for your remote device:
  - DE: Passwort für die Fernbedienung eingeben:
  - [ ] OK    Fix: 

- **`findMyRemotePassword`**
  - EN: Password:
  - DE: Passwort:
  - [ ] OK    Fix: 

- **`findMyRemoteTitle`**
  - EN: Find My Remote
  - DE: Fernbedienung finden
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Network Configuration
  - DE: Netzwerkkonfiguration importieren
  - [ ] OK    Fix: 

- **`removeTargetMessage`**
  - EN: Delete target '{name}'?
  - DE: Ziel „{name}" löschen?
  - [ ] OK    Fix: 

- **`removeTargetTitle`**
  - EN: Remove Target
  - DE: Ziel entfernen
  - [ ] OK    Fix: 

- **`trackingConflictsContinue`**
  - EN: Continue
  - DE: Fortfahren
  - [ ] OK    Fix: 

- **`trackingConflictsMessage`**
  - EN: \nOnly one tracked input per cluster is allowed. If you continue, tracking will be kept only for the first input in each cluster.
  - DE: \nNur ein getrackter Eingang pro Cluster ist erlaubt. Wenn Sie fortfahren, wird das Tracking nur für den ersten Eingang in jedem Cluster beibehalten.
  - [ ] OK    Fix: 

- **`trackingConflictsTitle`**
  - EN: Tracking Conflicts Detected
  - DE: Tracking-Konflikte erkannt
  - [ ] OK    Fix: 

## `network.help`

- **`addTarget`**
  - EN: Add new network target.
  - DE: Neues Netzwerkziel hinzufügen.
  - [ ] OK    Fix: 

- **`admAxisSwap`**
  - EN: Which incoming ADM-OSC axis maps to this internal axis.
  - DE: Welche eingehende ADM-OSC-Achse auf diese interne Achse abgebildet wird.
  - [ ] OK    Fix: 

- **`admAzFlip`**
  - EN: Invert the direction of incoming azimuth.
  - DE: Richtung des eingehenden Azimuts invertieren.
  - [ ] OK    Fix: 

- **`admAzOffset`**
  - EN: Azimuth offset (deg) applied to incoming ADM-OSC azimuth.
  - DE: Azimut-Offset (Grad), der auf den eingehenden ADM-OSC-Azimut angewandt wird.
  - [ ] OK    Fix: 

- **`admBreakpoint`**
  - EN: Normalized breakpoint (0-1) for piecewise linear stretch.
  - DE: Normalisierter Breakpoint (0-1) für stückweise lineare Dehnung.
  - [ ] OK    Fix: 

- **`admCenterOffset`**
  - EN: Physical position (m) where normalized 0.0 maps to.
  - DE: Physische Position (m), auf die der normalisierte Wert 0.0 abgebildet wird.
  - [ ] OK    Fix: 

- **`admDistMax`**
  - EN: Maximum physical distance (m) at ADM-OSC distance=1.
  - DE: Maximale physische Entfernung (m) bei ADM-OSC distance=1.
  - [ ] OK    Fix: 

- **`admDistMin`**
  - EN: Minimum physical distance (m) at ADM-OSC distance=0.
  - DE: Minimale physische Entfernung (m) bei ADM-OSC distance=0.
  - [ ] OK    Fix: 

- **`admElFlip`**
  - EN: Invert the sign of incoming elevation.
  - DE: Vorzeichen der eingehenden Elevation invertieren.
  - [ ] OK    Fix: 

- **`admInnerWidth`**
  - EN: Physical extent (m) from center to breakpoint.
  - DE: Physische Ausdehnung (m) von der Mitte bis zum Breakpoint.
  - [ ] OK    Fix: 

- **`admInputAssign`**
  - EN: Assign this input to an ADM-OSC mapping for receive/transmit.
  - DE: Diesen Eingang einer ADM-OSC-Zuordnung für Empfang/Sendung zuweisen.
  - [ ] OK    Fix: 

- **`admLinkAll`**
  - EN: Select all 6 sides at once for uniform editing.
  - DE: Alle 6 Seiten gleichzeitig für einheitliche Bearbeitung auswählen.
  - [ ] OK    Fix: 

- **`admMapping`**
  - EN: Select an ADM-OSC mapping to configure. Cart = Cartesian (xyz), Polar = spherical (aed).
  - DE: ADM-OSC-Zuordnung auswählen. Cart = Kartesisch (xyz), Polar = sphärisch (aed).
  - [ ] OK    Fix: 

- **`admMappingPanel`**
  - EN: Drag dots to edit mapping. Click axis title to swap, click Flip to invert. Hold Shift to edit both sides symmetrically.
  - DE: Punkte ziehen, um die Zuordnung zu bearbeiten. Achsentitel klicken zum Tauschen, Flip klicken zum Invertieren. Shift halten, um beide Seiten symmetrisch zu bearbeiten.
  - [ ] OK    Fix: 

- **`admOuterWidth`**
  - EN: Physical extent (m) from breakpoint to ±1.
  - DE: Physische Ausdehnung (m) vom Breakpoint bis ±1.
  - [ ] OK    Fix: 

- **`admSideSelect`**
  - EN: Select sides to edit. Changes apply to all selected sides at once.
  - DE: Seiten zum Bearbeiten auswählen. Änderungen gelten für alle ausgewählten Seiten gleichzeitig.
  - [ ] OK    Fix: 

- **`admSignFlip`**
  - EN: Invert the sign of the incoming axis value.
  - DE: Vorzeichen des eingehenden Achsenwerts invertieren.
  - [ ] OK    Fix: 

- **`currentIP`**
  - EN: IP address of the Processor.
  - DE: IP-Adresse des Prozessors.
  - [ ] OK    Fix: 

- **`dataMode`**
  - EN: Select UDP or TCP data transmission.
  - DE: UDP- oder TCP-Datenübertragung auswählen.
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Network Configuration to file.
  - DE: Netzwerkkonfiguration in Datei exportieren.
  - [ ] OK    Fix: 

- **`findMyRemote`**
  - EN: Make your Remote Flash and Buzz to Find it.
  - DE: Lassen Sie Ihre Fernbedienung blinken und summen, um sie zu finden.
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Network Configuration from file.
  - DE: Netzwerkkonfiguration aus Datei importieren.
  - [ ] OK    Fix: 

- **`networkInterface`**
  - EN: Select the Network Interface.
  - DE: Network Interface auswählen.
  - [ ] OK    Fix: 

- **`openLogWindow`**
  - EN: Open Network Logging window.
  - DE: Netzwerk-Protokollfenster öffnen.
  - [ ] OK    Fix: 

- **`oscQueryEnable`**
  - EN: Enable/disable OSC Query server for automatic parameter discovery via HTTP/WebSocket.
  - DE: OSC-Query-Server für automatische Parametererkennung über HTTP/WebSocket aktivieren/deaktivieren.
  - [ ] OK    Fix: 

- **`oscQueryPort`**
  - EN: HTTP port for OSC Query discovery. Other apps can browse parameters at http://localhost:<port>/
  - DE: HTTP-Port für OSC-Query-Discovery. Andere Anwendungen können Parameter unter http://localhost:<port>/ durchsuchen.
  - [ ] OK    Fix: 

- **`oscSourceFilter`**
  - EN: Filter incoming OSC: Accept All sources or only Registered connections with Rx enabled.
  - DE: Eingehendes OSC filtern: Alle Quellen akzeptieren oder nur registrierte Verbindungen mit aktiviertem Rx.
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Select the Protocol: DISABLED, OSC, REMOTE, or ADM-OSC.
  - DE: Protokoll auswählen: DISABLED, OSC, REMOTE oder ADM-OSC.
  - [ ] OK    Fix: 

- **`psnInterface`**
  - EN: Network interface for PSN multicast reception
  - DE: Netzwerk-Schnittstelle für den PSN-Multicast-Empfang
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Network Configuration from backup file.
  - DE: Netzwerkkonfiguration aus Sicherungsdatei neu laden.
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Network Configuration from file.
  - DE: Netzwerkkonfiguration aus Datei neu laden.
  - [ ] OK    Fix: 

- **`removeTarget`**
  - EN: Delete this Network Target.
  - DE: Dieses Netzwerkziel löschen.
  - [ ] OK    Fix: 

- **`rxEnable`**
  - EN: Enable or Disable Data Reception.
  - DE: Datenempfang aktivieren oder deaktivieren.
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Network Configuration to file.
  - DE: Netzwerkkonfiguration in Datei speichern.
  - [ ] OK    Fix: 

- **`targetIP`**
  - EN: IP Address of the Target (use 127.0.0.1 for local host).
  - DE: IP-Adresse des Ziels (127.0.0.1 für den lokalen Host).
  - [ ] OK    Fix: 

- **`targetName`**
  - EN: Network Target Name.
  - DE: Name des Netzwerkziels.
  - [ ] OK    Fix: 

- **`tcpPort`**
  - EN: TCP Receive Port of the Processor.
  - DE: TCP-Empfangsport des Prozessors.
  - [ ] OK    Fix: 

- **`trackingEnabled`**
  - EN: Enable or Disable Incoming Tracking data processing.
  - DE: Verarbeitung eingehender Tracking-Daten aktivieren oder deaktivieren.
  - [ ] OK    Fix: 

- **`trackingFlipX`**
  - EN: Invert Axis of Tracking X Coordinate.
  - DE: Achse der X-Koordinate des Trackings invertieren.
  - [ ] OK    Fix: 

- **`trackingFlipY`**
  - EN: Invert Axis of Tracking Y Coordinate.
  - DE: Achse der Y-Koordinate des Trackings invertieren.
  - [ ] OK    Fix: 

- **`trackingFlipZ`**
  - EN: Invert Axis of Tracking Z Coordinate.
  - DE: Achse der Z-Koordinate des Trackings invertieren.
  - [ ] OK    Fix: 

- **`trackingOffsetX`**
  - EN: Offset Tracking X Coordinate.
  - DE: X-Koordinate des Trackings versetzen.
  - [ ] OK    Fix: 

- **`trackingOffsetY`**
  - EN: Offset Tracking Y Coordinate.
  - DE: Y-Koordinate des Trackings versetzen.
  - [ ] OK    Fix: 

- **`trackingOffsetZ`**
  - EN: Offset Tracking Z Coordinate.
  - DE: Z-Koordinate des Trackings versetzen.
  - [ ] OK    Fix: 

- **`trackingOscPath`**
  - EN: OSC Path for the Tracking in OSC Mode (starts with a /)
  - DE: OSC Path für das Tracking im OSC-Modus (beginnt mit /)
  - [ ] OK    Fix: 

- **`trackingPort`**
  - EN: Specify the Port to receive Tracking data.
  - DE: Port zum Empfang von Tracking-Daten angeben.
  - [ ] OK    Fix: 

- **`trackingProtocol`**
  - EN: Select the type of Tracking Protocol.
  - DE: Art des Tracking-Protokolls auswählen.
  - [ ] OK    Fix: 

- **`trackingScaleX`**
  - EN: Scale Tracking X Coordinate.
  - DE: X-Koordinate des Trackings skalieren.
  - [ ] OK    Fix: 

- **`trackingScaleY`**
  - EN: Scale Tracking Y Coordinate.
  - DE: Y-Koordinate des Trackings skalieren.
  - [ ] OK    Fix: 

- **`trackingScaleZ`**
  - EN: Scale Tracking Z Coordinate.
  - DE: Z-Koordinate des Trackings skalieren.
  - [ ] OK    Fix: 

- **`txEnable`**
  - EN: Enable or Disable Data Transmission.
  - DE: Datenübertragung aktivieren oder deaktivieren.
  - [ ] OK    Fix: 

- **`txPort`**
  - EN: Transmit Port for this Target.
  - DE: Sendeport für dieses Ziel.
  - [ ] OK    Fix: 

- **`udpPort`**
  - EN: UDP Receive Port of the Processor.
  - DE: UDP-Empfangsport des Prozessors.
  - [ ] OK    Fix: 

## `network.messages`

- **`configExported`**
  - EN: Network configuration exported.
  - DE: Netzwerkkonfiguration exportiert.
  - [ ] OK    Fix: 

- **`configImported`**
  - EN: Network configuration imported.
  - DE: Netzwerkkonfiguration importiert.
  - [ ] OK    Fix: 

- **`configLoadedFromBackup`**
  - EN: Network configuration loaded from backup.
  - DE: Netzwerkkonfiguration aus Backup geladen.
  - [ ] OK    Fix: 

- **`configNotFound`**
  - EN: Network config file not found.
  - DE: Netzwerkkonfigurationsdatei nicht gefunden.
  - [ ] OK    Fix: 

- **`configReloaded`**
  - EN: Network configuration reloaded.
  - DE: Netzwerkkonfiguration neu geladen.
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Network configuration saved.
  - DE: Netzwerkkonfiguration gespeichert.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - DE: Fehler: {error}
  - [ ] OK    Fix: 

- **`findDeviceSent`**
  - EN: Find Device command sent.
  - DE: Gerät-Suchen-Befehl gesendet.
  - [ ] OK    Fix: 

- **`maxTargetsReached`**
  - EN: Maximum Number of Targets/Servers Reached.
  - DE: Maximale Anzahl von Zielen/Servern erreicht.
  - [ ] OK    Fix: 

- **`noBackupFound`**
  - EN: No backup files found.
  - DE: Keine Backup-Dateien gefunden.
  - [ ] OK    Fix: 

- **`onlyOneRemote`**
  - EN: Only one REMOTE connection is allowed.
  - DE: Nur eine FERNSTEUERUNG-Verbindung ist erlaubt.
  - [ ] OK    Fix: 

- **`oscManagerError`**
  - EN: Error: OSC Manager not available
  - DE: Fehler: OSC-Manager nicht verfügbar
  - [ ] OK    Fix: 

- **`passwordEmpty`**
  - EN: Password cannot be empty.
  - DE: Passwort darf nicht leer sein.
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - DE: Bitte wählen Sie zuerst einen Projektordner in der System Config.
  - [ ] OK    Fix: 

## `network.remote`

- **`notResponding`**
  - EN: Remote not responding — the tablet app may be outdated or unreachable
  - DE: (missing — falls back to English)
  - [ ] OK    Fix: 

- **`protocolMismatch`**
  - EN: Remote app uses protocol v{remote}, expected v{local} — update the tablet app
  - DE: (missing — falls back to English)
  - [ ] OK    Fix: 

## `networkLog.dialogs`

- **`exportCompleteMessage`**
  - EN: Log exported to: {path}
  - DE: Log exportiert nach: {path}
  - [ ] OK    Fix: 

- **`exportCompleteTitle`**
  - EN: Export Complete
  - DE: Export abgeschlossen
  - [ ] OK    Fix: 

- **`exportFailedMessage`**
  - EN: Could not write to file: {path}
  - DE: Konnte nicht in Datei schreiben: {path}
  - [ ] OK    Fix: 

- **`exportFailedTitle`**
  - EN: Export Failed
  - DE: Export fehlgeschlagen
  - [ ] OK    Fix: 

## `networkLog.status`

- **`rejected`**
  - EN: REJECTED
  - DE: ABGELEHNT
  - [ ] OK    Fix: 

## `outputs.dialogs`

- **`export`**
  - EN: Export Output Configuration
  - DE: Ausgangskonfiguration exportieren
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import Output Configuration
  - DE: Ausgangskonfiguration importieren
  - [ ] OK    Fix: 

## `outputs.help`

- **`angleOff`**
  - EN: Output Channel Will Not Amplify Objects in this Angle in Front of it. (changes may affect the rest of the array)
  - DE: Der Ausgangskanal verstärkt keine Objekte in diesem Winkel vor ihm. (Änderungen können den Rest des Arrays betreffen)
  - [ ] OK    Fix: 

- **`angleOn`**
  - EN: Output Channel Will Amplify Objects in this Angle in its Back. (changes may affect the rest of the array)
  - DE: Der Ausgangskanal verstärkt Objekte in diesem Winkel hinter ihm. (Änderungen können den Rest des Arrays betreffen)
  - [ ] OK    Fix: 

- **`applyToArray`**
  - EN: Apply Changes to the rest of the Array (Absolute value or Relative changes).
  - DE: Änderungen auf den Rest des Arrays anwenden (Absolutwert oder relative Änderungen).
  - [ ] OK    Fix: 

- **`arraySelector`**
  - EN: Selected Output Channel is part of Array.
  - DE: Der ausgewählte Ausgangskanal gehört zu einem Array.
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Output Channel Attenuation. (changes may affect the rest of the array)
  - DE: Dämpfung des Ausgangskanals. (Änderungen können den Rest des Arrays betreffen)
  - [ ] OK    Fix: 

- **`channelSelector`**
  - EN: Output Channel Number and Selection.
  - DE: Ausgangskanalnummer und -auswahl.
  - [ ] OK    Fix: 

- **`coordMode`**
  - EN: Coordinate display mode: Cartesian (X/Y/Z), Cylindrical (radius/azimuth/height), or Spherical (radius/azimuth/elevation).
  - DE: Koordinaten-Anzeigemodus: Kartesisch (X/Y/Z), Zylindrisch (Radius/Azimut/Höhe) oder Sphärisch (Radius/Azimut/Elevation).
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Output Channel Delay (positive values) or Latency Compensation (negative values). (changes may affect the rest of the array)
  - DE: Verzögerung des Ausgangskanals (positive Werte) oder Latenzkompensation (negative Werte). (Änderungen können den Rest des Arrays betreffen)
  - [ ] OK    Fix: 

- **`directional`**
  - EN: Output Channel Directional Control. Drag to change orientation, Shift+drag for Angle Off, Alt+drag for Angle On. (changes may affect the rest of the array)
  - DE: Richtungssteuerung des Ausgangskanals. Ziehen zum Ändern der Orientierung, Umschalt+Ziehen für Angle Off, Alt+Ziehen für Angle On. (Änderungen können den Rest des Arrays betreffen)
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Ratio of Distance Attenuation for Selected Output. (changes may affect the rest of the array)
  - DE: Verhältnis der Entfernungsdämpfung für den ausgewählten Ausgang. (Änderungen können den Rest des Arrays betreffen)
  - [ ] OK    Fix: 

- **`eqBandToggle`**
  - EN: Toggle Band {band} on/off. When off the band is bypassed.
  - DE: Band {band} ein-/ausschalten. Ausgeschaltet wird das Band umgangen.
  - [ ] OK    Fix: 

- **`eqEnable`**
  - EN: Enable or disable EQ processing for this output.
  - DE: EQ-Verarbeitung für diesen Ausgang aktivieren/deaktivieren.
  - [ ] OK    Fix: 

- **`eqFlatten`**
  - EN: Long press to reset all EQ bands to their default values.
  - DE: Langer Druck, um alle EQ-Bänder zurückzusetzen.
  - [ ] OK    Fix: 

- **`eqFreq`**
  - EN: Output EQ Band {band} frequency (20 Hz - 20 kHz).
  - DE: Output EQ Band {band} Frequenz (20 Hz - 20 kHz).
  - [ ] OK    Fix: 

- **`eqGain`**
  - EN: Output EQ Band {band} gain (-24 to +24 dB).
  - DE: Output EQ Band {band} Gain (-24 bis +24 dB).
  - [ ] OK    Fix: 

- **`eqQ`**
  - EN: Output EQ Band {band} Q factor (0.1 - 10.0).
  - DE: Output EQ Band {band} Q-Faktor (0.1 - 10.0).
  - [ ] OK    Fix: 

- **`eqResetBand`**
  - EN: Long press to reset Band {band} to its default values.
  - DE: Langer Druck, um Band {band} zurückzusetzen.
  - [ ] OK    Fix: 

- **`eqShape`**
  - EN: Output EQ Band {band} filter shape.
  - DE: Output EQ Band {band} Filterform.
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Output Configuration to file (with file explorer window).
  - DE: Ausgangskonfiguration in Datei exportieren (mit Datei-Explorer-Fenster).
  - [ ] OK    Fix: 

- **`floorReflections`**
  - EN: Allow or exclude this speaker from Floor Reflections.
  - DE: Diesen Lautsprecher für Floor Reflections zulassen oder ausschließen.
  - [ ] OK    Fix: 

- **`hParallax`**
  - EN: Horizontal Distance from Speaker to 'Targeted' Listener. (changes may affect the rest of the array)
  - DE: Horizontaler Abstand vom Lautsprecher zum „anvisierten“ Zuhörer. (Änderungen können den Rest des Arrays betreffen)
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: Loss of High Frequency Depending on Distance from Object to Output. (changes may affect the rest of the array)
  - DE: Hochfrequenzverlust abhängig von der Entfernung des Objekts zum Ausgang. (Änderungen können den Rest des Arrays betreffen)
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Output Configuration from file (with file explorer window).
  - DE: Ausgangskonfiguration aus Datei importieren (mit Datei-Explorer-Fenster).
  - [ ] OK    Fix: 

- **`liveSource`**
  - EN: Allow or exclude this speaker from Live Source Attenuation. (may affect the rest of the array)
  - DE: Diesen Lautsprecher für die Live-Source-Dämpfung zulassen oder ausschließen. (kann den Rest des Arrays betreffen)
  - [ ] OK    Fix: 

- **`mapVisibility`**
  - EN: Make Visible or Hide The Selected Output on the Map.
  - DE: Den ausgewählten Ausgang auf der Karte sichtbar machen oder ausblenden.
  - [ ] OK    Fix: 

- **`minLatency`**
  - EN: Allow or exclude this speaker from Minimal Latency processing. (may affect the rest of the array)
  - DE: Diesen Lautsprecher für die Minimal-Latency-Verarbeitung zulassen oder ausschließen. (kann den Rest des Arrays betreffen)
  - [ ] OK    Fix: 

- **`nameEditor`**
  - EN: Displayed Output Channel Name (editable).
  - DE: Angezeigter Name des Ausgangskanals (bearbeitbar).
  - [ ] OK    Fix: 

- **`pitch`**
  - EN: Output Channel Vertical Orientation used to Determine which Objects get amplified. (changes may affect the rest of the array)
  - DE: Vertikale Orientierung des Ausgangskanals zur Bestimmung, welche Objekte verstärkt werden. (Änderungen können den Rest des Arrays betreffen)
  - [ ] OK    Fix: 

- **`position1`**
  - EN: Output Channel {name} ({unit}).
  - DE: Ausgangskanal {name} ({unit}).
  - [ ] OK    Fix: 

- **`position2`**
  - EN: Output Channel {name} ({unit}).
  - DE: Ausgangskanal {name} ({unit}).
  - [ ] OK    Fix: 

- **`position3`**
  - EN: Output Channel {name} ({unit}).
  - DE: Ausgangskanal {name} ({unit}).
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Output Configuration from backup file.
  - DE: Ausgangskonfiguration aus Backup-Datei neu laden.
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Output Configuration from file.
  - DE: Ausgangskonfiguration aus Datei neu laden.
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Output Configuration to file (with backup).
  - DE: Ausgangskonfiguration in Datei speichern (mit Backup).
  - [ ] OK    Fix: 

- **`vParallax`**
  - EN: Vertical Distance from Speaker to 'Targeted' Listener. Positive when the Speaker is Below the head of the Listener. (changes may affect the rest of the array)
  - DE: Vertikaler Abstand vom Lautsprecher zum „anvisierten“ Zuhörer. Positiv, wenn der Lautsprecher unter dem Kopf des Zuhörers ist. (Änderungen können den Rest des Arrays betreffen)
  - [ ] OK    Fix: 

- **`wizardOfOutZ`**
  - EN: Open Wizard of OutZ to Position Speaker Arrays Conveniently.
  - DE: Wizard of OutZ öffnen, um Lautsprecher-Arrays bequem zu positionieren.
  - [ ] OK    Fix: 

## `outputs.messages`

- **`arrayEditSingle`**
  - EN: Ctrl edit: change applied to this output only (Array {array} not affected)
  - DE: Strg-Bearbeitung: Änderung nur auf diesen Ausgang angewendet (Array {array} nicht betroffen)
  - [ ] OK    Fix: 

- **`assignedToArray`**
  - EN: Output {num} assigned to Array {array}
  - DE: Ausgang {num} zu Array {array} zugewiesen
  - [ ] OK    Fix: 

- **`backupLoaded`**
  - EN: Output configuration loaded from backup.
  - DE: Ausgangskonfiguration aus Backup geladen.
  - [ ] OK    Fix: 

- **`configExported`**
  - EN: Output configuration exported.
  - DE: Ausgangskonfiguration exportiert.
  - [ ] OK    Fix: 

- **`configImported`**
  - EN: Output configuration imported.
  - DE: Ausgangskonfiguration importiert.
  - [ ] OK    Fix: 

- **`configLoaded`**
  - EN: Output configuration loaded.
  - DE: Ausgangskonfiguration geladen.
  - [ ] OK    Fix: 

- **`configNotFound`**
  - EN: Output config file not found.
  - DE: Ausgangskonfigurationsdatei nicht gefunden.
  - [ ] OK    Fix: 

- **`configReloaded`**
  - EN: Output configuration reloaded.
  - DE: Ausgangskonfiguration neu geladen.
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Output configuration saved.
  - DE: Ausgangskonfiguration gespeichert.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - DE: Fehler: {error}
  - [ ] OK    Fix: 

- **`noBackup`**
  - EN: No backup output configuration found.
  - DE: Keine Backup-Ausgangskonfiguration gefunden.
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - DE: Bitte wählen Sie zuerst einen Projektordner in der System Config.
  - [ ] OK    Fix: 

- **`setToSingle`**
  - EN: Output {num} set to Single
  - DE: Ausgang {num} auf Einzeln gesetzt
  - [ ] OK    Fix: 

## `reverbs.dialogs`

- **`export`**
  - EN: Export Reverb Configuration
  - DE: Hallkonfiguration exportieren
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import Reverb Configuration
  - DE: Hallkonfiguration importieren
  - [ ] OK    Fix: 

## `reverbs.help`

- **`algoCrossoverHigh`**
  - EN: High crossover frequency for 3-band decay (1 - 10 kHz).
  - DE: Obere Trennfrequenz für 3-Band-Abklingen (1 - 10 kHz).
  - [ ] OK    Fix: 

- **`algoCrossoverLow`**
  - EN: Low crossover frequency for 3-band decay (50 - 500 Hz).
  - DE: Untere Trennfrequenz für 3-Band-Abklingen (50 - 500 Hz).
  - [ ] OK    Fix: 

- **`algoDiffusion`**
  - EN: Diffusion amount controlling echo density (0 - 100%).
  - DE: Diffusionsmenge zur Steuerung der Echodichte (0 - 100%).
  - [ ] OK    Fix: 

- **`algoFDN`**
  - EN: Select FDN (Feedback Delay Network) reverb algorithm.
  - DE: FDN (Feedback Delay Network) Hallalgorithmus auswählen.
  - [ ] OK    Fix: 

- **`algoFDNGpu`**
  - EN: Run the FDN reverb on the GPU (adds ~20 ms to the wet path only) or on the CPU. Falls back to CPU automatically if the GPU is unavailable.
  - DE: Führt den FDN-Hall auf der GPU aus (fügt nur dem Wet-Signal ~20 ms hinzu) oder auf der CPU. Fällt automatisch auf die CPU zurück, wenn keine GPU verfügbar ist.
  - [ ] OK    Fix: 

- **`algoFDNSize`**
  - EN: FDN delay line size multiplier (0.5 - 2.0x).
  - DE: FDN Verzögerungsleitungsgröße-Multiplikator (0.5 - 2.0x).
  - [ ] OK    Fix: 

- **`algoIR`**
  - EN: Select IR (Impulse Response convolution) reverb algorithm.
  - DE: IR (Impulsantwort-Faltung) Hallalgorithmus auswählen.
  - [ ] OK    Fix: 

- **`algoIRFile`**
  - EN: Select or import an impulse response file for convolution.
  - DE: Impulsantwort-Datei für Faltung auswählen oder importieren.
  - [ ] OK    Fix: 

- **`algoIRGpu`**
  - EN: Run the IR convolution on the GPU (adds ~20 ms to the wet path only) or on the CPU. Falls back to CPU automatically if the GPU is unavailable.
  - DE: Führt die IR-Faltung auf der GPU aus (fügt nur dem Wet-Signal ~20 ms hinzu) oder auf der CPU. Fällt automatisch auf die CPU zurück, wenn keine GPU verfügbar ist.
  - [ ] OK    Fix: 

- **`algoIRLength`**
  - EN: Maximum impulse response length (0.1 - 6.0 seconds).
  - DE: Maximale Impulsantwortlänge (0.1 - 6.0 Sekunden).
  - [ ] OK    Fix: 

- **`algoIRTrim`**
  - EN: Trim the start of the impulse response (0 - 100 ms).
  - DE: Anfang der Impulsantwort trimmen (0 - 100 ms).
  - [ ] OK    Fix: 

- **`algoPerNode`**
  - EN: Use a separate IR for each reverb node, or share one IR.
  - DE: Separate IR für jeden Hallknoten verwenden oder eine IR teilen.
  - [ ] OK    Fix: 

- **`algoRT60`**
  - EN: Reverb decay time RT60 (0.2 - 8.0 seconds).
  - DE: Hallabklingzeit RT60 (0.2 - 8.0 Sekunden).
  - [ ] OK    Fix: 

- **`algoRT60HighMult`**
  - EN: High frequency RT60 multiplier (0.1 - 9.0x).
  - DE: Hochfrequenz-RT60-Multiplikator (0.1 - 9.0x).
  - [ ] OK    Fix: 

- **`algoRT60LowMult`**
  - EN: Low frequency RT60 multiplier (0.1 - 9.0x).
  - DE: Tieffrequenz-RT60-Multiplikator (0.1 - 9.0x).
  - [ ] OK    Fix: 

- **`algoSDN`**
  - EN: Select SDN (Scattering Delay Network) reverb algorithm.
  - DE: SDN (Scattering Delay Network) Hallalgorithmus auswählen.
  - [ ] OK    Fix: 

- **`algoSDNGpu`**
  - EN: Run the SDN reverb on the GPU (adds ~20 ms to the wet path only) or on the CPU. Falls back to CPU automatically if the GPU is unavailable.
  - DE: Führt den SDN-Hall auf der GPU aus (fügt nur dem Wet-Signal ~20 ms hinzu) oder auf der CPU. Fällt automatisch auf die CPU zurück, wenn keine GPU verfügbar ist.
  - [ ] OK    Fix: 

- **`algoSDNScale`**
  - EN: SDN inter-node delay scale factor (0.5 - 4.0x).
  - DE: SDN Inter-Knoten-Verzögerungsskalierungsfaktor (0.5 - 4.0x).
  - [ ] OK    Fix: 

- **`algoWetLevel`**
  - EN: Wet/dry mix level for reverb output (-60 to +12 dB).
  - DE: Wet/Dry-Mischpegel für den Hallausgang (-60 bis +12 dB).
  - [ ] OK    Fix: 

- **`angleOff`**
  - EN: Angle at which no amplification occurs (0-179 degrees).
  - DE: Winkel, bei dem keine Verstärkung stattfindet (0-179 Grad).
  - [ ] OK    Fix: 

- **`angleOn`**
  - EN: Angle at which amplification starts (1-180 degrees).
  - DE: Winkel, ab dem die Verstärkung beginnt (1-180 Grad).
  - [ ] OK    Fix: 

- **`applyToAllNodes`**
  - EN: When ON, parameter edits are applied to every reverb node at once; when OFF, only the selected node. Runtime only - only future edits propagate.
  - DE: Wenn EIN, werden Parameteränderungen auf alle Hallknoten gleichzeitig angewendet; wenn AUS, nur auf den ausgewählten Knoten. Nur zur Laufzeit - nur künftige Änderungen werden übernommen.
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Reverb channel attenuation (-92 to 0 dB).
  - DE: Hallkanal-Dämpfung (-92 bis 0 dB).
  - [ ] OK    Fix: 

- **`channelSelector`**
  - EN: Reverb Channel Number and Selection.
  - DE: Hallkanal-Nummer und Auswahl.
  - [ ] OK    Fix: 

- **`commonAtten`**
  - EN: Common attenuation percentage (0-100%).
  - DE: Gemeinsamer Dämpfungsprozentsatz (0-100%).
  - [ ] OK    Fix: 

- **`coordMode`**
  - EN: Coordinate display mode: Cartesian (X/Y/Z), Cylindrical (radius/azimuth/height), or Spherical (radius/azimuth/elevation).
  - DE: Koordinaten-Anzeigemodus: Kartesisch (X/Y/Z), Zylindrisch (Radius/Azimut/Höhe) oder Sphärisch (Radius/Azimut/Elevation).
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Reverb delay/latency compensation (-100 to +100 ms).
  - DE: Verzögerungs-/Latenzkompensation der Reverb (-100 bis +100 ms).
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Distance attenuation for reverb return (-6.0 to 0.0 dB/m).
  - DE: Distance attenuation für den Hallrücklauf (-6.0 bis 0.0 dB/m).
  - [ ] OK    Fix: 

- **`distanceAttenEnable`**
  - EN: Distance attenuation percentage (0-200%).
  - DE: Prozentsatz der Distance attenuation (0-200%).
  - [ ] OK    Fix: 

- **`eqBandToggle`**
  - EN: Toggle pre-EQ Band {band} on/off. When off the band is bypassed.
  - DE: Pre-EQ Band {band} ein-/ausschalten.
  - [ ] OK    Fix: 

- **`eqEnable`**
  - EN: Enable or disable EQ processing for this reverb.
  - DE: EQ-Verarbeitung für diesen Hallkanal aktivieren oder deaktivieren.
  - [ ] OK    Fix: 

- **`eqFlatten`**
  - EN: Long press to reset all pre-EQ bands to their default values.
  - DE: Langer Druck, um alle Pre-EQ-Bänder zurückzusetzen.
  - [ ] OK    Fix: 

- **`eqFreq`**
  - EN: Pre-EQ Band {band} frequency (20 Hz - 20 kHz).
  - DE: Pre-EQ Band {band} Frequenz (20 Hz - 20 kHz).
  - [ ] OK    Fix: 

- **`eqGain`**
  - EN: Pre-EQ Band {band} gain (-24 to +24 dB).
  - DE: Pre-EQ Band {band} Gain (-24 bis +24 dB).
  - [ ] OK    Fix: 

- **`eqQ`**
  - EN: Pre-EQ Band {band} Q factor (0.1 - 20.0).
  - DE: Pre-EQ Band {band} Q-Faktor (0.1 - 20.0).
  - [ ] OK    Fix: 

- **`eqResetBand`**
  - EN: Long press to reset pre-EQ Band {band} to its default values.
  - DE: Langer Druck, um Pre-EQ Band {band} zurückzusetzen.
  - [ ] OK    Fix: 

- **`eqShape`**
  - EN: Pre-EQ Band {band} filter shape.
  - DE: Pre-EQ Band {band} Filterform.
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Reverb Configuration to file (with file explorer window).
  - DE: Hallkonfiguration in Datei exportieren (mit Datei-Explorer).
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: High frequency loss per meter (-6.0 to 0.0 dB/m).
  - DE: Hochfrequenzverlust pro Meter (-6.0 bis 0.0 dB/m).
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Reverb Configuration from file (with file explorer window).
  - DE: Hallkonfiguration aus Datei importieren (mit Datei-Explorer).
  - [ ] OK    Fix: 

- **`liveSourceTooltip`**
  - EN: Allow or exclude this reverb feed from Live Source Attenuation.
  - DE: Diesen Reverb-Feed für die Live-Source-Dämpfung zulassen oder ausschließen.
  - [ ] OK    Fix: 

- **`mapVisibility`**
  - EN: Make Visible or Hide All Reverb Channels on the Map.
  - DE: Alle Reverb Channels auf der Karte sichtbar machen oder ausblenden.
  - [ ] OK    Fix: 

- **`miniLatencyTooltip`**
  - EN: Allow or exclude this reverb feed from Minimal Latency processing.
  - DE: Diesen Reverb-Feed für die Minimal-Latency-Verarbeitung zulassen oder ausschließen.
  - [ ] OK    Fix: 

- **`muteButton`**
  - EN: Toggle mute for this output channel's reverb return.
  - DE: Mute für den Reverb-Rückkanal dieses Ausgangs umschalten.
  - [ ] OK    Fix: 

- **`muteMacros`**
  - EN: Quick mute operations for output channels.
  - DE: Schnelle Stummschaltungsoperationen für Ausgangskanäle.
  - [ ] OK    Fix: 

- **`nameEditor`**
  - EN: Displayed Reverb Channel Name (editable).
  - DE: Angezeigter Hallkanal-Name (bearbeitbar).
  - [ ] OK    Fix: 

- **`orientation`**
  - EN: Reverb orientation angle (-179 to +180 degrees).
  - DE: Orientierungswinkel der Reverb (-179 bis +180 Grad).
  - [ ] OK    Fix: 

- **`pitch`**
  - EN: Vertical orientation of reverb (-90 to +90 degrees).
  - DE: Vertikale Ausrichtung der Reverb (-90 bis +90 Grad).
  - [ ] OK    Fix: 

- **`position1`**
  - EN: Reverb virtual source {name} ({unit}).
  - DE: Reverb-Quellposition {name} ({unit}).
  - [ ] OK    Fix: 

- **`position2`**
  - EN: Reverb virtual source {name} ({unit}).
  - DE: Reverb-Quellposition {name} ({unit}).
  - [ ] OK    Fix: 

- **`position3`**
  - EN: Reverb virtual source {name} ({unit}).
  - DE: Reverb-Quellposition {name} ({unit}).
  - [ ] OK    Fix: 

- **`postEqBandToggle`**
  - EN: Toggle post-EQ Band {band} on/off. When off the band is bypassed.
  - DE: Post-EQ Band {band} ein-/ausschalten.
  - [ ] OK    Fix: 

- **`postEqEnable`**
  - EN: Enable or disable post-processing EQ.
  - DE: Nachbearbeitungs-EQ aktivieren oder deaktivieren.
  - [ ] OK    Fix: 

- **`postEqFlatten`**
  - EN: Long press to reset all post-EQ bands to their default values.
  - DE: Langer Druck, um alle Post-EQ-Bänder zurückzusetzen.
  - [ ] OK    Fix: 

- **`postEqFreq`**
  - EN: Post-EQ Band {band} frequency (20 Hz - 20 kHz).
  - DE: Frequenz von Post-EQ Band {band} (20 Hz - 20 kHz).
  - [ ] OK    Fix: 

- **`postEqGain`**
  - EN: Post-EQ Band {band} gain (-24 to +24 dB).
  - DE: Verstärkung von Post-EQ Band {band} (-24 bis +24 dB).
  - [ ] OK    Fix: 

- **`postEqQ`**
  - EN: Post-EQ Band {band} Q factor (0.1 - 20.0).
  - DE: Q-Faktor von Post-EQ Band {band} (0.1 - 20.0).
  - [ ] OK    Fix: 

- **`postEqResetBand`**
  - EN: Long press to reset post-EQ Band {band} to its default values.
  - DE: Langer Druck, um Post-EQ Band {band} zurückzusetzen.
  - [ ] OK    Fix: 

- **`postEqShape`**
  - EN: Post-EQ Band {band} filter shape.
  - DE: Filterform von Post-EQ Band {band}.
  - [ ] OK    Fix: 

- **`postExpAttack`**
  - EN: Post-expander attack time (0.1 - 50 ms).
  - DE: Attack-Zeit des Nachexpanders (0.1 - 50 ms).
  - [ ] OK    Fix: 

- **`postExpBypass`**
  - EN: Bypass or enable the post-expander on reverb returns.
  - DE: Nachexpander für Hallrückläufe umgehen oder aktivieren.
  - [ ] OK    Fix: 

- **`postExpRatio`**
  - EN: Post-expander ratio (1:1 to 1:8).
  - DE: Verhältnis des Nachexpanders (1:1 bis 1:8).
  - [ ] OK    Fix: 

- **`postExpRelease`**
  - EN: Post-expander release time (50 - 2000 ms).
  - DE: Release-Zeit des Nachexpanders (50 - 2000 ms).
  - [ ] OK    Fix: 

- **`postExpThreshold`**
  - EN: Post-expander threshold (-80 to -10 dB).
  - DE: Schwelle des Nachexpanders (-80 bis -10 dB).
  - [ ] OK    Fix: 

- **`preCompAttack`**
  - EN: Pre-compressor attack time (0.1 - 100 ms).
  - DE: Attack-Zeit des Vorkompressors (0.1 - 100 ms).
  - [ ] OK    Fix: 

- **`preCompBypass`**
  - EN: Bypass or enable the pre-compressor on reverb feeds.
  - DE: Vorkompressor für Hallsendungen umgehen oder aktivieren.
  - [ ] OK    Fix: 

- **`preCompRatio`**
  - EN: Pre-compressor ratio (1:1 to 20:1).
  - DE: Verhältnis des Vorkompressors (1:1 bis 20:1).
  - [ ] OK    Fix: 

- **`preCompRelease`**
  - EN: Pre-compressor release time (10 - 1000 ms).
  - DE: Release-Zeit des Vorkompressors (10 - 1000 ms).
  - [ ] OK    Fix: 

- **`preCompThreshold`**
  - EN: Pre-compressor threshold (-60 to 0 dB).
  - DE: Schwelle des Vorkompressors (-60 bis 0 dB).
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Reverb Configuration from backup file.
  - DE: Hallkonfiguration aus Backup-Datei neu laden.
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Reverb Configuration from file.
  - DE: Hallkonfiguration aus Datei neu laden.
  - [ ] OK    Fix: 

- **`returnOffset1`**
  - EN: Reverb return offset {name} ({unit}).
  - DE: Reverb-Rückgabe-Versatz {name} ({unit}).
  - [ ] OK    Fix: 

- **`returnOffset2`**
  - EN: Reverb return offset {name} ({unit}).
  - DE: Reverb-Rückgabe-Versatz {name} ({unit}).
  - [ ] OK    Fix: 

- **`returnOffset3`**
  - EN: Reverb return offset {name} ({unit}).
  - DE: Reverb-Rückgabe-Versatz {name} ({unit}).
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Reverb Configuration to file (with backup).
  - DE: Hallkonfiguration in Datei speichern (mit Backup).
  - [ ] OK    Fix: 

## `reverbs.messages`

- **`backupLoaded`**
  - EN: Reverb configuration loaded from backup.
  - DE: Hallkonfiguration aus Backup geladen.
  - [ ] OK    Fix: 

- **`configExported`**
  - EN: Reverb configuration exported.
  - DE: Hallkonfiguration exportiert.
  - [ ] OK    Fix: 

- **`configImported`**
  - EN: Reverb configuration imported.
  - DE: Hallkonfiguration importiert.
  - [ ] OK    Fix: 

- **`configLoaded`**
  - EN: Reverb configuration loaded.
  - DE: Hallkonfiguration geladen.
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Reverb configuration saved.
  - DE: Hallkonfiguration gespeichert.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - DE: Fehler: {error}
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - DE: Bitte wählen Sie zuerst einen Projektordner in der System Config.
  - [ ] OK    Fix: 

## `sampler`

- **`guide`**
  - EN: Select a cell on the grid to edit its properties.\nDouble-click to load a sample.\nUse Ctrl+Click to assign cells to the active set.
  - DE: Wählen Sie eine Zelle im Raster aus, um ihre Eigenschaften zu bearbeiten.\nDoppelklicken Sie, um ein Sample zu laden.\nMit Strg+Klick können Sie Zellen dem aktiven Set zuweisen.
  - [ ] OK    Fix: 

## `sampler.grid`

- **`help`**
  - EN: Click = select\nShift = multi\nCtrl = set toggle\nDblClick = load
  - DE: Klick=Auswahl | Shift=Mehrfach | Strg=Set umschalten | Doppelklick=Laden
  - [ ] OK    Fix: 

## `sampler.tooltips`

- **`addSet`**
  - EN: Create a new set. If cells are selected they will be assigned to it.
  - DE: Neues Set erstellen. Wenn Zellen ausgewählt sind, werden sie ihm zugewiesen.
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Cell attenuation in dB (0 = no attenuation, -60 = silent)
  - DE: Zellendämpfung in dB (0 = keine Dämpfung, -60 = Stille)
  - [ ] OK    Fix: 

- **`clear`**
  - EN: Remove the sample from the selected cell (long press)
  - DE: Sample aus der ausgewählten Zelle entfernen (langer Druck)
  - [ ] OK    Fix: 

- **`copy`**
  - EN: Copy selected cell or active set to clipboard
  - DE: Ausgewählte Zelle oder aktives Set in die Zwischenablage kopieren
  - [ ] OK    Fix: 

- **`deleteSet`**
  - EN: Delete the active set (long press)
  - DE: Aktives Set löschen (langer Druck)
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export sampler configuration to file
  - DE: Sampler-Konfiguration in Datei exportieren
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import sampler configuration from file
  - DE: Sampler-Konfiguration aus Datei importieren
  - [ ] OK    Fix: 

- **`inOut`**
  - EN: Set the In/Out time range in milliseconds. Drag between thumbs to shift both.
  - DE: Ein-/Aus-Zeitbereich in Millisekunden festlegen. Zwischen den Reglern ziehen, um beide zu verschieben.
  - [ ] OK    Fix: 

- **`load`**
  - EN: Load a sample file into the selected cell
  - DE: Eine Sampledatei in die ausgewählte Zelle laden
  - [ ] OK    Fix: 

- **`offset`**
  - EN: Offset position in meters (X, Y, Z) relative to the set position
  - DE: Positionsversatz in Metern (X, Y, Z) relativ zur Set-Position
  - [ ] OK    Fix: 

- **`paste`**
  - EN: Paste clipboard data to selected cell or active set
  - DE: Zwischenablagedaten in die ausgewählte Zelle oder das aktive Set einfügen
  - [ ] OK    Fix: 

- **`playMode`**
  - EN: Toggle between Sequential and Round-Robin playback
  - DE: Zwischen sequentieller und Round-Robin-Wiedergabe wechseln
  - [ ] OK    Fix: 

- **`pressCurve`**
  - EN: Pressure response curve (0 = concave, 0.5 = linear, 1 = convex)
  - DE: Druck-Antwortkurve (0 = konkav, 0,5 = linear, 1 = konvex)
  - [ ] OK    Fix: 

- **`pressDir`**
  - EN: Toggle pressure direction: + = more pressure increases, - = decreases
  - DE: Druckrichtung umschalten: + = mehr Druck erhöht, - = verringert
  - [ ] OK    Fix: 

- **`pressHF`**
  - EN: Map finger pressure to high-frequency shelf attenuation
  - DE: Fingerdruck auf die High-Shelf-Dämpfung mappen
  - [ ] OK    Fix: 

- **`pressHeight`**
  - EN: Map finger pressure to vertical position (Z)
  - DE: Fingerdruck auf die vertikale Position (Z) mappen
  - [ ] OK    Fix: 

- **`pressLevel`**
  - EN: Map finger pressure to output level
  - DE: Fingerdruck auf den Ausgangspegel mappen
  - [ ] OK    Fix: 

- **`pressXY`**
  - EN: Map finger pressure to XY position movement
  - DE: Fingerdruck auf XY-Positionsbewegung mappen
  - [ ] OK    Fix: 

- **`pressXYScale`**
  - EN: Sensitivity: how far the source moves per pressure step
  - DE: Empfindlichkeit: wie weit sich die Quelle pro Druckschritt bewegt
  - [ ] OK    Fix: 

- **`preview`**
  - EN: Preview the loaded sample
  - DE: Geladenes Sample vorhören
  - [ ] OK    Fix: 

- **`renameSet`**
  - EN: Rename the active set
  - DE: Aktives Set umbenennen
  - [ ] OK    Fix: 

- **`setLevel`**
  - EN: Set output level in dB (0 = unity, -60 = silent)
  - DE: Ausgangspegel in dB festlegen (0 = Einheit, -60 = Stille)
  - [ ] OK    Fix: 

- **`setPos`**
  - EN: Set base position in meters (X, Y, Z)
  - DE: Basisposition in Metern festlegen (X, Y, Z)
  - [ ] OK    Fix: 

## `systemConfig.dialogs`

- **`exportSystemConfig`**
  - EN: Export System Configuration
  - DE: Systemkonfiguration exportieren
  - [ ] OK    Fix: 

- **`importSystemConfig`**
  - EN: Import System Configuration
  - DE: Systemkonfiguration importieren
  - [ ] OK    Fix: 

- **`reduce`**
  - EN: Reduce
  - DE: Reduzieren
  - [ ] OK    Fix: 

- **`reduceInputChannels.message`**
  - EN: Reducing from {current} to {new} input channels will remove settings for channels {start} to {end}.\n\nThis cannot be undone.
  - DE: Reduzierung von {current} auf {new} Eingangskanäle entfernt die Einstellungen für Kanäle {start} bis {end}.\n\nDies kann nicht rückgängig gemacht werden.
  - [ ] OK    Fix: 

- **`reduceInputChannels.title`**
  - EN: Reduce Input Channels?
  - DE: Input Channels reduzieren?
  - [ ] OK    Fix: 

- **`reduceOutputChannels.message`**
  - EN: Reducing from {current} to {new} output channels will remove settings for channels {start} to {end}.\n\nThis cannot be undone.
  - DE: Reduzierung von {current} auf {new} Ausgangskanäle entfernt die Einstellungen für Kanäle {start} bis {end}.\n\nDies kann nicht rückgängig gemacht werden.
  - [ ] OK    Fix: 

- **`reduceOutputChannels.title`**
  - EN: Reduce Output Channels?
  - DE: Output Channels reduzieren?
  - [ ] OK    Fix: 

- **`reduceReverbChannels.message`**
  - EN: Reducing from {current} to {new} reverb channels will remove settings for channels {start} to {end}.\n\nThis cannot be undone.
  - DE: Reduzierung von {current} auf {new} Hallkanäle entfernt die Einstellungen für Kanäle {start} bis {end}.\n\nDies kann nicht rückgängig gemacht werden.
  - [ ] OK    Fix: 

- **`reduceReverbChannels.title`**
  - EN: Reduce Reverb Channels?
  - DE: Reverb Channels reduzieren?
  - [ ] OK    Fix: 

- **`selectProjectFolder`**
  - EN: Select Project Folder
  - DE: Projektordner auswählen
  - [ ] OK    Fix: 

## `systemConfig.help`

- **`algorithm`**
  - EN: Select the rendering algorithm from the menu.
  - DE: Wählen Sie den Rendering-Algorithmus aus dem Menü.
  - [ ] OK    Fix: 

- **`audioPatch`**
  - EN: Opens the Audio Interface and Patching Window.
  - DE: Öffnet das Audio-Interface und Patch-Fenster.
  - [ ] OK    Fix: 

- **`binauralAngle`**
  - EN: Horizontal rotation of binaural listener view (degrees, 0 = facing stage).
  - DE: Horizontale Drehung der binauralen Hörerperspektive (Grad, 0 = zur Bühne gewandt).
  - [ ] OK    Fix: 

- **`binauralAtten`**
  - EN: Overall level offset for binaural output (dB).
  - DE: Globaler Pegelversatz für die binaurale Ausgabe (dB).
  - [ ] OK    Fix: 

- **`binauralDelay`**
  - EN: Additional delay for binaural output (milliseconds).
  - DE: Zusätzliche Verzögerung für die binaurale Ausgabe (Millisekunden).
  - [ ] OK    Fix: 

- **`binauralDistance`**
  - EN: Distance of binaural listener from stage origin (meters).
  - DE: Abstand des binauralen Hörers zum Bühnenmittelpunkt (Meter).
  - [ ] OK    Fix: 

- **`binauralEnable`**
  - EN: Enable or disable the binaural renderer processing.
  - DE: Aktivieren oder deaktivieren Sie die binaurale Renderer-Verarbeitung.
  - [ ] OK    Fix: 

- **`binauralOutput`**
  - EN: Select output channel pair for binaural monitoring. Off disables binaural output.
  - DE: Wählen Sie das Ausgangskanalpaar für das binaurale Monitoring. Off deaktiviert die binaurale Ausgabe.
  - [ ] OK    Fix: 

- **`clearSolo`**
  - EN: Clear all input solo states.
  - DE: Clear all input solo states.
  - [ ] OK    Fix: 

- **`colorScheme`**
  - EN: Select the color scheme: Default (dark gray), Black (pure black for OLED displays), or Light (daytime use).
  - DE: Wählen Sie das Farbschema: Standard (Dunkelgrau), Schwarz (für OLED-Displays) oder Hell (für Tageslicht).
  - [ ] OK    Fix: 

- **`copySystemInfo`**
  - EN: Copy detailed system information to the clipboard for support requests.
  - DE: Detaillierte Systeminformationen für Support-Anfragen in die Zwischenablage kopieren.
  - [ ] OK    Fix: 

- **`diagnosticsToggle`**
  - EN: Long-press to show or hide the diagnostic tools (export logs, open log folder, copy system info, report issue).
  - DE: Lange drücken, um Diagnose-Tools ein- oder auszublenden (Logs exportieren, Log-Ordner öffnen, Systeminfo kopieren, Problem melden).
  - [ ] OK    Fix: 

- **`dialsAndButtons`**
  - EN: Select the hardware controller for dials and buttons: Stream Deck+.
  - DE: Wählen Sie den Hardware-Controller für Drehregler und Tasten: Stream Deck+.
  - [ ] OK    Fix: 

- **`domeElevation`**
  - EN: Dome elevation angle: 180 = hemisphere, 360 = full sphere.
  - DE: Kuppel-Elevationswinkel: 180 = Halbkugel, 360 = Vollkugel.
  - [ ] OK    Fix: 

- **`exportLogs`**
  - EN: Export diagnostic logs to a zip file for debugging or support.
  - DE: Diagnose-Logs in eine Zip-Datei zur Fehlersuche oder zum Support exportieren.
  - [ ] OK    Fix: 

- **`exportSystem`**
  - EN: Export System Configuration to file (with file explorer window).
  - DE: Systemkonfiguration in Datei exportieren (mit Datei-Explorer).
  - [ ] OK    Fix: 

- **`gpuPipelineDepth`**
  - EN: GPU pipeline depth in audio blocks. Adds depth x buffer/sample-rate of constant latency (pre-subtracted from WFS delays) and absorbs GPU stalls of the same length: deeper = immune to desktop/UI hiccups, shallower = lower latency. Applies live. Default 4.
  - DE: GPU-Pipeline-Tiefe in Audioblöcken. Fügt depth x Puffer/Abtastrate an konstanter Latenz hinzu (vorab von den WFS-Verzögerungen abgezogen) und gleicht GPU-Aussetzer gleicher Länge aus: tiefer = immun gegen Desktop-/UI-Ruckler, flacher = geringere Latenz. Wird live angewendet. Standard 4.
  - [ ] OK    Fix: 

- **`haasEffect`**
  - EN: Hass Effect to apply to the system. Will take into account the Latency Compensations (System, Input and Output).
  - DE: Haas-Effekt für das System anwenden. Berücksichtigt die Latenzkompensationen (System, Eingang und Ausgang).
  - [ ] OK    Fix: 

- **`importSystem`**
  - EN: Import System Configuration from file (with file explorer window).
  - DE: Systemkonfiguration aus Datei importieren (mit Datei-Explorer).
  - [ ] OK    Fix: 

- **`inputChannels`**
  - EN: Number of Input Channels.
  - DE: Anzahl der Input Channels.
  - [ ] OK    Fix: 

- **`language`**
  - EN: Select the user interface language. Changes take full effect after restarting the application.
  - DE: Wählen Sie die Sprache der Benutzeroberfläche. Änderungen werden nach Neustart der Anwendung vollständig wirksam.
  - [ ] OK    Fix: 

- **`levelMeter`**
  - EN: Opens the Level Meter Window.
  - DE: Opens the Level Meter Window.
  - [ ] OK    Fix: 

- **`lightpadSetup`**
  - EN: Display the connected Roli Lightpads and allow for splitting them in 4 smaller pads.
  - DE: Verbundene Roli Lightpads anzeigen und in 4 kleinere Pads aufteilen.
  - [ ] OK    Fix: 

- **`masterLevel`**
  - EN: Master Level (affects all outputs).
  - DE: Master Level (beeinflusst alle Ausgänge).
  - [ ] OK    Fix: 

- **`openLogFolder`**
  - EN: Open the application's log folder in the system file explorer.
  - DE: Log-Ordner der Anwendung im System-Dateibrowser öffnen.
  - [ ] OK    Fix: 

- **`originCenter`**
  - EN: Set origin to center of stage volume. Typical for Spherical Dome setups.
  - DE: Ursprung auf Bühnenmitte setzen. Typisch für Kugelkuppel-Aufbauten.
  - [ ] OK    Fix: 

- **`originCenterGround`**
  - EN: Set origin to center of stage at ground level. Typical for Surround or Cylindrical setups.
  - DE: Ursprung auf Bühnenmitte am Boden setzen. Typisch für Surround- oder Zylinderaufbauten.
  - [ ] OK    Fix: 

- **`originDepth`**
  - EN: Origin Y offset from stage center (0 = centered, negative = front/downstage).
  - DE: Ursprung Y-Versatz von Bühnenmitte (0 = zentriert, negativ = vorne).
  - [ ] OK    Fix: 

- **`originDismissInputs`**
  - EN: Long press to dismiss and keep current input positions.
  - DE: Lange drücken, um die aktuellen Eingangspositionen beizubehalten.
  - [ ] OK    Fix: 

- **`originDismissOutputs`**
  - EN: Long press to dismiss and keep current output positions.
  - DE: Lange drücken, um die aktuellen Ausgangspositionen beizubehalten.
  - [ ] OK    Fix: 

- **`originDismissReverbs`**
  - EN: Long press to dismiss and keep current reverb positions.
  - DE: Lange drücken, um die aktuellen Reverb-Positionen beizubehalten.
  - [ ] OK    Fix: 

- **`originFront`**
  - EN: Set origin to front center of stage. Typical for frontal stages.
  - DE: Ursprung auf Bühnenvorderseite setzen. Typisch für Frontalbühnen.
  - [ ] OK    Fix: 

- **`originHeight`**
  - EN: Origin Z offset from floor (0 = floor level, positive = above floor).
  - DE: Ursprung Z-Versatz vom Boden (0 = Bodenniveau, positiv = über Boden).
  - [ ] OK    Fix: 

- **`originShiftInputs`**
  - EN: Long press to shift all input positions by the origin change.
  - DE: Lange drücken, um alle Eingangspositionen entsprechend der Ursprungsänderung zu verschieben.
  - [ ] OK    Fix: 

- **`originShiftOutputs`**
  - EN: Long press to shift all output positions by the origin change.
  - DE: Lange drücken, um alle Ausgangspositionen entsprechend der Ursprungsänderung zu verschieben.
  - [ ] OK    Fix: 

- **`originShiftReverbs`**
  - EN: Long press to shift all reverb positions by the origin change.
  - DE: Lange drücken, um alle Reverb-Positionen entsprechend der Ursprungsänderung zu verschieben.
  - [ ] OK    Fix: 

- **`originWidth`**
  - EN: Origin X offset from stage center (0 = centered, negative = left).
  - DE: Ursprung X-Versatz von Bühnenmitte (0 = zentriert, negativ = links).
  - [ ] OK    Fix: 

- **`outputChannels`**
  - EN: Number of Output Channels.
  - DE: Anzahl der Output Channels.
  - [ ] OK    Fix: 

- **`positionControl`**
  - EN: Select the hardware controller for position control: Space Mouse, Joystick, or Game Pad.
  - DE: Wählen Sie den Hardware-Controller für die Positionssteuerung: Space Mouse, Joystick oder Gamepad.
  - [ ] OK    Fix: 

- **`processing`**
  - EN: Lock all I/O parameters and start the DSP. Long press to stop the DSP.
  - DE: Alle E/A-Parameter sperren und DSP starten. Lang drücken um DSP zu stoppen.
  - [ ] OK    Fix: 

- **`quickLongPress`**
  - EN: Long press duration. Instead of confirmation windows this software uses long presses.
  - DE: Langdruck-Dauer. Anstelle von Bestätigungsfenstern verwendet diese Software Langdrücke.
  - [ ] OK    Fix: 

- **`reloadComplete`**
  - EN: Reload Complete Configuration from files.
  - DE: Gesamtkonfiguration aus Dateien neu laden.
  - [ ] OK    Fix: 

- **`reloadCompleteBackup`**
  - EN: Reload Complete Configuration from backup files.
  - DE: Gesamtkonfiguration aus Backup-Dateien neu laden.
  - [ ] OK    Fix: 

- **`reloadSystem`**
  - EN: Reload System Configuration from file.
  - DE: Systemkonfiguration aus Datei neu laden.
  - [ ] OK    Fix: 

- **`reloadSystemBackup`**
  - EN: Reload System Configuration from backup file.
  - DE: Systemkonfiguration aus Backup-Datei neu laden.
  - [ ] OK    Fix: 

- **`remoteSetup`**
  - EN: Select the number of pads in the Remote's XY Pads tab.
  - DE: Anzahl der Pads im XY-Pads-Tab des Remote auswählen.
  - [ ] OK    Fix: 

- **`reportIssue`**
  - EN: Open the WFS-DIY GitHub issues page in your default browser.
  - DE: Die WFS-DIY GitHub-Issue-Seite im Standardbrowser öffnen.
  - [ ] OK    Fix: 

- **`reverbChannels`**
  - EN: Number of Reverb Channels.
  - DE: Anzahl der Reverb Channels.
  - [ ] OK    Fix: 

- **`sampler`**
  - EN: Enable or disable the Sampler feature for input channels.
  - DE: Aktivieren oder deaktivieren Sie die Sampler-Funktion für Eingangskanäle. Wählen Sie den Controller: Lightpad oder Fernbedienung.
  - [ ] OK    Fix: 

- **`screenReader`**
  - EN: Enable or disable screen reader announcements. When enabled, parameter names and values are announced on hover, and help text is read after a few seconds.
  - DE: Bildschirmleser-Ansagen aktivieren oder deaktivieren. Bei Aktivierung werden Parameternamen und -werte beim Überfahren angesagt, Hilfetexte nach einigen Sekunden vorgelesen.
  - [ ] OK    Fix: 

- **`selectProjectFolder`**
  - EN: Select the Location of the Current Project Folder where to store files.
  - DE: Wählen Sie den Speicherort des aktuellen Projektordners.
  - [ ] OK    Fix: 

- **`showLocation`**
  - EN: Location of the current show.
  - DE: Ort der aktuellen Show.
  - [ ] OK    Fix: 

- **`showName`**
  - EN: Name of the current show.
  - DE: Name der aktuellen Show.
  - [ ] OK    Fix: 

- **`soloMode`**
  - EN: Single: one input at a time. Multi: multiple inputs simultaneously.
  - DE: Einzeln: ein Eingang nach dem anderen. Mehrfach: mehrere Eingänge gleichzeitig.
  - [ ] OK    Fix: 

- **`speedOfSound`**
  - EN: Speed of Sound (related to the temperature).
  - DE: Speed of Sound (abhängig von der Temperatur).
  - [ ] OK    Fix: 

- **`stageDepth`**
  - EN: Depth of the stage in meters (Box shape only).
  - DE: Tiefe der Bühne in Metern (nur Quaderform).
  - [ ] OK    Fix: 

- **`stageDiameter`**
  - EN: Diameter of the stage in meters (Cylinder and Dome shapes).
  - DE: Durchmesser der Bühne in Metern (Zylinder- und Kuppelform).
  - [ ] OK    Fix: 

- **`stageDismiss`**
  - EN: Long press to dismiss and keep current input positions.
  - DE: Lange drücken, um die aktuellen Eingangspositionen beizubehalten.
  - [ ] OK    Fix: 

- **`stageFit`**
  - EN: Long press to move outlier inputs back within the new stage bounds.
  - DE: Lange drücken, um Eingänge außerhalb der neuen Bühnengrenzen zurückzubringen.
  - [ ] OK    Fix: 

- **`stageHeight`**
  - EN: Height of the stage in meters (Box and Cylinder shapes).
  - DE: Höhe der Bühne in Metern (Quader- und Zylinderform).
  - [ ] OK    Fix: 

- **`stageScale`**
  - EN: Long press to scale all input positions proportionally to the new stage dimensions.
  - DE: Lange drücken, um alle Eingangspositionen proportional zu den neuen Bühnenmaßen zu skalieren.
  - [ ] OK    Fix: 

- **`stageShape`**
  - EN: Select the shape of the stage (Box, Cylinder, or Dome).
  - DE: Wählen Sie die Form der Bühne (Quader, Zylinder oder Kuppel).
  - [ ] OK    Fix: 

- **`stageWidth`**
  - EN: Width of the stage in meters (Box shape only).
  - DE: Breite der Bühne in Metern (nur Quaderform).
  - [ ] OK    Fix: 

- **`storeComplete`**
  - EN: Store Complete Configuration to files (with backup).
  - DE: Gesamtkonfiguration in Dateien speichern (mit Backup).
  - [ ] OK    Fix: 

- **`storeSystem`**
  - EN: Store System Configuration to file (with backup).
  - DE: Systemkonfiguration in Datei speichern (mit Backup).
  - [ ] OK    Fix: 

- **`systemLatency`**
  - EN: Total latency of the system (Mixing board & Computer) / Specific Input and Output Latency/Delay can be set in the respective Input and Output settings.
  - DE: Gesamtlatenz des Systems (Mischpult & Computer) / Spezifische Ein- und Ausgangslatenz kann in den jeweiligen Einstellungen gesetzt werden.
  - [ ] OK    Fix: 

- **`temperature`**
  - EN: Temperature (gives the Speed of Sound).
  - DE: Temperatur (bestimmt die Speed of Sound).
  - [ ] OK    Fix: 

## `systemConfig.messages`

- **`configLoaded`**
  - EN: Complete configuration loaded.
  - DE: Gesamtkonfiguration geladen.
  - [ ] OK    Fix: 

- **`configLoadedFromBackup`**
  - EN: Configuration loaded from backup.
  - DE: Konfiguration aus Backup geladen.
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Complete configuration saved.
  - DE: Gesamtkonfiguration gespeichert.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - DE: Fehler: {error}
  - [ ] OK    Fix: 

- **`languageChanged`**
  - EN: Language changed to: {language} (requires restart for full effect)
  - DE: Sprache geändert zu: {language} (Neustart für volle Wirkung erforderlich)
  - [ ] OK    Fix: 

- **`logDirNotFound`**
  - EN: Log directory not found
  - DE: Protokollverzeichnis nicht gefunden
  - [ ] OK    Fix: 

- **`logsExportFailed`**
  - EN: Failed to export logs
  - DE: Protokollexport fehlgeschlagen
  - [ ] OK    Fix: 

- **`logsExported`**
  - EN: Logs exported to {path}
  - DE: Protokolle exportiert nach {path}
  - [ ] OK    Fix: 

- **`noBackupFilesFound`**
  - EN: No backup files found.
  - DE: Keine Backup-Dateien gefunden.
  - [ ] OK    Fix: 

- **`partialLoad`**
  - EN: Partial load: {error}
  - DE: Teilweise geladen: {error}
  - [ ] OK    Fix: 

- **`partialLoadFromBackup`**
  - EN: Partial load from backup: {error}
  - DE: Teilweise aus Backup geladen: {error}
  - [ ] OK    Fix: 

- **`restartBody`**
  - EN: Please restart the application for the language change to take full effect.
  - DE: Bitte starten Sie die Anwendung neu, damit die Sprachänderung vollständig wirksam wird.
  - [ ] OK    Fix: 

- **`restartTitle`**
  - EN: Restart Required
  - DE: Neustart erforderlich
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder first.
  - DE: Bitte wählen Sie zuerst einen Projektordner aus.
  - [ ] OK    Fix: 

- **`selectLogExportFolder`**
  - EN: Select destination for log export
  - DE: Zielordner für den Protokollexport auswählen
  - [ ] OK    Fix: 

- **`systemConfigExported`**
  - EN: System configuration exported.
  - DE: Systemkonfiguration exportiert.
  - [ ] OK    Fix: 

- **`systemConfigFileNotFound`**
  - EN: System configuration file not found.
  - DE: Systemkonfigurationsdatei nicht gefunden.
  - [ ] OK    Fix: 

- **`systemConfigImported`**
  - EN: System configuration imported.
  - DE: Systemkonfiguration importiert.
  - [ ] OK    Fix: 

- **`systemConfigLoaded`**
  - EN: System configuration loaded.
  - DE: Systemkonfiguration geladen.
  - [ ] OK    Fix: 

- **`systemConfigLoadedFromBackup`**
  - EN: System configuration loaded from backup.
  - DE: Systemkonfiguration aus Backup geladen.
  - [ ] OK    Fix: 

- **`systemConfigSaved`**
  - EN: System configuration saved.
  - DE: Systemkonfiguration gespeichert.
  - [ ] OK    Fix: 

- **`systemInfoCopied`**
  - EN: System info copied to clipboard
  - DE: Systeminfo in die Zwischenablage kopiert
  - [ ] OK    Fix:
