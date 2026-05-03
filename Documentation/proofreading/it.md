# Proofreading checklist — Italian (Italiano)

Locale: `it`  |  Total keys: 1520  |  Source: `Resources/lang/en.json` vs `Resources/lang/it.json`

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
  - IT: applicato
  - [ ] OK    Fix: 

- **`atHead`**
  - EN: (no undone records — at the head)
  - IT: (nessun record annullato — all'inizio)
  - [ ] OK    Fix: 

- **`batch`**
  - EN: batch {id}
  - IT: batch {id}
  - [ ] OK    Fix: 

- **`cursorLabel`**
  - EN: ◂  cursor (↑ applied  /  ↓ undone, redoable)
  - IT: ◂  cursore (↑ applicato  /  ↓ annullato, ripristinabile)
  - [ ] OK    Fix: 

- **`noChanges`**
  - EN: No AI changes yet.
  - IT: Nessuna modifica IA finora.
  - [ ] OK    Fix: 

- **`of`**
  - EN: of
  - IT: di
  - [ ] OK    Fix: 

- **`stepBack`**
  - EN: ⏮ Step Back
  - IT: ⏮ Indietro
  - [ ] OK    Fix: 

- **`stepForward`**
  - EN: Step Forward ⏭
  - IT: Avanti ⏭
  - [ ] OK    Fix: 

- **`undone`**
  - EN: undone
  - IT: annullato
  - [ ] OK    Fix: 

- **`windowTitle`**
  - EN: AI Change History
  - IT: Cronologia modifiche IA
  - [ ] OK    Fix: 

## `ai.server`

- **`copyUrlConfirm`**
  - EN: MCP URL copied to clipboard: {url}
  - IT: URL MCP copiato negli appunti: {url}
  - [ ] OK    Fix: 

- **`label`**
  - EN: MCP Server:
  - IT: Server MCP:
  - [ ] OK    Fix: 

- **`openHistory`**
  - EN: Open AI History
  - IT: Apri cronologia IA
  - [ ] OK    Fix: 

- **`urlButtonRunning`**
  - EN: http://{host}:{port}/mcp
  - IT: http://{host}:{port}/mcp
  - [ ] OK    Fix: 

- **`urlButtonStopped`**
  - EN: (server not running)
  - IT: (server non in esecuzione)
  - [ ] OK    Fix: 

## `ai.tier`

- **`aiOff`**
  - EN: AI: OFF
  - IT: IA: OFF
  - [ ] OK    Fix: 

- **`aiOn`**
  - EN: AI: ON
  - IT: IA: ON
  - [ ] OK    Fix: 

- **`gateAllowed`**
  - EN: AI critical actions: ALLOWED
  - IT: Azioni IA critiche: CONSENTITE
  - [ ] OK    Fix: 

- **`gateBlocked`**
  - EN: AI critical actions: blocked
  - IT: Azioni IA critiche: bloccate
  - [ ] OK    Fix: 

- **`tier2AutoOff`**
  - EN: Tier 2 auto-confirm: off
  - IT: Auto-conferma livello 2: off
  - [ ] OK    Fix: 

- **`tier2AutoOn`**
  - EN: Tier 2 auto-confirm: ON (5 min)
  - IT: Auto-conferma livello 2: ON (5 min)
  - [ ] OK    Fix: 

## `ai.toast`

- **`clientPrefix`**
  - EN: Claude
  - IT: Claude
  - [ ] OK    Fix: 

- **`header`**
  - EN: AI changes
  - IT: Modifiche IA
  - [ ] OK    Fix: 

- **`moreOlder`**
  - EN: …and {count} older
  - IT: …e {count} più vecchie
  - [ ] OK    Fix: 

## `ai.tooltips`

- **`aiToggle`**
  - EN: Master switch for the whole MCP integration. When OFF every AI tool call is refused; when ON, normal tier handling applies (the critical-actions toggle still controls destructive calls separately).
  - IT: Interruttore principale per l'intera integrazione MCP. In OFF, ogni chiamata di strumento IA viene rifiutata; in ON, si applica la normale gestione per tier (l'interruttore delle azioni critiche controlla separatamente le chiamate distruttive).
  - [ ] OK    Fix: 

- **`gate`**
  - EN: Allow destructive AI actions (channel-count changes, port reconfigure, runDSP, etc.) AND skip the per-call confirm handshake for less-destructive tier-2 actions while open. Acts as a superset of the Tier 2 auto-confirm toggle. The red fill drains over 5 minutes, then auto-blocks again.
  - IT: Permetti azioni IA distruttive (modifiche al numero di canali, riconfigurazione delle porte, runDSP, ecc.) E salta la conferma per chiamata per azioni di tier-2 meno distruttive mentre è aperto. Agisce come superinsieme dell'interruttore di auto-conferma di Tier 2. Il riempimento rosso si svuota in 5 minuti, poi si blocca automaticamente.
  - [ ] OK    Fix: 

- **`openHistory`**
  - EN: Open the AI History window: scrollable timeline of every recent AI change with per-row undo / redo and a step-by-step cursor.
  - IT: Apri la finestra Cronologia IA: cronologia scorrevole di ogni modifica IA recente con annulla/ripristina per riga e cursore passo-passo.
  - [ ] OK    Fix: 

- **`urlButton`**
  - EN: Click to copy the MCP server URL. Useful for Claude Code (claude mcp add wfs-diy <URL> -t http) or any MCP client that takes a URL. Claude Desktop instead uses the JSON config snippet — open the (?) help card.
  - IT: Cliccare per copiare l'URL del server MCP. Utile per Claude Code (claude mcp add wfs-diy <URL> -t http) o qualsiasi client MCP che accetti un URL. Claude Desktop usa invece il frammento di configurazione JSON — aprire la scheda di aiuto (?).
  - [ ] OK    Fix: 

## `ai.undo`

- **`errorPrefix`**
  - EN: AI {verb}: {message}
  - IT: IA {verb}: {message}
  - [ ] OK    Fix: 

- **`successPrefix`**
  - EN: AI {verb}: {description}
  - IT: IA {verb}: {description}
  - [ ] OK    Fix: 

- **`verbRedo`**
  - EN: redo
  - IT: ripristino
  - [ ] OK    Fix: 

- **`verbUndo`**
  - EN: undo
  - IT: annullamento
  - [ ] OK    Fix: 

## `arrayHelper.acoustic`

- **`distanceAtten`**
  - EN: Distance Atten (%):
  - IT: Atten Distanza (%):
  - [ ] OK    Fix: 

- **`floorReflections`**
  - EN: Floor Reflections
  - IT: Riflessioni Pavimento
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: HF Damping (dB/m):
  - IT: Smorzamento HF (dB/m):
  - [ ] OK    Fix: 

- **`highCut`**
  - EN: High Cut (Hz):
  - IT: Taglia Alti (Hz):
  - [ ] OK    Fix: 

- **`hParallax`**
  - EN: H Parallax (m):
  - IT: Parallasse H (m):
  - [ ] OK    Fix: 

- **`liveSource`**
  - EN: Live Source
  - IT: Live Source
  - [ ] OK    Fix: 

- **`lowCut`**
  - EN: Low Cut (Hz):
  - IT: Taglia Bassi (Hz):
  - [ ] OK    Fix: 

- **`vParallax`**
  - EN: V Parallax (m):
  - IT: Parallasse V (m):
  - [ ] OK    Fix: 

## `arrayHelper.buttons`

- **`apply`**
  - EN: Apply
  - IT: Applica
  - [ ] OK    Fix: 

- **`close`**
  - EN: Close
  - IT: Chiudi
  - [ ] OK    Fix: 

## `arrayHelper.errors`

- **`noPositions`**
  - EN: No positions to apply. Check geometry parameters.
  - IT: Nessuna posizione da applicare. Verifica i parametri geometrici.
  - [ ] OK    Fix: 

- **`notEnoughOutputs`**
  - EN: Not enough output channels! Need {count} starting from {start}
  - IT: Canali di uscita insufficienti! Necessari {count} a partire da {start}
  - [ ] OK    Fix: 

- **`prefix`**
  - EN: Error: 
  - IT: Errore: 
  - [ ] OK    Fix: 

- **`speakerCountZero`**
  - EN: Number of speakers must be greater than 0
  - IT: Il numero di altoparlanti deve essere maggiore di 0
  - [ ] OK    Fix: 

## `arrayHelper.geometry`

- **`backFacing`**
  - EN: Back Facing
  - IT: Rivolto Indietro
  - [ ] OK    Fix: 

- **`centerSpacing`**
  - EN: Center + Spacing
  - IT: Centro + Spaziatura
  - [ ] OK    Fix: 

- **`centerX`**
  - EN: Center X (m):
  - IT: Centro X (m):
  - [ ] OK    Fix: 

- **`centerY`**
  - EN: Center Y (m):
  - IT: Centro Y (m):
  - [ ] OK    Fix: 

- **`endpoints`**
  - EN: Endpoints
  - IT: Punti Estremi
  - [ ] OK    Fix: 

- **`endX`**
  - EN: End X (m):
  - IT: Fine X (m):
  - [ ] OK    Fix: 

- **`endY`**
  - EN: End Y (m):
  - IT: Fine Y (m):
  - [ ] OK    Fix: 

- **`facingInward`**
  - EN: Facing Inward
  - IT: Rivolto all'Interno
  - [ ] OK    Fix: 

- **`facingOutward`**
  - EN: Facing Outward
  - IT: Rivolto all'Esterno
  - [ ] OK    Fix: 

- **`frontFacing`**
  - EN: Front Facing
  - IT: Rivolto in Avanti
  - [ ] OK    Fix: 

- **`nPairs`**
  - EN: N Pairs:
  - IT: N Coppie:
  - [ ] OK    Fix: 

- **`nSpeakers`**
  - EN: N Speakers:
  - IT: N Altoparlanti:
  - [ ] OK    Fix: 

- **`orientation`**
  - EN: Orientation (deg):
  - IT: Orientamento (gradi):
  - [ ] OK    Fix: 

- **`radius`**
  - EN: Radius (m):
  - IT: Raggio (m):
  - [ ] OK    Fix: 

- **`sag`**
  - EN: Sag (m):
  - IT: Freccia (m):
  - [ ] OK    Fix: 

- **`spacing`**
  - EN: Spacing (m):
  - IT: Spaziatura (m):
  - [ ] OK    Fix: 

- **`startAngle`**
  - EN: Start Angle (deg):
  - IT: Angolo Iniziale (gradi):
  - [ ] OK    Fix: 

- **`startX`**
  - EN: Start X (m):
  - IT: Inizio X (m):
  - [ ] OK    Fix: 

- **`startY`**
  - EN: Start Y (m):
  - IT: Inizio Y (m):
  - [ ] OK    Fix: 

- **`width`**
  - EN: Width (m):
  - IT: Larghezza (m):
  - [ ] OK    Fix: 

- **`yEnd`**
  - EN: Y End (m):
  - IT: Y Fine (m):
  - [ ] OK    Fix: 

- **`yStart`**
  - EN: Y Start (m):
  - IT: Y Inizio (m):
  - [ ] OK    Fix: 

- **`zHeight`**
  - EN: Z Height (m):
  - IT: Altezza Z (m):
  - [ ] OK    Fix: 

## `arrayHelper.presets`

- **`circle`**
  - EN: Circle
  - IT: Cerchio
  - [ ] OK    Fix: 

- **`delayLine`**
  - EN: Delay Line
  - IT: Linea di Ritardo
  - [ ] OK    Fix: 

- **`label`**
  - EN: Preset:
  - IT: Preset:
  - [ ] OK    Fix: 

- **`mainRoomStraight`**
  - EN: Main Flown Array Straight
  - IT: Array Principale Sospeso Dritto
  - [ ] OK    Fix: 

- **`nearFieldCurved`**
  - EN: Near Field Array Curved
  - IT: Array Near Field Curvo
  - [ ] OK    Fix: 

- **`nearFieldStraight`**
  - EN: Near Field Array Straight
  - IT: Array Near Field Dritto
  - [ ] OK    Fix: 

- **`subBass`**
  - EN: Sub Bass
  - IT: Sub Bass
  - [ ] OK    Fix: 

- **`surround`**
  - EN: Surround
  - IT: Surround
  - [ ] OK    Fix: 

## `arrayHelper.preview`

- **`audience`**
  - EN: Audience
  - IT: Pubblico
  - [ ] OK    Fix: 

## `arrayHelper.sections`

- **`acousticDefaults`**
  - EN: Acoustic Defaults
  - IT: Predefiniti Acustici
  - [ ] OK    Fix: 

- **`geometry`**
  - EN: Geometry
  - IT: Geometria
  - [ ] OK    Fix: 

- **`target`**
  - EN: Target
  - IT: Destinazione
  - [ ] OK    Fix: 

## `arrayHelper.status`

- **`applied`**
  - EN: Applied {count} speakers to Array {array}. Ready for next array.
  - IT: Applicati {count} altoparlanti all'Array {array}. Pronto per il prossimo array.
  - [ ] OK    Fix: 

- **`calculated`**
  - EN: Calculated {count} positions
  - IT: Calcolate {count} posizioni
  - [ ] OK    Fix: 

- **`ready`**
  - EN: Ready
  - IT: Pronto
  - [ ] OK    Fix: 

## `arrayHelper.target`

- **`array`**
  - EN: Array:
  - IT: Array:
  - [ ] OK    Fix: 

- **`arrayPrefix`**
  - EN: Array
  - IT: Array
  - [ ] OK    Fix: 

- **`startingOutput`**
  - EN: Starting Output:
  - IT: Uscita Iniziale:
  - [ ] OK    Fix: 

## `arrayHelper.window`

- **`contentName`**
  - EN: Output Array Helper
  - IT: Assistente Array Uscite
  - [ ] OK    Fix: 

- **`title`**
  - EN: Wizard of OutZ
  - IT: Procedura OutZ
  - [ ] OK    Fix: 

## `audioPatch`

- **`windowTitle`**
  - EN: Audio Interface and Patching
  - IT: Interfaccia Audio e Routing
  - [ ] OK    Fix: 

## `audioPatch.buttons`

- **`hold`**
  - EN: Hold
  - IT: Mantieni
  - [ ] OK    Fix: 

- **`unpatchAll`**
  - EN: Unpatch All
  - IT: Rimuovi Tutti i Patch
  - [ ] OK    Fix: 

## `audioPatch.deviceSettings`

- **`buttons.controlPanel`**
  - EN: Control Panel
  - IT: Pannello di Controllo
  - [ ] OK    Fix: 

- **`buttons.resetDevice`**
  - EN: Reset Device
  - IT: Resetta Dispositivo
  - [ ] OK    Fix: 

- **`labels.bufferSize`**
  - EN: Audio buffer size:
  - IT: Dimensione buffer audio:
  - [ ] OK    Fix: 

- **`labels.device`**
  - EN: Device:
  - IT: Dispositivo:
  - [ ] OK    Fix: 

- **`labels.deviceType`**
  - EN: Audio device type:
  - IT: Tipo dispositivo audio:
  - [ ] OK    Fix: 

- **`labels.sampleRate`**
  - EN: Sample rate:
  - IT: Frequenza di campionamento:
  - [ ] OK    Fix: 

- **`noDevice`**
  - EN: No Device
  - IT: Nessun Dispositivo
  - [ ] OK    Fix: 

- **`notConfigured`**
  - EN: Not configured
  - IT: Non configurato
  - [ ] OK    Fix: 

## `audioPatch.labels`

- **`interfaceInput`**
  - EN: Audio Interface Input
  - IT: Ingresso interfaccia audio
  - [ ] OK    Fix: 

- **`interfaceOutput`**
  - EN: Audio Interface Output
  - IT: Uscita interfaccia audio
  - [ ] OK    Fix: 

- **`processorInputs`**
  - EN: Processor Inputs
  - IT: Ingressi processore
  - [ ] OK    Fix: 

- **`processorOutputs`**
  - EN: Processor Outputs
  - IT: Uscite processore
  - [ ] OK    Fix: 

## `audioPatch.messages`

- **`chooseTestSignal`**
  - EN: Choose a Test Signal to Enable Testing
  - IT: Scegli un Segnale di Test per Abilitare il Test
  - [ ] OK    Fix: 

## `audioPatch.modes`

- **`patching`**
  - EN: Patching
  - IT: Patching
  - [ ] OK    Fix: 

- **`scrolling`**
  - EN: Scrolling
  - IT: Scorrimento
  - [ ] OK    Fix: 

- **`testing`**
  - EN: Testing
  - IT: Test
  - [ ] OK    Fix: 

## `audioPatch.tabs`

- **`deviceSettings`**
  - EN: Device Settings
  - IT: Impostazioni Dispositivo
  - [ ] OK    Fix: 

- **`inputPatch`**
  - EN: Input Patch
  - IT: Patch Ingressi
  - [ ] OK    Fix: 

- **`outputPatch`**
  - EN: Output Patch
  - IT: Patch Uscite
  - [ ] OK    Fix: 

## `audioPatch.testSignal`

- **`labels.frequency`**
  - EN: Frequency:
  - IT: Frequenza:
  - [ ] OK    Fix: 

- **`labels.level`**
  - EN: Level:
  - IT: Livello:
  - [ ] OK    Fix: 

- **`labels.signal`**
  - EN: Signal:
  - IT: Segnale:
  - [ ] OK    Fix: 

- **`types.diracPulse`**
  - EN: Dirac Pulse
  - IT: Impulso Dirac
  - [ ] OK    Fix: 

- **`types.off`**
  - EN: Off
  - IT: Off
  - [ ] OK    Fix: 

- **`types.pinkNoise`**
  - EN: Pink Noise
  - IT: Rumore Rosa
  - [ ] OK    Fix: 

- **`types.pulse`**
  - EN: Pulse
  - IT: Impulso
  - [ ] OK    Fix: 

- **`types.sweep`**
  - EN: Sweep
  - IT: Sweep
  - [ ] OK    Fix: 

- **`types.tone`**
  - EN: Tone
  - IT: Tono
  - [ ] OK    Fix: 

## `clusters.help`

- **`attenuationSlider`**
  - EN: Adjust attenuation of all cluster inputs (dB).
  - IT: Regolare l'attenuazione di tutti gli ingressi del cluster (dB).
  - [ ] OK    Fix: 

- **`exportPresets`**
  - EN: Export all 16 LFO presets to an XML file.
  - IT: Esporta tutti i 16 preset LFO in un file XML.
  - [ ] OK    Fix: 

- **`importPresets`**
  - EN: Import LFO presets from an XML file.
  - IT: Importa preset LFO da un file XML.
  - [ ] OK    Fix: 

- **`inputsVisibility`**
  - EN: Show or hide this cluster's inputs on the Map tab. Hide propagates to new members; removing an input restores its visibility.
  - IT: Mostra o nascondi gli ingressi di questo cluster sulla Mappa. Nascondere si propaga ai nuovi membri; rimuovere un ingresso ripristina la sua visibilità.
  - [ ] OK    Fix: 

- **`lfoActiveButton`**
  - EN: Enable or Disable the Periodic Movement of the Cluster (LFO).
  - IT: Attivare o disattivare il movimento periodico del cluster (LFO).
  - [ ] OK    Fix: 

- **`lfoAmplitudeRotSlider`**
  - EN: Maximum Rotation Angle (-360 to 360 Degrees).
  - IT: Angolo di rotazione massimo (-360 a 360 gradi).
  - [ ] OK    Fix: 

- **`lfoAmplitudeScaleSlider`**
  - EN: Maximum Scale Factor (0.1x to 10x, Logarithmic).
  - IT: Fattore di scala massimo (0,1× a 10×, logaritmico).
  - [ ] OK    Fix: 

- **`lfoAmplitudeXSlider`**
  - EN: Width of Movement in Relation to Cluster Reference Position.
  - IT: Larghezza del movimento rispetto alla posizione di riferimento del cluster.
  - [ ] OK    Fix: 

- **`lfoAmplitudeYSlider`**
  - EN: Depth of Movement in Relation to Cluster Reference Position.
  - IT: Profondità del movimento rispetto alla posizione di riferimento del cluster.
  - [ ] OK    Fix: 

- **`lfoAmplitudeZSlider`**
  - EN: Height of Movement in Relation to Cluster Reference Position.
  - IT: Altezza del movimento rispetto alla posizione di riferimento del cluster.
  - [ ] OK    Fix: 

- **`lfoPeriodDial`**
  - EN: Base Period of the Cluster Movement.
  - IT: Periodo di base del movimento del cluster.
  - [ ] OK    Fix: 

- **`lfoPhaseDial`**
  - EN: Global Phase Offset of the Cluster Movement.
  - IT: Sfasamento globale del movimento del cluster.
  - [ ] OK    Fix: 

- **`lfoPhaseRotDial`**
  - EN: Phase Offset of the Cluster Rotation.
  - IT: Sfasamento della rotazione del cluster.
  - [ ] OK    Fix: 

- **`lfoPhaseScaleDial`**
  - EN: Phase Offset of the Cluster Scaling.
  - IT: Sfasamento del ridimensionamento del cluster.
  - [ ] OK    Fix: 

- **`lfoPhaseXDial`**
  - EN: Phase Offset of the Cluster Movement in Width.
  - IT: Sfasamento del movimento del cluster in larghezza.
  - [ ] OK    Fix: 

- **`lfoPhaseYDial`**
  - EN: Phase Offset of the Cluster Movement in Depth.
  - IT: Sfasamento del movimento del cluster in profondità.
  - [ ] OK    Fix: 

- **`lfoPhaseZDial`**
  - EN: Phase Offset of the Cluster Movement in Height.
  - IT: Sfasamento del movimento del cluster in altezza.
  - [ ] OK    Fix: 

- **`lfoRateRotSlider`**
  - EN: Faster or Slower Rotation in Relation to Base Period.
  - IT: Rotazione più rapida o più lenta rispetto al periodo di base.
  - [ ] OK    Fix: 

- **`lfoRateScaleSlider`**
  - EN: Faster or Slower Scaling in Relation to Base Period.
  - IT: Ridimensionamento più rapido o più lento rispetto al periodo di base.
  - [ ] OK    Fix: 

- **`lfoRateXSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Width.
  - IT: Movimento più rapido o più lento rispetto al periodo di base, in larghezza.
  - [ ] OK    Fix: 

- **`lfoRateYSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Depth.
  - IT: Movimento più rapido o più lento rispetto al periodo di base, in profondità.
  - [ ] OK    Fix: 

- **`lfoRateZSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Height.
  - IT: Movimento più rapido o più lento rispetto al periodo di base, in altezza.
  - [ ] OK    Fix: 

- **`lfoShapeRotSelector`**
  - EN: Rotation Behaviour of the Cluster.
  - IT: Comportamento di rotazione del cluster.
  - [ ] OK    Fix: 

- **`lfoShapeScaleSelector`**
  - EN: Scale Behaviour of the Cluster.
  - IT: Comportamento di scala del cluster.
  - [ ] OK    Fix: 

- **`lfoShapeXSelector`**
  - EN: Movement Behaviour of the Cluster in Width.
  - IT: Comportamento del movimento del cluster in larghezza.
  - [ ] OK    Fix: 

- **`lfoShapeYSelector`**
  - EN: Movement Behaviour of the Cluster in Depth.
  - IT: Comportamento del movimento del cluster in profondità.
  - [ ] OK    Fix: 

- **`lfoShapeZSelector`**
  - EN: Movement Behaviour of the Cluster in Height.
  - IT: Comportamento del movimento del cluster in altezza.
  - [ ] OK    Fix: 

- **`planeSelector`**
  - EN: Select the plane for rotation and scale operations (XY, XZ, YZ).
  - IT: Selezionare il piano per le operazioni di rotazione e scala (XY, XZ, YZ).
  - [ ] OK    Fix: 

- **`positionJoystick`**
  - EN: Move all cluster inputs in X/Y. Hold and drag to translate.
  - IT: Spostare tutti gli ingressi del cluster in X/Y. Tenere premuto e trascinare per traslare.
  - [ ] OK    Fix: 

- **`presetTile`**
  - EN: Click: recall preset. Double-click: recall + start. Middle/right-click: store current LFO.
  - IT: Clic: richiama preset. Doppio clic: richiama + avvia. Clic centrale/destro: memorizza l'LFO corrente.
  - [ ] OK    Fix: 

- **`qlabPreset`**
  - EN: Create a QLab network cue to recall the last selected LFO preset for the current cluster.
  - IT: Crea una cue di rete QLab per richiamare l'ultimo preset LFO selezionato per il cluster corrente.
  - [ ] OK    Fix: 

- **`referenceMode`**
  - EN: Select the reference point for cluster transforms: First Input or Barycenter.
  - IT: Selezionare il punto di riferimento per le trasformazioni del cluster: Primo Ingresso o Baricentro.
  - [ ] OK    Fix: 

- **`rotationDial`**
  - EN: Rotate all cluster inputs around the reference point in the selected plane.
  - IT: Ruotare tutti gli ingressi del cluster attorno al punto di riferimento nel piano selezionato.
  - [ ] OK    Fix: 

- **`scaleJoystick`**
  - EN: Scale cluster inputs relative to the reference point in the selected plane.
  - IT: Scalare gli ingressi del cluster rispetto al punto di riferimento nel piano selezionato.
  - [ ] OK    Fix: 

- **`stopAllLFO`**
  - EN: Stop LFO on all 10 clusters.
  - IT: Ferma l'LFO su tutti e 10 i cluster.
  - [ ] OK    Fix: 

- **`zSlider`**
  - EN: Move all cluster inputs along the Z axis (height).
  - IT: Spostare tutti gli ingressi del cluster lungo l'asse Z (altezza).
  - [ ] OK    Fix: 

## `clusters.labels`

- **`assignedInputs`**
  - EN: Assigned Inputs
  - IT: Ingressi Assegnati
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Atten
  - IT: Atten
  - [ ] OK    Fix: 

- **`controls`**
  - EN: Controls
  - IT: Controlli
  - [ ] OK    Fix: 

- **`inputPrefix`**
  - EN: Input
  - IT: Ingresso
  - [ ] OK    Fix: 

- **`position`**
  - EN: Position
  - IT: Posizione
  - [ ] OK    Fix: 

- **`posPrefix`**
  - EN: Pos:
  - IT: Pos:
  - [ ] OK    Fix: 

- **`reference`**
  - EN: Reference:
  - IT: Riferimento:
  - [ ] OK    Fix: 

- **`rotation`**
  - EN: Rotation
  - IT: Rotazione
  - [ ] OK    Fix: 

- **`scale`**
  - EN: Scale
  - IT: Scala
  - [ ] OK    Fix: 

- **`x`**
  - EN: X:
  - IT: X:
  - [ ] OK    Fix: 

- **`y`**
  - EN: Y:
  - IT: Y:
  - [ ] OK    Fix: 

- **`z`**
  - EN: Z
  - IT: Z
  - [ ] OK    Fix: 

## `clusters.lfo`

- **`labels.amplitude`**
  - EN: Amplitude:
  - IT: Ampiezza:
  - [ ] OK    Fix: 

- **`labels.angle`**
  - EN: Angle:
  - IT: Angolo:
  - [ ] OK    Fix: 

- **`labels.period`**
  - EN: Period:
  - IT: Periodo:
  - [ ] OK    Fix: 

- **`labels.phase`**
  - EN: Phase:
  - IT: Fase:
  - [ ] OK    Fix: 

- **`labels.rate`**
  - EN: Rate:
  - IT: Velocità:
  - [ ] OK    Fix: 

- **`labels.ratio`**
  - EN: Ratio:
  - IT: Rapporto:
  - [ ] OK    Fix: 

- **`labels.rot`**
  - EN: Rotation
  - IT: Rotazione
  - [ ] OK    Fix: 

- **`labels.scaleLfo`**
  - EN: Scale
  - IT: Scala
  - [ ] OK    Fix: 

- **`labels.section`**
  - EN: LFO
  - IT: LFO
  - [ ] OK    Fix: 

- **`labels.x`**
  - EN: X
  - IT: X
  - [ ] OK    Fix: 

- **`labels.y`**
  - EN: Y
  - IT: Y
  - [ ] OK    Fix: 

- **`labels.z`**
  - EN: Z
  - IT: Z
  - [ ] OK    Fix: 

## `clusters.osc`

- **`lfoActive`**
  - EN: /wfs/cluster/lfoActive <id> <0|1>
  - IT: /wfs/cluster/lfoActive <id> <0|1>
  - [ ] OK    Fix: 

- **`lfoAmplitudeRot`**
  - EN: /wfs/cluster/lfoAmplitudeRot <id> <degrees>
  - IT: /wfs/cluster/lfoAmplitudeRot <id> <gradi>
  - [ ] OK    Fix: 

- **`lfoAmplitudeScale`**
  - EN: /wfs/cluster/lfoAmplitudeScale <id> <factor>
  - IT: /wfs/cluster/lfoAmplitudeScale <id> <fattore>
  - [ ] OK    Fix: 

- **`lfoAmplitudeX`**
  - EN: /wfs/cluster/lfoAmplitudeX <id> <meters>
  - IT: /wfs/cluster/lfoAmplitudeX <id> <metri>
  - [ ] OK    Fix: 

- **`lfoAmplitudeY`**
  - EN: /wfs/cluster/lfoAmplitudeY <id> <meters>
  - IT: /wfs/cluster/lfoAmplitudeY <id> <metri>
  - [ ] OK    Fix: 

- **`lfoAmplitudeZ`**
  - EN: /wfs/cluster/lfoAmplitudeZ <id> <meters>
  - IT: /wfs/cluster/lfoAmplitudeZ <id> <metri>
  - [ ] OK    Fix: 

- **`lfoPeriod`**
  - EN: /wfs/cluster/lfoPeriod <id> <seconds>
  - IT: /wfs/cluster/lfoPeriod <id> <secondi>
  - [ ] OK    Fix: 

- **`lfoPhase`**
  - EN: /wfs/cluster/lfoPhase <id> <degrees>
  - IT: /wfs/cluster/lfoPhase <id> <gradi>
  - [ ] OK    Fix: 

- **`lfoPhaseRot`**
  - EN: /wfs/cluster/lfoPhaseRot <id> <degrees>
  - IT: /wfs/cluster/lfoPhaseRot <id> <gradi>
  - [ ] OK    Fix: 

- **`lfoPhaseScale`**
  - EN: /wfs/cluster/lfoPhaseScale <id> <degrees>
  - IT: /wfs/cluster/lfoPhaseScale <id> <gradi>
  - [ ] OK    Fix: 

- **`lfoPhaseX`**
  - EN: /wfs/cluster/lfoPhaseX <id> <degrees>
  - IT: /wfs/cluster/lfoPhaseX <id> <gradi>
  - [ ] OK    Fix: 

- **`lfoPhaseY`**
  - EN: /wfs/cluster/lfoPhaseY <id> <degrees>
  - IT: /wfs/cluster/lfoPhaseY <id> <gradi>
  - [ ] OK    Fix: 

- **`lfoPhaseZ`**
  - EN: /wfs/cluster/lfoPhaseZ <id> <degrees>
  - IT: /wfs/cluster/lfoPhaseZ <id> <gradi>
  - [ ] OK    Fix: 

- **`lfoPresetRecall`**
  - EN: /wfs/cluster/lfoPresetRecall <clusterId> <presetNumber>
  - IT: /wfs/cluster/lfoPresetRecall <clusterId> <numeroPreset>
  - [ ] OK    Fix: 

- **`lfoRateRot`**
  - EN: /wfs/cluster/lfoRateRot <id> <multiplier>
  - IT: /wfs/cluster/lfoRateRot <id> <moltiplicatore>
  - [ ] OK    Fix: 

- **`lfoRateScale`**
  - EN: /wfs/cluster/lfoRateScale <id> <multiplier>
  - IT: /wfs/cluster/lfoRateScale <id> <moltiplicatore>
  - [ ] OK    Fix: 

- **`lfoRateX`**
  - EN: /wfs/cluster/lfoRateX <id> <multiplier>
  - IT: /wfs/cluster/lfoRateX <id> <moltiplicatore>
  - [ ] OK    Fix: 

- **`lfoRateY`**
  - EN: /wfs/cluster/lfoRateY <id> <multiplier>
  - IT: /wfs/cluster/lfoRateY <id> <moltiplicatore>
  - [ ] OK    Fix: 

- **`lfoRateZ`**
  - EN: /wfs/cluster/lfoRateZ <id> <multiplier>
  - IT: /wfs/cluster/lfoRateZ <id> <moltiplicatore>
  - [ ] OK    Fix: 

- **`lfoShapeRot`**
  - EN: /wfs/cluster/lfoShapeRot <id> <0-8>
  - IT: /wfs/cluster/lfoShapeRot <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoShapeScale`**
  - EN: /wfs/cluster/lfoShapeScale <id> <0-8>
  - IT: /wfs/cluster/lfoShapeScale <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoShapeX`**
  - EN: /wfs/cluster/lfoShapeX <id> <0-8>
  - IT: /wfs/cluster/lfoShapeX <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoShapeY`**
  - EN: /wfs/cluster/lfoShapeY <id> <0-8>
  - IT: /wfs/cluster/lfoShapeY <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoShapeZ`**
  - EN: /wfs/cluster/lfoShapeZ <id> <0-8>
  - IT: /wfs/cluster/lfoShapeZ <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoStopAll`**
  - EN: /wfs/cluster/lfoStopAll
  - IT: /wfs/cluster/lfoStopAll
  - [ ] OK    Fix: 

