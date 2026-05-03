# Proofreading checklist — German (Deutsch)

Locale: `de`  |  Total keys: 1520  |  Source: `Resources/lang/en.json` vs `Resources/lang/de.json`

## How to use this file

Walk through each section. For every entry:
- If the translation reads naturally and matches the meaning of the English source, leave the `[ ]` checkbox blank or mark `[x] OK`.
- If the translation is wrong, awkward, or has a typo, write the corrected text under `Fix:`.
- If the translation is **identical to English**, that may be intentional: loanwords (Sampler, Tracking, Pre, Post, ON/OFF, LFO, HF, EQ, AI), proper nouns (QLab, Lightpad, Stream Deck, ADM-OSC, MQTT), technical terms (OSC Path:, Localhost), short tokens that match across languages (German System, Spanish Error). Mark `[x] OK` to confirm intent, or write `Fix:` if it should be translated.
- Curly braces `{name}`, `{num}`, `{path}` etc. are runtime placeholders; keep them in the translation.
- `\n` in the value is a literal newline in the rendered UI; preserve it.

---

## `ai.history`

- **`applied`**
  - EN: applied
  - DE: angewendet
  - [ ] OK    Fix: 

- **`atHead`**
  - EN: (no undone records — at the head)
  - DE: (keine rückgängig gemachten Einträge — am Anfang)
  - [ ] OK    Fix: 

- **`batch`**
  - EN: batch {id}
  - DE: Stapel {id}
  - [ ] OK    Fix: 

- **`cursorLabel`**
  - EN: ◂  cursor (↑ applied  /  ↓ undone, redoable)
  - DE: ◂  Cursor (↑ angewendet  /  ↓ rückgängig, wiederherstellbar)
  - [ ] OK    Fix: 

- **`noChanges`**
  - EN: No AI changes yet.
  - DE: Noch keine KI-Änderungen.
  - [ ] OK    Fix: 

- **`of`**
  - EN: of
  - DE: von
  - [ ] OK    Fix: 

- **`stepBack`**
  - EN: ⏮ Step Back
  - DE: ⏮ Zurück
  - [ ] OK    Fix: 

- **`stepForward`**
  - EN: Step Forward ⏭
  - DE: Vor ⏭
  - [ ] OK    Fix: 

- **`undone`**
  - EN: undone
  - DE: rückgängig
  - [ ] OK    Fix: 

- **`windowTitle`**
  - EN: AI Change History
  - DE: KI-Änderungsverlauf
  - [ ] OK    Fix: 

## `ai.server`

- **`copyUrlConfirm`**
  - EN: MCP URL copied to clipboard: {url}
  - DE: MCP-URL in die Zwischenablage kopiert: {url}
  - [ ] OK    Fix: 

- **`label`**
  - EN: MCP Server:
  - DE: MCP-Server:
  - [ ] OK    Fix: 

- **`openHistory`**
  - EN: Open AI History
  - DE: KI-Verlauf öffnen
  - [ ] OK    Fix: 

- **`urlButtonRunning`**
  - EN: http://{host}:{port}/mcp
  - DE: http://{host}:{port}/mcp
  - [ ] OK    Fix: 

- **`urlButtonStopped`**
  - EN: (server not running)
  - DE: (Server läuft nicht)
  - [ ] OK    Fix: 

## `ai.tier`

- **`aiOff`**
  - EN: AI: OFF
  - DE: KI: AUS
  - [ ] OK    Fix: 

- **`aiOn`**
  - EN: AI: ON
  - DE: KI: EIN
  - [ ] OK    Fix: 

- **`gateAllowed`**
  - EN: AI critical actions: ALLOWED
  - DE: Kritische KI-Aktionen: ERLAUBT
  - [ ] OK    Fix: 

- **`gateBlocked`**
  - EN: AI critical actions: blocked
  - DE: Kritische KI-Aktionen: gesperrt
  - [ ] OK    Fix: 

- **`tier2AutoOff`**
  - EN: Tier 2 auto-confirm: off
  - DE: Tier-2-Auto-Bestätigung: aus
  - [ ] OK    Fix: 

- **`tier2AutoOn`**
  - EN: Tier 2 auto-confirm: ON (5 min)
  - DE: Tier-2-Auto-Bestätigung: EIN (5 Min)
  - [ ] OK    Fix: 

## `ai.toast`

- **`clientPrefix`**
  - EN: Claude
  - DE: Claude
  - [ ] OK    Fix: 

- **`header`**
  - EN: AI changes
  - DE: KI-Änderungen
  - [ ] OK    Fix: 

- **`moreOlder`**
  - EN: …and {count} older
  - DE: …und {count} ältere
  - [ ] OK    Fix: 

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

## `ai.undo`

- **`errorPrefix`**
  - EN: AI {verb}: {message}
  - DE: KI {verb}: {message}
  - [ ] OK    Fix: 

- **`successPrefix`**
  - EN: AI {verb}: {description}
  - DE: KI {verb}: {description}
  - [ ] OK    Fix: 

- **`verbRedo`**
  - EN: redo
  - DE: Wiederherstellen
  - [ ] OK    Fix: 

- **`verbUndo`**
  - EN: undo
  - DE: Rückgängig
  - [ ] OK    Fix: 

## `arrayHelper.acoustic`

- **`distanceAtten`**
  - EN: Distance Atten (%):
  - DE: Entf.-Dämpf. (%):
  - [ ] OK    Fix: 

- **`floorReflections`**
  - EN: Floor Reflections
  - DE: Bodenreflexionen
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: HF Damping (dB/m):
  - DE: HF-Dämpfung (dB/m):
  - [ ] OK    Fix: 

- **`highCut`**
  - EN: High Cut (Hz):
  - DE: Hochpass (Hz):
  - [ ] OK    Fix: 

- **`hParallax`**
  - EN: H Parallax (m):
  - DE: H-Parallaxe (m):
  - [ ] OK    Fix: 

- **`liveSource`**
  - EN: Live Source
  - DE: Live-Quelle
  - [ ] OK    Fix: 

- **`lowCut`**
  - EN: Low Cut (Hz):
  - DE: Tiefpass (Hz):
  - [ ] OK    Fix: 

- **`vParallax`**
  - EN: V Parallax (m):
  - DE: V-Parallaxe (m):
  - [ ] OK    Fix: 

## `arrayHelper.buttons`

- **`apply`**
  - EN: Apply
  - DE: Anwenden
  - [ ] OK    Fix: 

- **`close`**
  - EN: Close
  - DE: Schließen
  - [ ] OK    Fix: 

## `arrayHelper.errors`

- **`noPositions`**
  - EN: No positions to apply. Check geometry parameters.
  - DE: Keine Positionen anzuwenden. Überprüfen Sie die Geometrieparameter.
  - [ ] OK    Fix: 

- **`notEnoughOutputs`**
  - EN: Not enough output channels! Need {count} starting from {start}
  - DE: Nicht genug Ausgangskanäle! Benötigt {count} ab {start}
  - [ ] OK    Fix: 

- **`prefix`**
  - EN: Error: 
  - DE: Fehler: 
  - [ ] OK    Fix: 

- **`speakerCountZero`**
  - EN: Number of speakers must be greater than 0
  - DE: Lautsprecheranzahl muss größer als 0 sein
  - [ ] OK    Fix: 

## `arrayHelper.geometry`

- **`backFacing`**
  - EN: Back Facing
  - DE: Nach hinten gerichtet
  - [ ] OK    Fix: 

- **`centerSpacing`**
  - EN: Center + Spacing
  - DE: Mitte + Abstand
  - [ ] OK    Fix: 

- **`centerX`**
  - EN: Center X (m):
  - DE: Mitte X (m):
  - [ ] OK    Fix: 

- **`centerY`**
  - EN: Center Y (m):
  - DE: Mitte Y (m):
  - [ ] OK    Fix: 

- **`endpoints`**
  - EN: Endpoints
  - DE: Endpunkte
  - [ ] OK    Fix: 

- **`endX`**
  - EN: End X (m):
  - DE: Ende X (m):
  - [ ] OK    Fix: 

- **`endY`**
  - EN: End Y (m):
  - DE: Ende Y (m):
  - [ ] OK    Fix: 

- **`facingInward`**
  - EN: Facing Inward
  - DE: Nach innen gerichtet
  - [ ] OK    Fix: 

- **`facingOutward`**
  - EN: Facing Outward
  - DE: Nach außen gerichtet
  - [ ] OK    Fix: 

- **`frontFacing`**
  - EN: Front Facing
  - DE: Nach vorne gerichtet
  - [ ] OK    Fix: 

- **`nPairs`**
  - EN: N Pairs:
  - DE: Anz. Paare:
  - [ ] OK    Fix: 

- **`nSpeakers`**
  - EN: N Speakers:
  - DE: Anz. Lautspr.:
  - [ ] OK    Fix: 

- **`orientation`**
  - EN: Orientation (deg):
  - DE: Orientierung (Grad):
  - [ ] OK    Fix: 

- **`radius`**
  - EN: Radius (m):
  - DE: Radius (m):
  - [ ] OK    Fix: 

- **`sag`**
  - EN: Sag (m):
  - DE: Durchhang (m):
  - [ ] OK    Fix: 

- **`spacing`**
  - EN: Spacing (m):
  - DE: Abstand (m):
  - [ ] OK    Fix: 

- **`startAngle`**
  - EN: Start Angle (deg):
  - DE: Startwinkel (Grad):
  - [ ] OK    Fix: 

- **`startX`**
  - EN: Start X (m):
  - DE: Anfang X (m):
  - [ ] OK    Fix: 

- **`startY`**
  - EN: Start Y (m):
  - DE: Anfang Y (m):
  - [ ] OK    Fix: 

- **`width`**
  - EN: Width (m):
  - DE: Breite (m):
  - [ ] OK    Fix: 

- **`yEnd`**
  - EN: Y End (m):
  - DE: Y Ende (m):
  - [ ] OK    Fix: 

- **`yStart`**
  - EN: Y Start (m):
  - DE: Y Anfang (m):
  - [ ] OK    Fix: 

- **`zHeight`**
  - EN: Z Height (m):
  - DE: Z-Höhe (m):
  - [ ] OK    Fix: 

## `arrayHelper.presets`

- **`circle`**
  - EN: Circle
  - DE: Kreis
  - [ ] OK    Fix: 

- **`delayLine`**
  - EN: Delay Line
  - DE: Delay-Line
  - [ ] OK    Fix: 

- **`label`**
  - EN: Preset:
  - DE: Vorlage:
  - [ ] OK    Fix: 

- **`mainRoomStraight`**
  - EN: Main Flown Array Straight
  - DE: Geflogenes Haupt-Array gerade
  - [ ] OK    Fix: 

- **`nearFieldCurved`**
  - EN: Near Field Array Curved
  - DE: Nahfeld-Array gebogen
  - [ ] OK    Fix: 

- **`nearFieldStraight`**
  - EN: Near Field Array Straight
  - DE: Nahfeld-Array gerade
  - [ ] OK    Fix: 

- **`subBass`**
  - EN: Sub Bass
  - DE: Subbass
  - [ ] OK    Fix: 

- **`surround`**
  - EN: Surround
  - DE: Surround
  - [ ] OK    Fix: 

## `arrayHelper.preview`

- **`audience`**
  - EN: Audience
  - DE: Publikum
  - [ ] OK    Fix: 

## `arrayHelper.sections`

- **`acousticDefaults`**
  - EN: Acoustic Defaults
  - DE: Akustische Vorgaben
  - [ ] OK    Fix: 

- **`geometry`**
  - EN: Geometry
  - DE: Geometrie
  - [ ] OK    Fix: 

- **`target`**
  - EN: Target
  - DE: Ziel
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

## `arrayHelper.target`

- **`array`**
  - EN: Array:
  - DE: Array:
  - [ ] OK    Fix: 

- **`arrayPrefix`**
  - EN: Array
  - DE: Array
  - [ ] OK    Fix: 

- **`startingOutput`**
  - EN: Starting Output:
  - DE: Startausgang:
  - [ ] OK    Fix: 

## `arrayHelper.window`

- **`contentName`**
  - EN: Output Array Helper
  - DE: Ausgangs-Array-Assistent
  - [ ] OK    Fix: 

- **`title`**
  - EN: Wizard of OutZ
  - DE: Assistent OutZ
  - [ ] OK    Fix: 

## `audioPatch`

- **`windowTitle`**
  - EN: Audio Interface and Patching
  - DE: Audio-Interface und Patch
  - [ ] OK    Fix: 

## `audioPatch.buttons`

- **`hold`**
  - EN: Hold
  - DE: Halten
  - [ ] OK    Fix: 

- **`unpatchAll`**
  - EN: Unpatch All
  - DE: Alle trennen
  - [ ] OK    Fix: 

## `audioPatch.deviceSettings`

- **`buttons.controlPanel`**
  - EN: Control Panel
  - DE: Systemsteuerung
  - [ ] OK    Fix: 

- **`buttons.resetDevice`**
  - EN: Reset Device
  - DE: Gerät zurücksetzen
  - [ ] OK    Fix: 

- **`labels.bufferSize`**
  - EN: Audio buffer size:
  - DE: Audio-Puffergröße:
  - [ ] OK    Fix: 

- **`labels.device`**
  - EN: Device:
  - DE: Gerät:
  - [ ] OK    Fix: 

- **`labels.deviceType`**
  - EN: Audio device type:
  - DE: Audio-Gerätetyp:
  - [ ] OK    Fix: 

- **`labels.sampleRate`**
  - EN: Sample rate:
  - DE: Abtastrate:
  - [ ] OK    Fix: 

- **`noDevice`**
  - EN: No Device
  - DE: Kein Gerät
  - [ ] OK    Fix: 

- **`notConfigured`**
  - EN: Not configured
  - DE: Nicht konfiguriert
  - [ ] OK    Fix: 

## `audioPatch.labels`

- **`interfaceInput`**
  - EN: Audio Interface Input
  - DE: Audio-Interface Eingang
  - [ ] OK    Fix: 

- **`interfaceOutput`**
  - EN: Audio Interface Output
  - DE: Audio-Interface Ausgang
  - [ ] OK    Fix: 

- **`processorInputs`**
  - EN: Processor Inputs
  - DE: Prozessor-Eingänge
  - [ ] OK    Fix: 

- **`processorOutputs`**
  - EN: Processor Outputs
  - DE: Prozessor-Ausgänge
  - [ ] OK    Fix: 

## `audioPatch.messages`

- **`chooseTestSignal`**
  - EN: Choose a Test Signal to Enable Testing
  - DE: Wählen Sie ein Testsignal zum Aktivieren des Testmodus
  - [ ] OK    Fix: 

## `audioPatch.modes`

- **`patching`**
  - EN: Patching
  - DE: Patchen
  - [ ] OK    Fix: 

- **`scrolling`**
  - EN: Scrolling
  - DE: Scrollen
  - [ ] OK    Fix: 

- **`testing`**
  - EN: Testing
  - DE: Testen
  - [ ] OK    Fix: 

## `audioPatch.tabs`

- **`deviceSettings`**
  - EN: Device Settings
  - DE: Geräteeinstellungen
  - [ ] OK    Fix: 

- **`inputPatch`**
  - EN: Input Patch
  - DE: Eingangs-Patch
  - [ ] OK    Fix: 

- **`outputPatch`**
  - EN: Output Patch
  - DE: Ausgangs-Patch
  - [ ] OK    Fix: 

## `audioPatch.testSignal`

- **`labels.frequency`**
  - EN: Frequency:
  - DE: Frequenz:
  - [ ] OK    Fix: 

- **`labels.level`**
  - EN: Level:
  - DE: Pegel:
  - [ ] OK    Fix: 

- **`labels.signal`**
  - EN: Signal:
  - DE: Signal:
  - [ ] OK    Fix: 

- **`types.diracPulse`**
  - EN: Dirac Pulse
  - DE: Dirac-Impuls
  - [ ] OK    Fix: 

- **`types.off`**
  - EN: Off
  - DE: Aus
  - [ ] OK    Fix: 

- **`types.pinkNoise`**
  - EN: Pink Noise
  - DE: Rosa Rauschen
  - [ ] OK    Fix: 

- **`types.pulse`**
  - EN: Pulse
  - DE: Impuls
  - [ ] OK    Fix: 

- **`types.sweep`**
  - EN: Sweep
  - DE: Sweep
  - [ ] OK    Fix: 

- **`types.tone`**
  - EN: Tone
  - DE: Ton
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
  - EN: Select the reference point for cluster transforms: First Input or Barycenter.
  - DE: Wählen Sie den Referenzpunkt für Cluster-Transformationen: Erster Eingang oder Schwerpunkt.
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

## `clusters.labels`

- **`assignedInputs`**
  - EN: Assigned Inputs
  - DE: Zugewiesene Eingänge
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Atten
  - DE: Dämpf.
  - [ ] OK    Fix: 

- **`controls`**
  - EN: Controls
  - DE: Steuerung
  - [ ] OK    Fix: 

- **`inputPrefix`**
  - EN: Input
  - DE: Eingang
  - [ ] OK    Fix: 

- **`position`**
  - EN: Position
  - DE: Position
  - [ ] OK    Fix: 

- **`posPrefix`**
  - EN: Pos:
  - DE: Pos:
  - [ ] OK    Fix: 

- **`reference`**
  - EN: Reference:
  - DE: Referenz:
  - [ ] OK    Fix: 

- **`rotation`**
  - EN: Rotation
  - DE: Drehung
  - [ ] OK    Fix: 

- **`scale`**
  - EN: Scale
  - DE: Skalierung
  - [ ] OK    Fix: 

- **`x`**
  - EN: X:
  - DE: X:
  - [ ] OK    Fix: 

- **`y`**
  - EN: Y:
  - DE: Y:
  - [ ] OK    Fix: 

- **`z`**
  - EN: Z
  - DE: Z
  - [ ] OK    Fix: 

## `clusters.lfo`

- **`labels.amplitude`**
  - EN: Amplitude:
  - DE: Amplitude:
  - [ ] OK    Fix: 

- **`labels.angle`**
  - EN: Angle:
  - DE: Winkel:
  - [ ] OK    Fix: 

- **`labels.period`**
  - EN: Period:
  - DE: Periode:
  - [ ] OK    Fix: 

- **`labels.phase`**
  - EN: Phase:
  - DE: Phase:
  - [ ] OK    Fix: 

- **`labels.rate`**
  - EN: Rate:
  - DE: Rate:
  - [ ] OK    Fix: 

- **`labels.ratio`**
  - EN: Ratio:
  - DE: Verhältnis:
  - [ ] OK    Fix: 

- **`labels.rot`**
  - EN: Rotation
  - DE: Drehung
  - [ ] OK    Fix: 

- **`labels.scaleLfo`**
  - EN: Scale
  - DE: Skalierung
  - [ ] OK    Fix: 

- **`labels.section`**
  - EN: LFO
  - DE: LFO
  - [ ] OK    Fix: 

- **`labels.x`**
  - EN: X
  - DE: X
  - [ ] OK    Fix: 

- **`labels.y`**
  - EN: Y
  - DE: Y
  - [ ] OK    Fix: 

- **`labels.z`**
  - EN: Z
  - DE: Z
  - [ ] OK    Fix: 

## `clusters.osc`

- **`lfoActive`**
  - EN: /wfs/cluster/lfoActive <id> <0|1>
  - DE: /wfs/cluster/lfoActive <id> <0|1>
  - [ ] OK    Fix: 

- **`lfoAmplitudeRot`**
  - EN: /wfs/cluster/lfoAmplitudeRot <id> <degrees>
  - DE: /wfs/cluster/lfoAmplitudeRot <id> <Grad>
  - [ ] OK    Fix: 

- **`lfoAmplitudeScale`**
  - EN: /wfs/cluster/lfoAmplitudeScale <id> <factor>
  - DE: /wfs/cluster/lfoAmplitudeScale <id> <Faktor>
  - [ ] OK    Fix: 

- **`lfoAmplitudeX`**
  - EN: /wfs/cluster/lfoAmplitudeX <id> <meters>
  - DE: /wfs/cluster/lfoAmplitudeX <id> <Meter>
  - [ ] OK    Fix: 

- **`lfoAmplitudeY`**
  - EN: /wfs/cluster/lfoAmplitudeY <id> <meters>
  - DE: /wfs/cluster/lfoAmplitudeY <id> <Meter>
  - [ ] OK    Fix: 

- **`lfoAmplitudeZ`**
  - EN: /wfs/cluster/lfoAmplitudeZ <id> <meters>
  - DE: /wfs/cluster/lfoAmplitudeZ <id> <Meter>
  - [ ] OK    Fix: 

- **`lfoPeriod`**
  - EN: /wfs/cluster/lfoPeriod <id> <seconds>
  - DE: /wfs/cluster/lfoPeriod <id> <Sekunden>
  - [ ] OK    Fix: 

- **`lfoPhase`**
  - EN: /wfs/cluster/lfoPhase <id> <degrees>
  - DE: /wfs/cluster/lfoPhase <id> <Grad>
  - [ ] OK    Fix: 

- **`lfoPhaseRot`**
  - EN: /wfs/cluster/lfoPhaseRot <id> <degrees>
  - DE: /wfs/cluster/lfoPhaseRot <id> <Grad>
  - [ ] OK    Fix: 

- **`lfoPhaseScale`**
  - EN: /wfs/cluster/lfoPhaseScale <id> <degrees>
  - DE: /wfs/cluster/lfoPhaseScale <id> <Grad>
  - [ ] OK    Fix: 

- **`lfoPhaseX`**
  - EN: /wfs/cluster/lfoPhaseX <id> <degrees>
  - DE: /wfs/cluster/lfoPhaseX <id> <Grad>
  - [ ] OK    Fix: 

- **`lfoPhaseY`**
  - EN: /wfs/cluster/lfoPhaseY <id> <degrees>
  - DE: /wfs/cluster/lfoPhaseY <id> <Grad>
  - [ ] OK    Fix: 

- **`lfoPhaseZ`**
  - EN: /wfs/cluster/lfoPhaseZ <id> <degrees>
  - DE: /wfs/cluster/lfoPhaseZ <id> <Grad>
  - [ ] OK    Fix: 

- **`lfoPresetRecall`**
  - EN: /wfs/cluster/lfoPresetRecall <clusterId> <presetNumber>
  - DE: /wfs/cluster/lfoPresetRecall <clusterId> <PresetNummer>
  - [ ] OK    Fix: 

- **`lfoRateRot`**
  - EN: /wfs/cluster/lfoRateRot <id> <multiplier>
  - DE: /wfs/cluster/lfoRateRot <id> <Multiplikator>
  - [ ] OK    Fix: 

- **`lfoRateScale`**
  - EN: /wfs/cluster/lfoRateScale <id> <multiplier>
  - DE: /wfs/cluster/lfoRateScale <id> <Multiplikator>
  - [ ] OK    Fix: 

- **`lfoRateX`**
  - EN: /wfs/cluster/lfoRateX <id> <multiplier>
  - DE: /wfs/cluster/lfoRateX <id> <Multiplikator>
  - [ ] OK    Fix: 

- **`lfoRateY`**
  - EN: /wfs/cluster/lfoRateY <id> <multiplier>
  - DE: /wfs/cluster/lfoRateY <id> <Multiplikator>
  - [ ] OK    Fix: 

- **`lfoRateZ`**
  - EN: /wfs/cluster/lfoRateZ <id> <multiplier>
  - DE: /wfs/cluster/lfoRateZ <id> <Multiplikator>
  - [ ] OK    Fix: 

