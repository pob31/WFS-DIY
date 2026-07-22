# Proofreading checklist — Italian (Italiano)

Locale: `it`  |  Total keys: 687  |  Source: `Resources/lang/en.json` vs `Resources/lang/it.json`

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

## `audioPatch.dialogs`

- **`unpatchInputsMessage`**
  - EN: Are you sure you want to remove all input patches?
  - IT: Sei sicuro di voler rimuovere tutti i patch degli ingressi?
  - [ ] OK    Fix: 

- **`unpatchInputsTitle`**
  - EN: Unpatch All Inputs
  - IT: Rimuovi Tutti i Patch Ingressi
  - [ ] OK    Fix: 

- **`unpatchOutputsMessage`**
  - EN: Are you sure you want to remove all output patches?
  - IT: Sei sicuro di voler rimuovere tutti i patch delle uscite?
  - [ ] OK    Fix: 

- **`unpatchOutputsTitle`**
  - EN: Unpatch All Outputs
  - IT: Rimuovi Tutti i Patch Uscite
  - [ ] OK    Fix: 

## `audioPatch.messages`

- **`chooseTestSignal`**
  - EN: Choose a Test Signal to Enable Testing
  - IT: Scegli un Segnale di Test per Abilitare il Test
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
  - EN: Select the reference point for cluster transforms: First Input, Barycenter, or Shared Position (all members coincide; scale and rotation apply to per-input offsets).
  - IT: Selezionare il punto di riferimento per le trasformazioni del cluster: First Input, Baricentro o Shared Position (tutti i membri coincidono; scala e rotazione si applicano agli offset individuali).
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

## `common`

- **`add`**
  - EN: Add
  - IT: Aggiungi
  - [ ] OK    Fix: 

- **`all`**
  - EN: All
  - IT: Tutti
  - [ ] OK    Fix: 

- **`apply`**
  - EN: Apply
  - IT: Applica
  - [ ] OK    Fix: 

- **`cancel`**
  - EN: Cancel
  - IT: Annulla
  - [ ] OK    Fix: 

- **`close`**
  - EN: Close
  - IT: Chiudi
  - [ ] OK    Fix: 

- **`delete`**
  - EN: Delete
  - IT: Elimina
  - [ ] OK    Fix: 

- **`disable`**
  - EN: Disable
  - IT: Disattiva
  - [ ] OK    Fix: 

- **`edit`**
  - EN: Edit
  - IT: Modifica
  - [ ] OK    Fix: 

- **`enable`**
  - EN: Enable
  - IT: Attiva
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - IT: Esporta
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - IT: Importa
  - [ ] OK    Fix: 

- **`load`**
  - EN: Load
  - IT: Carica
  - [ ] OK    Fix: 

- **`no`**
  - EN: No
  - IT: No
  - [ ] OK    Fix: 

- **`none`**
  - EN: None
  - IT: Nessuno
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

- **`reload`**
  - EN: Reload
  - IT: Ricarica
  - [ ] OK    Fix: 

- **`remove`**
  - EN: Remove
  - IT: Rimuovi
  - [ ] OK    Fix: 

- **`reset`**
  - EN: Reset
  - IT: Ripristina
  - [ ] OK    Fix: 

- **`save`**
  - EN: Save
  - IT: Salva
  - [ ] OK    Fix: 

- **`select`**
  - EN: Select
  - IT: Seleziona
  - [ ] OK    Fix: 

- **`store`**
  - EN: Store
  - IT: Memorizza
  - [ ] OK    Fix: 

- **`yes`**
  - EN: Yes
  - IT: Sì
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

## `help.admOsc`

- **`body`**
  - EN: ADM-OSC is a protocol aiming to improve interoperability for spatial sound. It sends Cartesian positions (X, Y, Z) or polar values (AED for Azimuth, Elevation, Distance) from the console or from a DAW's automation curves.\nData is sent normalised:\n- between -1.0 and 1.0 for X, Y and Z;\n- between 0.0 to 1.0 for distance,\n- between -180° to 180° for Azimuth\n- between -90° to 90° for elevation.\nThe origin point can be moved and the mapping can also be adjusted in different segments for the inner and outer parts of the stage.\nWhen dragging the handles on the graphs, holding the shift key will apply symmetrical adjustments on the opposite side.
  - IT: Mappature ADM-OSC\n\nADM-OSC è un protocollo che mira a migliorare l'interoperabilità del suono spaziale. Invia posizioni cartesiane (X, Y, Z) o valori polari (AED per Azimut, Elevazione, Distanza) dalla console o dalle curve di automazione di una DAW.\nI dati vengono inviati normalizzati:\n- tra -1.0 e 1.0 per X, Y e Z;\n- tra 0.0 e 1.0 per la distanza,\n- tra -180° e 180° per l'azimut\n- tra -90° e 90° per l'elevazione.\nIl punto di origine può essere spostato e la mappatura può essere regolata in segmenti diversi per le parti interne ed esterne del palcoscenico.\nTrascinando le maniglie sui grafici, tenendo premuto il tasto Maiusc si applicano regolazioni simmetriche sul lato opposto.
  - [ ] OK    Fix: 

- **`title`**
  - EN: ADM-OSC Mappings
  - IT: ADM-OSC Mappings
  - [ ] OK    Fix: 

## `help.automOtion`

- **`body`**
  - EN: One time movement can also be programmed and triggered manually or triggered by sound level.\nThe coordinates are either relative from the start position or absolute relative to the origin point.\nThe input can either stay at the end position or revert to the starting position.\nInput position cannot be changed while the input is moving this way, but interacting with the input will change the position offset of the input instead.\nFor audio level triggering select the sound level above which the movement will start. When the sound drops below the reset level the movement will be rearmed, ready to be triggered again. New movement automation will not be possible while the input is moving.
  - IT: Movimenti singoli possono essere programmati e attivati manualmente o dal livello sonoro.\nLe coordinate sono relative dalla posizione iniziale o assolute rispetto al punto di origine.\nL'ingresso può rimanere alla posizione finale o tornare alla posizione iniziale.\nLa posizione non può essere modificata durante il movimento, ma l'interazione cambierà l'offset di posizione.\nPer l'attivazione per livello audio, selezionare la soglia. Quando il suono scende sotto il livello di ripristino, il movimento verrà riarmato.
  - [ ] OK    Fix: 

- **`title`**
  - EN: AutomOtion
  - IT: AutomOtion
  - [ ] OK    Fix: 

## `help.binaural`

- **`body`**
  - EN: The Binaural renderer is used for:\n- listening to a rough spatial mix on headphones,\n- creating a mix for stereo output,\n- listening to a single soloed track through the spatial processing.\nThis may take the place of your master mix if it's only feeding headphones and media mix.\nThe position of the listening position may be adjusted in depth from the origin point and in orientation. Delay and level settings allow you to eventually match the sound at the FOH position.
  - IT: Renderer Binaurale\n\nIl Binaural Renderer è utilizzato per:\n- ascoltare un mix spaziale approssimativo in cuffia,\n- creare un mix per uscita stereo,\n- ascoltare una traccia solista attraverso l'elaborazione spaziale.\nPuò sostituire il mix master se alimenta solo cuffie e mix media.\nLa posizione di ascolto può essere regolata in profondità dal punto di origine e in orientamento. Le impostazioni di ritardo e livello permettono di allineare il suono alla posizione FOH.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Binaural Renderer
  - IT: Binaural Renderer
  - [ ] OK    Fix: 

## `help.clusters`

- **`body`**
  - EN: Clusters are groups of inputs that can be manipulated and animated as a whole.\nEach input can only be part of one cluster.\nEach cluster can only have one input with tracking fully enabled. Then this input will become the reference points for the cluster.\nIf no input with tracking is part of the cluster then there are two modes for the reference point of the cluster. Either the first input assigned in the list becomes the reference or the barycentre, in other words the center of gravity or the middle of the shape formed by the assigned inputs.\nAll inputs of the clusters can be moved by dragging the reference point. The individual inputs (other than a first input that would be a reference point) can still be adjusted individually. Dragging an input with tracking activated that is also a reference point for a cluster will affect its position offset and the position of the other inputs of the cluster normally.\nAll inputs in a cluster can be rotated or scaled around the reference point.\nAll clusters can be assigned an animation via an LFO. The positions X, Y and Z, the rotation and scale of the cluster can be controlled. The LFO has a period and a phase setting. Each individual parameter has shape, amplitude, rate and phase. The LFO settings can be assigned to pads for a quick recall. A right click will store the LFO parameters to a pad. Double clicking the top of the pad will allow to edit the name of the preset. Clicking or tapping a pad will recall the settings whether the LFO is running or not, but it will not start it if is isn't. A double click/tap will load and start the LFO.\nAll input clusters share the same set of LFO presets.
  - IT: I cluster sono gruppi di ingressi che possono essere manipolati e animati come un insieme.\nOgni ingresso può far parte di un solo cluster.\nOgni cluster può avere un solo ingresso con tracking completamente abilitato, che diventa il punto di riferimento.\nSe nessun ingresso con tracking è presente, ci sono due modi: il primo ingresso assegnato o il baricentro degli ingressi assegnati.\nTutti gli ingressi possono essere spostati trascinando il punto di riferimento. Gli ingressi individuali possono essere regolati separatamente. Trascinare un ingresso con tracking attivato che è anche punto di riferimento influenzerà il suo offset di posizione e la posizione degli altri ingressi del cluster normalmente.\nTutti gli ingressi di un cluster possono essere ruotati o scalati attorno al punto di riferimento.\nTutti i cluster possono ricevere un'animazione via LFO. Le posizioni X, Y, Z, la rotazione e la scala possono essere controllate. Le impostazioni LFO possono essere assegnate ai pad. Un clic destro memorizzerà i parametri LFO in un pad. Doppio clic sulla parte superiore del pad permette di modificare il nome del preset. Un clic o tap richiama le impostazioni che il LFO sia attivo o meno, ma non lo avvierà. Un doppio clic/tap caricherà e avvierà il LFO.\nTutti i cluster condividono lo stesso set di preset LFO.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Clusters
  - IT: Cluster
  - [ ] OK    Fix: 

## `help.diagnostics`