## `clusters.planes`

- **`xy`**
  - EN: XY
  - IT: XY
  - [ ] OK    Fix: 

- **`xz`**
  - EN: XZ
  - IT: XZ
  - [ ] OK    Fix: 

- **`yz`**
  - EN: YZ
  - IT: YZ
  - [ ] OK    Fix: 

## `clusters.presets`

- **`export`**
  - EN: Export
  - IT: Esporta
  - [ ] OK    Fix: 

- **`exportDialog`**
  - EN: Export LFO Presets
  - IT: Esporta preset LFO
  - [ ] OK    Fix: 

- **`exported`**
  - EN: LFO presets exported.
  - IT: Preset LFO esportati.
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - IT: Importa
  - [ ] OK    Fix: 

- **`importDialog`**
  - EN: Import LFO Presets
  - IT: Importa preset LFO
  - [ ] OK    Fix: 

- **`imported`**
  - EN: LFO presets imported.
  - IT: Preset LFO importati.
  - [ ] OK    Fix: 

- **`recalled`**
  - EN: LFO preset recalled from tile {n}.
  - IT: Preset LFO richiamato dalla casella {n}.
  - [ ] OK    Fix: 

- **`stopAll`**
  - EN: Stop All
  - IT: Ferma tutto
  - [ ] OK    Fix: 

- **`stored`**
  - EN: LFO preset stored in tile {n}.
  - IT: Preset LFO memorizzato nella casella {n}.
  - [ ] OK    Fix: 

## `clusters.referenceMode`

- **`barycenter`**
  - EN: Barycenter
  - IT: Baricentro
  - [ ] OK    Fix: 

- **`firstInput`**
  - EN: First Input
  - IT: Primo Ingresso
  - [ ] OK    Fix: 

## `clusters.status`

- **`noInputs`**
  - EN: No inputs assigned
  - IT: Nessun ingresso assegnato
  - [ ] OK    Fix: 

- **`trackedMarker`**
  - EN: [T]
  - IT: [T]
  - [ ] OK    Fix: 

- **`tracking`**
  - EN: Tracking: Input {num} (overrides reference)
  - IT: Tracking: Ingresso {num} (sovrascrive riferimento)
  - [ ] OK    Fix: 

## `clusters.toggles`

- **`inputsHidden`**
  - EN: Inputs: Hidden
  - IT: Ingressi: Nascosti
  - [ ] OK    Fix: 

- **`inputsVisible`**
  - EN: Inputs: Visible
  - IT: Ingressi: Visibili
  - [ ] OK    Fix: 

- **`lfoOff`**
  - EN: L.F.O: OFF
  - IT: L.F.O: OFF
  - [ ] OK    Fix: 

- **`lfoOn`**
  - EN: L.F.O: ON
  - IT: L.F.O: ON
  - [ ] OK    Fix: 

## `common`

- **`cancel`**
  - EN: Cancel
  - IT: Annulla
  - [ ] OK    Fix: 

- **`off`**
  - EN: OFF
  - IT: OFF
  - [ ] OK    Fix: 

- **`ok`**
  - EN: OK
  - IT: OK
  - [ ] OK    Fix: 

- **`on`**
  - EN: ON
  - IT: ON
  - [ ] OK    Fix: 

## `eq.buttons`

- **`flattenEQ`**
  - EN: Flatten EQ
  - IT: Resetta EQ
  - [ ] OK    Fix: 

- **`resetBand`**
  - EN: Reset
  - IT: Reset
  - [ ] OK    Fix: 

## `eq.filterTypes`

- **`allPass`**
  - EN: All-Pass
  - IT: Passa-tutto
  - [ ] OK    Fix: 

- **`bandPass`**
  - EN: Band Pass
  - IT: Passa Banda
  - [ ] OK    Fix: 

- **`highCut`**
  - EN: High Cut
  - IT: Taglia Alti
  - [ ] OK    Fix: 

- **`highShelf`**
  - EN: High Shelf
  - IT: Shelf Alti
  - [ ] OK    Fix: 

- **`lowCut`**
  - EN: Low Cut
  - IT: Taglia Bassi
  - [ ] OK    Fix: 

- **`lowShelf`**
  - EN: Low Shelf
  - IT: Shelf Bassi
  - [ ] OK    Fix: 

- **`peakNotch`**
  - EN: Peak/Notch
  - IT: Peak/Notch
  - [ ] OK    Fix: 

## `eq.labels`

- **`band`**
  - EN: Band
  - IT: Banda
  - [ ] OK    Fix: 

- **`freq`**
  - EN: Freq:
  - IT: Freq:
  - [ ] OK    Fix: 

- **`gain`**
  - EN: Gain
  - IT: Gain
  - [ ] OK    Fix: 

- **`q`**
  - EN: Q
  - IT: Q
  - [ ] OK    Fix: 

## `eq.status`

- **`off`**
  - EN: EQ OFF
  - IT: EQ OFF
  - [ ] OK    Fix: 

- **`on`**
  - EN: EQ ON
  - IT: EQ ON
  - [ ] OK    Fix: 

## `fileManager.dialogs`

- **`selectProjectFolder`**
  - EN: Select Project Folder
  - IT: Seleziona cartella di progetto
  - [ ] OK    Fix: 

## `fileManager.errors`

- **`backupNotFound`**
  - EN: Backup not found
  - IT: Backup non trovato
  - [ ] OK    Fix: 

- **`configStateInvalid`**
  - EN: Config state is invalid
  - IT: Stato di configurazione non valido
  - [ ] OK    Fix: 

- **`failedApply`**
  - EN: Failed to apply: {sections}
  - IT: Applicazione fallita: {sections}
  - [ ] OK    Fix: 

- **`failedCreateFolder`**
  - EN: Failed to create project folder: {path}
  - IT: Creazione della cartella di progetto fallita: {path}
  - [ ] OK    Fix: 

- **`failedCreateValueTree`**
  - EN: Failed to create ValueTree from XML: {path}
  - IT: Creazione del ValueTree da XML fallita: {path}
  - [ ] OK    Fix: 

- **`failedCreateXML`**
  - EN: Failed to create XML from state
  - IT: Creazione dell'XML dallo stato fallita
  - [ ] OK    Fix: 

- **`failedParseXML`**
  - EN: Failed to parse XML file: {path}
  - IT: Analisi del file XML fallita: {path}
  - [ ] OK    Fix: 

- **`failedWriteFile`**
  - EN: Failed to write file: {path}
  - IT: Scrittura del file fallita: {path}
  - [ ] OK    Fix: 

- **`fileNotFound`**
  - EN: File not found: {path}
  - IT: File non trovato: {path}
  - [ ] OK    Fix: 

- **`invalidConfigStructure`**
  - EN: Invalid configuration file structure
  - IT: Struttura del file di configurazione non valida
  - [ ] OK    Fix: 

- **`noInputDataInFile`**
  - EN: No input data found in file
  - IT: Nessun dato di ingresso trovato nel file
  - [ ] OK    Fix: 

- **`noInputDataInSnapshot`**
  - EN: No input data in snapshot
  - IT: Nessun dato di ingresso nello snapshot
  - [ ] OK    Fix: 

- **`noLFOPresetDataInFile`**
  - EN: No LFO preset data found in file
  - IT: Nessun dato di preset LFO trovato nel file
  - [ ] OK    Fix: 

- **`noNetworkDataInFile`**
  - EN: No network data found in file
  - IT: Nessun dato di rete trovato nel file
  - [ ] OK    Fix: 

- **`noNetworkSections`**
  - EN: No network sections found in file
  - IT: Nessuna sezione di rete trovata nel file
  - [ ] OK    Fix: 

- **`noOutputDataInFile`**
  - EN: No output data found in file
  - IT: Nessun dato di uscita trovato nel file
  - [ ] OK    Fix: 

- **`noProjectFolder`**
  - EN: No project folder specified
  - IT: Nessuna cartella di progetto specificata
  - [ ] OK    Fix: 

- **`noReverbDataInFile`**
  - EN: No reverb data found in file
  - IT: Nessun dato di riverbero trovato nel file
  - [ ] OK    Fix: 

- **`noSystemDataInFile`**
  - EN: No valid system data found in file: {path}
  - IT: Nessun dato di sistema valido trovato nel file: {path}
  - [ ] OK    Fix: 

- **`noValidProjectFolder`**
  - EN: No valid project folder
  - IT: Nessuna cartella di progetto valida
  - [ ] OK    Fix: 

- **`prefixInputs`**
  - EN: Inputs: 
  - IT: Ingressi: 
  - [ ] OK    Fix: 

- **`prefixNetwork`**
  - EN: Network: 
  - IT: Rete: 
  - [ ] OK    Fix: 

- **`prefixOutputs`**
  - EN: Outputs: 
  - IT: Uscite: 
  - [ ] OK    Fix: 

- **`prefixReverbs`**
  - EN: Reverbs: 
  - IT: Riverberi: 
  - [ ] OK    Fix: 

- **`prefixSystem`**
  - EN: System: 
  - IT: Sistema: 
  - [ ] OK    Fix: 

- **`snapshotNotFound`**
  - EN: Snapshot not found
  - IT: Snapshot non trovato
  - [ ] OK    Fix: 

- **`snapshotNotFoundNamed`**
  - EN: Snapshot not found: {name}
  - IT: Snapshot non trovato: {name}
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
  - IT: ADM-OSC è un protocollo che mira a migliorare l'interoperabilità del suono spaziale. Invia posizioni cartesiane (X, Y, Z) o valori polari (AED per Azimut, Elevazione, Distanza) dalla console o dalle curve di automazione di una DAW.
I dati vengono inviati normalizzati:
- tra -1.0 e 1.0 per X, Y e Z;
- tra 0.0 e 1.0 per la distanza,
- tra -180° e 180° per l'azimut
- tra -90° e 90° per l'elevazione.
Il punto di origine può essere spostato e la mappatura può essere regolata in segmenti diversi per le parti interne ed esterne del palcoscenico.
Trascinando le maniglie sui grafici, tenendo premuto il tasto Maiusc si applicano regolazioni simmetriche sul lato opposto.
  - [ ] OK    Fix: 

- **`title`**
  - EN: ADM-OSC Mappings
  - IT: Mappature ADM-OSC
  - [ ] OK    Fix: 

## `help.automOtion`

- **`body`**
  - EN: One time movement can also be programmed and triggered manually or triggered by sound level.
The coordinates are either relative from the start position or absolute relative to the origin point.
The input can either stay at the end position or revert to the starting position.
Input position cannot be changed while the input is moving this way, but interacting with the input will change the position offset of the input instead.
For audio level triggering select the sound level above which the movement will start. When the sound drops below the reset level the movement will be rearmed, ready to be triggered again. New movement automation will not be possible while the input is moving.
  - IT: Movimenti singoli possono essere programmati e attivati manualmente o dal livello sonoro.
Le coordinate sono relative dalla posizione iniziale o assolute rispetto al punto di origine.
L'ingresso può rimanere alla posizione finale o tornare alla posizione iniziale.
La posizione non può essere modificata durante il movimento, ma l'interazione cambierà l'offset di posizione.
Per l'attivazione per livello audio, selezionare la soglia. Quando il suono scende sotto il livello di ripristino, il movimento verrà riarmato.
  - [ ] OK    Fix: 

- **`title`**
  - EN: AutomOtion
  - IT: AutomOtion
  - [ ] OK    Fix: 

## `help.binaural`

- **`body`**
  - EN: The Binaural renderer is used for:
- listening to a rough spatial mix on headphones,
- creating a mix for stereo output,
- listening to a single soloed track through the spatial processing.
This may take the place of your master mix if it's only feeding headphones and media mix.
The position of the listening position may be adjusted in depth from the origin point and in orientation. Delay and level settings allow you to eventually match the sound at the FOH position.
  - IT: Il renderer binaurale è utilizzato per:
- ascoltare un mix spaziale approssimativo in cuffia,
- creare un mix per uscita stereo,
- ascoltare una traccia solista attraverso l'elaborazione spaziale.
Può sostituire il mix master se alimenta solo cuffie e mix media.
La posizione di ascolto può essere regolata in profondità dal punto di origine e in orientamento. Le impostazioni di ritardo e livello permettono di allineare il suono alla posizione FOH.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Binaural Renderer
  - IT: Renderer Binaurale
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
  - IT: I cluster sono gruppi di ingressi che possono essere manipolati e animati come un insieme.
Ogni ingresso può far parte di un solo cluster.
Ogni cluster può avere un solo ingresso con tracking completamente abilitato, che diventa il punto di riferimento.
Se nessun ingresso con tracking è presente, ci sono due modi: il primo ingresso assegnato o il baricentro degli ingressi assegnati.
Tutti gli ingressi possono essere spostati trascinando il punto di riferimento. Gli ingressi individuali possono essere regolati separatamente. Trascinare un ingresso con tracking attivato che è anche punto di riferimento influenzerà il suo offset di posizione e la posizione degli altri ingressi del cluster normalmente.
Tutti gli ingressi di un cluster possono essere ruotati o scalati attorno al punto di riferimento.
Tutti i cluster possono ricevere un'animazione via LFO. Le posizioni X, Y, Z, la rotazione e la scala possono essere controllate. Le impostazioni LFO possono essere assegnate ai pad. Un clic destro memorizzerà i parametri LFO in un pad. Doppio clic sulla parte superiore del pad permette di modificare il nome del preset. Un clic o tap richiama le impostazioni che il LFO sia attivo o meno, ma non lo avvierà. Un doppio clic/tap caricherà e avvierà il LFO.
Tutti i cluster condividono lo stesso set di preset LFO.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Clusters
  - IT: Cluster
  - [ ] OK    Fix: 

## `help.floorReflections`

- **`body`**
  - EN: Simulating the floor reflections improves the naturalness of the sound. We don't expect sounds to play in a soundproof anechoic chamber. This setting helps recreate the expected floor reflections.
The level of the floor reflections can be adjusted as well as the low cut and the high frequency shelf filters. Diffusion adds a bit of chaos to simulate the unevenness of the floor.
  - IT: Simulare le riflessioni del pavimento migliora la naturalezza del suono. Non ci aspettiamo che i suoni vengano riprodotti in una camera anecoica insonorizzata. Questa impostazione aiuta a ricreare le riflessioni del pavimento attese.
Il livello delle riflessioni del pavimento può essere regolato così come i filtri taglia basso e shelf alte frequenze. La diffusione aggiunge un po' di caos per simulare le irregolarità del pavimento.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Floor Reflections
  - IT: Riflessioni del Pavimento
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
  - IT: Le mappe gradiente permettono di controllare attenuazione, altezza e filtraggio alte frequenze (shelf con pendenza morbida centrata a 1kHz) in funzione della posizione X, Y. Ad esempio, è possibile attenuare un suono entrando in una zona, applicare roll-off delle alte frequenze allontanandosi dal fronte del palco o regolare automaticamente l'altezza di un attore su piattaforme rialzate.
Ci sono tre livelli: attenuazione, altezza e shelf HF. Possono essere attivati/disattivati e nascosti.
Ogni livello ha controlli di mappatura bianco e nero per regolare il range dell'effetto. L'impostazione della curva regola la transizione.
Ogni livello può avere forme modificabili (rettangolo, ellisse o poligono) con grigio uniforme, gradiente lineare o radiale.
Per creare un poligono, cliccare per ogni angolo. Doppio clic chiude la forma.
Doppio clic su un punto lo rimuove. Doppio clic su un lato aggiunge un punto.
Scala e rotazione possono essere modificate dal centro o dall'origine.
Forme e livelli possono essere copiati.
Le impostazioni sono salvate nei file di ingresso.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Gradient Maps
  - IT: Mappe Gradiente
  - [ ] OK    Fix: 

## `help.inputAdvanced`

- **`body`**
  - EN: - Sidelines and Fringe enable muting when an input comes close to the limits of a rectangular stage (except downstage).
- Tracking can be enabled and the ID of the tracker can be selected. The amount of smoothing on the position can also be adjusted.
- Maximum Speed can be engaged and the speed limit adjusted. The system will apply a gradual acceleration and deceleration when the movement starts and finishes. When Path is enabled, the system will track the path followed by the input and will not go in a straight line to the final position. This is especially handy if movements have to be operated manually.
- Height Factor lets you work in 2D, when set to 0%, or full 3D, when set at 100%, and anything in between. It's the ratio of the height in the level and delay calculations. If you wish to use floor reflections, set it to 100% and use the parallax correction of the output parameters.
  - IT: - Le linee laterali e la zona di transizione consentono il mute quando un ingresso si avvicina ai bordi di un palco rettangolare (eccetto il bordo anteriore lato pubblico).
- Il Tracking può essere attivato e l'ID del tracker selezionato. Anche lo smussamento della posizione può essere regolato.
- La Velocità Massima può essere attivata e il limite di velocità regolato. Il sistema applicherà accelerazione e decelerazione graduali all'inizio e alla fine del movimento. Quando la modalità Percorso è attiva, il sistema seguirà il tracciato preso dall'ingresso e non andrà in linea retta verso la posizione finale. È particolarmente utile se i movimenti devono essere operati manualmente.
- Il Fattore di Altezza permette di lavorare in 2D, quando impostato a 0%, o in 3D completo, quando impostato a 100%, e tutto in mezzo. È il rapporto dell'altezza nei calcoli di livello e ritardo. Se desidera utilizzare le riflessioni a pavimento, impostarlo al 100% e usare la correzione di parallasse nei parametri di uscita.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Advanced Controls
  - IT: Controlli avanzati
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
  - IT: Gli ingressi dispongono di un'ampia varietà di impostazioni per adattarsi a diverse situazioni che richiedono sonorizzazione realistica o strumenti creativi per il sound design.
- Il livello d'ingresso può essere regolato.
- Gli ingressi possono essere ritardati o possono tentare di tenere conto di una latenza specifica (elaborazione digitale di trasmissione wireless o effetti digitali) e compensarla per allineare meglio l'amplificazione e il suono acustico.
- La Latenza Minima può essere attivata al posto dell'Acoustic Precedence. Questo cerca di far uscire il suono attraverso il sistema il più velocemente possibile. Il sistema scansiona gli invii di questo ingresso alle uscite per il ritardo più basso e lo sottrae da tutti i ritardi, bypassando l'effetto Haas aggiuntivo.
- La posizione (posizione e offset) può essere data in coordinate Cartesiane, Cilindriche o Sferiche indipendentemente dalla forma del palco o dagli altri canali.
- La posizione può essere vincolata alle dimensioni del palco in coordinate Cartesiane o a un intervallo di distanza specifico in coordinate polari.
- Flip prenderà la posizione simmetrica per la coordinata data attorno al punto di origine.
- Il joystick e il cursore verticale permettono il controllo relativo della posizione.
- Gli ingressi possono essere assegnati a un cluster per raggrupparli in movimenti coordinati.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Inputs Basic Parameters
  - IT: Parametri di base degli ingressi
  - [ ] OK    Fix: 

## `help.inputHF`

- **`body`**
  - EN: When talking away, the timbre of a voice will sound less bright. Reproducing this was the initial goal here even though we usually want to have support for voices when not addressing the audience or in bi-frontal configurations. This can be put to good use for creative effects such as having a brighter reverb on a dampened direct sound.
The orientation of the input in azimuth and in pitch can be set as well as the angle where the high frequencies will not be filtered.
The HF Shelf will set the maximum attenuation in the back of the input. There is a smooth fade (like a cosine curve) from full brightness in front to damped at the rear.
  - IT: Quando si parla girandosi, il timbro di una voce suona meno brillante. Riprodurre questo era l'obiettivo iniziale qui, anche se generalmente si desidera avere supporto per le voci quando non si rivolgono al pubblico o in configurazioni bi-frontali. Questo può essere usato per effetti creativi come avere un riverbero più brillante su un suono diretto attenuato.
L'orientamento dell'ingresso in azimut e in elevazione può essere impostato così come l'angolo dove le alte frequenze non saranno filtrate.
L'HF Shelf imposterà l'attenuazione massima sul retro dell'ingresso. C'è una dissolvenza graduale (come una curva coseno) dalla piena brillantezza davanti all'attenuazione dietro.
  - [ ] OK    Fix: 

- **`title`**
  - EN: High Frequency Directivity
  - IT: Direttività alta frequenza
  - [ ] OK    Fix: 

## `help.inputLevel`

- **`body`**
  - EN: There are two level attenuation models. One where the level decreases with the distance by a given ratio in dB/m. Otherwise the level is halved each time the distance doubles. This might be more realistic, but it may be too loud close to the source or not give enough focus. The former may be less physically accurate, it offers usually better control to have a more even and stable mix.
  - IT: Esistono due modelli di attenuazione del livello. Uno in cui il livello diminuisce con la distanza secondo un rapporto dato in dB/m. Altrimenti il livello si dimezza ogni volta che la distanza raddoppia. Quest'ultimo può essere più realistico, ma può essere troppo forte vicino alla sorgente o non dare abbastanza focalizzazione. Il primo può essere meno accurato fisicamente, ma offre generalmente un migliore controllo per un mix più uniforme e stabile.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Level Adjustments
  - IT: Regolazioni di livello
  - [ ] OK    Fix: 

## `help.inputMutes`

- **`body`**
  - EN: You can set for each output array a specific attenuation for the selected input.
You can mute each send to any output individually. There are macros to speed up the process.
  - IT: Puoi impostare per ogni array di uscita un'attenuazione specifica per l'ingresso selezionato.
Puoi silenziare ogni invio a qualsiasi uscita individualmente. Sono disponibili macro per velocizzare il processo.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Array Attenuation and Output Mutes
  - IT: Attenuazione per array e mute di uscita
  - [ ] OK    Fix: 

## `help.lfo`

- **`body`**
  - EN: Input position can be automated. The LFO can control X, Y and Z position coordinates individually as well as the rotation of HF directivity (gyrophone).
Adjust the global period and phase for the LFO.
For X, Y and Z coordinates select a shape, amplitude, rate and phase as desired. A circle in the XY plane would have sine for shape for X and for Y and an offset of ±90° between the two. A square would be the same but with keystone shapes.
Input position can be moved while the LFO is running.
  - IT: La posizione dell'ingresso può essere automatizzata. Il LFO può controllare le coordinate X, Y e Z individualmente e la rotazione della direttività HF (girofono).
Regolare il periodo e la fase globali del LFO.
Per X, Y e Z selezionare forma, ampiezza, tasso e fase. Un cerchio nel piano XY userebbe forma sinusoidale per X e Y con ±90° di sfasamento. Un quadrato sarebbe uguale ma con forme trapezoidali.
La posizione può essere spostata mentre il LFO è attivo.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Input LFO
  - IT: LFO Ingresso
  - [ ] OK    Fix: 

## `help.liveSource`

- **`body`**
  - EN: A loud source on stage may not need much reinforcement through the nearby speakers. Imagine an opera singer near the edge of the stage. Normally the level distribution would make the level louder near the input position. But if it is already loud enough we should be able to not over-amplify it. This function manages this.
The radius and shape describe how to attenuate the level for a specific source for speakers within the radius of influence of this input. There are various shapes: a V shaped linear effect; a U shape for fast decrease within the radius; a tight V effect or a mix of the previous (sine).
The attenuation can be constant or level dependent like a local compression reacting to transients and to average RMS level.
  - IT: Una sorgente potente sul palco potrebbe non aver bisogno di rinforzo attraverso gli altoparlanti vicini. Immaginate un cantante d'opera vicino al bordo del palco. Normalmente la distribuzione del livello renderebbe il suono più forte vicino alla posizione dell'ingresso. Ma se è già abbastanza forte, non dovremmo sovra-amplificarlo. Questa funzione gestisce questo.
Il raggio e la forma descrivono come attenuare il livello per gli altoparlanti nel raggio d'influenza di questa sorgente. Ci sono varie forme: un effetto lineare a V; una U per diminuzione rapida; una V stretta o un mix dei precedenti (seno).
L'attenuazione può essere costante o dipendente dal livello, come una compressione locale che reagisce ai transienti e al livello RMS medio.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Live Source Tamer
  - IT: Attenuatore Sorgente Live
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
  - IT: - Clic sinistro su un ingresso o cluster per spostarlo trascinando.
- Clic sinistro con Maiusc aggiunge o rimuove ingressi dalla selezione.
- Clic sinistro trascinato disegna un rettangolo di selezione.
- Doppio clic ripristina l'offset di posizione.
- Clic lungo senza movimento passa alla scheda dell'ingresso selezionato.
- Clic fuori da qualsiasi ingresso cancella la selezione.
- Clic destro trascinato sposta la vista della mappa. Trascinamento a due dita anche.
- La rotella del mouse fa zoom. Pizzico a due dita anche.
- Clic centrale ripristina la vista.
- I tasti freccia spostano X/Y, PagSu/Giù l'altezza.
- Un secondo dito può ruotare la direttività e regolare l'altezza.
- Nei cluster, un secondo dito può ruotare e scalare.
- Gli ingressi, array di uscita e nodi di riverbero possono essere nascosti.
- Gli ingressi possono essere bloccati.
- I nodi di riverbero possono essere spostati. Ctrl/Cmd sposta le coppie in simmetria.
- Il raggio del Live Source Tamer viene visualizzato quando attivato.
- I livelli audio possono essere visualizzati sulla mappa.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Map
  - IT: Mappa
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
  - IT: Il server MCP consente a un assistente IA (Claude Desktop, Claude Code, ChatGPT con connettori personalizzati) di leggere e scrivere i parametri di questa sessione WFS-DIY tramite una connessione di rete locale.

Cosa può fare l'IA:
• Leggere lo stato in diretta: numero di canali, nomi, posizioni, attenuazioni, EQ, snapshot, cluster, l'intera superficie dei parametri.
• Spostare sorgenti, rinominare canali, impostare assegnazioni di cluster, regolare il layout degli array, posizionare uscite e riverberi.
• Eseguire flussi di lavoro guidati (assistenti di accordatura del sistema, risoluzione di problemi di localizzazione, gestione degli snapshot) tramite modelli di prompt preparati.

Controlli operatore in questa riga:
• IA: ON / OFF — interruttore principale. In OFF, ogni chiamata IA viene rifiutata; in ON, l'IA opera secondo le regole sottostanti.
• Azioni IA critiche: bloccate / CONSENTITE — le azioni distruttive (eliminare snapshot, ripristinare DSP, modificare il numero di canali) sono bloccate per impostazione predefinita. Cliccare per consentirle per 10 minuti; il riempimento rosso si svuota allo scadere della finestra, poi si blocca automaticamente.
• Apri Cronologia IA — cronologia scorrevole di ogni modifica IA recente con annulla/ripristina per riga.
• Il pulsante URL MCP copia l'URL del server negli appunti per i client IA che accettano un URL direttamente.

Consapevolezza dell'operatore:
• Ogni azione IA viene registrata con tag di origine. La finestra Cronologia IA mostra l'intera cronologia; la × per riga inverte un'azione con le sue dipendenze.
• Se regola manualmente un parametro che l'IA ha appena spostato, l'IA viene avvisata e non riproverà ciecamente. Ha sempre l'ultima parola.
• Le scorciatoie Cmd/Ctrl+Alt+Z e Cmd/Ctrl+Alt+Y annullano e ripristinano l'ultima modifica IA senza influire sulle modifiche manuali (che usano Ctrl+Z normale).

Per aggiungere questo server a Claude Desktop:
  1. Aprire Impostazioni → Sviluppatore → Modifica configurazione.
  2. Incollare il frammento JSON sottostante in claude_desktop_config.json (unire al blocco mcpServers esistente se ne ha già uno).
  3. Riavviare Claude Desktop. Il server appare come 'wfs-diy' nel menu strumenti.

Per aggiungere a Claude Code, eseguire:
  claude mcp add wfs-diy <url> -t http

L'URL cambia se si cambia interfaccia di rete o se il server fa fallback su una porta diversa. Il pulsante URL in questa riga riflette sempre l'URL in diretta.
  - [ ] OK    Fix: 

- **`copyButton`**
  - EN: Copy Config
  - IT: Copia configurazione
  - [ ] OK    Fix: 

- **`copyConfirm`**
  - EN: MCP config JSON copied to clipboard
  - IT: Configurazione MCP JSON copiata negli appunti
  - [ ] OK    Fix: 

- **`title`**
  - EN: AI / MCP Server
  - IT: Server IA / MCP
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
  - IT: Il sistema può comunicare attraverso diversi protocolli di rete (UDP o TCP) usando OSC. OSC Query può essere abilitato per permettere ai client di scoprire i percorsi OSC possibili e sottoscrivere aggiornamenti dei parametri.