- **`lfoShapeRot`**
  - EN: /wfs/cluster/lfoShapeRot <id> <0-8>
  - DE: /wfs/cluster/lfoShapeRot <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoShapeScale`**
  - EN: /wfs/cluster/lfoShapeScale <id> <0-8>
  - DE: /wfs/cluster/lfoShapeScale <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoShapeX`**
  - EN: /wfs/cluster/lfoShapeX <id> <0-8>
  - DE: /wfs/cluster/lfoShapeX <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoShapeY`**
  - EN: /wfs/cluster/lfoShapeY <id> <0-8>
  - DE: /wfs/cluster/lfoShapeY <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoShapeZ`**
  - EN: /wfs/cluster/lfoShapeZ <id> <0-8>
  - DE: /wfs/cluster/lfoShapeZ <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoStopAll`**
  - EN: /wfs/cluster/lfoStopAll
  - DE: /wfs/cluster/lfoStopAll
  - [ ] OK    Fix: 

## `clusters.planes`

- **`xy`**
  - EN: XY
  - DE: XY
  - [ ] OK    Fix: 

- **`xz`**
  - EN: XZ
  - DE: XZ
  - [ ] OK    Fix: 

- **`yz`**
  - EN: YZ
  - DE: YZ
  - [ ] OK    Fix: 

## `clusters.presets`

- **`export`**
  - EN: Export
  - DE: Exportieren
  - [ ] OK    Fix: 

- **`exportDialog`**
  - EN: Export LFO Presets
  - DE: LFO-Presets exportieren
  - [ ] OK    Fix: 

- **`exported`**
  - EN: LFO presets exported.
  - DE: LFO-Presets exportiert.
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - DE: Importieren
  - [ ] OK    Fix: 

- **`importDialog`**
  - EN: Import LFO Presets
  - DE: LFO-Presets importieren
  - [ ] OK    Fix: 

- **`imported`**
  - EN: LFO presets imported.
  - DE: LFO-Presets importiert.
  - [ ] OK    Fix: 

- **`recalled`**
  - EN: LFO preset recalled from tile {n}.
  - DE: LFO-Preset aus Kachel {n} abgerufen.
  - [ ] OK    Fix: 

- **`stopAll`**
  - EN: Stop All
  - DE: Alle stoppen
  - [ ] OK    Fix: 

- **`stored`**
  - EN: LFO preset stored in tile {n}.
  - DE: LFO-Preset in Kachel {n} gespeichert.
  - [ ] OK    Fix: 

## `clusters.referenceMode`

- **`barycenter`**
  - EN: Barycenter
  - DE: Schwerpunkt
  - [ ] OK    Fix: 

- **`firstInput`**
  - EN: First Input
  - DE: Erster Eingang
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

## `clusters.toggles`

- **`inputsHidden`**
  - EN: Inputs: Hidden
  - DE: Eingänge: Versteckt
  - [ ] OK    Fix: 

- **`inputsVisible`**
  - EN: Inputs: Visible
  - DE: Eingänge: Sichtbar
  - [ ] OK    Fix: 

- **`lfoOff`**
  - EN: L.F.O: OFF
  - DE: L.F.O: AUS
  - [ ] OK    Fix: 

- **`lfoOn`**
  - EN: L.F.O: ON
  - DE: L.F.O: EIN
  - [ ] OK    Fix: 

## `common`

- **`cancel`**
  - EN: Cancel
  - DE: Abbrechen
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

## `eq.buttons`

- **`flattenEQ`**
  - EN: Flatten EQ
  - DE: EQ zurücksetzen
  - [ ] OK    Fix: 

- **`resetBand`**
  - EN: Reset
  - DE: Reset
  - [ ] OK    Fix: 

## `eq.filterTypes`

- **`allPass`**
  - EN: All-Pass
  - DE: Allpass
  - [ ] OK    Fix: 

- **`bandPass`**
  - EN: Band Pass
  - DE: Bandpass
  - [ ] OK    Fix: 

- **`highCut`**
  - EN: High Cut
  - DE: Hochpassfilter
  - [ ] OK    Fix: 

- **`highShelf`**
  - EN: High Shelf
  - DE: Höhenregler
  - [ ] OK    Fix: 

- **`lowCut`**
  - EN: Low Cut
  - DE: Tiefpassfilter
  - [ ] OK    Fix: 

- **`lowShelf`**
  - EN: Low Shelf
  - DE: Tiefenregler
  - [ ] OK    Fix: 

- **`peakNotch`**
  - EN: Peak/Notch
  - DE: Glocke/Kerbe
  - [ ] OK    Fix: 

## `eq.labels`

- **`band`**
  - EN: Band
  - DE: Band
  - [ ] OK    Fix: 

- **`freq`**
  - EN: Freq:
  - DE: Freq:
  - [ ] OK    Fix: 

- **`gain`**
  - EN: Gain
  - DE: Gain
  - [ ] OK    Fix: 

- **`q`**
  - EN: Q
  - DE: Q
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

## `fileManager.errors`

- **`backupNotFound`**
  - EN: Backup not found
  - DE: Sicherung nicht gefunden
  - [ ] OK    Fix: 

- **`configStateInvalid`**
  - EN: Config state is invalid
  - DE: Konfigurationszustand ist ungültig
  - [ ] OK    Fix: 

- **`failedApply`**
  - EN: Failed to apply: {sections}
  - DE: Anwenden fehlgeschlagen: {sections}
  - [ ] OK    Fix: 

- **`failedCreateFolder`**
  - EN: Failed to create project folder: {path}
  - DE: Erstellung des Projektordners fehlgeschlagen: {path}
  - [ ] OK    Fix: 

- **`failedCreateValueTree`**
  - EN: Failed to create ValueTree from XML: {path}
  - DE: ValueTree aus XML erstellen fehlgeschlagen: {path}
  - [ ] OK    Fix: 

- **`failedCreateXML`**
  - EN: Failed to create XML from state
  - DE: XML aus Zustand erstellen fehlgeschlagen
  - [ ] OK    Fix: 

- **`failedParseXML`**
  - EN: Failed to parse XML file: {path}
  - DE: XML-Datei parsen fehlgeschlagen: {path}
  - [ ] OK    Fix: 

- **`failedWriteFile`**
  - EN: Failed to write file: {path}
  - DE: Datei schreiben fehlgeschlagen: {path}
  - [ ] OK    Fix: 

- **`fileNotFound`**
  - EN: File not found: {path}
  - DE: Datei nicht gefunden: {path}
  - [ ] OK    Fix: 

- **`invalidConfigStructure`**
  - EN: Invalid configuration file structure
  - DE: Ungültige Struktur der Konfigurationsdatei
  - [ ] OK    Fix: 

- **`noInputDataInFile`**
  - EN: No input data found in file
  - DE: Keine Eingangsdaten in der Datei gefunden
  - [ ] OK    Fix: 

- **`noInputDataInSnapshot`**
  - EN: No input data in snapshot
  - DE: Keine Eingangsdaten im Snapshot
  - [ ] OK    Fix: 

- **`noLFOPresetDataInFile`**
  - EN: No LFO preset data found in file
  - DE: Keine LFO-Preset-Daten in der Datei gefunden
  - [ ] OK    Fix: 

- **`noNetworkDataInFile`**
  - EN: No network data found in file
  - DE: Keine Netzwerkdaten in der Datei gefunden
  - [ ] OK    Fix: 

- **`noNetworkSections`**
  - EN: No network sections found in file
  - DE: Keine Netzwerk-Abschnitte in der Datei gefunden
  - [ ] OK    Fix: 

- **`noOutputDataInFile`**
  - EN: No output data found in file
  - DE: Keine Ausgangsdaten in der Datei gefunden
  - [ ] OK    Fix: 

- **`noProjectFolder`**
  - EN: No project folder specified
  - DE: Kein Projektordner angegeben
  - [ ] OK    Fix: 

- **`noReverbDataInFile`**
  - EN: No reverb data found in file
  - DE: Keine Hall-Daten in der Datei gefunden
  - [ ] OK    Fix: 

- **`noSystemDataInFile`**
  - EN: No valid system data found in file: {path}
  - DE: Keine gültigen Systemdaten in der Datei gefunden: {path}
  - [ ] OK    Fix: 

- **`noValidProjectFolder`**
  - EN: No valid project folder
  - DE: Kein gültiger Projektordner
  - [ ] OK    Fix: 

- **`prefixInputs`**
  - EN: Inputs: 
  - DE: Eingänge: 
  - [ ] OK    Fix: 

- **`prefixNetwork`**
  - EN: Network: 
  - DE: Netzwerk: 
  - [ ] OK    Fix: 

- **`prefixOutputs`**
  - EN: Outputs: 
  - DE: Ausgänge: 
  - [ ] OK    Fix: 

- **`prefixReverbs`**
  - EN: Reverbs: 
  - DE: Hall: 
  - [ ] OK    Fix: 

- **`prefixSystem`**
  - EN: System: 
  - DE: System: 
  - [ ] OK    Fix: 

- **`snapshotNotFound`**
  - EN: Snapshot not found
  - DE: Snapshot nicht gefunden
  - [ ] OK    Fix: 

- **`snapshotNotFoundNamed`**
  - EN: Snapshot not found: {name}
  - DE: Snapshot nicht gefunden: {name}
  - [ ] OK    Fix: 

## `help.admOsc`

- **`body`**
  - EN: ADM-OSC is a protocol aiming to improve interoperability for spatial sound. It sends Cartesian positions (X, Y, Z) or polar values (AED for Azimuth, Elevation, Distance) from the console or from a DAW's automation curves.
Data is sent normalised:
- between -1.0 and 1.0 for X, Y and Z;
- between 0.0 to 1.0 for distance,
- between -180° to 180° for Azimuth
- between -90° to 90° for elevation.
The origin point can be moved and the mapping can also be adjusted in different segments for the inner and outer parts of the stage.
When dragging the handles on the graphs, holding the shift key will apply symmetrical adjustments on the opposite side.
  - DE: ADM-OSC ist ein Protokoll zur Verbesserung der Interoperabilität für räumlichen Klang. Es sendet kartesische Positionen (X, Y, Z) oder polare Werte (AED für Azimut, Elevation, Distanz) von der Konsole oder den Automationskurven einer DAW.
Die Daten werden normalisiert gesendet:
- zwischen -1,0 und 1,0 für X, Y und Z;
- zwischen 0,0 und 1,0 für Distanz,
- zwischen -180° und 180° für Azimut
- zwischen -90° und 90° für Elevation.
Der Ursprungspunkt kann verschoben und das Mapping in verschiedenen Segmenten für den inneren und äußeren Bühnenbereich angepasst werden.
Beim Ziehen der Griffe in den Diagrammen werden mit gedrückter Umschalttaste symmetrische Anpassungen auf der gegenüberliegenden Seite vorgenommen.
  - [ ] OK    Fix: 

- **`title`**
  - EN: ADM-OSC Mappings
  - DE: ADM-OSC-Zuordnungen
  - [ ] OK    Fix: 

## `help.automOtion`

- **`body`**
  - EN: One time movement can also be programmed and triggered manually or triggered by sound level.
The coordinates are either relative from the start position or absolute relative to the origin point.
The input can either stay at the end position or revert to the starting position.
Input position cannot be changed while the input is moving this way, but interacting with the input will change the position offset of the input instead.
For audio level triggering select the sound level above which the movement will start. When the sound drops below the reset level the movement will be rearmed, ready to be triggered again. New movement automation will not be possible while the input is moving.
  - DE: Einmalige Bewegungen können programmiert und manuell oder durch Schallpegel ausgelöst werden.
Die Koordinaten sind entweder relativ zur Startposition oder absolut zum Ursprungspunkt.
Der Eingang kann an der Endposition bleiben oder zur Startposition zurückkehren.
Die Position kann während der Bewegung nicht geändert werden, aber Interaktion ändert den Positionsversatz.
Für Pegelauslösung wählen Sie den Schwellwert. Wenn der Pegel unter den Rücksetzpegel fällt, wird die Bewegung wieder bereitgestellt.
  - [ ] OK    Fix: 

- **`title`**
  - EN: AutomOtion
  - DE: AutomOtion
  - [ ] OK    Fix: 

## `help.binaural`

- **`body`**
  - EN: The Binaural renderer is used for:
- listening to a rough spatial mix on headphones,
- creating a mix for stereo output,
- listening to a single soloed track through the spatial processing.
This may take the place of your master mix if it's only feeding headphones and media mix.
The position of the listening position may be adjusted in depth from the origin point and in orientation. Delay and level settings allow you to eventually match the sound at the FOH position.
  - DE: Der Binaural-Renderer wird verwendet für:
- das Abhören eines groben räumlichen Mixes über Kopfhörer,
- das Erstellen eines Mixes für Stereoausgabe,
- das Abhören einer einzelnen Solo-Spur durch die räumliche Verarbeitung.
Er kann Ihren Master-Mix ersetzen, wenn dieser nur Kopfhörer und Medienmix versorgt.
Die Hörposition kann in der Tiefe vom Ursprungspunkt und in der Orientierung angepasst werden. Delay- und Pegeleinstellungen ermöglichen die Anpassung an die FOH-Position.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Binaural Renderer
  - DE: Binaural-Renderer
  - [ ] OK    Fix: 

## `help.clusters`

- **`body`**
  - EN: Clusters are groups of inputs that can be manipulated and animated as a whole.
Each input can only be part of one cluster.
Each cluster can only have one input with tracking fully enabled. Then this input will become the reference points for the cluster.
If no input with tracking is part of the cluster then there are two modes for the reference point of the cluster. Either the first input assigned in the list becomes the reference or the barycentre, in other words the center of gravity or the middle of the shape formed by the assigned inputs.
All inputs of the clusters can be moved by dragging the reference point. The individual inputs (other than a first input that would be a reference point) can still be adjusted individually. Dragging an input with tracking activated that is also a reference point for a cluster will affect its position offset and the position of the other inputs of the cluster normally.
All inputs in a cluster can be rotated or scaled around the reference point.
All clusters can be assigned an animation via an LFO. The positions X, Y and Z, the rotation and scale of the cluster can be controlled. The LFO has a period and a phase setting. Each individual parameter has shape, amplitude, rate and phase. The LFO settings can be assigned to pads for a quick recall. A right click will store the LFO parameters to a pad. Double clicking the top of the pad will allow to edit the name of the preset. Clicking or tapping a pad will recall the settings whether the LFO is running or not, but it will not start it if is isn't. A double click/tap will load and start the LFO.
All input clusters share the same set of LFO presets.
  - DE: Cluster sind Gruppen von Eingängen, die als Ganzes manipuliert und animiert werden können.
Jeder Eingang kann nur Teil eines Clusters sein.
Jeder Cluster kann nur einen Eingang mit vollständig aktiviertem Tracking haben. Dieser wird zum Referenzpunkt des Clusters.
Wenn kein Eingang mit Tracking vorhanden ist, gibt es zwei Modi: entweder der erste zugewiesene Eingang oder der Schwerpunkt der zugewiesenen Eingänge.
Alle Eingänge können durch Ziehen des Referenzpunkts verschoben werden. Einzelne Eingänge können weiterhin individuell angepasst werden. Das Ziehen eines Eingangs mit aktiviertem Tracking, der auch Referenzpunkt ist, beeinflusst seinen Positionsversatz und die Position der anderen Eingänge des Clusters normal.
Alle Eingänge eines Clusters können um den Referenzpunkt rotiert oder skaliert werden.
Allen Clustern kann eine Animation über einen LFO zugewiesen werden. Positionen X, Y, Z, Rotation und Skalierung können gesteuert werden. LFO-Einstellungen können Pads zugewiesen werden. Ein Rechtsklick speichert die LFO-Parameter in einem Pad. Doppelklick auf den oberen Bereich des Pads ermöglicht die Bearbeitung des Preset-Namens. Klicken oder Tippen auf ein Pad ruft die Einstellungen ab, ob der LFO läuft oder nicht, startet ihn aber nicht. Ein Doppelklick/Doppeltipp lädt und startet den LFO.
Alle Cluster teilen sich denselben Satz von LFO-Presets.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Clusters
  - DE: Cluster
  - [ ] OK    Fix: 

## `help.floorReflections`

- **`body`**
  - EN: Simulating the floor reflections improves the naturalness of the sound. We don't expect sounds to play in a soundproof anechoic chamber. This setting helps recreate the expected floor reflections.
The level of the floor reflections can be adjusted as well as the low cut and the high frequency shelf filters. Diffusion adds a bit of chaos to simulate the unevenness of the floor.
  - DE: Die Simulation von Bodenreflexionen verbessert die Natürlichkeit des Klangs. Wir erwarten nicht, dass Klänge in einem schalltoten Raum wiedergegeben werden. Diese Einstellung hilft, die erwarteten Bodenreflexionen nachzubilden.
Der Pegel der Bodenreflexionen kann ebenso angepasst werden wie die Tiefensperre und die Hochfrequenz-Shelf-Filter. Diffusion fügt etwas Chaos hinzu, um die Unebenheiten des Bodens zu simulieren.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Floor Reflections
  - DE: Bodenreflexionen
  - [ ] OK    Fix: 

## `help.gradientMap`

- **`body`**
  - EN: Gradient maps allow to control attenuation, height and high frequency filtering (shelf with a smooth slope centered at 1kHz) depending on the X, Y position. For example, you can fade out a sound when entering a certain zone, you can have high frequency roll-off when moving away from the front of the stage, you can automatically adjust the height of an actor even when they are standing on elevated platforms without having to control height manually.
There are three layers for attenuation, height and HF shelf. They can be toggled on and off and they also can be hidden. The focused layer will look dimmed if disabled. Unfocused layers will look dimmed if active and only the shape outlines will be visible if they are deactivated.
Each layer has a mapping control for white and black to adjust the range of the effect. The curve setting adjusts the transition.
Each layer can have editable shapes (rectangle, ellipse or polygon) with either a single shade of grey, a linear gradient or a radial gradient. End points of the gradients can be adjusted.
When creating a polygon click for each corner. Double-clicking will create a last corner and close the shape.
Double-clicking an existing point on a rectangle or a polygon will remove this corner. Double-clicking on a side will add a new point.
The scale and rotation of each shape can be edited for its center or from the origin point.
When enabled the corner points of the rectangles and polygons can also be edited individually.
Shapes and layers can be copied to another layer on the same input or any other input.
Gradient map settings are stored in the input files.
  - DE: Gradientenkarten ermöglichen die Steuerung von Dämpfung, Höhe und Hochfrequenzfilterung (Shelf mit sanfter Flanke bei 1kHz) abhängig von der X, Y Position. Zum Beispiel können Sie einen Klang beim Betreten einer Zone ausblenden, Höhenabfall beim Entfernen von der Bühnenvorderkante anwenden oder die Höhe eines Schauspielers auf erhöhten Plattformen automatisch anpassen.
Drei Ebenen stehen zur Verfügung: Dämpfung, Höhe und HF-Shelf. Sie können ein-/ausgeschaltet und ausgeblendet werden.
Jede Ebene hat Mapping-Regler für Weiß und Schwarz zur Bereichsanpassung. Die Kurveneinstellung steuert den Übergang.
Jede Ebene kann editierbare Formen haben (Rechteck, Ellipse oder Polygon) mit einheitlichem Grau, linearem oder radialem Gradient.
Beim Erstellen eines Polygons klicken Sie für jede Ecke. Doppelklick schließt die Form.
Doppelklick auf einen Punkt entfernt ihn. Doppelklick auf eine Seite fügt einen neuen Punkt hinzu.
Skalierung und Rotation können vom Zentrum oder Ursprung aus bearbeitet werden.
Formen und Ebenen können kopiert werden.
Einstellungen werden in den Eingangsdateien gespeichert.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Gradient Maps
  - DE: Gradientenkarten
  - [ ] OK    Fix: 

## `help.inputAdvanced`

- **`body`**
  - EN: - Sidelines and Fringe enable muting when an input comes close to the limits of a rectangular stage (except downstage).
- Tracking can be enabled and the ID of the tracker can be selected. The amount of smoothing on the position can also be adjusted.
- Maximum Speed can be engaged and the speed limit adjusted. The system will apply a gradual acceleration and deceleration when the movement starts and finishes. When Path is enabled, the system will track the path followed by the input and will not go in a straight line to the final position. This is especially handy if movements have to be operated manually.
- Height Factor lets you work in 2D, when set to 0%, or full 3D, when set at 100%, and anything in between. It's the ratio of the height in the level and delay calculations. If you wish to use floor reflections, set it to 100% and use the parallax correction of the output parameters.
  - DE: - Seitenlinien und Randbereich ermöglichen die Stummschaltung, wenn ein Eingang sich den Grenzen einer rechteckigen Bühne nähert (ausser Publikumsseite).
- Tracking kann aktiviert und die Tracker-ID ausgewählt werden. Die Positionsglättung kann ebenfalls angepasst werden.
- Die Höchstgeschwindigkeit kann engagiert und die Geschwindigkeitsgrenze angepasst werden. Das System wendet eine graduelle Beschleunigung und Verzögerung am Anfang und Ende der Bewegung an. Wenn der Pfadmodus aktiviert ist, folgt das System dem vom Eingang genommenen Pfad und geht nicht in gerader Linie zur Endposition. Das ist besonders nützlich, wenn Bewegungen manuell gesteuert werden sollen.
- Der Höhenfaktor erlaubt das Arbeiten in 2D, wenn auf 0 % gesetzt, oder in vollem 3D, wenn auf 100 % gesetzt, und alles dazwischen. Das ist das Verhältnis der Höhe in den Pegel- und Verzögerungsberechnungen. Wenn Sie Bodenreflexionen verwenden möchten, stellen Sie es auf 100 % und nutzen Sie die Parallaxenkorrektur in den Ausgangsparametern.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Advanced Controls
  - DE: Erweiterte Steuerung
  - [ ] OK    Fix: 

## `help.inputBasic`

- **`body`**
  - EN: Inputs have a wide variety of settings to account for different situations necessitating realistic sound reinforcement or creative tools for sound design.
- Input level can be adjusted.
- Inputs can be delayed or they can try to take into account specific latency (digital processing of wireless transmission or digital effects) and compensate for it to better align the amplification and the acoustic sound.
- Minimal Latency can be toggled instead of Acoustic Precedence. On the other hand this tries to let the sound out through the system as soon as possible. The system scans this input's feeds to the outputs for lowest delay and subtracts it from all delays and bypasses additional Haas effect. Here the idea would be to beat the acoustic sound on stage to try and place a sound in a slightly different position by altering the location first perceived.
- The location (position and offset) for any input can be given in Cartesian, Cylindrical or Spherical coordinates independently from the stage shape or other channels.
- The position can be constrained to the dimensions of the stage in Cartesian coordinates or to a specific distance range in polar coordinates.
- Flip will take symmetrical position for the given coordinate around the origin point.
- The joystick and vertical slider allow relative control of the position.
- Inputs can be assigned to a cluster to group them for coordinated movements.
  - DE: Eingänge verfügen über eine Vielzahl von Einstellungen für verschiedene Situationen, die realistische Beschallung oder kreative Werkzeuge für Sounddesign erfordern.
- Der Eingangspegel kann angepasst werden.
- Eingänge können verzögert werden oder versuchen, spezifische Latenz (digitale Verarbeitung von Funkübertragung oder digitalen Effekten) zu berücksichtigen und zu kompensieren, um Verstärkung und akustischen Klang besser auszurichten.
- Minimale Latenz kann anstelle von Acoustic Precedence aktiviert werden. Dies versucht den Klang so schnell wie möglich durch das System zu bringen. Das System scannt die Sends dieses Eingangs zu den Ausgängen auf die niedrigste Verzögerung und zieht sie von allen Verzögerungen ab, wobei der zusätzliche Haas-Effekt umgangen wird.
- Die Position (Position und Offset) kann in Kartesischen, Zylindrischen oder Sphärischen Koordinaten unabhängig von der Bühnenform oder anderen Kanälen angegeben werden.
- Die Position kann auf die Bühnendimensionen in Kartesischen Koordinaten oder auf einen bestimmten Entfernungsbereich in Polarkoordinaten beschränkt werden.
- Flip nimmt die symmetrische Position für die gegebene Koordinate um den Ursprungspunkt.
- Der Joystick und der vertikale Schieberegler ermöglichen relative Positionssteuerung.
- Eingänge können einem Cluster zugewiesen werden, um sie für koordinierte Bewegungen zu gruppieren.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Inputs Basic Parameters
  - DE: Grundlegende Eingangsparameter
  - [ ] OK    Fix: 

## `help.inputHF`

- **`body`**
  - EN: When talking away, the timbre of a voice will sound less bright. Reproducing this was the initial goal here even though we usually want to have support for voices when not addressing the audience or in bi-frontal configurations. This can be put to good use for creative effects such as having a brighter reverb on a dampened direct sound.
The orientation of the input in azimuth and in pitch can be set as well as the angle where the high frequencies will not be filtered.
The HF Shelf will set the maximum attenuation in the back of the input. There is a smooth fade (like a cosine curve) from full brightness in front to damped at the rear.
  - DE: Wenn man sich abwendet, klingt das Timbre einer Stimme weniger brillant. Dies zu reproduzieren war das ursprüngliche Ziel, auch wenn wir normalerweise Unterstützung für Stimmen wünschen, die nicht zum Publikum sprechen oder in bi-frontalen Konfigurationen. Dies kann für kreative Effekte genutzt werden, wie eine brillantere Nachhall auf einem gedämpften Direktsignal.
Die Orientierung des Eingangs in Azimut und Neigung kann eingestellt werden sowie der Winkel, in dem die Hochfrequenzen nicht gefiltert werden.
Das HF Shelf bestimmt die maximale Dämpfung hinter dem Eingang. Es gibt einen sanften Übergang (wie eine Kosinuskurve) von voller Brillanz vorne zu gedämpft hinten.
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
  - EN: You can set for each output array a specific attenuation for the selected input.
You can mute each send to any output individually. There are macros to speed up the process.
  - DE: Sie können für jedes Ausgangs-Array eine spezifische Abschwächung für den ausgewählten Eingang festlegen.
Sie können jeden Send zu jedem Ausgang einzeln stummschalten. Makros stehen zur Verfügung, um den Prozess zu beschleunigen.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Array Attenuation and Output Mutes
  - DE: Array-Abschwächung und Ausgangs-Mutes
  - [ ] OK    Fix: 

## `help.lfo`

- **`body`**
  - EN: Input position can be automated. The LFO can control X, Y and Z position coordinates individually as well as the rotation of HF directivity (gyrophone).
Adjust the global period and phase for the LFO.
For X, Y and Z coordinates select a shape, amplitude, rate and phase as desired. A circle in the XY plane would have sine for shape for X and for Y and an offset of ±90° between the two. A square would be the same but with keystone shapes.
Input position can be moved while the LFO is running.
  - DE: Die Eingangsposition kann automatisiert werden. Der LFO kann X-, Y- und Z-Koordinaten einzeln steuern sowie die Rotation der HF-Direktionalität (Gyrophon).
Passen Sie Periode und Phase des LFO global an.
Für X, Y und Z wählen Sie Form, Amplitude, Rate und Phase. Ein Kreis in der XY-Ebene verwendet Sinusform für X und Y mit ±90° Versatz. Ein Quadrat wäre gleich aber mit Trapezformen.
Die Eingangsposition kann während des LFO-Betriebs verschoben werden.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Input LFO
  - DE: Eingangs-LFO
  - [ ] OK    Fix: 

## `help.liveSource`

- **`body`**
  - EN: A loud source on stage may not need much reinforcement through the nearby speakers. Imagine an opera singer near the edge of the stage. Normally the level distribution would make the level louder near the input position. But if it is already loud enough we should be able to not over-amplify it. This function manages this.
The radius and shape describe how to attenuate the level for a specific source for speakers within the radius of influence of this input. There are various shapes: a V shaped linear effect; a U shape for fast decrease within the radius; a tight V effect or a mix of the previous (sine).
The attenuation can be constant or level dependent like a local compression reacting to transients and to average RMS level.
  - DE: Eine laute Quelle auf der Bühne braucht möglicherweise keine Verstärkung durch die nahegelegenen Lautsprecher. Stellen Sie sich einen Opernsänger am Bühnenrand vor. Normalerweise würde die Pegelverteilung den Pegel in der Nähe der Eingangsposition erhöhen. Aber wenn es bereits laut genug ist, sollten wir nicht überverstärken. Diese Funktion verwaltet dies.
Radius und Form beschreiben, wie der Pegel für Lautsprecher innerhalb des Einflussradius dieser Quelle abgeschwächt wird. Es gibt verschiedene Formen: ein V-förmiger linearer Effekt; ein U für schnelle Abnahme; ein enges V oder eine Mischung (Sinus).
Die Dämpfung kann konstant oder pegelabhängig sein, wie eine lokale Kompression, die auf Transienten und den durchschnittlichen RMS-Pegel reagiert.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Live Source Tamer
  - DE: Live-Quellen-Dämpfer
  - [ ] OK    Fix: 

## `help.map`

- **`body`**
  - EN: - A left click on an input or a cluster will allow to move it by dragging it. A single finger touch will do the same.
- A left click with the shift key pressed will add or remove inputs to the selection. A double tap and drag will act the same way.
- A left click drag will draw a selection rectangle to select multiple inputs and clusters at the same time.
- A left double-click or tap will reset the position offset of the input.
- A long left click or press with no movement will switch to the input tab with the focus on the selected input on release.
- A left click away from any input will clear the selection.
- A right click and drag will pan the view of the map. A two finger drag with no selected input or cluster will do the same if your operating system supports multitouch.
- The mouse wheel will zoom in and out. A two finger pinch with no selected input or cluster will also zoom in and out.
- A middle click will reset the view to fit the stage on the map display. There is also a dedicated set of buttons to reset the view to fit all inputs and to fit the stage respectively.
- Selected inputs and clusters can also be moved with the arrow keys for X and Y and with the PageUp and PageDown keys for height. Hardware controllers can be used too.
- When an input is touched, a second finger nearby can rotate the input directivity and adjust the height by pinching if your operating system allows multitouch interaction.
- When a cluster is touched, a second finger nearby can rotate the cluster and scale it by pinching.
- Inputs, output arrays and the reverb nodes can be hidden on the map.
- Inputs can also be locked to prevent selecting and moving them on the map. They will still be moved by clusters, network commands, tracking and hardware controllers.
- All reverb nodes can be moved on the map if this is enabled on the reverb tab. Holding the Ctrl/Cmd key will move each pair of reverb nodes in symmetry.
- Inputs with offsets, LFO or with speed regulation will have a temporary position marker. But the point of interaction will remain the normal marker.
- The Live Source Tamer radius will be displayed around input when activated.
- There is a toggle to display the audio level for the inputs and outputs on the map tab, that's active when the audio processing is running.
  - DE: - Linksklick auf einen Eingang oder Cluster ermöglicht das Verschieben durch Ziehen.
- Linksklick mit Umschalttaste fügt Eingänge zur Auswahl hinzu/entfernt sie.
- Linksklick-Ziehen zeichnet ein Auswahlrechteck.
- Doppelklick setzt den Positionsversatz zurück.
- Langer Klick ohne Bewegung wechselt zum Eingangs-Tab.
- Klick außerhalb löscht die Auswahl.
- Rechtsklick-Ziehen verschiebt die Kartenansicht. Zwei-Finger-Ziehen ebenso.
- Mausrad zoomt. Zwei-Finger-Pinch ebenfalls.
- Mittelklick setzt die Ansicht zurück.
- Pfeiltasten verschieben X/Y, BildAuf/Ab die Höhe.
- Ein zweiter Finger kann die Direktionalität drehen und die Höhe anpassen.
- Bei Clustern kann ein zweiter Finger rotieren und skalieren.
- Eingänge, Ausgabe-Arrays und Hallknoten können ausgeblendet werden.
- Eingänge können gesperrt werden.
- Hallknoten können verschoben werden. Strg/Cmd bewegt Paare symmetrisch.
- Der Live-Source-Tamer-Radius wird angezeigt.
- Audiopegel können auf der Karte angezeigt werden.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Map
  - DE: Karte
  - [ ] OK    Fix: 

## `help.mcp`

- **`body`**
  - EN: The MCP server lets an AI assistant (Claude Desktop, Claude Code, ChatGPT with custom connectors) read and write the parameters of this WFS-DIY session over a local network connection.

What the AI can do:
• Read live state: channel counts, names, positions, attenuations, EQs, snapshots, clusters, the full parameter surface.
• Move sources, rename channels, set cluster assignments, adjust the array layout, place outputs and reverbs.
• Run guided workflows (system tuning walkthroughs, troubleshooting localization, snapshot management) via prepared prompt templates.

Operator controls on this row:
• AI: ON / OFF — master switch. When OFF every AI tool call is refused; when ON the AI works under the rules below.
• AI critical actions: blocked / ALLOWED — the destructive actions (deleting snapshots, resetting DSP, changing channel counts) are blocked by default. Click to allow them for 10 minutes; the red fill drains as the window expires, then they auto-block again.
• Open AI History — scrollable timeline of every recent AI change with per-row undo / redo and a step-by-step cursor.
• The MCP URL button copies the server URL to the clipboard for AI clients that take a URL directly.

Operator awareness:
• Every AI action is recorded with origin tags. The AI History window shows the full timeline; per-row × reverses an action with its dependents.
• If you manually adjust a parameter the AI just moved, the AI is notified and will not blindly retry. You always have the last word.
• The Cmd/Ctrl+Alt+Z and Cmd/Ctrl+Alt+Y shortcuts undo and redo the last AI change without affecting your manual edits (which use plain Ctrl+Z as usual).

To add this server to Claude Desktop:
  1. Open Settings → Developer → Edit Config.
  2. Paste the JSON snippet below into claude_desktop_config.json (merge into the existing mcpServers block if you already have one).
  3. Restart Claude Desktop. The server appears as 'wfs-diy' in the tools menu.

To add to Claude Code, run:
  claude mcp add wfs-diy <url> -t http

The URL changes if you switch network interface or if the server falls back to a different port. The URL button on this row always reflects the live URL.
  - DE: Der MCP-Server ermöglicht es einem KI-Assistenten (Claude Desktop, Claude Code, ChatGPT mit benutzerdefinierten Konnektoren), die Parameter dieser WFS-DIY-Sitzung über eine lokale Netzwerkverbindung zu lesen und zu schreiben.

Was die KI tun kann:
• Live-Status lesen: Kanalanzahl, Namen, Positionen, Dämpfungen, EQs, Snapshots, Cluster, die gesamte Parameteroberfläche.
• Quellen verschieben, Kanäle umbenennen, Cluster-Zuweisungen festlegen, Array-Layout anpassen, Ausgänge und Hall platzieren.
• Geführte Workflows ausführen (Systemabstimmung, Lokalisations-Fehlersuche, Snapshot-Verwaltung) über vorbereitete Prompt-Vorlagen.

Bedienelemente in dieser Zeile:
• KI: EIN / AUS — Hauptschalter. Im AUS-Zustand wird jeder KI-Tool-Aufruf abgelehnt; im EIN-Zustand arbeitet die KI nach den unten stehenden Regeln.
• Kritische KI-Aktionen: gesperrt / ERLAUBT — destruktive Aktionen (Snapshots löschen, DSP zurücksetzen, Kanalanzahl ändern) sind standardmässig gesperrt. Klicken, um sie 10 Minuten lang zu erlauben; die rote Füllung läuft mit dem Ablauf des Fensters ab und wird dann automatisch wieder gesperrt.
• KI-Verlauf öffnen — scrollbare Chronik aller jüngsten KI-Änderungen mit Rückgängig/Wiederherstellen pro Zeile.
• Die MCP-URL-Schaltfläche kopiert die Server-URL in die Zwischenablage für KI-Clients, die eine URL direkt akzeptieren.

Betreiber-Aufmerksamkeit:
• Jede KI-Aktion wird mit Herkunfts-Tags aufgezeichnet. Das KI-Verlaufsfenster zeigt die vollständige Chronik; das × pro Zeile macht eine Aktion mit ihren Abhängigkeiten rückgängig.
• Wenn Sie einen Parameter manuell anpassen, den die KI gerade verschoben hat, wird die KI benachrichtigt und versucht es nicht blindlings erneut. Sie haben immer das letzte Wort.
• Die Tastenkombinationen Cmd/Strg+Alt+Z und Cmd/Strg+Alt+Y machen die letzte KI-Änderung rückgängig bzw. stellen sie wieder her, ohne Ihre manuellen Änderungen zu beeinflussen (die wie üblich Strg+Z verwenden).

Um diesen Server zu Claude Desktop hinzuzufügen:
  1. Einstellungen → Entwickler → Konfiguration bearbeiten öffnen.
  2. Den JSON-Ausschnitt unten in claude_desktop_config.json einfügen (in den vorhandenen mcpServers-Block einfügen, falls bereits vorhanden).
  3. Claude Desktop neu starten. Der Server erscheint als 'wfs-diy' im Tools-Menü.

Für Claude Code ausführen:
  claude mcp add wfs-diy <url> -t http

Die URL ändert sich, wenn Sie die Netzwerkschnittstelle wechseln oder der Server auf einen anderen Port ausweicht. Die URL-Schaltfläche in dieser Zeile zeigt immer die aktuelle Live-URL.
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
  - DE: KI / MCP-Server
  - [ ] OK    Fix: 

## `help.network`

- **`body`**
  - EN: The system can communicate through several network protocols, (UDP or TCP) using OSC. OSC query can be enabled to allow the clients to discover the possible OSC paths and subscribe to some parameter updates.
The IP of the local machine corresponding to the selected network interface is shown. The incoming TCP and UDP ports as well as the OSC Query port.
There are a few specialised OSC clients such as:
- Remote for the Android application for multitouch operation and for remote control.
- QLab that can send data and that can also be programmed directly from the application.
- ADM-OSC for control from consoles and DAW (see specific help).
The data can be filtered to only allow the data from the recorded IP addresses or to allow any client sending on the correct ports.
There is a Log window to see what data comes in or out, filter by the type of protocol, client and so on.
There is also a locator function to find a lost remote Android tablet. It will flash and sound the alarm on the missing device.
  - DE: Das System kann über mehrere Netzwerkprotokolle (UDP oder TCP) mittels OSC kommunizieren. OSC Query kann aktiviert werden, damit Clients die möglichen OSC-Pfade entdecken und Parameteraktualisierungen abonnieren können.
Die IP der lokalen Maschine entsprechend der ausgewählten Netzwerkschnittstelle wird angezeigt. Die eingehenden TCP- und UDP-Ports sowie der OSC Query-Port.
Es gibt einige spezialisierte OSC-Clients wie:
- Remote für die Android-Anwendung für Multitouch-Bedienung und Fernsteuerung.
- QLab zum Senden von Daten und zur direkten Programmierung aus der Anwendung.
- ADM-OSC zur Steuerung von Konsolen und DAW (siehe spezifische Hilfe).
Die Daten können gefiltert werden. Ein Log-Fenster zeigt ein- und ausgehende Daten.
Es gibt auch eine Ortungsfunktion für verlorene Android-Tablets.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Network
  - DE: Netzwerk
  - [ ] OK    Fix: 

## `help.outputAdvanced`

- **`body`**
  - EN: There are a few parameters to help you adjust to the acoustic sound.
Most of these parameters are set for whole arrays unless the propagation mode is switched to off for this output in the array. Relative change can also be selected after a specific setting.
- Orientation and On/Off Angles define what inputs each speaker will amplify. By default the speakers are pointing to the audience, away from the stage. Inputs in the green sector will be amplified, but not the ones in front of the speaker, in the red sector. There is a fade between both sectors. For sub-bass speakers which usually come in limited numbers and locations, opening all the way to the maximum will allow you to have all inputs possibly picked up by the subwoofers.
- HF Damping simulates the loss of high frequency with distance. Speakers close to the listeners can have more than speakers away from the stage and the listeners.
- Distance Attenuation percentage allows you to define if more or less of the attenuation calculated from the distance and parameters of the input is applied. Again for Sub-bass in case you only have two and don't want to lose too much level or have hot-spots it may be wise to lower this setting to 50%.
- Minimal Latency toggles whether the output is scanned for the smallest calculated delay and also if the setting once engaged on an input will reduce the delay for that input through this speaker.
- Live Source Attenuation toggles the reduction of level of nearby input if they have this setting enabled. This may not be necessary for speakers away from the audience or for sub-bass.
- Floor Reflections toggles if the reflections are applied to the signal for this output such as sub-bass and flown arrays...
  - DE: Es gibt einige Parameter zur Anpassung an den akustischen Klang.
Die meisten dieser Parameter gelten für ganze Arrays, es sei denn, der Ausbreitungsmodus ist für diesen Ausgang im Array deaktiviert. Relative Änderung kann auch nach einer bestimmten Einstellung gewählt werden.
- Orientierung und On/Off-Winkel definieren, welche Eingänge jeder Lautsprecher verstärkt. Standardmäßig zeigen die Lautsprecher zum Publikum, weg von der Bühne. Eingänge im grünen Sektor werden verstärkt, nicht aber die vor dem Lautsprecher im roten Sektor. Es gibt einen Übergang zwischen beiden Sektoren. Für Subbass-Lautsprecher kann das vollständige Öffnen alle Eingänge einbeziehen.
- HF-Dämpfung simuliert den Hochfrequenzverlust mit der Entfernung.
- Der Prozentsatz der Entfernungsdämpfung definiert, wie viel der berechneten Dämpfung angewendet wird. Für Subbass kann es sinnvoll sein, auf 50% zu senken.
- Minimale Latenz aktiviert das Scannen nach der kleinsten berechneten Verzögerung.
- Live-Source-Attenuation aktiviert die Pegelreduzierung naher Eingänge.
- Bodenreflexionen aktiviert ob Reflexionen für diesen Ausgang angewendet werden, wie Subbass und geflogene Arrays...
  - [ ] OK    Fix: 

- **`title`**
  - EN: Advanced Parameters
  - DE: Erweiterte Parameter
  - [ ] OK    Fix: 

## `help.outputs`

- **`body`**
  - EN: The design of the WFS system has to do with the right choice of equipment and their position. Here is a guide to help you with the design and the tuning of your arrays.
An array is a line (straight or curved) of speakers. This is one of the most important concepts in WFS adapted to sound reinforcement and for creative sound design.
A rule of thumb would be that each listener should hear three speakers of an array to have enough psycho-acoustic cues to feel the direction each sound is coming from. There will be a sweet spot to find between the distance between the speakers and the listeners, their spacing and coverage angle. This is especially true for short range arrays, a.k.a front fills. Speakers with a 120° coverage angle can be spaced by the same distance between the array and the first row of listeners. The number of speakers also depends on the sound pressure level. When placed as a flown array, trapezoidal/asymmetric horns with wide coverage angle (120°) below axis and narrow coverage (60°) on axis will give good coverage for seat rows a bit too far from the ground array and will have enough throw to reach 20m or 30m away while avoiding the walls of the venue where they would create reflections that give away the position of the speakers to the ears of the listeners. Most of the time for larger venues coaxial (elliptical or conical horns) have not enough reach and require one or more delay lines. They are more suited for smaller venues with few rows of seats.
The positioning of the speakers in the system can be done through the 'Wizard of OutZ' and the editable presets it has.
  - DE: Das Design des WFS-Systems betrifft die richtige Auswahl der Ausrüstung und deren Positionierung. Hier ist ein Leitfaden für das Design und die Abstimmung Ihrer Arrays.
Ein Array ist eine Linie (gerade oder gekrümmt) von Lautsprechern. Dies ist eines der wichtigsten Konzepte in WFS, angepasst an Beschallung und kreatives Sounddesign.
Als Faustregel sollte jeder Zuhörer drei Lautsprecher eines Arrays hören, um genügend psychoakustische Hinweise für die Richtungswahrnehmung zu haben. Es gibt einen Sweet Spot zwischen dem Abstand der Lautsprecher zu den Zuhörern, ihrem Abstand zueinander und ihrem Abstrahlwinkel. Lautsprecher mit 120° Abstrahlwinkel können im gleichen Abstand wie zwischen Array und erster Reihe platziert werden. Die Anzahl hängt auch vom Schalldruckpegel ab. Als Hänge-Array bieten trapezförmige/asymmetrische Hörner mit breitem Abstrahlwinkel (120°) unter der Achse und schmalem (60°) in der Achse gute Abdeckung und Reichweite bis 20-30m, während Wandreflexionen vermieden werden. Koaxiale Lautsprecher haben meist nicht genug Reichweite für große Räume und benötigen Delay-Lines.
Die Positionierung kann über den 'Wizard of OutZ' und seine editierbaren Presets erfolgen.
  - [ ] OK    Fix: 

- **`title`**
  - EN: WFS Array Design
  - DE: WFS-Array-Design
  - [ ] OK    Fix: 

## `help.overview`

- **`body`**
  - EN: This WFS spatial sound processor aims to be a tool for natural sound reinforcement and also a creative tool opening new avenues for writing sound in space.
Some parameters are straightforward: place sound (Map, Tracking, Speed Limiting, Gradient Maps...), work its shape (Attenuation Profile) and its acoustic presence (Directivity, Floor Reflections), give it a one-off motion (AutomOtion) or a repetitive movement (L.F.O). In some cases amplification should be limited around loud sources on stage (Live Source Tamer). All of these functionalities can be stored and recalled internally or with the help of QLab. On the other side the system allows real-time interaction to trigger and move samples, move large clusters of inputs manually or thanks to easily recallable LFO presets.
  - DE: Dieser räumliche WFS-Klangprozessor soll ein Werkzeug für natürliche Beschallung und zugleich ein kreatives Werkzeug sein, das neue Wege eröffnet, Klang im Raum zu gestalten.
Einige Parameter sind unkompliziert: Klang platzieren (Karte, Tracking, Geschwindigkeitsbegrenzung, Gradientenkarten...), seine Form bearbeiten (Attenuation-Profil) und seine akustische Präsenz (Directivity, Bodenreflexionen), ihm eine einmalige Bewegung (AutomOtion) oder eine repetitive Bewegung (L.F.O) geben. In manchen Fällen sollte die Verstärkung um laute Quellen auf der Bühne begrenzt werden (Live Source Tamer). Alle diese Funktionen können intern oder mit Hilfe von QLab gespeichert und abgerufen werden. Darüber hinaus ermöglicht das System Echtzeit-Interaktion zum Auslösen und Bewegen von Samples, zum Bewegen großer Eingabe-Cluster manuell oder dank einfach abrufbarer LFO-Presets.
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
  - EN: Reverb helps blur the actual reflections of the speakers in the acoustic space. Without some reverb the listener might perceive the sound missing some depth because the reflections of the speakers in the acoustic of the room gives away their positions and all sounds might feel as coming from the plane of the speakers.
Place the reverb node according to the number of channels and the geometry of the stage and listening area, and the positions of speakers. If necessary the return position can be offset from the feed position.
Other parameters are very similar to Outputs' and Inputs'.
  - DE: Hall hilft, die Reflexionen der Lautsprecher zu verschleiern.
Platzieren Sie die Hall-Knoten entsprechend der Kanalanzahl und Bühnengeometrie.
Andere Parameter sind ähnlich wie bei Ausgängen und Eingängen.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb
  - DE: Hall
  - [ ] OK    Fix: 

## `help.reverbAlgo`

- **`body`**
  - EN: There are three built-in reverb types inside this spatial sound processor:
- SDN (Scattered Delay Network): The sound bounces between every reverb node which act as reflecting surfaces. This reverb algorithm favours an odd number of nodes that should not have too much symmetry. This helps reduce the artifacts of the reverb or any metallic sounding resonance.
- FDN (Feedback Delay Network): Each reverb node functions as a separate reverb processor with a classical reverb algorithm. Place nodes around the stage and eventually around the audience. Inputs are sent to each node in a similar fashion as with outputs. The return signal from the reverb is then distributed like other inputs to all outputs.
- IR (Impulse Response): This is a classical convolution reverb. You can load audio samples as impulse responses. Each reverb node can share the same IR or they can have different ones. Place nodes around the stage and eventually around the audience.
The node positions can be adjusted directly on the map. The Ctrl/Cmd key will move a reverb node pair in symmetry.
  - DE: Drei eingebaute Halltypen stehen in diesem räumlichen Klangprozessor zur Verfügung:
- SDN (Scattered Delay Network): Der Klang wird zwischen allen Hall-Knoten reflektiert, die als reflektierende Oberflächen wirken. Dieser Algorithmus bevorzugt eine ungerade Anzahl von Knoten ohne zu viel Symmetrie, um Artefakte oder metallische Resonanzen zu reduzieren.
- FDN (Feedback Delay Network): Jeder Hall-Knoten funktioniert als separater Hallprozessor mit einem klassischen Algorithmus. Platzieren Sie Knoten um die Bühne und eventuell um das Publikum.
- IR (Impulsantwort): Klassische Faltungshall. Sie können Audiosamples als Impulsantworten laden. Jeder Knoten kann dieselbe IR teilen oder verschiedene verwenden.
Die Knotenpositionen können direkt auf der Karte angepasst werden. Die Strg/Cmd-Taste bewegt ein Hall-Knotenpaar symmetrisch.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Algorithms
  - DE: Hallalgorithmen
  - [ ] OK    Fix: 

## `help.reverbFeed`

- **`body`**
  - EN: Pre-processing sending of the input channels to the reverb nodes.
- Orientation and On/Off Angles define what inputs each reverb node will receive. Inputs in the green sector will be picked-up, but not the ones in front in the red sector. There is a fade between both sectors.
- HF Damping simulates the loss of high frequency with distance.
- Distance Attenuation percentage allows you to define if more or less of the attenuation calculated from the distance and parameters of the input is applied.
- Minimal Latency toggles whether the output is scanned for the smallest calculated delay and also if the setting once engaged on an input will reduce the delay for that input through this reverb node.
- Live Source Attenuation toggles the reduction of level of nearby input if they have this setting enabled.
  - DE: Vorverarbeitung der Eingangskanalzuführung zu den Hall-Knoten.
- Orientierung und On/Off-Winkel bestimmen welche Eingänge empfangen werden.
- HF-Dämpfung simuliert Hochfrequenzverlust mit Distanz.
- Distanz-Dämpfung bestimmt die angewandte Abschwächung.
- Minimale Latenz schaltet die Verwendung des kleinsten Delays um.
- Live-Source-Dämpfung reduziert den Pegel nahegelegener Eingänge.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Feed
  - DE: Hall-Einspeisung
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
  - EN: Post-processing sending to the speakers.
- Distance Attenuation defines the level drop per meter to the speakers.
- Common Attenuation will look for the lowest attenuation and keep only a percentage of this level drop and apply this across all other outputs.
- Mutes and Mute Macros allow to prevent a reverb channel from feeding an output. For instance it may not be necessary to send reverb to sub-bass speakers.
  - DE: Nachbearbeitung an die Lautsprecher.
- Distanz-Dämpfung definiert den Pegelabfall pro Meter.
- Gemeinsame Dämpfung behält einen Prozentsatz der niedrigsten Dämpfung.
- Mutes verhindern, dass ein Hall-Kanal einen Ausgang speist.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Return
  - DE: Hall-Rückgabe
  - [ ] OK    Fix: 

## `help.sampler`

- **`body`**
  - EN: The sampler allows to trigger samples and interact with them in real time.
The sampler when enabled on a track will replace the live input at all times.
Several samplers can be assigned to different inputs and triggered individually.
To use the sampler:
- Select a Roli Lightpad or a pad on the connected Android Remote app.
- Add samples to the different tiles in the grid to the left. Adjust their relative starting position and their level and eventually their in and out points. Several samples can be selected using the shift key while clicking.
- Create sets of samples: selected samples will be added to new sets. Samples can be added or removed after the creation of a set by holding Ctrl/Cmd while clicking on the tiles. Each set can be renamed. Each set can either have a fixed sequence or a random order (round robin, each sample is played once before a new random order is drawn). Each set has an attenuation setting. Each set has a base position applied to the input when selecting the set. It can be moved on the map or using external control. The sample position offset is added to the set position each time a sample is triggered.
- Press a Roli Lightpad or a pad on the Android app to trigger a sample. The pressure applied to the pad can be mapped to any of the following controls: level, height and high frequency filtering. The sensitivity can be adjusted for each. The movement of the finger on the pad will cause the sound to move. This acts by measuring the deflection from the initial contact point like a joystick. This can be disabled. All sets have their respective settings for the interaction.
Releasing the pad will stop the triggered sample.
Sampler settings are stored in the input files.
For convenience sample tiles and sets can be copied, exported and imported.
  - DE: Der Sampler ermöglicht das Auslösen von Samples und die Echtzeit-Interaktion mit ihnen.
Wenn der Sampler auf einer Spur aktiviert ist, ersetzt er den Live-Eingang dauerhaft.
Mehrere Sampler können verschiedenen Eingängen zugewiesen und einzeln ausgelöst werden.
Verwendung des Samplers:
- Wählen Sie ein Roli Lightpad oder ein Pad in der verbundenen Android Remote App.
- Fügen Sie Samples zu den verschiedenen Kacheln im Raster hinzu. Passen Sie die relative Startposition, den Pegel und die Ein-/Ausstiegspunkte an. Mehrere Samples können mit Umschalttaste und Klick ausgewählt werden.
- Erstellen Sie Sample-Sets: Ausgewählte Samples werden neuen Sets hinzugefügt. Samples können nach der Erstellung eines Sets mit Strg/Cmd und Klick hinzugefügt oder entfernt werden. Jedes Set kann umbenannt werden und eine feste Sequenz oder zufällige Reihenfolge haben. Jedes Set hat eine Dämpfungseinstellung und eine Basisposition.
- Drücken Sie ein Lightpad oder Pad, um ein Sample auszulösen. Der Druck kann auf Pegel, Höhe und Hochfrequenzfilterung gemappt werden. Die Fingerbewegung bewegt den Klang wie ein Joystick.
Loslassen stoppt das ausgelöste Sample.
Sampler-Einstellungen werden in den Eingangsdateien gespeichert.
Kacheln und Sets können kopiert, exportiert und importiert werden.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Sampler
  - DE: Sampler
  - [ ] OK    Fix: 

## `help.sessionData`

- **`body`**
  - EN: When starting a session select the working folder where the system will place the files and eventual audio files. For new projects create a new folder. When reloading a previous session navigate to the corresponding folder.
Each section has a separate xml file (System configuration, Network, Outputs, Reverbs, Inputs) and backups. Additionally convolution reverb impulse responses and audio samples will be stored in sub-directories.
Each section can be stored and recalled individually or as a whole.
Each section can also export and import files from other projects.
  - DE: Beim Start einer Sitzung wählen Sie den Arbeitsordner, in dem das System die Dateien und eventuellen Audiodateien ablegen wird. Für neue Projekte erstellen Sie einen neuen Ordner. Beim Laden einer vorherigen Sitzung navigieren Sie zum entsprechenden Ordner.
Jeder Abschnitt hat eine separate XML-Datei (Systemkonfiguration, Netzwerk, Ausgänge, Halleffekte, Eingänge) und Backups. Faltungshall-Impulsantworten und Audiosamples werden in Unterverzeichnissen gespeichert.
Jeder Abschnitt kann einzeln oder als Ganzes gespeichert und abgerufen werden.
Jeder Abschnitt kann auch Dateien aus anderen Projekten exportieren und importieren.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Session Data
  - DE: Sitzungsdaten
  - [ ] OK    Fix: 

## `help.snapshotScope`

- **`body`**
  - EN: Snapshots store input parameters, but can have a scope to be recalled during a performance. They can have between all parameters for all inputs and only one parameter for a single channel. They can be updated and renamed for convenience.
The Scope tells the system what data to store or recall. It's the opposite of 'safe' parameters.
There are several ways to do this in this application:
- Record only the needed data in local files. The scope filter is applied when storing the data. A recall cue can be created in QLab to trigger the reading of the local file.
- Record all data and a filter in local files. The scope filter is applied when recalling the data. This allows to eventually recall all data not taking into account the scope filter. This may come in handy when a complete configuration should be recalled during rehearsal for example. A recall cue can be created in QLab to trigger the reading of the local file.
- Record all data in scope in QLab cues. This should not be used to recall all parameters for large configurations since QLab may stall when recalling so much data.
The scope can show and automatically pre-select the parameters that have been manually changed (local UI, hardware controllers, remote Android application). Changed parameters are marked with a yellow mark.
  - DE: Snapshots speichern Eingangsparameter, können aber einen Bereich haben, der während einer Aufführung abgerufen wird.
Der Bereich teilt dem System mit, welche Daten gespeichert oder abgerufen werden sollen.
Mehrere Methoden sind verfügbar:
- Nur benötigte Daten in lokalen Dateien speichern. Der Filter wird beim Speichern angewandt.
- Alle Daten und einen Filter in lokalen Dateien speichern. Der Filter wird beim Abrufen angewandt.
- Alle Daten im Bereich in QLab-Cues speichern. Nicht für große Konfigurationen empfohlen.
Der Bereich kann manuell geänderte Parameter anzeigen und automatisch vorauswählen. Geänderte Parameter sind gelb markiert.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Input Snapshots and Scope
  - DE: Eingangs-Snapshots und Bereich
  - [ ] OK    Fix: 

## `help.tracking`

- **`body`**
  - EN: Tracking allows to follow the position in 2D or 3D of actors and musicians. There are several tracking solutions based on UWB (UltraWide Band) tags, 3D camera, computer vision systems and Infra-Red LED's followed by IR sensitive cameras.
This application allows you to receive tracking data from several protocols: OSC, MQTT, PosiStageNet/PSN, RTTrP.
You can select which protocol you're using and enter its settings. You may also adjust the mapping (offset, scaling and orientation).
Each input has a toggle to enable tracking, an ID to select which tracking marker it should follow and a smoothing algorithm to reduce jittery movements.
  - DE: Tracking ermöglicht die Verfolgung der 2D- oder 3D-Position von Schauspielern und Musikern. Es gibt verschiedene Tracking-Lösungen basierend auf UWB-Tags, 3D-Kameras, Computer-Vision-Systemen und Infrarot-LEDs mit IR-empfindlichen Kameras.
Diese Anwendung kann Tracking-Daten über mehrere Protokolle empfangen: OSC, MQTT, PosiStageNet/PSN, RTTrP.
Sie können das verwendete Protokoll auswählen und die Einstellungen eingeben. Das Mapping (Versatz, Skalierung und Orientierung) kann ebenfalls angepasst werden.
Jeder Eingang hat einen Schalter zur Aktivierung des Trackings, eine ID zur Auswahl des Markers und einen Glättungsalgorithmus.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Tracking
  - DE: Tracking
  - [ ] OK    Fix: 

## `help.tuning`

- **`body`**
  - EN: System tuning for WFS is different from standard PA tuning. It can proceed as follows:
- Start with the flown array muted. Set the desired levels for the near-field speakers when listening to them in the first row. Adjust the high frequency shelf attenuation so the near-field speakers are not too bright and draw too much attention.
- Mute the near-field array and unmute the flown array, find a suitable level towards the back of the house.
- Unmute both arrays, adjust the delay of the flown array to bring the sound down to the correct height in the lower rows. Adjust the levels, HF shelf/distance ratio and vertical and horizontal parallax for each array to have consistent level wherever your inputs are on stage. This is especially important if your inputs are actors, singers or acoustic music instruments on stage. You may also test adding some Haas effect to delay the whole system if you feel the WFS is too early or adding comb filtering with the acoustic sound.
You may follow a different workflow for tuning or go for different cues.
  - DE: Die Systemabstimmung für WFS unterscheidet sich von der Standard-PA-Abstimmung. Sie kann wie folgt vorgehen:
- Beginnen Sie mit stummgeschaltetem Hänge-Array. Stellen Sie die gewünschten Pegel für die Nahfeld-Lautsprecher ein, wenn Sie sie in der ersten Reihe hören. Passen Sie die HF-Shelf-Dämpfung an, damit die Nahfeld-Lautsprecher nicht zu brilliant sind.
- Schalten Sie das Nahfeld-Array stumm und das Hänge-Array ein, finden Sie einen geeigneten Pegel zum Ende des Saals hin.
- Aktivieren Sie beide Arrays, passen Sie die Verzögerung des Hänge-Arrays an. Passen Sie Pegel, HF-Shelf/Entfernungsverhältnis und vertikale und horizontale Parallaxe für jedes Array an, um einen konsistenten Pegel zu erreichen, wo auch immer Ihre Eingänge auf der Bühne sind.
Sie können einen anderen Workflow für die Abstimmung verfolgen oder für verschiedene Situationen unterschiedliche Einstellungen wählen.
  - [ ] OK    Fix: 

- **`title`**
  - EN: System Tuning
  - DE: Systemabstimmung
  - [ ] OK    Fix: 

## `inputs`

- **`arrayPrefix`**
  - EN: Array
  - DE: Array
  - [ ] OK    Fix: 

## `inputs.buttons`

- **`deleteSnapshot`**
  - EN: Delete Snapshot
  - DE: Snapshot löschen
  - [ ] OK    Fix: 

- **`editScope`**
  - EN: Edit Scope
  - DE: Umfang bearbeiten
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - DE: Exportieren
  - [ ] OK    Fix: 

- **`hiddenOnMap`**
  - EN: Input Hidden on Map
  - DE: Eingang auf Karte ausgeblendet
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - DE: Importieren
  - [ ] OK    Fix: 

- **`lockOnMap`**
  - EN: Lock on Map
  - DE: Auf Karte sperren
  - [ ] OK    Fix: 

- **`pauseAll`**
  - EN: Pause All
  - DE: Alle pausieren
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Backup
  - DE: Backup laden
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Input Config
  - DE: Eingangskonf. neu laden
  - [ ] OK    Fix: 

- **`reloadSnapshot`**
  - EN: Reload Snapshot
  - DE: Snapshot laden
  - [ ] OK    Fix: 

- **`reloadWithoutScope`**
  - EN: Reload Without Scope
  - DE: Ohne Filter laden
  - [ ] OK    Fix: 

- **`resumeAll`**
  - EN: Resume All
  - DE: Alle fortsetzen
  - [ ] OK    Fix: 

- **`samplerOff`**
  - EN: Sampler: OFF
  - DE: Sampler: AUS
  - [ ] OK    Fix: 

- **`samplerOn`**
  - EN: Sampler: ON
  - DE: Sampler: EIN
  - [ ] OK    Fix: 

- **`setAllInputs`**
  - EN: Set all Inputs...
  - DE: Alle Eingänge setzen...
  - [ ] OK    Fix: 

- **`solo`**
  - EN: Solo
  - DE: Solo
  - [ ] OK    Fix: 

- **`soloModeMulti`**
  - EN: Multi
  - DE: Multi
  - [ ] OK    Fix: 

- **`soloModeSingle`**
  - EN: Single
  - DE: Single
  - [ ] OK    Fix: 

- **`stopAll`**
  - EN: Stop All
  - DE: Alle stoppen
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Input Config
  - DE: Eingangskonf. speichern
  - [ ] OK    Fix: 

- **`storeSnapshot`**
  - EN: Store Snapshot
  - DE: Snapshot speichern
  - [ ] OK    Fix: 

- **`updateSnapshot`**
  - EN: Update Snapshot
  - DE: Snapshot aktualisieren
  - [ ] OK    Fix: 

- **`visibleOnMap`**
  - EN: Input Visible on Map
  - DE: Eingang auf Karte sichtbar
  - [ ] OK    Fix: 

## `inputs.clusters`

- **`clusterPrefix`**
  - EN: Cluster
  - DE: Cluster
  - [ ] OK    Fix: 

- **`single`**
  - EN: Single
  - DE: Einzeln
  - [ ] OK    Fix: 

## `inputs.coordinates`

- **`xyz`**
  - EN: XYZ
  - DE: XYZ
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
  - EN: Input {current} has tracking enabled, but Input {existing} in Cluster {cluster} is already tracked.

Only one tracked input per cluster is allowed.
  - DE: Eingang {current} hat Tracking aktiviert, aber Eingang {existing} in Cluster {cluster} wird bereits getrackt.

Nur ein getrackter Eingang pro Cluster ist erlaubt.
  - [ ] OK    Fix: 

- **`trackingConflictContinue`**
  - EN: Continue (disable tracking)
  - DE: Fortfahren (Tracking deaktivieren)
  - [ ] OK    Fix: 

- **`trackingConflictSwitch`**
  - EN: Input {existing} in Cluster {cluster} already has tracking enabled.

Only one tracked input per cluster is allowed.

Do you want to disable tracking on Input {existing} and enable it on Input {to}?
  - DE: Eingang {existing} in Cluster {cluster} hat bereits Tracking aktiviert.

Nur ein getrackter Eingang pro Cluster ist erlaubt.

Möchten Sie das Tracking auf Eingang {existing} deaktivieren und auf Eingang {to} aktivieren?
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

- **`buttons.copyLayer`**
  - EN: Copy Layer
  - DE: Ebene kopieren
  - [ ] OK    Fix: 

- **`buttons.copyShape`**
  - EN: Copy Shape
  - DE: Form kopieren
  - [ ] OK    Fix: 

- **`buttons.delete`**
  - EN: Delete
  - DE: Löschen
  - [ ] OK    Fix: 

- **`buttons.enable`**
  - EN: On
  - DE: An
  - [ ] OK    Fix: 

- **`buttons.lock`**
  - EN: Lock
  - DE: Sperren
  - [ ] OK    Fix: 

- **`buttons.pasteLayer`**
  - EN: Paste Layer
  - DE: Ebene einfügen
  - [ ] OK    Fix: 

- **`buttons.pasteShape`**
  - EN: Paste Shape
  - DE: Form einfügen
  - [ ] OK    Fix: 

- **`header.attenuationLayer`**
  - EN: Attenuation Layer
  - DE: Dämpfungs-Ebene
  - [ ] OK    Fix: 

- **`header.heightLayer`**
  - EN: Height Layer
  - DE: Höhen-Ebene
  - [ ] OK    Fix: 

- **`header.hfShelfLayer`**
  - EN: HF Shelf Layer
  - DE: HF-Shelf-Ebene
  - [ ] OK    Fix: 

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

- **`hints.darkMaxAtten`**
  - EN: Dark = max attenuation | Light = none
  - DE: Dunkel = max. Dämpfung | Hell = keine
  - [ ] OK    Fix: 

- **`hints.darkMaxHeight`**
  - EN: Dark = max height | Light = ground
  - DE: Dunkel = max. Höhe | Hell = Boden
  - [ ] OK    Fix: 

- **`hints.darkMaxHF`**
  - EN: Dark = max HF shelf | Light = none
  - DE: Dunkel = max. HF-Shelf | Hell = keins
  - [ ] OK    Fix: 

- **`hints.polygonClose`**
  - EN: Double-click to close polygon
  - DE: Doppelklick zum Schließen des Polygons
  - [ ] OK    Fix: 

- **`hints.whiteMaxAtten`**
  - EN: White = max attenuation | Black = none
  - DE: Weiß = max. Dämpfung | Schwarz = keine
  - [ ] OK    Fix: 

- **`hints.whiteMaxHeight`**
  - EN: White = max height | Black = ground
  - DE: Weiß = max. Höhe | Schwarz = Boden
  - [ ] OK    Fix: 

- **`hints.whiteMaxHF`**
  - EN: White = max HF shelf | Black = none
  - DE: Weiß = max. HF-Shelf | Schwarz = keins
  - [ ] OK    Fix: 

- **`labels.black`**
  - EN: Black:
  - DE: Schwarz:
  - [ ] OK    Fix: 

- **`labels.blur`**
  - EN: Blur:
  - DE: Unschärfe:
  - [ ] OK    Fix: 

- **`labels.center`**
  - EN: Center:
  - DE: Mitte:
  - [ ] OK    Fix: 

- **`labels.curve`**
  - EN: Curve:
  - DE: Kurve:
  - [ ] OK    Fix: 

- **`labels.edge`**
  - EN: Edge:
  - DE: Rand:
  - [ ] OK    Fix: 

- **`labels.end`**
  - EN: End:
  - DE: Ende:
  - [ ] OK    Fix: 

- **`labels.fill`**
  - EN: Fill:
  - DE: Füllung:
  - [ ] OK    Fix: 

- **`labels.name`**
  - EN: Name:
  - DE: Name:
  - [ ] OK    Fix: 

- **`labels.start`**
  - EN: Start:
  - DE: Start:
  - [ ] OK    Fix: 

- **`labels.white`**
  - EN: White:
  - DE: Weiß:
  - [ ] OK    Fix: 

- **`layers.attenuation`**
  - EN: Attenuation
  - DE: Dämpfung
  - [ ] OK    Fix: 

- **`layers.height`**
  - EN: Height
  - DE: Höhe
  - [ ] OK    Fix: 

- **`layers.hfShelf`**
  - EN: HF Shelf
  - DE: HF-Shelf
  - [ ] OK    Fix: 

- **`tools.editPoints`**
  - EN: Edit Points
  - DE: Punkte bearbeiten
  - [ ] OK    Fix: 

- **`tools.ellipse`**
  - EN: Ellipse
  - DE: Ellipse
  - [ ] OK    Fix: 

- **`tools.fill`**
  - EN: Fill
  - DE: Füllen
  - [ ] OK    Fix: 

- **`tools.linGrad`**
  - EN: Linear Grad.
  - DE: Lin. Verlauf
  - [ ] OK    Fix: 

- **`tools.polygon`**
  - EN: Polygon
  - DE: Polygon
  - [ ] OK    Fix: 

- **`tools.radGrad`**
  - EN: Radial Grad.
  - DE: Rad. Verlauf
  - [ ] OK    Fix: 

- **`tools.rect`**
  - EN: Rectangle
  - DE: Rechteck
  - [ ] OK    Fix: 

- **`tools.select`**
  - EN: Select
  - DE: Auswahl
  - [ ] OK    Fix: 

- **`warnings.heightRatioZero`**
  - EN: Height Ratio is 0% — increase it for height to take effect
  - DE: Höhenverhältnis ist 0% — erhöhen Sie es, damit die Höhe wirksam wird
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
  - DE: Dämpfungsgesetz-Modell (lineare Lautstärkeabnahme mit dem Abstand zwischen Objekt und Lautsprecher oder quadratisch).
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
  - DE: Simulierte Bodenreflexionen für das Objekt aktivieren.
  - [ ] OK    Fix: 

- **`frAttenuationSlider`**
  - EN: Attenuation of the Simulated Floor Reflections for the Object.
  - DE: Dämpfung der simulierten Bodenreflexionen für das Objekt.
  - [ ] OK    Fix: 

- **`frDiffusionDial`**
  - EN: Diffusion Effect of the Simulated Floor Reflections for the Object.
  - DE: Diffusionseffekt der simulierten Bodenreflexionen für das Objekt.
  - [ ] OK    Fix: 

- **`frHighShelfActiveButton`**
  - EN: Enable High Shelf Filter for Floor Reflections.
  - DE: High-Shelf-Filter für Bodenreflexionen aktivieren.
  - [ ] OK    Fix: 

- **`frHighShelfFreqSlider`**
  - EN: High Shelf Frequency for Floor Reflections.
  - DE: High-Shelf-Frequenz für Bodenreflexionen.
  - [ ] OK    Fix: 

- **`frHighShelfGainSlider`**
  - EN: High Shelf Gain for Floor Reflections.
  - DE: High-Shelf-Gain für Bodenreflexionen.
  - [ ] OK    Fix: 

- **`frHighShelfSlopeSlider`**
  - EN: High Shelf Slope for Floor Reflections.
  - DE: High-Shelf-Steilheit für Bodenreflexionen.
  - [ ] OK    Fix: 

- **`frLowCutActiveButton`**
  - EN: Enable Low Cut Filter for Floor Reflections.
  - DE: Low-Cut-Filter für Bodenreflexionen aktivieren.
  - [ ] OK    Fix: 

- **`frLowCutFreqSlider`**
  - EN: Low Cut Frequency for Floor Reflections.
  - DE: Low-Cut-Frequenz für Bodenreflexionen.
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
  - DE: Schnellen (Peak-) Kompressor des Live-Source-Zähmers aktivieren oder deaktivieren.
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
  - DE: Langsamen Kompressor des Live-Source-Zähmers aktivieren oder deaktivieren.
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
  - DE: Wahl zwischen akustischer Präzedenz und minimaler Latenz für die Verstärkungspräzedenz.
  - [ ] OK    Fix: 

- **`muteButton`**
  - EN: Mute Output {num} for this Object.
  - DE: Ausgang {num} für dieses Objekt stummschalten.
  - [ ] OK    Fix: 

- **`muteMacrosSelector`**
  - EN: Mute Macros for Fast Muting and Unmuting of Arrays.
  - DE: Mute-Makros zum schnellen Stummschalten und Aufheben für Arrays.
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

## `inputs.labels`

- **`admMapping`**
  - EN: ADM:
  - DE: ADM:
  - [ ] OK    Fix: 

- **`amplitudeX`**
  - EN: Amplitude X:
  - DE: Amplitude X:
  - [ ] OK    Fix: 

- **`amplitudeY`**
  - EN: Amplitude Y:
  - DE: Amplitude Y:
  - [ ] OK    Fix: 

- **`amplitudeZ`**
  - EN: Amplitude Z:
  - DE: Amplitude Z:
  - [ ] OK    Fix: 

- **`arrayAttenuation`**
  - EN: Array Attenuation:
  - DE: Array-Dämpfung:
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Attenuation:
  - DE: Dämpfung:
  - [ ] OK    Fix: 

- **`attenuationLaw`**
  - EN: Attenuation Law:
  - DE: Dämpfungskurve:
  - [ ] OK    Fix: 

- **`cluster`**
  - EN: Cluster:
  - DE: Cluster:
  - [ ] OK    Fix: 

- **`commonAtten`**
  - EN: Common Atten:
  - DE: Gem. Dämpf.:
  - [ ] OK    Fix: 

- **`coord`**
  - EN: Coord:
  - DE: Koord.:
  - [ ] OK    Fix: 

- **`curve`**
  - EN: Curve:
  - DE: Kurve:
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Delay/Latency:
  - DE: Verzögerung/Latenz:
  - [ ] OK    Fix: 

- **`destX`**
  - EN: Dest. X:
  - DE: Ziel X:
  - [ ] OK    Fix: 

- **`destY`**
  - EN: Dest. Y:
  - DE: Ziel Y:
  - [ ] OK    Fix: 

- **`destZ`**
  - EN: Dest. Z:
  - DE: Ziel Z:
  - [ ] OK    Fix: 

- **`diffusion`**
  - EN: Diffusion:
  - DE: Diffusion:
  - [ ] OK    Fix: 

- **`directivity`**
  - EN: Directivity:
  - DE: Richtcharakteristik:
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Distance Atten:
  - DE: Entf.-Dämpf.:
  - [ ] OK    Fix: 

- **`distanceRatio`**
  - EN: Distance Ratio:
  - DE: Entf.-Verhältnis:
  - [ ] OK    Fix: 

- **`duration`**
  - EN: Duration:
  - DE: Dauer:
  - [ ] OK    Fix: 

- **`frequency`**
  - EN: Frequency:
  - DE: Frequenz:
  - [ ] OK    Fix: 

- **`fringe`**
  - EN: Fringe:
  - DE: Randbereich:
  - [ ] OK    Fix: 

- **`gain`**
  - EN: Gain:
  - DE: Gain:
  - [ ] OK    Fix: 

- **`gyrophone`**
  - EN: Gyrophone:
  - DE: Gyrophone:
  - [ ] OK    Fix: 

- **`heightFactor`**
  - EN: Height Factor:
  - DE: Höhenfaktor:
  - [ ] OK    Fix: 

- **`hfShelf`**
  - EN: HF Shelf:
  - DE: HF-Regler:
  - [ ] OK    Fix: 

- **`jitter`**
  - EN: Jitter:
  - DE: Jitter:
  - [ ] OK    Fix: 

- **`max`**
  - EN: Max:
  - DE: Max:
  - [ ] OK    Fix: 

- **`maxSpeed`**
  - EN: Max Speed:
  - DE: Max. Geschw.:
  - [ ] OK    Fix: 

- **`min`**
  - EN: Min:
  - DE: Min:
  - [ ] OK    Fix: 

- **`muteMacros`**
  - EN: Mute Macros:
  - DE: Stummsch.-Makros:
  - [ ] OK    Fix: 

- **`name`**
  - EN: Name:
  - DE: Name:
  - [ ] OK    Fix: 

- **`offsetX`**
  - EN: Offset X:
  - DE: Offset X:
  - [ ] OK    Fix: 

- **`offsetY`**
  - EN: Offset Y:
  - DE: Offset Y:
  - [ ] OK    Fix: 

- **`offsetZ`**
  - EN: Offset Z:
  - DE: Offset Z:
  - [ ] OK    Fix: 

- **`outX`**
  - EN: Out X:
  - DE: Aus X:
  - [ ] OK    Fix: 

- **`outY`**
  - EN: Out Y:
  - DE: Aus Y:
  - [ ] OK    Fix: 

- **`outZ`**
  - EN: Out Z:
  - DE: Aus Z:
  - [ ] OK    Fix: 

- **`peakRatio`**
  - EN: Peak Ratio:
  - DE: Spitzenverhältnis:
  - [ ] OK    Fix: 

- **`peakThreshold`**
  - EN: Peak Threshold:
  - DE: Spitzenschwelle:
  - [ ] OK    Fix: 

- **`period`**
  - EN: Period:
  - DE: Periode:
  - [ ] OK    Fix: 

- **`phase`**
  - EN: Phase:
  - DE: Phase:
  - [ ] OK    Fix: 

- **`phaseX`**
  - EN: Phase X:
  - DE: Phase X:
  - [ ] OK    Fix: 

- **`phaseY`**
  - EN: Phase Y:
  - DE: Phase Y:
  - [ ] OK    Fix: 

- **`phaseZ`**
  - EN: Phase Z:
  - DE: Phase Z:
  - [ ] OK    Fix: 

- **`positionX`**
  - EN: Position X:
  - DE: Position X:
  - [ ] OK    Fix: 

- **`positionY`**
  - EN: Position Y:
  - DE: Position Y:
  - [ ] OK    Fix: 

- **`positionZ`**
  - EN: Position Z:
  - DE: Position Z:
  - [ ] OK    Fix: 

- **`radius`**
  - EN: Radius:
  - DE: Radius:
  - [ ] OK    Fix: 

- **`rateX`**
  - EN: Rate X:
  - DE: Rate X:
  - [ ] OK    Fix: 

- **`rateY`**
  - EN: Rate Y:
  - DE: Rate Y:
  - [ ] OK    Fix: 

- **`rateZ`**
  - EN: Rate Z:
  - DE: Rate Z:
  - [ ] OK    Fix: 

- **`reset`**
  - EN: Reset:
  - DE: Zurücksetzen:
  - [ ] OK    Fix: 

- **`rotation`**
  - EN: Rotation:
  - DE: Drehung:
  - [ ] OK    Fix: 

- **`shape`**
  - EN: Shape:
  - DE: Form:
  - [ ] OK    Fix: 

- **`shapeX`**
  - EN: Shape X:
  - DE: Form X:
  - [ ] OK    Fix: 

- **`shapeY`**
  - EN: Shape Y:
  - DE: Form Y:
  - [ ] OK    Fix: 

- **`shapeZ`**
  - EN: Shape Z:
  - DE: Form Z:
  - [ ] OK    Fix: 

- **`slope`**
  - EN: Slope:
  - DE: Steigung:
  - [ ] OK    Fix: 

- **`slowRatio`**
  - EN: Slow Ratio:
  - DE: Langsamverhältnis:
  - [ ] OK    Fix: 

- **`slowThreshold`**
  - EN: Slow Threshold:
  - DE: Langsamschwelle:
  - [ ] OK    Fix: 

- **`speedProfile`**
  - EN: Speed Profile:
  - DE: Geschw.-Profil:
  - [ ] OK    Fix: 

- **`threshold`**
  - EN: Threshold:
  - DE: Schwelle:
  - [ ] OK    Fix: 

- **`tilt`**
  - EN: Tilt:
  - DE: Neigung:
  - [ ] OK    Fix: 

- **`trackingId`**
  - EN: Tracking ID:
  - DE: Tracking-ID:
  - [ ] OK    Fix: 

- **`trackingSmooth`**
  - EN: Tracking Smooth:
  - DE: Tracking-Glättung:
  - [ ] OK    Fix: 

- **`xyJoystick`**
  - EN: X/Y
  - DE: X/Y
  - [ ] OK    Fix: 

- **`zSlider`**
  - EN: Z
  - DE: Z
  - [ ] OK    Fix: 

## `inputs.lfo`

- **`gyrophone.antiClockwise`**
  - EN: Anti-Clockwise
  - DE: Gegen Uhrzeigersinn
  - [ ] OK    Fix: 

- **`gyrophone.clockwise`**
  - EN: Clockwise
  - DE: Im Uhrzeigersinn
  - [ ] OK    Fix: 

- **`gyrophone.off`**
  - EN: OFF
  - DE: AUS
  - [ ] OK    Fix: 

- **`shapes.exp`**
  - EN: exp
  - DE: Exp
  - [ ] OK    Fix: 

- **`shapes.keystone`**
  - EN: keystone
  - DE: Trapez
  - [ ] OK    Fix: 

- **`shapes.log`**
  - EN: log
  - DE: Log
  - [ ] OK    Fix: 

- **`shapes.off`**
  - EN: OFF
  - DE: AUS
  - [ ] OK    Fix: 

- **`shapes.random`**
  - EN: random
  - DE: Zufall
  - [ ] OK    Fix: 

- **`shapes.sawtooth`**
  - EN: sawtooth
  - DE: Sägezahn
  - [ ] OK    Fix: 

- **`shapes.sine`**
  - EN: sine
  - DE: Sinus
  - [ ] OK    Fix: 

- **`shapes.square`**
  - EN: square
  - DE: Rechteck
  - [ ] OK    Fix: 

- **`shapes.triangle`**
  - EN: triangle
  - DE: Dreieck
  - [ ] OK    Fix: 

## `inputs.liveSource`

- **`linear`**
  - EN: linear
  - DE: linear
  - [ ] OK    Fix: 

- **`log`**
  - EN: log
  - DE: log
  - [ ] OK    Fix: 

- **`sine`**
  - EN: sine
  - DE: sinus
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
  - DE: Bitte wählen Sie zuerst einen Projektordner in der Systemkonfiguration.
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

## `inputs.muteMacros`

- **`invertMutes`**
  - EN: INVERT MUTES
  - DE: STUMMSCH. INVERTIEREN
  - [ ] OK    Fix: 

- **`muteAll`**
  - EN: MUTE ALL
  - DE: ALLE STUMM
  - [ ] OK    Fix: 

- **`muteArrayPrefix`**
  - EN: MUTE ARRAY
  - DE: ARRAY STUMM
  - [ ] OK    Fix: 

- **`muteEven`**
  - EN: MUTE EVEN
  - DE: GERADE STUMM
  - [ ] OK    Fix: 

- **`muteOdd`**
  - EN: MUTE ODD
  - DE: UNGERADE STUMM
  - [ ] OK    Fix: 

- **`selectMacro`**
  - EN: Select Macro...
  - DE: Makro auswählen...
  - [ ] OK    Fix: 

- **`unmuteAll`**
  - EN: UNMUTE ALL
  - DE: ALLE LAUT
  - [ ] OK    Fix: 

- **`unmuteArrayPrefix`**
  - EN: UNMUTE ARRAY
  - DE: ARRAY LAUT
  - [ ] OK    Fix: 

## `inputs.prefixes`

- **`delay`**
  - EN: Delay:
  - DE: Verzögerung:
  - [ ] OK    Fix: 

## `inputs.sections`

- **`automotion`**
  - EN: AutomOtion
  - DE: AutomOtion
  - [ ] OK    Fix: 

## `inputs.snapshots`

- **`selectSnapshot`**
  - EN: Select Snapshot...
  - DE: Snapshot auswählen...
  - [ ] OK    Fix: 

## `inputs.tabs`

- **`gradientMaps`**
  - EN: Gradient Maps
  - DE: Gradientenkarten
  - [ ] OK    Fix: 

- **`inputParams`**
  - EN: Input Parameters
  - DE: Eingangsparameter
  - [ ] OK    Fix: 

- **`liveSourceHackoustics`**
  - EN: Live Source & Hackoustics
  - DE: Live Source & Hackoustik
  - [ ] OK    Fix: 

- **`movements`**
  - EN: Movements
  - DE: Bewegungen
  - [ ] OK    Fix: 

- **`sampler`**
  - EN: Sampler
  - DE: Sampler
  - [ ] OK    Fix: 

- **`visualisation`**
  - EN: Visualisation
  - DE: Visualisierung
  - [ ] OK    Fix: 

## `inputs.toggles`

- **`absolute`**
  - EN: Absolute
  - DE: Absolut
  - [ ] OK    Fix: 

- **`acousticPrecedence`**
  - EN: Acoustic Precedence
  - DE: Akustische Präzedenz
  - [ ] OK    Fix: 

- **`attenuationLawLog`**
  - EN: Log
  - DE: Log
  - [ ] OK    Fix: 

- **`constraintROff`**
  - EN: Constraint R: OFF
  - DE: Beschränkung R: AUS
  - [ ] OK    Fix: 

- **`constraintROn`**
  - EN: Constraint R: ON
  - DE: Beschränkung R: EIN
  - [ ] OK    Fix: 

- **`constraintXOff`**
  - EN: Constraint X: OFF
  - DE: Beschränkung X: AUS
  - [ ] OK    Fix: 

- **`constraintXOn`**
  - EN: Constraint X: ON
  - DE: Beschränkung X: EIN
  - [ ] OK    Fix: 

- **`constraintYOff`**
  - EN: Constraint Y: OFF
  - DE: Beschränkung Y: AUS
  - [ ] OK    Fix: 

- **`constraintYOn`**
  - EN: Constraint Y: ON
  - DE: Beschränkung Y: EIN
  - [ ] OK    Fix: 

- **`constraintZOff`**
  - EN: Constraint Z: OFF
  - DE: Beschränkung Z: AUS
  - [ ] OK    Fix: 

- **`constraintZOn`**
  - EN: Constraint Z: ON
  - DE: Beschränkung Z: EIN
  - [ ] OK    Fix: 

- **`flipXOff`**
  - EN: Flip X: OFF
  - DE: X spiegeln: AUS
  - [ ] OK    Fix: 

- **`flipXOn`**
  - EN: Flip X: ON
  - DE: X spiegeln: EIN
  - [ ] OK    Fix: 

- **`flipYOff`**
  - EN: Flip Y: OFF
  - DE: Y spiegeln: AUS
  - [ ] OK    Fix: 

- **`flipYOn`**
  - EN: Flip Y: ON
  - DE: Y spiegeln: EIN
  - [ ] OK    Fix: 

- **`flipZOff`**
  - EN: Flip Z: OFF
  - DE: Z spiegeln: AUS
  - [ ] OK    Fix: 

- **`flipZOn`**
  - EN: Flip Z: ON
  - DE: Z spiegeln: EIN
  - [ ] OK    Fix: 

- **`floorReflectionsOff`**
  - EN: Floor Reflections: OFF
  - DE: Bodenreflexionen: AUS
  - [ ] OK    Fix: 

- **`floorReflectionsOn`**
  - EN: Floor Reflections: ON
  - DE: Bodenreflexionen: EIN
  - [ ] OK    Fix: 

- **`highShelfOff`**
  - EN: High Shelf: OFF
  - DE: Höhenregler: AUS
  - [ ] OK    Fix: 

- **`highShelfOn`**
  - EN: High Shelf: ON
  - DE: Höhenregler: EIN
  - [ ] OK    Fix: 

- **`lfoOff`**
  - EN: L.F.O: OFF
  - DE: L.F.O: AUS
  - [ ] OK    Fix: 

- **`lfoOn`**
  - EN: L.F.O: ON
  - DE: L.F.O: EIN
  - [ ] OK    Fix: 

- **`liveSourceTamerOff`**
  - EN: Live Source Tamer: OFF
  - DE: Zähmer: AUS
  - [ ] OK    Fix: 

- **`liveSourceTamerOn`**
  - EN: Live Source Tamer: ON
  - DE: Zähmer: EIN
  - [ ] OK    Fix: 

- **`lowCutOff`**
  - EN: Low Cut: OFF
  - DE: Tiefpass: AUS
  - [ ] OK    Fix: 

- **`lowCutOn`**
  - EN: Low Cut: ON
  - DE: Tiefpass: EIN
  - [ ] OK    Fix: 

- **`lsPeakOff`**
  - EN: Peak: OFF
  - DE: Peak: AUS
  - [ ] OK    Fix: 

- **`lsPeakOn`**
  - EN: Peak: ON
  - DE: Peak: EIN
  - [ ] OK    Fix: 

- **`lsSlowOff`**
  - EN: Slow: OFF
  - DE: Slow: AUS
  - [ ] OK    Fix: 

- **`lsSlowOn`**
  - EN: Slow: ON
  - DE: Slow: EIN
  - [ ] OK    Fix: 

- **`manual`**
  - EN: Manual
  - DE: Manuell
  - [ ] OK    Fix: 

- **`maxSpeedOff`**
  - EN: Max Speed: OFF
  - DE: Max. Geschw.: AUS
  - [ ] OK    Fix: 

- **`maxSpeedOn`**
  - EN: Max Speed: ON
  - DE: Max. Geschw.: EIN
  - [ ] OK    Fix: 

- **`minimalLatency`**
  - EN: Minimal Latency
  - DE: Minimale Latenz
  - [ ] OK    Fix: 

- **`pathModeOff`**
  - EN: Path Mode: OFF
  - DE: Pfadmodus: AUS
  - [ ] OK    Fix: 

- **`pathModeOn`**
  - EN: Path Mode: ON
  - DE: Pfadmodus: EIN
  - [ ] OK    Fix: 

- **`relative`**
  - EN: Relative
  - DE: Relativ
  - [ ] OK    Fix: 

- **`return`**
  - EN: Return
  - DE: Zurückkehren
  - [ ] OK    Fix: 

- **`reverbSendsMuted`**
  - EN: Sends to Reverbs: Muted
  - DE: Hall-Sends: Stumm
  - [ ] OK    Fix: 

- **`reverbSendsUnmuted`**
  - EN: Sends to Reverbs: Unmuted
  - DE: Hall-Sends: Aktiv
  - [ ] OK    Fix: 

- **`sidelinesOff`**
  - EN: Sidelines Off
  - DE: Seitenlinien Aus
  - [ ] OK    Fix: 

- **`sidelinesOn`**
  - EN: Sidelines On
  - DE: Seitenlinien Ein
  - [ ] OK    Fix: 

- **`stay`**
  - EN: Stay
  - DE: Bleiben
  - [ ] OK    Fix: 

- **`trackingOff`**
  - EN: Tracking: OFF
  - DE: Tracking: AUS
  - [ ] OK    Fix: 

- **`trackingOn`**
  - EN: Tracking: ON
  - DE: Tracking: EIN
  - [ ] OK    Fix: 

- **`triggered`**
  - EN: Triggered
  - DE: Ausgelöst
  - [ ] OK    Fix: 

## `inputs.visualisation`

- **`delay`**
  - EN: delay
  - DE: Delay
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: HF
damping
  - DE: HF-
Dämpfung
  - [ ] OK    Fix: 

- **`level`**
  - EN: level
  - DE: Pegel
  - [ ] OK    Fix: 

## `levelMeter`

- **`inputs`**
  - EN: Inputs
  - DE: Eingänge
  - [ ] OK    Fix: 

- **`outputs`**
  - EN: Outputs
  - DE: Ausgänge
  - [ ] OK    Fix: 

- **`windowTitle`**
  - EN: Level Meters
  - DE: Pegelmesser
  - [ ] OK    Fix: 

## `levelMeter.buttons`

- **`clearSolo`**
  - EN: Clear Solo
  - DE: Solo löschen
  - [ ] OK    Fix: 

- **`soloModeMulti`**
  - EN: Multi
  - DE: Multi
  - [ ] OK    Fix: 

- **`soloModeSingle`**
  - EN: Single
  - DE: Single
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

## `map`

- **`detachedMessage`**
  - EN: The map is displayed in a separate window.
  - DE: Die Karte wird in einem separaten Fenster angezeigt.
  - [ ] OK    Fix: 

- **`reattach`**
  - EN: Re-attach Map
  - DE: Karte wieder anheften
  - [ ] OK    Fix: 

## `map.buttons`

- **`fitInputs`**
  - EN: Fit All Inputs to Screen
  - DE: Alle Eingänge an Bildschirm anpassen
  - [ ] OK    Fix: 

- **`fitStage`**
  - EN: Fit Stage to Screen
  - DE: Bühne an Bildschirm anpassen
  - [ ] OK    Fix: 

- **`hideLevels`**
  - EN: Hide Levels
  - DE: Pegel ausblenden
  - [ ] OK    Fix: 

- **`showLevels`**
  - EN: Show Levels
  - DE: Pegel anzeigen
  - [ ] OK    Fix: 

## `map.labels`

- **`reverbPrefix`**
  - EN: R
  - DE: H
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

## `network.buttons`

- **`add`**
  - EN: ADD
  - DE: HINZUFÜGEN
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - DE: Exportieren
  - [ ] OK    Fix: 

- **`findMyRemote`**
  - EN: Find My Remote
  - DE: Fernbedienung finden
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - DE: Importieren
  - [ ] OK    Fix: 

- **`openLogWindow`**
  - EN: Open Log Window
  - DE: Log-Fenster öffnen
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Backup
  - DE: Backup laden
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Network Config
  - DE: Netzwerkkonf. neu laden
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Network Config
  - DE: Netzwerkkonf. speichern
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
  - EN: 
Only one tracked input per cluster is allowed. If you continue, tracking will be kept only for the first input in each cluster.
  - DE: 
Nur ein getrackter Eingang pro Cluster ist erlaubt. Wenn Sie fortfahren, wird das Tracking nur für den ersten Eingang in jedem Cluster beibehalten.
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

- **`admMapping`**
  - EN: Select an ADM-OSC mapping to configure. Cart = Cartesian (xyz), Polar = spherical (aed).
  - DE: ADM-OSC-Zuordnung auswählen. Cart = Kartesisch (xyz), Polar = sphärisch (aed).
  - [ ] OK    Fix: 

- **`admMappingPanel`**
  - EN: Drag dots to edit mapping. Click axis title to swap, click Flip to invert. Hold Shift to edit both sides symmetrically.
  - DE: Punkte ziehen, um die Zuordnung zu bearbeiten. Achsentitel klicken zum Tauschen, Flip klicken zum Invertieren. Shift halten, um beide Seiten symmetrisch zu bearbeiten.
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
  - DE: Netzwerk-Schnittstelle auswählen.
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
  - DE: OSC-Pfad für das Tracking im OSC-Modus (beginnt mit /)
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

## `network.labels`

- **`admMapping`**
  - EN: Mapping:
  - DE: Zuordnung:
  - [ ] OK    Fix: 

- **`currentIPv4`**
  - EN: Current IPv4:
  - DE: Aktuelle IPv4:
  - [ ] OK    Fix: 

- **`interface`**
  - EN: Network Interface:
  - DE: Netzwerkschnittstelle:
  - [ ] OK    Fix: 

- **`localhost`**
  - EN: Localhost (127.0.0.1)
  - DE: Localhost (127.0.0.1)
  - [ ] OK    Fix: 

- **`mqttHost`**
  - EN: Host:
  - DE: Host:
  - [ ] OK    Fix: 

- **`mqttJsonQ`**
  - EN: Q:
  - DE: Q:
  - [ ] OK    Fix: 

- **`mqttJsonX`**
  - EN: X:
  - DE: X:
  - [ ] OK    Fix: 

- **`mqttJsonY`**
  - EN: Y:
  - DE: Y:
  - [ ] OK    Fix: 

- **`mqttJsonZ`**
  - EN: Z:
  - DE: Z:
  - [ ] OK    Fix: 

- **`mqttTagIds`**
  - EN: Tag IDs...
  - DE: Tag-IDs...
  - [ ] OK    Fix: 

- **`mqttTopic`**
  - EN: Topic:
  - DE: Thema:
  - [ ] OK    Fix: 

- **`notAvailable`**
  - EN: Not available
  - DE: Nicht verfügbar
  - [ ] OK    Fix: 

- **`offsetX`**
  - EN: Offset X:
  - DE: Offset X:
  - [ ] OK    Fix: 

- **`offsetY`**
  - EN: Offset Y:
  - DE: Offset Y:
  - [ ] OK    Fix: 

- **`offsetZ`**
  - EN: Offset Z:
  - DE: Offset Z:
  - [ ] OK    Fix: 

- **`oscPath`**
  - EN: OSC Path:
  - DE: OSC Path:
  - [ ] OK    Fix: 

- **`oscQuery`**
  - EN: OSC Query:
  - DE: OSC Query:
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Protocol:
  - DE: Protokoll:
  - [ ] OK    Fix: 

- **`psnInterface`**
  - EN: PSN Interface:
  - DE: PSN Interface:
  - [ ] OK    Fix: 

- **`rxPort`**
  - EN: Rx Port:
  - DE: Rx-Port:
  - [ ] OK    Fix: 

- **`scaleX`**
  - EN: Scale X:
  - DE: Skalierung X:
  - [ ] OK    Fix: 

- **`scaleY`**
  - EN: Scale Y:
  - DE: Skalierung Y:
  - [ ] OK    Fix: 

- **`scaleZ`**
  - EN: Scale Z:
  - DE: Skalierung Z:
  - [ ] OK    Fix: 

- **`tcpPort`**
  - EN: TCP Port:
  - DE: TCP-Port:
  - [ ] OK    Fix: 

- **`udpPort`**
  - EN: UDP Port:
  - DE: UDP-Port:
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
  - DE: Bitte wählen Sie zuerst einen Projektordner in der Systemkonfiguration.
  - [ ] OK    Fix: 

## `network.protocols`

- **`admOsc`**
  - EN: ADM-OSC
  - DE: ADM-OSC
  - [ ] OK    Fix: 

- **`disabled`**
  - EN: DISABLED
  - DE: DEAKTIVIERT
  - [ ] OK    Fix: 

- **`mqtt`**
  - EN: MQTT
  - DE: MQTT
  - [ ] OK    Fix: 

- **`osc`**
  - EN: OSC
  - DE: OSC
  - [ ] OK    Fix: 

- **`psn`**
  - EN: PosiStageNet (PSN)
  - DE: PosiStageNet (PSN)
  - [ ] OK    Fix: 

- **`qlab`**
  - EN: QLab
  - DE: QLab
  - [ ] OK    Fix: 

- **`remote`**
  - EN: Remote
  - DE: Fernsteuerung
  - [ ] OK    Fix: 

- **`rttrp`**
  - EN: RTTrP
  - DE: RTTrP
  - [ ] OK    Fix: 

- **`tcp`**
  - EN: TCP
  - DE: TCP
  - [ ] OK    Fix: 

- **`udp`**
  - EN: UDP
  - DE: UDP
  - [ ] OK    Fix: 

## `network.sections`

- **`admOsc`**
  - EN: ADM-OSC Mappings
  - DE: ADM-OSC-Zuordnungen
  - [ ] OK    Fix: 

- **`connections`**
  - EN: Network Connections
  - DE: Netzwerkverbindungen
  - [ ] OK    Fix: 

- **`network`**
  - EN: Network
  - DE: Netzwerk
  - [ ] OK    Fix: 

- **`tracking`**
  - EN: Tracking
  - DE: Tracking
  - [ ] OK    Fix: 

## `network.table`

- **`defaultTarget`**
  - EN: Target {num}
  - DE: Ziel {num}
  - [ ] OK    Fix: 

- **`ipv4Address`**
  - EN: IPv4 Address
  - DE: IPv4-Adresse
  - [ ] OK    Fix: 

- **`mode`**
  - EN: Mode
  - DE: Modus
  - [ ] OK    Fix: 

- **`name`**
  - EN: Name
  - DE: Name
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Protocol
  - DE: Protokoll
  - [ ] OK    Fix: 

- **`rx`**
  - EN: Rx
  - DE: Rx
  - [ ] OK    Fix: 

- **`tx`**
  - EN: Tx
  - DE: Tx
  - [ ] OK    Fix: 

- **`txPort`**
  - EN: Tx Port
  - DE: Tx-Port
  - [ ] OK    Fix: 

## `network.toggles`

- **`disabled`**
  - EN: Disabled
  - DE: Deaktiviert
  - [ ] OK    Fix: 

- **`enabled`**
  - EN: Enabled
  - DE: Aktiviert
  - [ ] OK    Fix: 

- **`flipXOff`**
  - EN: Flip X: OFF
  - DE: X spiegeln: AUS
  - [ ] OK    Fix: 

- **`flipXOn`**
  - EN: Flip X: ON
  - DE: X spiegeln: EIN
  - [ ] OK    Fix: 

- **`flipYOff`**
  - EN: Flip Y: OFF
  - DE: Y spiegeln: AUS
  - [ ] OK    Fix: 

- **`flipYOn`**
  - EN: Flip Y: ON
  - DE: Y spiegeln: EIN
  - [ ] OK    Fix: 

- **`flipZOff`**
  - EN: Flip Z: OFF
  - DE: Z spiegeln: AUS
  - [ ] OK    Fix: 

- **`flipZOn`**
  - EN: Flip Z: ON
  - DE: Z spiegeln: EIN
  - [ ] OK    Fix: 

- **`off`**
  - EN: OFF
  - DE: AUS
  - [ ] OK    Fix: 

- **`on`**
  - EN: ON
  - DE: EIN
  - [ ] OK    Fix: 

- **`oscFilterAcceptAll`**
  - EN: OSC Filter: Accept All
  - DE: OSC-Filter: Alle akzeptieren
  - [ ] OK    Fix: 

- **`oscFilterRegisteredOnly`**
  - EN: OSC Filter: Registered Only
  - DE: OSC-Filter: Nur registrierte
  - [ ] OK    Fix: 

- **`trackingOff`**
  - EN: Tracking: OFF
  - DE: Tracking: AUS
  - [ ] OK    Fix: 

- **`trackingOn`**
  - EN: Tracking: ON
  - DE: Tracking: EIN
  - [ ] OK    Fix: 

## `networkLog`

- **`windowTitle`**
  - EN: Network Log
  - DE: Netzwerk-Log
  - [ ] OK    Fix: 

## `networkLog.columns`

- **`address`**
  - EN: Address
  - DE: Adresse
  - [ ] OK    Fix: 

- **`arguments`**
  - EN: Arguments
  - DE: Argumente
  - [ ] OK    Fix: 

- **`direction`**
  - EN: Dir
  - DE: Richt.
  - [ ] OK    Fix: 

- **`ip`**
  - EN: IP
  - DE: IP
  - [ ] OK    Fix: 

- **`origin`**
  - EN: Origin
  - DE: Quelle
  - [ ] OK    Fix: 

- **`port`**
  - EN: Port
  - DE: Port
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Protocol
  - DE: Protokoll
  - [ ] OK    Fix: 

- **`time`**
  - EN: Time
  - DE: Zeit
  - [ ] OK    Fix: 

- **`transport`**
  - EN: Trans
  - DE: Trans.
  - [ ] OK    Fix: 

## `networkLog.controls`

- **`clear`**
  - EN: CLEAR
  - DE: LÖSCHEN
  - [ ] OK    Fix: 

- **`export`**
  - EN: EXPORT
  - DE: EXPORTIEREN
  - [ ] OK    Fix: 

- **`hideHeartbeat`**
  - EN: Hide Heartbeat
  - DE: Heartbeat ausblenden
  - [ ] OK    Fix: 

- **`logging`**
  - EN: Logging
  - DE: Protokollierung
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

## `networkLog.exportMenu`

- **`exportAll`**
  - EN: Export All
  - DE: Alle exportieren
  - [ ] OK    Fix: 

- **`exportFiltered`**
  - EN: Export Filtered
  - DE: Gefilterte exportieren
  - [ ] OK    Fix: 

## `networkLog.filterModes`

- **`clientIp`**
  - EN: Client IP
  - DE: Client-IP
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Protocol
  - DE: Protokoll
  - [ ] OK    Fix: 

- **`rejected`**
  - EN: Rejected
  - DE: Abgelehnt
  - [ ] OK    Fix: 

- **`tcpUdp`**
  - EN: TCP/UDP
  - DE: TCP/UDP
  - [ ] OK    Fix: 

## `networkLog.filterToggles`

- **`incoming`**
  - EN: Incoming
  - DE: Eingehend
  - [ ] OK    Fix: 

- **`outgoing`**
  - EN: Outgoing
  - DE: Ausgehend
  - [ ] OK    Fix: 

- **`tcp`**
  - EN: TCP
  - DE: TCP
  - [ ] OK    Fix: 

- **`udp`**
  - EN: UDP
  - DE: UDP
  - [ ] OK    Fix: 

## `networkLog.status`

- **`rejected`**
  - EN: REJECTED
  - DE: ABGELEHNT
  - [ ] OK    Fix: 

## `outputs.arrayModes`

- **`absolute`**
  - EN: ABSOLUTE
  - DE: ABSOLUT
  - [ ] OK    Fix: 

- **`array`**
  - EN: Array
  - DE: Array
  - [ ] OK    Fix: 

- **`off`**
  - EN: OFF
  - DE: AUS
  - [ ] OK    Fix: 

- **`relative`**
  - EN: RELATIVE
  - DE: RELATIV
  - [ ] OK    Fix: 

- **`single`**
  - EN: Single
  - DE: Einzeln
  - [ ] OK    Fix: 

## `outputs.buttons`

- **`arrayHidden`**
  - EN: Array Hidden on Map
  - DE: Array auf Karte ausgeblendet
  - [ ] OK    Fix: 

- **`arrayVisible`**
  - EN: Array Visible on Map
  - DE: Array auf Karte sichtbar
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - DE: Exportieren
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - DE: Importieren
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Backup
  - DE: Backup laden
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Output Config
  - DE: Ausgangskonf. neu laden
  - [ ] OK    Fix: 

- **`speakerHidden`**
  - EN: Speaker Hidden on Map
  - DE: Lautsprecher auf Karte ausgeblendet
  - [ ] OK    Fix: 

- **`speakerVisible`**
  - EN: Speaker Visible on Map
  - DE: Lautsprecher auf Karte sichtbar
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Output Config
  - DE: Ausgangskonf. speichern
  - [ ] OK    Fix: 

- **`wizardOfOutZ`**
  - EN: Wizard of OutZ...
  - DE: Assistent OutZ...
  - [ ] OK    Fix: 

## `outputs.coordModes`

- **`xyz`**
  - EN: XYZ
  - DE: XYZ
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
  - DE: Ausgangs-EQ Band {band} Frequenz (20 Hz - 20 kHz).
  - [ ] OK    Fix: 

- **`eqGain`**
  - EN: Output EQ Band {band} gain (-24 to +24 dB).
  - DE: Ausgangs-EQ Band {band} Gain (-24 bis +24 dB).
  - [ ] OK    Fix: 

- **`eqQ`**
  - EN: Output EQ Band {band} Q factor (0.1 - 10.0).
  - DE: Ausgangs-EQ Band {band} Q-Faktor (0.1 - 10.0).
  - [ ] OK    Fix: 

- **`eqResetBand`**
  - EN: Long press to reset Band {band} to its default values.
  - DE: Langer Druck, um Band {band} zurückzusetzen.
  - [ ] OK    Fix: 

- **`eqShape`**
  - EN: Output EQ Band {band} filter shape.
  - DE: Ausgangs-EQ Band {band} Filterform.
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Output Configuration to file (with file explorer window).
  - DE: Ausgangskonfiguration in Datei exportieren (mit Datei-Explorer-Fenster).
  - [ ] OK    Fix: 

- **`floorReflections`**
  - EN: Enable or Disable the Floor Reflections for this Speaker.
  - DE: Bodenreflexionen für diesen Lautsprecher aktivieren oder deaktivieren.
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: Loss of High Frequency Depending on Distance from Object to Output. (changes may affect the rest of the array)
  - DE: Hochfrequenzverlust abhängig von der Entfernung des Objekts zum Ausgang. (Änderungen können den Rest des Arrays betreffen)
  - [ ] OK    Fix: 

- **`hParallax`**
  - EN: Horizontal Distance from Speaker to 'Targeted' Listener. (changes may affect the rest of the array)
  - DE: Horizontaler Abstand vom Lautsprecher zum „anvisierten“ Zuhörer. (Änderungen können den Rest des Arrays betreffen)
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Output Configuration from file (with file explorer window).
  - DE: Ausgangskonfiguration aus Datei importieren (mit Datei-Explorer-Fenster).
  - [ ] OK    Fix: 

- **`liveSource`**
  - EN: Disables Live Source Attenuation for Selected Output. (changes may affect the rest of the array)
  - DE: Deaktiviert die Live-Quellen-Dämpfung für den ausgewählten Ausgang. (Änderungen können den Rest des Arrays betreffen)
  - [ ] OK    Fix: 

- **`mapVisibility`**
  - EN: Make Visible or Hide The Selected Output on the Map.
  - DE: Den ausgewählten Ausgang auf der Karte sichtbar machen oder ausblenden.
  - [ ] OK    Fix: 

- **`minLatency`**
  - EN: Disables Minimal Latency Mode for Selected Output. (changes may affect the rest of the array)
  - DE: Deaktiviert den Modus minimale Latenz für den ausgewählten Ausgang. (Änderungen können den Rest des Arrays betreffen)
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

## `outputs.labels`

- **`angleOff`**
  - EN: Angle Off:
  - DE: Winkel Aus:
  - [ ] OK    Fix: 

- **`angleOn`**
  - EN: Angle On:
  - DE: Winkel Ein:
  - [ ] OK    Fix: 

- **`applyToArray`**
  - EN: Apply to Array:
  - DE: Auf Array anwenden:
  - [ ] OK    Fix: 

- **`array`**
  - EN: Array:
  - DE: Array:
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Attenuation:
  - DE: Dämpfung:
  - [ ] OK    Fix: 

- **`coordinates`**
  - EN: Coordinates:
  - DE: Koordinaten:
  - [ ] OK    Fix: 

- **`delay`**
  - EN: Delay:
  - DE: Verzögerung:
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Delay/Latency:
  - DE: Verzögerung/Latenz:
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Distance Atten:
  - DE: Entfernungsdämpf.:
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: HF Damping:
  - DE: HF-Dämpfung:
  - [ ] OK    Fix: 

- **`hParallax`**
  - EN: Horizontal Parallax:
  - DE: Horiz. Parallaxe:
  - [ ] OK    Fix: 

- **`latency`**
  - EN: Latency:
  - DE: Latenz:
  - [ ] OK    Fix: 

- **`name`**
  - EN: Name:
  - DE: Name:
  - [ ] OK    Fix: 

- **`orientation`**
  - EN: Orientation:
  - DE: Orientierung:
  - [ ] OK    Fix: 

- **`pitch`**
  - EN: Pitch:
  - DE: Neigung:
  - [ ] OK    Fix: 

- **`positionX`**
  - EN: Position X:
  - DE: Position X:
  - [ ] OK    Fix: 

- **`positionY`**
  - EN: Position Y:
  - DE: Position Y:
  - [ ] OK    Fix: 

- **`positionZ`**
  - EN: Position Z:
  - DE: Position Z:
  - [ ] OK    Fix: 

- **`vParallax`**
  - EN: Vertical Parallax:
  - DE: Vert. Parallaxe:
  - [ ] OK    Fix: 

## `outputs.messages`

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

- **`configSaved`**
  - EN: Output configuration saved.
  - DE: Ausgangskonfiguration gespeichert.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - DE: Fehler: {error}
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - DE: Bitte wählen Sie zuerst einen Projektordner in der Systemkonfiguration.
  - [ ] OK    Fix: 

- **`setToSingle`**
  - EN: Output {num} set to Single
  - DE: Ausgang {num} auf Einzeln gesetzt
  - [ ] OK    Fix: 

## `outputs.tabs`

- **`eq`**
  - EN: Output EQ
  - DE: Ausgangs-EQ
  - [ ] OK    Fix: 

- **`parameters`**
  - EN: Output Parameters
  - DE: Ausgangsparameter
  - [ ] OK    Fix: 

## `outputs.toggles`

- **`floorReflectionsOff`**
  - EN: Floor Reflections: OFF
  - DE: Bodenreflexionen: AUS
  - [ ] OK    Fix: 

- **`floorReflectionsOn`**
  - EN: Floor Reflections: ON
  - DE: Bodenreflexionen: EIN
  - [ ] OK    Fix: 

- **`liveSourceOff`**
  - EN: Live Source Atten: OFF
  - DE: Live-Quelle Dämpf.: AUS
  - [ ] OK    Fix: 

- **`liveSourceOn`**
  - EN: Live Source Atten: ON
  - DE: Live-Quelle Dämpf.: EIN
  - [ ] OK    Fix: 

- **`minLatencyOff`**
  - EN: Minimal Latency: OFF
  - DE: Minimale Latenz: AUS
  - [ ] OK    Fix: 

- **`minLatencyOn`**
  - EN: Minimal Latency: ON
  - DE: Minimale Latenz: EIN
  - [ ] OK    Fix: 

## `outputs.units`

- **`meters`**
  - EN: m
  - DE: m
  - [ ] OK    Fix: 

- **`ms`**
  - EN: ms
  - DE: ms
  - [ ] OK    Fix: 

## `reverbs`

- **`noChannels`**
  - EN: No reverb channels configured.

Set the number of Reverb Channels in System Config.
  - DE: Keine Hallkanäle konfiguriert.

Setzen Sie die Anzahl der Hallkanäle in der Systemkonfiguration.
  - [ ] OK    Fix: 

## `reverbs.algorithm`

- **`crossoverHigh`**
  - EN: Crossover High:
  - DE: Trennfrequenz hoch:
  - [ ] OK    Fix: 

- **`crossoverLow`**
  - EN: Crossover Low:
  - DE: Trennfrequenz tief:
  - [ ] OK    Fix: 

- **`decaySection`**
  - EN: Decay
  - DE: Abklingen
  - [ ] OK    Fix: 

- **`diffusion`**
  - EN: Diffusion:
  - DE: Diffusion:
  - [ ] OK    Fix: 

- **`fdn`**
  - EN: FDN
  - DE: FDN
  - [ ] OK    Fix: 

- **`fdnSection`**
  - EN: FDN
  - DE: FDN
  - [ ] OK    Fix: 

- **`ir`**
  - EN: IR
  - DE: IR
  - [ ] OK    Fix: 

- **`irFile`**
  - EN: IR File:
  - DE: IR-Datei:
  - [ ] OK    Fix: 

- **`irImport`**
  - EN: Import IR...
  - DE: IR importieren...
  - [ ] OK    Fix: 

- **`irImportSuccess`**
  - EN: IR imported: {file}
  - DE: IR importiert: {file}
  - [ ] OK    Fix: 

- **`irLength`**
  - EN: IR Length:
  - DE: IR-Länge:
  - [ ] OK    Fix: 

- **`irNoProject`**
  - EN: Set a project folder first
  - DE: Projektordner zuerst festlegen
  - [ ] OK    Fix: 

- **`irSection`**
  - EN: IR
  - DE: IR
  - [ ] OK    Fix: 

- **`irTrim`**
  - EN: IR Trim:
  - DE: IR-Zuschnitt:
  - [ ] OK    Fix: 

- **`noFileLoaded`**
  - EN: No IR loaded
  - DE: Keine IR geladen
  - [ ] OK    Fix: 

- **`perNodeOff`**
  - EN: Per-node IR OFF
  - DE: IR pro Knoten AUS
  - [ ] OK    Fix: 

- **`perNodeOn`**
  - EN: Per-node IR ON
  - DE: IR pro Knoten EIN
  - [ ] OK    Fix: 

- **`rt60`**
  - EN: RT60:
  - DE: RT60:
  - [ ] OK    Fix: 

- **`rt60HighMult`**
  - EN: RT60 High ×:
  - DE: RT60 Hoch ×:
  - [ ] OK    Fix: 

- **`rt60LowMult`**
  - EN: RT60 Low ×:
  - DE: RT60 Tief ×:
  - [ ] OK    Fix: 

- **`scale`**
  - EN: Scale:
  - DE: Skalierung:
  - [ ] OK    Fix: 

- **`sdn`**
  - EN: SDN
  - DE: SDN
  - [ ] OK    Fix: 

- **`sdnSection`**
  - EN: SDN
  - DE: SDN
  - [ ] OK    Fix: 

- **`size`**
  - EN: Size:
  - DE: Größe:
  - [ ] OK    Fix: 

- **`wetLevel`**
  - EN: Wet Level:
  - DE: Wet-Pegel:
  - [ ] OK    Fix: 

## `reverbs.buttons`

- **`editOnMap`**
  - EN: Edit on Map
  - DE: Auf Karte bearbeiten
  - [ ] OK    Fix: 

- **`editOnMapOn`**
  - EN: Edit on Map ON
  - DE: Auf Karte bearbeiten EIN
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - DE: Exportieren
  - [ ] OK    Fix: 

- **`hiddenOnMap`**
  - EN: Reverbs Hidden on Map
  - DE: Hall auf Karte ausgeblendet
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - DE: Importieren
  - [ ] OK    Fix: 

- **`mutePost`**
  - EN: Mute Post
  - DE: Mute Post
  - [ ] OK    Fix: 

- **`mutePostOn`**
  - EN: Mute Post ON
  - DE: Mute Post EIN
  - [ ] OK    Fix: 

- **`mutePre`**
  - EN: Mute Pre
  - DE: Mute Pre
  - [ ] OK    Fix: 

- **`mutePreOn`**
  - EN: Mute Pre ON
  - DE: Mute Pre EIN
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Backup
  - DE: Backup laden
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Reverb Config
  - DE: Hallkonf. neu laden
  - [ ] OK    Fix: 

- **`soloReverbs`**
  - EN: Solo Reverbs
  - DE: Solo Reverbs
  - [ ] OK    Fix: 

- **`soloReverbsOn`**
  - EN: Solo Reverbs ON
  - DE: Solo Hall EIN
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Reverb Config
  - DE: Hallkonf. speichern
  - [ ] OK    Fix: 

- **`visibleOnMap`**
  - EN: Reverbs Visible on Map
  - DE: Hall auf Karte sichtbar
  - [ ] OK    Fix: 

## `reverbs.coordModes`

- **`xyz`**
  - EN: XYZ
  - DE: XYZ
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
  - DE: Entfernungsdämpfung für den Hallrücklauf (-6.0 bis 0.0 dB/m).
  - [ ] OK    Fix: 

- **`distanceAttenEnable`**
  - EN: Distance attenuation percentage (0-200%).
  - DE: Prozentsatz der Entfernungsdämpfung (0-200%).
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
  - EN: Enable Live Source attenuation tamer. Reduces level fluctuations from sources close to the array.
  - DE: Live-Source-Dämpfungsbändiger aktivieren. Reduziert Pegelschwankungen von Quellen nahe dem Array.
  - [ ] OK    Fix: 

- **`mapVisibility`**
  - EN: Make Visible or Hide All Reverb Channels on the Map.
  - DE: Alle Hallkanäle auf der Karte sichtbar machen oder ausblenden.
  - [ ] OK    Fix: 

- **`miniLatencyTooltip`**
  - EN: Enable minimal latency mode for this reverb channel. Reduces processing delay at the cost of higher CPU usage.
  - DE: Minimaler Latenzmodus für diesen Hallkanal aktivieren. Reduziert die Verarbeitungsverzögerung auf Kosten höherer CPU-Auslastung.
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

## `reverbs.labels`

- **`angleOff`**
  - EN: Angle Off:
  - DE: Winkel Aus:
  - [ ] OK    Fix: 

- **`angleOn`**
  - EN: Angle On:
  - DE: Winkel Ein:
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Attenuation:
  - DE: Dämpfung:
  - [ ] OK    Fix: 

- **`commonAtten`**
  - EN: Common Atten:
  - DE: Gem. Dämpf.:
  - [ ] OK    Fix: 

- **`coordinates`**
  - EN: Coord:
  - DE: Koord.:
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Delay/Latency:
  - DE: Verzögerung/Latenz:
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Distance Atten:
  - DE: Entf.-Dämpf.:
  - [ ] OK    Fix: 

- **`distanceAttenPercent`**
  - EN: Distance Atten %:
  - DE: Entf.-Dämpf. %:
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: HF Damping:
  - DE: HF-Dämpfung:
  - [ ] OK    Fix: 

- **`muteMacro`**
  - EN: Mute Macro:
  - DE: Stummschaltungsmakro:
  - [ ] OK    Fix: 

- **`name`**
  - EN: Name:
  - DE: Name:
  - [ ] OK    Fix: 

- **`orientation`**
  - EN: Orientation:
  - DE: Orientierung:
  - [ ] OK    Fix: 

- **`outputMutes`**
  - EN: Output Mutes:
  - DE: Ausgangsstummschaltungen:
  - [ ] OK    Fix: 

- **`pitch`**
  - EN: Pitch:
  - DE: Neigung:
  - [ ] OK    Fix: 

- **`positionX`**
  - EN: Position X:
  - DE: Position X:
  - [ ] OK    Fix: 

- **`positionY`**
  - EN: Position Y:
  - DE: Position Y:
  - [ ] OK    Fix: 

- **`positionZ`**
  - EN: Position Z:
  - DE: Position Z:
  - [ ] OK    Fix: 

- **`returnOffsetX`**
  - EN: Return Offset X:
  - DE: Rücklauf-Offset X:
  - [ ] OK    Fix: 

- **`returnOffsetY`**
  - EN: Return Offset Y:
  - DE: Rücklauf-Offset Y:
  - [ ] OK    Fix: 

- **`returnOffsetZ`**
  - EN: Return Offset Z:
  - DE: Rücklauf-Offset Z:
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
  - DE: Bitte wählen Sie zuerst einen Projektordner in der Systemkonfiguration.
  - [ ] OK    Fix: 

## `reverbs.muteMacros`

- **`invertMutes`**
  - EN: INVERT MUTES
  - DE: STUMMSCH. INVERTIEREN
  - [ ] OK    Fix: 

- **`muteAll`**
  - EN: MUTE ALL
  - DE: ALLE STUMM
  - [ ] OK    Fix: 

- **`muteArray`**
  - EN: MUTE ARRAY
  - DE: ARRAY STUMM
  - [ ] OK    Fix: 

- **`muteEven`**
  - EN: MUTE EVEN
  - DE: GERADE STUMM
  - [ ] OK    Fix: 

- **`muteOdd`**
  - EN: MUTE ODD
  - DE: UNGERADE STUMM
  - [ ] OK    Fix: 

- **`select`**
  - EN: Mute Macro Select
  - DE: Stummschaltungsmakro wählen
  - [ ] OK    Fix: 

- **`unmuteAll`**
  - EN: UNMUTE ALL
  - DE: ALLE LAUT
  - [ ] OK    Fix: 

- **`unmuteArray`**
  - EN: UNMUTE ARRAY
  - DE: ARRAY LAUT
  - [ ] OK    Fix: 

## `reverbs.postProcessing`

- **`attack`**
  - EN: Attack:
  - DE: Attack:
  - [ ] OK    Fix: 

- **`expander`**
  - EN: Expander
  - DE: Expander
  - [ ] OK    Fix: 

- **`expanderOff`**
  - EN: Expander OFF
  - DE: Expander AUS
  - [ ] OK    Fix: 

- **`expanderOn`**
  - EN: Expander ON
  - DE: Expander EIN
  - [ ] OK    Fix: 

- **`ratio`**
  - EN: Ratio:
  - DE: Verhältnis:
  - [ ] OK    Fix: 

- **`release`**
  - EN: Release:
  - DE: Release:
  - [ ] OK    Fix: 

- **`threshold`**
  - EN: Threshold:
  - DE: Schwelle:
  - [ ] OK    Fix: 

## `reverbs.preProcessing`

- **`attack`**
  - EN: Attack:
  - DE: Attack:
  - [ ] OK    Fix: 

- **`compressor`**
  - EN: Compressor
  - DE: Kompressor
  - [ ] OK    Fix: 

- **`compressorOff`**
  - EN: Compressor OFF
  - DE: Kompressor AUS
  - [ ] OK    Fix: 

- **`compressorOn`**
  - EN: Compressor ON
  - DE: Kompressor EIN
  - [ ] OK    Fix: 

- **`ratio`**
  - EN: Ratio:
  - DE: Verhältnis:
  - [ ] OK    Fix: 

- **`release`**
  - EN: Release:
  - DE: Release:
  - [ ] OK    Fix: 

- **`threshold`**
  - EN: Threshold:
  - DE: Schwelle:
  - [ ] OK    Fix: 

## `reverbs.sections`

- **`reverbFeed`**
  - EN: Reverb Feed
  - DE: Hall-Send
  - [ ] OK    Fix: 

- **`reverbReturn`**
  - EN: Reverb Return
  - DE: Hall-Return
  - [ ] OK    Fix: 

## `reverbs.tabs`

- **`algorithm`**
  - EN: Algorithm
  - DE: Algorithmus
  - [ ] OK    Fix: 

- **`channelParams`**
  - EN: Channel Parameters
  - DE: Kanalparameter
  - [ ] OK    Fix: 

- **`postProcessing`**
  - EN: Post-Processing
  - DE: Nachbearbeitung
  - [ ] OK    Fix: 

- **`preProcessing`**
  - EN: Pre-Processing
  - DE: Vorbearbeitung
  - [ ] OK    Fix: 

## `reverbs.toggles`

- **`liveSourceOff`**
  - EN: Live Source Atten OFF
  - DE: Live-Quelle Dämpf. AUS
  - [ ] OK    Fix: 

- **`liveSourceOn`**
  - EN: Live Source Atten ON
  - DE: Live-Quelle Dämpf. EIN
  - [ ] OK    Fix: 

- **`minLatencyOff`**
  - EN: Minimal Latency OFF
  - DE: Minimale Latenz AUS
  - [ ] OK    Fix: 

- **`minLatencyOn`**
  - EN: Minimal Latency ON
  - DE: Minimale Latenz EIN
  - [ ] OK    Fix: 

## `sampler.buttons`

- **`copy`**
  - EN: Copy
  - DE: Kopieren
  - [ ] OK    Fix: 

- **`copyCell`**
  - EN: Copy Cell
  - DE: Zelle kopieren
  - [ ] OK    Fix: 

- **`copySet`**
  - EN: Copy Set
  - DE: Set kopieren
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - DE: Exportieren
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - DE: Importieren
  - [ ] OK    Fix: 

- **`paste`**
  - EN: Paste
  - DE: Einfügen
  - [ ] OK    Fix: 

- **`pasteCell`**
  - EN: Paste Cell
  - DE: Zelle einfügen
  - [ ] OK    Fix: 

- **`pasteSet`**
  - EN: Paste Set
  - DE: Set einfügen
  - [ ] OK    Fix: 

## `sampler.cell`

- **`attenuation`**
  - EN: Attenuation (dB)
  - DE: Dämpfung (dB)
  - [ ] OK    Fix: 

- **`clear`**
  - EN: Clear
  - DE: Löschen
  - [ ] OK    Fix: 

- **`inOut`**
  - EN: In/Out (ms)
  - DE: Ein/Aus (ms)
  - [ ] OK    Fix: 

- **`load`**
  - EN: Load
  - DE: Laden
  - [ ] OK    Fix: 

- **`loadTitle`**
  - EN: Load Sample
  - DE: Sample laden
  - [ ] OK    Fix: 

- **`offset`**
  - EN: Offset (m)
  - DE: Offset (m)
  - [ ] OK    Fix: 

- **`preview`**
  - EN: Preview
  - DE: Vorhören
  - [ ] OK    Fix: 

- **`previewStop`**
  - EN: Stop
  - DE: Stopp
  - [ ] OK    Fix: 

## `sampler.grid`

- **`help`**
  - EN: Click = select
Shift = multi
Ctrl = set toggle
DblClick = load
  - DE: Klick=Auswahl | Shift=Mehrfach | Strg=Set umschalten | Doppelklick=Laden
  - [ ] OK    Fix: 

## `sampler.labels`

- **`lightpadZone`**
  - EN: Lightpad Zone
  - DE: Lightpad-Zone
  - [ ] OK    Fix: 

- **`selectZone`**
  - EN: Select Zone
  - DE: Zone auswählen
  - [ ] OK    Fix: 

## `sampler.lightpadZone`

- **`none`**
  - EN: None
  - DE: Keine
  - [ ] OK    Fix: 

## `sampler.press`

- **`height`**
  - EN: Height
  - DE: Höhe
  - [ ] OK    Fix: 

- **`hf`**
  - EN: HF Shelf
  - DE: HF Shelf
  - [ ] OK    Fix: 

- **`level`**
  - EN: Level
  - DE: Pegel
  - [ ] OK    Fix: 

- **`xy`**
  - EN: Position XY
  - DE: Position XY
  - [ ] OK    Fix: 

## `sampler.remote`

- **`gridLayout`**
  - EN: Grid Layout
  - DE: Raster-Layout
  - [ ] OK    Fix: 

## `sampler.section`

- **`actions`**
  - EN: ACTIONS
  - DE: AKTIONEN
  - [ ] OK    Fix: 

- **`cell`**
  - EN: CELL PROPERTIES
  - DE: ZELLENEIGENSCHAFTEN
  - [ ] OK    Fix: 

- **`pressure`**
  - EN: PRESSURE MAPPINGS
  - DE: DRUCKZUWEISUNGEN
  - [ ] OK    Fix: 

- **`set`**
  - EN: SET MANAGEMENT
  - DE: SET-VERWALTUNG
  - [ ] OK    Fix: 

## `sampler.set`

- **`copy`**
  - EN: (copy)
  - DE: (Kopie)
  - [ ] OK    Fix: 

- **`default`**
  - EN: Set
  - DE: Set
  - [ ] OK    Fix: 

- **`level`**
  - EN: Level (dB)
  - DE: Pegel (dB)
  - [ ] OK    Fix: 

- **`pos`**
  - EN: Position (m)
  - DE: Position (m)
  - [ ] OK    Fix: 

- **`rename`**
  - EN: Rename
  - DE: Umbenennen
  - [ ] OK    Fix: 

- **`roundRobin`**
  - EN: Round-Robin
  - DE: Round-Robin
  - [ ] OK    Fix: 

- **`sequential`**
  - EN: Sequential
  - DE: Sequenziell
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

- **`pressHeight`**
  - EN: Map finger pressure to vertical position (Z)
  - DE: Fingerdruck auf die vertikale Position (Z) mappen
  - [ ] OK    Fix: 

- **`pressHF`**
  - EN: Map finger pressure to high-frequency shelf attenuation
  - DE: Fingerdruck auf die High-Shelf-Dämpfung mappen
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

## `sampler.zone`

- **`remotePad`**
  - EN: Pad {num}
  - DE: Pad {num}
  - [ ] OK    Fix: 

- **`selectRemotePad`**
  - EN: Select Remote Pad
  - DE: Fern-Pad auswählen
  - [ ] OK    Fix: 

## `setAllInputs`

- **`warning`**
  - EN: Changes will apply to ALL inputs
  - DE: Änderungen werden auf ALLE Eingänge angewendet
  - [ ] OK    Fix: 

- **`windowTitle`**
  - EN: Set All Inputs
  - DE: Alle Eingänge setzen
  - [ ] OK    Fix: 

## `setAllInputs.buttons`

- **`all1d`**
  - EN: All 1/d
  - DE: Alle 1/d
  - [ ] OK    Fix: 

- **`allLog`**
  - EN: All Log
  - DE: Alle Log
  - [ ] OK    Fix: 

- **`closeWindow`**
  - EN: CLOSE WINDOW
  - DE: FENSTER SCHLIESSEN
  - [ ] OK    Fix: 

- **`flipXyzOff`**
  - EN: Flip XYZ > OFF
  - DE: XYZ spiegeln > AUS
  - [ ] OK    Fix: 

- **`resetDirectivity`**
  - EN: Reset directivity
  - DE: Richtcharakt. zurücksetzen
  - [ ] OK    Fix: 

- **`turnOffJitterLfo`**
  - EN: Turn OFF jitter & LFO
  - DE: Jitter & LFO AUS
  - [ ] OK    Fix: 

- **`turnOffLiveSource`**
  - EN: Turn OFF Live source atten.
  - DE: Live-Quelle Dämpf. AUS
  - [ ] OK    Fix: 

## `setAllInputs.coordinateModes`

- **`rThetaPhi`**
  - EN: r θ φ
  - DE: r θ φ
  - [ ] OK    Fix: 

- **`rThetaZ`**
  - EN: r θ Z
  - DE: r θ Z
  - [ ] OK    Fix: 

- **`xyz`**
  - EN: XYZ
  - DE: XYZ
  - [ ] OK    Fix: 

## `setAllInputs.labels`

- **`common`**
  - EN: common
  - DE: Gemeinsam
  - [ ] OK    Fix: 

- **`constraintPositions`**
  - EN: Constraint positions:
  - DE: Positionsbeschränkungen:
  - [ ] OK    Fix: 

- **`coordinateMode`**
  - EN: Coordinate mode:
  - DE: Koordinatenmodus:
  - [ ] OK    Fix: 

- **`dbPerMeter`**
  - EN: dB/m
  - DE: dB/m
  - [ ] OK    Fix: 

- **`distanceAttenuation`**
  - EN: Distance attenuation
  - DE: Entfernungsdämpfung
  - [ ] OK    Fix: 

- **`floorReflections`**
  - EN: Floor Reflections:
  - DE: Bodenreflexionen:
  - [ ] OK    Fix: 

- **`fringe`**
  - EN: Fringe:
  - DE: Randbereich:
  - [ ] OK    Fix: 

- **`heightFactor`**
  - EN: Height factor:
  - DE: Höhenfaktor:
  - [ ] OK    Fix: 

- **`minimalLatency`**
  - EN: Minimal Latency:
  - DE: Minimale Latenz:
  - [ ] OK    Fix: 

- **`muteMacros`**
  - EN: Mute macros:
  - DE: Stummsch.-Makros:
  - [ ] OK    Fix: 

- **`ratio`**
  - EN: ratio
  - DE: Verhältnis
  - [ ] OK    Fix: 

- **`sidelines`**
  - EN: Sidelines:
  - DE: Seitenlinien:
  - [ ] OK    Fix: 

## `snapshot`

- **`qlabExportDone`**
  - EN: QLab export complete: {count} cues created
  - DE: QLab-Export abgeschlossen: {count} Cues erstellt
  - [ ] OK    Fix: 

- **`qlabExportStarted`**
  - EN: Writing {count} cues to QLab...
  - DE: {count} Cues werden an QLab gesendet...
  - [ ] OK    Fix: 

- **`qlabGroupName`**
  - EN: Snapshot "{name}"
  - DE: Snapshot „{name}"
  - [ ] OK    Fix: 

- **`qlabMemoText`**
  - EN: Run either of the following cues to recall or update this snapshot
  - DE: Einen der folgenden Cues starten, um diesen Snapshot zu laden oder zu aktualisieren
  - [ ] OK    Fix: 

- **`qlabNoTarget`**
  - EN: No QLab target configured
  - DE: Kein QLab-Ziel konfiguriert
  - [ ] OK    Fix: 

- **`qlabReloadName`**
  - EN: Reload "{name}"
  - DE: Laden "{name}"
  - [ ] OK    Fix: 

- **`qlabUpdateName`**
  - EN: Update "{name}"
  - DE: Aktualisieren "{name}"
  - [ ] OK    Fix: 

## `snapshotScope`

- **`all`**
  - EN: ALL
  - DE: ALLE
  - [ ] OK    Fix: 

- **`applyScope`**
  - EN: Apply scope:
  - DE: Umfang anwenden:
  - [ ] OK    Fix: 

- **`autoPreselectModified`**
  - EN: Auto-preselect modified parameters
  - DE: Geänderte Parameter automatisch vorauswählen
  - [ ] OK    Fix: 

- **`title`**
  - EN: Snapshot Scope: {name}
  - DE: Snapshot-Umfang: {name}
  - [ ] OK    Fix: 

- **`whenRecalling`**
  - EN: When Recalling
  - DE: Beim Abrufen
  - [ ] OK    Fix: 

- **`whenSaving`**
  - EN: When Saving
  - DE: Beim Speichern
  - [ ] OK    Fix: 

- **`windowTitle`**
  - EN: Input Snapshot Scope
  - DE: Eingangs-Snapshot-Umfang
  - [ ] OK    Fix: 

- **`writeSnapshotLoadCue`**
  - EN: Write Snapshot Load Cue to QLab
  - DE: Snapshot-Lade-Cue in QLab schreiben
  - [ ] OK    Fix: 

- **`writeSnapshotLoadCueTooltip`**
  - EN: Also create a QLab cue to load this snapshot via OSC
  - DE: Zusätzlich einen QLab-Cue zum Laden dieses Snapshots via OSC erstellen
  - [ ] OK    Fix: 

- **`writeToQLab`**
  - EN: Write to QLab
  - DE: Zu QLab schreiben
  - [ ] OK    Fix: 

- **`writeToQLabTooltip`**
  - EN: Export scope to QLab instead of saving to file
  - DE: Scope nach QLab exportieren statt in Datei zu speichern
  - [ ] OK    Fix: 

## `snapshotScope.buttons`

- **`cancel`**
  - EN: Cancel
  - DE: Abbrechen
  - [ ] OK    Fix: 

- **`clearChanges`**
  - EN: Clear Changes
  - DE: Änderungen löschen
  - [ ] OK    Fix: 

- **`ok`**
  - EN: OK
  - DE: OK
  - [ ] OK    Fix: 

- **`selectModified`**
  - EN: Select Modified
  - DE: Geänderte auswählen
  - [ ] OK    Fix: 

## `snapshotScope.sections`

- **`attenuation`**
  - EN: Attenuation
  - DE: Dämpfung
  - [ ] OK    Fix: 

- **`automOtion`**
  - EN: AutomOtion
  - DE: AutomOtion
  - [ ] OK    Fix: 

- **`directivity`**
  - EN: Directivity
  - DE: Richtcharakteristik
  - [ ] OK    Fix: 

- **`hackoustics`**
  - EN: Hackoustics
  - DE: Hackoustik
  - [ ] OK    Fix: 

- **`input`**
  - EN: Input
  - DE: Eingang
  - [ ] OK    Fix: 

- **`lfo`**
  - EN: LFO
  - DE: LFO
  - [ ] OK    Fix: 

- **`liveSource`**
  - EN: Live Source
  - DE: Live-Quelle
  - [ ] OK    Fix: 

- **`mutes`**
  - EN: Mutes
  - DE: Stummschaltungen
  - [ ] OK    Fix: 

- **`position`**
  - EN: Position
  - DE: Position
  - [ ] OK    Fix: 

## `statusBar`

- **`displayLabel`**
  - EN: Display:
  - DE: Anzeige:
  - [ ] OK    Fix: 

- **`helpMode`**
  - EN: Help
  - DE: Hilfe
  - [ ] OK    Fix: 

- **`oscMode`**
  - EN: OSC
  - DE: OSC
  - [ ] OK    Fix: 

## `systemConfig.algorithms`

- **`inputBuffer`**
  - EN: InputBuffer (read-time delays)
  - DE: InputBuffer (Lesezeitverzögerungen)
  - [ ] OK    Fix: 

- **`outputBuffer`**
  - EN: OutputBuffer (write-time delays)
  - DE: OutputBuffer (Schreibzeitverzögerungen)
  - [ ] OK    Fix: 

## `systemConfig.binauralOutput`

- **`select`**
  - EN: Select...
  - DE: Auswählen...
  - [ ] OK    Fix: 

## `systemConfig.buttons`

- **`audioPatch`**
  - EN: Audio Interface and Patching Window
  - DE: Audio-Interface und Patch-Fenster
  - [ ] OK    Fix: 

- **`binauralOff`**
  - EN: Binaural: OFF
  - DE: Binaural: AUS
  - [ ] OK    Fix: 

- **`binauralOn`**
  - EN: Binaural: ON
  - DE: Binaural: EIN
  - [ ] OK    Fix: 

- **`clearSolo`**
  - EN: Clear Solo
  - DE: Clear Solo
  - [ ] OK    Fix: 

- **`copySystemInfo`**
  - EN: Copy System Info
  - DE: Systeminfo kopieren
  - [ ] OK    Fix: 

- **`diagnosticsCollapsed`**
  - EN: Diagnostics  [+]
  - DE: Diagnose  [+]
  - [ ] OK    Fix: 

- **`diagnosticsExpanded`**
  - EN: Diagnostics  [-]
  - DE: Diagnose  [-]
  - [ ] OK    Fix: 

- **`exportLogs`**
  - EN: Export Logs
  - DE: Protokolle exportieren
  - [ ] OK    Fix: 

- **`exportSystem`**
  - EN: Export System Configuration
  - DE: Systemkonfiguration exportieren
  - [ ] OK    Fix: 

- **`importSystem`**
  - EN: Import System Configuration
  - DE: Systemkonfiguration importieren
  - [ ] OK    Fix: 

- **`levelMeter`**
  - EN: Level Meters
  - DE: Level Meters
  - [ ] OK    Fix: 

- **`openLogFolder`**
  - EN: Open Log Folder
  - DE: Protokollordner öffnen
  - [ ] OK    Fix: 

- **`processingOff`**
  - EN: Processing: OFF
  - DE: Verarbeitung: AUS
  - [ ] OK    Fix: 

- **`processingOn`**
  - EN: Processing: ON
  - DE: Verarbeitung: EIN
  - [ ] OK    Fix: 

- **`quickLongPressOff`**
  - EN: Normal
  - DE: Normal
  - [ ] OK    Fix: 

- **`quickLongPressOn`**
  - EN: Quick
  - DE: Schnell
  - [ ] OK    Fix: 

- **`reloadComplete`**
  - EN: Reload Complete Configuration
  - DE: Gesamtkonfiguration neu laden
  - [ ] OK    Fix: 

- **`reloadCompleteBackup`**
  - EN: Reload Complete Config. Backup
  - DE: Gesamtkonf. aus Backup laden
  - [ ] OK    Fix: 

- **`reloadSystem`**
  - EN: Reload System Configuration
  - DE: Systemkonfiguration neu laden
  - [ ] OK    Fix: 

- **`reloadSystemBackup`**
  - EN: Reload System Config. Backup
  - DE: Systemkonf. aus Backup laden
  - [ ] OK    Fix: 

- **`reportIssue`**
  - EN: Report Issue
  - DE: Problem melden
  - [ ] OK    Fix: 

- **`selectProjectFolder`**
  - EN: Select Project Folder
  - DE: Projektordner auswählen
  - [ ] OK    Fix: 

- **`setup`**
  - EN: Setup
  - DE: Einr.
  - [ ] OK    Fix: 

- **`soloModeMulti`**
  - EN: Solo: Multi
  - DE: Solo: Mehrfach
  - [ ] OK    Fix: 

- **`soloModeSingle`**
  - EN: Solo: Single
  - DE: Solo: Einzeln
  - [ ] OK    Fix: 

- **`storeComplete`**
  - EN: Store Complete Configuration
  - DE: Gesamtkonfiguration speichern
  - [ ] OK    Fix: 

- **`storeSystem`**
  - EN: Store System Configuration
  - DE: Systemkonfiguration speichern
  - [ ] OK    Fix: 

## `systemConfig.colorSchemes`

- **`black`**
  - EN: Black
  - DE: Schwarz
  - [ ] OK    Fix: 

- **`default`**
  - EN: Default (Dark Gray)
  - DE: Standard (Dunkelgrau)
  - [ ] OK    Fix: 

- **`light`**
  - EN: Light
  - DE: Hell
  - [ ] OK    Fix: 

## `systemConfig.controller`

- **`lightpad`**
  - EN: Lightpad
  - DE: Lightpad
  - [ ] OK    Fix: 

- **`off`**
  - EN: Off
  - DE: Aus
  - [ ] OK    Fix: 

- **`remote`**
  - EN: Remote
  - DE: Fernbedienung
  - [ ] OK    Fix: 

## `systemConfig.devices`

- **`off`**
  - EN: Off
  - DE: Aus
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
  - EN: Reducing from {current} to {new} input channels will remove settings for channels {start} to {end}.

This cannot be undone.
  - DE: Reduzierung von {current} auf {new} Eingangskanäle entfernt die Einstellungen für Kanäle {start} bis {end}.

Dies kann nicht rückgängig gemacht werden.
  - [ ] OK    Fix: 

- **`reduceInputChannels.title`**
  - EN: Reduce Input Channels?
  - DE: Eingangskanäle reduzieren?
  - [ ] OK    Fix: 

- **`reduceOutputChannels.message`**
  - EN: Reducing from {current} to {new} output channels will remove settings for channels {start} to {end}.

This cannot be undone.
  - DE: Reduzierung von {current} auf {new} Ausgangskanäle entfernt die Einstellungen für Kanäle {start} bis {end}.

Dies kann nicht rückgängig gemacht werden.
  - [ ] OK    Fix: 

- **`reduceOutputChannels.title`**
  - EN: Reduce Output Channels?
  - DE: Ausgangskanäle reduzieren?
  - [ ] OK    Fix: 

- **`reduceReverbChannels.message`**
  - EN: Reducing from {current} to {new} reverb channels will remove settings for channels {start} to {end}.

This cannot be undone.
  - DE: Reduzierung von {current} auf {new} Hallkanäle entfernt die Einstellungen für Kanäle {start} bis {end}.

Dies kann nicht rückgängig gemacht werden.
  - [ ] OK    Fix: 

- **`reduceReverbChannels.title`**
  - EN: Reduce Reverb Channels?
  - DE: Hallkanäle reduzieren?
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
  - EN: Select the hardware controller for dials and buttons: Stream Deck+ or XenceLabs Quick Keys.
  - DE: Wählen Sie den Hardware-Controller für Drehregler und Tasten: Stream Deck+ oder XenceLabs Quick Keys.
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
  - DE: Anzahl der Eingangskanäle.
  - [ ] OK    Fix: 

- **`language`**
  - EN: Select the user interface language. Changes take full effect after restarting the application.
  - DE: Wählen Sie die Sprache der Benutzeroberfläche. Änderungen werden nach Neustart der Anwendung vollständig wirksam.
  - [ ] OK    Fix: 

- **`lightpadSetup`**
  - EN: Display the connected Roli Lightpads and allow for splitting them in 4 smaller pads.
  - DE: Verbundene Roli Lightpads anzeigen und in 4 kleinere Pads aufteilen.
  - [ ] OK    Fix: 

- **`masterLevel`**
  - EN: Master Level (affects all outputs).
  - DE: Masterpegel (beeinflusst alle Ausgänge).
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
  - DE: Anzahl der Ausgangskanäle.
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
  - DE: Anzahl der Hallkanäle.
  - [ ] OK    Fix: 

- **`sampler`**
  - EN: Enable or disable the Sampler feature for input channels.
  - DE: Aktivieren oder deaktivieren Sie die Sampler-Funktion für Eingangskanäle. Wählen Sie den Controller: Lightpad oder Fernbedienung.
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
  - DE: Schallgeschwindigkeit (abhängig von der Temperatur).
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
  - DE: Temperatur (bestimmt die Schallgeschwindigkeit).
  - [ ] OK    Fix: 

## `systemConfig.labels`

- **`algorithm`**
  - EN: Algorithm:
  - DE: Algorithmus:
  - [ ] OK    Fix: 

- **`binauralAngle`**
  - EN: Listener Angle:
  - DE: Hörerwinkel:
  - [ ] OK    Fix: 

- **`binauralAtten`**
  - EN: Binaural Level:
  - DE: Binaural-Pegel:
  - [ ] OK    Fix: 

- **`binauralDelay`**
  - EN: Binaural Delay:
  - DE: Binaural-Verzögerung:
  - [ ] OK    Fix: 

- **`binauralDistance`**
  - EN: Listener Distance:
  - DE: Hörerabstand:
  - [ ] OK    Fix: 

- **`binauralOutput`**
  - EN: Binaural Output:
  - DE: Binaural-Ausgang:
  - [ ] OK    Fix: 

- **`clickToSplit`**
  - EN: Click to split
  - DE: Klicken zum Teilen
  - [ ] OK    Fix: 

- **`colorScheme`**
  - EN: Color Scheme:
  - DE: Farbschema:
  - [ ] OK    Fix: 

- **`dialsAndButtons`**
  - EN: Dials and Buttons:
  - DE: Drehregler und Tasten:
  - [ ] OK    Fix: 

- **`domeElevation`**
  - EN: Elevation:
  - DE: Kuppelwinkel:
  - [ ] OK    Fix: 

- **`haasEffect`**
  - EN: Haas Effect:
  - DE: Haas-Effekt:
  - [ ] OK    Fix: 

- **`inputChannels`**
  - EN: Input Channels:
  - DE: Eingangskanäle:
  - [ ] OK    Fix: 

- **`language`**
  - EN: Language:
  - DE: Sprache:
  - [ ] OK    Fix: 

- **`lightpadArrangement`**
  - EN: Lightpad Arrangement
  - DE: Lightpad-Anordnung
  - [ ] OK    Fix: 

- **`masterLevel`**
  - EN: Master Level:
  - DE: Masterpegel:
  - [ ] OK    Fix: 

- **`originDepth`**
  - EN: Origin Depth:
  - DE: Ursprung Tiefe:
  - [ ] OK    Fix: 

- **`originHeight`**
  - EN: Origin Height:
  - DE: Ursprung Höhe:
  - [ ] OK    Fix: 

- **`originWidth`**
  - EN: Origin Width:
  - DE: Ursprung Breite:
  - [ ] OK    Fix: 

- **`outputChannels`**
  - EN: Output Channels:
  - DE: Ausgangskanäle:
  - [ ] OK    Fix: 

- **`positionControl`**
  - EN: Position Control:
  - DE: Positionssteuerung:
  - [ ] OK    Fix: 

- **`quickLongPress`**
  - EN: Long Press:
  - DE: Langdruck:
  - [ ] OK    Fix: 

- **`reverbChannels`**
  - EN: Reverb Channels:
  - DE: Hallkanäle:
  - [ ] OK    Fix: 

- **`sampler`**
  - EN: Sampler:
  - DE: Sampler:
  - [ ] OK    Fix: 

- **`showLocation`**
  - EN: Location:
  - DE: Ort:
  - [ ] OK    Fix: 

- **`showName`**
  - EN: Name:
  - DE: Name:
  - [ ] OK    Fix: 

- **`speedOfSound`**
  - EN: Speed of Sound:
  - DE: Schallgeschwindigkeit:
  - [ ] OK    Fix: 

- **`split`**
  - EN: Split
  - DE: Teilen
  - [ ] OK    Fix: 

- **`stageDepth`**
  - EN: Depth:
  - DE: Tiefe:
  - [ ] OK    Fix: 

- **`stageDiameter`**
  - EN: Diameter:
  - DE: Durchmesser:
  - [ ] OK    Fix: 

- **`stageHeight`**
  - EN: Height:
  - DE: Höhe:
  - [ ] OK    Fix: 

- **`stageShape`**
  - EN: Stage Shape:
  - DE: Bühnenform:
  - [ ] OK    Fix: 

- **`stageWidth`**
  - EN: Width:
  - DE: Breite:
  - [ ] OK    Fix: 

- **`systemLatency`**
  - EN: System Latency:
  - DE: Systemlatenz:
  - [ ] OK    Fix: 

- **`temperature`**
  - EN: Temperature:
  - DE: Temperatur:
  - [ ] OK    Fix: 

- **`updateAvailable`**
  - EN: Update {version} available
  - DE: Update {version} verfügbar
  - [ ] OK    Fix: 

- **`version`**
  - EN: Version {version}
  - DE: Version {version}
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

- **`logsExported`**
  - EN: Logs exported to {path}
  - DE: Protokolle exportiert nach {path}
  - [ ] OK    Fix: 

- **`logsExportFailed`**
  - EN: Failed to export logs
  - DE: Protokollexport fehlgeschlagen
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

## `systemConfig.sections`

- **`binauralRenderer`**
  - EN: Binaural Renderer
  - DE: Binaural-Renderer
  - [ ] OK    Fix: 

- **`controllers`**
  - EN: Controllers
  - DE: Controllers
  - [ ] OK    Fix: 

- **`io`**
  - EN: I/O
  - DE: E/A
  - [ ] OK    Fix: 

- **`master`**
  - EN: Master Section
  - DE: Masterbereich
  - [ ] OK    Fix: 

- **`show`**
  - EN: Show
  - DE: Show
  - [ ] OK    Fix: 

- **`stage`**
  - EN: Stage
  - DE: Bühne
  - [ ] OK    Fix: 

- **`ui`**
  - EN: UI
  - DE: Oberfläche
  - [ ] OK    Fix: 

- **`wfsProcessor`**
  - EN: WFS Processor
  - DE: WFS-Prozessor
  - [ ] OK    Fix: 

## `systemConfig.stageShapes`

- **`box`**
  - EN: Box
  - DE: Quader
  - [ ] OK    Fix: 

- **`cylinder`**
  - EN: Cylinder
  - DE: Zylinder
  - [ ] OK    Fix: 

- **`dome`**
  - EN: Dome
  - DE: Kuppel
  - [ ] OK    Fix: 

## `tabs`

- **`clusters`**
  - EN: Clusters
  - DE: Cluster
  - [ ] OK    Fix: 

- **`inputs`**
  - EN: Inputs
  - DE: Eingänge
  - [ ] OK    Fix: 

- **`map`**
  - EN: Map
  - DE: Karte
  - [ ] OK    Fix: 

- **`network`**
  - EN: Network
  - DE: Netzwerk
  - [ ] OK    Fix: 

- **`outputs`**
  - EN: Outputs
  - DE: Ausgänge
  - [ ] OK    Fix: 

- **`reverb`**
  - EN: Reverb
  - DE: Hall
  - [ ] OK    Fix: 

- **`systemConfig`**
  - EN: System Config
  - DE: Systemkonfiguration
  - [ ] OK    Fix: 

## `touchscreens`

- **`button`**
  - EN: Setup
  - DE: Einrichten
  - [ ] OK    Fix: 

- **`label`**
  - EN: Touchscreen
  - DE: Touchscreen
  - [ ] OK    Fix: 

## `units`

- **`celsius`**
  - EN: °C
  - DE: °C
  - [ ] OK    Fix: 

- **`decibelPerMeter`**
  - EN: dB/m
  - DE: dB/m
  - [ ] OK    Fix: 

- **`decibels`**
  - EN: dB
  - DE: dB
  - [ ] OK    Fix: 

- **`degrees`**
  - EN: °
  - DE: °
  - [ ] OK    Fix: 

- **`meters`**
  - EN: m
  - DE: m
  - [ ] OK    Fix: 

- **`metersPerSecond`**
  - EN: m/s
  - DE: m/s
  - [ ] OK    Fix: 

- **`milliseconds`**
  - EN: ms
  - DE: ms
  - [ ] OK    Fix: 

## `wizard.buttons`

- **`back`**
  - EN: Back
  - DE: Zurück
  - [ ] OK    Fix: 

- **`close`**
  - EN: Close
  - DE: Schließen
  - [ ] OK    Fix: 

- **`done`**
  - EN: Done
  - DE: Fertig
  - [ ] OK    Fix: 

- **`gettingStarted`**
  - EN: Getting Started
  - DE: Erste Schritte
  - [ ] OK    Fix: 

- **`gettingStartedHelp`**
  - EN: Help cards guiding you through the first parameters to adjust when starting a new project
  - DE: Hilfekarten, die Sie durch die ersten Parameter führen, die beim Start eines neuen Projekts eingestellt werden müssen
  - [ ] OK    Fix: 

- **`next`**
  - EN: Next
  - DE: Weiter
  - [ ] OK    Fix: 

- **`skip`**
  - EN: Skip
  - DE: Überspringen
  - [ ] OK    Fix: 

## `wizard.steps`

- **`audioDevice.description`**
  - EN: Select your audio driver and device, set the sample rate and buffer size. Check the patch routing and test your outputs. Close this window when done.
  - DE: Wählen Sie Ihren Audiotreiber und Ihr Gerät, stellen Sie Abtastrate und Puffergröße ein. Überprüfen Sie das Routing und testen Sie Ihre Ausgänge. Schließen Sie dieses Fenster, wenn Sie fertig sind.
  - [ ] OK    Fix: 

- **`audioDevice.title`**
  - EN: Configure the Audio Interface
  - DE: Audio-Interface konfigurieren
  - [ ] OK    Fix: 

- **`audioInterface.description`**
  - EN: Click the button above or press Next to open the Audio Interface window.
  - DE: Klicken Sie auf die Schaltfläche oben oder drücken Sie Weiter, um das Audio-Interface-Fenster zu öffnen.
  - [ ] OK    Fix: 

- **`audioInterface.title`**
  - EN: Open the Audio Interface
  - DE: Audio-Interface öffnen
  - [ ] OK    Fix: 

- **`configureOutputs.description`**
  - EN: Use the array presets and geometry tools to calculate speaker positions for your arrays. Close this window when done.
  - DE: Verwenden Sie die Array-Presets und Geometrie-Tools, um Lautsprecherpositionen zu berechnen. Schließen Sie dieses Fenster, wenn Sie fertig sind.
  - [ ] OK    Fix: 

- **`configureOutputs.title`**
  - EN: Configure Output Positions
  - DE: Ausgangspositionen konfigurieren
  - [ ] OK    Fix: 

- **`exploreInputs.description`**
  - EN: Click an input on the map to select it, or lasso several to move them together. Drag to position your sources. Zoom with the mouse wheel or pinch gesture, pan with right-drag or two-finger drag. Add inputs, group them into clusters, and shape your sound field. You can also control positions with a keyboard, SpaceMouse, or other external controllers. Have fun!
  - DE: Klicken Sie auf einen Eingang auf der Karte, um ihn auszuwählen, oder verwenden Sie das Lasso für mehrere. Ziehen Sie, um Ihre Quellen zu positionieren. Zoomen Sie mit dem Mausrad oder Pinch-Geste, verschieben Sie die Ansicht mit Rechtsklick oder Zwei-Finger-Geste. Fügen Sie Eingänge hinzu, gruppieren Sie sie in Cluster und gestalten Sie Ihr Klangfeld. Sie können Positionen auch mit Tastatur, SpaceMouse oder anderen Controllern steuern. Viel Spaß!
  - [ ] OK    Fix: 

- **`exploreInputs.title`**
  - EN: Start Creating!
  - DE: Loslegen!
  - [ ] OK    Fix: 

- **`inputChannels.description`**
  - EN: How many audio sources will you be spatializing?
Set the number of input channels to match your source count.
  - DE: Wie viele Audioquellen werden Sie räumlich darstellen?
Stellen Sie die Anzahl der Eingangskanäle entsprechend ein.
  - [ ] OK    Fix: 

- **`inputChannels.title`**
  - EN: Set Input Channels
  - DE: Eingangskanäle festlegen
  - [ ] OK    Fix: 

- **`originPoint.description`**
  - EN: The origin is the reference point for all coordinates. Use the preset buttons or enter custom values. 'Front' places it at the audience edge.
  - DE: Der Ursprung ist der Referenzpunkt für alle Koordinaten. Verwenden Sie die Preset-Buttons oder geben Sie eigene Werte ein. 'Front' platziert ihn am Publikumsrand.
  - [ ] OK    Fix: 

- **`originPoint.title`**
  - EN: Set the Origin Point
  - DE: Ursprungspunkt festlegen
  - [ ] OK    Fix: 

- **`outputChannels.description`**
  - EN: Set the number of output channels to match your speaker array.
Each output corresponds to one physical speaker.
  - DE: Stellen Sie die Anzahl der Ausgangskanäle passend zu Ihrem Lautsprecher-Array ein.
Jeder Ausgang entspricht einem physischen Lautsprecher.
  - [ ] OK    Fix: 

- **`outputChannels.title`**
  - EN: Set Output Channels
  - DE: Ausgangskanäle festlegen
  - [ ] OK    Fix: 

- **`projectFolder.description`**
  - EN: Choose a folder to store your WFS project files. This will hold configurations, snapshots, IR files, and samples. Click the button to open the folder selector.
  - DE: Wählen Sie einen Ordner für Ihre WFS-Projektdateien. Hier werden Konfigurationen, Snapshots, IR-Dateien und Samples gespeichert. Klicken Sie auf die Schaltfläche, um den Ordner auszuwählen.
  - [ ] OK    Fix: 

- **`projectFolder.title`**
  - EN: Select a Project Folder
  - DE: Projektordner auswählen
  - [ ] OK    Fix: 

- **`reverbChannels.description`**
  - EN: Reverb channels add room simulation. Set to 0 if you don't need reverb.
  - DE: Hallkanäle fügen eine Raumsimulation hinzu. Setzen Sie 0, wenn Sie keinen Hall benötigen.
  - [ ] OK    Fix: 

- **`reverbChannels.title`**
  - EN: Set Reverb Channels
  - DE: Hallkanäle festlegen
  - [ ] OK    Fix: 

- **`stageConfig.description`**
  - EN: Set the shape and dimensions of your performance space. Choose box, cylinder, or dome, then enter the size in meters.
  - DE: Legen Sie Form und Abmessungen Ihres Aufführungsraums fest. Wählen Sie Box, Zylinder oder Kuppel und geben Sie die Maße in Metern ein.
  - [ ] OK    Fix: 

- **`stageConfig.title`**
  - EN: Define the Stage
  - DE: Bühne definieren
  - [ ] OK    Fix: 

- **`startProcessing.description`**
  - EN: You're all set! Long-press the Processing button to start the WFS engine. You can also start the Binaural Renderer for headphone monitoring.
  - DE: Alles bereit! Halten Sie den Processing-Button gedrückt, um die WFS-Engine zu starten. Sie können auch den Binaural-Renderer für Kopfhörer-Monitoring starten.
  - [ ] OK    Fix: 

- **`startProcessing.title`**
  - EN: Start the WFS Engine
  - DE: WFS-Engine starten
  - [ ] OK    Fix: 

- **`wizardOfOutZ.description`**
  - EN: Click the Wizard of OutZ button or press Next to open the output array helper.
  - DE: Klicken Sie auf den Wizard of OutZ Button oder drücken Sie Weiter, um den Positionierungsassistenten zu öffnen.
  - [ ] OK    Fix: 

- **`wizardOfOutZ.title`**
  - EN: Position Your Outputs
  - DE: Ausgänge positionieren
  - [ ] OK    Fix: 