- **`body`**
  - EN: The diagnostic tools are hidden by default: long-press the Diagnostics button below to show or hide them. They appear automatically when the previous session did not shut down cleanly.\nTo send feedback or report a problem, click Report Issue: it opens the project's GitHub issue tracker in your browser. Describe what happened, what you expected and the steps to reproduce it, then attach the exported diagnostic data.\nExport Logs copies the useful data to a WFS-DIY-logs folder at the location you choose: the logs of the current and up to five previous sessions, plus the application settings file. Attach this folder (or a zip of it) to your report.\nThe session logs contain start-up information (application version, operating system, CPU, channel counts), project loading, network activity and errors. No audio is ever recorded.\nOpen Log Folder shows the raw session logs on disk (the WFS-DIY/logs folder in the user application data directory), useful to find a specific session.\nCopy System Info puts a short summary on the clipboard — application version, operating system, CPU and the current audio device with its sample rate and buffer size — ready to paste into an issue.
  - IT: Gli strumenti di diagnostica sono nascosti per impostazione predefinita: una pressione prolungata sul pulsante Diagnostics li mostra o li nasconde. Compaiono automaticamente se la sessione precedente non si è chiusa correttamente.\nPer inviare un feedback o segnalare un problema, fare clic su «Segnala problema» (Report Issue): si apre l'issue tracker GitHub del progetto nel browser. Descrivere cosa è successo, cosa ci si aspettava e i passaggi per riprodurlo, quindi allegare i dati di diagnostica esportati.\n«Esporta log» (Export Logs) copia i dati utili in una cartella WFS-DIY-logs nella posizione scelta: i log della sessione corrente e di fino a cinque sessioni precedenti, più il file delle impostazioni dell'applicazione. Allegare questa cartella (o uno zip) alla segnalazione.\nI log di sessione contengono le informazioni di avvio (versione dell'applicazione, sistema operativo, CPU, numero di canali), il caricamento dei progetti, l'attività di rete e gli errori. L'audio non viene mai registrato.\n«Apri cartella log» (Open Log Folder) mostra i log grezzi su disco (cartella WFS-DIY/logs nei dati applicazione dell'utente), utile per trovare una sessione specifica.\n«Copia info di sistema» (Copy System Info) mette un breve riepilogo negli appunti — versione dell'applicazione, sistema operativo, CPU e dispositivo audio corrente con frequenza di campionamento e dimensione del buffer — pronto da incollare in una segnalazione.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Diagnostics & Feedback
  - IT: Diagnostica e feedback
  - [ ] OK    Fix: 

## `help.floorReflections`

- **`body`**
  - EN: Simulating the floor reflections improves the naturalness of the sound. We don't expect sounds to play in a soundproof anechoic chamber. This setting helps recreate the expected floor reflections.\nThe level of the floor reflections can be adjusted as well as the low cut and the high frequency shelf filters. Diffusion adds a bit of chaos to simulate the unevenness of the floor.
  - IT: Riflessioni del Pavimento\n\nSimulare le riflessioni del pavimento migliora la naturalezza del suono. Non ci aspettiamo che i suoni vengano riprodotti in una camera anecoica insonorizzata. Questa impostazione aiuta a ricreare le riflessioni del pavimento attese.\nIl livello delle riflessioni del pavimento può essere regolato così come i filtri taglia basso e shelf alte frequenze. La diffusione aggiunge un po' di caos per simulare le irregolarità del pavimento.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Floor Reflections
  - IT: Floor Reflections
  - [ ] OK    Fix: 

## `help.gradientMap`

- **`body`**
  - EN: Gradient maps allow to control attenuation, height and high frequency filtering (shelf with a smooth slope centered at 1kHz) depending on the X, Y position. For example, you can fade out a sound when entering a certain zone, you can have high frequency roll-off when moving away from the front of the stage, you can automatically adjust the height of an actor even when they are standing on elevated platforms without having to control height manually.\nThere are three layers for attenuation, height and HF shelf. They can be toggled on and off and they also can be hidden. The focused layer will look dimmed if disabled. Unfocused layers will look dimmed if active and only the shape outlines will be visible if they are deactivated.\nEach layer has a mapping control for white and black to adjust the range of the effect. The curve setting adjusts the transition.\nEach layer can have editable shapes (rectangle, ellipse or polygon) with either a single shade of grey, a linear gradient or a radial gradient. End points of the gradients can be adjusted.\nWhen creating a polygon click for each corner. Double-clicking will create a last corner and close the shape.\nDouble-clicking an existing point on a rectangle or a polygon will remove this corner. Double-clicking on a side will add a new point.\nThe scale and rotation of each shape can be edited for its center or from the origin point.\nWhen enabled the corner points of the rectangles and polygons can also be edited individually.\nShapes and layers can be copied to another layer on the same input or any other input.\nGradient map settings are stored in the input files.
  - IT: Mappe Gradiente\n\nLe mappe gradiente permettono di controllare attenuazione, altezza e filtraggio alte frequenze (shelf con pendenza morbida centrata a 1kHz) in funzione della posizione X, Y. Ad esempio, è possibile attenuare un suono entrando in una zona, applicare roll-off delle alte frequenze allontanandosi dal fronte del palco o regolare automaticamente l'altezza di un attore su piattaforme rialzate.\nCi sono tre livelli: attenuazione, altezza e shelf HF. Possono essere attivati/disattivati e nascosti.\nOgni livello ha controlli di mappatura bianco e nero per regolare il range dell'effetto. L'impostazione della curva regola la transizione.\nOgni livello può avere forme modificabili (rettangolo, ellisse o poligono) con grigio uniforme, gradiente lineare o radiale.\nPer creare un poligono, cliccare per ogni angolo. Doppio clic chiude la forma.\nDoppio clic su un punto lo rimuove. Doppio clic su un lato aggiunge un punto.\nScala e rotazione possono essere modificate dal centro o dall'origine.\nForme e livelli possono essere copiati.\nLe impostazioni sono salvate nei file di ingresso.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Gradient Maps
  - IT: Gradient Maps
  - [ ] OK    Fix: 

## `help.inputAdvanced`

- **`body`**
  - EN: - Sidelines and Fringe enable muting when an input comes close to the limits of a rectangular stage (except downstage).\n- Tracking can be enabled and the ID of the tracker can be selected. The amount of smoothing on the position can also be adjusted.\n- Maximum Speed can be engaged and the speed limit adjusted. The system will apply a gradual acceleration and deceleration when the movement starts and finishes. When Path is enabled, the system will track the path followed by the input and will not go in a straight line to the final position. This is especially handy if movements have to be operated manually.\n- Height Factor lets you work in 2D, when set to 0%, or full 3D, when set at 100%, and anything in between. It's the ratio of the height in the level and delay calculations. If you wish to use floor reflections, set it to 100% and use the parallax correction of the output parameters.
  - IT: - Le linee laterali e la zona di transizione consentono il mute quando un ingresso si avvicina ai bordi di un palco rettangolare (eccetto il bordo anteriore lato pubblico).\n- Il Tracking può essere attivato e l'ID del tracker selezionato. Anche lo smussamento della posizione può essere regolato.\n- La Velocità Massima può essere attivata e il limite di velocità regolato. Il sistema applicherà accelerazione e decelerazione graduali all'inizio e alla fine del movimento. Quando la modalità Percorso è attiva, il sistema seguirà il tracciato preso dall'ingresso e non andrà in linea retta verso la posizione finale. È particolarmente utile se i movimenti devono essere operati manualmente.\n- Il Fattore di Altezza permette di lavorare in 2D, quando impostato a 0%, o in 3D completo, quando impostato a 100%, e tutto in mezzo. È il rapporto dell'altezza nei calcoli di livello e ritardo. Se desidera utilizzare le Floor Reflections, impostarlo al 100% e usare la correzione di parallasse nei parametri di uscita.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Advanced Controls
  - IT: Controlli avanzati
  - [ ] OK    Fix: 

## `help.inputBasic`

- **`body`**
  - EN: Inputs have a wide variety of settings to account for different situations necessitating realistic sound reinforcement or creative tools for sound design.\n- Input level can be adjusted.\n- Inputs can be delayed or they can try to take into account specific latency (digital processing of wireless transmission or digital effects) and compensate for it to better align the amplification and the acoustic sound.\n- Minimal Latency can be toggled instead of Acoustic Precedence. On the other hand this tries to let the sound out through the system as soon as possible. The system scans this input's feeds to the outputs for lowest delay and subtracts it from all delays and bypasses additional Haas effect. Here the idea would be to beat the acoustic sound on stage to try and place a sound in a slightly different position by altering the location first perceived.\n- The location (position and offset) for any input can be given in Cartesian, Cylindrical or Spherical coordinates independently from the stage shape or other channels.\n- The position can be constrained to the dimensions of the stage in Cartesian coordinates or to a specific distance range in polar coordinates.\n- Flip will take symmetrical position for the given coordinate around the origin point.\n- The joystick and vertical slider allow relative control of the position.\n- Inputs can be assigned to a cluster to group them for coordinated movements.
  - IT: Gli ingressi dispongono di un'ampia varietà di impostazioni per adattarsi a diverse situazioni che richiedono sonorizzazione realistica o strumenti creativi per il sound design.\n- Il livello d'ingresso può essere regolato.\n- Gli ingressi possono essere ritardati o possono tentare di tenere conto di una latenza specifica (elaborazione digitale di trasmissione wireless o effetti digitali) e compensarla per allineare meglio l'amplificazione e il suono acustico.\n- La Minimal Latency può essere attivata al posto dell'Acoustic Precedence. Questo cerca di far uscire il suono attraverso il sistema il più velocemente possibile. Il sistema scansiona gli invii di questo ingresso alle uscite per il ritardo più basso e lo sottrae da tutti i ritardi, bypassando l'effetto Haas aggiuntivo.\n- La posizione (posizione e offset) può essere data in coordinate Cartesiane, Cilindriche o Sferiche indipendentemente dalla forma del palco o dagli altri canali.\n- La posizione può essere vincolata alle dimensioni del palco in coordinate Cartesiane o a un intervallo di distanza specifico in coordinate polari.\n- Flip prenderà la posizione simmetrica per la coordinata data attorno al punto di origine.\n- Il joystick e il cursore verticale permettono il controllo relativo della posizione.\n- Gli ingressi possono essere assegnati a un cluster per raggrupparli in movimenti coordinati.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Inputs Basic Parameters
  - IT: Parametri di base degli ingressi
  - [ ] OK    Fix: 