Viene mostrato l'IP della macchina locale corrispondente all'interfaccia di rete selezionata. Le porte TCP e UDP in entrata e la porta OSC Query.
Ci sono alcuni client OSC specializzati come:
- Remote per l'applicazione Android per operazioni multitouch e controllo remoto.
- QLab che può inviare dati e può essere programmato direttamente dall'applicazione.
- ADM-OSC per il controllo da console e DAW (vedere l'aiuto specifico).
I dati possono essere filtrati. Una finestra di Log mostra i dati in entrata e in uscita.
C'è anche una funzione di localizzazione per trovare tablet Android smarriti.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Network
  - IT: Rete
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
  - IT: Ci sono alcuni parametri per aiutarvi ad adattarvi al suono acustico.
La maggior parte di questi parametri è impostata per interi array a meno che la modalità di propagazione sia disattivata per questa uscita nell'array. Può anche essere selezionata una modifica relativa dopo un'impostazione specifica.
- Orientamento e Angoli On/Off definiscono quali ingressi ogni altoparlante amplificherà. Di default gli altoparlanti puntano al pubblico, lontano dal palco. Gli ingressi nel settore verde saranno amplificati, ma non quelli davanti all'altoparlante, nel settore rosso. C'è una dissolvenza tra i due settori. Per gli altoparlanti sub-bass, aprire completamente al massimo permetterà di avere tutti gli ingressi potenzialmente captati dai subwoofer.
- L'Attenuazione HF simula la perdita di alte frequenze con la distanza.
- La percentuale di Attenuazione per Distanza definisce quanta dell'attenuazione calcolata viene applicata. Per i Sub-bass può essere saggio abbassare al 50%.
- La Latenza Minima attiva la scansione del ritardo calcolato più piccolo.
- L'Attenuazione Live Source attiva la riduzione di livello degli ingressi vicini.
- Le Riflessioni del Pavimento attivano se le riflessioni vengono applicate al segnale per questa uscita come sub-bass e array sospesi...
  - [ ] OK    Fix: 

- **`title`**
  - EN: Advanced Parameters
  - IT: Parametri avanzati
  - [ ] OK    Fix: 

## `help.outputs`

- **`body`**
  - EN: The design of the WFS system has to do with the right choice of equipment and their position. Here is a guide to help you with the design and the tuning of your arrays.
An array is a line (straight or curved) of speakers. This is one of the most important concepts in WFS adapted to sound reinforcement and for creative sound design.
A rule of thumb would be that each listener should hear three speakers of an array to have enough psycho-acoustic cues to feel the direction each sound is coming from. There will be a sweet spot to find between the distance between the speakers and the listeners, their spacing and coverage angle. This is especially true for short range arrays, a.k.a front fills. Speakers with a 120° coverage angle can be spaced by the same distance between the array and the first row of listeners. The number of speakers also depends on the sound pressure level. When placed as a flown array, trapezoidal/asymmetric horns with wide coverage angle (120°) below axis and narrow coverage (60°) on axis will give good coverage for seat rows a bit too far from the ground array and will have enough throw to reach 20m or 30m away while avoiding the walls of the venue where they would create reflections that give away the position of the speakers to the ears of the listeners. Most of the time for larger venues coaxial (elliptical or conical horns) have not enough reach and require one or more delay lines. They are more suited for smaller venues with few rows of seats.
The positioning of the speakers in the system can be done through the 'Wizard of OutZ' and the editable presets it has.
  - IT: Il design del sistema WFS riguarda la giusta scelta dell'attrezzatura e il suo posizionamento. Ecco una guida per aiutarvi con il design e l'accordatura dei vostri array.
Un array è una linea (dritta o curva) di altoparlanti. Questo è uno dei concetti più importanti in WFS adattato alla sonorizzazione e al design sonoro creativo.
Come regola generale, ogni ascoltatore dovrebbe sentire tre altoparlanti di un array per avere sufficienti indizi psicoacustici per percepire la direzione di ogni suono. Ci sarà un punto ottimale tra la distanza tra gli altoparlanti e gli ascoltatori, la loro spaziatura e l'angolo di copertura. Altoparlanti con angolo di copertura di 120° possono essere spaziati della stessa distanza tra l'array e la prima fila. Il numero dipende anche dal livello di pressione sonora. Come array sospeso, trombe trapezoidali/asimmetriche con ampio angolo di copertura (120°) sotto asse e stretto (60°) in asse daranno buona copertura e portata di 20-30m evitando riflessioni sulle pareti. Gli altoparlanti coassiali generalmente non hanno abbastanza portata per grandi ambienti e richiedono linee di ritardo.
Il posizionamento può essere fatto tramite il 'Wizard of OutZ' e i suoi preset modificabili.
  - [ ] OK    Fix: 

- **`title`**
  - EN: WFS Array Design
  - IT: Design degli array WFS
  - [ ] OK    Fix: 

## `help.overview`

- **`body`**
  - EN: This WFS spatial sound processor aims to be a tool for natural sound reinforcement and also a creative tool opening new avenues for writing sound in space.
Some parameters are straightforward: place sound (Map, Tracking, Speed Limiting, Gradient Maps...), work its shape (Attenuation Profile) and its acoustic presence (Directivity, Floor Reflections), give it a one-off motion (AutomOtion) or a repetitive movement (L.F.O). In some cases amplification should be limited around loud sources on stage (Live Source Tamer). All of these functionalities can be stored and recalled internally or with the help of QLab. On the other side the system allows real-time interaction to trigger and move samples, move large clusters of inputs manually or thanks to easily recallable LFO presets.
  - IT: Questo processore spaziale WFS vuole essere uno strumento per la sonorizzazione naturale e anche uno strumento creativo che apre nuove strade per scrivere il suono nello spazio.
Alcuni parametri sono semplici: posizionare il suono (Mappa, Tracking, Limitazione velocità, Mappe gradiente...), lavorare la sua forma (Profilo di attenuazione) e la sua presenza acustica (Direttività, Riflessioni del pavimento), dargli un movimento singolo (AutomOtion) o ripetitivo (L.F.O). In alcuni casi l'amplificazione dovrebbe essere limitata attorno a sorgenti potenti sul palco (Live Source Tamer). Tutte queste funzionalità possono essere memorizzate e richiamate internamente o con l'aiuto di QLab. D'altra parte il sistema permette l'interazione in tempo reale per attivare e spostare campioni, spostare grandi cluster di ingressi manualmente o grazie a preset LFO facilmente richiamabili.
  - [ ] OK    Fix: 

- **`dontShow`**
  - EN: Don't show this again
  - IT: Non mostrare più
  - [ ] OK    Fix: 

- **`title`**
  - EN: System Overview
  - IT: Panoramica del sistema
  - [ ] OK    Fix: 

## `help.parallax`

- **`body`**
  - EN: Each speaker points more or less clearly at a listener. To calculate the delay for an input for each speaker, let's consider the distance from the input to this listener, we can also calculate the distance the sound from the speaker to this listener. To match the time of arrival of both we need to apply the difference in the aforementioned distances as a delay for this input and this speaker. This gives greater stability when inputs are moved on stage and especially when they move upstage, away from the edge of the stage. This also can enable the synthesis of floor reflections. This setting can be tuned for finer adjustments, rather than simply measured. Use your ears!
  - IT: Ogni altoparlante punta più o meno chiaramente verso un ascoltatore. Per calcolare il ritardo per un ingresso per ogni altoparlante, consideriamo la distanza dall'ingresso a questo ascoltatore, possiamo anche calcolare la distanza del suono dall'altoparlante a questo ascoltatore. Per far coincidere il tempo di arrivo di entrambi dobbiamo applicare la differenza delle distanze menzionate come ritardo. Questo dà maggiore stabilità quando gli ingressi vengono spostati sul palco e specialmente quando si allontanano dal bordo del palco. Questo può anche permettere la sintesi delle riflessioni del pavimento. Questa impostazione può essere regolata finemente, piuttosto che semplicemente misurata. Fidatevi delle vostre orecchie!
  - [ ] OK    Fix: 

- **`title`**
  - EN: Parallax Correction
  - IT: Correzione di parallasse
  - [ ] OK    Fix: 

## `help.reverb`

- **`body`**
  - EN: Reverb helps blur the actual reflections of the speakers in the acoustic space. Without some reverb the listener might perceive the sound missing some depth because the reflections of the speakers in the acoustic of the room gives away their positions and all sounds might feel as coming from the plane of the speakers.
Place the reverb node according to the number of channels and the geometry of the stage and listening area, and the positions of speakers. If necessary the return position can be offset from the feed position.
Other parameters are very similar to Outputs' and Inputs'.
  - IT: Il riverbero aiuta a sfumare le riflessioni degli altoparlanti.
Posizionare i nodi secondo la geometria del palco.
Altri parametri sono simili a quelli di Uscite e Ingressi.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb
  - IT: Riverbero
  - [ ] OK    Fix: 

## `help.reverbAlgo`

- **`body`**
  - EN: There are three built-in reverb types inside this spatial sound processor:
- SDN (Scattered Delay Network): The sound bounces between every reverb node which act as reflecting surfaces. This reverb algorithm favours an odd number of nodes that should not have too much symmetry. This helps reduce the artifacts of the reverb or any metallic sounding resonance.
- FDN (Feedback Delay Network): Each reverb node functions as a separate reverb processor with a classical reverb algorithm. Place nodes around the stage and eventually around the audience. Inputs are sent to each node in a similar fashion as with outputs. The return signal from the reverb is then distributed like other inputs to all outputs.
- IR (Impulse Response): This is a classical convolution reverb. You can load audio samples as impulse responses. Each reverb node can share the same IR or they can have different ones. Place nodes around the stage and eventually around the audience.
The node positions can be adjusted directly on the map. The Ctrl/Cmd key will move a reverb node pair in symmetry.
  - IT: Tre tipi di riverbero sono integrati in questo processore di suono spaziale:
- SDN (Scattered Delay Network): Il suono rimbalza tra ogni nodo di riverbero che agisce come superficie riflettente. Questo algoritmo favorisce un numero dispari di nodi senza troppa simmetria, per ridurre artefatti o risonanze metalliche.
- FDN (Feedback Delay Network): Ogni nodo funziona come un processore di riverbero separato con un algoritmo classico. Posizionare i nodi intorno al palco e eventualmente intorno al pubblico.
- IR (Risposta all'Impulso): Riverbero a convoluzione classico. È possibile caricare campioni audio come risposte all'impulso. Ogni nodo può condividere la stessa IR o usarne diverse.
Le posizioni dei nodi possono essere regolate direttamente sulla mappa. Il tasto Ctrl/Cmd sposta una coppia di nodi in simmetria.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Algorithms
  - IT: Algoritmi di Riverbero
  - [ ] OK    Fix: 

## `help.reverbFeed`

- **`body`**
  - EN: Pre-processing sending of the input channels to the reverb nodes.
- Orientation and On/Off Angles define what inputs each reverb node will receive. Inputs in the green sector will be picked-up, but not the ones in front in the red sector. There is a fade between both sectors.
- HF Damping simulates the loss of high frequency with distance.
- Distance Attenuation percentage allows you to define if more or less of the attenuation calculated from the distance and parameters of the input is applied.
- Minimal Latency toggles whether the output is scanned for the smallest calculated delay and also if the setting once engaged on an input will reduce the delay for that input through this reverb node.
- Live Source Attenuation toggles the reduction of level of nearby input if they have this setting enabled.
  - IT: Invio pre-elaborazione dei canali di ingresso ai nodi.
- Orientamento e angoli On/Off definiscono quali ingressi riceve ogni nodo.
- Smorzamento HF simula la perdita di alta frequenza.
- Percentuale di attenuazione distanza definisce l'attenuazione applicata.
- Latenza minima determina se viene utilizzato il ritardo minimo.
- Attenuazione sorgente live riduce il livello degli ingressi vicini.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Feed
  - IT: Alimentazione Riverbero
  - [ ] OK    Fix: 

## `help.reverbPostProc`

- **`body`**
  - EN: Includes a 4-band EQ and an Expander that looks at the signal entering the reverb processor to reduce long reverb tails when the inputs are quiet.
  - IT: Include un EQ a 4 bande e un Espansore che monitora il segnale in ingresso al processore di riverbero per ridurre le code lunghe di riverbero quando gli ingressi sono silenziosi.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Post-Processing
  - IT: Post-Elaborazione Riverbero
  - [ ] OK    Fix: 

## `help.reverbPreProc`

- **`body`**
  - EN: Includes a 4-band EQ and a Compressor to remove transients which might excite the reverb processor a bit too much.
  - IT: Include un EQ a 4 bande e un Compressore per rimuovere i transienti che potrebbero eccitare troppo il processore di riverbero.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Pre-Processing
  - IT: Pre-Elaborazione Riverbero
  - [ ] OK    Fix: 

## `help.reverbReturn`

- **`body`**
  - EN: Post-processing sending to the speakers.
- Distance Attenuation defines the level drop per meter to the speakers.
- Common Attenuation will look for the lowest attenuation and keep only a percentage of this level drop and apply this across all other outputs.
- Mutes and Mute Macros allow to prevent a reverb channel from feeding an output. For instance it may not be necessary to send reverb to sub-bass speakers.
  - IT: Post-elaborazione inviata agli altoparlanti.
- Attenuazione Distanza definisce il calo di livello per metro.
- Attenuazione Comune mantiene una percentuale dell'attenuazione più bassa.
- Mute impediscono a un canale di riverbero di alimentare un'uscita.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Return
  - IT: Ritorno Riverbero
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
  - IT: Il sampler permette di attivare campioni e interagire con essi in tempo reale.
Quando abilitato su una traccia, il sampler sostituisce l'ingresso live in ogni momento.
Diversi sampler possono essere assegnati a ingressi diversi e attivati individualmente.
Per usare il sampler:
- Selezionare un Roli Lightpad o un pad sull'app Android Remote collegata.
- Aggiungere campioni alle diverse celle della griglia. Regolare la posizione di partenza relativa, il livello e i punti di ingresso e uscita. Più campioni possono essere selezionati tenendo premuto Maiusc mentre si clicca.
- Creare set di campioni: i campioni selezionati verranno aggiunti ai nuovi set. Possono essere aggiunti o rimossi tenendo premuto Ctrl/Cmd mentre si clicca sulle celle. Ogni set può essere rinominato e avere una sequenza fissa o casuale. Ogni set ha un'impostazione di attenuazione e una posizione base.
- Premere un Lightpad o pad per attivare un campione. La pressione può essere mappata a livello, altezza e filtraggio alte frequenze. Il movimento del dito sposta il suono come un joystick.
Rilasciare il pad ferma il campione.
Le impostazioni del sampler sono salvate nei file di ingresso.
Celle e set possono essere copiati, esportati e importati.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Sampler
  - IT: Sampler
  - [ ] OK    Fix: 

## `help.sessionData`

- **`body`**
  - EN: When starting a session select the working folder where the system will place the files and eventual audio files. For new projects create a new folder. When reloading a previous session navigate to the corresponding folder.
Each section has a separate xml file (System configuration, Network, Outputs, Reverbs, Inputs) and backups. Additionally convolution reverb impulse responses and audio samples will be stored in sub-directories.
Each section can be stored and recalled individually or as a whole.
Each section can also export and import files from other projects.
  - IT: All'avvio di una sessione, selezionare la cartella di lavoro dove il sistema posizionerà i file e gli eventuali file audio. Per nuovi progetti, creare una nuova cartella. Per ricaricare una sessione precedente, navigare alla cartella corrispondente.
Ogni sezione ha un file xml separato (Configurazione sistema, Rete, Uscite, Riverbero, Ingressi) e backup. Le risposte all'impulso del riverbero a convoluzione e i campioni audio saranno memorizzati in sottodirectory.
Ogni sezione può essere memorizzata e richiamata individualmente o nel suo insieme.
Ogni sezione può anche esportare e importare file da altri progetti.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Session Data
  - IT: Dati di Sessione
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
  - IT: Gli snapshot memorizzano i parametri di ingresso, ma possono avere un ambito per essere richiamati durante uno spettacolo.
L'Ambito indica al sistema quali dati memorizzare o richiamare.
Sono disponibili diversi metodi:
- Registrare solo i dati necessari in file locali. Il filtro viene applicato al salvataggio.
- Registrare tutti i dati e un filtro in file locali. Il filtro viene applicato al richiamo.
- Registrare tutti i dati nelle cue di QLab. Non raccomandato per configurazioni grandi.
L'ambito può mostrare e preselezionare automaticamente i parametri modificati manualmente. Le modifiche sono contrassegnate in giallo.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Input Snapshots and Scope
  - IT: Snapshot di Ingresso e Ambito
  - [ ] OK    Fix: 

## `help.tracking`

- **`body`**
  - EN: Tracking allows to follow the position in 2D or 3D of actors and musicians. There are several tracking solutions based on UWB (UltraWide Band) tags, 3D camera, computer vision systems and Infra-Red LED's followed by IR sensitive cameras.
This application allows you to receive tracking data from several protocols: OSC, MQTT, PosiStageNet/PSN, RTTrP.
You can select which protocol you're using and enter its settings. You may also adjust the mapping (offset, scaling and orientation).
Each input has a toggle to enable tracking, an ID to select which tracking marker it should follow and a smoothing algorithm to reduce jittery movements.
  - IT: Il tracking permette di seguire la posizione in 2D o 3D di attori e musicisti. Esistono diverse soluzioni basate su tag UWB, telecamere 3D, sistemi di visione artificiale e LED infrarossi con telecamere sensibili all'IR.
Questa applicazione permette di ricevere dati di tracking da diversi protocolli: OSC, MQTT, PosiStageNet/PSN, RTTrP.
È possibile selezionare il protocollo utilizzato e configurare le impostazioni. Si può anche regolare il mapping (offset, scala e orientamento).
Ogni ingresso ha un toggle per attivare il tracking, un ID per selezionare il marcatore e un algoritmo di smoothing.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Tracking
  - IT: Tracking
  - [ ] OK    Fix: 

## `help.tuning`

- **`body`**
  - EN: System tuning for WFS is different from standard PA tuning. It can proceed as follows:
- Start with the flown array muted. Set the desired levels for the near-field speakers when listening to them in the first row. Adjust the high frequency shelf attenuation so the near-field speakers are not too bright and draw too much attention.
- Mute the near-field array and unmute the flown array, find a suitable level towards the back of the house.
- Unmute both arrays, adjust the delay of the flown array to bring the sound down to the correct height in the lower rows. Adjust the levels, HF shelf/distance ratio and vertical and horizontal parallax for each array to have consistent level wherever your inputs are on stage. This is especially important if your inputs are actors, singers or acoustic music instruments on stage. You may also test adding some Haas effect to delay the whole system if you feel the WFS is too early or adding comb filtering with the acoustic sound.
You may follow a different workflow for tuning or go for different cues.
  - IT: L'accordatura del sistema WFS è diversa dall'accordatura PA standard. Può procedere come segue:
- Iniziate con l'array sospeso silenziato. Impostate i livelli desiderati per gli altoparlanti di prossimità ascoltandoli dalla prima fila. Regolate l'attenuazione dello shelf di alta frequenza affinché gli altoparlanti di prossimità non siano troppo brillanti.
- Silenziate l'array di prossimità e attivate l'array sospeso, trovate un livello adeguato verso il fondo della sala.
- Attivate entrambi gli array, regolate il ritardo dell'array sospeso per portare il suono all'altezza corretta nelle file inferiori. Regolate livelli, shelf HF/rapporto di distanza e parallasse verticale e orizzontale per ogni array per un livello coerente ovunque siano i vostri ingressi sul palco.
Potete seguire un flusso di lavoro diverso per l'accordatura o mirare a impostazioni diverse per diverse situazioni.
  - [ ] OK    Fix: 

- **`title`**
  - EN: System Tuning
  - IT: Accordatura del sistema
  - [ ] OK    Fix: 

## `inputs`

- **`arrayPrefix`**
  - EN: Array
  - IT: Array
  - [ ] OK    Fix: 

## `inputs.buttons`

- **`deleteSnapshot`**
  - EN: Delete Snapshot
  - IT: Elimina Snapshot
  - [ ] OK    Fix: 

- **`editScope`**
  - EN: Edit Scope
  - IT: Modifica Ambito
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - IT: Esporta
  - [ ] OK    Fix: 

- **`hiddenOnMap`**
  - EN: Input Hidden on Map
  - IT: Ingresso Nascosto sulla Mappa
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - IT: Importa
  - [ ] OK    Fix: 

- **`lockOnMap`**
  - EN: Lock on Map
  - IT: Blocca sulla Mappa
  - [ ] OK    Fix: 

- **`pauseAll`**
  - EN: Pause All
  - IT: Pausa Tutti
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Backup
  - IT: Ricarica Backup
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Input Config
  - IT: Ricarica Config Ingressi
  - [ ] OK    Fix: 

- **`reloadSnapshot`**
  - EN: Reload Snapshot
  - IT: Ricarica Snapshot
  - [ ] OK    Fix: 

- **`reloadWithoutScope`**
  - EN: Reload Without Scope
  - IT: Ricarica senza filtro
  - [ ] OK    Fix: 

- **`resumeAll`**
  - EN: Resume All
  - IT: Riprendi Tutti
  - [ ] OK    Fix: 

- **`samplerOff`**
  - EN: Sampler: OFF
  - IT: Sampler: OFF
  - [ ] OK    Fix: 

- **`samplerOn`**
  - EN: Sampler: ON
  - IT: Sampler: ON
  - [ ] OK    Fix: 

- **`setAllInputs`**
  - EN: Set all Inputs...
  - IT: Imposta tutti gli Ingressi...
  - [ ] OK    Fix: 

- **`solo`**
  - EN: Solo
  - IT: Solo
  - [ ] OK    Fix: 

- **`soloModeMulti`**
  - EN: Multi
  - IT: Multi
  - [ ] OK    Fix: 

- **`soloModeSingle`**
  - EN: Single
  - IT: Single
  - [ ] OK    Fix: 

- **`stopAll`**
  - EN: Stop All
  - IT: Ferma Tutti
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Input Config
  - IT: Memorizza Config Ingressi
  - [ ] OK    Fix: 

- **`storeSnapshot`**
  - EN: Store Snapshot
  - IT: Memorizza Snapshot
  - [ ] OK    Fix: 

- **`updateSnapshot`**
  - EN: Update Snapshot
  - IT: Aggiorna Snapshot
  - [ ] OK    Fix: 

- **`visibleOnMap`**
  - EN: Input Visible on Map
  - IT: Ingresso Visibile sulla Mappa
  - [ ] OK    Fix: 

## `inputs.clusters`

- **`clusterPrefix`**
  - EN: Cluster
  - IT: Cluster
  - [ ] OK    Fix: 

- **`single`**
  - EN: Single
  - IT: Singolo
  - [ ] OK    Fix: 

## `inputs.coordinates`

- **`xyz`**
  - EN: XYZ
  - IT: XYZ
  - [ ] OK    Fix: 

## `inputs.dialogs`

- **`exportConfig`**
  - EN: Export Input Configuration
  - IT: Esporta Configurazione Ingressi
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Input Configuration
  - IT: Importa Configurazione Ingressi
  - [ ] OK    Fix: 

- **`selectChannel`**
  - EN: Select Channel
  - IT: Seleziona canale
  - [ ] OK    Fix: 

- **`snapshotNameLabel`**
  - EN: Name:
  - IT: Nome:
  - [ ] OK    Fix: 

- **`storeSnapshotMessage`**
  - EN: Enter a name for the new snapshot:
  - IT: Inserisci un nome per il nuovo snapshot:
  - [ ] OK    Fix: 

- **`storeSnapshotTitle`**
  - EN: Store New Snapshot
  - IT: Memorizza nuovo snapshot
  - [ ] OK    Fix: 

- **`trackingConflictCluster`**
  - EN: Input {current} has tracking enabled, but Input {existing} in Cluster {cluster} is already tracked.

Only one tracked input per cluster is allowed.
  - IT: L'ingresso {current} ha il tracking attivo, ma l'ingresso {existing} nel cluster {cluster} è già tracciato.

È consentito solo un ingresso tracciato per cluster.
  - [ ] OK    Fix: 

- **`trackingConflictContinue`**
  - EN: Continue (disable tracking)
  - IT: Continua (disattiva tracking)
  - [ ] OK    Fix: 

- **`trackingConflictSwitch`**
  - EN: Input {existing} in Cluster {cluster} already has tracking enabled.

Only one tracked input per cluster is allowed.

Do you want to disable tracking on Input {existing} and enable it on Input {to}?
  - IT: L'ingresso {existing} nel cluster {cluster} ha già il tracking attivo.

È consentito solo un ingresso tracciato per cluster.

Vuoi disattivare il tracking sull'ingresso {existing} e attivarlo sull'ingresso {to}?
  - [ ] OK    Fix: 

- **`trackingConflictTitle`**
  - EN: Tracking Conflict
  - IT: Conflitto di tracking
  - [ ] OK    Fix: 

- **`trackingConflictYes`**
  - EN: Yes, switch tracking
  - IT: Sì, trasferisci tracking
  - [ ] OK    Fix: 

## `inputs.gradientMap`

- **`buttons.copyLayer`**
  - EN: Copy Layer
  - IT: Copia livello
  - [ ] OK    Fix: 

- **`buttons.copyShape`**
  - EN: Copy Shape
  - IT: Copia forma
  - [ ] OK    Fix: 

- **`buttons.delete`**
  - EN: Delete
  - IT: Elimina
  - [ ] OK    Fix: 

- **`buttons.enable`**
  - EN: On
  - IT: On
  - [ ] OK    Fix: 

- **`buttons.lock`**
  - EN: Lock
  - IT: Blocca
  - [ ] OK    Fix: 

- **`buttons.pasteLayer`**
  - EN: Paste Layer
  - IT: Incolla livello
  - [ ] OK    Fix: 

- **`buttons.pasteShape`**
  - EN: Paste Shape
  - IT: Incolla forma
  - [ ] OK    Fix: 

- **`header.attenuationLayer`**
  - EN: Attenuation Layer
  - IT: Livello Attenuazione
  - [ ] OK    Fix: 

- **`header.heightLayer`**
  - EN: Height Layer
  - IT: Livello Altezza
  - [ ] OK    Fix: 

- **`header.hfShelfLayer`**
  - EN: HF Shelf Layer
  - IT: Livello Shelf HF
  - [ ] OK    Fix: 

- **`help.blackValue`**
  - EN: Parameter value mapped to black (0.00–1.00)
  - IT: Valore del parametro mappato al nero (0.00–1.00)
  - [ ] OK    Fix: 

- **`help.blur`**
  - EN: Edge blur in meters
  - IT: Sfocatura bordi in metri
  - [ ] OK    Fix: 

- **`help.copy`**
  - EN: Copy selected shape or layer
  - IT: Copia forma o livello selezionato
  - [ ] OK    Fix: 

- **`help.curve`**
  - EN: Gamma curve (-1 to 1, 0 = linear)
  - IT: Curva gamma (-1 a 1, 0 = lineare)
  - [ ] OK    Fix: 

- **`help.ellipseTool`**
  - EN: Draw ellipse
  - IT: Disegna ellisse
  - [ ] OK    Fix: 

- **`help.fillTool`**
  - EN: Apply uniform fill to shape
  - IT: Applica riempimento uniforme alla forma
  - [ ] OK    Fix: 

- **`help.fillValue`**
  - EN: Fill brightness (0 = black, 1 = white)
  - IT: Luminosità riempimento (0 = nero, 1 = bianco)
  - [ ] OK    Fix: 

- **`help.layerEnable`**
  - EN: Enable/disable layer (affects output and OSC)
  - IT: Attiva/disattiva livello (influisce su uscita e OSC)
  - [ ] OK    Fix: 

- **`help.layerSelect`**
  - EN: Select this layer for editing
  - IT: Seleziona questo livello per la modifica
  - [ ] OK    Fix: 

- **`help.layerVisible`**
  - EN: Show/hide layer preview on canvas
  - IT: Mostra/nascondi anteprima livello sul canvas
  - [ ] OK    Fix: 

- **`help.linGradTool`**
  - EN: Apply linear gradient to shape
  - IT: Applica gradiente lineare alla forma
  - [ ] OK    Fix: 

- **`help.paste`**
  - EN: Paste shape or layer from clipboard
  - IT: Incolla forma o livello dagli appunti
  - [ ] OK    Fix: 

- **`help.polygonTool`**
  - EN: Draw polygon (double-click to close)
  - IT: Disegna poligono (doppio clic per chiudere)
  - [ ] OK    Fix: 

- **`help.radGradTool`**
  - EN: Apply radial gradient to shape
  - IT: Applica gradiente radiale alla forma
  - [ ] OK    Fix: 

- **`help.rectTool`**
  - EN: Draw rectangle
  - IT: Disegna rettangolo
  - [ ] OK    Fix: 

- **`help.selectTool`**
  - EN: Select and move shapes
  - IT: Seleziona e sposta forme
  - [ ] OK    Fix: 

- **`help.shapeDelete`**
  - EN: Delete selected shape(s)
  - IT: Elimina forma/e selezionata/e
  - [ ] OK    Fix: 

- **`help.shapeEnable`**
  - EN: Enable/disable shape
  - IT: Attiva/disattiva forma
  - [ ] OK    Fix: 

- **`help.shapeLock`**
  - EN: Lock shape position
  - IT: Blocca posizione della forma
  - [ ] OK    Fix: 

- **`help.whiteValue`**
  - EN: Parameter value mapped to white (0.00–1.00)
  - IT: Valore del parametro mappato al bianco (0.00–1.00)
  - [ ] OK    Fix: 

- **`hints.darkMaxAtten`**
  - EN: Dark = max attenuation | Light = none
  - IT: Scuro = max attenuazione | Chiaro = nessuna
  - [ ] OK    Fix: 

- **`hints.darkMaxHeight`**
  - EN: Dark = max height | Light = ground
  - IT: Scuro = max altezza | Chiaro = suolo
  - [ ] OK    Fix: 

- **`hints.darkMaxHF`**
  - EN: Dark = max HF shelf | Light = none
  - IT: Scuro = max shelf HF | Chiaro = nessuno
  - [ ] OK    Fix: 

- **`hints.polygonClose`**
  - EN: Double-click to close polygon
  - IT: Doppio clic per chiudere il poligono
  - [ ] OK    Fix: 

- **`hints.whiteMaxAtten`**
  - EN: White = max attenuation | Black = none
  - IT: Bianco = max attenuazione | Nero = nessuna
  - [ ] OK    Fix: 

- **`hints.whiteMaxHeight`**
  - EN: White = max height | Black = ground
  - IT: Bianco = max altezza | Nero = suolo
  - [ ] OK    Fix: 

- **`hints.whiteMaxHF`**
  - EN: White = max HF shelf | Black = none
  - IT: Bianco = max shelf HF | Nero = nessuno
  - [ ] OK    Fix: 

- **`labels.black`**
  - EN: Black:
  - IT: Nero:
  - [ ] OK    Fix: 

- **`labels.blur`**
  - EN: Blur:
  - IT: Sfocatura:
  - [ ] OK    Fix: 

- **`labels.center`**
  - EN: Center:
  - IT: Centro:
  - [ ] OK    Fix: 

- **`labels.curve`**
  - EN: Curve:
  - IT: Curva:
  - [ ] OK    Fix: 

- **`labels.edge`**
  - EN: Edge:
  - IT: Bordo:
  - [ ] OK    Fix: 

- **`labels.end`**
  - EN: End:
  - IT: Fine:
  - [ ] OK    Fix: 

- **`labels.fill`**
  - EN: Fill:
  - IT: Riempimento:
  - [ ] OK    Fix: 

- **`labels.name`**
  - EN: Name:
  - IT: Nome:
  - [ ] OK    Fix: 

- **`labels.start`**
  - EN: Start:
  - IT: Inizio:
  - [ ] OK    Fix: 

- **`labels.white`**
  - EN: White:
  - IT: Bianco:
  - [ ] OK    Fix: 

- **`layers.attenuation`**
  - EN: Attenuation
  - IT: Attenuazione
  - [ ] OK    Fix: 

- **`layers.height`**
  - EN: Height
  - IT: Altezza
  - [ ] OK    Fix: 

- **`layers.hfShelf`**
  - EN: HF Shelf
  - IT: Shelf HF
  - [ ] OK    Fix: 

- **`tools.editPoints`**
  - EN: Edit Points
  - IT: Modifica punti
  - [ ] OK    Fix: 

- **`tools.ellipse`**
  - EN: Ellipse
  - IT: Ellisse
  - [ ] OK    Fix: 

- **`tools.fill`**
  - EN: Fill
  - IT: Riempi
  - [ ] OK    Fix: 

- **`tools.linGrad`**
  - EN: Linear Grad.
  - IT: Grad. Lineare
  - [ ] OK    Fix: 

- **`tools.polygon`**
  - EN: Polygon
  - IT: Poligono
  - [ ] OK    Fix: 

- **`tools.radGrad`**
  - EN: Radial Grad.
  - IT: Grad. Radiale
  - [ ] OK    Fix: 

- **`tools.rect`**
  - EN: Rectangle
  - IT: Rettangolo
  - [ ] OK    Fix: 

- **`tools.select`**
  - EN: Select
  - IT: Selezione
  - [ ] OK    Fix: 

- **`warnings.heightRatioZero`**
  - EN: Height Ratio is 0% — increase it for height to take effect
  - IT: Il rapporto altezza è 0% — aumentalo perché l'altezza abbia effetto
  - [ ] OK    Fix: 

## `inputs.help`

- **`admMapping`**
  - EN: Assign this input to an ADM-OSC mapping for position receive/transmit.
  - IT: Assegnare questo ingresso a una mappatura ADM-OSC per la ricezione/trasmissione della posizione.
  - [ ] OK    Fix: 

- **`arrayAttenDial`**
  - EN: Attenuation for Array {num} (-60 to 0 dB).
  - IT: Attenuazione per l'array {num} (-60 a 0 dB).
  - [ ] OK    Fix: 

- **`attenuationLawButton`**
  - EN: Attenuation Law Model (Linear Decrease of Volume with Distance Between Object and Speaker or Squared).
  - IT: Modello di legge di attenuazione (diminuzione lineare del volume con la distanza tra oggetto e altoparlante, o quadratica).
  - [ ] OK    Fix: 

- **`attenuationSlider`**
  - EN: Input Channel Attenuation.
  - IT: Attenuazione del canale di ingresso.
  - [ ] OK    Fix: 

- **`channelSelector`**
  - EN: Input Channel Number and Selection.
  - IT: Numero e selezione del canale di ingresso.
  - [ ] OK    Fix: 

- **`clusterSelector`**
  - EN: Object is Part of a Cluster.
  - IT: L'oggetto fa parte di un cluster.
  - [ ] OK    Fix: 

- **`commonAttenDial`**
  - EN: Percentage of the Common Part of the Attenuation for selected Object Relative to All Outputs.
  - IT: Percentuale della parte comune dell'attenuazione per l'oggetto selezionato rispetto a tutte le uscite.
  - [ ] OK    Fix: 

- **`constraintDistanceButton`**
  - EN: Limit Position to Distance Range from Origin (for Cylindrical/Spherical modes).
  - IT: Limitare la posizione a un intervallo di distanza dall'origine (per modalità cilindrica/sferica).
  - [ ] OK    Fix: 

- **`constraintXButton`**
  - EN: Limit Position to the Bounds of the Stage in Width.
  - IT: Limitare la posizione ai limiti del palco in larghezza.
  - [ ] OK    Fix: 

- **`constraintYButton`**
  - EN: Limit Position to the Bounds of the Stage in Depth.
  - IT: Limitare la posizione ai limiti del palco in profondità.
  - [ ] OK    Fix: 

- **`constraintZButton`**
  - EN: Limit Position to the Bounds of the Stage in Height.
  - IT: Limitare la posizione ai limiti del palco in altezza.
  - [ ] OK    Fix: 

- **`coordModeSelector`**
  - EN: Coordinate display mode: Cartesian (X/Y/Z), Cylindrical (radius/azimuth/height), or Spherical (radius/azimuth/elevation).
  - IT: Modalità di visualizzazione delle coordinate: Cartesiana (X/Y/Z), Cilindrica (raggio/azimut/altezza) o Sferica (raggio/azimut/elevazione).
  - [ ] OK    Fix: 

- **`delayLatencySlider`**
  - EN: Input Channel Delay (positive values) or Latency Compensation (negative values).
  - IT: Ritardo del canale di ingresso (valori positivi) o compensazione di latenza (valori negativi).
  - [ ] OK    Fix: 

- **`deleteSnapshot`**
  - EN: Delete Selected Input Snapshot With Confirmation.
  - IT: Elimina lo snapshot di ingresso selezionato con conferma.
  - [ ] OK    Fix: 

- **`directivitySlider`**
  - EN: How Wide is the Brightness of The Object.
  - IT: Larghezza del cono di brillantezza dell'oggetto.
  - [ ] OK    Fix: 

- **`distanceAttenDial`**
  - EN: Attenuation per Meter Between Object and Speaker.
  - IT: Attenuazione per metro tra oggetto e altoparlante.
  - [ ] OK    Fix: 

- **`distanceMaxEditor`**
  - EN: Maximum Distance from Origin in Meters.
  - IT: Distanza massima dall'origine in metri.
  - [ ] OK    Fix: 

- **`distanceMinEditor`**
  - EN: Minimum Distance from Origin in Meters.
  - IT: Distanza minima dall'origine in metri.
  - [ ] OK    Fix: 

- **`distanceRangeSlider`**
  - EN: Set Minimum and Maximum Distance from Origin.
  - IT: Impostare la distanza minima e massima dall'origine.
  - [ ] OK    Fix: 

- **`distanceRatioDial`**
  - EN: Attenuation Ratio for Squared Model.
  - IT: Rapporto di attenuazione per il modello quadratico.
  - [ ] OK    Fix: 

- **`editScope`**
  - EN: Open Selected Input Snapshot Scope Window.
  - IT: Apri la finestra del filtro dello snapshot di ingresso selezionato.
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Input Configuration to file (with file explorer window).
  - IT: Esporta la configurazione di ingresso su file (con esplora file).
  - [ ] OK    Fix: 

- **`flipXButton`**
  - EN: X will be Symetrical to the Origin. Keyboard Nudging will be Inverted.
  - IT: X sarà simmetrica rispetto all'origine. Lo spostamento da tastiera sarà invertito.
  - [ ] OK    Fix: 

- **`flipYButton`**
  - EN: Y will be Symetrical to the Origin. Keyboard Nudging will be Inverted.
  - IT: Y sarà simmetrica rispetto all'origine. Lo spostamento da tastiera sarà invertito.
  - [ ] OK    Fix: 

- **`flipZButton`**
  - EN: Z will be Symetrical to the Origin. Keyboard Nudging will be Inverted.
  - IT: Z sarà simmetrica rispetto all'origine. Lo spostamento da tastiera sarà invertito.
  - [ ] OK    Fix: 

- **`frActiveButton`**
  - EN: Enable Simulated Floor Reflections for the Object.
  - IT: Attivare le riflessioni a pavimento simulate per l'oggetto.
  - [ ] OK    Fix: 

- **`frAttenuationSlider`**
  - EN: Attenuation of the Simulated Floor Reflections for the Object.
  - IT: Attenuazione delle riflessioni a pavimento simulate per l'oggetto.
  - [ ] OK    Fix: 

- **`frDiffusionDial`**
  - EN: Diffusion Effect of the Simulated Floor Reflections for the Object.
  - IT: Effetto di diffusione delle riflessioni a pavimento simulate per l'oggetto.
  - [ ] OK    Fix: 

- **`frHighShelfActiveButton`**
  - EN: Enable High Shelf Filter for Floor Reflections.
  - IT: Attivare il filtro shelving alto per le riflessioni a pavimento.
  - [ ] OK    Fix: 

- **`frHighShelfFreqSlider`**
  - EN: High Shelf Frequency for Floor Reflections.
  - IT: Frequenza dello shelving alto per le riflessioni a pavimento.
  - [ ] OK    Fix: 

- **`frHighShelfGainSlider`**
  - EN: High Shelf Gain for Floor Reflections.
  - IT: Guadagno dello shelving alto per le riflessioni a pavimento.
  - [ ] OK    Fix: 

- **`frHighShelfSlopeSlider`**
  - EN: High Shelf Slope for Floor Reflections.
  - IT: Pendenza dello shelving alto per le riflessioni a pavimento.
  - [ ] OK    Fix: 

- **`frLowCutActiveButton`**
  - EN: Enable Low Cut Filter for Floor Reflections.
  - IT: Attivare il filtro passa-alto per le riflessioni a pavimento.
  - [ ] OK    Fix: 

- **`frLowCutFreqSlider`**
  - EN: Low Cut Frequency for Floor Reflections.
  - IT: Frequenza del passa-alto per le riflessioni a pavimento.
  - [ ] OK    Fix: 

- **`heightFactorDial`**
  - EN: Take Elevation of Object into Account Fully, Partially or Not.
  - IT: Considerare l'elevazione dell'oggetto totalmente, parzialmente o per nulla.
  - [ ] OK    Fix: 

- **`hfShelfSlider`**
  - EN: How Much Brightness is lost in the Back of the Object, Out of its Brightness Cone.
  - IT: Quanta brillantezza si perde sul retro dell'oggetto, fuori dal suo cono di brillantezza.
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Input Configuration from file (with file explorer window).
  - IT: Importa la configurazione di ingresso da file (con esplora file).
  - [ ] OK    Fix: 

- **`inputDirectivityDial`**
  - EN: Where is the Object pointing to in the Horizontal Plane.
  - IT: Direzione dell'oggetto sul piano orizzontale.
  - [ ] OK    Fix: 

- **`jitterSlider`**
  - EN: Sphere of Rapid Movements of the Object.
  - IT: Sfera dei movimenti rapidi dell'oggetto.
  - [ ] OK    Fix: 

- **`lfoActiveButton`**
  - EN: Enable or Disable the Periodic Movement of the Object (LFO).
  - IT: Attivare o disattivare il movimento periodico dell'oggetto (LFO).
  - [ ] OK    Fix: 

- **`lfoAmplitudeXSlider`**
  - EN: Width of Movement in Relation to Base Position of the Object.
  - IT: Larghezza del movimento rispetto alla posizione di base dell'oggetto.
  - [ ] OK    Fix: 

- **`lfoAmplitudeYSlider`**
  - EN: Depth of Movement in Relation to Base Position of the Object.
  - IT: Profondità del movimento rispetto alla posizione di base dell'oggetto.
  - [ ] OK    Fix: 

- **`lfoAmplitudeZSlider`**
  - EN: Height of Movement in Relation to Base Position of the Object.
  - IT: Altezza del movimento rispetto alla posizione di base dell'oggetto.
  - [ ] OK    Fix: 

- **`lfoGyrophoneSelector`**
  - EN: Rotation of the Brightness Cone of the Object.
  - IT: Rotazione del cono di brillantezza dell'oggetto.
  - [ ] OK    Fix: 

- **`lfoPeriodDial`**
  - EN: Base Period of the Movement of the Object.
  - IT: Periodo di base del movimento dell'oggetto.
  - [ ] OK    Fix: 

- **`lfoPhaseDial`**
  - EN: Phase Offset of the Movement of the Object.
  - IT: Sfasamento del movimento dell'oggetto.
  - [ ] OK    Fix: 

- **`lfoPhaseXDial`**
  - EN: Phase Offset of the Movement of the Object in Width.
  - IT: Sfasamento del movimento dell'oggetto in larghezza.
  - [ ] OK    Fix: 

- **`lfoPhaseYDial`**
  - EN: Phase Offset of the Movement of the Object in Depth.
  - IT: Sfasamento del movimento dell'oggetto in profondità.
  - [ ] OK    Fix: 

- **`lfoPhaseZDial`**
  - EN: Phase Offset of the Movement of the Object in Height.
  - IT: Sfasamento del movimento dell'oggetto in altezza.
  - [ ] OK    Fix: 

- **`lfoRateXSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Width.
  - IT: Movimento più rapido o più lento rispetto al periodo di base, in larghezza.
  - [ ] OK    Fix: 

- **`lfoRateYSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Depth.
  - IT: Movimento più rapido o più lento rispetto al periodo di base, in profondità.
  - [ ] OK    Fix: 

- **`lfoRateZSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Height.
  - IT: Movimento più rapido o più lento rispetto al periodo di base, in altezza.
  - [ ] OK    Fix: 

- **`lfoShapeXSelector`**
  - EN: Movement Behaviour of the Object in Width.
  - IT: Comportamento del movimento dell'oggetto in larghezza.
  - [ ] OK    Fix: 

- **`lfoShapeYSelector`**
  - EN: Movement Behaviour of the Object in Depth.
  - IT: Comportamento del movimento dell'oggetto in profondità.
  - [ ] OK    Fix: 

- **`lfoShapeZSelector`**
  - EN: Movement Behaviour of the Object in Height.
  - IT: Comportamento del movimento dell'oggetto in altezza.
  - [ ] OK    Fix: 

- **`lsActiveButton`**
  - EN: If You Need to Reduce the Level in Speakers Close to the Object. (eg. Loud Source Present on Stage)
  - IT: Se è necessario ridurre il livello degli altoparlanti vicini all'oggetto (es.: sorgente forte presente sul palco).
  - [ ] OK    Fix: 

- **`lsAttenuationSlider`**
  - EN: Constant Attenuation of Speakers Around the Object.
  - IT: Attenuazione costante degli altoparlanti attorno all'oggetto.
  - [ ] OK    Fix: 

- **`lsPeakEnableButton`**
  - EN: Enable or Disable the Fast (Peak) Compressor for the Live Source Tamer.
  - IT: Attivare o disattivare il compressore rapido (picco) per il Domatore di Source Live.
  - [ ] OK    Fix: 

- **`lsPeakRatioDial`**
  - EN: Ratio to Apply the Fast Compression for Speakers Around the Object.
  - IT: Ratio applicato alla compressione rapida per gli altoparlanti attorno all'oggetto.
  - [ ] OK    Fix: 

- **`lsPeakThresholdSlider`**
  - EN: Fast Compression Threshold for Speakers Around the Object to Control Transients.
  - IT: Soglia di compressione rapida per gli altoparlanti attorno all'oggetto, per controllare i transitori.
  - [ ] OK    Fix: 

- **`lsRadiusSlider`**
  - EN: How Far does the Attenuation Affect The Speakers.
  - IT: Fino a dove l'attenuazione influisce sugli altoparlanti.
  - [ ] OK    Fix: 

- **`lsShapeSelector`**
  - EN: Profile of the Attenuation Around the Object.
  - IT: Profilo dell'attenuazione attorno all'oggetto.
  - [ ] OK    Fix: 

- **`lsSlowEnableButton`**
  - EN: Enable or Disable the Slow Compressor for the Live Source Tamer.
  - IT: Attivare o disattivare il compressore lento per il Domatore di Source Live.
  - [ ] OK    Fix: 

- **`lsSlowRatioDial`**
  - EN: Ratio to Apply the Slow Compression for Speakers Around the Object.
  - IT: Ratio applicato alla compressione lenta per gli altoparlanti attorno all'oggetto.
  - [ ] OK    Fix: 

- **`lsSlowThresholdSlider`**
  - EN: Slow Compression Threshold for Speakers Around the Object to Control Sustained Level.
  - IT: Soglia di compressione lenta per gli altoparlanti attorno all'oggetto, per controllare il livello sostenuto.
  - [ ] OK    Fix: 

- **`mapLockButton`**
  - EN: Prevent Interaction on the Map Tab
  - IT: Impedire l'interazione sulla scheda Mappa.
  - [ ] OK    Fix: 

- **`mapVisibilityButton`**
  - EN: Make Visible or Hide The Selected Input on the Map
  - IT: Mostrare o nascondere l'ingresso selezionato sulla mappa.
  - [ ] OK    Fix: 

- **`maxSpeedActiveButton`**
  - EN: Enable or Disable Speed Limiting for Object.
  - IT: Attivare o disattivare la limitazione della velocità per l'oggetto.
  - [ ] OK    Fix: 

- **`maxSpeedDial`**
  - EN: Maximum Speed Limit for Object.
  - IT: Limite di velocità massima per l'oggetto.
  - [ ] OK    Fix: 

- **`minimalLatencyButton`**
  - EN: Select between Acoustic Precedence and Minimal Latency for Amplification Precedence.
  - IT: Scegliere tra precedenza acustica e latenza minima per la precedenza di amplificazione.
  - [ ] OK    Fix: 

- **`muteButton`**
  - EN: Mute Output {num} for this Object.
  - IT: Silenziare l'uscita {num} per questo oggetto.
  - [ ] OK    Fix: 

- **`muteMacrosSelector`**
  - EN: Mute Macros for Fast Muting and Unmuting of Arrays.
  - IT: Macro di mute per silenziare e riattivare rapidamente gli array.
  - [ ] OK    Fix: 

- **`muteReverbSendsButton`**
  - EN: Mute sends from this input to all reverb channels.
  - IT: Silenziare gli invii da questo ingresso a tutti i canali di riverbero.
  - [ ] OK    Fix: 

- **`nameEditor`**
  - EN: Displayed Input Channel Name (editable).
  - IT: Nome visualizzato del canale di ingresso (modificabile).
  - [ ] OK    Fix: 

- **`nudgeHint1`**
  - EN:  Nudge with Left and Right Arrow Keys.
  - IT:  Regolare con i tasti Sinistra e Destra.
  - [ ] OK    Fix: 

- **`nudgeHint2`**
  - EN:  Nudge with Up and Down Arrow Keys.
  - IT:  Regolare con i tasti Su e Giù.
  - [ ] OK    Fix: 

- **`nudgeHint3`**
  - EN:  Nudge with Page Up and Page Down Keys.
  - IT:  Regolare con Pagina Su e Pagina Giù.
  - [ ] OK    Fix: 

- **`offset1`**
  - EN: Object Offset {name} ({unit}). Adjusted when Tracking is Enabled.
  - IT: Offset {name} dell'oggetto ({unit}). Regolato quando il tracciamento è attivato.
  - [ ] OK    Fix: 

- **`offset2`**
  - EN: Object Offset {name} ({unit}). Adjusted when Tracking is Enabled.
  - IT: Offset {name} dell'oggetto ({unit}). Regolato quando il tracciamento è attivato.
  - [ ] OK    Fix: 

- **`offset3`**
  - EN: Object Offset {name} ({unit}). Adjusted when Tracking is Enabled.
  - IT: Offset {name} dell'oggetto ({unit}). Regolato quando il tracciamento è attivato.
  - [ ] OK    Fix: 

- **`otomoAbsRelButton`**
  - EN: Select Relative or Absolute Coordinates of Displacement.
  - IT: Scegliere coordinate di spostamento relative o assolute.
  - [ ] OK    Fix: 

- **`otomoCoordMode`**
  - EN: Coordinate display mode for AutomOtion destinations: Cartesian (X/Y/Z), Cylindrical (r/θ/Z), or Spherical (r/θ/φ).
  - IT: Modalità coordinate per le destinazioni AutomOtion: Cartesiano (X/Y/Z), Cilindrico (r/θ/Z) o Sferico (r/θ/φ).
  - [ ] OK    Fix: 

- **`otomoCurveDial`**
  - EN: Bend the Path to the Left (Negative) or Right (Positive) of the Direction of Travel.
  - IT: Curvare il percorso a sinistra (negativo) o a destra (positivo) della direzione di movimento.
  - [ ] OK    Fix: 

- **`otomoDest1`**
  - EN: Relative or Absolute Destination {name} ({unit}).
  - IT: Destinazione relativa o assoluta {name} ({unit}).
  - [ ] OK    Fix: 

- **`otomoDest2`**
  - EN: Relative or Absolute Destination {name} ({unit}).
  - IT: Destinazione relativa o assoluta {name} ({unit}).
  - [ ] OK    Fix: 

- **`otomoDest3`**
  - EN: Relative or Absolute Destination {name} ({unit}).
  - IT: Destinazione relativa o assoluta {name} ({unit}).
  - [ ] OK    Fix: 

- **`otomoDurationDial`**
  - EN: Duration of the Movement in Seconds (0.1s to 1 hour).
  - IT: Durata del movimento in secondi (0,1 s a 1 ora).
  - [ ] OK    Fix: 

- **`otomoPauseButton`**
  - EN: Pause and Resume the Movement.
  - IT: Mettere in pausa e riprendere il movimento.
  - [ ] OK    Fix: 

- **`otomoPauseResumeAllButton`**
  - EN: Pause or Resume All Active Movements Globally.
  - IT: Mettere in pausa o riprendere globalmente tutti i movimenti attivi.
  - [ ] OK    Fix: 

- **`otomoResetDial`**
  - EN: Set the Reset Level for the Automatic Trigger.
  - IT: Impostare il livello di reset per il trigger automatico.
  - [ ] OK    Fix: 

- **`otomoSpeedProfileDial`**
  - EN: Constant Speed or Gradual Acceleration and Slow Down at the Start and the End of the Movement.
  - IT: Velocità costante o accelerazione e decelerazione graduali all'inizio e alla fine del movimento.
  - [ ] OK    Fix: 

- **`otomoStartButton`**
  - EN: Start the Movement Manually.
  - IT: Avviare il movimento manualmente.
  - [ ] OK    Fix: 

- **`otomoStayReturnButton`**
  - EN: At the End of the Movement, should the Source Stay or Return to the Original Position.
  - IT: Alla fine del movimento, la sorgente rimane o ritorna alla posizione originale.
  - [ ] OK    Fix: 

- **`otomoStopAllButton`**
  - EN: Stop All Active Movements Globally.
  - IT: Fermare globalmente tutti i movimenti attivi.
  - [ ] OK    Fix: 

- **`otomoStopButton`**
  - EN: Stop the Movement.
  - IT: Fermare il movimento.
  - [ ] OK    Fix: 

- **`otomoThresholdDial`**
  - EN: Set the Threshold for the Automatic Trigger of the Movement.
  - IT: Impostare la soglia per il trigger automatico del movimento.
  - [ ] OK    Fix: 

- **`otomoTriggerButton`**
  - EN: Manual Start of Displacement or Automatic Trigger on the Audio Level.
  - IT: Avvio manuale dello spostamento o trigger automatico sul livello audio.
  - [ ] OK    Fix: 

- **`pathModeButton`**
  - EN: Enable Path Mode to Follow Drawn Movement Paths Instead of Direct Lines.
  - IT: Attivare la modalità Percorso per seguire i tracciati di movimento disegnati anziché linee dirette.
  - [ ] OK    Fix: 

- **`position1`**
  - EN: Object {name} ({unit}).
  - IT: {name} dell'oggetto ({unit}).
  - [ ] OK    Fix: 

- **`position2`**
  - EN: Object {name} ({unit}).
  - IT: {name} dell'oggetto ({unit}).
  - [ ] OK    Fix: 

- **`position3`**
  - EN: Object {name} ({unit}).
  - IT: {name} dell'oggetto ({unit}).
  - [ ] OK    Fix: 

- **`positionJoystick`**
  - EN: Drag to adjust X/Y position in real-time. Returns to center on release.
  - IT: Trascinare per regolare la posizione X/Y in tempo reale. Torna al centro al rilascio.
  - [ ] OK    Fix: 

- **`positionZSlider`**
  - EN: Drag to adjust Z (height) position in real-time. Returns to center on release.
  - IT: Trascinare per regolare la posizione Z (altezza) in tempo reale. Torna al centro al rilascio.
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Input Configuration from backup file.
  - IT: Ricarica la configurazione di ingresso dal file di backup.
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Input Configuration from file.
  - IT: Ricarica la configurazione di ingresso da file.
  - [ ] OK    Fix: 

- **`reloadSnapshot`**
  - EN: Reload Selected Input Snapshot for All Objects Taking the Scope into Account.
  - IT: Ricarica lo snapshot di ingresso selezionato per tutti gli oggetti tenendo conto del filtro.
  - [ ] OK    Fix: 

- **`reloadWithoutScope`**
  - EN: Reload Selected Input Snapshot for All Objects Without the Scope.
  - IT: Ricarica lo snapshot di ingresso selezionato per tutti gli oggetti senza il filtro.
  - [ ] OK    Fix: 

- **`sidelinesActiveButton`**
  - EN: Enable Automatic Muting when Source Approaches Stage Edges. Does Not Apply to Downstage (Front) Edge.
  - IT: Attivare il mute automatico quando la sorgente si avvicina ai bordi del palco. Non si applica al bordo anteriore (lato pubblico).
  - [ ] OK    Fix: 

- **`sidelinesFringeDial`**
  - EN: Fringe Zone Size in Meters. Outer Half is Full Mute, Inner Half Fades Linearly.
  - IT: Dimensione della zona di transizione in metri. La metà esterna silenzia completamente, la metà interna attenua linearmente.
  - [ ] OK    Fix: 

- **`snapshotSelector`**
  - EN: Select Input Snapshot Without Loading.
  - IT: Seleziona lo snapshot di ingresso senza caricarlo.
  - [ ] OK    Fix: 

- **`solo`**
  - EN: Listen to Binaural Rendering of this channel.
  - IT: Ascoltare il rendering binaurale di questo canale.
  - [ ] OK    Fix: 

- **`soloMode`**
  - EN: Single: one input at a time. Multi: multiple inputs simultaneously.
  - IT: Singolo: un ingresso alla volta. Multi: più ingressi simultaneamente.
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Input Configuration to file (with backup).
  - IT: Salva la configurazione di ingresso su file (con backup).
  - [ ] OK    Fix: 

- **`storeSnapshot`**
  - EN: Store new Input Snapshot for All Objects.
  - IT: Salva un nuovo snapshot di ingresso per tutti gli oggetti.
  - [ ] OK    Fix: 

- **`tiltSlider`**
  - EN: Where is the Object pointing to in the Vertical Plane.
  - IT: Direzione dell'oggetto sul piano verticale.
  - [ ] OK    Fix: 

- **`trackingActiveButton`**
  - EN: Enable or Disable Tracking for Object.
  - IT: Attivare o disattivare il tracking per l'oggetto.
  - [ ] OK    Fix: 

- **`trackingIdSelector`**
  - EN: Tracker ID for Object.
  - IT: ID del tracker per l'oggetto.
  - [ ] OK    Fix: 

- **`trackingSmoothDial`**
  - EN: Smoothing of Tracking Data for Object.
  - IT: Smussamento dei dati di tracking per l'oggetto.
  - [ ] OK    Fix: 

- **`updateSnapshot`**
  - EN: Update Selected Input Snapshot (with backup).
  - IT: Aggiorna lo snapshot di ingresso selezionato (con backup).
  - [ ] OK    Fix: 

## `inputs.labels`

- **`admMapping`**
  - EN: ADM:
  - IT: ADM:
  - [ ] OK    Fix: 

- **`amplitudeX`**
  - EN: Amplitude X:
  - IT: Ampiezza X:
  - [ ] OK    Fix: 

- **`amplitudeY`**
  - EN: Amplitude Y:
  - IT: Ampiezza Y:
  - [ ] OK    Fix: 

- **`amplitudeZ`**
  - EN: Amplitude Z:
  - IT: Ampiezza Z:
  - [ ] OK    Fix: 

- **`arrayAttenuation`**
  - EN: Array Attenuation:
  - IT: Attenuazione Array:
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Attenuation:
  - IT: Attenuazione:
  - [ ] OK    Fix: 

- **`attenuationLaw`**
  - EN: Attenuation Law:
  - IT: Legge Attenuazione:
  - [ ] OK    Fix: 

- **`cluster`**
  - EN: Cluster:
  - IT: Cluster:
  - [ ] OK    Fix: 

- **`commonAtten`**
  - EN: Common Atten:
  - IT: Atten Comune:
  - [ ] OK    Fix: 

- **`coord`**
  - EN: Coord:
  - IT: Coord:
  - [ ] OK    Fix: 

- **`curve`**
  - EN: Curve:
  - IT: Curva:
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Delay/Latency:
  - IT: Ritardo/Latenza:
  - [ ] OK    Fix: 

- **`destX`**
  - EN: Dest. X:
  - IT: Dest. X:
  - [ ] OK    Fix: 

- **`destY`**
  - EN: Dest. Y:
  - IT: Dest. Y:
  - [ ] OK    Fix: 

- **`destZ`**
  - EN: Dest. Z:
  - IT: Dest. Z:
  - [ ] OK    Fix: 

- **`diffusion`**
  - EN: Diffusion:
  - IT: Diffusione:
  - [ ] OK    Fix: 

- **`directivity`**
  - EN: Directivity:
  - IT: Direttività:
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Distance Atten:
  - IT: Atten Distanza:
  - [ ] OK    Fix: 

- **`distanceRatio`**
  - EN: Distance Ratio:
  - IT: Rapporto Distanza:
  - [ ] OK    Fix: 

- **`duration`**
  - EN: Duration:
  - IT: Durata:
  - [ ] OK    Fix: 

- **`frequency`**
  - EN: Frequency:
  - IT: Frequenza:
  - [ ] OK    Fix: 

- **`fringe`**
  - EN: Fringe:
  - IT: Frangia:
  - [ ] OK    Fix: 

- **`gain`**
  - EN: Gain:
  - IT: Guadagno:
  - [ ] OK    Fix: 

- **`gyrophone`**
  - EN: Gyrophone:
  - IT: Girofono:
  - [ ] OK    Fix: 

- **`heightFactor`**
  - EN: Height Factor:
  - IT: Fattore Altezza:
  - [ ] OK    Fix: 

- **`hfShelf`**
  - EN: HF Shelf:
  - IT: Shelf HF:
  - [ ] OK    Fix: 

- **`jitter`**
  - EN: Jitter:
  - IT: Jitter:
  - [ ] OK    Fix: 

- **`max`**
  - EN: Max:
  - IT: Max:
  - [ ] OK    Fix: 

- **`maxSpeed`**
  - EN: Max Speed:
  - IT: Velocità Max:
  - [ ] OK    Fix: 

- **`min`**
  - EN: Min:
  - IT: Min:
  - [ ] OK    Fix: 

- **`muteMacros`**
  - EN: Mute Macros:
  - IT: Macro Mute:
  - [ ] OK    Fix: 

- **`name`**
  - EN: Name:
  - IT: Nome:
  - [ ] OK    Fix: 

- **`offsetX`**
  - EN: Offset X:
  - IT: Offset X:
  - [ ] OK    Fix: 

- **`offsetY`**
  - EN: Offset Y:
  - IT: Offset Y:
  - [ ] OK    Fix: 

- **`offsetZ`**
  - EN: Offset Z:
  - IT: Offset Z:
  - [ ] OK    Fix: 

- **`outX`**
  - EN: Out X:
  - IT: Out X:
  - [ ] OK    Fix: 

- **`outY`**
  - EN: Out Y:
  - IT: Out Y:
  - [ ] OK    Fix: 

- **`outZ`**
  - EN: Out Z:
  - IT: Out Z:
  - [ ] OK    Fix: 

- **`peakRatio`**
  - EN: Peak Ratio:
  - IT: Rapporto Picco:
  - [ ] OK    Fix: 

- **`peakThreshold`**
  - EN: Peak Threshold:
  - IT: Soglia Picco:
  - [ ] OK    Fix: 

- **`period`**
  - EN: Period:
  - IT: Periodo:
  - [ ] OK    Fix: 

- **`phase`**
  - EN: Phase:
  - IT: Fase:
  - [ ] OK    Fix: 

- **`phaseX`**
  - EN: Phase X:
  - IT: Fase X:
  - [ ] OK    Fix: 

- **`phaseY`**
  - EN: Phase Y:
  - IT: Fase Y:
  - [ ] OK    Fix: 

- **`phaseZ`**
  - EN: Phase Z:
  - IT: Fase Z:
  - [ ] OK    Fix: 

- **`positionX`**
  - EN: Position X:
  - IT: Posizione X:
  - [ ] OK    Fix: 

- **`positionY`**
  - EN: Position Y:
  - IT: Posizione Y:
  - [ ] OK    Fix: 

- **`positionZ`**
  - EN: Position Z:
  - IT: Posizione Z:
  - [ ] OK    Fix: 

- **`radius`**
  - EN: Radius:
  - IT: Raggio:
  - [ ] OK    Fix: 

- **`rateX`**
  - EN: Rate X:
  - IT: Velocità X:
  - [ ] OK    Fix: 

- **`rateY`**
  - EN: Rate Y:
  - IT: Velocità Y:
  - [ ] OK    Fix: 

- **`rateZ`**
  - EN: Rate Z:
  - IT: Velocità Z:
  - [ ] OK    Fix: 

- **`reset`**
  - EN: Reset:
  - IT: Reset:
  - [ ] OK    Fix: 

- **`rotation`**
  - EN: Rotation:
  - IT: Rotazione:
  - [ ] OK    Fix: 

- **`shape`**
  - EN: Shape:
  - IT: Forma:
  - [ ] OK    Fix: 

- **`shapeX`**
  - EN: Shape X:
  - IT: Forma X:
  - [ ] OK    Fix: 

- **`shapeY`**
  - EN: Shape Y:
  - IT: Forma Y:
  - [ ] OK    Fix: 

- **`shapeZ`**
  - EN: Shape Z:
  - IT: Forma Z:
  - [ ] OK    Fix: 

- **`slope`**
  - EN: Slope:
  - IT: Pendenza:
  - [ ] OK    Fix: 

- **`slowRatio`**
  - EN: Slow Ratio:
  - IT: Rapporto Lento:
  - [ ] OK    Fix: 

- **`slowThreshold`**
  - EN: Slow Threshold:
  - IT: Soglia Lento:
  - [ ] OK    Fix: 

- **`speedProfile`**
  - EN: Speed Profile:
  - IT: Profilo Velocità:
  - [ ] OK    Fix: 

- **`threshold`**
  - EN: Threshold:
  - IT: Soglia:
  - [ ] OK    Fix: 

- **`tilt`**
  - EN: Tilt:
  - IT: Inclinazione:
  - [ ] OK    Fix: 

- **`trackingId`**
  - EN: Tracking ID:
  - IT: ID Tracking:
  - [ ] OK    Fix: 

- **`trackingSmooth`**
  - EN: Tracking Smooth:
  - IT: Smooth Tracking:
  - [ ] OK    Fix: 

- **`xyJoystick`**
  - EN: X/Y
  - IT: X/Y
  - [ ] OK    Fix: 

- **`zSlider`**
  - EN: Z
  - IT: Z
  - [ ] OK    Fix: 

## `inputs.lfo`

- **`gyrophone.antiClockwise`**
  - EN: Anti-Clockwise
  - IT: Antiorario
  - [ ] OK    Fix: 

- **`gyrophone.clockwise`**
  - EN: Clockwise
  - IT: Orario
  - [ ] OK    Fix: 

- **`gyrophone.off`**
  - EN: OFF
  - IT: OFF
  - [ ] OK    Fix: 

- **`shapes.exp`**
  - EN: exp
  - IT: exp
  - [ ] OK    Fix: 

- **`shapes.keystone`**
  - EN: keystone
  - IT: trapezio
  - [ ] OK    Fix: 

- **`shapes.log`**
  - EN: log
  - IT: log
  - [ ] OK    Fix: 

- **`shapes.off`**
  - EN: OFF
  - IT: OFF
  - [ ] OK    Fix: 

- **`shapes.random`**
  - EN: random
  - IT: casuale
  - [ ] OK    Fix: 

- **`shapes.sawtooth`**
  - EN: sawtooth
  - IT: dente di sega
  - [ ] OK    Fix: 

- **`shapes.sine`**
  - EN: sine
  - IT: sinusoide
  - [ ] OK    Fix: 

- **`shapes.square`**
  - EN: square
  - IT: quadra
  - [ ] OK    Fix: 

- **`shapes.triangle`**
  - EN: triangle
  - IT: triangolare
  - [ ] OK    Fix: 

## `inputs.liveSource`

- **`linear`**
  - EN: linear
  - IT: lineare
  - [ ] OK    Fix: 

- **`log`**
  - EN: log
  - IT: log
  - [ ] OK    Fix: 

- **`sine`**
  - EN: sine
  - IT: sinusoide
  - [ ] OK    Fix: 

## `inputs.messages`

- **`assignedCluster`**
  - EN: Input {channel} assigned to Cluster {cluster}
  - IT: Ingresso {channel} assegnato al Cluster {cluster}
  - [ ] OK    Fix: 

- **`backupLoaded`**
  - EN: Input configuration loaded from backup.
  - IT: Configurazione ingressi caricata dal backup.
  - [ ] OK    Fix: 

- **`configExported`**
  - EN: Input configuration exported.
  - IT: Configurazione ingressi esportata.
  - [ ] OK    Fix: 

- **`configImported`**
  - EN: Input configuration imported.
  - IT: Configurazione ingressi importata.
  - [ ] OK    Fix: 

- **`configLoaded`**
  - EN: Input configuration loaded.
  - IT: Configurazione ingressi caricata.
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Input configuration saved.
  - IT: Configurazione ingressi salvata.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - IT: Errore: {error}
  - [ ] OK    Fix: 

- **`noSnapshotSelected`**
  - EN: No snapshot selected.
  - IT: Nessun snapshot selezionato.
  - [ ] OK    Fix: 

- **`scopeConfigured`**
  - EN: Scope configured for next snapshot.
  - IT: Ambito configurato per il prossimo snapshot.
  - [ ] OK    Fix: 

- **`scopeSaved`**
  - EN: Snapshot scope saved.
  - IT: Ambito snapshot salvato.
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - IT: Seleziona prima una cartella progetto in Config Sistema.
  - [ ] OK    Fix: 

- **`setSingle`**
  - EN: Input {channel} set to Single
  - IT: Ingresso {channel} impostato su Singolo
  - [ ] OK    Fix: 

- **`snapshotDeleted`**
  - EN: Snapshot '{name}' deleted.
  - IT: Snapshot '{name}' eliminato.
  - [ ] OK    Fix: 

- **`snapshotLoaded`**
  - EN: Snapshot '{name}' loaded.
  - IT: Snapshot '{name}' caricato.
  - [ ] OK    Fix: 

- **`snapshotLoadedWithoutScope`**
  - EN: Snapshot '{name}' loaded (without scope).
  - IT: Snapshot '{name}' caricato (senza ambito).
  - [ ] OK    Fix: 

- **`snapshotStored`**
  - EN: Snapshot '{name}' stored.
  - IT: Snapshot '{name}' memorizzato.
  - [ ] OK    Fix: 

- **`snapshotUpdated`**
  - EN: Snapshot '{name}' updated.
  - IT: Snapshot '{name}' aggiornato.
  - [ ] OK    Fix: 

- **`trackingDisabled`**
  - EN: Tracking disabled for Input {channel}
  - IT: Tracking disabilitato per Ingresso {channel}
  - [ ] OK    Fix: 

- **`trackingSwitched`**
  - EN: Tracking switched from Input {from} to Input {to}
  - IT: Tracking trasferito da Ingresso {from} a Ingresso {to}
  - [ ] OK    Fix: 

## `inputs.muteMacros`

- **`invertMutes`**
  - EN: INVERT MUTES
  - IT: INVERTI MUTE
  - [ ] OK    Fix: 

- **`muteAll`**
  - EN: MUTE ALL
  - IT: MUTA TUTTI
  - [ ] OK    Fix: 

- **`muteArrayPrefix`**
  - EN: MUTE ARRAY
  - IT: MUTA ARRAY
  - [ ] OK    Fix: 

- **`muteEven`**
  - EN: MUTE EVEN
  - IT: MUTA PARI
  - [ ] OK    Fix: 

- **`muteOdd`**
  - EN: MUTE ODD
  - IT: MUTA DISPARI
  - [ ] OK    Fix: 

- **`selectMacro`**
  - EN: Select Macro...
  - IT: Seleziona Macro...
  - [ ] OK    Fix: 

- **`unmuteAll`**
  - EN: UNMUTE ALL
  - IT: SMUTA TUTTI
  - [ ] OK    Fix: 

- **`unmuteArrayPrefix`**
  - EN: UNMUTE ARRAY
  - IT: SMUTA ARRAY
  - [ ] OK    Fix: 

## `inputs.prefixes`

- **`delay`**
  - EN: Delay:
  - IT: Ritardo:
  - [ ] OK    Fix: 

## `inputs.sections`

- **`automotion`**
  - EN: AutomOtion
  - IT: AutomOzione
  - [ ] OK    Fix: 

## `inputs.snapshots`

- **`selectSnapshot`**
  - EN: Select Snapshot...
  - IT: Seleziona Snapshot...
  - [ ] OK    Fix: 

## `inputs.tabs`

- **`gradientMaps`**
  - EN: Gradient Maps
  - IT: Mappe di Gradiente
  - [ ] OK    Fix: 

- **`inputParams`**
  - EN: Input Parameters
  - IT: Parametri Ingresso
  - [ ] OK    Fix: 

- **`liveSourceHackoustics`**
  - EN: Live Source & Hackoustics
  - IT: Sorgente Live & Hackustica
  - [ ] OK    Fix: 

- **`movements`**
  - EN: Movements
  - IT: Movimenti
  - [ ] OK    Fix: 

- **`sampler`**
  - EN: Sampler
  - IT: Sampler
  - [ ] OK    Fix: 

- **`visualisation`**
  - EN: Visualisation
  - IT: Visualizzazione
  - [ ] OK    Fix: 

## `inputs.toggles`

- **`absolute`**
  - EN: Absolute
  - IT: Assoluto
  - [ ] OK    Fix: 

- **`acousticPrecedence`**
  - EN: Acoustic Precedence
  - IT: Precedenza Acustica
  - [ ] OK    Fix: 

- **`attenuationLawLog`**
  - EN: Log
  - IT: Log
  - [ ] OK    Fix: 

- **`constraintROff`**
  - EN: Constraint R: OFF
  - IT: Vincolo R: OFF
  - [ ] OK    Fix: 

- **`constraintROn`**
  - EN: Constraint R: ON
  - IT: Vincolo R: ON
  - [ ] OK    Fix: 

- **`constraintXOff`**
  - EN: Constraint X: OFF
  - IT: Vincolo X: OFF
  - [ ] OK    Fix: 

- **`constraintXOn`**
  - EN: Constraint X: ON
  - IT: Vincolo X: ON
  - [ ] OK    Fix: 

- **`constraintYOff`**
  - EN: Constraint Y: OFF
  - IT: Vincolo Y: OFF
  - [ ] OK    Fix: 

- **`constraintYOn`**
  - EN: Constraint Y: ON
  - IT: Vincolo Y: ON
  - [ ] OK    Fix: 

- **`constraintZOff`**
  - EN: Constraint Z: OFF
  - IT: Vincolo Z: OFF
  - [ ] OK    Fix: 

- **`constraintZOn`**
  - EN: Constraint Z: ON
  - IT: Vincolo Z: ON
  - [ ] OK    Fix: 

- **`flipXOff`**
  - EN: Flip X: OFF
  - IT: Inverti X: OFF
  - [ ] OK    Fix: 

- **`flipXOn`**
  - EN: Flip X: ON
  - IT: Inverti X: ON
  - [ ] OK    Fix: 

- **`flipYOff`**
  - EN: Flip Y: OFF
  - IT: Inverti Y: OFF
  - [ ] OK    Fix: 

- **`flipYOn`**
  - EN: Flip Y: ON
  - IT: Inverti Y: ON
  - [ ] OK    Fix: 

- **`flipZOff`**
  - EN: Flip Z: OFF
  - IT: Inverti Z: OFF
  - [ ] OK    Fix: 

- **`flipZOn`**
  - EN: Flip Z: ON
  - IT: Inverti Z: ON
  - [ ] OK    Fix: 

- **`floorReflectionsOff`**
  - EN: Floor Reflections: OFF
  - IT: Riflessioni Pavimento: OFF
  - [ ] OK    Fix: 

- **`floorReflectionsOn`**
  - EN: Floor Reflections: ON
  - IT: Riflessioni Pavimento: ON
  - [ ] OK    Fix: 

- **`highShelfOff`**
  - EN: High Shelf: OFF
  - IT: Shelf Alti: OFF
  - [ ] OK    Fix: 

- **`highShelfOn`**
  - EN: High Shelf: ON
  - IT: Shelf Alti: ON
  - [ ] OK    Fix: 

- **`lfoOff`**
  - EN: L.F.O: OFF
  - IT: L.F.O: OFF
  - [ ] OK    Fix: 

- **`lfoOn`**
  - EN: L.F.O: ON
  - IT: L.F.O: ON
  - [ ] OK    Fix: 

- **`liveSourceTamerOff`**
  - EN: Live Source Tamer: OFF
  - IT: Domatore: OFF
  - [ ] OK    Fix: 

- **`liveSourceTamerOn`**
  - EN: Live Source Tamer: ON
  - IT: Domatore: ON
  - [ ] OK    Fix: 

- **`lowCutOff`**
  - EN: Low Cut: OFF
  - IT: Taglia Bassi: OFF
  - [ ] OK    Fix: 

- **`lowCutOn`**
  - EN: Low Cut: ON
  - IT: Taglia Bassi: ON
  - [ ] OK    Fix: 

- **`lsPeakOff`**
  - EN: Peak: OFF
  - IT: Picco: OFF
  - [ ] OK    Fix: 

- **`lsPeakOn`**
  - EN: Peak: ON
  - IT: Picco: ON
  - [ ] OK    Fix: 

- **`lsSlowOff`**
  - EN: Slow: OFF
  - IT: Lento: OFF
  - [ ] OK    Fix: 

- **`lsSlowOn`**
  - EN: Slow: ON
  - IT: Lento: ON
  - [ ] OK    Fix: 

- **`manual`**
  - EN: Manual
  - IT: Manuale
  - [ ] OK    Fix: 

- **`maxSpeedOff`**
  - EN: Max Speed: OFF
  - IT: Velocità Max: OFF
  - [ ] OK    Fix: 

- **`maxSpeedOn`**
  - EN: Max Speed: ON
  - IT: Velocità Max: ON
  - [ ] OK    Fix: 

- **`minimalLatency`**
  - EN: Minimal Latency
  - IT: Latenza Minima
  - [ ] OK    Fix: 

- **`pathModeOff`**
  - EN: Path Mode: OFF
  - IT: Modo Percorso: OFF
  - [ ] OK    Fix: 

- **`pathModeOn`**
  - EN: Path Mode: ON
  - IT: Modo Percorso: ON
  - [ ] OK    Fix: 

- **`relative`**
  - EN: Relative
  - IT: Relativo
  - [ ] OK    Fix: 

- **`return`**
  - EN: Return
  - IT: Ritorna
  - [ ] OK    Fix: 

- **`reverbSendsMuted`**
  - EN: Sends to Reverbs: Muted
  - IT: Mandate Riverbero: Mute
  - [ ] OK    Fix: 

- **`reverbSendsUnmuted`**
  - EN: Sends to Reverbs: Unmuted
  - IT: Mandate Riverbero: Attive
  - [ ] OK    Fix: 

- **`sidelinesOff`**
  - EN: Sidelines Off
  - IT: Linee laterali Off
  - [ ] OK    Fix: 

- **`sidelinesOn`**
  - EN: Sidelines On
  - IT: Sidelines On
  - [ ] OK    Fix: 

- **`stay`**
  - EN: Stay
  - IT: Resta
  - [ ] OK    Fix: 

- **`trackingOff`**
  - EN: Tracking: OFF
  - IT: Tracking: OFF
  - [ ] OK    Fix: 

- **`trackingOn`**
  - EN: Tracking: ON
  - IT: Tracking: ON
  - [ ] OK    Fix: 

- **`triggered`**
  - EN: Triggered
  - IT: Attivato
  - [ ] OK    Fix: 

## `inputs.visualisation`

- **`delay`**
  - EN: delay
  - IT: ritardo
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: HF
damping
  - IT: attenuazione
HF
  - [ ] OK    Fix: 

- **`level`**
  - EN: level
  - IT: livello
  - [ ] OK    Fix: 

## `levelMeter`

- **`inputs`**
  - EN: Inputs
  - IT: Ingressi
  - [ ] OK    Fix: 

- **`outputs`**
  - EN: Outputs
  - IT: Uscite
  - [ ] OK    Fix: 

- **`windowTitle`**
  - EN: Level Meters
  - IT: Indicatori di livello
  - [ ] OK    Fix: 

## `levelMeter.buttons`

- **`clearSolo`**
  - EN: Clear Solo
  - IT: Cancella solo
  - [ ] OK    Fix: 

- **`soloModeMulti`**
  - EN: Multi
  - IT: Multi
  - [ ] OK    Fix: 

- **`soloModeSingle`**
  - EN: Single
  - IT: Single
  - [ ] OK    Fix: 

## `levelMeter.tooltips`

- **`clearSolo`**
  - EN: Disengage all solo toggles
  - IT: Disattiva tutti i solo
  - [ ] OK    Fix: 

- **`solo`**
  - EN: Display the contribution of the input to all outputs in the Level Meter display (in Single mode) and play binaural render of soloed inputs
  - IT: Mostra il contributo dell'ingresso a tutte le uscite nell'indicatore di livello (in modalità Solo singolo) e riproduce il rendering binaurale degli ingressi in solo
  - [ ] OK    Fix: 

- **`soloMode`**
  - EN: Single: one input at a time. Multi: multiple inputs simultaneously.
  - IT: Singolo: un ingresso alla volta. Multi: più ingressi simultaneamente.
  - [ ] OK    Fix: 

## `map`

- **`detachedMessage`**
  - EN: The map is displayed in a separate window.
  - IT: La mappa è visualizzata in una finestra separata.
  - [ ] OK    Fix: 

- **`reattach`**
  - EN: Re-attach Map
  - IT: Riattacca mappa
  - [ ] OK    Fix: 

## `map.buttons`

- **`fitInputs`**
  - EN: Fit All Inputs to Screen
  - IT: Adatta tutti gli ingressi allo schermo
  - [ ] OK    Fix: 

- **`fitStage`**
  - EN: Fit Stage to Screen
  - IT: Adatta palco allo schermo
  - [ ] OK    Fix: 

- **`hideLevels`**
  - EN: Hide Levels
  - IT: Nascondi Livelli
  - [ ] OK    Fix: 

- **`showLevels`**
  - EN: Show Levels
  - IT: Mostra Livelli
  - [ ] OK    Fix: 

## `map.labels`

- **`reverbPrefix`**
  - EN: R
  - IT: R
  - [ ] OK    Fix: 

## `map.messages`

- **`assignedCluster`**
  - EN: Input {channel} assigned to Cluster {cluster}
  - IT: Ingresso {channel} assegnato al Cluster {cluster}
  - [ ] OK    Fix: 

- **`assignedClusterMulti`**
  - EN: {count} inputs assigned to Cluster {cluster}
  - IT: {count} ingressi assegnati al Cluster {cluster}
  - [ ] OK    Fix: 

- **`clusterBrokenUp`**
  - EN: Cluster {cluster} broken up
  - IT: Cluster {cluster} sciolto
  - [ ] OK    Fix: 

- **`setSingle`**
  - EN: Input {channel} removed from cluster
  - IT: Ingresso {channel} rimosso dal cluster
  - [ ] OK    Fix: 

- **`setSingleMulti`**
  - EN: {count} inputs removed from clusters
  - IT: {count} ingressi rimossi dai cluster
  - [ ] OK    Fix: 

## `map.tooltips`

- **`detach`**
  - EN: Detach the map into a separate window for dual-screen setups
  - IT: Stacca la mappa in una finestra separata per configurazioni a doppio schermo
  - [ ] OK    Fix: 

- **`fitInputs`**
  - EN: Adjust the zoom and panning to fit all visible inputs on the display
  - IT: Regola zoom e panoramica per mostrare tutti gli ingressi visibili
  - [ ] OK    Fix: 

- **`fitStage`**
  - EN: Adjust the zoom and panning to fit the stage on the display
  - IT: Regola zoom e panoramica per mostrare il palco
  - [ ] OK    Fix: 

- **`levels`**
  - EN: Display the levels for inputs and outputs on the map
  - IT: Mostra i livelli di ingressi e uscite sulla mappa
  - [ ] OK    Fix: 

## `network.buttons`

- **`add`**
  - EN: ADD
  - IT: AGGIUNGI
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - IT: Esporta
  - [ ] OK    Fix: 

- **`findMyRemote`**
  - EN: Find My Remote
  - IT: Trova il Mio Remote
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - IT: Importa
  - [ ] OK    Fix: 

- **`openLogWindow`**
  - EN: Open Log Window
  - IT: Apri Finestra Log
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Backup
  - IT: Ricarica Backup
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Network Config
  - IT: Ricarica Config Rete
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Network Config
  - IT: Memorizza Config Rete
  - [ ] OK    Fix: 

## `network.dialogs`

- **`exportConfig`**
  - EN: Export Network Configuration
  - IT: Esporta Configurazione di Rete
  - [ ] OK    Fix: 

- **`findMyRemoteMessage`**
  - EN: Enter the password for your remote device:
  - IT: Inserisci la password per il tuo telecomando:
  - [ ] OK    Fix: 

- **`findMyRemotePassword`**
  - EN: Password:
  - IT: Password telecomando:
  - [ ] OK    Fix: 

- **`findMyRemoteTitle`**
  - EN: Find My Remote
  - IT: Trova il mio telecomando
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Network Configuration
  - IT: Importa Configurazione di Rete
  - [ ] OK    Fix: 

- **`removeTargetMessage`**
  - EN: Delete target '{name}'?
  - IT: Eliminare la destinazione '{name}'?
  - [ ] OK    Fix: 

- **`removeTargetTitle`**
  - EN: Remove Target
  - IT: Rimuovi destinazione
  - [ ] OK    Fix: 

- **`trackingConflictsContinue`**
  - EN: Continue
  - IT: Continua
  - [ ] OK    Fix: 

- **`trackingConflictsMessage`**
  - EN: 
Only one tracked input per cluster is allowed. If you continue, tracking will be kept only for the first input in each cluster.
  - IT: 
È consentito solo un ingresso tracciato per cluster. Se continui, il tracking verrà mantenuto solo per il primo ingresso di ciascun cluster.
  - [ ] OK    Fix: 

- **`trackingConflictsTitle`**
  - EN: Tracking Conflicts Detected
  - IT: Conflitti di tracking rilevati
  - [ ] OK    Fix: 

## `network.help`

- **`addTarget`**
  - EN: Add new network target.
  - IT: Aggiungere una nuova destinazione di rete.
  - [ ] OK    Fix: 

- **`admMapping`**
  - EN: Select an ADM-OSC mapping to configure. Cart = Cartesian (xyz), Polar = spherical (aed).
  - IT: Selezionare una mappatura ADM-OSC da configurare. Cart = Cartesiano (xyz), Polar = sferico (aed).
  - [ ] OK    Fix: 

- **`admMappingPanel`**
  - EN: Drag dots to edit mapping. Click axis title to swap, click Flip to invert. Hold Shift to edit both sides symmetrically.
  - IT: Trascinare i punti per modificare la mappatura. Cliccare sul titolo dell'asse per scambiare, cliccare su Flip per invertire. Tenere premuto Maiusc per modificare entrambi i lati simmetricamente.
  - [ ] OK    Fix: 

- **`currentIP`**
  - EN: IP address of the Processor.
  - IT: Indirizzo IP del processore.
  - [ ] OK    Fix: 

- **`dataMode`**
  - EN: Select UDP or TCP data transmission.
  - IT: Selezionare la trasmissione dati UDP o TCP.
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Network Configuration to file.
  - IT: Esportare la configurazione di rete in un file.
  - [ ] OK    Fix: 

- **`findMyRemote`**
  - EN: Make your Remote Flash and Buzz to Find it.
  - IT: Far lampeggiare e vibrare il tuo telecomando per trovarlo.
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Network Configuration from file.
  - IT: Importare la configurazione di rete da un file.
  - [ ] OK    Fix: 

- **`networkInterface`**
  - EN: Select the Network Interface.
  - IT: Selezionare l'interfaccia di rete.
  - [ ] OK    Fix: 

- **`openLogWindow`**
  - EN: Open Network Logging window.
  - IT: Aprire la finestra di log di rete.
  - [ ] OK    Fix: 

- **`oscQueryEnable`**
  - EN: Enable/disable OSC Query server for automatic parameter discovery via HTTP/WebSocket.
  - IT: Attivare/disattivare il server OSC Query per la scoperta automatica dei parametri tramite HTTP/WebSocket.
  - [ ] OK    Fix: 

- **`oscQueryPort`**
  - EN: HTTP port for OSC Query discovery. Other apps can browse parameters at http://localhost:<port>/
  - IT: Porta HTTP per la scoperta OSC Query. Altre applicazioni possono sfogliare i parametri su http://localhost:<port>/
  - [ ] OK    Fix: 

- **`oscSourceFilter`**
  - EN: Filter incoming OSC: Accept All sources or only Registered connections with Rx enabled.
  - IT: Filtrare l'OSC in entrata: accettare tutte le sorgenti o solo le connessioni registrate con Rx attivo.
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Select the Protocol: DISABLED, OSC, REMOTE, or ADM-OSC.
  - IT: Selezionare il protocollo: DISABLED, OSC, REMOTE o ADM-OSC.
  - [ ] OK    Fix: 

- **`psnInterface`**
  - EN: Network interface for PSN multicast reception
  - IT: Interfaccia di rete per la ricezione multicast PSN
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Network Configuration from backup file.
  - IT: Ricaricare la configurazione di rete dal file di backup.
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Network Configuration from file.
  - IT: Ricaricare la configurazione di rete da un file.
  - [ ] OK    Fix: 

- **`removeTarget`**
  - EN: Delete this Network Target.
  - IT: Eliminare questa destinazione di rete.
  - [ ] OK    Fix: 

- **`rxEnable`**
  - EN: Enable or Disable Data Reception.
  - IT: Attivare o disattivare la ricezione dei dati.
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Network Configuration to file.
  - IT: Salvare la configurazione di rete in un file.
  - [ ] OK    Fix: 

- **`targetIP`**
  - EN: IP Address of the Target (use 127.0.0.1 for local host).
  - IT: Indirizzo IP della destinazione (usare 127.0.0.1 per l'host locale).
  - [ ] OK    Fix: 

- **`targetName`**
  - EN: Network Target Name.
  - IT: Nome della destinazione di rete.
  - [ ] OK    Fix: 

- **`tcpPort`**
  - EN: TCP Receive Port of the Processor.
  - IT: Porta di ricezione TCP del processore.
  - [ ] OK    Fix: 

- **`trackingEnabled`**
  - EN: Enable or Disable Incoming Tracking data processing.
  - IT: Attivare o disattivare l'elaborazione dei dati di tracking in entrata.
  - [ ] OK    Fix: 

- **`trackingFlipX`**
  - EN: Invert Axis of Tracking X Coordinate.
  - IT: Invertire l'asse della coordinata X del tracking.
  - [ ] OK    Fix: 

- **`trackingFlipY`**
  - EN: Invert Axis of Tracking Y Coordinate.
  - IT: Invertire l'asse della coordinata Y del tracking.
  - [ ] OK    Fix: 

- **`trackingFlipZ`**
  - EN: Invert Axis of Tracking Z Coordinate.
  - IT: Invertire l'asse della coordinata Z del tracking.
  - [ ] OK    Fix: 

- **`trackingOffsetX`**
  - EN: Offset Tracking X Coordinate.
  - IT: Offset della coordinata X del tracking.
  - [ ] OK    Fix: 

- **`trackingOffsetY`**
  - EN: Offset Tracking Y Coordinate.
  - IT: Offset della coordinata Y del tracking.
  - [ ] OK    Fix: 

- **`trackingOffsetZ`**
  - EN: Offset Tracking Z Coordinate.
  - IT: Offset della coordinata Z del tracking.
  - [ ] OK    Fix: 

- **`trackingOscPath`**
  - EN: OSC Path for the Tracking in OSC Mode (starts with a /)
  - IT: Percorso OSC per il tracking in modalità OSC (inizia con /)
  - [ ] OK    Fix: 

- **`trackingPort`**
  - EN: Specify the Port to receive Tracking data.
  - IT: Specificare la porta per ricevere i dati di tracking.
  - [ ] OK    Fix: 

- **`trackingProtocol`**
  - EN: Select the type of Tracking Protocol.
  - IT: Selezionare il tipo di protocollo di tracking.
  - [ ] OK    Fix: 

- **`trackingScaleX`**
  - EN: Scale Tracking X Coordinate.
  - IT: Scala della coordinata X del tracking.
  - [ ] OK    Fix: 

- **`trackingScaleY`**
  - EN: Scale Tracking Y Coordinate.
  - IT: Scala della coordinata Y del tracking.
  - [ ] OK    Fix: 

- **`trackingScaleZ`**
  - EN: Scale Tracking Z Coordinate.
  - IT: Scala della coordinata Z del tracking.
  - [ ] OK    Fix: 

- **`txEnable`**
  - EN: Enable or Disable Data Transmission.
  - IT: Attivare o disattivare la trasmissione dei dati.
  - [ ] OK    Fix: 

- **`txPort`**
  - EN: Transmit Port for this Target.
  - IT: Porta di trasmissione per questa destinazione.
  - [ ] OK    Fix: 

- **`udpPort`**
  - EN: UDP Receive Port of the Processor.
  - IT: Porta di ricezione UDP del processore.
  - [ ] OK    Fix: 

## `network.labels`

- **`admMapping`**
  - EN: Mapping:
  - IT: Mappatura:
  - [ ] OK    Fix: 

- **`currentIPv4`**
  - EN: Current IPv4:
  - IT: IPv4 Corrente:
  - [ ] OK    Fix: 

- **`interface`**
  - EN: Network Interface:
  - IT: Interfaccia di Rete:
  - [ ] OK    Fix: 

- **`localhost`**
  - EN: Localhost (127.0.0.1)
  - IT: Localhost (127.0.0.1)
  - [ ] OK    Fix: 

- **`mqttHost`**
  - EN: Host:
  - IT: Host:
  - [ ] OK    Fix: 

- **`mqttJsonQ`**
  - EN: Q:
  - IT: Q:
  - [ ] OK    Fix: 

- **`mqttJsonX`**
  - EN: X:
  - IT: X:
  - [ ] OK    Fix: 

- **`mqttJsonY`**
  - EN: Y:
  - IT: Y:
  - [ ] OK    Fix: 

- **`mqttJsonZ`**
  - EN: Z:
  - IT: Z:
  - [ ] OK    Fix: 

- **`mqttTagIds`**
  - EN: Tag IDs...
  - IT: ID Tag...
  - [ ] OK    Fix: 

- **`mqttTopic`**
  - EN: Topic:
  - IT: Argomento:
  - [ ] OK    Fix: 

- **`notAvailable`**
  - EN: Not available
  - IT: Non disponibile
  - [ ] OK    Fix: 

- **`offsetX`**
  - EN: Offset X:
  - IT: Offset X:
  - [ ] OK    Fix: 

- **`offsetY`**
  - EN: Offset Y:
  - IT: Offset Y:
  - [ ] OK    Fix: 

- **`offsetZ`**
  - EN: Offset Z:
  - IT: Offset Z:
  - [ ] OK    Fix: 

- **`oscPath`**
  - EN: OSC Path:
  - IT: OSC Path:
  - [ ] OK    Fix: 

- **`oscQuery`**
  - EN: OSC Query:
  - IT: OSC Query:
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Protocol:
  - IT: Protocollo:
  - [ ] OK    Fix: 

- **`psnInterface`**
  - EN: PSN Interface:
  - IT: PSN Interface:
  - [ ] OK    Fix: 

- **`rxPort`**
  - EN: Rx Port:
  - IT: Porta Rx:
  - [ ] OK    Fix: 

- **`scaleX`**
  - EN: Scale X:
  - IT: Scala X:
  - [ ] OK    Fix: 

- **`scaleY`**
  - EN: Scale Y:
  - IT: Scala Y:
  - [ ] OK    Fix: 

- **`scaleZ`**
  - EN: Scale Z:
  - IT: Scala Z:
  - [ ] OK    Fix: 

- **`tcpPort`**
  - EN: TCP Port:
  - IT: Porta TCP:
  - [ ] OK    Fix: 

- **`udpPort`**
  - EN: UDP Port:
  - IT: Porta UDP:
  - [ ] OK    Fix: 

## `network.messages`

- **`configExported`**
  - EN: Network configuration exported.
  - IT: Configurazione di rete esportata.
  - [ ] OK    Fix: 

- **`configImported`**
  - EN: Network configuration imported.
  - IT: Configurazione di rete importata.
  - [ ] OK    Fix: 

- **`configLoadedFromBackup`**
  - EN: Network configuration loaded from backup.
  - IT: Configurazione di rete caricata dal backup.
  - [ ] OK    Fix: 

- **`configNotFound`**
  - EN: Network config file not found.
  - IT: File config rete non trovato.
  - [ ] OK    Fix: 

- **`configReloaded`**
  - EN: Network configuration reloaded.
  - IT: Configurazione di rete ricaricata.
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Network configuration saved.
  - IT: Configurazione di rete salvata.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - IT: Errore: {error}
  - [ ] OK    Fix: 

- **`findDeviceSent`**
  - EN: Find Device command sent.
  - IT: Comando Trova dispositivo inviato.
  - [ ] OK    Fix: 

- **`maxTargetsReached`**
  - EN: Maximum Number of Targets/Servers Reached.
  - IT: Numero massimo di destinazioni/server raggiunto.
  - [ ] OK    Fix: 

- **`noBackupFound`**
  - EN: No backup files found.
  - IT: Nessun file di backup trovato.
  - [ ] OK    Fix: 

- **`onlyOneRemote`**
  - EN: Only one REMOTE connection is allowed.
  - IT: È consentita una sola connessione REMOTE.
  - [ ] OK    Fix: 

- **`oscManagerError`**
  - EN: Error: OSC Manager not available
  - IT: Errore: OSC Manager non disponibile
  - [ ] OK    Fix: 

- **`passwordEmpty`**
  - EN: Password cannot be empty.
  - IT: La password non può essere vuota.
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - IT: Seleziona prima una cartella progetto in Config Sistema.
  - [ ] OK    Fix: 

## `network.protocols`

- **`admOsc`**
  - EN: ADM-OSC
  - IT: ADM-OSC
  - [ ] OK    Fix: 

- **`disabled`**
  - EN: DISABLED
  - IT: DISABILITATO
  - [ ] OK    Fix: 

- **`mqtt`**
  - EN: MQTT
  - IT: MQTT
  - [ ] OK    Fix: 

- **`osc`**
  - EN: OSC
  - IT: OSC
  - [ ] OK    Fix: 

- **`psn`**
  - EN: PosiStageNet (PSN)
  - IT: PosiStageNet (PSN)
  - [ ] OK    Fix: 

- **`qlab`**
  - EN: QLab
  - IT: QLab
  - [ ] OK    Fix: 

- **`remote`**
  - EN: Remote
  - IT: Remote
  - [ ] OK    Fix: 

- **`rttrp`**
  - EN: RTTrP
  - IT: RTTrP
  - [ ] OK    Fix: 

- **`tcp`**
  - EN: TCP
  - IT: TCP
  - [ ] OK    Fix: 

- **`udp`**
  - EN: UDP
  - IT: UDP
  - [ ] OK    Fix: 

## `network.sections`

- **`admOsc`**
  - EN: ADM-OSC Mappings
  - IT: Mappature ADM-OSC
  - [ ] OK    Fix: 

- **`connections`**
  - EN: Network Connections
  - IT: Connessioni di Rete
  - [ ] OK    Fix: 

- **`network`**
  - EN: Network
  - IT: Rete
  - [ ] OK    Fix: 

- **`tracking`**
  - EN: Tracking
  - IT: Tracking
  - [ ] OK    Fix: 

## `network.table`

- **`defaultTarget`**
  - EN: Target {num}
  - IT: Target {num}
  - [ ] OK    Fix: 

- **`ipv4Address`**
  - EN: IPv4 Address
  - IT: Indirizzo IPv4
  - [ ] OK    Fix: 

- **`mode`**
  - EN: Mode
  - IT: Modo
  - [ ] OK    Fix: 

- **`name`**
  - EN: Name
  - IT: Nome
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Protocol
  - IT: Protocollo
  - [ ] OK    Fix: 

- **`rx`**
  - EN: Rx
  - IT: Rx
  - [ ] OK    Fix: 

- **`tx`**
  - EN: Tx
  - IT: Tx
  - [ ] OK    Fix: 

- **`txPort`**
  - EN: Tx Port
  - IT: Porta Tx
  - [ ] OK    Fix: 

## `network.toggles`

- **`disabled`**
  - EN: Disabled
  - IT: Disattivo
  - [ ] OK    Fix: 

- **`enabled`**
  - EN: Enabled
  - IT: Attivo
  - [ ] OK    Fix: 

- **`flipXOff`**
  - EN: Flip X: OFF
  - IT: Inverti X: OFF
  - [ ] OK    Fix: 

- **`flipXOn`**
  - EN: Flip X: ON
  - IT: Inverti X: ON
  - [ ] OK    Fix: 

- **`flipYOff`**
  - EN: Flip Y: OFF
  - IT: Inverti Y: OFF
  - [ ] OK    Fix: 

- **`flipYOn`**
  - EN: Flip Y: ON
  - IT: Inverti Y: ON
  - [ ] OK    Fix: 

- **`flipZOff`**
  - EN: Flip Z: OFF
  - IT: Inverti Z: OFF
  - [ ] OK    Fix: 

- **`flipZOn`**
  - EN: Flip Z: ON
  - IT: Inverti Z: ON
  - [ ] OK    Fix: 

- **`off`**
  - EN: OFF
  - IT: OFF
  - [ ] OK    Fix: 

- **`on`**
  - EN: ON
  - IT: ON
  - [ ] OK    Fix: 

- **`oscFilterAcceptAll`**
  - EN: OSC Filter: Accept All
  - IT: Filtro OSC: Accetta Tutto
  - [ ] OK    Fix: 

- **`oscFilterRegisteredOnly`**
  - EN: OSC Filter: Registered Only
  - IT: Filtro OSC: Solo Registrati
  - [ ] OK    Fix: 

- **`trackingOff`**
  - EN: Tracking: OFF
  - IT: Tracking: OFF
  - [ ] OK    Fix: 

- **`trackingOn`**
  - EN: Tracking: ON
  - IT: Tracking: ON
  - [ ] OK    Fix: 

## `networkLog`

- **`windowTitle`**
  - EN: Network Log
  - IT: Log di Rete
  - [ ] OK    Fix: 

## `networkLog.columns`

- **`address`**
  - EN: Address
  - IT: Indirizzo
  - [ ] OK    Fix: 

- **`arguments`**
  - EN: Arguments
  - IT: Argomenti
  - [ ] OK    Fix: 

- **`direction`**
  - EN: Dir
  - IT: Dir
  - [ ] OK    Fix: 

- **`ip`**
  - EN: IP
  - IT: IP
  - [ ] OK    Fix: 

- **`origin`**
  - EN: Origin
  - IT: Origine
  - [ ] OK    Fix: 

- **`port`**
  - EN: Port
  - IT: Porta
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Protocol
  - IT: Protocollo
  - [ ] OK    Fix: 

- **`time`**
  - EN: Time
  - IT: Ora
  - [ ] OK    Fix: 

- **`transport`**
  - EN: Trans
  - IT: Trasp
  - [ ] OK    Fix: 

## `networkLog.controls`

- **`clear`**
  - EN: CLEAR
  - IT: CANCELLA
  - [ ] OK    Fix: 

- **`export`**
  - EN: EXPORT
  - IT: ESPORTA
  - [ ] OK    Fix: 

- **`hideHeartbeat`**
  - EN: Hide Heartbeat
  - IT: Nascondi Heartbeat
  - [ ] OK    Fix: 

- **`logging`**
  - EN: Logging
  - IT: Registrazione
  - [ ] OK    Fix: 

## `networkLog.dialogs`

- **`exportCompleteMessage`**
  - EN: Log exported to: {path}
  - IT: Log esportato in: {path}
  - [ ] OK    Fix: 

- **`exportCompleteTitle`**
  - EN: Export Complete
  - IT: Esportazione Completata
  - [ ] OK    Fix: 

- **`exportFailedMessage`**
  - EN: Could not write to file: {path}
  - IT: Impossibile scrivere nel file: {path}
  - [ ] OK    Fix: 

- **`exportFailedTitle`**
  - EN: Export Failed
  - IT: Esportazione Fallita
  - [ ] OK    Fix: 

## `networkLog.exportMenu`

- **`exportAll`**
  - EN: Export All
  - IT: Esporta Tutto
  - [ ] OK    Fix: 

- **`exportFiltered`**
  - EN: Export Filtered
  - IT: Esporta Filtrato
  - [ ] OK    Fix: 

## `networkLog.filterModes`

- **`clientIp`**
  - EN: Client IP
  - IT: IP Client
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Protocol
  - IT: Protocollo
  - [ ] OK    Fix: 

- **`rejected`**
  - EN: Rejected
  - IT: Rifiutati
  - [ ] OK    Fix: 

- **`tcpUdp`**
  - EN: TCP/UDP
  - IT: TCP/UDP
  - [ ] OK    Fix: 

## `networkLog.filterToggles`

- **`incoming`**
  - EN: Incoming
  - IT: In Entrata
  - [ ] OK    Fix: 

- **`outgoing`**
  - EN: Outgoing
  - IT: In Uscita
  - [ ] OK    Fix: 

- **`tcp`**
  - EN: TCP
  - IT: TCP
  - [ ] OK    Fix: 

- **`udp`**
  - EN: UDP
  - IT: UDP
  - [ ] OK    Fix: 

## `networkLog.status`

- **`rejected`**
  - EN: REJECTED
  - IT: RIFIUTATO
  - [ ] OK    Fix: 

## `outputs.arrayModes`

- **`absolute`**
  - EN: ABSOLUTE
  - IT: ASSOLUTO
  - [ ] OK    Fix: 

- **`array`**
  - EN: Array
  - IT: Array
  - [ ] OK    Fix: 

- **`off`**
  - EN: OFF
  - IT: OFF
  - [ ] OK    Fix: 

- **`relative`**
  - EN: RELATIVE
  - IT: RELATIVO
  - [ ] OK    Fix: 

- **`single`**
  - EN: Single
  - IT: Singolo
  - [ ] OK    Fix: 

## `outputs.buttons`

- **`arrayHidden`**
  - EN: Array Hidden on Map
  - IT: Array nascosto sulla mappa
  - [ ] OK    Fix: 

- **`arrayVisible`**
  - EN: Array Visible on Map
  - IT: Array visibile sulla mappa
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - IT: Esporta
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - IT: Importa
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Backup
  - IT: Ricarica Backup
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Output Config
  - IT: Ricarica Config Uscite
  - [ ] OK    Fix: 

- **`speakerHidden`**
  - EN: Speaker Hidden on Map
  - IT: Altoparlante nascosto sulla mappa
  - [ ] OK    Fix: 

- **`speakerVisible`**
  - EN: Speaker Visible on Map
  - IT: Altoparlante Visibile sulla Mappa
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Output Config
  - IT: Memorizza Config Uscite
  - [ ] OK    Fix: 

- **`wizardOfOutZ`**
  - EN: Wizard of OutZ...
  - IT: Procedura OutZ...
  - [ ] OK    Fix: 

## `outputs.coordModes`

- **`xyz`**
  - EN: XYZ
  - IT: XYZ
  - [ ] OK    Fix: 

## `outputs.dialogs`

- **`export`**
  - EN: Export Output Configuration
  - IT: Esporta Configurazione Uscite
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import Output Configuration
  - IT: Importa Configurazione Uscite
  - [ ] OK    Fix: 

## `outputs.help`

- **`angleOff`**
  - EN: Output Channel Will Not Amplify Objects in this Angle in Front of it. (changes may affect the rest of the array)
  - IT: Il canale di uscita non amplificherà gli oggetti in questo angolo davanti ad esso. (le modifiche possono influenzare il resto dell'array)
  - [ ] OK    Fix: 

- **`angleOn`**
  - EN: Output Channel Will Amplify Objects in this Angle in its Back. (changes may affect the rest of the array)
  - IT: Il canale di uscita amplificherà gli oggetti in questo angolo dietro di esso. (le modifiche possono influenzare il resto dell'array)
  - [ ] OK    Fix: 

- **`applyToArray`**
  - EN: Apply Changes to the rest of the Array (Absolute value or Relative changes).
  - IT: Applicare le modifiche al resto dell'array (valore assoluto o modifiche relative).
  - [ ] OK    Fix: 

- **`arraySelector`**
  - EN: Selected Output Channel is part of Array.
  - IT: Il canale di uscita selezionato fa parte di un array.
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Output Channel Attenuation. (changes may affect the rest of the array)
  - IT: Attenuazione del canale di uscita. (le modifiche possono influenzare il resto dell'array)
  - [ ] OK    Fix: 

- **`channelSelector`**
  - EN: Output Channel Number and Selection.
  - IT: Numero e selezione del canale di uscita.
  - [ ] OK    Fix: 

- **`coordMode`**
  - EN: Coordinate display mode: Cartesian (X/Y/Z), Cylindrical (radius/azimuth/height), or Spherical (radius/azimuth/elevation).
  - IT: Modalità di visualizzazione coordinate: cartesiano (X/Y/Z), cilindrico (raggio/azimut/altezza) o sferico (raggio/azimut/elevazione).
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Output Channel Delay (positive values) or Latency Compensation (negative values). (changes may affect the rest of the array)
  - IT: Ritardo del canale di uscita (valori positivi) o compensazione di latenza (valori negativi). (le modifiche possono influenzare il resto dell'array)
  - [ ] OK    Fix: 

- **`directional`**
  - EN: Output Channel Directional Control. Drag to change orientation, Shift+drag for Angle Off, Alt+drag for Angle On. (changes may affect the rest of the array)
  - IT: Controllo direzionale del canale di uscita. Trascinare per cambiare orientamento, Maiusc+trascinare per Angle Off, Alt+trascinare per Angle On. (le modifiche possono influenzare il resto dell'array)
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Ratio of Distance Attenuation for Selected Output. (changes may affect the rest of the array)
  - IT: Rapporto di attenuazione per distanza per l'uscita selezionata. (le modifiche possono influenzare il resto dell'array)
  - [ ] OK    Fix: 

- **`eqBandToggle`**
  - EN: Toggle Band {band} on/off. When off the band is bypassed.
  - IT: Attiva/disattiva la banda {band}. Disattivata, la banda è bypassata.
  - [ ] OK    Fix: 

- **`eqEnable`**
  - EN: Enable or disable EQ processing for this output.
  - IT: Attivare o disattivare l'EQ per questa uscita.
  - [ ] OK    Fix: 

- **`eqFlatten`**
  - EN: Long press to reset all EQ bands to their default values.
  - IT: Pressione lunga per resettare tutte le bande EQ.
  - [ ] OK    Fix: 

- **`eqFreq`**
  - EN: Output EQ Band {band} frequency (20 Hz - 20 kHz).
  - IT: Frequenza EQ uscita banda {band} (20 Hz - 20 kHz).
  - [ ] OK    Fix: 

- **`eqGain`**
  - EN: Output EQ Band {band} gain (-24 to +24 dB).
  - IT: Guadagno EQ uscita banda {band} (-24 a +24 dB).
  - [ ] OK    Fix: 

- **`eqQ`**
  - EN: Output EQ Band {band} Q factor (0.1 - 10.0).
  - IT: Fattore Q EQ uscita banda {band} (0.1 - 10.0).
  - [ ] OK    Fix: 

- **`eqResetBand`**
  - EN: Long press to reset Band {band} to its default values.
  - IT: Pressione lunga per resettare la banda {band}.
  - [ ] OK    Fix: 

- **`eqShape`**
  - EN: Output EQ Band {band} filter shape.
  - IT: Forma del filtro EQ uscita banda {band}.
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Output Configuration to file (with file explorer window).
  - IT: Esportare la configurazione delle uscite su file (con finestra esplora risorse).
  - [ ] OK    Fix: 

- **`floorReflections`**
  - EN: Enable or Disable the Floor Reflections for this Speaker.
  - IT: Attivare o disattivare le riflessioni del pavimento per questo altoparlante.
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: Loss of High Frequency Depending on Distance from Object to Output. (changes may affect the rest of the array)
  - IT: Perdita di alte frequenze in funzione della distanza dall'oggetto all'uscita. (le modifiche possono influenzare il resto dell'array)
  - [ ] OK    Fix: 

- **`hParallax`**
  - EN: Horizontal Distance from Speaker to 'Targeted' Listener. (changes may affect the rest of the array)
  - IT: Distanza orizzontale dall'altoparlante all'ascoltatore « mirato ». (le modifiche possono influenzare il resto dell'array)
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Output Configuration from file (with file explorer window).
  - IT: Importare la configurazione delle uscite da file (con finestra esplora risorse).
  - [ ] OK    Fix: 

- **`liveSource`**
  - EN: Disables Live Source Attenuation for Selected Output. (changes may affect the rest of the array)
  - IT: Disattiva l'attenuazione della sorgente dal vivo per l'uscita selezionata. (le modifiche possono influenzare il resto dell'array)
  - [ ] OK    Fix: 

- **`mapVisibility`**
  - EN: Make Visible or Hide The Selected Output on the Map.
  - IT: Rendere visibile o nascondere l'uscita selezionata sulla mappa.
  - [ ] OK    Fix: 

- **`minLatency`**
  - EN: Disables Minimal Latency Mode for Selected Output. (changes may affect the rest of the array)
  - IT: Disattiva la modalità latenza minima per l'uscita selezionata. (le modifiche possono influenzare il resto dell'array)
  - [ ] OK    Fix: 

- **`nameEditor`**
  - EN: Displayed Output Channel Name (editable).
  - IT: Nome visualizzato del canale di uscita (modificabile).
  - [ ] OK    Fix: 

- **`pitch`**
  - EN: Output Channel Vertical Orientation used to Determine which Objects get amplified. (changes may affect the rest of the array)
  - IT: Orientamento verticale del canale di uscita usato per determinare quali oggetti vengono amplificati. (le modifiche possono influenzare il resto dell'array)
  - [ ] OK    Fix: 

- **`position1`**
  - EN: Output Channel {name} ({unit}).
  - IT: Canale di uscita {name} ({unit}).
  - [ ] OK    Fix: 

- **`position2`**
  - EN: Output Channel {name} ({unit}).
  - IT: Canale di uscita {name} ({unit}).
  - [ ] OK    Fix: 

- **`position3`**
  - EN: Output Channel {name} ({unit}).
  - IT: Canale di uscita {name} ({unit}).
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Output Configuration from backup file.
  - IT: Ricaricare la configurazione delle uscite dal file di backup.
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Output Configuration from file.
  - IT: Ricaricare la configurazione delle uscite da file.
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Output Configuration to file (with backup).
  - IT: Salvare la configurazione delle uscite su file (con backup).
  - [ ] OK    Fix: 

- **`vParallax`**
  - EN: Vertical Distance from Speaker to 'Targeted' Listener. Positive when the Speaker is Below the head of the Listener. (changes may affect the rest of the array)
  - IT: Distanza verticale dall'altoparlante all'ascoltatore « mirato ». Positivo quando l'altoparlante è sotto la testa dell'ascoltatore. (le modifiche possono influenzare il resto dell'array)
  - [ ] OK    Fix: 

- **`wizardOfOutZ`**
  - EN: Open Wizard of OutZ to Position Speaker Arrays Conveniently.
  - IT: Aprire il Wizard of OutZ per posizionare comodamente gli array di altoparlanti.
  - [ ] OK    Fix: 

## `outputs.labels`

- **`angleOff`**
  - EN: Angle Off:
  - IT: Angolo Off:
  - [ ] OK    Fix: 

- **`angleOn`**
  - EN: Angle On:
  - IT: Angolo On:
  - [ ] OK    Fix: 

- **`applyToArray`**
  - EN: Apply to Array:
  - IT: Applica all'Array:
  - [ ] OK    Fix: 

- **`array`**
  - EN: Array:
  - IT: Array:
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Attenuation:
  - IT: Attenuazione:
  - [ ] OK    Fix: 

- **`coordinates`**
  - EN: Coordinates:
  - IT: Coordinate:
  - [ ] OK    Fix: 

- **`delay`**
  - EN: Delay:
  - IT: Ritardo:
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Delay/Latency:
  - IT: Ritardo/Latenza:
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Distance Atten:
  - IT: Atten Distanza:
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: HF Damping:
  - IT: Smorzamento HF:
  - [ ] OK    Fix: 

- **`hParallax`**
  - EN: Horizontal Parallax:
  - IT: Parallasse Orizzontale:
  - [ ] OK    Fix: 

- **`latency`**
  - EN: Latency:
  - IT: Latenza:
  - [ ] OK    Fix: 

- **`name`**
  - EN: Name:
  - IT: Nome:
  - [ ] OK    Fix: 

- **`orientation`**
  - EN: Orientation:
  - IT: Orientamento:
  - [ ] OK    Fix: 

- **`pitch`**
  - EN: Pitch:
  - IT: Inclinazione:
  - [ ] OK    Fix: 

- **`positionX`**
  - EN: Position X:
  - IT: Posizione X:
  - [ ] OK    Fix: 

- **`positionY`**
  - EN: Position Y:
  - IT: Posizione Y:
  - [ ] OK    Fix: 

- **`positionZ`**
  - EN: Position Z:
  - IT: Posizione Z:
  - [ ] OK    Fix: 

- **`vParallax`**
  - EN: Vertical Parallax:
  - IT: Parallasse Verticale:
  - [ ] OK    Fix: 

## `outputs.messages`

- **`assignedToArray`**
  - EN: Output {num} assigned to Array {array}
  - IT: Uscita {num} assegnata all'Array {array}
  - [ ] OK    Fix: 

- **`backupLoaded`**
  - EN: Output configuration loaded from backup.
  - IT: Configurazione uscite caricata dal backup.
  - [ ] OK    Fix: 

- **`configExported`**
  - EN: Output configuration exported.
  - IT: Configurazione uscite esportata.
  - [ ] OK    Fix: 

- **`configImported`**
  - EN: Output configuration imported.
  - IT: Configurazione uscite importata.
  - [ ] OK    Fix: 

- **`configLoaded`**
  - EN: Output configuration loaded.
  - IT: Configurazione uscite caricata.
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Output configuration saved.
  - IT: Configurazione uscite salvata.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - IT: Errore: {error}
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - IT: Seleziona prima una cartella progetto in Config Sistema.
  - [ ] OK    Fix: 

- **`setToSingle`**
  - EN: Output {num} set to Single
  - IT: Uscita {num} impostata su Singolo
  - [ ] OK    Fix: 

## `outputs.tabs`

- **`eq`**
  - EN: Output EQ
  - IT: EQ Uscita
  - [ ] OK    Fix: 

- **`parameters`**
  - EN: Output Parameters
  - IT: Parametri Uscita
  - [ ] OK    Fix: 

## `outputs.toggles`

- **`floorReflectionsOff`**
  - EN: Floor Reflections: OFF
  - IT: Riflessioni Pavimento: OFF
  - [ ] OK    Fix: 

- **`floorReflectionsOn`**
  - EN: Floor Reflections: ON
  - IT: Riflessioni Pavimento: ON
  - [ ] OK    Fix: 

- **`liveSourceOff`**
  - EN: Live Source Atten: OFF
  - IT: Atten Live Source: OFF
  - [ ] OK    Fix: 

- **`liveSourceOn`**
  - EN: Live Source Atten: ON
  - IT: Atten Live Source: ON
  - [ ] OK    Fix: 

- **`minLatencyOff`**
  - EN: Minimal Latency: OFF
  - IT: Latenza Minima: OFF
  - [ ] OK    Fix: 

- **`minLatencyOn`**
  - EN: Minimal Latency: ON
  - IT: Latenza Minima: ON
  - [ ] OK    Fix: 

## `outputs.units`

- **`meters`**
  - EN: m
  - IT: m
  - [ ] OK    Fix: 

- **`ms`**
  - EN: ms
  - IT: ms
  - [ ] OK    Fix: 

## `reverbs`

- **`noChannels`**
  - EN: No reverb channels configured.

Set the number of Reverb Channels in System Config.
  - IT: Nessun canale di riverbero configurato.

Imposta il numero di Canali di Riverbero in Config Sistema.
  - [ ] OK    Fix: 

## `reverbs.algorithm`

- **`crossoverHigh`**
  - EN: Crossover High:
  - IT: Crossover alto:
  - [ ] OK    Fix: 

- **`crossoverLow`**
  - EN: Crossover Low:
  - IT: Crossover basso:
  - [ ] OK    Fix: 

- **`decaySection`**
  - EN: Decay
  - IT: Decadimento
  - [ ] OK    Fix: 

- **`diffusion`**
  - EN: Diffusion:
  - IT: Diffusione:
  - [ ] OK    Fix: 

- **`fdn`**
  - EN: FDN
  - IT: FDN
  - [ ] OK    Fix: 

- **`fdnSection`**
  - EN: FDN
  - IT: FDN
  - [ ] OK    Fix: 

- **`ir`**
  - EN: IR
  - IT: IR
  - [ ] OK    Fix: 

- **`irFile`**
  - EN: IR File:
  - IT: File IR:
  - [ ] OK    Fix: 

- **`irImport`**
  - EN: Import IR...
  - IT: Importa IR...
  - [ ] OK    Fix: 

- **`irImportSuccess`**
  - EN: IR imported: {file}
  - IT: IR importato: {file}
  - [ ] OK    Fix: 

- **`irLength`**
  - EN: IR Length:
  - IT: Lunghezza IR:
  - [ ] OK    Fix: 

- **`irNoProject`**
  - EN: Set a project folder first
  - IT: Impostare prima una cartella di progetto
  - [ ] OK    Fix: 

- **`irSection`**
  - EN: IR
  - IT: IR
  - [ ] OK    Fix: 

- **`irTrim`**
  - EN: IR Trim:
  - IT: Taglio IR:
  - [ ] OK    Fix: 

- **`noFileLoaded`**
  - EN: No IR loaded
  - IT: Nessun IR caricato
  - [ ] OK    Fix: 

- **`perNodeOff`**
  - EN: Per-node IR OFF
  - IT: IR per nodo OFF
  - [ ] OK    Fix: 

- **`perNodeOn`**
  - EN: Per-node IR ON
  - IT: IR per nodo ON
  - [ ] OK    Fix: 

- **`rt60`**
  - EN: RT60:
  - IT: RT60:
  - [ ] OK    Fix: 

- **`rt60HighMult`**
  - EN: RT60 High ×:
  - IT: RT60 Alto ×:
  - [ ] OK    Fix: 

- **`rt60LowMult`**
  - EN: RT60 Low ×:
  - IT: RT60 Basso ×:
  - [ ] OK    Fix: 

- **`scale`**
  - EN: Scale:
  - IT: Scala:
  - [ ] OK    Fix: 

- **`sdn`**
  - EN: SDN
  - IT: SDN
  - [ ] OK    Fix: 

- **`sdnSection`**
  - EN: SDN
  - IT: SDN
  - [ ] OK    Fix: 

- **`size`**
  - EN: Size:
  - IT: Dimensione:
  - [ ] OK    Fix: 

- **`wetLevel`**
  - EN: Wet Level:
  - IT: Livello Wet:
  - [ ] OK    Fix: 

## `reverbs.buttons`

- **`editOnMap`**
  - EN: Edit on Map
  - IT: Modifica sulla mappa
  - [ ] OK    Fix: 

- **`editOnMapOn`**
  - EN: Edit on Map ON
  - IT: Modifica sulla mappa ON
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - IT: Esporta
  - [ ] OK    Fix: 

- **`hiddenOnMap`**
  - EN: Reverbs Hidden on Map
  - IT: Riverberi Nascosti sulla Mappa
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - IT: Importa
  - [ ] OK    Fix: 

- **`mutePost`**
  - EN: Mute Post
  - IT: Mute Post
  - [ ] OK    Fix: 

- **`mutePostOn`**
  - EN: Mute Post ON
  - IT: Mute Post attivo
  - [ ] OK    Fix: 

- **`mutePre`**
  - EN: Mute Pre
  - IT: Mute Pre
  - [ ] OK    Fix: 

- **`mutePreOn`**
  - EN: Mute Pre ON
  - IT: Mute Pre attivo
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Backup
  - IT: Ricarica Backup
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Reverb Config
  - IT: Ricarica Config Riverbero
  - [ ] OK    Fix: 

- **`soloReverbs`**
  - EN: Solo Reverbs
  - IT: Solo Reverbs
  - [ ] OK    Fix: 

- **`soloReverbsOn`**
  - EN: Solo Reverbs ON
  - IT: Solo Riverberi ON
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Reverb Config
  - IT: Memorizza Config Riverbero
  - [ ] OK    Fix: 

- **`visibleOnMap`**
  - EN: Reverbs Visible on Map
  - IT: Riverberi Visibili sulla Mappa
  - [ ] OK    Fix: 

## `reverbs.coordModes`

- **`xyz`**
  - EN: XYZ
  - IT: XYZ
  - [ ] OK    Fix: 

## `reverbs.dialogs`

- **`export`**
  - EN: Export Reverb Configuration
  - IT: Esporta Configurazione Riverbero
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import Reverb Configuration
  - IT: Importa Configurazione Riverbero
  - [ ] OK    Fix: 

## `reverbs.help`

- **`algoCrossoverHigh`**
  - EN: High crossover frequency for 3-band decay (1 - 10 kHz).
  - IT: Frequenza di crossover alta per decadimento a 3 bande (1 - 10 kHz).
  - [ ] OK    Fix: 

- **`algoCrossoverLow`**
  - EN: Low crossover frequency for 3-band decay (50 - 500 Hz).
  - IT: Frequenza di crossover bassa per decadimento a 3 bande (50 - 500 Hz).
  - [ ] OK    Fix: 

- **`algoDiffusion`**
  - EN: Diffusion amount controlling echo density (0 - 100%).
  - IT: Quantità di diffusione che controlla la densità degli echi (0 - 100%).
  - [ ] OK    Fix: 

- **`algoFDN`**
  - EN: Select FDN (Feedback Delay Network) reverb algorithm.
  - IT: Selezionare l'algoritmo di riverbero FDN (Feedback Delay Network).
  - [ ] OK    Fix: 

- **`algoFDNSize`**
  - EN: FDN delay line size multiplier (0.5 - 2.0x).
  - IT: Moltiplicatore della dimensione delle linee di ritardo FDN (0.5 - 2.0x).
  - [ ] OK    Fix: 

- **`algoIR`**
  - EN: Select IR (Impulse Response convolution) reverb algorithm.
  - IT: Selezionare l'algoritmo di riverbero IR (convoluzione a risposta impulsiva).
  - [ ] OK    Fix: 

- **`algoIRFile`**
  - EN: Select or import an impulse response file for convolution.
  - IT: Selezionare o importare un file di risposta impulsiva per la convoluzione.
  - [ ] OK    Fix: 

- **`algoIRLength`**
  - EN: Maximum impulse response length (0.1 - 6.0 seconds).
  - IT: Lunghezza massima della risposta impulsiva (0.1 - 6.0 secondi).
  - [ ] OK    Fix: 

- **`algoIRTrim`**
  - EN: Trim the start of the impulse response (0 - 100 ms).
  - IT: Tagliare l'inizio della risposta impulsiva (0 - 100 ms).
  - [ ] OK    Fix: 

- **`algoPerNode`**
  - EN: Use a separate IR for each reverb node, or share one IR.
  - IT: Usare una IR separata per ogni nodo di riverbero, o condividere una sola IR.
  - [ ] OK    Fix: 

- **`algoRT60`**
  - EN: Reverb decay time RT60 (0.2 - 8.0 seconds).
  - IT: Tempo di decadimento del riverbero RT60 (0.2 - 8.0 secondi).
  - [ ] OK    Fix: 

- **`algoRT60HighMult`**
  - EN: High frequency RT60 multiplier (0.1 - 9.0x).
  - IT: Moltiplicatore RT60 alta frequenza (0.1 - 9.0x).
  - [ ] OK    Fix: 

- **`algoRT60LowMult`**
  - EN: Low frequency RT60 multiplier (0.1 - 9.0x).
  - IT: Moltiplicatore RT60 bassa frequenza (0.1 - 9.0x).
  - [ ] OK    Fix: 

- **`algoSDN`**
  - EN: Select SDN (Scattering Delay Network) reverb algorithm.
  - IT: Selezionare l'algoritmo di riverbero SDN (Scattering Delay Network).
  - [ ] OK    Fix: 

- **`algoSDNScale`**
  - EN: SDN inter-node delay scale factor (0.5 - 4.0x).
  - IT: Fattore di scala del ritardo inter-nodo SDN (0.5 - 4.0x).
  - [ ] OK    Fix: 

- **`algoWetLevel`**
  - EN: Wet/dry mix level for reverb output (-60 to +12 dB).
  - IT: Livello di mix wet/dry per l'uscita del riverbero (-60 a +12 dB).
  - [ ] OK    Fix: 

- **`angleOff`**
  - EN: Angle at which no amplification occurs (0-179 degrees).
  - IT: Angolo al quale non si produce amplificazione (0-179 gradi).
  - [ ] OK    Fix: 

- **`angleOn`**
  - EN: Angle at which amplification starts (1-180 degrees).
  - IT: Angolo al quale inizia l'amplificazione (1-180 gradi).
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Reverb channel attenuation (-92 to 0 dB).
  - IT: Attenuazione del canale di riverbero (-92 a 0 dB).
  - [ ] OK    Fix: 

- **`channelSelector`**
  - EN: Reverb Channel Number and Selection.
  - IT: Numero e selezione del canale di riverbero.
  - [ ] OK    Fix: 

- **`commonAtten`**
  - EN: Common attenuation percentage (0-100%).
  - IT: Percentuale di attenuazione comune (0-100%).
  - [ ] OK    Fix: 

- **`coordMode`**
  - EN: Coordinate display mode: Cartesian (X/Y/Z), Cylindrical (radius/azimuth/height), or Spherical (radius/azimuth/elevation).
  - IT: Modalità di visualizzazione coordinate: Cartesiano (X/Y/Z), Cilindrico (raggio/azimut/altezza), o Sferico (raggio/azimut/elevazione).
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Reverb delay/latency compensation (-100 to +100 ms).
  - IT: Compensazione ritardo/latenza del riverbero (-100 a +100 ms).
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Distance attenuation for reverb return (-6.0 to 0.0 dB/m).
  - IT: Attenuazione per distanza per il ritorno di riverbero (-6.0 a 0.0 dB/m).
  - [ ] OK    Fix: 

- **`distanceAttenEnable`**
  - EN: Distance attenuation percentage (0-200%).
  - IT: Percentuale di attenuazione per distanza (0-200%).
  - [ ] OK    Fix: 

- **`eqBandToggle`**
  - EN: Toggle pre-EQ Band {band} on/off. When off the band is bypassed.
  - IT: Attiva/disattiva la banda pre-EQ {band}.
  - [ ] OK    Fix: 

- **`eqEnable`**
  - EN: Enable or disable EQ processing for this reverb.
  - IT: Attivare o disattivare l'elaborazione EQ per questo riverbero.
  - [ ] OK    Fix: 

- **`eqFlatten`**
  - EN: Long press to reset all pre-EQ bands to their default values.
  - IT: Pressione lunga per resettare tutte le bande pre-EQ.
  - [ ] OK    Fix: 

- **`eqFreq`**
  - EN: Pre-EQ Band {band} frequency (20 Hz - 20 kHz).
  - IT: Frequenza pre-EQ banda {band} (20 Hz - 20 kHz).
  - [ ] OK    Fix: 

- **`eqGain`**
  - EN: Pre-EQ Band {band} gain (-24 to +24 dB).
  - IT: Guadagno pre-EQ banda {band} (-24 a +24 dB).
  - [ ] OK    Fix: 

- **`eqQ`**
  - EN: Pre-EQ Band {band} Q factor (0.1 - 20.0).
  - IT: Fattore Q pre-EQ banda {band} (0.1 - 20.0).
  - [ ] OK    Fix: 

- **`eqResetBand`**
  - EN: Long press to reset pre-EQ Band {band} to its default values.
  - IT: Pressione lunga per resettare la banda pre-EQ {band}.
  - [ ] OK    Fix: 

- **`eqShape`**
  - EN: Pre-EQ Band {band} filter shape.
  - IT: Forma del filtro pre-EQ banda {band}.
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Reverb Configuration to file (with file explorer window).
  - IT: Esportare la configurazione di riverbero su file (con esplora file).
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: High frequency loss per meter (-6.0 to 0.0 dB/m).
  - IT: Perdita alta frequenza per metro (-6.0 a 0.0 dB/m).
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Reverb Configuration from file (with file explorer window).
  - IT: Importare la configurazione di riverbero da file (con esplora file).
  - [ ] OK    Fix: 

- **`liveSourceTooltip`**
  - EN: Enable Live Source attenuation tamer. Reduces level fluctuations from sources close to the array.
  - IT: Attiva il Domatore di attenuazione Live Source. Riduce le fluttuazioni di livello delle sorgenti vicine all'array.
  - [ ] OK    Fix: 

- **`mapVisibility`**
  - EN: Make Visible or Hide All Reverb Channels on the Map.
  - IT: Rendere visibili o nascondere tutti i canali di riverbero sulla mappa.
  - [ ] OK    Fix: 

- **`miniLatencyTooltip`**
  - EN: Enable minimal latency mode for this reverb channel. Reduces processing delay at the cost of higher CPU usage.
  - IT: Attiva la modalità di latenza minima per questo canale di riverbero. Riduce il ritardo di elaborazione al costo di un maggiore uso della CPU.
  - [ ] OK    Fix: 

- **`muteButton`**
  - EN: Toggle mute for this output channel's reverb return.
  - IT: Attiva/disattiva il mute del ritorno di riverbero per questa uscita.
  - [ ] OK    Fix: 

- **`muteMacros`**
  - EN: Quick mute operations for output channels.
  - IT: Operazioni rapide di mute per i canali di uscita.
  - [ ] OK    Fix: 

- **`nameEditor`**
  - EN: Displayed Reverb Channel Name (editable).
  - IT: Nome visualizzato del canale di riverbero (modificabile).
  - [ ] OK    Fix: 

- **`orientation`**
  - EN: Reverb orientation angle (-179 to +180 degrees).
  - IT: Angolo di orientamento del riverbero (-179 a +180 gradi).
  - [ ] OK    Fix: 

- **`pitch`**
  - EN: Vertical orientation of reverb (-90 to +90 degrees).
  - IT: Orientamento verticale del riverbero (-90 a +90 gradi).
  - [ ] OK    Fix: 

- **`position1`**
  - EN: Reverb virtual source {name} ({unit}).
  - IT: Sorgente virtuale riverbero {name} ({unit}).
  - [ ] OK    Fix: 

- **`position2`**
  - EN: Reverb virtual source {name} ({unit}).
  - IT: Sorgente virtuale riverbero {name} ({unit}).
  - [ ] OK    Fix: 

- **`position3`**
  - EN: Reverb virtual source {name} ({unit}).
  - IT: Sorgente virtuale riverbero {name} ({unit}).
  - [ ] OK    Fix: 

- **`postEqBandToggle`**
  - EN: Toggle post-EQ Band {band} on/off. When off the band is bypassed.
  - IT: Attiva/disattiva la banda post-EQ {band}.
  - [ ] OK    Fix: 

- **`postEqEnable`**
  - EN: Enable or disable post-processing EQ.
  - IT: Attivare o disattivare l'EQ di post-elaborazione.
  - [ ] OK    Fix: 

- **`postEqFlatten`**
  - EN: Long press to reset all post-EQ bands to their default values.
  - IT: Pressione lunga per resettare tutte le bande post-EQ.
  - [ ] OK    Fix: 

- **`postEqFreq`**
  - EN: Post-EQ Band {band} frequency (20 Hz - 20 kHz).
  - IT: Frequenza della banda {band} del post-EQ (20 Hz - 20 kHz).
  - [ ] OK    Fix: 

- **`postEqGain`**
  - EN: Post-EQ Band {band} gain (-24 to +24 dB).
  - IT: Guadagno della banda {band} del post-EQ (-24 a +24 dB).
  - [ ] OK    Fix: 

- **`postEqQ`**
  - EN: Post-EQ Band {band} Q factor (0.1 - 20.0).
  - IT: Fattore Q della banda {band} del post-EQ (0.1 - 20.0).
  - [ ] OK    Fix: 

- **`postEqResetBand`**
  - EN: Long press to reset post-EQ Band {band} to its default values.
  - IT: Pressione lunga per resettare la banda post-EQ {band}.
  - [ ] OK    Fix: 

- **`postEqShape`**
  - EN: Post-EQ Band {band} filter shape.
  - IT: Forma del filtro della banda {band} del post-EQ.
  - [ ] OK    Fix: 

- **`postExpAttack`**
  - EN: Post-expander attack time (0.1 - 50 ms).
  - IT: Tempo di attacco del post-espansore (0.1 - 50 ms).
  - [ ] OK    Fix: 

- **`postExpBypass`**
  - EN: Bypass or enable the post-expander on reverb returns.
  - IT: Bypassare o attivare il post-espansore sui ritorni di riverbero.
  - [ ] OK    Fix: 

- **`postExpRatio`**
  - EN: Post-expander ratio (1:1 to 1:8).
  - IT: Rapporto del post-espansore (1:1 a 1:8).
  - [ ] OK    Fix: 

- **`postExpRelease`**
  - EN: Post-expander release time (50 - 2000 ms).
  - IT: Tempo di rilascio del post-espansore (50 - 2000 ms).
  - [ ] OK    Fix: 

- **`postExpThreshold`**
  - EN: Post-expander threshold (-80 to -10 dB).
  - IT: Soglia del post-espansore (-80 a -10 dB).
  - [ ] OK    Fix: 

- **`preCompAttack`**
  - EN: Pre-compressor attack time (0.1 - 100 ms).
  - IT: Tempo di attacco del pre-compressore (0.1 - 100 ms).
  - [ ] OK    Fix: 

- **`preCompBypass`**
  - EN: Bypass or enable the pre-compressor on reverb feeds.
  - IT: Bypassare o attivare il pre-compressore sulle mandate di riverbero.
  - [ ] OK    Fix: 

- **`preCompRatio`**
  - EN: Pre-compressor ratio (1:1 to 20:1).
  - IT: Rapporto del pre-compressore (1:1 a 20:1).
  - [ ] OK    Fix: 

- **`preCompRelease`**
  - EN: Pre-compressor release time (10 - 1000 ms).
  - IT: Tempo di rilascio del pre-compressore (10 - 1000 ms).
  - [ ] OK    Fix: 

- **`preCompThreshold`**
  - EN: Pre-compressor threshold (-60 to 0 dB).
  - IT: Soglia del pre-compressore (-60 a 0 dB).
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Reverb Configuration from backup file.
  - IT: Ricaricare la configurazione di riverbero dal file di backup.
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Reverb Configuration from file.
  - IT: Ricaricare la configurazione di riverbero da file.
  - [ ] OK    Fix: 

- **`returnOffset1`**
  - EN: Reverb return offset {name} ({unit}).
  - IT: Offset di ritorno riverbero {name} ({unit}).
  - [ ] OK    Fix: 

- **`returnOffset2`**
  - EN: Reverb return offset {name} ({unit}).
  - IT: Offset di ritorno riverbero {name} ({unit}).
  - [ ] OK    Fix: 

- **`returnOffset3`**
  - EN: Reverb return offset {name} ({unit}).
  - IT: Offset di ritorno riverbero {name} ({unit}).
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Reverb Configuration to file (with backup).
  - IT: Memorizzare la configurazione di riverbero su file (con backup).
  - [ ] OK    Fix: 

## `reverbs.labels`

- **`angleOff`**
  - EN: Angle Off:
  - IT: Angolo Off:
  - [ ] OK    Fix: 

- **`angleOn`**
  - EN: Angle On:
  - IT: Angolo On:
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Attenuation:
  - IT: Attenuazione:
  - [ ] OK    Fix: 

- **`commonAtten`**
  - EN: Common Atten:
  - IT: Atten Comune:
  - [ ] OK    Fix: 

- **`coordinates`**
  - EN: Coord:
  - IT: Coord:
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Delay/Latency:
  - IT: Ritardo/Latenza:
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Distance Atten:
  - IT: Atten Distanza:
  - [ ] OK    Fix: 

- **`distanceAttenPercent`**
  - EN: Distance Atten %:
  - IT: Atten Distanza %:
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: HF Damping:
  - IT: Smorzamento HF:
  - [ ] OK    Fix: 

- **`muteMacro`**
  - EN: Mute Macro:
  - IT: Macro Mute:
  - [ ] OK    Fix: 

- **`name`**
  - EN: Name:
  - IT: Nome:
  - [ ] OK    Fix: 

- **`orientation`**
  - EN: Orientation:
  - IT: Orientamento:
  - [ ] OK    Fix: 

- **`outputMutes`**
  - EN: Output Mutes:
  - IT: Mute Uscite:
  - [ ] OK    Fix: 

- **`pitch`**
  - EN: Pitch:
  - IT: Inclinazione:
  - [ ] OK    Fix: 

- **`positionX`**
  - EN: Position X:
  - IT: Posizione X:
  - [ ] OK    Fix: 

- **`positionY`**
  - EN: Position Y:
  - IT: Posizione Y:
  - [ ] OK    Fix: 

- **`positionZ`**
  - EN: Position Z:
  - IT: Posizione Z:
  - [ ] OK    Fix: 

- **`returnOffsetX`**
  - EN: Return Offset X:
  - IT: Offset Ritorno X:
  - [ ] OK    Fix: 

- **`returnOffsetY`**
  - EN: Return Offset Y:
  - IT: Offset Ritorno Y:
  - [ ] OK    Fix: 

- **`returnOffsetZ`**
  - EN: Return Offset Z:
  - IT: Offset Ritorno Z:
  - [ ] OK    Fix: 

## `reverbs.messages`

- **`backupLoaded`**
  - EN: Reverb configuration loaded from backup.
  - IT: Configurazione riverbero caricata dal backup.
  - [ ] OK    Fix: 

- **`configExported`**
  - EN: Reverb configuration exported.
  - IT: Configurazione riverbero esportata.
  - [ ] OK    Fix: 

- **`configImported`**
  - EN: Reverb configuration imported.
  - IT: Configurazione riverbero importata.
  - [ ] OK    Fix: 

- **`configLoaded`**
  - EN: Reverb configuration loaded.
  - IT: Configurazione riverbero caricata.
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Reverb configuration saved.
  - IT: Configurazione riverbero salvata.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - IT: Errore: {error}
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - IT: Seleziona prima una cartella progetto in Config Sistema.
  - [ ] OK    Fix: 

## `reverbs.muteMacros`

- **`invertMutes`**
  - EN: INVERT MUTES
  - IT: INVERTI MUTE
  - [ ] OK    Fix: 

- **`muteAll`**
  - EN: MUTE ALL
  - IT: MUTA TUTTI
  - [ ] OK    Fix: 

- **`muteArray`**
  - EN: MUTE ARRAY
  - IT: MUTA ARRAY
  - [ ] OK    Fix: 

- **`muteEven`**
  - EN: MUTE EVEN
  - IT: MUTA PARI
  - [ ] OK    Fix: 

- **`muteOdd`**
  - EN: MUTE ODD
  - IT: MUTA DISPARI
  - [ ] OK    Fix: 

- **`select`**
  - EN: Mute Macro Select
  - IT: Seleziona Macro Mute
  - [ ] OK    Fix: 

- **`unmuteAll`**
  - EN: UNMUTE ALL
  - IT: SMUTA TUTTI
  - [ ] OK    Fix: 

- **`unmuteArray`**
  - EN: UNMUTE ARRAY
  - IT: SMUTA ARRAY
  - [ ] OK    Fix: 

## `reverbs.postProcessing`

- **`attack`**
  - EN: Attack:
  - IT: Attacco:
  - [ ] OK    Fix: 

- **`expander`**
  - EN: Expander
  - IT: Espansore
  - [ ] OK    Fix: 

- **`expanderOff`**
  - EN: Expander OFF
  - IT: Espansore OFF
  - [ ] OK    Fix: 

- **`expanderOn`**
  - EN: Expander ON
  - IT: Espansore ON
  - [ ] OK    Fix: 

- **`ratio`**
  - EN: Ratio:
  - IT: Rapporto:
  - [ ] OK    Fix: 

- **`release`**
  - EN: Release:
  - IT: Rilascio:
  - [ ] OK    Fix: 

- **`threshold`**
  - EN: Threshold:
  - IT: Soglia:
  - [ ] OK    Fix: 

## `reverbs.preProcessing`

- **`attack`**
  - EN: Attack:
  - IT: Attacco:
  - [ ] OK    Fix: 

- **`compressor`**
  - EN: Compressor
  - IT: Compressore
  - [ ] OK    Fix: 

- **`compressorOff`**
  - EN: Compressor OFF
  - IT: Compressore OFF
  - [ ] OK    Fix: 

- **`compressorOn`**
  - EN: Compressor ON
  - IT: Compressore ON
  - [ ] OK    Fix: 

- **`ratio`**
  - EN: Ratio:
  - IT: Rapporto:
  - [ ] OK    Fix: 

- **`release`**
  - EN: Release:
  - IT: Rilascio:
  - [ ] OK    Fix: 

- **`threshold`**
  - EN: Threshold:
  - IT: Soglia:
  - [ ] OK    Fix: 

## `reverbs.sections`

- **`reverbFeed`**
  - EN: Reverb Feed
  - IT: Mandata Riverbero
  - [ ] OK    Fix: 

- **`reverbReturn`**
  - EN: Reverb Return
  - IT: Ritorno Riverbero
  - [ ] OK    Fix: 

## `reverbs.tabs`

- **`algorithm`**
  - EN: Algorithm
  - IT: Algoritmo
  - [ ] OK    Fix: 

- **`channelParams`**
  - EN: Channel Parameters
  - IT: Parametri Canale
  - [ ] OK    Fix: 

- **`postProcessing`**
  - EN: Post-Processing
  - IT: Post-elaborazione
  - [ ] OK    Fix: 

- **`preProcessing`**
  - EN: Pre-Processing
  - IT: Pre-elaborazione
  - [ ] OK    Fix: 

## `reverbs.toggles`

- **`liveSourceOff`**
  - EN: Live Source Atten OFF
  - IT: Atten Live Source OFF
  - [ ] OK    Fix: 

- **`liveSourceOn`**
  - EN: Live Source Atten ON
  - IT: Atten Live Source ON
  - [ ] OK    Fix: 

- **`minLatencyOff`**
  - EN: Minimal Latency OFF
  - IT: Latenza Minima OFF
  - [ ] OK    Fix: 

- **`minLatencyOn`**
  - EN: Minimal Latency ON
  - IT: Latenza Minima ON
  - [ ] OK    Fix: 

## `sampler.buttons`

- **`copy`**
  - EN: Copy
  - IT: Copia
  - [ ] OK    Fix: 

- **`copyCell`**
  - EN: Copy Cell
  - IT: Copia cella
  - [ ] OK    Fix: 

- **`copySet`**
  - EN: Copy Set
  - IT: Copia set
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - IT: Esporta
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - IT: Importa
  - [ ] OK    Fix: 

- **`paste`**
  - EN: Paste
  - IT: Incolla
  - [ ] OK    Fix: 

- **`pasteCell`**
  - EN: Paste Cell
  - IT: Incolla cella
  - [ ] OK    Fix: 

- **`pasteSet`**
  - EN: Paste Set
  - IT: Incolla set
  - [ ] OK    Fix: 

## `sampler.cell`

- **`attenuation`**
  - EN: Attenuation (dB)
  - IT: Attenuazione (dB)
  - [ ] OK    Fix: 

- **`clear`**
  - EN: Clear
  - IT: Cancella
  - [ ] OK    Fix: 

- **`inOut`**
  - EN: In/Out (ms)
  - IT: Iniz/Fine (ms)
  - [ ] OK    Fix: 

- **`load`**
  - EN: Load
  - IT: Carica
  - [ ] OK    Fix: 

- **`loadTitle`**
  - EN: Load Sample
  - IT: Carica sample
  - [ ] OK    Fix: 

- **`offset`**
  - EN: Offset (m)
  - IT: Offset (m)
  - [ ] OK    Fix: 

- **`preview`**
  - EN: Preview
  - IT: Anteprima
  - [ ] OK    Fix: 

- **`previewStop`**
  - EN: Stop
  - IT: Stop
  - [ ] OK    Fix: 

## `sampler.grid`

- **`help`**
  - EN: Click = select
Shift = multi
Ctrl = set toggle
DblClick = load
  - IT: Clic=seleziona | Shift=multi | Ctrl=alterna set | DblClic=carica
  - [ ] OK    Fix: 

## `sampler.labels`

- **`lightpadZone`**
  - EN: Lightpad Zone
  - IT: Zona Lightpad
  - [ ] OK    Fix: 

- **`selectZone`**
  - EN: Select Zone
  - IT: Seleziona zona
  - [ ] OK    Fix: 

## `sampler.lightpadZone`

- **`none`**
  - EN: None
  - IT: Nessuna
  - [ ] OK    Fix: 

## `sampler.press`

- **`height`**
  - EN: Height
  - IT: Altezza
  - [ ] OK    Fix: 

- **`hf`**
  - EN: HF Shelf
  - IT: HF Shelf
  - [ ] OK    Fix: 

- **`level`**
  - EN: Level
  - IT: Livello
  - [ ] OK    Fix: 

- **`xy`**
  - EN: Position XY
  - IT: Position XY
  - [ ] OK    Fix: 

## `sampler.remote`

- **`gridLayout`**
  - EN: Grid Layout
  - IT: Layout della griglia
  - [ ] OK    Fix: 

## `sampler.section`

- **`actions`**
  - EN: ACTIONS
  - IT: AZIONI
  - [ ] OK    Fix: 

- **`cell`**
  - EN: CELL PROPERTIES
  - IT: PROPRIETÀ CELLA
  - [ ] OK    Fix: 

- **`pressure`**
  - EN: PRESSURE MAPPINGS
  - IT: MAPPATURE PRESSIONE
  - [ ] OK    Fix: 

- **`set`**
  - EN: SET MANAGEMENT
  - IT: GESTIONE SET
  - [ ] OK    Fix: 

## `sampler.set`

- **`copy`**
  - EN: (copy)
  - IT: (copia)
  - [ ] OK    Fix: 

- **`default`**
  - EN: Set
  - IT: Set
  - [ ] OK    Fix: 

- **`level`**
  - EN: Level (dB)
  - IT: Livello (dB)
  - [ ] OK    Fix: 

- **`pos`**
  - EN: Position (m)
  - IT: Posizione (m)
  - [ ] OK    Fix: 

- **`rename`**
  - EN: Rename
  - IT: Rinomina
  - [ ] OK    Fix: 

- **`roundRobin`**
  - EN: Round-Robin
  - IT: Round-Robin
  - [ ] OK    Fix: 

- **`sequential`**
  - EN: Sequential
  - IT: Sequenziale
  - [ ] OK    Fix: 

## `sampler.tooltips`

- **`addSet`**
  - EN: Create a new set. If cells are selected they will be assigned to it.
  - IT: Crea un nuovo set. Se ci sono celle selezionate, gli verranno assegnate.
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Cell attenuation in dB (0 = no attenuation, -60 = silent)
  - IT: Attenuazione cella in dB (0 = nessuna attenuazione, -60 = silenzio)
  - [ ] OK    Fix: 

- **`clear`**
  - EN: Remove the sample from the selected cell (long press)
  - IT: Rimuovi il campione dalla cella selezionata (pressione lunga)
  - [ ] OK    Fix: 

- **`copy`**
  - EN: Copy selected cell or active set to clipboard
  - IT: Copia la cella selezionata o il set attivo negli appunti
  - [ ] OK    Fix: 

- **`deleteSet`**
  - EN: Delete the active set (long press)
  - IT: Elimina il set attivo (pressione lunga)
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export sampler configuration to file
  - IT: Esporta la configurazione del sampler su file
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import sampler configuration from file
  - IT: Importa la configurazione del sampler da file
  - [ ] OK    Fix: 

- **`inOut`**
  - EN: Set the In/Out time range in milliseconds. Drag between thumbs to shift both.
  - IT: Imposta l'intervallo di tempo Entrata/Uscita in millisecondi. Trascina tra le maniglie per spostare entrambe.
  - [ ] OK    Fix: 

- **`load`**
  - EN: Load a sample file into the selected cell
  - IT: Carica un file di campione nella cella selezionata
  - [ ] OK    Fix: 

- **`offset`**
  - EN: Offset position in meters (X, Y, Z) relative to the set position
  - IT: Spostamento di posizione in metri (X, Y, Z) relativo alla posizione del set
  - [ ] OK    Fix: 

- **`paste`**
  - EN: Paste clipboard data to selected cell or active set
  - IT: Incolla i dati degli appunti nella cella selezionata o nel set attivo
  - [ ] OK    Fix: 

- **`playMode`**
  - EN: Toggle between Sequential and Round-Robin playback
  - IT: Alterna tra riproduzione Sequenziale e Round-Robin
  - [ ] OK    Fix: 

- **`pressCurve`**
  - EN: Pressure response curve (0 = concave, 0.5 = linear, 1 = convex)
  - IT: Curva di risposta della pressione (0 = concava, 0,5 = lineare, 1 = convessa)
  - [ ] OK    Fix: 

- **`pressDir`**
  - EN: Toggle pressure direction: + = more pressure increases, - = decreases
  - IT: Alterna la direzione di pressione: + = più pressione aumenta, - = diminuisce
  - [ ] OK    Fix: 

- **`pressHeight`**
  - EN: Map finger pressure to vertical position (Z)
  - IT: Mappa la pressione del dito sulla posizione verticale (Z)
  - [ ] OK    Fix: 

- **`pressHF`**
  - EN: Map finger pressure to high-frequency shelf attenuation
  - IT: Mappa la pressione del dito sull'attenuazione dello shelving alto
  - [ ] OK    Fix: 

- **`pressLevel`**
  - EN: Map finger pressure to output level
  - IT: Mappa la pressione del dito sul livello di uscita
  - [ ] OK    Fix: 

- **`pressXY`**
  - EN: Map finger pressure to XY position movement
  - IT: Mappa la pressione del dito sullo spostamento di posizione XY
  - [ ] OK    Fix: 

- **`pressXYScale`**
  - EN: Sensitivity: how far the source moves per pressure step
  - IT: Sensibilità: di quanto si sposta la sorgente per ogni passo di pressione
  - [ ] OK    Fix: 

- **`preview`**
  - EN: Preview the loaded sample
  - IT: Pre-ascolta il campione caricato
  - [ ] OK    Fix: 

- **`renameSet`**
  - EN: Rename the active set
  - IT: Rinomina il set attivo
  - [ ] OK    Fix: 

- **`setLevel`**
  - EN: Set output level in dB (0 = unity, -60 = silent)
  - IT: Imposta il livello di uscita in dB (0 = unità, -60 = silenzio)
  - [ ] OK    Fix: 

- **`setPos`**
  - EN: Set base position in meters (X, Y, Z)
  - IT: Imposta la posizione di base in metri (X, Y, Z)
  - [ ] OK    Fix: 

## `sampler.zone`

- **`remotePad`**
  - EN: Pad {num}
  - IT: Pad {num}
  - [ ] OK    Fix: 

- **`selectRemotePad`**
  - EN: Select Remote Pad
  - IT: Seleziona Pad del Telecomando
  - [ ] OK    Fix: 

## `setAllInputs`

- **`warning`**
  - EN: Changes will apply to ALL inputs
  - IT: Le modifiche verranno applicate a TUTTI gli ingressi
  - [ ] OK    Fix: 

- **`windowTitle`**
  - EN: Set All Inputs
  - IT: Imposta Tutti gli Ingressi
  - [ ] OK    Fix: 

## `setAllInputs.buttons`

- **`all1d`**
  - EN: All 1/d
  - IT: Tutti 1/d
  - [ ] OK    Fix: 

- **`allLog`**
  - EN: All Log
  - IT: Tutti Log
  - [ ] OK    Fix: 

- **`closeWindow`**
  - EN: CLOSE WINDOW
  - IT: CHIUDI FINESTRA
  - [ ] OK    Fix: 

- **`flipXyzOff`**
  - EN: Flip XYZ > OFF
  - IT: Inverti XYZ > OFF
  - [ ] OK    Fix: 

- **`resetDirectivity`**
  - EN: Reset directivity
  - IT: Reset direttività
  - [ ] OK    Fix: 

- **`turnOffJitterLfo`**
  - EN: Turn OFF jitter & LFO
  - IT: Disattiva jitter & LFO
  - [ ] OK    Fix: 

- **`turnOffLiveSource`**
  - EN: Turn OFF Live source atten.
  - IT: Disattiva atten. Live source
  - [ ] OK    Fix: 

## `setAllInputs.coordinateModes`

- **`rThetaPhi`**
  - EN: r θ φ
  - IT: r θ φ
  - [ ] OK    Fix: 

- **`rThetaZ`**
  - EN: r θ Z
  - IT: r θ Z
  - [ ] OK    Fix: 

- **`xyz`**
  - EN: XYZ
  - IT: XYZ
  - [ ] OK    Fix: 

## `setAllInputs.labels`

- **`common`**
  - EN: common
  - IT: comune
  - [ ] OK    Fix: 

- **`constraintPositions`**
  - EN: Constraint positions:
  - IT: Vincoli posizione:
  - [ ] OK    Fix: 

- **`coordinateMode`**
  - EN: Coordinate mode:
  - IT: Modo coordinate:
  - [ ] OK    Fix: 

- **`dbPerMeter`**
  - EN: dB/m
  - IT: dB/m
  - [ ] OK    Fix: 

- **`distanceAttenuation`**
  - EN: Distance attenuation
  - IT: Attenuazione distanza
  - [ ] OK    Fix: 

- **`floorReflections`**
  - EN: Floor Reflections:
  - IT: Riflessioni Pavimento:
  - [ ] OK    Fix: 

- **`fringe`**
  - EN: Fringe:
  - IT: Frangia:
  - [ ] OK    Fix: 

- **`heightFactor`**
  - EN: Height factor:
  - IT: Fattore altezza:
  - [ ] OK    Fix: 

- **`minimalLatency`**
  - EN: Minimal Latency:
  - IT: Latenza Minima:
  - [ ] OK    Fix: 

- **`muteMacros`**
  - EN: Mute macros:
  - IT: Macro mute:
  - [ ] OK    Fix: 

- **`ratio`**
  - EN: ratio
  - IT: rapporto
  - [ ] OK    Fix: 

- **`sidelines`**
  - EN: Sidelines:
  - IT: Sidelines:
  - [ ] OK    Fix: 

## `snapshot`

- **`qlabExportDone`**
  - EN: QLab export complete: {count} cues created
  - IT: Esportazione QLab completata: {count} cue creati
  - [ ] OK    Fix: 

- **`qlabExportStarted`**
  - EN: Writing {count} cues to QLab...
  - IT: Scrittura di {count} cue su QLab...
  - [ ] OK    Fix: 

- **`qlabGroupName`**
  - EN: Snapshot "{name}"
  - IT: Istantanea "{name}"
  - [ ] OK    Fix: 

- **`qlabMemoText`**
  - EN: Run either of the following cues to recall or update this snapshot
  - IT: Eseguire uno dei seguenti cue per richiamare o aggiornare questo snapshot
  - [ ] OK    Fix: 

- **`qlabNoTarget`**
  - EN: No QLab target configured
  - IT: Nessuna destinazione QLab configurata
  - [ ] OK    Fix: 

- **`qlabReloadName`**
  - EN: Reload "{name}"
  - IT: Ricarica "{name}"
  - [ ] OK    Fix: 

- **`qlabUpdateName`**
  - EN: Update "{name}"
  - IT: Aggiorna "{name}"
  - [ ] OK    Fix: 

## `snapshotScope`

- **`all`**
  - EN: ALL
  - IT: TUTTI
  - [ ] OK    Fix: 

- **`applyScope`**
  - EN: Apply scope:
  - IT: Applica ambito:
  - [ ] OK    Fix: 

- **`autoPreselectModified`**
  - EN: Auto-preselect modified parameters
  - IT: Preseleziona automaticamente i parametri modificati
  - [ ] OK    Fix: 

- **`title`**
  - EN: Snapshot Scope: {name}
  - IT: Ambito Snapshot: {name}
  - [ ] OK    Fix: 

- **`whenRecalling`**
  - EN: When Recalling
  - IT: Al Richiamo
  - [ ] OK    Fix: 

- **`whenSaving`**
  - EN: When Saving
  - IT: Al Salvataggio
  - [ ] OK    Fix: 

- **`windowTitle`**
  - EN: Input Snapshot Scope
  - IT: Ambito Snapshot Ingresso
  - [ ] OK    Fix: 

- **`writeSnapshotLoadCue`**
  - EN: Write Snapshot Load Cue to QLab
  - IT: Scrivi cue di caricamento su QLab
  - [ ] OK    Fix: 

- **`writeSnapshotLoadCueTooltip`**
  - EN: Also create a QLab cue to load this snapshot via OSC
  - IT: Crea anche un cue QLab per caricare questo snapshot via OSC
  - [ ] OK    Fix: 

- **`writeToQLab`**
  - EN: Write to QLab
  - IT: Scrivi su QLab
  - [ ] OK    Fix: 

- **`writeToQLabTooltip`**
  - EN: Export scope to QLab instead of saving to file
  - IT: Esporta lo scope su QLab invece di salvare su file
  - [ ] OK    Fix: 

## `snapshotScope.buttons`

- **`cancel`**
  - EN: Cancel
  - IT: Annulla
  - [ ] OK    Fix: 

- **`clearChanges`**
  - EN: Clear Changes
  - IT: Cancella modifiche
  - [ ] OK    Fix: 

- **`ok`**
  - EN: OK
  - IT: OK
  - [ ] OK    Fix: 

- **`selectModified`**
  - EN: Select Modified
  - IT: Seleziona modificati
  - [ ] OK    Fix: 

## `snapshotScope.sections`

- **`attenuation`**
  - EN: Attenuation
  - IT: Attenuazione
  - [ ] OK    Fix: 

- **`automOtion`**
  - EN: AutomOtion
  - IT: AutomOzione
  - [ ] OK    Fix: 

- **`directivity`**
  - EN: Directivity
  - IT: Direttività
  - [ ] OK    Fix: 

- **`hackoustics`**
  - EN: Hackoustics
  - IT: Hackustica
  - [ ] OK    Fix: 

- **`input`**
  - EN: Input
  - IT: Ingresso
  - [ ] OK    Fix: 

- **`lfo`**
  - EN: LFO
  - IT: LFO
  - [ ] OK    Fix: 

- **`liveSource`**
  - EN: Live Source
  - IT: Live Source
  - [ ] OK    Fix: 

- **`mutes`**
  - EN: Mutes
  - IT: Mute
  - [ ] OK    Fix: 

- **`position`**
  - EN: Position
  - IT: Posizione
  - [ ] OK    Fix: 

## `statusBar`

- **`displayLabel`**
  - EN: Display:
  - IT: Display:
  - [ ] OK    Fix: 

- **`helpMode`**
  - EN: Help
  - IT: Aiuto
  - [ ] OK    Fix: 

- **`oscMode`**
  - EN: OSC
  - IT: OSC
  - [ ] OK    Fix: 

## `systemConfig.algorithms`

- **`inputBuffer`**
  - EN: InputBuffer (read-time delays)
  - IT: InputBuffer (ritardi in lettura)
  - [ ] OK    Fix: 

- **`outputBuffer`**
  - EN: OutputBuffer (write-time delays)
  - IT: OutputBuffer (ritardi in scrittura)
  - [ ] OK    Fix: 

## `systemConfig.binauralOutput`

- **`select`**
  - EN: Select...
  - IT: Seleziona...
  - [ ] OK    Fix: 

## `systemConfig.buttons`

- **`audioPatch`**
  - EN: Audio Interface and Patching Window
  - IT: Interfaccia Audio e Routing
  - [ ] OK    Fix: 

- **`binauralOff`**
  - EN: Binaural: OFF
  - IT: Binaurale: OFF
  - [ ] OK    Fix: 

- **`binauralOn`**
  - EN: Binaural: ON
  - IT: Binaurale: ON
  - [ ] OK    Fix: 

- **`clearSolo`**
  - EN: Clear Solo
  - IT: Clear Solo
  - [ ] OK    Fix: 

- **`copySystemInfo`**
  - EN: Copy System Info
  - IT: Copia info di sistema
  - [ ] OK    Fix: 

- **`diagnosticsCollapsed`**
  - EN: Diagnostics  [+]
  - IT: Diagnostica  [+]
  - [ ] OK    Fix: 

- **`diagnosticsExpanded`**
  - EN: Diagnostics  [-]
  - IT: Diagnostica  [-]
  - [ ] OK    Fix: 

- **`exportLogs`**
  - EN: Export Logs
  - IT: Esporta log
  - [ ] OK    Fix: 

- **`exportSystem`**
  - EN: Export System Configuration
  - IT: Esporta Configurazione Sistema
  - [ ] OK    Fix: 

- **`importSystem`**
  - EN: Import System Configuration
  - IT: Importa Configurazione Sistema
  - [ ] OK    Fix: 

- **`levelMeter`**
  - EN: Level Meters
  - IT: Level Meters
  - [ ] OK    Fix: 

- **`openLogFolder`**
  - EN: Open Log Folder
  - IT: Apri cartella log
  - [ ] OK    Fix: 

- **`processingOff`**
  - EN: Processing: OFF
  - IT: Elaborazione: OFF
  - [ ] OK    Fix: 

- **`processingOn`**
  - EN: Processing: ON
  - IT: Elaborazione: ON
  - [ ] OK    Fix: 

- **`quickLongPressOff`**
  - EN: Normal
  - IT: Normale
  - [ ] OK    Fix: 

- **`quickLongPressOn`**
  - EN: Quick
  - IT: Rapido
  - [ ] OK    Fix: 

- **`reloadComplete`**
  - EN: Reload Complete Configuration
  - IT: Ricarica Configurazione Completa
  - [ ] OK    Fix: 

- **`reloadCompleteBackup`**
  - EN: Reload Complete Config. Backup
  - IT: Ricarica Backup Config. Completa
  - [ ] OK    Fix: 

- **`reloadSystem`**
  - EN: Reload System Configuration
  - IT: Ricarica Configurazione Sistema
  - [ ] OK    Fix: 

- **`reloadSystemBackup`**
  - EN: Reload System Config. Backup
  - IT: Ricarica Backup Config. Sistema
  - [ ] OK    Fix: 

- **`reportIssue`**
  - EN: Report Issue
  - IT: Segnala problema
  - [ ] OK    Fix: 

- **`selectProjectFolder`**
  - EN: Select Project Folder
  - IT: Seleziona Cartella Progetto
  - [ ] OK    Fix: 

- **`setup`**
  - EN: Setup
  - IT: Config.
  - [ ] OK    Fix: 

- **`soloModeMulti`**
  - EN: Solo: Multi
  - IT: Solo: Multiplo
  - [ ] OK    Fix: 

- **`soloModeSingle`**
  - EN: Solo: Single
  - IT: Solo: Singolo
  - [ ] OK    Fix: 

- **`storeComplete`**
  - EN: Store Complete Configuration
  - IT: Memorizza Configurazione Completa
  - [ ] OK    Fix: 

- **`storeSystem`**
  - EN: Store System Configuration
  - IT: Memorizza Configurazione Sistema
  - [ ] OK    Fix: 

## `systemConfig.colorSchemes`

- **`black`**
  - EN: Black
  - IT: Nero
  - [ ] OK    Fix: 

- **`default`**
  - EN: Default (Dark Gray)
  - IT: Predefinito (Grigio Scuro)
  - [ ] OK    Fix: 

- **`light`**
  - EN: Light
  - IT: Chiaro
  - [ ] OK    Fix: 

## `systemConfig.controller`

- **`lightpad`**
  - EN: Lightpad
  - IT: Lightpad
  - [ ] OK    Fix: 

- **`off`**
  - EN: Off
  - IT: Disattivato
  - [ ] OK    Fix: 

- **`remote`**
  - EN: Remote
  - IT: Telecomando
  - [ ] OK    Fix: 

## `systemConfig.devices`

- **`off`**
  - EN: Off
  - IT: Off
  - [ ] OK    Fix: 

## `systemConfig.dialogs`

- **`exportSystemConfig`**
  - EN: Export System Configuration
  - IT: Esporta Configurazione Sistema
  - [ ] OK    Fix: 

- **`importSystemConfig`**
  - EN: Import System Configuration
  - IT: Importa Configurazione Sistema
  - [ ] OK    Fix: 

- **`reduce`**
  - EN: Reduce
  - IT: Riduci
  - [ ] OK    Fix: 

- **`reduceInputChannels.message`**
  - EN: Reducing from {current} to {new} input channels will remove settings for channels {start} to {end}.

This cannot be undone.
  - IT: Riducendo da {current} a {new} canali di ingresso verranno rimosse le impostazioni per i canali da {start} a {end}.

Questa operazione non può essere annullata.
  - [ ] OK    Fix: 

- **`reduceInputChannels.title`**
  - EN: Reduce Input Channels?
  - IT: Ridurre i Canali di Ingresso?
  - [ ] OK    Fix: 

- **`reduceOutputChannels.message`**
  - EN: Reducing from {current} to {new} output channels will remove settings for channels {start} to {end}.

This cannot be undone.
  - IT: Riducendo da {current} a {new} canali di uscita verranno rimosse le impostazioni per i canali da {start} a {end}.

Questa operazione non può essere annullata.
  - [ ] OK    Fix: 

- **`reduceOutputChannels.title`**
  - EN: Reduce Output Channels?
  - IT: Ridurre i Canali di Uscita?
  - [ ] OK    Fix: 

- **`reduceReverbChannels.message`**
  - EN: Reducing from {current} to {new} reverb channels will remove settings for channels {start} to {end}.

This cannot be undone.
  - IT: Riducendo da {current} a {new} canali di riverbero verranno rimosse le impostazioni per i canali da {start} a {end}.

Questa operazione non può essere annullata.
  - [ ] OK    Fix: 

- **`reduceReverbChannels.title`**
  - EN: Reduce Reverb Channels?
  - IT: Ridurre i Canali di Riverbero?
  - [ ] OK    Fix: 

- **`selectProjectFolder`**
  - EN: Select Project Folder
  - IT: Seleziona Cartella Progetto
  - [ ] OK    Fix: 

## `systemConfig.help`

- **`algorithm`**
  - EN: Select the rendering algorithm from the menu.
  - IT: Seleziona l'algoritmo di rendering dal menu.
  - [ ] OK    Fix: 

- **`audioPatch`**
  - EN: Opens the Audio Interface and Patching Window.
  - IT: Apre la finestra dell'interfaccia audio e routing.
  - [ ] OK    Fix: 

- **`binauralAngle`**
  - EN: Horizontal rotation of binaural listener view (degrees, 0 = facing stage).
  - IT: Rotazione orizzontale del punto di vista dell'ascoltatore binaurale (gradi, 0 = rivolto al palco).
  - [ ] OK    Fix: 

- **`binauralAtten`**
  - EN: Overall level offset for binaural output (dB).
  - IT: Offset di livello globale per l'uscita binaurale (dB).
  - [ ] OK    Fix: 

- **`binauralDelay`**
  - EN: Additional delay for binaural output (milliseconds).
  - IT: Ritardo aggiuntivo per l'uscita binaurale (millisecondi).
  - [ ] OK    Fix: 

- **`binauralDistance`**
  - EN: Distance of binaural listener from stage origin (meters).
  - IT: Distanza dell'ascoltatore binaurale dall'origine del palco (metri).
  - [ ] OK    Fix: 

- **`binauralEnable`**
  - EN: Enable or disable the binaural renderer processing.
  - IT: Attivare o disattivare l'elaborazione del renderer binaurale.
  - [ ] OK    Fix: 

- **`binauralOutput`**
  - EN: Select output channel pair for binaural monitoring. Off disables binaural output.
  - IT: Selezionare la coppia di canali di uscita per il monitoraggio binaurale. Off disattiva l'uscita binaurale.
  - [ ] OK    Fix: 

- **`colorScheme`**
  - EN: Select the color scheme: Default (dark gray), Black (pure black for OLED displays), or Light (daytime use).
  - IT: Seleziona lo schema colori: Predefinito (grigio scuro), Nero (nero puro per display OLED) o Chiaro (uso diurno).
  - [ ] OK    Fix: 

- **`copySystemInfo`**
  - EN: Copy detailed system information to the clipboard for support requests.
  - IT: Copia informazioni di sistema dettagliate negli appunti per le richieste di supporto.
  - [ ] OK    Fix: 

- **`diagnosticsToggle`**
  - EN: Long-press to show or hide the diagnostic tools (export logs, open log folder, copy system info, report issue).
  - IT: Pressione lunga per mostrare o nascondere gli strumenti di diagnostica (esporta log, apri cartella log, copia info di sistema, segnala problema).
  - [ ] OK    Fix: 

- **`dialsAndButtons`**
  - EN: Select the hardware controller for dials and buttons: Stream Deck+ or XenceLabs Quick Keys.
  - IT: Selezionare il controller hardware per manopole e tasti: Stream Deck+ o XenceLabs Quick Keys.
  - [ ] OK    Fix: 

- **`domeElevation`**
  - EN: Dome elevation angle: 180 = hemisphere, 360 = full sphere.
  - IT: Angolo di elevazione della cupola: 180 = semisfera, 360 = sfera completa.
  - [ ] OK    Fix: 

- **`exportLogs`**
  - EN: Export diagnostic logs to a zip file for debugging or support.
  - IT: Esporta i log di diagnostica in un file zip per il debug o il supporto.
  - [ ] OK    Fix: 

- **`exportSystem`**
  - EN: Export System Configuration to file (with file explorer window).
  - IT: Esporta la configurazione di sistema su file (con finestra esplora file).
  - [ ] OK    Fix: 

- **`haasEffect`**
  - EN: Hass Effect to apply to the system. Will take into account the Latency Compensations (System, Input and Output).
  - IT: Effetto Haas da applicare al sistema. Terrà conto delle compensazioni di latenza (Sistema, Ingresso e Uscita).
  - [ ] OK    Fix: 

- **`importSystem`**
  - EN: Import System Configuration from file (with file explorer window).
  - IT: Importa la configurazione di sistema da file (con finestra esplora file).
  - [ ] OK    Fix: 

- **`inputChannels`**
  - EN: Number of Input Channels.
  - IT: Numero di canali di ingresso.
  - [ ] OK    Fix: 

- **`language`**
  - EN: Select the user interface language. Changes take full effect after restarting the application.
  - IT: Seleziona la lingua dell'interfaccia utente. Le modifiche avranno pieno effetto dopo il riavvio dell'applicazione.
  - [ ] OK    Fix: 

- **`lightpadSetup`**
  - EN: Display the connected Roli Lightpads and allow for splitting them in 4 smaller pads.
  - IT: Visualizzare i Roli Lightpad collegati e consentire di dividerli in 4 pad più piccoli.
  - [ ] OK    Fix: 

- **`masterLevel`**
  - EN: Master Level (affects all outputs).
  - IT: Livello master (influisce su tutte le uscite).
  - [ ] OK    Fix: 

- **`openLogFolder`**
  - EN: Open the application's log folder in the system file explorer.
  - IT: Apri la cartella dei log dell'applicazione nell'esploratore di file di sistema.
  - [ ] OK    Fix: 

- **`originCenter`**
  - EN: Set origin to center of stage volume. Typical for Spherical Dome setups.
  - IT: Imposta l'origine al centro del volume del palco. Tipico per configurazioni a Cupola Sferica.
  - [ ] OK    Fix: 

- **`originCenterGround`**
  - EN: Set origin to center of stage at ground level. Typical for Surround or Cylindrical setups.
  - IT: Imposta l'origine al centro del palco a livello del suolo. Tipico per configurazioni Surround o Cilindriche.
  - [ ] OK    Fix: 

- **`originDepth`**
  - EN: Origin Y offset from stage center (0 = centered, negative = front/downstage).
  - IT: Offset Y dell'origine dal centro del palco (0 = centrato, negativo = fronte/proscenio).
  - [ ] OK    Fix: 

- **`originDismissInputs`**
  - EN: Long press to dismiss and keep current input positions.
  - IT: Pressione prolungata per ignorare e mantenere le posizioni attuali degli ingressi.
  - [ ] OK    Fix: 

- **`originDismissOutputs`**
  - EN: Long press to dismiss and keep current output positions.
  - IT: Pressione prolungata per ignorare e mantenere le posizioni attuali delle uscite.
  - [ ] OK    Fix: 

- **`originDismissReverbs`**
  - EN: Long press to dismiss and keep current reverb positions.
  - IT: Pressione prolungata per ignorare e mantenere le posizioni attuali dei riverb.
  - [ ] OK    Fix: 

- **`originFront`**
  - EN: Set origin to front center of stage. Typical for frontal stages.
  - IT: Imposta l'origine al centro anteriore del palco. Tipico per palchi frontali.
  - [ ] OK    Fix: 

- **`originHeight`**
  - EN: Origin Z offset from floor (0 = floor level, positive = above floor).
  - IT: Offset Z dell'origine dal pavimento (0 = livello pavimento, positivo = sopra il pavimento).
  - [ ] OK    Fix: 

- **`originShiftInputs`**
  - EN: Long press to shift all input positions by the origin change.
  - IT: Pressione prolungata per spostare tutte le posizioni degli ingressi in base al cambio di origine.
  - [ ] OK    Fix: 

- **`originShiftOutputs`**
  - EN: Long press to shift all output positions by the origin change.
  - IT: Pressione prolungata per spostare tutte le posizioni delle uscite in base al cambio di origine.
  - [ ] OK    Fix: 

- **`originShiftReverbs`**
  - EN: Long press to shift all reverb positions by the origin change.
  - IT: Pressione prolungata per spostare tutte le posizioni dei riverb in base al cambio di origine.
  - [ ] OK    Fix: 

- **`originWidth`**
  - EN: Origin X offset from stage center (0 = centered, negative = left).
  - IT: Offset X dell'origine dal centro del palco (0 = centrato, negativo = sinistra).
  - [ ] OK    Fix: 

- **`outputChannels`**
  - EN: Number of Output Channels.
  - IT: Numero di canali di uscita.
  - [ ] OK    Fix: 

- **`positionControl`**
  - EN: Select the hardware controller for position control: Space Mouse, Joystick, or Game Pad.
  - IT: Selezionare il controller hardware per il controllo di posizione: Space Mouse, Joystick o gamepad.
  - [ ] OK    Fix: 

- **`processing`**
  - EN: Lock all I/O parameters and start the DSP. Long press to stop the DSP.
  - IT: Blocca tutti i parametri I/O e avvia il DSP. Tenere premuto per fermare il DSP.
  - [ ] OK    Fix: 

- **`quickLongPress`**
  - EN: Long press duration. Instead of confirmation windows this software uses long presses.
  - IT: Durata della pressione lunga. Invece di finestre di conferma, questo software usa pressioni lunghe.
  - [ ] OK    Fix: 

- **`reloadComplete`**
  - EN: Reload Complete Configuration from files.
  - IT: Ricarica la configurazione completa dai file.
  - [ ] OK    Fix: 

- **`reloadCompleteBackup`**
  - EN: Reload Complete Configuration from backup files.
  - IT: Ricarica la configurazione completa dai file di backup.
  - [ ] OK    Fix: 

- **`reloadSystem`**
  - EN: Reload System Configuration from file.
  - IT: Ricarica la configurazione di sistema dal file.
  - [ ] OK    Fix: 

- **`reloadSystemBackup`**
  - EN: Reload System Configuration from backup file.
  - IT: Ricarica la configurazione di sistema dal file di backup.
  - [ ] OK    Fix: 

- **`remoteSetup`**
  - EN: Select the number of pads in the Remote's XY Pads tab.
  - IT: Selezionare il numero di pad nella scheda XY Pads del Remote.
  - [ ] OK    Fix: 

- **`reportIssue`**
  - EN: Open the WFS-DIY GitHub issues page in your default browser.
  - IT: Apri la pagina degli issue GitHub di WFS-DIY nel browser predefinito.
  - [ ] OK    Fix: 

- **`reverbChannels`**
  - EN: Number of Reverb Channels.
  - IT: Numero di canali di riverbero.
  - [ ] OK    Fix: 

- **`sampler`**
  - EN: Enable or disable the Sampler feature for input channels.
  - IT: Attivare o disattivare la funzione Sampler per i canali di ingresso. Selezionare il controller: Lightpad o Telecomando.
  - [ ] OK    Fix: 

- **`selectProjectFolder`**
  - EN: Select the Location of the Current Project Folder where to store files.
  - IT: Seleziona la posizione della cartella del progetto corrente dove memorizzare i file.
  - [ ] OK    Fix: 

- **`showLocation`**
  - EN: Location of the current show.
  - IT: Luogo dello spettacolo corrente.
  - [ ] OK    Fix: 

- **`showName`**
  - EN: Name of the current show.
  - IT: Nome dello spettacolo corrente.
  - [ ] OK    Fix: 

- **`soloMode`**
  - EN: Single: one input at a time. Multi: multiple inputs simultaneously.
  - IT: Singolo: un ingresso alla volta. Multi: più ingressi simultaneamente.
  - [ ] OK    Fix: 

- **`speedOfSound`**
  - EN: Speed of Sound (related to the temperature).
  - IT: Velocità del suono (correlata alla temperatura).
  - [ ] OK    Fix: 

- **`stageDepth`**
  - EN: Depth of the stage in meters (Box shape only).
  - IT: Profondità del palco in metri (solo forma Scatola).
  - [ ] OK    Fix: 

- **`stageDiameter`**
  - EN: Diameter of the stage in meters (Cylinder and Dome shapes).
  - IT: Diametro del palco in metri (forme Cilindro e Cupola).
  - [ ] OK    Fix: 

- **`stageDismiss`**
  - EN: Long press to dismiss and keep current input positions.
  - IT: Pressione prolungata per ignorare e mantenere le posizioni attuali degli ingressi.
  - [ ] OK    Fix: 

- **`stageFit`**
  - EN: Long press to move outlier inputs back within the new stage bounds.
  - IT: Pressione prolungata per riportare gli ingressi fuori dai limiti all'interno delle nuove dimensioni del palco.
  - [ ] OK    Fix: 

- **`stageHeight`**
  - EN: Height of the stage in meters (Box and Cylinder shapes).
  - IT: Altezza del palco in metri (forme Scatola e Cilindro).
  - [ ] OK    Fix: 

- **`stageScale`**
  - EN: Long press to scale all input positions proportionally to the new stage dimensions.
  - IT: Pressione prolungata per scalare tutte le posizioni degli ingressi proporzionalmente alle nuove dimensioni del palco.
  - [ ] OK    Fix: 

- **`stageShape`**
  - EN: Select the shape of the stage (Box, Cylinder, or Dome).
  - IT: Seleziona la forma del palco (Scatola, Cilindro o Cupola).
  - [ ] OK    Fix: 

- **`stageWidth`**
  - EN: Width of the stage in meters (Box shape only).
  - IT: Larghezza del palco in metri (solo forma Scatola).
  - [ ] OK    Fix: 

- **`storeComplete`**
  - EN: Store Complete Configuration to files (with backup).
  - IT: Memorizza la configurazione completa nei file (con backup).
  - [ ] OK    Fix: 

- **`storeSystem`**
  - EN: Store System Configuration to file (with backup).
  - IT: Memorizza la configurazione di sistema nel file (con backup).
  - [ ] OK    Fix: 

- **`systemLatency`**
  - EN: Total latency of the system (Mixing board & Computer) / Specific Input and Output Latency/Delay can be set in the respective Input and Output settings.
  - IT: Latenza totale del sistema (Mixer e Computer) / La latenza/ritardo specifico di ingresso e uscita può essere impostata nelle rispettive impostazioni.
  - [ ] OK    Fix: 

- **`temperature`**
  - EN: Temperature (gives the Speed of Sound).
  - IT: Temperatura (determina la velocità del suono).
  - [ ] OK    Fix: 

## `systemConfig.labels`

- **`algorithm`**
  - EN: Algorithm:
  - IT: Algoritmo:
  - [ ] OK    Fix: 

- **`binauralAngle`**
  - EN: Listener Angle:
  - IT: Angolo ascoltatore:
  - [ ] OK    Fix: 

- **`binauralAtten`**
  - EN: Binaural Level:
  - IT: Livello binaurale:
  - [ ] OK    Fix: 

- **`binauralDelay`**
  - EN: Binaural Delay:
  - IT: Ritardo binaurale:
  - [ ] OK    Fix: 

- **`binauralDistance`**
  - EN: Listener Distance:
  - IT: Distanza ascoltatore:
  - [ ] OK    Fix: 

- **`binauralOutput`**
  - EN: Binaural Output:
  - IT: Uscita binaurale:
  - [ ] OK    Fix: 

- **`clickToSplit`**
  - EN: Click to split
  - IT: Clic per dividere
  - [ ] OK    Fix: 

- **`colorScheme`**
  - EN: Color Scheme:
  - IT: Schema Colori:
  - [ ] OK    Fix: 

- **`dialsAndButtons`**
  - EN: Dials and Buttons:
  - IT: Manopole e tasti:
  - [ ] OK    Fix: 

- **`domeElevation`**
  - EN: Elevation:
  - IT: Elevazione:
  - [ ] OK    Fix: 

- **`haasEffect`**
  - EN: Haas Effect:
  - IT: Effetto Haas:
  - [ ] OK    Fix: 

- **`inputChannels`**
  - EN: Input Channels:
  - IT: Canali Ingresso:
  - [ ] OK    Fix: 

- **`language`**
  - EN: Language:
  - IT: Lingua:
  - [ ] OK    Fix: 

- **`lightpadArrangement`**
  - EN: Lightpad Arrangement
  - IT: Disposizione Lightpad
  - [ ] OK    Fix: 

- **`masterLevel`**
  - EN: Master Level:
  - IT: Livello Master:
  - [ ] OK    Fix: 

- **`originDepth`**
  - EN: Origin Depth:
  - IT: Origine Profondità:
  - [ ] OK    Fix: 

- **`originHeight`**
  - EN: Origin Height:
  - IT: Origine Altezza:
  - [ ] OK    Fix: 

- **`originWidth`**
  - EN: Origin Width:
  - IT: Origine Larghezza:
  - [ ] OK    Fix: 

- **`outputChannels`**
  - EN: Output Channels:
  - IT: Canali Uscita:
  - [ ] OK    Fix: 

- **`positionControl`**
  - EN: Position Control:
  - IT: Controllo di posizione:
  - [ ] OK    Fix: 

- **`quickLongPress`**
  - EN: Long Press:
  - IT: Pressione lunga:
  - [ ] OK    Fix: 

- **`reverbChannels`**
  - EN: Reverb Channels:
  - IT: Canali Riverbero:
  - [ ] OK    Fix: 

- **`sampler`**
  - EN: Sampler:
  - IT: Sampler:
  - [ ] OK    Fix: 

- **`showLocation`**
  - EN: Location:
  - IT: Luogo:
  - [ ] OK    Fix: 

- **`showName`**
  - EN: Name:
  - IT: Nome:
  - [ ] OK    Fix: 

- **`speedOfSound`**
  - EN: Speed of Sound:
  - IT: Velocità del Suono:
  - [ ] OK    Fix: 

- **`split`**
  - EN: Split
  - IT: Dividi
  - [ ] OK    Fix: 

- **`stageDepth`**
  - EN: Depth:
  - IT: Profondità:
  - [ ] OK    Fix: 

- **`stageDiameter`**
  - EN: Diameter:
  - IT: Diametro:
  - [ ] OK    Fix: 

- **`stageHeight`**
  - EN: Height:
  - IT: Altezza:
  - [ ] OK    Fix: 

- **`stageShape`**
  - EN: Stage Shape:
  - IT: Forma Palco:
  - [ ] OK    Fix: 

- **`stageWidth`**
  - EN: Width:
  - IT: Larghezza:
  - [ ] OK    Fix: 

- **`systemLatency`**
  - EN: System Latency:
  - IT: Latenza Sistema:
  - [ ] OK    Fix: 

- **`temperature`**
  - EN: Temperature:
  - IT: Temperatura:
  - [ ] OK    Fix: 

- **`updateAvailable`**
  - EN: Update {version} available
  - IT: Aggiornamento {version} disponibile
  - [ ] OK    Fix: 

- **`version`**
  - EN: Version {version}
  - IT: Versione {version}
  - [ ] OK    Fix: 

## `systemConfig.messages`

- **`configLoaded`**
  - EN: Complete configuration loaded.
  - IT: Configurazione completa caricata.
  - [ ] OK    Fix: 

- **`configLoadedFromBackup`**
  - EN: Configuration loaded from backup.
  - IT: Configurazione caricata dal backup.
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Complete configuration saved.
  - IT: Configurazione completa salvata.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - IT: Errore: {error}
  - [ ] OK    Fix: 

- **`languageChanged`**
  - EN: Language changed to: {language} (requires restart for full effect)
  - IT: Lingua cambiata in: {language} (richiede riavvio per effetto completo)
  - [ ] OK    Fix: 

- **`logDirNotFound`**
  - EN: Log directory not found
  - IT: Cartella dei log non trovata
  - [ ] OK    Fix: 

- **`logsExported`**
  - EN: Logs exported to {path}
  - IT: Log esportati in {path}
  - [ ] OK    Fix: 

- **`logsExportFailed`**
  - EN: Failed to export logs
  - IT: Esportazione dei log fallita
  - [ ] OK    Fix: 

- **`noBackupFilesFound`**
  - EN: No backup files found.
  - IT: Nessun file di backup trovato.
  - [ ] OK    Fix: 

- **`partialLoad`**
  - EN: Partial load: {error}
  - IT: Caricamento parziale: {error}
  - [ ] OK    Fix: 

- **`partialLoadFromBackup`**
  - EN: Partial load from backup: {error}
  - IT: Caricamento parziale dal backup: {error}
  - [ ] OK    Fix: 

- **`restartBody`**
  - EN: Please restart the application for the language change to take full effect.
  - IT: Riavviare l'applicazione affinché il cambio di lingua abbia pieno effetto.
  - [ ] OK    Fix: 

- **`restartTitle`**
  - EN: Restart Required
  - IT: Riavvio necessario
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder first.
  - IT: Seleziona prima una cartella progetto.
  - [ ] OK    Fix: 

- **`selectLogExportFolder`**
  - EN: Select destination for log export
  - IT: Seleziona la destinazione per l'esportazione dei log
  - [ ] OK    Fix: 

- **`systemConfigExported`**
  - EN: System configuration exported.
  - IT: Configurazione di sistema esportata.
  - [ ] OK    Fix: 

- **`systemConfigFileNotFound`**
  - EN: System configuration file not found.
  - IT: File di configurazione di sistema non trovato.
  - [ ] OK    Fix: 

- **`systemConfigImported`**
  - EN: System configuration imported.
  - IT: Configurazione di sistema importata.
  - [ ] OK    Fix: 

- **`systemConfigLoaded`**
  - EN: System configuration loaded.
  - IT: Configurazione di sistema caricata.
  - [ ] OK    Fix: 

- **`systemConfigLoadedFromBackup`**
  - EN: System configuration loaded from backup.
  - IT: Configurazione di sistema caricata dal backup.
  - [ ] OK    Fix: 

- **`systemConfigSaved`**
  - EN: System configuration saved.
  - IT: Configurazione di sistema salvata.
  - [ ] OK    Fix: 

- **`systemInfoCopied`**
  - EN: System info copied to clipboard
  - IT: Informazioni di sistema copiate negli appunti
  - [ ] OK    Fix: 

## `systemConfig.sections`

- **`binauralRenderer`**
  - EN: Binaural Renderer
  - IT: Renderer Binaurale
  - [ ] OK    Fix: 

- **`controllers`**
  - EN: Controllers
  - IT: Controllers
  - [ ] OK    Fix: 

- **`io`**
  - EN: I/O
  - IT: I/O
  - [ ] OK    Fix: 

- **`master`**
  - EN: Master Section
  - IT: Sezione Master
  - [ ] OK    Fix: 

- **`show`**
  - EN: Show
  - IT: Spettacolo
  - [ ] OK    Fix: 

- **`stage`**
  - EN: Stage
  - IT: Palco
  - [ ] OK    Fix: 

- **`ui`**
  - EN: UI
  - IT: Interfaccia
  - [ ] OK    Fix: 

- **`wfsProcessor`**
  - EN: WFS Processor
  - IT: Processore WFS
  - [ ] OK    Fix: 

## `systemConfig.stageShapes`

- **`box`**
  - EN: Box
  - IT: Scatola
  - [ ] OK    Fix: 

- **`cylinder`**
  - EN: Cylinder
  - IT: Cilindro
  - [ ] OK    Fix: 

- **`dome`**
  - EN: Dome
  - IT: Cupola
  - [ ] OK    Fix: 

## `tabs`

- **`clusters`**
  - EN: Clusters
  - IT: Cluster
  - [ ] OK    Fix: 

- **`inputs`**
  - EN: Inputs
  - IT: Ingressi
  - [ ] OK    Fix: 

- **`map`**
  - EN: Map
  - IT: Mappa
  - [ ] OK    Fix: 

- **`network`**
  - EN: Network
  - IT: Rete
  - [ ] OK    Fix: 

- **`outputs`**
  - EN: Outputs
  - IT: Uscite
  - [ ] OK    Fix: 

- **`reverb`**
  - EN: Reverb
  - IT: Riverbero
  - [ ] OK    Fix: 

- **`systemConfig`**
  - EN: System Config
  - IT: Config Sistema
  - [ ] OK    Fix: 

## `touchscreens`

- **`button`**
  - EN: Setup
  - IT: Configura
  - [ ] OK    Fix: 

- **`label`**
  - EN: Touchscreen
  - IT: Schermo tattile
  - [ ] OK    Fix: 

## `units`

- **`celsius`**
  - EN: °C
  - IT: °C
  - [ ] OK    Fix: 

- **`decibelPerMeter`**
  - EN: dB/m
  - IT: dB/m
  - [ ] OK    Fix: 

- **`decibels`**
  - EN: dB
  - IT: dB
  - [ ] OK    Fix: 

- **`degrees`**
  - EN: °
  - IT: °
  - [ ] OK    Fix: 

- **`meters`**
  - EN: m
  - IT: m
  - [ ] OK    Fix: 

- **`metersPerSecond`**
  - EN: m/s
  - IT: m/s
  - [ ] OK    Fix: 

- **`milliseconds`**
  - EN: ms
  - IT: ms
  - [ ] OK    Fix: 

## `wizard.buttons`

- **`back`**
  - EN: Back
  - IT: Indietro
  - [ ] OK    Fix: 

- **`close`**
  - EN: Close
  - IT: Chiudi
  - [ ] OK    Fix: 

- **`done`**
  - EN: Done
  - IT: Fine
  - [ ] OK    Fix: 

- **`gettingStarted`**
  - EN: Getting Started
  - IT: Iniziamo
  - [ ] OK    Fix: 

- **`gettingStartedHelp`**
  - EN: Help cards guiding you through the first parameters to adjust when starting a new project
  - IT: Schede di aiuto che guidano attraverso i primi parametri da regolare all'avvio di un nuovo progetto
  - [ ] OK    Fix: 

- **`next`**
  - EN: Next
  - IT: Avanti
  - [ ] OK    Fix: 

- **`skip`**
  - EN: Skip
  - IT: Salta
  - [ ] OK    Fix: 

## `wizard.steps`

- **`audioDevice.description`**
  - EN: Select your audio driver and device, set the sample rate and buffer size. Check the patch routing and test your outputs. Close this window when done.
  - IT: Seleziona il driver e il dispositivo audio, imposta frequenza di campionamento e dimensione del buffer. Verifica il routing e testa le uscite. Chiudi questa finestra quando hai finito.
  - [ ] OK    Fix: 

- **`audioDevice.title`**
  - EN: Configure the Audio Interface
  - IT: Configura l'interfaccia audio
  - [ ] OK    Fix: 

- **`audioInterface.description`**
  - EN: Click the button above or press Next to open the Audio Interface window.
  - IT: Clicca il pulsante sopra o premi Avanti per aprire la finestra dell'interfaccia audio.
  - [ ] OK    Fix: 

- **`audioInterface.title`**
  - EN: Open the Audio Interface
  - IT: Apri l'interfaccia audio
  - [ ] OK    Fix: 

- **`configureOutputs.description`**
  - EN: Use the array presets and geometry tools to calculate speaker positions for your arrays. Close this window when done.
  - IT: Usa i preset degli array e gli strumenti geometrici per calcolare le posizioni degli altoparlanti. Chiudi questa finestra quando hai finito.
  - [ ] OK    Fix: 

- **`configureOutputs.title`**
  - EN: Configure Output Positions
  - IT: Configura le posizioni di uscita
  - [ ] OK    Fix: 

- **`exploreInputs.description`**
  - EN: Click an input on the map to select it, or lasso several to move them together. Drag to position your sources. Zoom with the mouse wheel or pinch gesture, pan with right-drag or two-finger drag. Add inputs, group them into clusters, and shape your sound field. You can also control positions with a keyboard, SpaceMouse, or other external controllers. Have fun!
  - IT: Clicca un ingresso sulla mappa per selezionarlo, o usa il lazo per selezionarne più di uno. Trascina per posizionare le sorgenti. Zoom con la rotella del mouse o il gesto di pizzico, sposta la vista con il tasto destro o il trascinamento a due dita. Aggiungi ingressi, raggruppali in cluster e modella il tuo campo sonoro. Puoi anche controllare le posizioni con tastiera, SpaceMouse o altri controller. Buon divertimento!
  - [ ] OK    Fix: 

- **`exploreInputs.title`**
  - EN: Start Creating!
  - IT: Inizia a creare!
  - [ ] OK    Fix: 

- **`inputChannels.description`**
  - EN: How many audio sources will you be spatializing?
Set the number of input channels to match your source count.
  - IT: Quante sorgenti audio spazializzerai?
Imposta il numero di canali di ingresso in base alle tue sorgenti.
  - [ ] OK    Fix: 

- **`inputChannels.title`**
  - EN: Set Input Channels
  - IT: Imposta i canali di ingresso
  - [ ] OK    Fix: 

- **`originPoint.description`**
  - EN: The origin is the reference point for all coordinates. Use the preset buttons or enter custom values. 'Front' places it at the audience edge.
  - IT: L'origine è il punto di riferimento per tutte le coordinate. Usa i pulsanti preset o inserisci valori personalizzati. 'Front' lo posiziona al bordo del pubblico.
  - [ ] OK    Fix: 

- **`originPoint.title`**
  - EN: Set the Origin Point
  - IT: Imposta il punto di origine
  - [ ] OK    Fix: 

- **`outputChannels.description`**
  - EN: Set the number of output channels to match your speaker array.
Each output corresponds to one physical speaker.
  - IT: Imposta il numero di canali di uscita in base al tuo array di altoparlanti.
Ogni uscita corrisponde a un altoparlante fisico.
  - [ ] OK    Fix: 

- **`outputChannels.title`**
  - EN: Set Output Channels
  - IT: Imposta i canali di uscita
  - [ ] OK    Fix: 

- **`projectFolder.description`**
  - EN: Choose a folder to store your WFS project files. This will hold configurations, snapshots, IR files, and samples. Click the button to open the folder selector.
  - IT: Scegli una cartella per i file del progetto WFS. Conterrà configurazioni, snapshot, file IR e campioni. Clicca il pulsante per aprire il selettore di cartelle.
  - [ ] OK    Fix: 

- **`projectFolder.title`**
  - EN: Select a Project Folder
  - IT: Seleziona una cartella di progetto
  - [ ] OK    Fix: 

- **`reverbChannels.description`**
  - EN: Reverb channels add room simulation. Set to 0 if you don't need reverb.
  - IT: I canali di riverbero aggiungono simulazione ambientale. Imposta 0 se non hai bisogno del riverbero.
  - [ ] OK    Fix: 

- **`reverbChannels.title`**
  - EN: Set Reverb Channels
  - IT: Imposta i canali di riverbero
  - [ ] OK    Fix: 

- **`stageConfig.description`**
  - EN: Set the shape and dimensions of your performance space. Choose box, cylinder, or dome, then enter the size in meters.
  - IT: Imposta la forma e le dimensioni del tuo spazio. Scegli box, cilindro o cupola, poi inserisci le dimensioni in metri.
  - [ ] OK    Fix: 

- **`stageConfig.title`**
  - EN: Define the Stage
  - IT: Definisci il palco
  - [ ] OK    Fix: 

- **`startProcessing.description`**
  - EN: You're all set! Long-press the Processing button to start the WFS engine. You can also start the Binaural Renderer for headphone monitoring.
  - IT: Tutto pronto! Tieni premuto il pulsante Processing per avviare il motore WFS. Puoi anche avviare il renderer binaurale per il monitoraggio in cuffia.
  - [ ] OK    Fix: 

- **`startProcessing.title`**
  - EN: Start the WFS Engine
  - IT: Avvia il motore WFS
  - [ ] OK    Fix: 

- **`wizardOfOutZ.description`**
  - EN: Click the Wizard of OutZ button or press Next to open the output array helper.
  - IT: Clicca il pulsante Wizard of OutZ o premi Avanti per aprire l'assistente di posizionamento.
  - [ ] OK    Fix: 

- **`wizardOfOutZ.title`**
  - EN: Position Your Outputs
  - IT: Posiziona le uscite
  - [ ] OK    Fix: 