## `help.inputHF`

- **`body`**
  - EN: When talking away, the timbre of a voice will sound less bright. Reproducing this was the initial goal here even though we usually want to have support for voices when not addressing the audience or in bi-frontal configurations. This can be put to good use for creative effects such as having a brighter reverb on a dampened direct sound.\nThe orientation of the input in azimuth and in pitch can be set as well as the angle where the high frequencies will not be filtered.\nThe HF Shelf will set the maximum attenuation in the back of the input. There is a smooth fade (like a cosine curve) from full brightness in front to damped at the rear.
  - IT: Quando si parla girandosi, il timbro di una voce suona meno brillante. Riprodurre questo era l'obiettivo iniziale qui, anche se generalmente si desidera avere supporto per le voci quando non si rivolgono al pubblico o in configurazioni bi-frontali. Questo può essere usato per effetti creativi come avere un riverbero più brillante su un suono diretto attenuato.\nL'orientamento dell'ingresso in azimut e in elevazione può essere impostato così come l'angolo dove le alte frequenze non saranno filtrate.\nL'HF Shelf imposterà l'attenuazione massima sul retro dell'ingresso. C'è una dissolvenza graduale (come una curva coseno) dalla piena brillantezza davanti all'attenuazione dietro.
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
  - EN: You can set for each output array a specific attenuation for the selected input.\nYou can mute each send to any output individually. There are macros to speed up the process.
  - IT: Attenuazione per array e mute di uscita\n\nPuoi impostare per ogni array di uscita un'attenuazione specifica per l'ingresso selezionato.\nPuoi silenziare ogni invio a qualsiasi uscita individualmente. Sono disponibili macro per velocizzare il processo.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Array Attenuation and Output Mutes
  - IT: Array Attenuation and Output Mutes
  - [ ] OK    Fix: 

## `help.lfo`

- **`body`**
  - EN: Input position can be automated. The LFO can control X, Y and Z position coordinates individually as well as the rotation of HF directivity (gyrophone).\nAdjust the global period and phase for the LFO.\nFor X, Y and Z coordinates select a shape, amplitude, rate and phase as desired. A circle in the XY plane would have sine for shape for X and for Y and an offset of ±90° between the two. A square would be the same but with keystone shapes.\nInput position can be moved while the LFO is running.
  - IT: La posizione dell'ingresso può essere automatizzata. Il LFO può controllare le coordinate X, Y e Z individualmente e la rotazione della direttività HF (girofono).\nRegolare il periodo e la fase globali del LFO.\nPer X, Y e Z selezionare forma, ampiezza, tasso e fase. Un cerchio nel piano XY userebbe forma sinusoidale per X e Y con ±90° di sfasamento. Un quadrato sarebbe uguale ma con forme trapezoidali.\nLa posizione può essere spostata mentre il LFO è attivo.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Input LFO
  - IT: LFO Ingresso
  - [ ] OK    Fix: 

## `help.liveSource`

- **`body`**
  - EN: A loud source on stage may not need much reinforcement through the nearby speakers. Imagine an opera singer near the edge of the stage. Normally the level distribution would make the level louder near the input position. But if it is already loud enough we should be able to not over-amplify it. This function manages this.\nThe radius and shape describe how to attenuate the level for a specific source for speakers within the radius of influence of this input. There are various shapes: a V shaped linear effect; a U shape for fast decrease within the radius; a tight V effect or a mix of the previous (sine).\nThe attenuation can be constant or level dependent like a local compression reacting to transients and to average RMS level.
  - IT: Attenuatore Sorgente Live\n\nUna sorgente potente sul palco potrebbe non aver bisogno di rinforzo attraverso gli altoparlanti vicini. Immaginate un cantante d'opera vicino al bordo del palco. Normalmente la distribuzione del livello renderebbe il suono più forte vicino alla posizione dell'ingresso. Ma se è già abbastanza forte, non dovremmo sovra-amplificarlo. Questa funzione gestisce questo.\nIl raggio e la forma descrivono come attenuare il livello per gli altoparlanti nel raggio d'influenza di questa sorgente. Ci sono varie forme: un effetto lineare a V; una U per diminuzione rapida; una V stretta o un mix dei precedenti (seno).\nL'attenuazione può essere costante o dipendente dal livello, come una compressione locale che reagisce ai transienti e al livello RMS medio.
  - [ ] OK    Fix: 

- **`legendAttenuation`**
  - EN: attenuation
  - IT: attenuazione
  - [ ] OK    Fix: 

- **`legendLinear`**
  - EN: linear
  - IT: lineare
  - [ ] OK    Fix: 

- **`legendLog`**
  - EN: log
  - IT: log
  - [ ] OK    Fix: 

- **`legendMaxAttenuation`**
  - EN: maximum attenuation
  - IT: attenuazione massima
  - [ ] OK    Fix: 

- **`legendNoAttenuation`**
  - EN: no attenuation
  - IT: nessuna attenuazione
  - [ ] OK    Fix: 

- **`legendPosition`**
  - EN: position of the source
  - IT: posizione della sorgente
  - [ ] OK    Fix: 

- **`legendRadius`**
  - EN: radius
  - IT: raggio
  - [ ] OK    Fix: 

- **`legendSine`**
  - EN: sine
  - IT: sinusoide
  - [ ] OK    Fix: 

- **`legendSquare`**
  - EN: square x²
  - IT: quadrato x²
  - [ ] OK    Fix: 

- **`title`**
  - EN: Live Source Tamer
  - IT: Live Source Tamer
  - [ ] OK    Fix: 

## `help.map`

- **`body`**
  - EN: - A left click on an input or a cluster will allow to move it by dragging it. A single finger touch will do the same.\n- A left click with the shift key pressed will add or remove inputs to the selection. A double tap and drag will act the same way.\n- A left click drag will draw a selection rectangle to select multiple inputs and clusters at the same time.\n- A left double-click or tap will reset the position offset of the input.\n- A long left click or press with no movement will switch to the input tab with the focus on the selected input on release.\n- A left click away from any input will clear the selection.\n- A right click and drag will pan the view of the map. A two finger drag with no selected input or cluster will do the same if your operating system supports multitouch.\n- The mouse wheel will zoom in and out. A two finger pinch with no selected input or cluster will also zoom in and out.\n- A middle click will reset the view to fit the stage on the map display. There is also a dedicated set of buttons to reset the view to fit all inputs and to fit the stage respectively.\n- Selected inputs and clusters can also be moved with the arrow keys for X and Y and with the PageUp and PageDown keys for height. Hardware controllers can be used too.\n- When an input is touched, a second finger nearby can rotate the input directivity and adjust the height by pinching if your operating system allows multitouch interaction.\n- When a cluster is touched, a second finger nearby can rotate the cluster and scale it by pinching.\n- Inputs, output arrays and the reverb nodes can be hidden on the map.\n- Inputs can also be locked to prevent selecting and moving them on the map. They will still be moved by clusters, network commands, tracking and hardware controllers.\n- All reverb nodes can be moved on the map if this is enabled on the reverb tab. Holding the Ctrl/Cmd key will move each pair of reverb nodes in symmetry.\n- Inputs with offsets, LFO or with speed regulation will have a temporary position marker. But the point of interaction will remain the normal marker.\n- The Live Source Tamer radius will be displayed around input when activated.\n- There is a toggle to display the audio level for the inputs and outputs on the map tab, that's active when the audio processing is running.
  - IT: - Clic sinistro su un ingresso o cluster per spostarlo trascinando.\n- Clic sinistro con Maiusc aggiunge o rimuove ingressi dalla selezione.\n- Clic sinistro trascinato disegna un rettangolo di selezione.\n- Doppio clic ripristina l'offset di posizione.\n- Clic lungo senza movimento passa alla scheda dell'ingresso selezionato.\n- Clic fuori da qualsiasi ingresso cancella la selezione.\n- Clic destro trascinato sposta la vista della mappa. Trascinamento a due dita anche.\n- La rotella del mouse fa zoom. Pizzico a due dita anche.\n- Clic centrale ripristina la vista.\n- I tasti freccia spostano X/Y, PagSu/Giù l'altezza.\n- Un secondo dito può ruotare la direttività e regolare l'altezza.\n- Nei cluster, un secondo dito può ruotare e scalare.\n- Gli ingressi, array di uscita e nodi di riverbero possono essere nascosti.\n- Gli ingressi possono essere bloccati.\n- I nodi di riverbero possono essere spostati. Ctrl/Cmd sposta le coppie in simmetria.\n- Il raggio del Live Source Tamer viene visualizzato quando attivato.\n- I livelli audio possono essere visualizzati sulla mappa.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Map
  - IT: Mappa
  - [ ] OK    Fix: 

## `help.mcp`

- **`body`**
  - EN: The MCP server lets an AI assistant (Claude Desktop, Claude Code, ChatGPT with custom connectors) read and write the parameters of this WFS-DIY session over a local network connection.\n\nWhat the AI can do:\n• Read live state: channel counts, names, positions, attenuations, EQs, snapshots, clusters, the full parameter surface.\n• Move sources, rename channels, set cluster assignments, adjust the array layout, place outputs and reverbs.\n• Run guided workflows (system tuning walkthroughs, troubleshooting localization, snapshot management) via prepared prompt templates.\n\nOperator controls on this row:\n• AI: ON / OFF — master switch. When OFF every AI tool call is refused; when ON the AI works under the rules below.\n• AI critical actions: blocked / ALLOWED — the destructive actions (deleting snapshots, resetting DSP, changing channel counts) are blocked by default. Click to allow them for 10 minutes; the red fill drains as the window expires, then they auto-block again.\n• Open AI History — scrollable timeline of every recent AI change with per-row undo / redo and a step-by-step cursor.\n• The MCP URL button copies the server URL to the clipboard for AI clients that take a URL directly.\n\nOperator awareness:\n• Every AI action is recorded with origin tags. The AI History window shows the full timeline; per-row × reverses an action with its dependents.\n• If you manually adjust a parameter the AI just moved, the AI is notified and will not blindly retry. You always have the last word.\n• The Cmd/Ctrl+Alt+Z and Cmd/Ctrl+Alt+Y shortcuts undo and redo the last AI change without affecting your manual edits (which use plain Ctrl+Z as usual).\n\nTo add this server to Claude Desktop:\n  1. Open Settings → Developer → Edit Config.\n  2. Paste the JSON snippet below into claude_desktop_config.json (merge into the existing mcpServers block if you already have one).\n  3. Restart Claude Desktop. The server appears as 'wfs-diy' in the tools menu.\n\nTo add to Claude Code, run:\n  claude mcp add wfs-diy <url> -t http\n\nThe URL changes if you switch network interface or if the server falls back to a different port. The URL button on this row always reflects the live URL.
  - IT: Server IA / MCP\n\nIl MCP Server consente a un assistente IA (Claude Desktop, Claude Code, ChatGPT con connettori personalizzati) di leggere e scrivere i parametri di questa sessione WFS-DIY tramite una connessione di rete locale.\n\nCosa può fare l'IA:\n• Leggere lo stato in diretta: numero di canali, nomi, posizioni, attenuazioni, EQ, snapshot, cluster, l'intera superficie dei parametri.\n• Spostare sorgenti, rinominare canali, impostare assegnazioni di cluster, regolare il layout degli array, posizionare uscite e riverberi.\n• Eseguire flussi di lavoro guidati (assistenti di accordatura del sistema, risoluzione di problemi di localizzazione, gestione degli snapshot) tramite modelli di prompt preparati.\n\nControlli operatore in questa riga:\n• IA: ON / OFF — interruttore principale. In OFF, ogni chiamata IA viene rifiutata; in ON, l'IA opera secondo le regole sottostanti.\n• Azioni IA critiche: bloccate / CONSENTITE — le azioni distruttive (eliminare snapshot, ripristinare DSP, modificare il numero di canali) sono bloccate per impostazione predefinita. Cliccare per consentirle per 10 minuti; il riempimento rosso si svuota allo scadere della finestra, poi si blocca automaticamente.\n• Apri Cronologia IA — cronologia scorrevole di ogni modifica IA recente con annulla/ripristina per riga.\n• Il pulsante URL MCP copia l'URL del server negli appunti per i client IA che accettano un URL direttamente.\n\nConsapevolezza dell'operatore:\n• Ogni azione IA viene registrata con tag di origine. La finestra Cronologia IA mostra l'intera cronologia; la × per riga inverte un'azione con le sue dipendenze.\n• Se regola manualmente un parametro che l'IA ha appena spostato, l'IA viene avvisata e non riproverà ciecamente. Ha sempre l'ultima parola.\n• Le scorciatoie Cmd/Ctrl+Alt+Z e Cmd/Ctrl+Alt+Y annullano e ripristinano l'ultima modifica IA senza influire sulle modifiche manuali (che usano Ctrl+Z normale).\n\nPer aggiungere questo server a Claude Desktop:\n  1. Aprire Impostazioni → Sviluppatore → Modifica configurazione.\n  2. Incollare il frammento JSON sottostante in claude_desktop_config.json (unire al blocco mcpServers esistente se ne ha già uno).\n  3. Riavviare Claude Desktop. Il server appare come 'wfs-diy' nel menu strumenti.\n\nPer aggiungere a Claude Code, eseguire:\n  claude mcp add wfs-diy <url> -t http\n\nL'URL cambia se si cambia Network Interface o se il server fa fallback su una porta diversa. Il pulsante URL in questa riga riflette sempre l'URL in diretta.
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
  - IT: AI / MCP Server
  - [ ] OK    Fix: 

## `help.network`

- **`body`**
  - EN: The system can communicate through several network protocols, (UDP or TCP) using OSC. OSC query can be enabled to allow the clients to discover the possible OSC paths and subscribe to some parameter updates.\nThe IP of the local machine corresponding to the selected network interface is shown. The incoming TCP and UDP ports as well as the OSC Query port.\nThere are a few specialised OSC clients such as:\n- Remote for the Android application for multitouch operation and for remote control.\n- QLab that can send data and that can also be programmed directly from the application.\n- ADM-OSC for control from consoles and DAW (see specific help).\nThe data can be filtered to only allow the data from the recorded IP addresses or to allow any client sending on the correct ports.\nThere is a Log window to see what data comes in or out, filter by the type of protocol, client and so on.\nThere is also a locator function to find a lost remote Android tablet. It will flash and sound the alarm on the missing device.
  - IT: Il sistema può comunicare attraverso diversi protocolli di rete (UDP o TCP) usando OSC. OSC Query può essere abilitato per permettere ai client di scoprire i percorsi OSC possibili e sottoscrivere aggiornamenti dei parametri.\nViene mostrato l'IP della macchina locale corrispondente all'interfaccia di rete selezionata. Le porte TCP e UDP in entrata e la porta OSC Query.\nCi sono alcuni client OSC specializzati come:\n- Remote per l'applicazione Android per operazioni multitouch e controllo remoto.\n- QLab che può inviare dati e può essere programmato direttamente dall'applicazione.\n- ADM-OSC per il controllo da console e DAW (vedere l'aiuto specifico).\nI dati possono essere filtrati. Una finestra di Log mostra i dati in entrata e in uscita.\nC'è anche una funzione di localizzazione per trovare tablet Android smarriti.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Network
  - IT: Rete
  - [ ] OK    Fix: 

## `help.outputAdvanced`

- **`body`**
  - EN: There are a few parameters to help you adjust to the acoustic sound.\nMost of these parameters are set for whole arrays unless the propagation mode is switched to off for this output in the array. Relative change can also be selected after a specific setting.\n- Orientation and On/Off Angles define what inputs each speaker will amplify. By default the speakers are pointing to the audience, away from the stage. Inputs in the green sector will be amplified, but not the ones in front of the speaker, in the red sector. There is a fade between both sectors. For sub-bass speakers which usually come in limited numbers and locations, opening all the way to the maximum will allow you to have all inputs possibly picked up by the subwoofers.\n- HF Damping simulates the loss of high frequency with distance. Speakers close to the listeners can have more than speakers away from the stage and the listeners.\n- Distance Attenuation percentage allows you to define if more or less of the attenuation calculated from the distance and parameters of the input is applied. Again for Sub-bass in case you only have two and don't want to lose too much level or have hot-spots it may be wise to lower this setting to 50%.\n- Minimal Latency allows or excludes this speaker from Minimal Latency processing. When allowed, the output is scanned for the smallest calculated delay and, once the setting is engaged on an input, the delay for that input through this speaker is reduced.\n- Live Source Attenuation allows or excludes this speaker from the level reduction of nearby inputs that have this setting enabled. This may not be necessary for speakers away from the audience or for sub-bass.\n- Floor Reflections allows or excludes this speaker from the reflections applied to the signal, such as sub-bass and flown arrays...
  - IT: Ci sono alcuni parametri per aiutarvi ad adattarvi al suono acustico.\nLa maggior parte di questi parametri è impostata per interi array a meno che la modalità di propagazione sia disattivata per questa uscita nell'array. Può anche essere selezionata una modifica relativa dopo un'impostazione specifica.\n- Orientamento e Angoli On/Off definiscono quali ingressi ogni altoparlante amplificherà. Di default gli altoparlanti puntano al pubblico, lontano dal palco. Gli ingressi nel settore verde saranno amplificati, ma non quelli davanti all'altoparlante, nel settore rosso. C'è una dissolvenza tra i due settori. Per gli altoparlanti sub-bass, aprire completamente al massimo permetterà di avere tutti gli ingressi potenzialmente captati dai subwoofer.\n- L'Attenuazione HF simula la perdita di alte frequenze con la distanza.\n- La percentuale di Distance attenuation definisce quanta dell'attenuazione calcolata viene applicata. Per i Sub-bass può essere saggio abbassare al 50%.\n- La Minimal Latency attiva la scansione del ritardo calcolato più piccolo.\n- L'Attenuazione Live Source attiva la riduzione di livello degli ingressi vicini.\n- Le Floor Reflections attivano se le riflessioni vengono applicate al segnale per questa uscita come sub-bass e array sospesi...
  - [ ] OK    Fix: 

- **`title`**
  - EN: Advanced Parameters
  - IT: Parametri avanzati
  - [ ] OK    Fix: 

## `help.outputs`

- **`body`**
  - EN: The design of the WFS system has to do with the right choice of equipment and their position. Here is a guide to help you with the design and the tuning of your arrays.\nAn array is a line (straight or curved) of speakers. This is one of the most important concepts in WFS adapted to sound reinforcement and for creative sound design.\nA rule of thumb would be that each listener should hear three speakers of an array to have enough psycho-acoustic cues to feel the direction each sound is coming from. There will be a sweet spot to find between the distance between the speakers and the listeners, their spacing and coverage angle. This is especially true for short range arrays, a.k.a front fills. Speakers with a 120° coverage angle can be spaced by the same distance between the array and the first row of listeners. The number of speakers also depends on the sound pressure level. When placed as a flown array, trapezoidal/asymmetric horns with wide coverage angle (120°) below axis and narrow coverage (60°) on axis will give good coverage for seat rows a bit too far from the ground array and will have enough throw to reach 20m or 30m away while avoiding the walls of the venue where they would create reflections that give away the position of the speakers to the ears of the listeners. Most of the time for larger venues coaxial (elliptical or conical horns) have not enough reach and require one or more delay lines. They are more suited for smaller venues with few rows of seats.\nThe positioning of the speakers in the system can be done through the 'Wizard of OutZ' and the editable presets it has.
  - IT: Il design del sistema WFS riguarda la giusta scelta dell'attrezzatura e il suo posizionamento. Ecco una guida per aiutarvi con il design e l'accordatura dei vostri array.\nUn array è una linea (dritta o curva) di altoparlanti. Questo è uno dei concetti più importanti in WFS adattato alla sonorizzazione e al design sonoro creativo.\nCome regola generale, ogni ascoltatore dovrebbe sentire tre altoparlanti di un array per avere sufficienti indizi psicoacustici per percepire la direzione di ogni suono. Ci sarà un punto ottimale tra la distanza tra gli altoparlanti e gli ascoltatori, la loro spaziatura e l'angolo di copertura. Altoparlanti con angolo di copertura di 120° possono essere spaziati della stessa distanza tra l'array e la prima fila. Il numero dipende anche dal livello di pressione sonora. Come array sospeso, trombe trapezoidali/asimmetriche con ampio angolo di copertura (120°) sotto asse e stretto (60°) in asse daranno buona copertura e portata di 20-30m evitando riflessioni sulle pareti. Gli altoparlanti coassiali generalmente non hanno abbastanza portata per grandi ambienti e richiedono linee di ritardo.\nIl posizionamento può essere fatto tramite il 'Wizard of OutZ' e i suoi preset modificabili.
  - [ ] OK    Fix: 

- **`title`**
  - EN: WFS Array Design
  - IT: Design degli array WFS
  - [ ] OK    Fix: 

## `help.overview`

- **`body`**
  - EN: This WFS spatial sound processor aims to be a tool for natural sound reinforcement and also a creative tool opening new avenues for writing sound in space.\nSome parameters are straightforward: place sound (Map, Tracking, Speed Limiting, Gradient Maps...), work its shape (Attenuation Profile) and its acoustic presence (Directivity, Floor Reflections), give it a one-off motion (AutomOtion) or a repetitive movement (L.F.O). In some cases amplification should be limited around loud sources on stage (Live Source Tamer). All of these functionalities can be stored and recalled internally or with the help of QLab. On the other side the system allows real-time interaction to trigger and move samples, move large clusters of inputs manually or thanks to easily recallable LFO presets.
  - IT: Questo processore spaziale WFS vuole essere uno strumento per la sonorizzazione naturale e anche uno strumento creativo che apre nuove strade per scrivere il suono nello spazio.\nAlcuni parametri sono semplici: posizionare il suono (Mappa, Tracking, Limitazione velocità, Gradient Maps...), lavorare la sua forma (Profilo di attenuazione) e la sua presenza acustica (Direttività, Floor Reflections), dargli un movimento singolo (AutomOtion) o ripetitivo (L.F.O). In alcuni casi l'amplificazione dovrebbe essere limitata attorno a sorgenti potenti sul palco (Live Source Tamer). Tutte queste funzionalità possono essere memorizzate e richiamate internamente o con l'aiuto di QLab. D'altra parte il sistema permette l'interazione in tempo reale per attivare e spostare campioni, spostare grandi cluster di ingressi manualmente o grazie a preset LFO facilmente richiamabili.
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
  - EN: Reverb helps blur the actual reflections of the speakers in the acoustic space. Without some reverb the listener might perceive the sound missing some depth because the reflections of the speakers in the acoustic of the room gives away their positions and all sounds might feel as coming from the plane of the speakers.\nPlace the reverb node according to the number of channels and the geometry of the stage and listening area, and the positions of speakers. If necessary the return position can be offset from the feed position.\nOther parameters are very similar to Outputs' and Inputs'.
  - IT: Il riverbero aiuta a sfumare le riflessioni degli altoparlanti.\nPosizionare i nodi secondo la geometria del palco.\nAltri parametri sono simili a quelli di Uscite e Ingressi.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb
  - IT: Riverbero
  - [ ] OK    Fix: 

## `help.reverbAlgo`

- **`body`**
  - EN: There are three built-in reverb types inside this spatial sound processor:\n- SDN (Scattered Delay Network): The sound bounces between every reverb node which act as reflecting surfaces. This reverb algorithm favours an odd number of nodes that should not have too much symmetry. This helps reduce the artifacts of the reverb or any metallic sounding resonance.\n- FDN (Feedback Delay Network): Each reverb node functions as a separate reverb processor with a classical reverb algorithm. Place nodes around the stage and eventually around the audience. Inputs are sent to each node in a similar fashion as with outputs. The return signal from the reverb is then distributed like other inputs to all outputs.\n- IR (Impulse Response): This is a classical convolution reverb. You can load audio samples as impulse responses. Each reverb node can share the same IR or they can have different ones. Place nodes around the stage and eventually around the audience.\nThe node positions can be adjusted directly on the map. The Ctrl/Cmd key will move a reverb node pair in symmetry.
  - IT: Tre tipi di riverbero sono integrati in questo processore di suono spaziale:\n- SDN (Scattered Delay Network): Il suono rimbalza tra ogni nodo di riverbero che agisce come superficie riflettente. Questo algoritmo favorisce un numero dispari di nodi senza troppa simmetria, per ridurre artefatti o risonanze metalliche.\n- FDN (Feedback Delay Network): Ogni nodo funziona come un processore di riverbero separato con un algoritmo classico. Posizionare i nodi intorno al palco e eventualmente intorno al pubblico.\n- IR (Risposta all'Impulso): Riverbero a convoluzione classico. È possibile caricare campioni audio come risposte all'impulso. Ogni nodo può condividere la stessa IR o usarne diverse.\nLe posizioni dei nodi possono essere regolate direttamente sulla mappa. Il tasto Ctrl/Cmd sposta una coppia di nodi in simmetria.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Algorithms
  - IT: Algoritmi di Riverbero
  - [ ] OK    Fix: 

## `help.reverbFeed`

- **`body`**
  - EN: Pre-processing sending of the input channels to the reverb nodes.\n- Orientation and On/Off Angles define what inputs each reverb node will receive. Inputs in the green sector will be picked-up, but not the ones in front in the red sector. There is a fade between both sectors.\n- HF Damping simulates the loss of high frequency with distance.\n- Distance Attenuation percentage allows you to define if more or less of the attenuation calculated from the distance and parameters of the input is applied.\n- Minimal Latency toggles whether the output is scanned for the smallest calculated delay and also if the setting once engaged on an input will reduce the delay for that input through this reverb node.\n- Live Source Attenuation toggles the reduction of level of nearby input if they have this setting enabled.
  - IT: Alimentazione Riverbero\n\nInvio pre-elaborazione dei Input Channels ai nodi.\n- Orientamento e angoli On/Off definiscono quali ingressi riceve ogni nodo.\n- Smorzamento HF simula la perdita di alta frequenza.\n- Percentuale di attenuazione distanza definisce l'attenuazione applicata.\n- Minimal Latency determina se viene utilizzato il ritardo minimo.\n- Attenuazione Live Source riduce il livello degli ingressi vicini.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Feed
  - IT: Reverb Feed
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
  - EN: Post-processing sending to the speakers.\n- Distance Attenuation defines the level drop per meter to the speakers.\n- Common Attenuation will look for the lowest attenuation and keep only a percentage of this level drop and apply this across all other outputs.\n- Mutes and Mute Macros allow to prevent a reverb channel from feeding an output. For instance it may not be necessary to send reverb to sub-bass speakers.
  - IT: Ritorno Riverbero\n\nPost-elaborazione inviata agli altoparlanti.\n- Attenuazione Distanza definisce il calo di livello per metro.\n- Attenuazione Comune mantiene una percentuale dell'attenuazione più bassa.\n- Mute impediscono a un canale di riverbero di alimentare un'uscita.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Return
  - IT: Reverb Return
  - [ ] OK    Fix: 

## `help.sampler`

- **`body`**
  - EN: The sampler allows to trigger samples and interact with them in real time.\nThe sampler when enabled on a track will replace the live input at all times.\nSeveral samplers can be assigned to different inputs and triggered individually.\nTo use the sampler:\n- Select a Roli Lightpad or a pad on the connected Android Remote app.\n- Add samples to the different tiles in the grid to the left. Adjust their relative starting position and their level and eventually their in and out points. Several samples can be selected using the shift key while clicking.\n- Create sets of samples: selected samples will be added to new sets. Samples can be added or removed after the creation of a set by holding Ctrl/Cmd while clicking on the tiles. Each set can be renamed. Each set can either have a fixed sequence or a random order (round robin, each sample is played once before a new random order is drawn). Each set has an attenuation setting. Each set has a base position applied to the input when selecting the set. It can be moved on the map or using external control. The sample position offset is added to the set position each time a sample is triggered.\n- Press a Roli Lightpad or a pad on the Android app to trigger a sample. The pressure applied to the pad can be mapped to any of the following controls: level, height and high frequency filtering. The sensitivity can be adjusted for each. The movement of the finger on the pad will cause the sound to move. This acts by measuring the deflection from the initial contact point like a joystick. This can be disabled. All sets have their respective settings for the interaction.\nReleasing the pad will stop the triggered sample.\nSampler settings are stored in the input files.\nFor convenience sample tiles and sets can be copied, exported and imported.
  - IT: Il sampler permette di attivare campioni e interagire con essi in tempo reale.\nQuando abilitato su una traccia, il sampler sostituisce l'ingresso live in ogni momento.\nDiversi sampler possono essere assegnati a ingressi diversi e attivati individualmente.\nPer usare il sampler:\n- Selezionare un Roli Lightpad o un pad sull'app Android Remote collegata.\n- Aggiungere campioni alle diverse celle della griglia. Regolare la posizione di partenza relativa, il livello e i punti di ingresso e uscita. Più campioni possono essere selezionati tenendo premuto Maiusc mentre si clicca.\n- Creare set di campioni: i campioni selezionati verranno aggiunti ai nuovi set. Possono essere aggiunti o rimossi tenendo premuto Ctrl/Cmd mentre si clicca sulle celle. Ogni set può essere rinominato e avere una sequenza fissa o casuale. Ogni set ha un'impostazione di attenuazione e una posizione base.\n- Premere un Lightpad o pad per attivare un campione. La pressione può essere mappata a livello, altezza e filtraggio alte frequenze. Il movimento del dito sposta il suono come un joystick.\nRilasciare il pad ferma il campione.\nLe impostazioni del sampler sono salvate nei file di ingresso.\nCelle e set possono essere copiati, esportati e importati.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Sampler
  - IT: Sampler
  - [ ] OK    Fix: 

## `help.sessionData`

- **`body`**
  - EN: When starting a session select the working folder where the system will place the files and eventual audio files. For new projects create a new folder. When reloading a previous session navigate to the corresponding folder.\nEach section has a separate xml file (System configuration, Network, Outputs, Reverbs, Inputs) and backups. Additionally convolution reverb impulse responses and audio samples will be stored in sub-directories.\nEach section can be stored and recalled individually or as a whole.\nEach section can also export and import files from other projects.
  - IT: All'avvio di una sessione, selezionare la cartella di lavoro dove il sistema posizionerà i file e gli eventuali file audio. Per nuovi progetti, creare una nuova cartella. Per ricaricare una sessione precedente, navigare alla cartella corrispondente.\nOgni sezione ha un file xml separato (Configurazione sistema, Rete, Uscite, Riverbero, Ingressi) e backup. Le risposte all'impulso del riverbero a convoluzione e i campioni audio saranno memorizzati in sottodirectory.\nOgni sezione può essere memorizzata e richiamata individualmente o nel suo insieme.\nOgni sezione può anche esportare e importare file da altri progetti.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Session Data
  - IT: Dati di Sessione
  - [ ] OK    Fix: 

## `help.shortcuts`

- **`body`**
  - EN: *H* opens the help card closest to the pointer.\n*I*, *O* and *R* open the Input, Output and Reverb tabs respectively; for a few seconds afterwards you can type a channel number to select it (confirm with *Enter*).\n*N* opens the Network tab.\n*C* opens the Clusters tab.\n*M* opens the Map tab.\n*Spacebar* scrolls to the next channel and *Shift+Spacebar* to the previous one in the Input, Output and Reverb tabs. On the Clusters tab they cycle through the clusters.\n*Ctrl/Cmd* while adjusting a parameter of an output channel that is part of an array adjusts the parameter for the selected channel only, temporarily disabling the propagation to the rest of the array.\n*F1* to *F10* assign inputs to the corresponding cluster in the Input and Map tabs, assign outputs to the corresponding array in the Output tab, and select the corresponding cluster in the Clusters tab. *F11* sets the channel back to Single.\n*Shift* while adjusting a parameter of an input that is part of a cluster adjusts this parameter for the other inputs of the cluster in relative mode: the variation affects all inputs of the cluster, but relative offsets are kept. *Ctrl/Cmd+Shift* changes the parameter in absolute mode: the value becomes identical across all inputs of the cluster.\n*Ctrl/Cmd+Z* undoes the last change; *Ctrl/Cmd+Y* or *Ctrl/Cmd+Shift+Z* redoes it.
  - IT: *H* apre la scheda di aiuto più vicina al puntatore.\n*I*, *O* e *R* aprono rispettivamente le schede Inputs (ingressi), Outputs (uscite) e Reverb; per alcuni secondi è poi possibile digitare un numero di canale per selezionarlo (confermare con *Invio*).\n*N* apre la scheda Network (rete).\n*C* apre la scheda Clusters.\n*M* apre la scheda Map (mappa).\nLa *barra spaziatrice* passa al canale successivo e *Maiusc+Spazio* al precedente nelle schede Inputs, Outputs e Reverb. Nella scheda Clusters scorrono i cluster.\n*Ctrl/Cmd* mentre si regola un parametro di un'uscita che fa parte di un array regola il parametro solo per il canale selezionato, disattivando temporaneamente la propagazione al resto dell'array.\nDa *F1* a *F10* si assegnano gli ingressi al cluster corrispondente nelle schede Inputs e Map, si assegnano le uscite all'array corrispondente nella scheda Outputs e si seleziona il cluster corrispondente nella scheda Clusters. *F11* riporta il canale a Single.\n*Maiusc* mentre si regola un parametro di un ingresso che fa parte di un cluster regola quel parametro per gli altri ingressi del cluster in modo relativo: la variazione interessa tutti gli ingressi del cluster, ma gli scostamenti relativi vengono mantenuti. *Ctrl/Cmd+Maiusc* modifica il parametro in modo assoluto: il valore diventa identico per tutti gli ingressi del cluster.\n*Ctrl/Cmd+Z* annulla l'ultima modifica; *Ctrl/Cmd+Y* o *Ctrl/Cmd+Maiusc+Z* la ripristina.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Keyboard Shortcuts
  - IT: Scorciatoie da tastiera
  - [ ] OK    Fix: 

## `help.snapshotScope`

- **`body`**
  - EN: Snapshots store input parameters, but can have a scope to be recalled during a performance. They can have between all parameters for all inputs and only one parameter for a single channel. They can be updated and renamed for convenience.\nThe Scope tells the system what data to store or recall. It's the opposite of 'safe' parameters.\nThere are several ways to do this in this application:\n- Record only the needed data in local files. The scope filter is applied when storing the data. A recall cue can be created in QLab to trigger the reading of the local file.\n- Record all data and a filter in local files. The scope filter is applied when recalling the data. This allows to eventually recall all data not taking into account the scope filter. This may come in handy when a complete configuration should be recalled during rehearsal for example. A recall cue can be created in QLab to trigger the reading of the local file.\n- Record all data in scope in QLab cues. This should not be used to recall all parameters for large configurations since QLab may stall when recalling so much data.\nThe scope can show and automatically pre-select the parameters that have been manually changed (local UI, hardware controllers, remote Android application). Changed parameters are marked with a yellow mark.
  - IT: Gli snapshot memorizzano i parametri di ingresso, ma possono avere un ambito per essere richiamati durante uno spettacolo.\nL'Ambito indica al sistema quali dati memorizzare o richiamare.\nSono disponibili diversi metodi:\n- Registrare solo i dati necessari in file locali. Il filtro viene applicato al salvataggio.\n- Registrare tutti i dati e un filtro in file locali. Il filtro viene applicato al richiamo.\n- Registrare tutti i dati nelle cue di QLab. Non raccomandato per configurazioni grandi.\nL'ambito può mostrare e preselezionare automaticamente i parametri modificati manualmente. Le modifiche sono contrassegnate in giallo.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Input Snapshots and Scope
  - IT: Snapshot di Ingresso e Ambito
  - [ ] OK    Fix: 

## `help.tracking`

- **`body`**
  - EN: Tracking allows to follow the position in 2D or 3D of actors and musicians. There are several tracking solutions based on UWB (UltraWide Band) tags, 3D camera, computer vision systems and Infra-Red LED's followed by IR sensitive cameras.\nThis application allows you to receive tracking data from several protocols: OSC, MQTT, PosiStageNet/PSN, RTTrP.\nYou can select which protocol you're using and enter its settings. You may also adjust the mapping (offset, scaling and orientation).\nEach input has a toggle to enable tracking, an ID to select which tracking marker it should follow and a smoothing algorithm to reduce jittery movements.
  - IT: Il tracking permette di seguire la posizione in 2D o 3D di attori e musicisti. Esistono diverse soluzioni basate su tag UWB, telecamere 3D, sistemi di visione artificiale e LED infrarossi con telecamere sensibili all'IR.\nQuesta applicazione permette di ricevere dati di tracking da diversi protocolli: OSC, MQTT, PosiStageNet/PSN, RTTrP.\nÈ possibile selezionare il protocollo utilizzato e configurare le impostazioni. Si può anche regolare il mapping (offset, scala e orientamento).\nOgni ingresso ha un toggle per attivare il tracking, un ID per selezionare il marcatore e un algoritmo di smoothing.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Tracking
  - IT: Tracking
  - [ ] OK    Fix: 

## `help.tuning`

- **`body`**
  - EN: System tuning for WFS is different from standard PA tuning. It can proceed as follows:\n- Start with the flown array muted. Set the desired levels for the near-field speakers when listening to them in the first row. Adjust the high frequency shelf attenuation so the near-field speakers are not too bright and draw too much attention.\n- Mute the near-field array and unmute the flown array, find a suitable level towards the back of the house.\n- Unmute both arrays, adjust the delay of the flown array to bring the sound down to the correct height in the lower rows. Adjust the levels, HF shelf/distance ratio and vertical and horizontal parallax for each array to have consistent level wherever your inputs are on stage. This is especially important if your inputs are actors, singers or acoustic music instruments on stage. You may also test adding some Haas effect to delay the whole system if you feel the WFS is too early or adding comb filtering with the acoustic sound.\nYou may follow a different workflow for tuning or go for different cues.
  - IT: L'accordatura del sistema WFS è diversa dall'accordatura PA standard. Può procedere come segue:\n- Iniziate con l'array sospeso silenziato. Impostate i livelli desiderati per gli altoparlanti di prossimità ascoltandoli dalla prima fila. Regolate l'attenuazione dello shelf di alta frequenza affinché gli altoparlanti di prossimità non siano troppo brillanti.\n- Silenziate l'array di prossimità e attivate l'array sospeso, trovate un livello adeguato verso il fondo della sala.\n- Attivate entrambi gli array, regolate il ritardo dell'array sospeso per portare il suono all'altezza corretta nelle file inferiori. Regolate livelli, shelf HF/rapporto di distanza e parallasse verticale e orizzontale per ogni array per un livello coerente ovunque siano i vostri ingressi sul palco.\nPotete seguire un flusso di lavoro diverso per l'accordatura o mirare a impostazioni diverse per diverse situazioni.
  - [ ] OK    Fix: 

- **`title`**
  - EN: System Tuning
  - IT: Accordatura del sistema
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
  - EN: Input {current} has tracking enabled, but Input {existing} in Cluster {cluster} is already tracked.\n\nOnly one tracked input per cluster is allowed.
  - IT: L'ingresso {current} ha il tracking attivo, ma l'ingresso {existing} nel cluster {cluster} è già tracciato.\n\nÈ consentito solo un ingresso tracciato per cluster.
  - [ ] OK    Fix: 

- **`trackingConflictContinue`**
  - EN: Continue (disable tracking)
  - IT: Continua (disattiva tracking)
  - [ ] OK    Fix: 

- **`trackingConflictSwitch`**
  - EN: Input {existing} in Cluster {cluster} already has tracking enabled.\n\nOnly one tracked input per cluster is allowed.\n\nDo you want to disable tracking on Input {existing} and enable it on Input {to}?
  - IT: L'ingresso {existing} nel cluster {cluster} ha già il tracking attivo.\n\nÈ consentito solo un ingresso tracciato per cluster.\n\nVuoi disattivare il tracking sull'ingresso {existing} e attivarlo sull'ingresso {to}?
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
  - IT: Modello di Attenuation Law (diminuzione lineare del volume con la distanza tra oggetto e altoparlante, o quadratica).
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
  - IT: Attivare le Floor Reflections simulate per l'oggetto.
  - [ ] OK    Fix: 

- **`frAttenuationSlider`**
  - EN: Attenuation of the Simulated Floor Reflections for the Object.
  - IT: Attenuazione delle Floor Reflections simulate per l'oggetto.
  - [ ] OK    Fix: 

- **`frDiffusionDial`**
  - EN: Diffusion Effect of the Simulated Floor Reflections for the Object.
  - IT: Effetto di diffusione delle Floor Reflections simulate per l'oggetto.
  - [ ] OK    Fix: 

- **`frHighShelfActiveButton`**
  - EN: Enable High Shelf Filter for Floor Reflections.
  - IT: Attivare il filtro High Shelf per le Floor Reflections.
  - [ ] OK    Fix: 

- **`frHighShelfFreqSlider`**
  - EN: High Shelf Frequency for Floor Reflections.
  - IT: Frequenza dello High Shelf per le Floor Reflections.
  - [ ] OK    Fix: 

- **`frHighShelfGainSlider`**
  - EN: High Shelf Gain for Floor Reflections.
  - IT: Guadagno dello High Shelf per le Floor Reflections.
  - [ ] OK    Fix: 

- **`frHighShelfSlopeSlider`**
  - EN: High Shelf Slope for Floor Reflections.
  - IT: Pendenza dello High Shelf per le Floor Reflections.
  - [ ] OK    Fix: 

- **`frLowCutActiveButton`**
  - EN: Enable Low Cut Filter for Floor Reflections.
  - IT: Attivare il filtro Low Cut per le Floor Reflections.
  - [ ] OK    Fix: 

- **`frLowCutFreqSlider`**
  - EN: Low Cut Frequency for Floor Reflections.
  - IT: Frequenza del Low Cut per le Floor Reflections.
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
  - IT: Attivare o disattivare il compressore rapido (picco) per il Live Source Tamer.
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
  - IT: Attivare o disattivare il compressore lento per il Live Source Tamer.
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
  - IT: Scegliere tra Acoustic Precedence e Minimal Latency per la precedenza di amplificazione.
  - [ ] OK    Fix: 

- **`muteButton`**
  - EN: Mute Output {num} for this Object.
  - IT: Silenziare l'uscita {num} per questo oggetto.
  - [ ] OK    Fix: 

- **`muteMacrosSelector`**
  - EN: Mute Macros for Fast Muting and Unmuting of Arrays.
  - IT: Mute Macros per silenziare e riattivare rapidamente gli array.
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

## `inputs.messages`

- **`assignedCluster`**
  - EN: Input {channel} assigned to Cluster {cluster}
  - IT: Ingresso {channel} assegnato al Cluster {cluster}
  - [ ] OK    Fix: 

- **`backupLoaded`**
  - EN: Input configuration loaded from backup.
  - IT: Configurazione ingressi caricata dal backup.
  - [ ] OK    Fix: 

- **`clusterEditAbsolute`**
  - EN: Ctrl+Shift edit: copying value to {count} other input(s) of Cluster {cluster}
  - IT: Modifica con Ctrl+Shift: valore copiato su {count} altri ingressi del Cluster {cluster}
  - [ ] OK    Fix: 

- **`clusterEditRelative`**
  - EN: Shift edit: applying relative change to {count} other input(s) of Cluster {cluster}
  - IT: Modifica con Shift: variazione relativa applicata a {count} altri ingressi del Cluster {cluster}
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
  - IT: Seleziona prima una cartella progetto in System Config.
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

- **`snapshotScopeUpdated`**
  - EN: Snapshot '{name}' scope updated.
  - IT: Ambito dello snapshot '{name}' aggiornato.
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

## `meta`

- **`author`**
  - EN: WFS-DIY Team
  - IT: WFS-DIY Team
  - [ ] OK    Fix: 

- **`language`**
  - EN: English
  - IT: Italiano
  - [ ] OK    Fix: 

- **`locale`**
  - EN: en
  - IT: it
  - [ ] OK    Fix: 

- **`version`**
  - EN: 1.0.0
  - IT: 1.0.0
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
  - EN: \nOnly one tracked input per cluster is allowed. If you continue, tracking will be kept only for the first input in each cluster.
  - IT: \nÈ consentito solo un ingresso tracciato per cluster. Se continui, il tracking verrà mantenuto solo per il primo ingresso di ciascun cluster.
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

- **`admAxisSwap`**
  - EN: Which incoming ADM-OSC axis maps to this internal axis.
  - IT: Quale asse ADM-OSC in ingresso viene mappato su questo asse interno.
  - [ ] OK    Fix: 

- **`admAzFlip`**
  - EN: Invert the direction of incoming azimuth.
  - IT: Invertire la direzione dell'azimut in ingresso.
  - [ ] OK    Fix: 

- **`admAzOffset`**
  - EN: Azimuth offset (deg) applied to incoming ADM-OSC azimuth.
  - IT: Offset azimutale (gradi) applicato all'azimut ADM-OSC in ingresso.
  - [ ] OK    Fix: 

- **`admBreakpoint`**
  - EN: Normalized breakpoint (0-1) for piecewise linear stretch.
  - IT: Punto di rottura normalizzato (0-1) per lo stiramento lineare a tratti.
  - [ ] OK    Fix: 

- **`admCenterOffset`**
  - EN: Physical position (m) where normalized 0.0 maps to.
  - IT: Posizione fisica (m) a cui il valore normalizzato 0.0 viene mappato.
  - [ ] OK    Fix: 

- **`admDistMax`**
  - EN: Maximum physical distance (m) at ADM-OSC distance=1.
  - IT: Distanza fisica massima (m) a ADM-OSC distance=1.
  - [ ] OK    Fix: 

- **`admDistMin`**
  - EN: Minimum physical distance (m) at ADM-OSC distance=0.
  - IT: Distanza fisica minima (m) a ADM-OSC distance=0.
  - [ ] OK    Fix: 

- **`admElFlip`**
  - EN: Invert the sign of incoming elevation.
  - IT: Invertire il segno dell'elevazione in ingresso.
  - [ ] OK    Fix: 

- **`admInnerWidth`**
  - EN: Physical extent (m) from center to breakpoint.
  - IT: Estensione fisica (m) dal centro al punto di rottura.
  - [ ] OK    Fix: 

- **`admInputAssign`**
  - EN: Assign this input to an ADM-OSC mapping for receive/transmit.
  - IT: Assegnare questo ingresso a una mappatura ADM-OSC per ricezione/trasmissione.
  - [ ] OK    Fix: 

- **`admLinkAll`**
  - EN: Select all 6 sides at once for uniform editing.
  - IT: Selezionare tutti i 6 lati contemporaneamente per una modifica uniforme.
  - [ ] OK    Fix: 

- **`admMapping`**
  - EN: Select an ADM-OSC mapping to configure. Cart = Cartesian (xyz), Polar = spherical (aed).
  - IT: Selezionare una mappatura ADM-OSC da configurare. Cart = Cartesiano (xyz), Polar = sferico (aed).
  - [ ] OK    Fix: 

- **`admMappingPanel`**
  - EN: Drag dots to edit mapping. Click axis title to swap, click Flip to invert. Hold Shift to edit both sides symmetrically.
  - IT: Trascinare i punti per modificare la mappatura. Cliccare sul titolo dell'asse per scambiare, cliccare su Flip per invertire. Tenere premuto Maiusc per modificare entrambi i lati simmetricamente.
  - [ ] OK    Fix: 

- **`admOuterWidth`**
  - EN: Physical extent (m) from breakpoint to ±1.
  - IT: Estensione fisica (m) dal punto di rottura a ±1.
  - [ ] OK    Fix: 

- **`admSideSelect`**
  - EN: Select sides to edit. Changes apply to all selected sides at once.
  - IT: Selezionare i lati da modificare. Le modifiche si applicano a tutti i lati selezionati contemporaneamente.
  - [ ] OK    Fix: 

- **`admSignFlip`**
  - EN: Invert the sign of the incoming axis value.
  - IT: Invertire il segno del valore dell'asse in ingresso.
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
  - IT: Selezionare l'Network Interface.
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
  - IT: OSC Path per il tracking in modalità OSC (inizia con /)
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
  - IT: Seleziona prima una cartella progetto in System Config.
  - [ ] OK    Fix: 

## `network.remote`

- **`notResponding`**
  - EN: Remote not responding — the tablet app may be outdated or unreachable
  - IT: (missing — falls back to English)
  - [ ] OK    Fix: 

- **`protocolMismatch`**
  - EN: Remote app uses protocol v{remote}, expected v{local} — update the tablet app
  - IT: (missing — falls back to English)
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

## `networkLog.status`

- **`rejected`**
  - EN: REJECTED
  - IT: RIFIUTATO
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
  - IT: Frequenza Output EQ banda {band} (20 Hz - 20 kHz).
  - [ ] OK    Fix: 

- **`eqGain`**
  - EN: Output EQ Band {band} gain (-24 to +24 dB).
  - IT: Guadagno Output EQ banda {band} (-24 a +24 dB).
  - [ ] OK    Fix: 

- **`eqQ`**
  - EN: Output EQ Band {band} Q factor (0.1 - 10.0).
  - IT: Fattore Q Output EQ banda {band} (0.1 - 10.0).
  - [ ] OK    Fix: 

- **`eqResetBand`**
  - EN: Long press to reset Band {band} to its default values.
  - IT: Pressione lunga per resettare la banda {band}.
  - [ ] OK    Fix: 

- **`eqShape`**
  - EN: Output EQ Band {band} filter shape.
  - IT: Forma del filtro Output EQ banda {band}.
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Output Configuration to file (with file explorer window).
  - IT: Esportare la configurazione delle uscite su file (con finestra esplora risorse).
  - [ ] OK    Fix: 

- **`floorReflections`**
  - EN: Allow or exclude this speaker from Floor Reflections.
  - IT: Consentire o escludere questo altoparlante dalle Floor Reflections.
  - [ ] OK    Fix: 

- **`hParallax`**
  - EN: Horizontal Distance from Speaker to 'Targeted' Listener. (changes may affect the rest of the array)
  - IT: Distanza orizzontale dall'altoparlante all'ascoltatore « mirato ». (le modifiche possono influenzare il resto dell'array)
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: Loss of High Frequency Depending on Distance from Object to Output. (changes may affect the rest of the array)
  - IT: Perdita di alte frequenze in funzione della distanza dall'oggetto all'uscita. (le modifiche possono influenzare il resto dell'array)
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Output Configuration from file (with file explorer window).
  - IT: Importare la configurazione delle uscite da file (con finestra esplora risorse).
  - [ ] OK    Fix: 

- **`liveSource`**
  - EN: Allow or exclude this speaker from Live Source Attenuation. (may affect the rest of the array)
  - IT: Consentire o escludere questo altoparlante dall'attenuazione Live Source. (può influenzare il resto dell'array)
  - [ ] OK    Fix: 

- **`mapVisibility`**
  - EN: Make Visible or Hide The Selected Output on the Map.
  - IT: Rendere visibile o nascondere l'uscita selezionata sulla mappa.
  - [ ] OK    Fix: 

- **`minLatency`**
  - EN: Allow or exclude this speaker from Minimal Latency processing. (may affect the rest of the array)
  - IT: Consentire o escludere questo altoparlante dall'elaborazione Minimal Latency. (può influenzare il resto dell'array)
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

## `outputs.messages`

- **`arrayEditSingle`**
  - EN: Ctrl edit: change applied to this output only (Array {array} not affected)
  - IT: Modifica con Ctrl: variazione applicata solo a questa uscita (Array {array} non interessato)
  - [ ] OK    Fix: 

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

- **`configNotFound`**
  - EN: Output config file not found.
  - IT: File config uscite non trovato.
  - [ ] OK    Fix: 

- **`configReloaded`**
  - EN: Output configuration reloaded.
  - IT: Configurazione uscite ricaricata.
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Output configuration saved.
  - IT: Configurazione uscite salvata.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - IT: Errore: {error}
  - [ ] OK    Fix: 

- **`noBackup`**
  - EN: No backup output configuration found.
  - IT: Nessun backup configurazione uscite trovato.
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - IT: Seleziona prima una cartella progetto in System Config.
  - [ ] OK    Fix: 

- **`setToSingle`**
  - EN: Output {num} set to Single
  - IT: Uscita {num} impostata su Singolo
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

- **`algoFDNGpu`**
  - EN: Run the FDN reverb on the GPU (adds ~20 ms to the wet path only) or on the CPU. Falls back to CPU automatically if the GPU is unavailable.
  - IT: Esegue il riverbero FDN sulla GPU (aggiunge ~20 ms solo al segnale wet) o sulla CPU. Passa automaticamente alla CPU se la GPU non è disponibile.
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

- **`algoIRGpu`**
  - EN: Run the IR convolution on the GPU (adds ~20 ms to the wet path only) or on the CPU. Falls back to CPU automatically if the GPU is unavailable.
  - IT: Esegue la convoluzione IR sulla GPU (aggiunge ~20 ms solo al segnale wet) o sulla CPU. Passa automaticamente alla CPU se la GPU non è disponibile.
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

- **`algoSDNGpu`**
  - EN: Run the SDN reverb on the GPU (adds ~20 ms to the wet path only) or on the CPU. Falls back to CPU automatically if the GPU is unavailable.
  - IT: Esegue il riverbero SDN sulla GPU (aggiunge ~20 ms solo al segnale wet) o sulla CPU. Passa automaticamente alla CPU se la GPU non è disponibile.
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

- **`applyToAllNodes`**
  - EN: When ON, parameter edits are applied to every reverb node at once; when OFF, only the selected node. Runtime only - only future edits propagate.
  - IT: Quando attivo, le modifiche dei parametri si applicano a tutti i nodi di riverbero contemporaneamente; quando disattivo, solo al nodo selezionato. Solo a runtime - si propagano solo le modifiche future.
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
  - IT: Distance attenuation per il ritorno di riverbero (-6.0 a 0.0 dB/m).
  - [ ] OK    Fix: 

- **`distanceAttenEnable`**
  - EN: Distance attenuation percentage (0-200%).
  - IT: Percentuale di Distance attenuation (0-200%).
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
  - EN: Allow or exclude this reverb feed from Live Source Attenuation.
  - IT: Consentire o escludere questa mandata di riverbero dall'attenuazione Live Source.
  - [ ] OK    Fix: 

- **`mapVisibility`**
  - EN: Make Visible or Hide All Reverb Channels on the Map.
  - IT: Rendere visibili o nascondere tutti i Reverb Channels sulla mappa.
  - [ ] OK    Fix: 

- **`miniLatencyTooltip`**
  - EN: Allow or exclude this reverb feed from Minimal Latency processing.
  - IT: Consentire o escludere questa mandata di riverbero dall'elaborazione Minimal Latency.
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
  - IT: Seleziona prima una cartella progetto in System Config.
  - [ ] OK    Fix: 

## `sampler`

- **`guide`**
  - EN: Select a cell on the grid to edit its properties.\nDouble-click to load a sample.\nUse Ctrl+Click to assign cells to the active set.
  - IT: Selezionare una cella sulla griglia per modificarne le proprietà.\nDoppio clic per caricare un campione.\nUsa Ctrl+Clic per assegnare le celle al set attivo.
  - [ ] OK    Fix: 

## `sampler.grid`

- **`help`**
  - EN: Click = select\nShift = multi\nCtrl = set toggle\nDblClick = load
  - IT: Clic=seleziona | Shift=multi | Ctrl=alterna set | DblClic=carica
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

- **`pressHF`**
  - EN: Map finger pressure to high-frequency shelf attenuation
  - IT: Mappa la pressione del dito sull'attenuazione dello shelving alto
  - [ ] OK    Fix: 

- **`pressHeight`**
  - EN: Map finger pressure to vertical position (Z)
  - IT: Mappa la pressione del dito sulla posizione verticale (Z)
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
  - EN: Reducing from {current} to {new} input channels will remove settings for channels {start} to {end}.\n\nThis cannot be undone.
  - IT: Riducendo da {current} a {new} canali di ingresso verranno rimosse le impostazioni per i canali da {start} a {end}.\n\nQuesta operazione non può essere annullata.
  - [ ] OK    Fix: 

- **`reduceInputChannels.title`**
  - EN: Reduce Input Channels?
  - IT: Ridurre i Input Channels?
  - [ ] OK    Fix: 

- **`reduceOutputChannels.message`**
  - EN: Reducing from {current} to {new} output channels will remove settings for channels {start} to {end}.\n\nThis cannot be undone.
  - IT: Riducendo da {current} a {new} canali di uscita verranno rimosse le impostazioni per i canali da {start} a {end}.\n\nQuesta operazione non può essere annullata.
  - [ ] OK    Fix: 

- **`reduceOutputChannels.title`**
  - EN: Reduce Output Channels?
  - IT: Ridurre i Output Channels?
  - [ ] OK    Fix: 

- **`reduceReverbChannels.message`**
  - EN: Reducing from {current} to {new} reverb channels will remove settings for channels {start} to {end}.\n\nThis cannot be undone.
  - IT: Riducendo da {current} a {new} canali di riverbero verranno rimosse le impostazioni per i canali da {start} a {end}.\n\nQuesta operazione non può essere annullata.
  - [ ] OK    Fix: 

- **`reduceReverbChannels.title`**
  - EN: Reduce Reverb Channels?
  - IT: Ridurre i Reverb Channels?
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

- **`clearSolo`**
  - EN: Clear all input solo states.
  - IT: Clear all input solo states.
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
  - EN: Select the hardware controller for dials and buttons: Stream Deck+.
  - IT: Selezionare il controller hardware per manopole e tasti: Stream Deck+.
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

- **`gpuPipelineDepth`**
  - EN: GPU pipeline depth in audio blocks. Adds depth x buffer/sample-rate of constant latency (pre-subtracted from WFS delays) and absorbs GPU stalls of the same length: deeper = immune to desktop/UI hiccups, shallower = lower latency. Applies live. Default 4.
  - IT: Profondità della pipeline GPU in blocchi audio. Aggiunge depth x buffer/frequenza di campionamento di latenza costante (pre-sottratta dai ritardi WFS) e assorbe gli stalli della GPU della stessa durata: più profonda = immune agli scatti del desktop/interfaccia, meno profonda = latenza inferiore. Si applica in tempo reale. Predefinito 4.
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
  - IT: Numero di Input Channels.
  - [ ] OK    Fix: 

- **`language`**
  - EN: Select the user interface language. Changes take full effect after restarting the application.
  - IT: Seleziona la lingua dell'interfaccia utente. Le modifiche avranno pieno effetto dopo il riavvio dell'applicazione.
  - [ ] OK    Fix: 

- **`levelMeter`**
  - EN: Opens the Level Meter Window.
  - IT: Opens the Level Meter Window.
  - [ ] OK    Fix: 

- **`lightpadSetup`**
  - EN: Display the connected Roli Lightpads and allow for splitting them in 4 smaller pads.
  - IT: Visualizzare i Roli Lightpad collegati e consentire di dividerli in 4 pad più piccoli.
  - [ ] OK    Fix: 

- **`masterLevel`**
  - EN: Master Level (affects all outputs).
  - IT: Master Level (influisce su tutte le uscite).
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
  - IT: Numero di Output Channels.
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
  - IT: Numero di Reverb Channels.
  - [ ] OK    Fix: 

- **`sampler`**
  - EN: Enable or disable the Sampler feature for input channels.
  - IT: Attivare o disattivare la funzione Sampler per i canali di ingresso. Selezionare il controller: Lightpad o Telecomando.
  - [ ] OK    Fix: 

- **`screenReader`**
  - EN: Enable or disable screen reader announcements. When enabled, parameter names and values are announced on hover, and help text is read after a few seconds.
  - IT: Attiva o disattiva gli annunci del lettore di schermo. Quando attivo, i nomi e i valori dei parametri vengono annunciati al passaggio del mouse, e il testo di aiuto viene letto dopo alcuni secondi.
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
  - IT: Speed of Sound (correlata alla temperatura).
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
  - IT: Temperatura (determina la Speed of Sound).
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

- **`logsExportFailed`**
  - EN: Failed to export logs
  - IT: Esportazione dei log fallita
  - [ ] OK    Fix: 

- **`logsExported`**
  - EN: Logs exported to {path}
  - IT: Log esportati in {path}
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
