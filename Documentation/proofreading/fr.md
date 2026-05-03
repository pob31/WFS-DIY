# Proofreading checklist — French (Français)

Locale: `fr`  |  Total keys: 1520  |  Source: `Resources/lang/en.json` vs `Resources/lang/fr.json`

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
  - FR: appliqué
  - [ ] OK    Fix: 

- **`atHead`**
  - EN: (no undone records — at the head)
  - FR: (aucun enregistrement annulé — en tête)
  - [ ] OK    Fix: 

- **`batch`**
  - EN: batch {id}
  - FR: lot {id}
  - [ ] OK    Fix: 

- **`cursorLabel`**
  - EN: ◂  cursor (↑ applied  /  ↓ undone, redoable)
  - FR: ◂  curseur (↑ appliqué  /  ↓ annulé, rétablissable)
  - [ ] OK    Fix: 

- **`noChanges`**
  - EN: No AI changes yet.
  - FR: Aucune modification IA pour l'instant.
  - [ ] OK    Fix: 

- **`of`**
  - EN: of
  - FR: sur
  - [ ] OK    Fix: 

- **`stepBack`**
  - EN: ⏮ Step Back
  - FR: ⏮ Reculer
  - [ ] OK    Fix: 

- **`stepForward`**
  - EN: Step Forward ⏭
  - FR: Avancer ⏭
  - [ ] OK    Fix: 

- **`undone`**
  - EN: undone
  - FR: annulé
  - [ ] OK    Fix: 

- **`windowTitle`**
  - EN: AI Change History
  - FR: Historique des modifications IA
  - [ ] OK    Fix: 

## `ai.server`

- **`copyUrlConfirm`**
  - EN: MCP URL copied to clipboard: {url}
  - FR: URL MCP copiée dans le presse-papiers : {url}
  - [ ] OK    Fix: 

- **`label`**
  - EN: MCP Server:
  - FR: Serveur MCP :
  - [ ] OK    Fix: 

- **`openHistory`**
  - EN: Open AI History
  - FR: Ouvrir l'historique IA
  - [ ] OK    Fix: 

- **`urlButtonRunning`**
  - EN: http://{host}:{port}/mcp
  - FR: http://{host}:{port}/mcp
  - [ ] OK    Fix: 

- **`urlButtonStopped`**
  - EN: (server not running)
  - FR: (serveur arrêté)
  - [ ] OK    Fix: 

## `ai.tier`

- **`aiOff`**
  - EN: AI: OFF
  - FR: IA : OFF
  - [ ] OK    Fix: 

- **`aiOn`**
  - EN: AI: ON
  - FR: IA : ON
  - [ ] OK    Fix: 

- **`gateAllowed`**
  - EN: AI critical actions: ALLOWED
  - FR: Actions IA critiques : AUTORISÉES
  - [ ] OK    Fix: 

- **`gateBlocked`**
  - EN: AI critical actions: blocked
  - FR: Actions IA critiques : bloquées
  - [ ] OK    Fix: 

- **`tier2AutoOff`**
  - EN: Tier 2 auto-confirm: off
  - FR: Auto-confirmation niveau 2 : off
  - [ ] OK    Fix: 

- **`tier2AutoOn`**
  - EN: Tier 2 auto-confirm: ON (5 min)
  - FR: Auto-confirmation niveau 2 : ON (5 min)
  - [ ] OK    Fix: 

## `ai.toast`

- **`clientPrefix`**
  - EN: Claude
  - FR: Claude
  - [ ] OK    Fix: 

- **`header`**
  - EN: AI changes
  - FR: Modifications IA
  - [ ] OK    Fix: 

- **`moreOlder`**
  - EN: …and {count} older
  - FR: …et {count} plus anciennes
  - [ ] OK    Fix: 

## `ai.tooltips`

- **`aiToggle`**
  - EN: Master switch for the whole MCP integration. When OFF every AI tool call is refused; when ON, normal tier handling applies (the critical-actions toggle still controls destructive calls separately).
  - FR: Interrupteur principal pour l'intégration MCP. Lorsqu'il est sur OFF, tout appel d'outil IA est refusé ; lorsqu'il est sur ON, la gestion normale par niveau s'applique (la bascule des actions critiques contrôle séparément les appels destructeurs).
  - [ ] OK    Fix: 

- **`gate`**
  - EN: Allow destructive AI actions (channel-count changes, port reconfigure, runDSP, etc.) AND skip the per-call confirm handshake for less-destructive tier-2 actions while open. Acts as a superset of the Tier 2 auto-confirm toggle. The red fill drains over 5 minutes, then auto-blocks again.
  - FR: Autoriser les actions IA destructrices (modifications du nombre de canaux, reconfiguration de port, runDSP, etc.) ET ignorer la confirmation par appel pour les actions de niveau 2 moins destructrices tant que la porte est ouverte. Surensemble de la bascule de confirmation automatique de niveau 2. Le remplissage rouge s'écoule sur 5 minutes, puis se bloque automatiquement.
  - [ ] OK    Fix: 

- **`openHistory`**
  - EN: Open the AI History window: scrollable timeline of every recent AI change with per-row undo / redo and a step-by-step cursor.
  - FR: Ouvrir la fenêtre Historique IA : chronologie déroulante de chaque modification IA récente avec annulation/rétablissement par ligne et curseur pas à pas.
  - [ ] OK    Fix: 

- **`urlButton`**
  - EN: Click to copy the MCP server URL. Useful for Claude Code (claude mcp add wfs-diy <URL> -t http) or any MCP client that takes a URL. Claude Desktop instead uses the JSON config snippet — open the (?) help card.
  - FR: Cliquer pour copier l'URL du serveur MCP. Utile pour Claude Code (claude mcp add wfs-diy <URL> -t http) ou tout client MCP qui accepte une URL. Claude Desktop utilise plutôt l'extrait de configuration JSON — ouvrez la fiche d'aide (?).
  - [ ] OK    Fix: 

## `ai.undo`

- **`errorPrefix`**
  - EN: AI {verb}: {message}
  - FR: IA {verb} : {message}
  - [ ] OK    Fix: 

- **`successPrefix`**
  - EN: AI {verb}: {description}
  - FR: IA {verb} : {description}
  - [ ] OK    Fix: 

- **`verbRedo`**
  - EN: redo
  - FR: rétablissement
  - [ ] OK    Fix: 

- **`verbUndo`**
  - EN: undo
  - FR: annulation
  - [ ] OK    Fix: 

## `arrayHelper.acoustic`

- **`distanceAtten`**
  - EN: Distance Atten (%):
  - FR: Atténuation distance (%) :
  - [ ] OK    Fix: 

- **`floorReflections`**
  - EN: Floor Reflections
  - FR: Réflexions sol
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: HF Damping (dB/m):
  - FR: Atténuation HF (dB/m) :
  - [ ] OK    Fix: 

- **`highCut`**
  - EN: High Cut (Hz):
  - FR: Coupe-haut (Hz) :
  - [ ] OK    Fix: 

- **`hParallax`**
  - EN: H Parallax (m):
  - FR: Parallaxe H (m) :
  - [ ] OK    Fix: 

- **`liveSource`**
  - EN: Live Source
  - FR: Source live
  - [ ] OK    Fix: 

- **`lowCut`**
  - EN: Low Cut (Hz):
  - FR: Coupe-bas (Hz) :
  - [ ] OK    Fix: 

- **`vParallax`**
  - EN: V Parallax (m):
  - FR: Parallaxe V (m) :
  - [ ] OK    Fix: 

## `arrayHelper.buttons`

- **`apply`**
  - EN: Apply
  - FR: Appliquer
  - [ ] OK    Fix: 

- **`close`**
  - EN: Close
  - FR: Fermer
  - [ ] OK    Fix: 

## `arrayHelper.errors`

- **`noPositions`**
  - EN: No positions to apply. Check geometry parameters.
  - FR: Aucune position à appliquer. Vérifiez les paramètres de géométrie.
  - [ ] OK    Fix: 

- **`notEnoughOutputs`**
  - EN: Not enough output channels! Need {count} starting from {start}
  - FR: Pas assez de canaux de sortie ! Besoin de {count} à partir de {start}
  - [ ] OK    Fix: 

- **`prefix`**
  - EN: Error: 
  - FR: Erreur : 
  - [ ] OK    Fix: 

- **`speakerCountZero`**
  - EN: Number of speakers must be greater than 0
  - FR: Le nombre d'enceintes doit être supérieur à 0
  - [ ] OK    Fix: 

## `arrayHelper.geometry`

- **`backFacing`**
  - EN: Back Facing
  - FR: Face arrière
  - [ ] OK    Fix: 

- **`centerSpacing`**
  - EN: Center + Spacing
  - FR: Centre + Espacement
  - [ ] OK    Fix: 

- **`centerX`**
  - EN: Center X (m):
  - FR: Centre X (m) :
  - [ ] OK    Fix: 

- **`centerY`**
  - EN: Center Y (m):
  - FR: Centre Y (m) :
  - [ ] OK    Fix: 

- **`endpoints`**
  - EN: Endpoints
  - FR: Points d'extrémité
  - [ ] OK    Fix: 

- **`endX`**
  - EN: End X (m):
  - FR: Fin X (m) :
  - [ ] OK    Fix: 

- **`endY`**
  - EN: End Y (m):
  - FR: Fin Y (m) :
  - [ ] OK    Fix: 

- **`facingInward`**
  - EN: Facing Inward
  - FR: Orienté vers l'intérieur
  - [ ] OK    Fix: 

- **`facingOutward`**
  - EN: Facing Outward
  - FR: Orienté vers l'extérieur
  - [ ] OK    Fix: 

- **`frontFacing`**
  - EN: Front Facing
  - FR: Face avant
  - [ ] OK    Fix: 

- **`nPairs`**
  - EN: N Pairs:
  - FR: N Paires :
  - [ ] OK    Fix: 

- **`nSpeakers`**
  - EN: N Speakers:
  - FR: N Enceintes :
  - [ ] OK    Fix: 

- **`orientation`**
  - EN: Orientation (deg):
  - FR: Orientation (deg) :
  - [ ] OK    Fix: 

- **`radius`**
  - EN: Radius (m):
  - FR: Rayon (m) :
  - [ ] OK    Fix: 

- **`sag`**
  - EN: Sag (m):
  - FR: Flèche (m) :
  - [ ] OK    Fix: 

- **`spacing`**
  - EN: Spacing (m):
  - FR: Espacement (m) :
  - [ ] OK    Fix: 

- **`startAngle`**
  - EN: Start Angle (deg):
  - FR: Angle début (deg) :
  - [ ] OK    Fix: 

- **`startX`**
  - EN: Start X (m):
  - FR: Début X (m) :
  - [ ] OK    Fix: 

- **`startY`**
  - EN: Start Y (m):
  - FR: Début Y (m) :
  - [ ] OK    Fix: 

- **`width`**
  - EN: Width (m):
  - FR: Largeur (m) :
  - [ ] OK    Fix: 

- **`yEnd`**
  - EN: Y End (m):
  - FR: Y Fin (m) :
  - [ ] OK    Fix: 

- **`yStart`**
  - EN: Y Start (m):
  - FR: Y Début (m) :
  - [ ] OK    Fix: 

- **`zHeight`**
  - EN: Z Height (m):
  - FR: Hauteur Z (m) :
  - [ ] OK    Fix: 

## `arrayHelper.presets`

- **`circle`**
  - EN: Circle
  - FR: Cercle
  - [ ] OK    Fix: 

- **`delayLine`**
  - EN: Delay Line
  - FR: Ligne de délai
  - [ ] OK    Fix: 

- **`label`**
  - EN: Preset:
  - FR: Préréglage :
  - [ ] OK    Fix: 

- **`mainRoomStraight`**
  - EN: Main Flown Array Straight
  - FR: Antenne Haute Droite
  - [ ] OK    Fix: 

- **`nearFieldCurved`**
  - EN: Near Field Array Curved
  - FR: Ligne courbe champ proche
  - [ ] OK    Fix: 

- **`nearFieldStraight`**
  - EN: Near Field Array Straight
  - FR: Ligne droite champ proche
  - [ ] OK    Fix: 

- **`subBass`**
  - EN: Sub Bass
  - FR: Sub Bass
  - [ ] OK    Fix: 

- **`surround`**
  - EN: Surround
  - FR: Surround
  - [ ] OK    Fix: 

## `arrayHelper.preview`

- **`audience`**
  - EN: Audience
  - FR: Public
  - [ ] OK    Fix: 

## `arrayHelper.sections`

- **`acousticDefaults`**
  - EN: Acoustic Defaults
  - FR: Paramètres acoustiques
  - [ ] OK    Fix: 

- **`geometry`**
  - EN: Geometry
  - FR: Géométrie
  - [ ] OK    Fix: 

- **`target`**
  - EN: Target
  - FR: Cible
  - [ ] OK    Fix: 

## `arrayHelper.status`

- **`applied`**
  - EN: Applied {count} speakers to Array {array}. Ready for next array.
  - FR: {count} enceintes appliquées au Groupe {array}. Prêt pour le suivant.
  - [ ] OK    Fix: 

- **`calculated`**
  - EN: Calculated {count} positions
  - FR: {count} positions calculées
  - [ ] OK    Fix: 

- **`ready`**
  - EN: Ready
  - FR: Prêt
  - [ ] OK    Fix: 

## `arrayHelper.target`

- **`array`**
  - EN: Array:
  - FR: Groupe :
  - [ ] OK    Fix: 

- **`arrayPrefix`**
  - EN: Array
  - FR: Groupe
  - [ ] OK    Fix: 

- **`startingOutput`**
  - EN: Starting Output:
  - FR: Sortie de départ :
  - [ ] OK    Fix: 

## `arrayHelper.window`

- **`contentName`**
  - EN: Output Array Helper
  - FR: Assistant de positionnement des sorties
  - [ ] OK    Fix: 

- **`title`**
  - EN: Wizard of OutZ
  - FR: Assistant OutZ
  - [ ] OK    Fix: 

## `audioPatch`

- **`windowTitle`**
  - EN: Audio Interface and Patching
  - FR: Interface audio et routage
  - [ ] OK    Fix: 

## `audioPatch.buttons`

- **`hold`**
  - EN: Hold
  - FR: Maintenir
  - [ ] OK    Fix: 

- **`unpatchAll`**
  - EN: Unpatch All
  - FR: Tout déconnecter
  - [ ] OK    Fix: 

## `audioPatch.deviceSettings`

- **`buttons.controlPanel`**
  - EN: Control Panel
  - FR: Panneau de contrôle
  - [ ] OK    Fix: 

- **`buttons.resetDevice`**
  - EN: Reset Device
  - FR: Réinitialiser appareil
  - [ ] OK    Fix: 

- **`labels.bufferSize`**
  - EN: Audio buffer size:
  - FR: Taille du buffer audio :
  - [ ] OK    Fix: 

- **`labels.device`**
  - EN: Device:
  - FR: Appareil :
  - [ ] OK    Fix: 

- **`labels.deviceType`**
  - EN: Audio device type:
  - FR: Type d'appareil audio :
  - [ ] OK    Fix: 

- **`labels.sampleRate`**
  - EN: Sample rate:
  - FR: Fréquence d'échantillonnage :
  - [ ] OK    Fix: 

- **`noDevice`**
  - EN: No Device
  - FR: Aucun appareil
  - [ ] OK    Fix: 

- **`notConfigured`**
  - EN: Not configured
  - FR: Non configuré
  - [ ] OK    Fix: 

## `audioPatch.labels`

- **`interfaceInput`**
  - EN: Audio Interface Input
  - FR: Entrée interface audio
  - [ ] OK    Fix: 

- **`interfaceOutput`**
  - EN: Audio Interface Output
  - FR: Sortie interface audio
  - [ ] OK    Fix: 

- **`processorInputs`**
  - EN: Processor Inputs
  - FR: Entrées processeur
  - [ ] OK    Fix: 

- **`processorOutputs`**
  - EN: Processor Outputs
  - FR: Sorties processeur
  - [ ] OK    Fix: 

## `audioPatch.messages`

- **`chooseTestSignal`**
  - EN: Choose a Test Signal to Enable Testing
  - FR: Choisissez un signal de test pour activer le test
  - [ ] OK    Fix: 

## `audioPatch.modes`

- **`patching`**
  - EN: Patching
  - FR: Routage
  - [ ] OK    Fix: 

- **`scrolling`**
  - EN: Scrolling
  - FR: Défilement
  - [ ] OK    Fix: 

- **`testing`**
  - EN: Testing
  - FR: Test
  - [ ] OK    Fix: 

## `audioPatch.tabs`

- **`deviceSettings`**
  - EN: Device Settings
  - FR: Paramètres appareil
  - [ ] OK    Fix: 

- **`inputPatch`**
  - EN: Input Patch
  - FR: Patch entrées
  - [ ] OK    Fix: 

- **`outputPatch`**
  - EN: Output Patch
  - FR: Patch sorties
  - [ ] OK    Fix: 

## `audioPatch.testSignal`

- **`labels.frequency`**
  - EN: Frequency:
  - FR: Fréquence :
  - [ ] OK    Fix: 

- **`labels.level`**
  - EN: Level:
  - FR: Niveau :
  - [ ] OK    Fix: 

- **`labels.signal`**
  - EN: Signal:
  - FR: Signal :
  - [ ] OK    Fix: 

- **`types.diracPulse`**
  - EN: Dirac Pulse
  - FR: Impulsion Dirac
  - [ ] OK    Fix: 

- **`types.off`**
  - EN: Off
  - FR: Off
  - [ ] OK    Fix: 

- **`types.pinkNoise`**
  - EN: Pink Noise
  - FR: Bruit rose
  - [ ] OK    Fix: 

- **`types.pulse`**
  - EN: Pulse
  - FR: Impulsion
  - [ ] OK    Fix: 

- **`types.sweep`**
  - EN: Sweep
  - FR: Balayage
  - [ ] OK    Fix: 

- **`types.tone`**
  - EN: Tone
  - FR: Tonalité
  - [ ] OK    Fix: 

## `clusters.help`

- **`attenuationSlider`**
  - EN: Adjust attenuation of all cluster inputs (dB).
  - FR: Régler l'atténuation de toutes les entrées du cluster (dB).
  - [ ] OK    Fix: 

- **`exportPresets`**
  - EN: Export all 16 LFO presets to an XML file.
  - FR: Exporter les 16 presets LFO vers un fichier XML.
  - [ ] OK    Fix: 

- **`importPresets`**
  - EN: Import LFO presets from an XML file.
  - FR: Importer les presets LFO depuis un fichier XML.
  - [ ] OK    Fix: 

- **`inputsVisibility`**
  - EN: Show or hide this cluster's inputs on the Map tab. Hide propagates to new members; removing an input restores its visibility.
  - FR: Afficher ou masquer les entrées de ce cluster sur la carte. Le masquage s'applique aussi aux nouvelles entrées ; retirer une entrée la rend à nouveau visible.
  - [ ] OK    Fix: 

- **`lfoActiveButton`**
  - EN: Enable or Disable the Periodic Movement of the Cluster (LFO).
  - FR: Activer ou désactiver le mouvement périodique du cluster (LFO).
  - [ ] OK    Fix: 

- **`lfoAmplitudeRotSlider`**
  - EN: Maximum Rotation Angle (-360 to 360 Degrees).
  - FR: Angle de rotation maximal (-360 à 360 degrés).
  - [ ] OK    Fix: 

- **`lfoAmplitudeScaleSlider`**
  - EN: Maximum Scale Factor (0.1x to 10x, Logarithmic).
  - FR: Facteur d'échelle maximal (0,1× à 10×, logarithmique).
  - [ ] OK    Fix: 

- **`lfoAmplitudeXSlider`**
  - EN: Width of Movement in Relation to Cluster Reference Position.
  - FR: Largeur du mouvement par rapport à la position de référence du cluster.
  - [ ] OK    Fix: 

- **`lfoAmplitudeYSlider`**
  - EN: Depth of Movement in Relation to Cluster Reference Position.
  - FR: Profondeur du mouvement par rapport à la position de référence du cluster.
  - [ ] OK    Fix: 

- **`lfoAmplitudeZSlider`**
  - EN: Height of Movement in Relation to Cluster Reference Position.
  - FR: Hauteur du mouvement par rapport à la position de référence du cluster.
  - [ ] OK    Fix: 

- **`lfoPeriodDial`**
  - EN: Base Period of the Cluster Movement.
  - FR: Période de base du mouvement du cluster.
  - [ ] OK    Fix: 

- **`lfoPhaseDial`**
  - EN: Global Phase Offset of the Cluster Movement.
  - FR: Décalage de phase global du mouvement du cluster.
  - [ ] OK    Fix: 

- **`lfoPhaseRotDial`**
  - EN: Phase Offset of the Cluster Rotation.
  - FR: Décalage de phase de la rotation du cluster.
  - [ ] OK    Fix: 

- **`lfoPhaseScaleDial`**
  - EN: Phase Offset of the Cluster Scaling.
  - FR: Décalage de phase de la mise à l'échelle du cluster.
  - [ ] OK    Fix: 

- **`lfoPhaseXDial`**
  - EN: Phase Offset of the Cluster Movement in Width.
  - FR: Décalage de phase du mouvement du cluster en largeur.
  - [ ] OK    Fix: 

- **`lfoPhaseYDial`**
  - EN: Phase Offset of the Cluster Movement in Depth.
  - FR: Décalage de phase du mouvement du cluster en profondeur.
  - [ ] OK    Fix: 

- **`lfoPhaseZDial`**
  - EN: Phase Offset of the Cluster Movement in Height.
  - FR: Décalage de phase du mouvement du cluster en hauteur.
  - [ ] OK    Fix: 

- **`lfoRateRotSlider`**
  - EN: Faster or Slower Rotation in Relation to Base Period.
  - FR: Rotation plus rapide ou plus lente par rapport à la période de base.
  - [ ] OK    Fix: 

- **`lfoRateScaleSlider`**
  - EN: Faster or Slower Scaling in Relation to Base Period.
  - FR: Mise à l'échelle plus rapide ou plus lente par rapport à la période de base.
  - [ ] OK    Fix: 

- **`lfoRateXSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Width.
  - FR: Mouvement plus rapide ou plus lent par rapport à la période de base, en largeur.
  - [ ] OK    Fix: 

- **`lfoRateYSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Depth.
  - FR: Mouvement plus rapide ou plus lent par rapport à la période de base, en profondeur.
  - [ ] OK    Fix: 

- **`lfoRateZSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Height.
  - FR: Mouvement plus rapide ou plus lent par rapport à la période de base, en hauteur.
  - [ ] OK    Fix: 

- **`lfoShapeRotSelector`**
  - EN: Rotation Behaviour of the Cluster.
  - FR: Comportement de rotation du cluster.
  - [ ] OK    Fix: 

- **`lfoShapeScaleSelector`**
  - EN: Scale Behaviour of the Cluster.
  - FR: Comportement d'échelle du cluster.
  - [ ] OK    Fix: 

- **`lfoShapeXSelector`**
  - EN: Movement Behaviour of the Cluster in Width.
  - FR: Comportement du mouvement du cluster en largeur.
  - [ ] OK    Fix: 

- **`lfoShapeYSelector`**
  - EN: Movement Behaviour of the Cluster in Depth.
  - FR: Comportement du mouvement du cluster en profondeur.
  - [ ] OK    Fix: 

- **`lfoShapeZSelector`**
  - EN: Movement Behaviour of the Cluster in Height.
  - FR: Comportement du mouvement du cluster en hauteur.
  - [ ] OK    Fix: 

- **`planeSelector`**
  - EN: Select the plane for rotation and scale operations (XY, XZ, YZ).
  - FR: Sélectionner le plan pour les opérations de rotation et d'échelle (XY, XZ, YZ).
  - [ ] OK    Fix: 

- **`positionJoystick`**
  - EN: Move all cluster inputs in X/Y. Hold and drag to translate.
  - FR: Déplacer toutes les entrées du cluster en X/Y. Cliquer et glisser pour translater.
  - [ ] OK    Fix: 

- **`presetTile`**
  - EN: Click: recall preset. Double-click: recall + start. Middle/right-click: store current LFO.
  - FR: Clic : rappeler le preset. Double-clic : rappeler + démarrer. Clic milieu/droit : enregistrer le LFO actuel.
  - [ ] OK    Fix: 

- **`qlabPreset`**
  - EN: Create a QLab network cue to recall the last selected LFO preset for the current cluster.
  - FR: Créer une cue réseau QLab pour rappeler le dernier preset LFO sélectionné pour le cluster courant.
  - [ ] OK    Fix: 

- **`referenceMode`**
  - EN: Select the reference point for cluster transforms: First Input or Barycenter.
  - FR: Sélectionner le point de référence pour les transformations du cluster : Première entrée ou Barycentre.
  - [ ] OK    Fix: 

- **`rotationDial`**
  - EN: Rotate all cluster inputs around the reference point in the selected plane.
  - FR: Faire pivoter toutes les entrées du cluster autour du point de référence dans le plan sélectionné.
  - [ ] OK    Fix: 

- **`scaleJoystick`**
  - EN: Scale cluster inputs relative to the reference point in the selected plane.
  - FR: Mettre à l'échelle les entrées du cluster par rapport au point de référence dans le plan sélectionné.
  - [ ] OK    Fix: 

- **`stopAllLFO`**
  - EN: Stop LFO on all 10 clusters.
  - FR: Arrêter le LFO sur les 10 clusters.
  - [ ] OK    Fix: 

- **`zSlider`**
  - EN: Move all cluster inputs along the Z axis (height).
  - FR: Déplacer toutes les entrées du cluster sur l'axe Z (hauteur).
  - [ ] OK    Fix: 

## `clusters.labels`

- **`assignedInputs`**
  - EN: Assigned Inputs
  - FR: Entrées assignées
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Atten
  - FR: Atténuation
  - [ ] OK    Fix: 

- **`controls`**
  - EN: Controls
  - FR: Contrôles
  - [ ] OK    Fix: 

- **`inputPrefix`**
  - EN: Input
  - FR: Entrée
  - [ ] OK    Fix: 

- **`position`**
  - EN: Position
  - FR: Position
  - [ ] OK    Fix: 

- **`posPrefix`**
  - EN: Pos:
  - FR: Pos :
  - [ ] OK    Fix: 

- **`reference`**
  - EN: Reference:
  - FR: Référence :
  - [ ] OK    Fix: 

- **`rotation`**
  - EN: Rotation
  - FR: Rotation
  - [ ] OK    Fix: 

- **`scale`**
  - EN: Scale
  - FR: Échelle
  - [ ] OK    Fix: 

- **`x`**
  - EN: X:
  - FR: X :
  - [ ] OK    Fix: 

- **`y`**
  - EN: Y:
  - FR: Y :
  - [ ] OK    Fix: 

- **`z`**
  - EN: Z
  - FR: Z
  - [ ] OK    Fix: 

## `clusters.lfo`

- **`labels.amplitude`**
  - EN: Amplitude:
  - FR: Amplitude :
  - [ ] OK    Fix: 

- **`labels.angle`**
  - EN: Angle:
  - FR: Angle :
  - [ ] OK    Fix: 

- **`labels.period`**
  - EN: Period:
  - FR: Période :
  - [ ] OK    Fix: 

- **`labels.phase`**
  - EN: Phase:
  - FR: Phase :
  - [ ] OK    Fix: 

- **`labels.rate`**
  - EN: Rate:
  - FR: Vitesse :
  - [ ] OK    Fix: 

- **`labels.ratio`**
  - EN: Ratio:
  - FR: Ratio :
  - [ ] OK    Fix: 

- **`labels.rot`**
  - EN: Rotation
  - FR: Rotation
  - [ ] OK    Fix: 

- **`labels.scaleLfo`**
  - EN: Scale
  - FR: Échelle
  - [ ] OK    Fix: 

- **`labels.section`**
  - EN: LFO
  - FR: LFO
  - [ ] OK    Fix: 

- **`labels.x`**
  - EN: X
  - FR: X
  - [ ] OK    Fix: 

- **`labels.y`**
  - EN: Y
  - FR: Y
  - [ ] OK    Fix: 

- **`labels.z`**
  - EN: Z
  - FR: Z
  - [ ] OK    Fix: 

## `clusters.osc`

- **`lfoActive`**
  - EN: /wfs/cluster/lfoActive <id> <0|1>
  - FR: /wfs/cluster/lfoActive <id> <0|1>
  - [ ] OK    Fix: 

- **`lfoAmplitudeRot`**
  - EN: /wfs/cluster/lfoAmplitudeRot <id> <degrees>
  - FR: /wfs/cluster/lfoAmplitudeRot <id> <degrés>
  - [ ] OK    Fix: 

- **`lfoAmplitudeScale`**
  - EN: /wfs/cluster/lfoAmplitudeScale <id> <factor>
  - FR: /wfs/cluster/lfoAmplitudeScale <id> <facteur>
  - [ ] OK    Fix: 

- **`lfoAmplitudeX`**
  - EN: /wfs/cluster/lfoAmplitudeX <id> <meters>
  - FR: /wfs/cluster/lfoAmplitudeX <id> <mètres>
  - [ ] OK    Fix: 

- **`lfoAmplitudeY`**
  - EN: /wfs/cluster/lfoAmplitudeY <id> <meters>
  - FR: /wfs/cluster/lfoAmplitudeY <id> <mètres>
  - [ ] OK    Fix: 

- **`lfoAmplitudeZ`**
  - EN: /wfs/cluster/lfoAmplitudeZ <id> <meters>
  - FR: /wfs/cluster/lfoAmplitudeZ <id> <mètres>
  - [ ] OK    Fix: 

- **`lfoPeriod`**
  - EN: /wfs/cluster/lfoPeriod <id> <seconds>
  - FR: /wfs/cluster/lfoPeriod <id> <secondes>
  - [ ] OK    Fix: 

- **`lfoPhase`**
  - EN: /wfs/cluster/lfoPhase <id> <degrees>
  - FR: /wfs/cluster/lfoPhase <id> <degrés>
  - [ ] OK    Fix: 

- **`lfoPhaseRot`**
  - EN: /wfs/cluster/lfoPhaseRot <id> <degrees>
  - FR: /wfs/cluster/lfoPhaseRot <id> <degrés>
  - [ ] OK    Fix: 

- **`lfoPhaseScale`**
  - EN: /wfs/cluster/lfoPhaseScale <id> <degrees>
  - FR: /wfs/cluster/lfoPhaseScale <id> <degrés>
  - [ ] OK    Fix: 

- **`lfoPhaseX`**
  - EN: /wfs/cluster/lfoPhaseX <id> <degrees>
  - FR: /wfs/cluster/lfoPhaseX <id> <degrés>
  - [ ] OK    Fix: 

- **`lfoPhaseY`**
  - EN: /wfs/cluster/lfoPhaseY <id> <degrees>
  - FR: /wfs/cluster/lfoPhaseY <id> <degrés>
  - [ ] OK    Fix: 

- **`lfoPhaseZ`**
  - EN: /wfs/cluster/lfoPhaseZ <id> <degrees>
  - FR: /wfs/cluster/lfoPhaseZ <id> <degrés>
  - [ ] OK    Fix: 

- **`lfoPresetRecall`**
  - EN: /wfs/cluster/lfoPresetRecall <clusterId> <presetNumber>
  - FR: /wfs/cluster/lfoPresetRecall <clusterId> <numéroPreset>
  - [ ] OK    Fix: 

- **`lfoRateRot`**
  - EN: /wfs/cluster/lfoRateRot <id> <multiplier>
  - FR: /wfs/cluster/lfoRateRot <id> <multiplicateur>
  - [ ] OK    Fix: 

- **`lfoRateScale`**
  - EN: /wfs/cluster/lfoRateScale <id> <multiplier>
  - FR: /wfs/cluster/lfoRateScale <id> <multiplicateur>
  - [ ] OK    Fix: 

- **`lfoRateX`**
  - EN: /wfs/cluster/lfoRateX <id> <multiplier>
  - FR: /wfs/cluster/lfoRateX <id> <multiplicateur>
  - [ ] OK    Fix: 

- **`lfoRateY`**
  - EN: /wfs/cluster/lfoRateY <id> <multiplier>
  - FR: /wfs/cluster/lfoRateY <id> <multiplicateur>
  - [ ] OK    Fix: 

- **`lfoRateZ`**
  - EN: /wfs/cluster/lfoRateZ <id> <multiplier>
  - FR: /wfs/cluster/lfoRateZ <id> <multiplicateur>
  - [ ] OK    Fix: 

- **`lfoShapeRot`**
  - EN: /wfs/cluster/lfoShapeRot <id> <0-8>
  - FR: /wfs/cluster/lfoShapeRot <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoShapeScale`**
  - EN: /wfs/cluster/lfoShapeScale <id> <0-8>
  - FR: /wfs/cluster/lfoShapeScale <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoShapeX`**
  - EN: /wfs/cluster/lfoShapeX <id> <0-8>
  - FR: /wfs/cluster/lfoShapeX <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoShapeY`**
  - EN: /wfs/cluster/lfoShapeY <id> <0-8>
  - FR: /wfs/cluster/lfoShapeY <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoShapeZ`**
  - EN: /wfs/cluster/lfoShapeZ <id> <0-8>
  - FR: /wfs/cluster/lfoShapeZ <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoStopAll`**
  - EN: /wfs/cluster/lfoStopAll
  - FR: /wfs/cluster/lfoStopAll
  - [ ] OK    Fix: 

## `clusters.planes`

- **`xy`**
  - EN: XY
  - FR: XY
  - [ ] OK    Fix: 

- **`xz`**
  - EN: XZ
  - FR: XZ
  - [ ] OK    Fix: 

- **`yz`**
  - EN: YZ
  - FR: YZ
  - [ ] OK    Fix: 

## `clusters.presets`

- **`export`**
  - EN: Export
  - FR: Exporter
  - [ ] OK    Fix: 

- **`exportDialog`**
  - EN: Export LFO Presets
  - FR: Exporter les presets LFO
  - [ ] OK    Fix: 

- **`exported`**
  - EN: LFO presets exported.
  - FR: Presets LFO exportés.
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - FR: Importer
  - [ ] OK    Fix: 

- **`importDialog`**
  - EN: Import LFO Presets
  - FR: Importer les presets LFO
  - [ ] OK    Fix: 

- **`imported`**
  - EN: LFO presets imported.
  - FR: Presets LFO importés.
  - [ ] OK    Fix: 

- **`recalled`**
  - EN: LFO preset recalled from tile {n}.
  - FR: Preset LFO rappelé depuis la case {n}.
  - [ ] OK    Fix: 

- **`stopAll`**
  - EN: Stop All
  - FR: Tout arrêter
  - [ ] OK    Fix: 

- **`stored`**
  - EN: LFO preset stored in tile {n}.
  - FR: Preset LFO enregistré dans la case {n}.
  - [ ] OK    Fix: 

## `clusters.referenceMode`

- **`barycenter`**
  - EN: Barycenter
  - FR: Barycentre
  - [ ] OK    Fix: 

- **`firstInput`**
  - EN: First Input
  - FR: Première entrée
  - [ ] OK    Fix: 

## `clusters.status`

- **`noInputs`**
  - EN: No inputs assigned
  - FR: Aucune entrée assignée
  - [ ] OK    Fix: 

- **`trackedMarker`**
  - EN: [T]
  - FR: [S]
  - [ ] OK    Fix: 

- **`tracking`**
  - EN: Tracking: Input {num} (overrides reference)
  - FR: Suivi : Entrée {num} (remplace la référence)
  - [ ] OK    Fix: 

## `clusters.toggles`

- **`inputsHidden`**
  - EN: Inputs: Hidden
  - FR: Entrées : Masquées
  - [ ] OK    Fix: 

- **`inputsVisible`**
  - EN: Inputs: Visible
  - FR: Entrées : Visibles
  - [ ] OK    Fix: 

- **`lfoOff`**
  - EN: L.F.O: OFF
  - FR: L.F.O : NON
  - [ ] OK    Fix: 

- **`lfoOn`**
  - EN: L.F.O: ON
  - FR: L.F.O : OUI
  - [ ] OK    Fix: 

## `common`

- **`cancel`**
  - EN: Cancel
  - FR: Annuler
  - [ ] OK    Fix: 

- **`off`**
  - EN: OFF
  - FR: OFF
  - [ ] OK    Fix: 

- **`ok`**
  - EN: OK
  - FR: OK
  - [ ] OK    Fix: 

- **`on`**
  - EN: ON
  - FR: ON
  - [ ] OK    Fix: 

## `eq.buttons`

- **`flattenEQ`**
  - EN: Flatten EQ
  - FR: Réinit. EQ
  - [ ] OK    Fix: 

- **`resetBand`**
  - EN: Reset
  - FR: Réinit.
  - [ ] OK    Fix: 

## `eq.filterTypes`

- **`allPass`**
  - EN: All-Pass
  - FR: Passe-tout
  - [ ] OK    Fix: 

- **`bandPass`**
  - EN: Band Pass
  - FR: Passe-bande
  - [ ] OK    Fix: 

- **`highCut`**
  - EN: High Cut
  - FR: Coupe-haut
  - [ ] OK    Fix: 

- **`highShelf`**
  - EN: High Shelf
  - FR: Aigu
  - [ ] OK    Fix: 

- **`lowCut`**
  - EN: Low Cut
  - FR: Coupe-bas
  - [ ] OK    Fix: 

- **`lowShelf`**
  - EN: Low Shelf
  - FR: Grave
  - [ ] OK    Fix: 

- **`peakNotch`**
  - EN: Peak/Notch
  - FR: Pic/Creux
  - [ ] OK    Fix: 

## `eq.labels`

- **`band`**
  - EN: Band
  - FR: Bande
  - [ ] OK    Fix: 

- **`freq`**
  - EN: Freq:
  - FR: Fréq :
  - [ ] OK    Fix: 

- **`gain`**
  - EN: Gain
  - FR: Gain
  - [ ] OK    Fix: 

- **`q`**
  - EN: Q
  - FR: Q
  - [ ] OK    Fix: 

## `eq.status`

- **`off`**
  - EN: EQ OFF
  - FR: EQ DÉSACTIVÉ
  - [ ] OK    Fix: 

- **`on`**
  - EN: EQ ON
  - FR: EQ ACTIVÉ
  - [ ] OK    Fix: 

## `fileManager.dialogs`

- **`selectProjectFolder`**
  - EN: Select Project Folder
  - FR: Sélectionner le dossier de projet
  - [ ] OK    Fix: 

## `fileManager.errors`

- **`backupNotFound`**
  - EN: Backup not found
  - FR: Sauvegarde introuvable
  - [ ] OK    Fix: 

- **`configStateInvalid`**
  - EN: Config state is invalid
  - FR: L'état de configuration est invalide
  - [ ] OK    Fix: 

- **`failedApply`**
  - EN: Failed to apply: {sections}
  - FR: Échec de l'application : {sections}
  - [ ] OK    Fix: 

- **`failedCreateFolder`**
  - EN: Failed to create project folder: {path}
  - FR: Échec de la création du dossier projet : {path}
  - [ ] OK    Fix: 

- **`failedCreateValueTree`**
  - EN: Failed to create ValueTree from XML: {path}
  - FR: Échec de la création du ValueTree depuis XML : {path}
  - [ ] OK    Fix: 

- **`failedCreateXML`**
  - EN: Failed to create XML from state
  - FR: Échec de la création du XML depuis l'état
  - [ ] OK    Fix: 

- **`failedParseXML`**
  - EN: Failed to parse XML file: {path}
  - FR: Échec de l'analyse du fichier XML : {path}
  - [ ] OK    Fix: 

- **`failedWriteFile`**
  - EN: Failed to write file: {path}
  - FR: Échec de l'écriture du fichier : {path}
  - [ ] OK    Fix: 

- **`fileNotFound`**
  - EN: File not found: {path}
  - FR: Fichier introuvable : {path}
  - [ ] OK    Fix: 

- **`invalidConfigStructure`**
  - EN: Invalid configuration file structure
  - FR: Structure du fichier de configuration invalide
  - [ ] OK    Fix: 

- **`noInputDataInFile`**
  - EN: No input data found in file
  - FR: Aucune donnée d'entrée trouvée dans le fichier
  - [ ] OK    Fix: 

- **`noInputDataInSnapshot`**
  - EN: No input data in snapshot
  - FR: Aucune donnée d'entrée dans le snapshot
  - [ ] OK    Fix: 

- **`noLFOPresetDataInFile`**
  - EN: No LFO preset data found in file
  - FR: Aucune donnée de preset LFO trouvée dans le fichier
  - [ ] OK    Fix: 

- **`noNetworkDataInFile`**
  - EN: No network data found in file
  - FR: Aucune donnée réseau trouvée dans le fichier
  - [ ] OK    Fix: 

- **`noNetworkSections`**
  - EN: No network sections found in file
  - FR: Aucune section réseau trouvée dans le fichier
  - [ ] OK    Fix: 

- **`noOutputDataInFile`**
  - EN: No output data found in file
  - FR: Aucune donnée de sortie trouvée dans le fichier
  - [ ] OK    Fix: 

- **`noProjectFolder`**
  - EN: No project folder specified
  - FR: Aucun dossier projet spécifié
  - [ ] OK    Fix: 

- **`noReverbDataInFile`**
  - EN: No reverb data found in file
  - FR: Aucune donnée de réverbération trouvée dans le fichier
  - [ ] OK    Fix: 

- **`noSystemDataInFile`**
  - EN: No valid system data found in file: {path}
  - FR: Aucune donnée système valide trouvée dans le fichier : {path}
  - [ ] OK    Fix: 

- **`noValidProjectFolder`**
  - EN: No valid project folder
  - FR: Aucun dossier projet valide
  - [ ] OK    Fix: 

- **`prefixInputs`**
  - EN: Inputs: 
  - FR: Entrées : 
  - [ ] OK    Fix: 

- **`prefixNetwork`**
  - EN: Network: 
  - FR: Réseau : 
  - [ ] OK    Fix: 

- **`prefixOutputs`**
  - EN: Outputs: 
  - FR: Sorties : 
  - [ ] OK    Fix: 

- **`prefixReverbs`**
  - EN: Reverbs: 
  - FR: Réverbérations : 
  - [ ] OK    Fix: 

- **`prefixSystem`**
  - EN: System: 
  - FR: Système : 
  - [ ] OK    Fix: 

- **`snapshotNotFound`**
  - EN: Snapshot not found
  - FR: Snapshot introuvable
  - [ ] OK    Fix: 

- **`snapshotNotFoundNamed`**
  - EN: Snapshot not found: {name}
  - FR: Snapshot introuvable : {name}
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
  - FR: ADM-OSC est un protocole visant à améliorer l'interopérabilité du son spatialisé. Il envoie des positions cartésiennes (X, Y, Z) ou des valeurs polaires (AED pour Azimut, Élévation, Distance) depuis la console ou les courbes d'automation d'un DAW.
Les données sont envoyées normalisées :
- entre -1.0 et 1.0 pour X, Y et Z ;
- entre 0.0 et 1.0 pour la distance,
- entre -180° et 180° pour l'azimut
- entre -90° et 90° pour l'élévation.
Le point d'origine peut être déplacé et le mapping peut aussi être ajusté en différents segments pour les parties intérieure et extérieure de la scène.
En glissant les poignées sur les graphiques, maintenir la touche Maj applique des ajustements symétriques du côté opposé.
  - [ ] OK    Fix: 

- **`title`**
  - EN: ADM-OSC Mappings
  - FR: Mappings ADM-OSC
  - [ ] OK    Fix: 

## `help.automOtion`

- **`body`**
  - EN: One time movement can also be programmed and triggered manually or triggered by sound level.
The coordinates are either relative from the start position or absolute relative to the origin point.
The input can either stay at the end position or revert to the starting position.
Input position cannot be changed while the input is moving this way, but interacting with the input will change the position offset of the input instead.
For audio level triggering select the sound level above which the movement will start. When the sound drops below the reset level the movement will be rearmed, ready to be triggered again. New movement automation will not be possible while the input is moving.
  - FR: Un mouvement unique peut être programmé et déclenché manuellement ou par le niveau sonore.
Les coordonnées sont soit relatives depuis la position de départ, soit absolues par rapport au point d'origine.
L'entrée peut rester à la position finale ou revenir à la position de départ.
La position ne peut pas être modifiée pendant le mouvement, mais interagir avec l'entrée modifiera son décalage de position.
Pour le déclenchement par niveau audio, sélectionnez le seuil au-dessus duquel le mouvement démarrera. Quand le son descend sous le niveau de réinitialisation, le mouvement sera réarmé.
  - [ ] OK    Fix: 

- **`title`**
  - EN: AutomOtion
  - FR: AutomOtion
  - [ ] OK    Fix: 

## `help.binaural`

- **`body`**
  - EN: The Binaural renderer is used for:
- listening to a rough spatial mix on headphones,
- creating a mix for stereo output,
- listening to a single soloed track through the spatial processing.
This may take the place of your master mix if it's only feeding headphones and media mix.
The position of the listening position may be adjusted in depth from the origin point and in orientation. Delay and level settings allow you to eventually match the sound at the FOH position.
  - FR: Le rendu binaural est utilisé pour :
- écouter un mix spatial approximatif au casque,
- créer un mix pour une sortie stéréo,
- écouter une piste solotée à travers le traitement spatial.
Il peut remplacer votre mix master s'il n'alimente que le casque et le mix média.
La position d'écoute peut être ajustée en profondeur depuis le point d'origine et en orientation. Les réglages de délai et de niveau permettent de faire correspondre le son à la position FOH.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Binaural Renderer
  - FR: Rendu Binaural
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
  - FR: Les clusters sont des groupes d'entrées qui peuvent être manipulés et animés en tant qu'ensemble.
Chaque entrée ne peut faire partie que d'un seul cluster.
Chaque cluster ne peut avoir qu'une seule entrée avec le tracking pleinement activé. Cette entrée devient alors le point de référence du cluster.
Si aucune entrée avec tracking ne fait partie du cluster, il y a deux modes de point de référence : soit la première entrée de la liste, soit le barycentre, c'est-à-dire le centre de gravité des entrées assignées.
Toutes les entrées du cluster peuvent être déplacées en glissant le point de référence. Les entrées individuelles peuvent toujours être ajustées séparément. Glisser une entrée avec le tracking activé qui est aussi un point de référence affectera son décalage de position et la position des autres entrées du cluster normalement.
Toutes les entrées d'un cluster peuvent être tournées ou mises à l'échelle autour du point de référence.
Tous les clusters peuvent recevoir une animation via un LFO. Les positions X, Y, Z, la rotation et l'échelle peuvent être contrôlées. Les réglages LFO peuvent être assignés à des pads pour un rappel rapide. Un clic droit stockera les paramètres LFO dans un pad. Un double-clic sur le haut du pad permet d'éditer le nom du preset. Un clic ou un tap rappellera les réglages que le LFO soit en cours ou non, mais ne le démarrera pas s'il ne l'est pas. Un double clic/tap chargera et démarrera le LFO.
Tous les clusters partagent le même ensemble de presets LFO.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Clusters
  - FR: Clusters
  - [ ] OK    Fix: 

## `help.floorReflections`

- **`body`**
  - EN: Simulating the floor reflections improves the naturalness of the sound. We don't expect sounds to play in a soundproof anechoic chamber. This setting helps recreate the expected floor reflections.
The level of the floor reflections can be adjusted as well as the low cut and the high frequency shelf filters. Diffusion adds a bit of chaos to simulate the unevenness of the floor.
  - FR: Simuler les réflexions du sol améliore le naturel du son. On ne s'attend pas à ce que les sons soient diffusés dans une chambre anéchoïque insonorisée. Ce réglage aide à recréer les réflexions du sol attendues.
Le niveau des réflexions du sol peut être ajusté ainsi que les filtres coupe-bas et le shelf hautes fréquences. La diffusion ajoute un peu de chaos pour simuler les irrégularités du sol.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Floor Reflections
  - FR: Réflexions du Sol
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
  - FR: Les cartes de gradient permettent de contrôler l'atténuation, la hauteur et le filtrage haute fréquence (shelf avec une pente douce centrée à 1kHz) en fonction de la position X, Y. Par exemple, vous pouvez atténuer un son en entrant dans une zone, appliquer un roll-off haute fréquence en s'éloignant de l'avant-scène, ou ajuster automatiquement la hauteur d'un acteur sur des plateformes surélevées.
Trois couches sont disponibles : atténuation, hauteur et shelf HF. Elles peuvent être activées/désactivées et masquées. La couche active apparaît atténuée si désactivée. Les couches non focalisées apparaissent atténuées si actives.
Chaque couche a un contrôle de mapping blanc et noir pour ajuster la plage de l'effet. Le réglage de courbe ajuste la transition.
Chaque couche peut avoir des formes éditables (rectangle, ellipse ou polygone) avec un gris uni, un gradient linéaire ou radial.
Pour créer un polygone, cliquez pour chaque coin. Double-cliquez pour fermer la forme.
Double-cliquer sur un point existant le supprime. Double-cliquer sur un côté ajoute un nouveau point.
L'échelle et la rotation de chaque forme peuvent être éditées depuis son centre ou depuis l'origine.
Les formes et couches peuvent être copiées vers une autre couche.
Les réglages sont stockés dans les fichiers d'entrée.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Gradient Maps
  - FR: Cartes de Gradient
  - [ ] OK    Fix: 

## `help.inputAdvanced`

- **`body`**
  - EN: - Sidelines and Fringe enable muting when an input comes close to the limits of a rectangular stage (except downstage).
- Tracking can be enabled and the ID of the tracker can be selected. The amount of smoothing on the position can also be adjusted.
- Maximum Speed can be engaged and the speed limit adjusted. The system will apply a gradual acceleration and deceleration when the movement starts and finishes. When Path is enabled, the system will track the path followed by the input and will not go in a straight line to the final position. This is especially handy if movements have to be operated manually.
- Height Factor lets you work in 2D, when set to 0%, or full 3D, when set at 100%, and anything in between. It's the ratio of the height in the level and delay calculations. If you wish to use floor reflections, set it to 100% and use the parallax correction of the output parameters.
  - FR: - Les Lignes latérales et la Marge permettent le mute quand une entrée s'approche des limites d'une scène rectangulaire (sauf côté public).
- Le Tracking peut être activé et l'ID du tracker sélectionné. Le lissage de la position peut aussi être ajusté.
- La Vitesse Maximum peut être engagée et la limite de vitesse ajustée. Le système appliquera une accélération et décélération graduelles au début et à la fin du mouvement. Quand le mode Trajectoire est activé, le système suivra le chemin emprunté par l'entrée et n'ira pas en ligne droite vers la position finale. C'est particulièrement utile si les mouvements doivent être opérés manuellement.
- Le Facteur de Hauteur permet de travailler en 2D, quand réglé à 0%, ou en 3D complet, quand réglé à 100%, et tout entre les deux. C'est le ratio de la hauteur dans les calculs de niveau et de délai. Si vous souhaitez utiliser les réflexions au sol, réglez-le à 100% et utilisez la correction de parallaxe des paramètres de sortie.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Advanced Controls
  - FR: Contrôles avancés
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
  - FR: Les entrées disposent d'une grande variété de réglages pour s'adapter à différentes situations nécessitant un renforcement sonore réaliste ou des outils créatifs pour le design sonore.
- Le niveau d'entrée peut être ajusté.
- Les entrées peuvent être retardées ou essayer de prendre en compte une latence spécifique (traitement numérique de transmission sans fil ou effets numériques) et la compenser pour mieux aligner l'amplification et le son acoustique.
- La Latence Minimale peut être activée au lieu de la Précédence Acoustique. Cela essaie au contraire de faire sortir le son à travers le système aussi vite que possible. Le système analyse les envois de cette entrée vers les sorties pour le délai le plus faible et le soustrait de tous les délais et contourne l'effet Haas supplémentaire. L'idée ici serait de battre le son acoustique sur scène pour essayer de placer un son dans une position légèrement différente en modifiant d'abord la localisation perçue.
- La position (position et décalage) pour toute entrée peut être donnée en coordonnées Cartésiennes, Cylindriques ou Sphériques indépendamment de la forme de la scène ou des autres canaux.
- La position peut être contrainte aux dimensions de la scène en coordonnées Cartésiennes ou à une plage de distance spécifique en coordonnées polaires.
- Flip prendra la position symétrique pour la coordonnée donnée autour du point d'origine.
- Le joystick et le curseur vertical permettent un contrôle relatif de la position.
- Les entrées peuvent être assignées à un cluster pour les regrouper pour des mouvements coordonnés.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Inputs Basic Parameters
  - FR: Paramètres de base des entrées
  - [ ] OK    Fix: 

## `help.inputHF`

- **`body`**
  - EN: When talking away, the timbre of a voice will sound less bright. Reproducing this was the initial goal here even though we usually want to have support for voices when not addressing the audience or in bi-frontal configurations. This can be put to good use for creative effects such as having a brighter reverb on a dampened direct sound.
The orientation of the input in azimuth and in pitch can be set as well as the angle where the high frequencies will not be filtered.
The HF Shelf will set the maximum attenuation in the back of the input. There is a smooth fade (like a cosine curve) from full brightness in front to damped at the rear.
  - FR: Quand on parle en se détournant, le timbre d'une voix semble moins brillant. Reproduire cela était l'objectif initial ici même si on souhaite généralement avoir du soutien pour les voix qui ne s'adressent pas au public ou dans des configurations bi-frontales. Cela peut être utilisé pour des effets créatifs comme avoir une réverbération plus brillante sur un son direct atténué.
L'orientation de l'entrée en azimut et en élévation peut être réglée ainsi que l'angle où les hautes fréquences ne seront pas filtrées.
Le HF Shelf définira l'atténuation maximale à l'arrière de l'entrée. Il y a un fondu progressif (comme une courbe cosinus) de la pleine brillance à l'avant à l'atténuation à l'arrière.
  - [ ] OK    Fix: 

- **`title`**
  - EN: High Frequency Directivity
  - FR: Directivité haute fréquence
  - [ ] OK    Fix: 

## `help.inputLevel`

- **`body`**
  - EN: There are two level attenuation models. One where the level decreases with the distance by a given ratio in dB/m. Otherwise the level is halved each time the distance doubles. This might be more realistic, but it may be too loud close to the source or not give enough focus. The former may be less physically accurate, it offers usually better control to have a more even and stable mix.
  - FR: Il existe deux modèles d'atténuation de niveau. Un où le niveau diminue avec la distance selon un ratio donné en dB/m. Sinon le niveau est divisé par deux à chaque fois que la distance double. Ce dernier peut être plus réaliste, mais peut être trop fort près de la source ou ne pas donner assez de focalisation. Le premier peut être moins précis physiquement, il offre généralement un meilleur contrôle pour avoir un mix plus uniforme et stable.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Level Adjustments
  - FR: Ajustements de niveau
  - [ ] OK    Fix: 

## `help.inputMutes`

- **`body`**
  - EN: You can set for each output array a specific attenuation for the selected input.
You can mute each send to any output individually. There are macros to speed up the process.
  - FR: Vous pouvez définir pour chaque array de sortie une atténuation spécifique pour l'entrée sélectionnée.
Vous pouvez couper chaque envoi vers n'importe quelle sortie individuellement. Des macros sont disponibles pour accélérer le processus.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Array Attenuation and Output Mutes
  - FR: Atténuation par array et mutes de sortie
  - [ ] OK    Fix: 

## `help.lfo`

- **`body`**
  - EN: Input position can be automated. The LFO can control X, Y and Z position coordinates individually as well as the rotation of HF directivity (gyrophone).
Adjust the global period and phase for the LFO.
For X, Y and Z coordinates select a shape, amplitude, rate and phase as desired. A circle in the XY plane would have sine for shape for X and for Y and an offset of ±90° between the two. A square would be the same but with keystone shapes.
Input position can be moved while the LFO is running.
  - FR: La position de l'entrée peut être automatisée. Le LFO peut contrôler les coordonnées X, Y et Z individuellement ainsi que la rotation de la directivité HF (gyrophone).
Ajustez la période et la phase globales du LFO.
Pour X, Y et Z, sélectionnez une forme, amplitude, taux et phase. Un cercle dans le plan XY utiliserait une forme sinusoïdale pour X et Y avec un décalage de ±90° entre les deux. Un carré serait identique mais avec des formes trapézoïdales.
La position de l'entrée peut être déplacée pendant que le LFO fonctionne.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Input LFO
  - FR: LFO d'Entrée
  - [ ] OK    Fix: 

## `help.liveSource`

- **`body`**
  - EN: A loud source on stage may not need much reinforcement through the nearby speakers. Imagine an opera singer near the edge of the stage. Normally the level distribution would make the level louder near the input position. But if it is already loud enough we should be able to not over-amplify it. This function manages this.
The radius and shape describe how to attenuate the level for a specific source for speakers within the radius of influence of this input. There are various shapes: a V shaped linear effect; a U shape for fast decrease within the radius; a tight V effect or a mix of the previous (sine).
The attenuation can be constant or level dependent like a local compression reacting to transients and to average RMS level.
  - FR: Une source puissante sur scène n'a pas forcément besoin d'être renforcée par les haut-parleurs proches. Imaginez un chanteur d'opéra près du bord de la scène. Normalement la distribution de niveau rendrait le son plus fort près de la position de la source. Mais si c'est déjà assez fort, on devrait pouvoir ne pas sur-amplifier. Cette fonction gère cela.
Le rayon et la forme décrivent comment atténuer le niveau pour les haut-parleurs dans le rayon d'influence de cette source. Il y a différentes formes : un effet linéaire en V ; un U pour une décroissance rapide ; un V serré ou un mélange des précédents (sinus).
L'atténuation peut être constante ou dépendante du niveau, comme une compression locale réagissant aux transitoires et au niveau RMS moyen.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Live Source Tamer
  - FR: Atténuateur de Source Live
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
  - FR: - Un clic gauche sur une entrée ou un cluster permet de le déplacer en glissant. Un toucher simple fait de même.
- Un clic gauche avec Maj ajoute ou retire des entrées de la sélection. Un double tap et glisser fait de même.
- Un clic gauche glissé dessine un rectangle de sélection.
- Un double-clic ou tap réinitialise le décalage de position.
- Un clic long sans mouvement bascule vers l'onglet de l'entrée sélectionnée.
- Un clic hors d'une entrée efface la sélection.
- Un clic droit glissé déplace la vue. Un glisser à deux doigts fait de même si le multitouch est supporté.
- La molette zoome. Un pincement à deux doigts aussi.
- Un clic molette réinitialise la vue.
- Les flèches déplacent la sélection en X/Y, PageHaut/Bas en hauteur.
- Un deuxième doigt près d'une entrée touchée peut tourner la directivité et ajuster la hauteur.
- Un deuxième doigt près d'un cluster touché peut le tourner et le redimensionner.
- Les entrées, tableaux de sorties et nœuds de réverbération peuvent être masqués.
- Les entrées peuvent être verrouillées pour empêcher leur sélection et déplacement.
- Les nœuds de réverbération peuvent être déplacés si activé. Ctrl/Cmd déplace les paires en symétrie.
- Le rayon du Live Source Tamer est affiché autour de l'entrée quand activé.
- Un bouton permet d'afficher les niveaux audio sur la carte.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Map
  - FR: Carte
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
  - FR: Le serveur MCP permet à un assistant IA (Claude Desktop, Claude Code, ChatGPT avec connecteurs personnalisés) de lire et d'écrire les paramètres de cette session WFS-DIY via une connexion réseau locale.

Ce que l'IA peut faire :
• Lire l'état en direct : nombres de canaux, noms, positions, atténuations, EQ, snapshots, clusters, l'ensemble des paramètres.
• Déplacer des sources, renommer des canaux, définir des assignations de clusters, ajuster la disposition des arrays, placer sorties et réverbérations.
• Exécuter des workflows guidés (assistants d'accord système, dépannage de localisation, gestion des snapshots) via des modèles de prompts préparés.

Contrôles opérateur sur cette ligne :
• IA : ON / OFF — interrupteur principal. Sur OFF, tout appel d'outil IA est refusé ; sur ON, l'IA fonctionne selon les règles ci-dessous.
• Actions IA critiques : bloquées / AUTORISÉES — les actions destructrices (suppression de snapshots, réinitialisation DSP, changement du nombre de canaux) sont bloquées par défaut. Cliquer pour les autoriser pendant 10 minutes ; le remplissage rouge s'écoule à l'expiration, puis se bloque à nouveau automatiquement.
• Ouvrir Historique IA — chronologie déroulante de chaque modification IA récente avec annulation/rétablissement par ligne et curseur pas à pas.
• Le bouton URL MCP copie l'URL du serveur dans le presse-papiers pour les clients IA qui acceptent une URL directement.

Vigilance opérateur :
• Chaque action IA est enregistrée avec des tags d'origine. La fenêtre Historique IA affiche la chronologie complète ; le × par ligne annule une action avec ses dépendances.
• Si vous ajustez manuellement un paramètre que l'IA vient de modifier, l'IA est notifiée et ne réessayera pas aveuglément. Vous avez toujours le dernier mot.
• Les raccourcis Cmd/Ctrl+Alt+Z et Cmd/Ctrl+Alt+Y annulent et rétablissent la dernière modification IA sans affecter vos modifications manuelles (qui utilisent Ctrl+Z classique).

Pour ajouter ce serveur à Claude Desktop :
  1. Ouvrir Réglages → Développeur → Modifier la configuration.
  2. Coller l'extrait JSON ci-dessous dans claude_desktop_config.json (fusionner dans le bloc mcpServers existant si vous en avez déjà un).
  3. Redémarrer Claude Desktop. Le serveur apparaît sous le nom 'wfs-diy' dans le menu outils.

Pour l'ajouter à Claude Code, exécuter :
  claude mcp add wfs-diy <url> -t http

L'URL change si vous changez d'interface réseau ou si le serveur bascule sur un port différent. Le bouton URL sur cette ligne reflète toujours l'URL en direct.
  - [ ] OK    Fix: 

- **`copyButton`**
  - EN: Copy Config
  - FR: Copier la config
  - [ ] OK    Fix: 

- **`copyConfirm`**
  - EN: MCP config JSON copied to clipboard
  - FR: Config MCP JSON copiée dans le presse-papiers
  - [ ] OK    Fix: 

- **`title`**
  - EN: AI / MCP Server
  - FR: Serveur IA / MCP
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
  - FR: Le système peut communiquer via plusieurs protocoles réseau (UDP ou TCP) utilisant OSC. La requête OSC peut être activée pour permettre aux clients de découvrir les chemins OSC possibles et de s'abonner à des mises à jour de paramètres.
L'IP de la machine locale correspondant à l'interface réseau sélectionnée est affichée. Les ports TCP et UDP entrants ainsi que le port OSC Query.
Il y a quelques clients OSC spécialisés tels que :
- Remote pour l'application Android pour le contrôle multitouch et à distance.
- QLab qui peut envoyer des données et être programmé directement depuis l'application.
- ADM-OSC pour le contrôle depuis les consoles et DAW (voir l'aide spécifique).
Les données peuvent être filtrées pour n'autoriser que les données des adresses IP enregistrées ou permettre tout client envoyant sur les bons ports.
Une fenêtre de Log permet de voir les données entrantes et sortantes.
Il y a aussi une fonction de localisation pour retrouver une tablette Android égarée.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Network
  - FR: Réseau
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
  - FR: Il y a quelques paramètres pour vous aider à ajuster au son acoustique.
La plupart de ces paramètres sont définis pour des arrays entiers sauf si le mode de propagation est désactivé pour cette sortie dans l'array. Un changement relatif peut aussi être sélectionné après un réglage spécifique.
- Orientation et Angles On/Off définissent quelles entrées chaque haut-parleur amplifiera. Par défaut les haut-parleurs pointent vers le public, dos à la scène. Les entrées dans le secteur vert seront amplifiées, mais pas celles devant le haut-parleur, dans le secteur rouge. Il y a un fondu entre les deux secteurs. Pour les haut-parleurs de sub-basses qui sont généralement en nombre et emplacements limités, ouvrir complètement au maximum permettra d'avoir toutes les entrées potentiellement captées par les subwoofers.
- L'Atténuation HF simule la perte de hautes fréquences avec la distance. Les haut-parleurs proches des auditeurs peuvent en avoir plus que ceux éloignés de la scène et des auditeurs.
- Le pourcentage d'Atténuation par Distance permet de définir si plus ou moins de l'atténuation calculée à partir de la distance et des paramètres de l'entrée est appliquée. Encore pour les Sub-basses, si vous n'en avez que deux et ne voulez pas trop perdre de niveau ou avoir des points chauds, il peut être judicieux de baisser ce réglage à 50%.
- La Latence Minimale active si la sortie est scannée pour le plus petit délai calculé et aussi si le réglage une fois engagé sur une entrée réduira le délai pour cette entrée à travers ce haut-parleur.
- L'Atténuation Live Source active la réduction de niveau des entrées proches si elles ont ce réglage activé. Cela peut ne pas être nécessaire pour les haut-parleurs éloignés du public ou pour les sub-basses.
- Les Réflexions au Sol activent si les réflexions sont appliquées au signal pour cette sortie comme les sub-basses et les arrays suspendus...
  - [ ] OK    Fix: 

- **`title`**
  - EN: Advanced Parameters
  - FR: Paramètres avancés
  - [ ] OK    Fix: 

## `help.outputs`

- **`body`**
  - EN: The design of the WFS system has to do with the right choice of equipment and their position. Here is a guide to help you with the design and the tuning of your arrays.
An array is a line (straight or curved) of speakers. This is one of the most important concepts in WFS adapted to sound reinforcement and for creative sound design.
A rule of thumb would be that each listener should hear three speakers of an array to have enough psycho-acoustic cues to feel the direction each sound is coming from. There will be a sweet spot to find between the distance between the speakers and the listeners, their spacing and coverage angle. This is especially true for short range arrays, a.k.a front fills. Speakers with a 120° coverage angle can be spaced by the same distance between the array and the first row of listeners. The number of speakers also depends on the sound pressure level. When placed as a flown array, trapezoidal/asymmetric horns with wide coverage angle (120°) below axis and narrow coverage (60°) on axis will give good coverage for seat rows a bit too far from the ground array and will have enough throw to reach 20m or 30m away while avoiding the walls of the venue where they would create reflections that give away the position of the speakers to the ears of the listeners. Most of the time for larger venues coaxial (elliptical or conical horns) have not enough reach and require one or more delay lines. They are more suited for smaller venues with few rows of seats.
The positioning of the speakers in the system can be done through the 'Wizard of OutZ' and the editable presets it has.
  - FR: La conception du système WFS a trait au bon choix d'équipement et à leur positionnement. Voici un guide pour vous aider avec la conception et l'accord de vos arrays.
Un array est une ligne (droite ou courbée) de haut-parleurs. C'est l'un des concepts les plus importants en WFS adapté au renforcement sonore et au design sonore créatif.
Une règle générale serait que chaque auditeur devrait entendre trois haut-parleurs d'un array pour avoir suffisamment d'indices psycho-acoustiques pour sentir la direction de chaque son. Il y aura un sweet spot à trouver entre la distance entre les haut-parleurs et les auditeurs, leur espacement et leur angle de couverture. C'est particulièrement vrai pour les arrays de courte portée, aussi appelés front fills. Des haut-parleurs avec un angle de couverture de 120° peuvent être espacés de la même distance entre l'array et le premier rang d'auditeurs. Le nombre de haut-parleurs dépend aussi du niveau de pression sonore. Quand placés en array suspendu, des pavillons trapézoïdaux/asymétriques avec un large angle de couverture (120°) sous l'axe et une couverture étroite (60°) dans l'axe donneront une bonne couverture pour les rangées un peu trop loin de l'array au sol et auront assez de portée pour atteindre 20m ou 30m tout en évitant les murs de la salle où ils créeraient des réflexions trahissant la position des haut-parleurs. La plupart du temps pour les grandes salles les haut-parleurs coaxiaux (pavillons elliptiques ou coniques) n'ont pas assez de portée et nécessitent une ou plusieurs lignes de délai. Ils conviennent mieux aux petites salles avec peu de rangées.
Le positionnement des haut-parleurs dans le système peut se faire via le 'Wizard of OutZ' et ses presets éditables.
  - [ ] OK    Fix: 

- **`title`**
  - EN: WFS Array Design
  - FR: Conception d'array WFS
  - [ ] OK    Fix: 

## `help.overview`

- **`body`**
  - EN: This WFS spatial sound processor aims to be a tool for natural sound reinforcement and also a creative tool opening new avenues for writing sound in space.
Some parameters are straightforward: place sound (Map, Tracking, Speed Limiting, Gradient Maps...), work its shape (Attenuation Profile) and its acoustic presence (Directivity, Floor Reflections), give it a one-off motion (AutomOtion) or a repetitive movement (L.F.O). In some cases amplification should be limited around loud sources on stage (Live Source Tamer). All of these functionalities can be stored and recalled internally or with the help of QLab. On the other side the system allows real-time interaction to trigger and move samples, move large clusters of inputs manually or thanks to easily recallable LFO presets.
  - FR: Ce processeur spatial WFS se veut un outil de renforcement sonore naturel et aussi un outil créatif ouvrant de nouvelles voies pour écrire le son dans l'espace.
Certains paramètres sont simples : placer le son (Carte, Tracking, Limitation de vitesse, Cartes de gradient...), travailler sa forme (Profil d'atténuation) et sa présence acoustique (Directivité, Réflexions au sol), lui donner un mouvement ponctuel (AutomOtion) ou répétitif (L.F.O). Dans certains cas l'amplification devrait être limitée autour de sources fortes sur scène (Live Source Tamer). Toutes ces fonctions peuvent être mémorisées et rappelées en interne ou avec l'aide de QLab. D'autre part le système permet l'interaction en temps réel pour déclencher et déplacer des samples, déplacer de grands clusters d'entrées manuellement ou grâce à des presets LFO facilement rappelables.
  - [ ] OK    Fix: 

- **`dontShow`**
  - EN: Don't show this again
  - FR: Ne plus afficher
  - [ ] OK    Fix: 

- **`title`**
  - EN: System Overview
  - FR: Vue d'ensemble du système
  - [ ] OK    Fix: 

## `help.parallax`

- **`body`**
  - EN: Each speaker points more or less clearly at a listener. To calculate the delay for an input for each speaker, let's consider the distance from the input to this listener, we can also calculate the distance the sound from the speaker to this listener. To match the time of arrival of both we need to apply the difference in the aforementioned distances as a delay for this input and this speaker. This gives greater stability when inputs are moved on stage and especially when they move upstage, away from the edge of the stage. This also can enable the synthesis of floor reflections. This setting can be tuned for finer adjustments, rather than simply measured. Use your ears!
  - FR: Chaque haut-parleur pointe plus ou moins clairement vers un auditeur. Pour calculer le délai pour une entrée pour chaque haut-parleur, considérons la distance de l'entrée à cet auditeur, nous pouvons aussi calculer la distance du son du haut-parleur à cet auditeur. Pour faire correspondre le temps d'arrivée des deux nous devons appliquer la différence des distances mentionnées comme délai pour cette entrée et ce haut-parleur. Cela donne une plus grande stabilité quand les entrées sont déplacées sur scène et surtout quand elles s'éloignent du bord de la scène. Cela peut aussi permettre la synthèse des réflexions au sol. Ce réglage peut être ajusté finement, plutôt que simplement mesuré. Faites confiance à vos oreilles !
  - [ ] OK    Fix: 

- **`title`**
  - EN: Parallax Correction
  - FR: Correction de parallaxe
  - [ ] OK    Fix: 

## `help.reverb`

- **`body`**
  - EN: Reverb helps blur the actual reflections of the speakers in the acoustic space. Without some reverb the listener might perceive the sound missing some depth because the reflections of the speakers in the acoustic of the room gives away their positions and all sounds might feel as coming from the plane of the speakers.
Place the reverb node according to the number of channels and the geometry of the stage and listening area, and the positions of speakers. If necessary the return position can be offset from the feed position.
Other parameters are very similar to Outputs' and Inputs'.
  - FR: La réverbération aide à estomper les réflexions réelles des haut-parleurs dans l'espace acoustique. Sans réverbération, l'auditeur pourrait percevoir un manque de profondeur.
Placez les nœuds de réverbération selon le nombre de canaux et la géométrie de la scène. Si nécessaire, la position de retour peut être décalée de la position d'alimentation.
Les autres paramètres sont similaires à ceux des Sorties et Entrées.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb
  - FR: Réverbération
  - [ ] OK    Fix: 

## `help.reverbAlgo`

- **`body`**
  - EN: There are three built-in reverb types inside this spatial sound processor:
- SDN (Scattered Delay Network): The sound bounces between every reverb node which act as reflecting surfaces. This reverb algorithm favours an odd number of nodes that should not have too much symmetry. This helps reduce the artifacts of the reverb or any metallic sounding resonance.
- FDN (Feedback Delay Network): Each reverb node functions as a separate reverb processor with a classical reverb algorithm. Place nodes around the stage and eventually around the audience. Inputs are sent to each node in a similar fashion as with outputs. The return signal from the reverb is then distributed like other inputs to all outputs.
- IR (Impulse Response): This is a classical convolution reverb. You can load audio samples as impulse responses. Each reverb node can share the same IR or they can have different ones. Place nodes around the stage and eventually around the audience.
The node positions can be adjusted directly on the map. The Ctrl/Cmd key will move a reverb node pair in symmetry.
  - FR: Trois types de réverbération sont intégrés dans ce processeur de son spatial :
- SDN (Scattered Delay Network) : Le son rebondit entre chaque nœud de réverbération qui agit comme des surfaces réfléchissantes. Cet algorithme favorise un nombre impair de nœuds sans trop de symétrie, pour réduire les artefacts ou les résonances métalliques.
- FDN (Feedback Delay Network) : Chaque nœud fonctionne comme un processeur de réverbération séparé avec un algorithme classique. Placez les nœuds autour de la scène et éventuellement autour du public.
- IR (Réponse Impulsionnelle) : Réverbération par convolution classique. Vous pouvez charger des échantillons audio comme réponses impulsionnelles. Chaque nœud peut partager la même IR ou utiliser des IR différentes.
Les positions des nœuds peuvent être ajustées directement sur la carte. La touche Ctrl/Cmd déplace une paire de nœuds de réverbération en symétrie.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Algorithms
  - FR: Algorithmes de Réverbération
  - [ ] OK    Fix: 

## `help.reverbFeed`

- **`body`**
  - EN: Pre-processing sending of the input channels to the reverb nodes.
- Orientation and On/Off Angles define what inputs each reverb node will receive. Inputs in the green sector will be picked-up, but not the ones in front in the red sector. There is a fade between both sectors.
- HF Damping simulates the loss of high frequency with distance.
- Distance Attenuation percentage allows you to define if more or less of the attenuation calculated from the distance and parameters of the input is applied.
- Minimal Latency toggles whether the output is scanned for the smallest calculated delay and also if the setting once engaged on an input will reduce the delay for that input through this reverb node.
- Live Source Attenuation toggles the reduction of level of nearby input if they have this setting enabled.
  - FR: Envoi pré-traitement des canaux d'entrée aux nœuds de réverbération.
- Orientation et angles On/Off définissent quelles entrées chaque nœud recevra.
- Amortissement HF simule la perte de hautes fréquences avec la distance.
- Pourcentage d'atténuation de distance définit l'atténuation appliquée.
- Latence minimale détermine si le plus petit délai calculé est utilisé.
- Atténuation Source Live active la réduction de niveau des entrées proches.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Feed
  - FR: Alimentation Réverbération
  - [ ] OK    Fix: 

## `help.reverbPostProc`

- **`body`**
  - EN: Includes a 4-band EQ and an Expander that looks at the signal entering the reverb processor to reduce long reverb tails when the inputs are quiet.
  - FR: Comprend un EQ 4 bandes et un Expandeur qui surveille le signal entrant dans le processeur de réverbération pour réduire les longues queues de réverbération lorsque les entrées sont silencieuses.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Post-Processing
  - FR: Post-Traitement Réverbération
  - [ ] OK    Fix: 

## `help.reverbPreProc`

- **`body`**
  - EN: Includes a 4-band EQ and a Compressor to remove transients which might excite the reverb processor a bit too much.
  - FR: Comprend un EQ 4 bandes et un Compresseur pour supprimer les transitoires qui pourraient exciter le processeur de réverbération un peu trop.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Pre-Processing
  - FR: Pré-Traitement Réverbération
  - [ ] OK    Fix: 

## `help.reverbReturn`

- **`body`**
  - EN: Post-processing sending to the speakers.
- Distance Attenuation defines the level drop per meter to the speakers.
- Common Attenuation will look for the lowest attenuation and keep only a percentage of this level drop and apply this across all other outputs.
- Mutes and Mute Macros allow to prevent a reverb channel from feeding an output. For instance it may not be necessary to send reverb to sub-bass speakers.
  - FR: Post-traitement envoyé aux haut-parleurs.
- Atténuation de Distance définit la perte de niveau par mètre.
- Atténuation Commune garde un pourcentage de la plus faible atténuation.
- Mutes et Macros de Mute empêchent un canal de réverbération d'alimenter une sortie.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Return
  - FR: Retour Réverbération
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
  - FR: Le sampler permet de déclencher des échantillons et d'interagir avec eux en temps réel.
Lorsqu'il est activé sur une piste, le sampler remplace l'entrée live en permanence.
Plusieurs samplers peuvent être assignés à différentes entrées et déclenchés individuellement.
Pour utiliser le sampler :
- Sélectionnez un Roli Lightpad ou un pad sur l'application Android Remote connectée.
- Ajoutez des échantillons aux différentes tuiles de la grille. Ajustez leur position de départ relative, leur niveau et éventuellement leurs points d'entrée et de sortie. Plusieurs échantillons peuvent être sélectionnés en maintenant Maj tout en cliquant.
- Créez des sets d'échantillons : les échantillons sélectionnés seront ajoutés aux nouveaux sets. Ils peuvent être ajoutés ou retirés après la création d'un set en maintenant Ctrl/Cmd tout en cliquant sur les tuiles. Chaque set peut être renommé. Chaque set peut avoir une séquence fixe ou un ordre aléatoire (tourniquet, chaque échantillon est joué une fois avant un nouveau tirage). Chaque set a un réglage d'atténuation et une position de base appliquée à l'entrée.
- Appuyez sur un Lightpad ou un pad pour déclencher un échantillon. La pression peut être mappée au niveau, à la hauteur et au filtrage haute fréquence. Le mouvement du doigt fait bouger le son comme un joystick.
Relâcher le pad arrête l'échantillon.
Les réglages du sampler sont stockés dans les fichiers d'entrée.
Les tuiles et sets peuvent être copiés, exportés et importés.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Sampler
  - FR: Sampler
  - [ ] OK    Fix: 

## `help.sessionData`

- **`body`**
  - EN: When starting a session select the working folder where the system will place the files and eventual audio files. For new projects create a new folder. When reloading a previous session navigate to the corresponding folder.
Each section has a separate xml file (System configuration, Network, Outputs, Reverbs, Inputs) and backups. Additionally convolution reverb impulse responses and audio samples will be stored in sub-directories.
Each section can be stored and recalled individually or as a whole.
Each section can also export and import files from other projects.
  - FR: Au démarrage d'une session, sélectionnez le dossier de travail où le système placera les fichiers et les éventuels fichiers audio. Pour les nouveaux projets, créez un nouveau dossier. Pour recharger une session précédente, naviguez vers le dossier correspondant.
Chaque section a un fichier xml séparé (Configuration système, Réseau, Sorties, Réverbérations, Entrées) et des sauvegardes. Les réponses impulsionnelles de réverbération par convolution et les échantillons audio seront stockés dans des sous-répertoires.
Chaque section peut être stockée et rappelée individuellement ou dans son ensemble.
Chaque section peut aussi exporter et importer des fichiers d'autres projets.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Session Data
  - FR: Données de Session
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
  - FR: Les snapshots stockent les paramètres d'entrée, mais peuvent avoir une portée pour être rappelés pendant un spectacle. Ils peuvent contenir de tous les paramètres pour toutes les entrées à un seul paramètre pour un seul canal.
La Portée indique au système quelles données stocker ou rappeler. C'est l'opposé des paramètres 'sûrs'.
Plusieurs méthodes sont disponibles :
- Enregistrer uniquement les données nécessaires dans des fichiers locaux. Le filtre de portée est appliqué lors du stockage.
- Enregistrer toutes les données et un filtre dans des fichiers locaux. Le filtre est appliqué au rappel, permettant éventuellement de tout rappeler.
- Enregistrer toutes les données dans des cues QLab. Non recommandé pour les grandes configurations.
La portée peut afficher et présélectionner automatiquement les paramètres modifiés manuellement. Les paramètres modifiés sont marqués en jaune.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Input Snapshots and Scope
  - FR: Snapshots d'Entrée et Portée
  - [ ] OK    Fix: 

## `help.tracking`

- **`body`**
  - EN: Tracking allows to follow the position in 2D or 3D of actors and musicians. There are several tracking solutions based on UWB (UltraWide Band) tags, 3D camera, computer vision systems and Infra-Red LED's followed by IR sensitive cameras.
This application allows you to receive tracking data from several protocols: OSC, MQTT, PosiStageNet/PSN, RTTrP.
You can select which protocol you're using and enter its settings. You may also adjust the mapping (offset, scaling and orientation).
Each input has a toggle to enable tracking, an ID to select which tracking marker it should follow and a smoothing algorithm to reduce jittery movements.
  - FR: Le tracking permet de suivre la position en 2D ou 3D des acteurs et musiciens. Il existe plusieurs solutions basées sur des tags UWB (Ultra Large Bande), des caméras 3D, des systèmes de vision par ordinateur et des LED infrarouges suivies par des caméras sensibles à l'IR.
Cette application permet de recevoir des données de tracking depuis plusieurs protocoles : OSC, MQTT, PosiStageNet/PSN, RTTrP.
Vous pouvez sélectionner le protocole utilisé et entrer ses paramètres. Vous pouvez aussi ajuster le mapping (décalage, mise à l'échelle et orientation).
Chaque entrée a un toggle pour activer le tracking, un ID pour sélectionner le marqueur à suivre et un algorithme de lissage pour réduire les mouvements saccadés.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Tracking
  - FR: Tracking
  - [ ] OK    Fix: 

## `help.tuning`

- **`body`**
  - EN: System tuning for WFS is different from standard PA tuning. It can proceed as follows:
- Start with the flown array muted. Set the desired levels for the near-field speakers when listening to them in the first row. Adjust the high frequency shelf attenuation so the near-field speakers are not too bright and draw too much attention.
- Mute the near-field array and unmute the flown array, find a suitable level towards the back of the house.
- Unmute both arrays, adjust the delay of the flown array to bring the sound down to the correct height in the lower rows. Adjust the levels, HF shelf/distance ratio and vertical and horizontal parallax for each array to have consistent level wherever your inputs are on stage. This is especially important if your inputs are actors, singers or acoustic music instruments on stage. You may also test adding some Haas effect to delay the whole system if you feel the WFS is too early or adding comb filtering with the acoustic sound.
You may follow a different workflow for tuning or go for different cues.
  - FR: L'accord du système WFS est différent de l'accord PA standard. Il peut procéder comme suit :
- Commencez avec l'array suspendu coupé. Réglez les niveaux souhaités pour les haut-parleurs de proximité en les écoutant depuis le premier rang. Ajustez l'atténuation du shelf haute fréquence pour que les haut-parleurs de proximité ne soient pas trop brillants et n'attirent pas trop l'attention.
- Coupez l'array de proximité et activez l'array suspendu, trouvez un niveau approprié vers le fond de la salle.
- Activez les deux arrays, ajustez le délai de l'array suspendu pour ramener le son à la bonne hauteur dans les rangées inférieures. Ajustez les niveaux, le shelf HF/ratio de distance et la parallaxe verticale et horizontale pour chaque array pour avoir un niveau cohérent où que soient vos entrées sur scène. C'est particulièrement important si vos entrées sont des acteurs, chanteurs ou instruments acoustiques sur scène. Vous pouvez aussi tester l'ajout d'effet Haas pour retarder tout le système si vous sentez que le WFS est trop en avance ou ajoute du filtrage en peigne avec le son acoustique.
Vous pouvez suivre un workflow différent pour l'accord ou viser des réglages différents selon les situations.
  - [ ] OK    Fix: 

- **`title`**
  - EN: System Tuning
  - FR: Accord du système
  - [ ] OK    Fix: 

## `inputs`

- **`arrayPrefix`**
  - EN: Array
  - FR: Réseau
  - [ ] OK    Fix: 

## `inputs.buttons`

- **`deleteSnapshot`**
  - EN: Delete Snapshot
  - FR: Supprimer Snapshot
  - [ ] OK    Fix: 

- **`editScope`**
  - EN: Edit Scope
  - FR: Modifier portée
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - FR: Exporter
  - [ ] OK    Fix: 

- **`hiddenOnMap`**
  - EN: Input Hidden on Map
  - FR: Entrée masquée sur la carte
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - FR: Importer
  - [ ] OK    Fix: 

- **`lockOnMap`**
  - EN: Lock on Map
  - FR: Verrouiller sur la carte
  - [ ] OK    Fix: 

- **`pauseAll`**
  - EN: Pause All
  - FR: Tout suspendre
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Backup
  - FR: Recharger sauvegarde
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Input Config
  - FR: Recharger Config Entrées
  - [ ] OK    Fix: 

- **`reloadSnapshot`**
  - EN: Reload Snapshot
  - FR: Recharger Snapshot
  - [ ] OK    Fix: 

- **`reloadWithoutScope`**
  - EN: Reload Without Scope
  - FR: Recharger Sans Filtre
  - [ ] OK    Fix: 

- **`resumeAll`**
  - EN: Resume All
  - FR: Tout reprendre
  - [ ] OK    Fix: 

- **`samplerOff`**
  - EN: Sampler: OFF
  - FR: Sampler : OFF
  - [ ] OK    Fix: 

- **`samplerOn`**
  - EN: Sampler: ON
  - FR: Sampler : ON
  - [ ] OK    Fix: 

- **`setAllInputs`**
  - EN: Set all Inputs...
  - FR: Définir toutes les entrées...
  - [ ] OK    Fix: 

- **`solo`**
  - EN: Solo
  - FR: Solo
  - [ ] OK    Fix: 

- **`soloModeMulti`**
  - EN: Multi
  - FR: Multi
  - [ ] OK    Fix: 

- **`soloModeSingle`**
  - EN: Single
  - FR: Single
  - [ ] OK    Fix: 

- **`stopAll`**
  - EN: Stop All
  - FR: Tout arrêter
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Input Config
  - FR: Enregistrer Config Entrées
  - [ ] OK    Fix: 

- **`storeSnapshot`**
  - EN: Store Snapshot
  - FR: Enregistrer Snapshot
  - [ ] OK    Fix: 

- **`updateSnapshot`**
  - EN: Update Snapshot
  - FR: Mettre à jour Snapshot
  - [ ] OK    Fix: 

- **`visibleOnMap`**
  - EN: Input Visible on Map
  - FR: Entrée visible sur la carte
  - [ ] OK    Fix: 

## `inputs.clusters`

- **`clusterPrefix`**
  - EN: Cluster
  - FR: Groupe
  - [ ] OK    Fix: 

- **`single`**
  - EN: Single
  - FR: Seul
  - [ ] OK    Fix: 

## `inputs.coordinates`

- **`xyz`**
  - EN: XYZ
  - FR: XYZ
  - [ ] OK    Fix: 

## `inputs.dialogs`

- **`exportConfig`**
  - EN: Export Input Configuration
  - FR: Exporter configuration entrées
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Input Configuration
  - FR: Importer configuration entrées
  - [ ] OK    Fix: 

- **`selectChannel`**
  - EN: Select Channel
  - FR: Sélectionner le canal
  - [ ] OK    Fix: 

- **`snapshotNameLabel`**
  - EN: Name:
  - FR: Nom :
  - [ ] OK    Fix: 

- **`storeSnapshotMessage`**
  - EN: Enter a name for the new snapshot:
  - FR: Entrez un nom pour le nouveau snapshot :
  - [ ] OK    Fix: 

- **`storeSnapshotTitle`**
  - EN: Store New Snapshot
  - FR: Enregistrer un nouveau snapshot
  - [ ] OK    Fix: 

- **`trackingConflictCluster`**
  - EN: Input {current} has tracking enabled, but Input {existing} in Cluster {cluster} is already tracked.

Only one tracked input per cluster is allowed.
  - FR: L'entrée {current} a le tracking activé, mais l'entrée {existing} du cluster {cluster} est déjà suivie.

Un seul tracking actif par cluster est autorisé.
  - [ ] OK    Fix: 

- **`trackingConflictContinue`**
  - EN: Continue (disable tracking)
  - FR: Continuer (désactiver le tracking)
  - [ ] OK    Fix: 

- **`trackingConflictSwitch`**
  - EN: Input {existing} in Cluster {cluster} already has tracking enabled.

Only one tracked input per cluster is allowed.

Do you want to disable tracking on Input {existing} and enable it on Input {to}?
  - FR: L'entrée {existing} du cluster {cluster} a déjà le tracking activé.

Un seul tracking actif par cluster est autorisé.

Voulez-vous désactiver le tracking sur l'entrée {existing} et l'activer sur l'entrée {to} ?
  - [ ] OK    Fix: 

- **`trackingConflictTitle`**
  - EN: Tracking Conflict
  - FR: Conflit de tracking
  - [ ] OK    Fix: 

- **`trackingConflictYes`**
  - EN: Yes, switch tracking
  - FR: Oui, transférer le tracking
  - [ ] OK    Fix: 

## `inputs.gradientMap`

- **`buttons.copyLayer`**
  - EN: Copy Layer
  - FR: Copier la couche
  - [ ] OK    Fix: 

- **`buttons.copyShape`**
  - EN: Copy Shape
  - FR: Copier la forme
  - [ ] OK    Fix: 

- **`buttons.delete`**
  - EN: Delete
  - FR: Supprimer
  - [ ] OK    Fix: 

- **`buttons.enable`**
  - EN: On
  - FR: On
  - [ ] OK    Fix: 

- **`buttons.lock`**
  - EN: Lock
  - FR: Verrouiller
  - [ ] OK    Fix: 

- **`buttons.pasteLayer`**
  - EN: Paste Layer
  - FR: Coller la couche
  - [ ] OK    Fix: 

- **`buttons.pasteShape`**
  - EN: Paste Shape
  - FR: Coller la forme
  - [ ] OK    Fix: 

- **`header.attenuationLayer`**
  - EN: Attenuation Layer
  - FR: Couche Atténuation
  - [ ] OK    Fix: 

- **`header.heightLayer`**
  - EN: Height Layer
  - FR: Couche Hauteur
  - [ ] OK    Fix: 

- **`header.hfShelfLayer`**
  - EN: HF Shelf Layer
  - FR: Couche Plateau HF
  - [ ] OK    Fix: 

- **`help.blackValue`**
  - EN: Parameter value mapped to black (0.00–1.00)
  - FR: Valeur du paramètre mappée au noir (0.00–1.00)
  - [ ] OK    Fix: 

- **`help.blur`**
  - EN: Edge blur in meters
  - FR: Flou des bords en mètres
  - [ ] OK    Fix: 

- **`help.copy`**
  - EN: Copy selected shape or layer
  - FR: Copier la forme ou la couche sélectionnée
  - [ ] OK    Fix: 

- **`help.curve`**
  - EN: Gamma curve (-1 to 1, 0 = linear)
  - FR: Courbe gamma (-1 à 1, 0 = linéaire)
  - [ ] OK    Fix: 

- **`help.ellipseTool`**
  - EN: Draw ellipse
  - FR: Dessiner une ellipse
  - [ ] OK    Fix: 

- **`help.fillTool`**
  - EN: Apply uniform fill to shape
  - FR: Appliquer un remplissage uniforme à la forme
  - [ ] OK    Fix: 

- **`help.fillValue`**
  - EN: Fill brightness (0 = black, 1 = white)
  - FR: Luminosité de remplissage (0 = noir, 1 = blanc)
  - [ ] OK    Fix: 

- **`help.layerEnable`**
  - EN: Enable/disable layer (affects output and OSC)
  - FR: Activer/désactiver la couche (affecte la sortie et OSC)
  - [ ] OK    Fix: 

- **`help.layerSelect`**
  - EN: Select this layer for editing
  - FR: Sélectionner cette couche pour l'édition
  - [ ] OK    Fix: 

- **`help.layerVisible`**
  - EN: Show/hide layer preview on canvas
  - FR: Afficher/masquer l'aperçu de la couche sur le canevas
  - [ ] OK    Fix: 

- **`help.linGradTool`**
  - EN: Apply linear gradient to shape
  - FR: Appliquer un dégradé linéaire à la forme
  - [ ] OK    Fix: 

- **`help.paste`**
  - EN: Paste shape or layer from clipboard
  - FR: Coller la forme ou la couche depuis le presse-papiers
  - [ ] OK    Fix: 

- **`help.polygonTool`**
  - EN: Draw polygon (double-click to close)
  - FR: Dessiner un polygone (double-clic pour fermer)
  - [ ] OK    Fix: 

- **`help.radGradTool`**
  - EN: Apply radial gradient to shape
  - FR: Appliquer un dégradé radial à la forme
  - [ ] OK    Fix: 

- **`help.rectTool`**
  - EN: Draw rectangle
  - FR: Dessiner un rectangle
  - [ ] OK    Fix: 

- **`help.selectTool`**
  - EN: Select and move shapes
  - FR: Sélectionner et déplacer les formes
  - [ ] OK    Fix: 

- **`help.shapeDelete`**
  - EN: Delete selected shape(s)
  - FR: Supprimer la ou les formes sélectionnées
  - [ ] OK    Fix: 

- **`help.shapeEnable`**
  - EN: Enable/disable shape
  - FR: Activer/désactiver la forme
  - [ ] OK    Fix: 

- **`help.shapeLock`**
  - EN: Lock shape position
  - FR: Verrouiller la position de la forme
  - [ ] OK    Fix: 

- **`help.whiteValue`**
  - EN: Parameter value mapped to white (0.00–1.00)
  - FR: Valeur du paramètre mappée au blanc (0.00–1.00)
  - [ ] OK    Fix: 

- **`hints.darkMaxAtten`**
  - EN: Dark = max attenuation | Light = none
  - FR: Foncé = atténuation max | Clair = aucune
  - [ ] OK    Fix: 

- **`hints.darkMaxHeight`**
  - EN: Dark = max height | Light = ground
  - FR: Foncé = hauteur max | Clair = sol
  - [ ] OK    Fix: 

- **`hints.darkMaxHF`**
  - EN: Dark = max HF shelf | Light = none
  - FR: Foncé = plateau HF max | Clair = aucun
  - [ ] OK    Fix: 

- **`hints.polygonClose`**
  - EN: Double-click to close polygon
  - FR: Double-cliquez pour fermer le polygone
  - [ ] OK    Fix: 

- **`hints.whiteMaxAtten`**
  - EN: White = max attenuation | Black = none
  - FR: Blanc = atténuation max | Noir = aucune
  - [ ] OK    Fix: 

- **`hints.whiteMaxHeight`**
  - EN: White = max height | Black = ground
  - FR: Blanc = hauteur max | Noir = sol
  - [ ] OK    Fix: 

- **`hints.whiteMaxHF`**
  - EN: White = max HF shelf | Black = none
  - FR: Blanc = plateau HF max | Noir = aucun
  - [ ] OK    Fix: 

- **`labels.black`**
  - EN: Black:
  - FR: Noir :
  - [ ] OK    Fix: 

- **`labels.blur`**
  - EN: Blur:
  - FR: Flou :
  - [ ] OK    Fix: 

- **`labels.center`**
  - EN: Center:
  - FR: Centre :
  - [ ] OK    Fix: 

- **`labels.curve`**
  - EN: Curve:
  - FR: Courbe :
  - [ ] OK    Fix: 

- **`labels.edge`**
  - EN: Edge:
  - FR: Bord :
  - [ ] OK    Fix: 

- **`labels.end`**
  - EN: End:
  - FR: Fin :
  - [ ] OK    Fix: 

- **`labels.fill`**
  - EN: Fill:
  - FR: Remplir :
  - [ ] OK    Fix: 

- **`labels.name`**
  - EN: Name:
  - FR: Nom :
  - [ ] OK    Fix: 

- **`labels.start`**
  - EN: Start:
  - FR: Début :
  - [ ] OK    Fix: 

- **`labels.white`**
  - EN: White:
  - FR: Blanc :
  - [ ] OK    Fix: 

- **`layers.attenuation`**
  - EN: Attenuation
  - FR: Atténuation
  - [ ] OK    Fix: 

- **`layers.height`**
  - EN: Height
  - FR: Hauteur
  - [ ] OK    Fix: 

- **`layers.hfShelf`**
  - EN: HF Shelf
  - FR: Plateau HF
  - [ ] OK    Fix: 

- **`tools.editPoints`**
  - EN: Edit Points
  - FR: Édit. Points
  - [ ] OK    Fix: 

- **`tools.ellipse`**
  - EN: Ellipse
  - FR: Ellipse
  - [ ] OK    Fix: 

- **`tools.fill`**
  - EN: Fill
  - FR: Remplir
  - [ ] OK    Fix: 

- **`tools.linGrad`**
  - EN: Linear Grad.
  - FR: Grad. Linéaire
  - [ ] OK    Fix: 

- **`tools.polygon`**
  - EN: Polygon
  - FR: Polygone
  - [ ] OK    Fix: 

- **`tools.radGrad`**
  - EN: Radial Grad.
  - FR: Grad. Radial
  - [ ] OK    Fix: 

- **`tools.rect`**
  - EN: Rectangle
  - FR: Rectangle
  - [ ] OK    Fix: 

- **`tools.select`**
  - EN: Select
  - FR: Sélection
  - [ ] OK    Fix: 

- **`warnings.heightRatioZero`**
  - EN: Height Ratio is 0% — increase it for height to take effect
  - FR: Le ratio de hauteur est à 0% — augmentez-le pour que la hauteur soit prise en compte
  - [ ] OK    Fix: 

## `inputs.help`

- **`admMapping`**
  - EN: Assign this input to an ADM-OSC mapping for position receive/transmit.
  - FR: Assigner cette entrée à un mappage ADM-OSC pour la réception/transmission de position.
  - [ ] OK    Fix: 

- **`arrayAttenDial`**
  - EN: Attenuation for Array {num} (-60 to 0 dB).
  - FR: Atténuation pour l'array {num} (-60 à 0 dB).
  - [ ] OK    Fix: 

- **`attenuationLawButton`**
  - EN: Attenuation Law Model (Linear Decrease of Volume with Distance Between Object and Speaker or Squared).
  - FR: Modèle de loi d'atténuation (décroissance linéaire du volume avec la distance entre l'objet et l'enceinte, ou quadratique).
  - [ ] OK    Fix: 

- **`attenuationSlider`**
  - EN: Input Channel Attenuation.
  - FR: Atténuation du canal d'entrée.
  - [ ] OK    Fix: 

- **`channelSelector`**
  - EN: Input Channel Number and Selection.
  - FR: Numéro et sélection du canal d'entrée.
  - [ ] OK    Fix: 

- **`clusterSelector`**
  - EN: Object is Part of a Cluster.
  - FR: L'objet fait partie d'un cluster.
  - [ ] OK    Fix: 

- **`commonAttenDial`**
  - EN: Percentage of the Common Part of the Attenuation for selected Object Relative to All Outputs.
  - FR: Pourcentage de la partie commune de l'atténuation pour l'objet sélectionné par rapport à toutes les sorties.
  - [ ] OK    Fix: 

- **`constraintDistanceButton`**
  - EN: Limit Position to Distance Range from Origin (for Cylindrical/Spherical modes).
  - FR: Limiter la position à une plage de distance depuis l'origine (pour les modes cylindrique/sphérique).
  - [ ] OK    Fix: 

- **`constraintXButton`**
  - EN: Limit Position to the Bounds of the Stage in Width.
  - FR: Limiter la position aux limites de la scène en largeur.
  - [ ] OK    Fix: 

- **`constraintYButton`**
  - EN: Limit Position to the Bounds of the Stage in Depth.
  - FR: Limiter la position aux limites de la scène en profondeur.
  - [ ] OK    Fix: 

- **`constraintZButton`**
  - EN: Limit Position to the Bounds of the Stage in Height.
  - FR: Limiter la position aux limites de la scène en hauteur.
  - [ ] OK    Fix: 

- **`coordModeSelector`**
  - EN: Coordinate display mode: Cartesian (X/Y/Z), Cylindrical (radius/azimuth/height), or Spherical (radius/azimuth/elevation).
  - FR: Mode d'affichage des coordonnées : Cartésien (X/Y/Z), Cylindrique (rayon/azimut/hauteur) ou Sphérique (rayon/azimut/élévation).
  - [ ] OK    Fix: 

- **`delayLatencySlider`**
  - EN: Input Channel Delay (positive values) or Latency Compensation (negative values).
  - FR: Délai du canal d'entrée (valeurs positives) ou compensation de latence (valeurs négatives).
  - [ ] OK    Fix: 

- **`deleteSnapshot`**
  - EN: Delete Selected Input Snapshot With Confirmation.
  - FR: Supprimer le snapshot d'entrée sélectionné avec confirmation.
  - [ ] OK    Fix: 

- **`directivitySlider`**
  - EN: How Wide is the Brightness of The Object.
  - FR: Largeur du cône de brillance de l'objet.
  - [ ] OK    Fix: 

- **`distanceAttenDial`**
  - EN: Attenuation per Meter Between Object and Speaker.
  - FR: Atténuation par mètre entre l'objet et l'enceinte.
  - [ ] OK    Fix: 

- **`distanceMaxEditor`**
  - EN: Maximum Distance from Origin in Meters.
  - FR: Distance maximale depuis l'origine en mètres.
  - [ ] OK    Fix: 

- **`distanceMinEditor`**
  - EN: Minimum Distance from Origin in Meters.
  - FR: Distance minimale depuis l'origine en mètres.
  - [ ] OK    Fix: 

- **`distanceRangeSlider`**
  - EN: Set Minimum and Maximum Distance from Origin.
  - FR: Définir la distance minimale et maximale depuis l'origine.
  - [ ] OK    Fix: 

- **`distanceRatioDial`**
  - EN: Attenuation Ratio for Squared Model.
  - FR: Ratio d'atténuation pour le modèle quadratique.
  - [ ] OK    Fix: 

- **`editScope`**
  - EN: Open Selected Input Snapshot Scope Window.
  - FR: Ouvrir la fenêtre de filtre du snapshot d'entrée sélectionné.
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Input Configuration to file (with file explorer window).
  - FR: Exporter la configuration d'entrée vers un fichier (avec explorateur de fichiers).
  - [ ] OK    Fix: 

- **`flipXButton`**
  - EN: X will be Symetrical to the Origin. Keyboard Nudging will be Inverted.
  - FR: X sera symétrique par rapport à l'origine. La navigation au clavier sera inversée.
  - [ ] OK    Fix: 

- **`flipYButton`**
  - EN: Y will be Symetrical to the Origin. Keyboard Nudging will be Inverted.
  - FR: Y sera symétrique par rapport à l'origine. La navigation au clavier sera inversée.
  - [ ] OK    Fix: 

- **`flipZButton`**
  - EN: Z will be Symetrical to the Origin. Keyboard Nudging will be Inverted.
  - FR: Z sera symétrique par rapport à l'origine. La navigation au clavier sera inversée.
  - [ ] OK    Fix: 

- **`frActiveButton`**
  - EN: Enable Simulated Floor Reflections for the Object.
  - FR: Activer la simulation des réflexions au sol pour l'objet.
  - [ ] OK    Fix: 

- **`frAttenuationSlider`**
  - EN: Attenuation of the Simulated Floor Reflections for the Object.
  - FR: Atténuation des réflexions au sol simulées pour l'objet.
  - [ ] OK    Fix: 

- **`frDiffusionDial`**
  - EN: Diffusion Effect of the Simulated Floor Reflections for the Object.
  - FR: Effet de diffusion des réflexions au sol simulées pour l'objet.
  - [ ] OK    Fix: 

- **`frHighShelfActiveButton`**
  - EN: Enable High Shelf Filter for Floor Reflections.
  - FR: Activer le filtre shelving aigu pour les réflexions au sol.
  - [ ] OK    Fix: 

- **`frHighShelfFreqSlider`**
  - EN: High Shelf Frequency for Floor Reflections.
  - FR: Fréquence du shelving aigu pour les réflexions au sol.
  - [ ] OK    Fix: 

- **`frHighShelfGainSlider`**
  - EN: High Shelf Gain for Floor Reflections.
  - FR: Gain du shelving aigu pour les réflexions au sol.
  - [ ] OK    Fix: 

- **`frHighShelfSlopeSlider`**
  - EN: High Shelf Slope for Floor Reflections.
  - FR: Pente du shelving aigu pour les réflexions au sol.
  - [ ] OK    Fix: 

- **`frLowCutActiveButton`**
  - EN: Enable Low Cut Filter for Floor Reflections.
  - FR: Activer le filtre coupe-bas pour les réflexions au sol.
  - [ ] OK    Fix: 

- **`frLowCutFreqSlider`**
  - EN: Low Cut Frequency for Floor Reflections.
  - FR: Fréquence du coupe-bas pour les réflexions au sol.
  - [ ] OK    Fix: 

- **`heightFactorDial`**
  - EN: Take Elevation of Object into Account Fully, Partially or Not.
  - FR: Prendre en compte l'élévation de l'objet entièrement, partiellement ou pas du tout.
  - [ ] OK    Fix: 

- **`hfShelfSlider`**
  - EN: How Much Brightness is lost in the Back of the Object, Out of its Brightness Cone.
  - FR: Atténuation des hautes fréquences à l'arrière de l'objet, hors de son cône de brillance.
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Input Configuration from file (with file explorer window).
  - FR: Importer la configuration d'entrée depuis un fichier (avec explorateur de fichiers).
  - [ ] OK    Fix: 

- **`inputDirectivityDial`**
  - EN: Where is the Object pointing to in the Horizontal Plane.
  - FR: Direction de l'objet dans le plan horizontal.
  - [ ] OK    Fix: 

- **`jitterSlider`**
  - EN: Sphere of Rapid Movements of the Object.
  - FR: Sphère des mouvements rapides de l'objet.
  - [ ] OK    Fix: 

- **`lfoActiveButton`**
  - EN: Enable or Disable the Periodic Movement of the Object (LFO).
  - FR: Activer ou désactiver le mouvement périodique de l'objet (LFO).
  - [ ] OK    Fix: 

- **`lfoAmplitudeXSlider`**
  - EN: Width of Movement in Relation to Base Position of the Object.
  - FR: Largeur du mouvement par rapport à la position de base de l'objet.
  - [ ] OK    Fix: 

- **`lfoAmplitudeYSlider`**
  - EN: Depth of Movement in Relation to Base Position of the Object.
  - FR: Profondeur du mouvement par rapport à la position de base de l'objet.
  - [ ] OK    Fix: 

- **`lfoAmplitudeZSlider`**
  - EN: Height of Movement in Relation to Base Position of the Object.
  - FR: Hauteur du mouvement par rapport à la position de base de l'objet.
  - [ ] OK    Fix: 

- **`lfoGyrophoneSelector`**
  - EN: Rotation of the Brightness Cone of the Object.
  - FR: Rotation du cône de brillance de l'objet.
  - [ ] OK    Fix: 

- **`lfoPeriodDial`**
  - EN: Base Period of the Movement of the Object.
  - FR: Période de base du mouvement de l'objet.
  - [ ] OK    Fix: 

- **`lfoPhaseDial`**
  - EN: Phase Offset of the Movement of the Object.
  - FR: Décalage de phase du mouvement de l'objet.
  - [ ] OK    Fix: 

- **`lfoPhaseXDial`**
  - EN: Phase Offset of the Movement of the Object in Width.
  - FR: Décalage de phase du mouvement de l'objet en largeur.
  - [ ] OK    Fix: 

- **`lfoPhaseYDial`**
  - EN: Phase Offset of the Movement of the Object in Depth.
  - FR: Décalage de phase du mouvement de l'objet en profondeur.
  - [ ] OK    Fix: 

- **`lfoPhaseZDial`**
  - EN: Phase Offset of the Movement of the Object in Height.
  - FR: Décalage de phase du mouvement de l'objet en hauteur.
  - [ ] OK    Fix: 

- **`lfoRateXSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Width.
  - FR: Mouvement plus rapide ou plus lent par rapport à la période de base, en largeur.
  - [ ] OK    Fix: 

- **`lfoRateYSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Depth.
  - FR: Mouvement plus rapide ou plus lent par rapport à la période de base, en profondeur.
  - [ ] OK    Fix: 

- **`lfoRateZSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Height.
  - FR: Mouvement plus rapide ou plus lent par rapport à la période de base, en hauteur.
  - [ ] OK    Fix: 

- **`lfoShapeXSelector`**
  - EN: Movement Behaviour of the Object in Width.
  - FR: Comportement du mouvement de l'objet en largeur.
  - [ ] OK    Fix: 

- **`lfoShapeYSelector`**
  - EN: Movement Behaviour of the Object in Depth.
  - FR: Comportement du mouvement de l'objet en profondeur.
  - [ ] OK    Fix: 

- **`lfoShapeZSelector`**
  - EN: Movement Behaviour of the Object in Height.
  - FR: Comportement du mouvement de l'objet en hauteur.
  - [ ] OK    Fix: 

- **`lsActiveButton`**
  - EN: If You Need to Reduce the Level in Speakers Close to the Object. (eg. Loud Source Present on Stage)
  - FR: Si vous avez besoin de réduire le niveau dans les haut-parleurs proches de l'objet (ex. : source forte présente sur scène).
  - [ ] OK    Fix: 

- **`lsAttenuationSlider`**
  - EN: Constant Attenuation of Speakers Around the Object.
  - FR: Atténuation constante des haut-parleurs autour de l'objet.
  - [ ] OK    Fix: 

- **`lsPeakEnableButton`**
  - EN: Enable or Disable the Fast (Peak) Compressor for the Live Source Tamer.
  - FR: Activer ou désactiver le compresseur rapide (crête) pour le Dompteur de Source Live.
  - [ ] OK    Fix: 

- **`lsPeakRatioDial`**
  - EN: Ratio to Apply the Fast Compression for Speakers Around the Object.
  - FR: Ratio appliqué à la compression rapide pour les haut-parleurs autour de l'objet.
  - [ ] OK    Fix: 

- **`lsPeakThresholdSlider`**
  - EN: Fast Compression Threshold for Speakers Around the Object to Control Transients.
  - FR: Seuil de compression rapide pour les haut-parleurs autour de l'objet, pour contrôler les transitoires.
  - [ ] OK    Fix: 

- **`lsRadiusSlider`**
  - EN: How Far does the Attenuation Affect The Speakers.
  - FR: Jusqu'où l'atténuation affecte les haut-parleurs.
  - [ ] OK    Fix: 

- **`lsShapeSelector`**
  - EN: Profile of the Attenuation Around the Object.
  - FR: Profil de l'atténuation autour de l'objet.
  - [ ] OK    Fix: 

- **`lsSlowEnableButton`**
  - EN: Enable or Disable the Slow Compressor for the Live Source Tamer.
  - FR: Activer ou désactiver le compresseur lent pour le Dompteur de Source Live.
  - [ ] OK    Fix: 

- **`lsSlowRatioDial`**
  - EN: Ratio to Apply the Slow Compression for Speakers Around the Object.
  - FR: Ratio appliqué à la compression lente pour les haut-parleurs autour de l'objet.
  - [ ] OK    Fix: 

- **`lsSlowThresholdSlider`**
  - EN: Slow Compression Threshold for Speakers Around the Object to Control Sustained Level.
  - FR: Seuil de compression lente pour les haut-parleurs autour de l'objet, pour contrôler le niveau soutenu.
  - [ ] OK    Fix: 

- **`mapLockButton`**
  - EN: Prevent Interaction on the Map Tab
  - FR: Empêcher l'interaction sur l'onglet Carte.
  - [ ] OK    Fix: 

- **`mapVisibilityButton`**
  - EN: Make Visible or Hide The Selected Input on the Map
  - FR: Afficher ou masquer l'entrée sélectionnée sur la carte.
  - [ ] OK    Fix: 

- **`maxSpeedActiveButton`**
  - EN: Enable or Disable Speed Limiting for Object.
  - FR: Activer ou désactiver la limitation de vitesse pour l'objet.
  - [ ] OK    Fix: 

- **`maxSpeedDial`**
  - EN: Maximum Speed Limit for Object.
  - FR: Limite de vitesse maximale pour l'objet.
  - [ ] OK    Fix: 

- **`minimalLatencyButton`**
  - EN: Select between Acoustic Precedence and Minimal Latency for Amplification Precedence.
  - FR: Choisir entre précédence acoustique et latence minimale pour la précédence d'amplification.
  - [ ] OK    Fix: 

- **`muteButton`**
  - EN: Mute Output {num} for this Object.
  - FR: Couper la sortie {num} pour cet objet.
  - [ ] OK    Fix: 

- **`muteMacrosSelector`**
  - EN: Mute Macros for Fast Muting and Unmuting of Arrays.
  - FR: Macros de mute pour couper et réactiver rapidement les arrays.
  - [ ] OK    Fix: 

- **`muteReverbSendsButton`**
  - EN: Mute sends from this input to all reverb channels.
  - FR: Couper les envois de cette entrée vers tous les canaux de réverbération.
  - [ ] OK    Fix: 

- **`nameEditor`**
  - EN: Displayed Input Channel Name (editable).
  - FR: Nom affiché du canal d'entrée (modifiable).
  - [ ] OK    Fix: 

- **`nudgeHint1`**
  - EN:  Nudge with Left and Right Arrow Keys.
  - FR:  Ajuster avec les touches Gauche et Droite.
  - [ ] OK    Fix: 

- **`nudgeHint2`**
  - EN:  Nudge with Up and Down Arrow Keys.
  - FR:  Ajuster avec les touches Haut et Bas.
  - [ ] OK    Fix: 

- **`nudgeHint3`**
  - EN:  Nudge with Page Up and Page Down Keys.
  - FR:  Ajuster avec Page Haut et Page Bas.
  - [ ] OK    Fix: 

- **`offset1`**
  - EN: Object Offset {name} ({unit}). Adjusted when Tracking is Enabled.
  - FR: Décalage {name} de l'objet ({unit}). Ajusté lorsque le suivi est activé.
  - [ ] OK    Fix: 

- **`offset2`**
  - EN: Object Offset {name} ({unit}). Adjusted when Tracking is Enabled.
  - FR: Décalage {name} de l'objet ({unit}). Ajusté lorsque le suivi est activé.
  - [ ] OK    Fix: 

- **`offset3`**
  - EN: Object Offset {name} ({unit}). Adjusted when Tracking is Enabled.
  - FR: Décalage {name} de l'objet ({unit}). Ajusté lorsque le suivi est activé.
  - [ ] OK    Fix: 

- **`otomoAbsRelButton`**
  - EN: Select Relative or Absolute Coordinates of Displacement.
  - FR: Choisir des coordonnées de déplacement relatives ou absolues.
  - [ ] OK    Fix: 

- **`otomoCoordMode`**
  - EN: Coordinate display mode for AutomOtion destinations: Cartesian (X/Y/Z), Cylindrical (r/θ/Z), or Spherical (r/θ/φ).
  - FR: Mode de coordonnées pour les destinations AutomOtion : Cartésien (X/Y/Z), Cylindrique (r/θ/Z) ou Sphérique (r/θ/φ).
  - [ ] OK    Fix: 

- **`otomoCurveDial`**
  - EN: Bend the Path to the Left (Negative) or Right (Positive) of the Direction of Travel.
  - FR: Courber la trajectoire vers la gauche (négatif) ou la droite (positif) du sens de déplacement.
  - [ ] OK    Fix: 

- **`otomoDest1`**
  - EN: Relative or Absolute Destination {name} ({unit}).
  - FR: Destination relative ou absolue {name} ({unit}).
  - [ ] OK    Fix: 

- **`otomoDest2`**
  - EN: Relative or Absolute Destination {name} ({unit}).
  - FR: Destination relative ou absolue {name} ({unit}).
  - [ ] OK    Fix: 

- **`otomoDest3`**
  - EN: Relative or Absolute Destination {name} ({unit}).
  - FR: Destination relative ou absolue {name} ({unit}).
  - [ ] OK    Fix: 

- **`otomoDurationDial`**
  - EN: Duration of the Movement in Seconds (0.1s to 1 hour).
  - FR: Durée du mouvement en secondes (0,1 s à 1 heure).
  - [ ] OK    Fix: 

- **`otomoPauseButton`**
  - EN: Pause and Resume the Movement.
  - FR: Mettre en pause et reprendre le mouvement.
  - [ ] OK    Fix: 

- **`otomoPauseResumeAllButton`**
  - EN: Pause or Resume All Active Movements Globally.
  - FR: Mettre en pause ou reprendre globalement tous les mouvements actifs.
  - [ ] OK    Fix: 

- **`otomoResetDial`**
  - EN: Set the Reset Level for the Automatic Trigger.
  - FR: Définir le niveau de réinitialisation pour le déclenchement automatique.
  - [ ] OK    Fix: 

- **`otomoSpeedProfileDial`**
  - EN: Constant Speed or Gradual Acceleration and Slow Down at the Start and the End of the Movement.
  - FR: Vitesse constante ou accélération et décélération graduelles au début et à la fin du mouvement.
  - [ ] OK    Fix: 

- **`otomoStartButton`**
  - EN: Start the Movement Manually.
  - FR: Démarrer le mouvement manuellement.
  - [ ] OK    Fix: 

- **`otomoStayReturnButton`**
  - EN: At the End of the Movement, should the Source Stay or Return to the Original Position.
  - FR: À la fin du mouvement, la source reste-t-elle ou revient-elle à la position d'origine.
  - [ ] OK    Fix: 

- **`otomoStopAllButton`**
  - EN: Stop All Active Movements Globally.
  - FR: Arrêter globalement tous les mouvements actifs.
  - [ ] OK    Fix: 

- **`otomoStopButton`**
  - EN: Stop the Movement.
  - FR: Arrêter le mouvement.
  - [ ] OK    Fix: 

- **`otomoThresholdDial`**
  - EN: Set the Threshold for the Automatic Trigger of the Movement.
  - FR: Définir le seuil pour le déclenchement automatique du mouvement.
  - [ ] OK    Fix: 

- **`otomoTriggerButton`**
  - EN: Manual Start of Displacement or Automatic Trigger on the Audio Level.
  - FR: Démarrage manuel du déplacement ou déclenchement automatique sur le niveau audio.
  - [ ] OK    Fix: 

- **`pathModeButton`**
  - EN: Enable Path Mode to Follow Drawn Movement Paths Instead of Direct Lines.
  - FR: Activer le mode Path pour suivre les trajectoires de mouvement dessinées au lieu de lignes directes.
  - [ ] OK    Fix: 

- **`position1`**
  - EN: Object {name} ({unit}).
  - FR: {name} de l'objet ({unit}).
  - [ ] OK    Fix: 

- **`position2`**
  - EN: Object {name} ({unit}).
  - FR: {name} de l'objet ({unit}).
  - [ ] OK    Fix: 

- **`position3`**
  - EN: Object {name} ({unit}).
  - FR: {name} de l'objet ({unit}).
  - [ ] OK    Fix: 

- **`positionJoystick`**
  - EN: Drag to adjust X/Y position in real-time. Returns to center on release.
  - FR: Glisser pour ajuster la position X/Y en temps réel. Revient au centre au relâchement.
  - [ ] OK    Fix: 

- **`positionZSlider`**
  - EN: Drag to adjust Z (height) position in real-time. Returns to center on release.
  - FR: Glisser pour ajuster la position Z (hauteur) en temps réel. Revient au centre au relâchement.
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Input Configuration from backup file.
  - FR: Recharger la configuration d'entrée depuis le fichier de sauvegarde.
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Input Configuration from file.
  - FR: Recharger la configuration d'entrée depuis le fichier.
  - [ ] OK    Fix: 

- **`reloadSnapshot`**
  - EN: Reload Selected Input Snapshot for All Objects Taking the Scope into Account.
  - FR: Recharger le snapshot d'entrée sélectionné pour tous les objets en tenant compte du filtre.
  - [ ] OK    Fix: 

- **`reloadWithoutScope`**
  - EN: Reload Selected Input Snapshot for All Objects Without the Scope.
  - FR: Recharger le snapshot d'entrée sélectionné pour tous les objets sans le filtre.
  - [ ] OK    Fix: 

- **`sidelinesActiveButton`**
  - EN: Enable Automatic Muting when Source Approaches Stage Edges. Does Not Apply to Downstage (Front) Edge.
  - FR: Activer le mute automatique quand la source approche des bords de la scène. Ne s'applique pas au bord avant (côté public).
  - [ ] OK    Fix: 

- **`sidelinesFringeDial`**
  - EN: Fringe Zone Size in Meters. Outer Half is Full Mute, Inner Half Fades Linearly.
  - FR: Taille de la zone de transition en mètres. La moitié externe coupe complètement, la moitié interne atténue linéairement.
  - [ ] OK    Fix: 

- **`snapshotSelector`**
  - EN: Select Input Snapshot Without Loading.
  - FR: Sélectionner un snapshot d'entrée sans le charger.
  - [ ] OK    Fix: 

- **`solo`**
  - EN: Listen to Binaural Rendering of this channel.
  - FR: Écouter le rendu binaural de ce canal.
  - [ ] OK    Fix: 

- **`soloMode`**
  - EN: Single: one input at a time. Multi: multiple inputs simultaneously.
  - FR: Simple : une entrée à la fois. Multi : plusieurs entrées simultanément.
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Input Configuration to file (with backup).
  - FR: Enregistrer la configuration d'entrée dans un fichier (avec sauvegarde).
  - [ ] OK    Fix: 

- **`storeSnapshot`**
  - EN: Store new Input Snapshot for All Objects.
  - FR: Enregistrer un nouveau snapshot d'entrée pour tous les objets.
  - [ ] OK    Fix: 

- **`tiltSlider`**
  - EN: Where is the Object pointing to in the Vertical Plane.
  - FR: Direction de l'objet dans le plan vertical.
  - [ ] OK    Fix: 

- **`trackingActiveButton`**
  - EN: Enable or Disable Tracking for Object.
  - FR: Activer ou désactiver le tracking pour l'objet.
  - [ ] OK    Fix: 

- **`trackingIdSelector`**
  - EN: Tracker ID for Object.
  - FR: ID du tracker pour l'objet.
  - [ ] OK    Fix: 

- **`trackingSmoothDial`**
  - EN: Smoothing of Tracking Data for Object.
  - FR: Lissage des données de tracking pour l'objet.
  - [ ] OK    Fix: 

- **`updateSnapshot`**
  - EN: Update Selected Input Snapshot (with backup).
  - FR: Mettre à jour le snapshot d'entrée sélectionné (avec sauvegarde).
  - [ ] OK    Fix: 

## `inputs.labels`

- **`admMapping`**
  - EN: ADM:
  - FR: ADM:
  - [ ] OK    Fix: 

- **`amplitudeX`**
  - EN: Amplitude X:
  - FR: Amplitude X :
  - [ ] OK    Fix: 

- **`amplitudeY`**
  - EN: Amplitude Y:
  - FR: Amplitude Y :
  - [ ] OK    Fix: 

- **`amplitudeZ`**
  - EN: Amplitude Z:
  - FR: Amplitude Z :
  - [ ] OK    Fix: 

- **`arrayAttenuation`**
  - EN: Array Attenuation:
  - FR: Atténuation réseau :
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Attenuation:
  - FR: Atténuation :
  - [ ] OK    Fix: 

- **`attenuationLaw`**
  - EN: Attenuation Law:
  - FR: Loi d'atténuation :
  - [ ] OK    Fix: 

- **`cluster`**
  - EN: Cluster:
  - FR: Groupe :
  - [ ] OK    Fix: 

- **`commonAtten`**
  - EN: Common Atten:
  - FR: Atténuation commune :
  - [ ] OK    Fix: 

- **`coord`**
  - EN: Coord:
  - FR: Coord :
  - [ ] OK    Fix: 

- **`curve`**
  - EN: Curve:
  - FR: Courbe :
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Delay/Latency:
  - FR: Délai/Latence :
  - [ ] OK    Fix: 

- **`destX`**
  - EN: Dest. X:
  - FR: Dest. X :
  - [ ] OK    Fix: 

- **`destY`**
  - EN: Dest. Y:
  - FR: Dest. Y :
  - [ ] OK    Fix: 

- **`destZ`**
  - EN: Dest. Z:
  - FR: Dest. Z :
  - [ ] OK    Fix: 

- **`diffusion`**
  - EN: Diffusion:
  - FR: Diffusion :
  - [ ] OK    Fix: 

- **`directivity`**
  - EN: Directivity:
  - FR: Directivité :
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Distance Atten:
  - FR: Atténuation distance :
  - [ ] OK    Fix: 

- **`distanceRatio`**
  - EN: Distance Ratio:
  - FR: Ratio distance :
  - [ ] OK    Fix: 

- **`duration`**
  - EN: Duration:
  - FR: Durée :
  - [ ] OK    Fix: 

- **`frequency`**
  - EN: Frequency:
  - FR: Fréquence :
  - [ ] OK    Fix: 

- **`fringe`**
  - EN: Fringe:
  - FR: Bordure :
  - [ ] OK    Fix: 

- **`gain`**
  - EN: Gain:
  - FR: Gain :
  - [ ] OK    Fix: 

- **`gyrophone`**
  - EN: Gyrophone:
  - FR: Gyrophone :
  - [ ] OK    Fix: 

- **`heightFactor`**
  - EN: Height Factor:
  - FR: Facteur de hauteur :
  - [ ] OK    Fix: 

- **`hfShelf`**
  - EN: HF Shelf:
  - FR: Atténuation HF :
  - [ ] OK    Fix: 

- **`jitter`**
  - EN: Jitter:
  - FR: Gigue :
  - [ ] OK    Fix: 

- **`max`**
  - EN: Max:
  - FR: Max :
  - [ ] OK    Fix: 

- **`maxSpeed`**
  - EN: Max Speed:
  - FR: Vitesse Max :
  - [ ] OK    Fix: 

- **`min`**
  - EN: Min:
  - FR: Min :
  - [ ] OK    Fix: 

- **`muteMacros`**
  - EN: Mute Macros:
  - FR: Macros Mute :
  - [ ] OK    Fix: 

- **`name`**
  - EN: Name:
  - FR: Nom :
  - [ ] OK    Fix: 

- **`offsetX`**
  - EN: Offset X:
  - FR: Décalage X :
  - [ ] OK    Fix: 

- **`offsetY`**
  - EN: Offset Y:
  - FR: Décalage Y :
  - [ ] OK    Fix: 

- **`offsetZ`**
  - EN: Offset Z:
  - FR: Décalage Z :
  - [ ] OK    Fix: 

- **`outX`**
  - EN: Out X:
  - FR: Sortie X :
  - [ ] OK    Fix: 

- **`outY`**
  - EN: Out Y:
  - FR: Sortie Y :
  - [ ] OK    Fix: 

- **`outZ`**
  - EN: Out Z:
  - FR: Sortie Z :
  - [ ] OK    Fix: 

- **`peakRatio`**
  - EN: Peak Ratio:
  - FR: Ratio crête :
  - [ ] OK    Fix: 

- **`peakThreshold`**
  - EN: Peak Threshold:
  - FR: Seuil crête :
  - [ ] OK    Fix: 

- **`period`**
  - EN: Period:
  - FR: Période :
  - [ ] OK    Fix: 

- **`phase`**
  - EN: Phase:
  - FR: Phase :
  - [ ] OK    Fix: 

- **`phaseX`**
  - EN: Phase X:
  - FR: Phase X :
  - [ ] OK    Fix: 

- **`phaseY`**
  - EN: Phase Y:
  - FR: Phase Y :
  - [ ] OK    Fix: 

- **`phaseZ`**
  - EN: Phase Z:
  - FR: Phase Z :
  - [ ] OK    Fix: 

- **`positionX`**
  - EN: Position X:
  - FR: Position X :
  - [ ] OK    Fix: 

- **`positionY`**
  - EN: Position Y:
  - FR: Position Y :
  - [ ] OK    Fix: 

- **`positionZ`**
  - EN: Position Z:
  - FR: Position Z :
  - [ ] OK    Fix: 

- **`radius`**
  - EN: Radius:
  - FR: Rayon :
  - [ ] OK    Fix: 

- **`rateX`**
  - EN: Rate X:
  - FR: Taux X :
  - [ ] OK    Fix: 

- **`rateY`**
  - EN: Rate Y:
  - FR: Taux Y :
  - [ ] OK    Fix: 

- **`rateZ`**
  - EN: Rate Z:
  - FR: Taux Z :
  - [ ] OK    Fix: 

- **`reset`**
  - EN: Reset:
  - FR: Réinitialisation :
  - [ ] OK    Fix: 

- **`rotation`**
  - EN: Rotation:
  - FR: Rotation :
  - [ ] OK    Fix: 

- **`shape`**
  - EN: Shape:
  - FR: Forme :
  - [ ] OK    Fix: 

- **`shapeX`**
  - EN: Shape X:
  - FR: Forme X :
  - [ ] OK    Fix: 

- **`shapeY`**
  - EN: Shape Y:
  - FR: Forme Y :
  - [ ] OK    Fix: 

- **`shapeZ`**
  - EN: Shape Z:
  - FR: Forme Z :
  - [ ] OK    Fix: 

- **`slope`**
  - EN: Slope:
  - FR: Pente :
  - [ ] OK    Fix: 

- **`slowRatio`**
  - EN: Slow Ratio:
  - FR: Ratio lent :
  - [ ] OK    Fix: 

- **`slowThreshold`**
  - EN: Slow Threshold:
  - FR: Seuil lent :
  - [ ] OK    Fix: 

- **`speedProfile`**
  - EN: Speed Profile:
  - FR: Profil de vitesse :
  - [ ] OK    Fix: 

- **`threshold`**
  - EN: Threshold:
  - FR: Seuil :
  - [ ] OK    Fix: 

- **`tilt`**
  - EN: Tilt:
  - FR: Inclinaison :
  - [ ] OK    Fix: 

- **`trackingId`**
  - EN: Tracking ID:
  - FR: ID Tracking :
  - [ ] OK    Fix: 

- **`trackingSmooth`**
  - EN: Tracking Smooth:
  - FR: Lissage Tracking :
  - [ ] OK    Fix: 

- **`xyJoystick`**
  - EN: X/Y
  - FR: X/Y
  - [ ] OK    Fix: 

- **`zSlider`**
  - EN: Z
  - FR: Z
  - [ ] OK    Fix: 

## `inputs.lfo`

- **`gyrophone.antiClockwise`**
  - EN: Anti-Clockwise
  - FR: Anti-horaire
  - [ ] OK    Fix: 

- **`gyrophone.clockwise`**
  - EN: Clockwise
  - FR: Horaire
  - [ ] OK    Fix: 

- **`gyrophone.off`**
  - EN: OFF
  - FR: OFF
  - [ ] OK    Fix: 

- **`shapes.exp`**
  - EN: exp
  - FR: exp
  - [ ] OK    Fix: 

- **`shapes.keystone`**
  - EN: keystone
  - FR: trapèze
  - [ ] OK    Fix: 

- **`shapes.log`**
  - EN: log
  - FR: log
  - [ ] OK    Fix: 

- **`shapes.off`**
  - EN: OFF
  - FR: OFF
  - [ ] OK    Fix: 

- **`shapes.random`**
  - EN: random
  - FR: aléatoire
  - [ ] OK    Fix: 

- **`shapes.sawtooth`**
  - EN: sawtooth
  - FR: dent de scie
  - [ ] OK    Fix: 

- **`shapes.sine`**
  - EN: sine
  - FR: sinus
  - [ ] OK    Fix: 

- **`shapes.square`**
  - EN: square
  - FR: carré
  - [ ] OK    Fix: 

- **`shapes.triangle`**
  - EN: triangle
  - FR: triangle
  - [ ] OK    Fix: 

## `inputs.liveSource`

- **`linear`**
  - EN: linear
  - FR: linéaire
  - [ ] OK    Fix: 

- **`log`**
  - EN: log
  - FR: log
  - [ ] OK    Fix: 

- **`sine`**
  - EN: sine
  - FR: sinus
  - [ ] OK    Fix: 

## `inputs.messages`

- **`assignedCluster`**
  - EN: Input {channel} assigned to Cluster {cluster}
  - FR: Entrée {channel} assignée au Groupe {cluster}
  - [ ] OK    Fix: 

- **`backupLoaded`**
  - EN: Input configuration loaded from backup.
  - FR: Configuration entrées chargée depuis la sauvegarde.
  - [ ] OK    Fix: 

- **`configExported`**
  - EN: Input configuration exported.
  - FR: Configuration entrées exportée.
  - [ ] OK    Fix: 

- **`configImported`**
  - EN: Input configuration imported.
  - FR: Configuration entrées importée.
  - [ ] OK    Fix: 

- **`configLoaded`**
  - EN: Input configuration loaded.
  - FR: Configuration entrées chargée.
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Input configuration saved.
  - FR: Configuration entrées enregistrée.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - FR: Erreur : {error}
  - [ ] OK    Fix: 

- **`noSnapshotSelected`**
  - EN: No snapshot selected.
  - FR: Aucun snapshot sélectionné.
  - [ ] OK    Fix: 

- **`scopeConfigured`**
  - EN: Scope configured for next snapshot.
  - FR: Portée configurée pour le prochain snapshot.
  - [ ] OK    Fix: 

- **`scopeSaved`**
  - EN: Snapshot scope saved.
  - FR: Portée du snapshot enregistrée.
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - FR: Veuillez d'abord sélectionner un dossier projet dans Config Système.
  - [ ] OK    Fix: 

- **`setSingle`**
  - EN: Input {channel} set to Single
  - FR: Entrée {channel} définie comme Seul
  - [ ] OK    Fix: 

- **`snapshotDeleted`**
  - EN: Snapshot '{name}' deleted.
  - FR: Snapshot '{name}' supprimé.
  - [ ] OK    Fix: 

- **`snapshotLoaded`**
  - EN: Snapshot '{name}' loaded.
  - FR: Snapshot '{name}' chargé.
  - [ ] OK    Fix: 

- **`snapshotLoadedWithoutScope`**
  - EN: Snapshot '{name}' loaded (without scope).
  - FR: Snapshot '{name}' chargé (sans portée).
  - [ ] OK    Fix: 

- **`snapshotStored`**
  - EN: Snapshot '{name}' stored.
  - FR: Snapshot '{name}' enregistré.
  - [ ] OK    Fix: 

- **`snapshotUpdated`**
  - EN: Snapshot '{name}' updated.
  - FR: Snapshot '{name}' mis à jour.
  - [ ] OK    Fix: 

- **`trackingDisabled`**
  - EN: Tracking disabled for Input {channel}
  - FR: Tracking désactivé pour Entrée {channel}
  - [ ] OK    Fix: 

- **`trackingSwitched`**
  - EN: Tracking switched from Input {from} to Input {to}
  - FR: Tracking transféré de Entrée {from} à Entrée {to}
  - [ ] OK    Fix: 

## `inputs.muteMacros`

- **`invertMutes`**
  - EN: INVERT MUTES
  - FR: INVERSER MUTES
  - [ ] OK    Fix: 

- **`muteAll`**
  - EN: MUTE ALL
  - FR: TOUT MUTER
  - [ ] OK    Fix: 

- **`muteArrayPrefix`**
  - EN: MUTE ARRAY
  - FR: MUTER RÉSEAU
  - [ ] OK    Fix: 

- **`muteEven`**
  - EN: MUTE EVEN
  - FR: MUTER PAIRS
  - [ ] OK    Fix: 

- **`muteOdd`**
  - EN: MUTE ODD
  - FR: MUTER IMPAIRS
  - [ ] OK    Fix: 

- **`selectMacro`**
  - EN: Select Macro...
  - FR: Sélectionner macro...
  - [ ] OK    Fix: 

- **`unmuteAll`**
  - EN: UNMUTE ALL
  - FR: TOUT DÉMUTER
  - [ ] OK    Fix: 

- **`unmuteArrayPrefix`**
  - EN: UNMUTE ARRAY
  - FR: DÉMUTER RÉSEAU
  - [ ] OK    Fix: 

## `inputs.prefixes`

- **`delay`**
  - EN: Delay:
  - FR: Délai :
  - [ ] OK    Fix: 

## `inputs.sections`

- **`automotion`**
  - EN: AutomOtion
  - FR: AutomOtion
  - [ ] OK    Fix: 

## `inputs.snapshots`

- **`selectSnapshot`**
  - EN: Select Snapshot...
  - FR: Sélectionner Snapshot...
  - [ ] OK    Fix: 

## `inputs.tabs`

- **`gradientMaps`**
  - EN: Gradient Maps
  - FR: Cartes de Gradient
  - [ ] OK    Fix: 

- **`inputParams`**
  - EN: Input Parameters
  - FR: Paramètres d'entrée
  - [ ] OK    Fix: 

- **`liveSourceHackoustics`**
  - EN: Live Source & Hackoustics
  - FR: Source Live & Hackoustique
  - [ ] OK    Fix: 

- **`movements`**
  - EN: Movements
  - FR: Mouvements
  - [ ] OK    Fix: 

- **`sampler`**
  - EN: Sampler
  - FR: Sampler
  - [ ] OK    Fix: 

- **`visualisation`**
  - EN: Visualisation
  - FR: Visualisation
  - [ ] OK    Fix: 

## `inputs.toggles`

- **`absolute`**
  - EN: Absolute
  - FR: Absolu
  - [ ] OK    Fix: 

- **`acousticPrecedence`**
  - EN: Acoustic Precedence
  - FR: Précédence acoustique
  - [ ] OK    Fix: 

- **`attenuationLawLog`**
  - EN: Log
  - FR: Log
  - [ ] OK    Fix: 

- **`constraintROff`**
  - EN: Constraint R: OFF
  - FR: Contrainte R : OFF
  - [ ] OK    Fix: 

- **`constraintROn`**
  - EN: Constraint R: ON
  - FR: Contrainte R : ON
  - [ ] OK    Fix: 

- **`constraintXOff`**
  - EN: Constraint X: OFF
  - FR: Contrainte X : OFF
  - [ ] OK    Fix: 

- **`constraintXOn`**
  - EN: Constraint X: ON
  - FR: Contrainte X : ON
  - [ ] OK    Fix: 

- **`constraintYOff`**
  - EN: Constraint Y: OFF
  - FR: Contrainte Y : OFF
  - [ ] OK    Fix: 

- **`constraintYOn`**
  - EN: Constraint Y: ON
  - FR: Contrainte Y : ON
  - [ ] OK    Fix: 

- **`constraintZOff`**
  - EN: Constraint Z: OFF
  - FR: Contrainte Z : OFF
  - [ ] OK    Fix: 

- **`constraintZOn`**
  - EN: Constraint Z: ON
  - FR: Contrainte Z : ON
  - [ ] OK    Fix: 

- **`flipXOff`**
  - EN: Flip X: OFF
  - FR: Inverser X : OFF
  - [ ] OK    Fix: 

- **`flipXOn`**
  - EN: Flip X: ON
  - FR: Inverser X : ON
  - [ ] OK    Fix: 

- **`flipYOff`**
  - EN: Flip Y: OFF
  - FR: Inverser Y : OFF
  - [ ] OK    Fix: 

- **`flipYOn`**
  - EN: Flip Y: ON
  - FR: Inverser Y : ON
  - [ ] OK    Fix: 

- **`flipZOff`**
  - EN: Flip Z: OFF
  - FR: Inverser Z : OFF
  - [ ] OK    Fix: 

- **`flipZOn`**
  - EN: Flip Z: ON
  - FR: Inverser Z : ON
  - [ ] OK    Fix: 

- **`floorReflectionsOff`**
  - EN: Floor Reflections: OFF
  - FR: Réflexions sol : OFF
  - [ ] OK    Fix: 

- **`floorReflectionsOn`**
  - EN: Floor Reflections: ON
  - FR: Réflexions sol : ON
  - [ ] OK    Fix: 

- **`highShelfOff`**
  - EN: High Shelf: OFF
  - FR: Shelving aigu : OFF
  - [ ] OK    Fix: 

- **`highShelfOn`**
  - EN: High Shelf: ON
  - FR: Shelving aigu : ON
  - [ ] OK    Fix: 

- **`lfoOff`**
  - EN: L.F.O: OFF
  - FR: L.F.O : OFF
  - [ ] OK    Fix: 

- **`lfoOn`**
  - EN: L.F.O: ON
  - FR: L.F.O : ON
  - [ ] OK    Fix: 

- **`liveSourceTamerOff`**
  - EN: Live Source Tamer: OFF
  - FR: Dompteur : OFF
  - [ ] OK    Fix: 

- **`liveSourceTamerOn`**
  - EN: Live Source Tamer: ON
  - FR: Dompteur : ON
  - [ ] OK    Fix: 

- **`lowCutOff`**
  - EN: Low Cut: OFF
  - FR: Coupe-bas : OFF
  - [ ] OK    Fix: 

- **`lowCutOn`**
  - EN: Low Cut: ON
  - FR: Coupe-bas : ON
  - [ ] OK    Fix: 

- **`lsPeakOff`**
  - EN: Peak: OFF
  - FR: Crête : OFF
  - [ ] OK    Fix: 

- **`lsPeakOn`**
  - EN: Peak: ON
  - FR: Crête : ON
  - [ ] OK    Fix: 

- **`lsSlowOff`**
  - EN: Slow: OFF
  - FR: Lent : OFF
  - [ ] OK    Fix: 

- **`lsSlowOn`**
  - EN: Slow: ON
  - FR: Lent : ON
  - [ ] OK    Fix: 

- **`manual`**
  - EN: Manual
  - FR: Manuel
  - [ ] OK    Fix: 

- **`maxSpeedOff`**
  - EN: Max Speed: OFF
  - FR: Vitesse max : OFF
  - [ ] OK    Fix: 

- **`maxSpeedOn`**
  - EN: Max Speed: ON
  - FR: Vitesse max : ON
  - [ ] OK    Fix: 

- **`minimalLatency`**
  - EN: Minimal Latency
  - FR: Latence minimale
  - [ ] OK    Fix: 

- **`pathModeOff`**
  - EN: Path Mode: OFF
  - FR: Mode chemin : OFF
  - [ ] OK    Fix: 

- **`pathModeOn`**
  - EN: Path Mode: ON
  - FR: Mode chemin : ON
  - [ ] OK    Fix: 

- **`relative`**
  - EN: Relative
  - FR: Relatif
  - [ ] OK    Fix: 

- **`return`**
  - EN: Return
  - FR: Retourner
  - [ ] OK    Fix: 

- **`reverbSendsMuted`**
  - EN: Sends to Reverbs: Muted
  - FR: Envois Réverb : Bloqués
  - [ ] OK    Fix: 

- **`reverbSendsUnmuted`**
  - EN: Sends to Reverbs: Unmuted
  - FR: Envois Réverb : Débloqués
  - [ ] OK    Fix: 

- **`sidelinesOff`**
  - EN: Sidelines Off
  - FR: Lignes latérales Off
  - [ ] OK    Fix: 

- **`sidelinesOn`**
  - EN: Sidelines On
  - FR: Lignes latérales On
  - [ ] OK    Fix: 

- **`stay`**
  - EN: Stay
  - FR: Rester
  - [ ] OK    Fix: 

- **`trackingOff`**
  - EN: Tracking: OFF
  - FR: Tracking : OFF
  - [ ] OK    Fix: 

- **`trackingOn`**
  - EN: Tracking: ON
  - FR: Tracking : ON
  - [ ] OK    Fix: 

- **`triggered`**
  - EN: Triggered
  - FR: Déclenché
  - [ ] OK    Fix: 

## `inputs.visualisation`

- **`delay`**
  - EN: delay
  - FR: délai
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: HF
damping
  - FR: atténuation
HF
  - [ ] OK    Fix: 

- **`level`**
  - EN: level
  - FR: niveau
  - [ ] OK    Fix: 

## `levelMeter`

- **`inputs`**
  - EN: Inputs
  - FR: Entrées
  - [ ] OK    Fix: 

- **`outputs`**
  - EN: Outputs
  - FR: Sorties
  - [ ] OK    Fix: 

- **`windowTitle`**
  - EN: Level Meters
  - FR: Niveaux
  - [ ] OK    Fix: 

## `levelMeter.buttons`

- **`clearSolo`**
  - EN: Clear Solo
  - FR: Effacer solo
  - [ ] OK    Fix: 

- **`soloModeMulti`**
  - EN: Multi
  - FR: Multi
  - [ ] OK    Fix: 

- **`soloModeSingle`**
  - EN: Single
  - FR: Single
  - [ ] OK    Fix: 

## `levelMeter.tooltips`

- **`clearSolo`**
  - EN: Disengage all solo toggles
  - FR: Désactiver tous les solos
  - [ ] OK    Fix: 

- **`solo`**
  - EN: Display the contribution of the input to all outputs in the Level Meter display (in Single mode) and play binaural render of soloed inputs
  - FR: Afficher la contribution de l'entrée à toutes les sorties dans l'affichage des niveaux (mode Solo unique) et jouer le rendu binaural des entrées en solo
  - [ ] OK    Fix: 

- **`soloMode`**
  - EN: Single: one input at a time. Multi: multiple inputs simultaneously.
  - FR: Simple : une entrée à la fois. Multi : plusieurs entrées simultanément.
  - [ ] OK    Fix: 

## `map`

- **`detachedMessage`**
  - EN: The map is displayed in a separate window.
  - FR: La carte est affichée dans une fenêtre séparée.
  - [ ] OK    Fix: 

- **`reattach`**
  - EN: Re-attach Map
  - FR: Rattacher la carte
  - [ ] OK    Fix: 

## `map.buttons`

- **`fitInputs`**
  - EN: Fit All Inputs to Screen
  - FR: Ajuster toutes les entrées à l'écran
  - [ ] OK    Fix: 

- **`fitStage`**
  - EN: Fit Stage to Screen
  - FR: Ajuster la scène à l'écran
  - [ ] OK    Fix: 

- **`hideLevels`**
  - EN: Hide Levels
  - FR: Masquer les niveaux
  - [ ] OK    Fix: 

- **`showLevels`**
  - EN: Show Levels
  - FR: Afficher les niveaux
  - [ ] OK    Fix: 

## `map.labels`

- **`reverbPrefix`**
  - EN: R
  - FR: R
  - [ ] OK    Fix: 

## `map.messages`

- **`assignedCluster`**
  - EN: Input {channel} assigned to Cluster {cluster}
  - FR: Entrée {channel} assignée au Cluster {cluster}
  - [ ] OK    Fix: 

- **`assignedClusterMulti`**
  - EN: {count} inputs assigned to Cluster {cluster}
  - FR: {count} entrées assignées au Cluster {cluster}
  - [ ] OK    Fix: 

- **`clusterBrokenUp`**
  - EN: Cluster {cluster} broken up
  - FR: Cluster {cluster} dissous
  - [ ] OK    Fix: 

- **`setSingle`**
  - EN: Input {channel} removed from cluster
  - FR: Entrée {channel} retirée du cluster
  - [ ] OK    Fix: 

- **`setSingleMulti`**
  - EN: {count} inputs removed from clusters
  - FR: {count} entrées retirées des clusters
  - [ ] OK    Fix: 

## `map.tooltips`

- **`detach`**
  - EN: Detach the map into a separate window for dual-screen setups
  - FR: Détacher la carte dans une fenêtre séparée pour les configurations double écran
  - [ ] OK    Fix: 

- **`fitInputs`**
  - EN: Adjust the zoom and panning to fit all visible inputs on the display
  - FR: Ajuster le zoom et le panoramique pour afficher toutes les entrées visibles
  - [ ] OK    Fix: 

- **`fitStage`**
  - EN: Adjust the zoom and panning to fit the stage on the display
  - FR: Ajuster le zoom et le panoramique pour afficher la scène
  - [ ] OK    Fix: 

- **`levels`**
  - EN: Display the levels for inputs and outputs on the map
  - FR: Afficher les niveaux des entrées et sorties sur la carte
  - [ ] OK    Fix: 

## `network.buttons`

- **`add`**
  - EN: ADD
  - FR: AJOUTER
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - FR: Exporter
  - [ ] OK    Fix: 

- **`findMyRemote`**
  - EN: Find My Remote
  - FR: Trouver ma télécommande
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - FR: Importer
  - [ ] OK    Fix: 

- **`openLogWindow`**
  - EN: Open Log Window
  - FR: Ouvrir fenêtre journal
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Backup
  - FR: Recharger sauvegarde
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Network Config
  - FR: Recharger config réseau
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Network Config
  - FR: Enregistrer config réseau
  - [ ] OK    Fix: 

## `network.dialogs`

- **`exportConfig`**
  - EN: Export Network Configuration
  - FR: Exporter configuration réseau
  - [ ] OK    Fix: 

- **`findMyRemoteMessage`**
  - EN: Enter the password for your remote device:
  - FR: Entrez le mot de passe de votre télécommande :
  - [ ] OK    Fix: 

- **`findMyRemotePassword`**
  - EN: Password:
  - FR: Mot de passe :
  - [ ] OK    Fix: 

- **`findMyRemoteTitle`**
  - EN: Find My Remote
  - FR: Trouver ma télécommande
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Network Configuration
  - FR: Importer configuration réseau
  - [ ] OK    Fix: 

- **`removeTargetMessage`**
  - EN: Delete target '{name}'?
  - FR: Supprimer la cible « {name} » ?
  - [ ] OK    Fix: 

- **`removeTargetTitle`**
  - EN: Remove Target
  - FR: Supprimer la cible
  - [ ] OK    Fix: 

- **`trackingConflictsContinue`**
  - EN: Continue
  - FR: Continuer
  - [ ] OK    Fix: 

- **`trackingConflictsMessage`**
  - EN: 
Only one tracked input per cluster is allowed. If you continue, tracking will be kept only for the first input in each cluster.
  - FR: 
Un seul tracking actif par cluster est autorisé. Si vous continuez, le tracking ne sera conservé que pour la première entrée de chaque cluster.
  - [ ] OK    Fix: 

- **`trackingConflictsTitle`**
  - EN: Tracking Conflicts Detected
  - FR: Conflits de tracking détectés
  - [ ] OK    Fix: 

## `network.help`

- **`addTarget`**
  - EN: Add new network target.
  - FR: Ajouter une nouvelle cible réseau.
  - [ ] OK    Fix: 

- **`admMapping`**
  - EN: Select an ADM-OSC mapping to configure. Cart = Cartesian (xyz), Polar = spherical (aed).
  - FR: Sélectionner une assignation ADM-OSC à configurer. Cart = Cartésien (xyz), Polar = sphérique (aed).
  - [ ] OK    Fix: 

- **`admMappingPanel`**
  - EN: Drag dots to edit mapping. Click axis title to swap, click Flip to invert. Hold Shift to edit both sides symmetrically.
  - FR: Glisser les points pour modifier le mapping. Cliquer sur le titre d'axe pour permuter, cliquer sur Flip pour inverser. Maintenir Maj pour éditer les deux côtés symétriquement.
  - [ ] OK    Fix: 

- **`currentIP`**
  - EN: IP address of the Processor.
  - FR: Adresse IP du processeur.
  - [ ] OK    Fix: 

- **`dataMode`**
  - EN: Select UDP or TCP data transmission.
  - FR: Sélectionner la transmission de données UDP ou TCP.
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Network Configuration to file.
  - FR: Exporter la configuration réseau dans un fichier.
  - [ ] OK    Fix: 

- **`findMyRemote`**
  - EN: Make your Remote Flash and Buzz to Find it.
  - FR: Faire clignoter et vibrer votre Remote pour le retrouver.
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Network Configuration from file.
  - FR: Importer la configuration réseau depuis un fichier.
  - [ ] OK    Fix: 

- **`networkInterface`**
  - EN: Select the Network Interface.
  - FR: Sélectionner l'interface réseau.
  - [ ] OK    Fix: 

- **`openLogWindow`**
  - EN: Open Network Logging window.
  - FR: Ouvrir la fenêtre de journalisation réseau.
  - [ ] OK    Fix: 

- **`oscQueryEnable`**
  - EN: Enable/disable OSC Query server for automatic parameter discovery via HTTP/WebSocket.
  - FR: Activer/désactiver le serveur OSC Query pour la découverte automatique des paramètres via HTTP/WebSocket.
  - [ ] OK    Fix: 

- **`oscQueryPort`**
  - EN: HTTP port for OSC Query discovery. Other apps can browse parameters at http://localhost:<port>/
  - FR: Port HTTP pour la découverte OSC Query. Les autres applications peuvent parcourir les paramètres à http://localhost:<port>/
  - [ ] OK    Fix: 

- **`oscSourceFilter`**
  - EN: Filter incoming OSC: Accept All sources or only Registered connections with Rx enabled.
  - FR: Filtrer l'OSC entrant : accepter toutes les sources ou uniquement les connexions enregistrées avec Rx activé.
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Select the Protocol: DISABLED, OSC, REMOTE, or ADM-OSC.
  - FR: Sélectionner le protocole : DISABLED, OSC, REMOTE ou ADM-OSC.
  - [ ] OK    Fix: 

- **`psnInterface`**
  - EN: Network interface for PSN multicast reception
  - FR: Interface réseau pour la réception multicast PSN
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Network Configuration from backup file.
  - FR: Recharger la configuration réseau depuis le fichier de sauvegarde.
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Network Configuration from file.
  - FR: Recharger la configuration réseau depuis un fichier.
  - [ ] OK    Fix: 

- **`removeTarget`**
  - EN: Delete this Network Target.
  - FR: Supprimer cette cible réseau.
  - [ ] OK    Fix: 

- **`rxEnable`**
  - EN: Enable or Disable Data Reception.
  - FR: Activer ou désactiver la réception de données.
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Network Configuration to file.
  - FR: Enregistrer la configuration réseau dans un fichier.
  - [ ] OK    Fix: 

- **`targetIP`**
  - EN: IP Address of the Target (use 127.0.0.1 for local host).
  - FR: Adresse IP de la cible (utilisez 127.0.0.1 pour l'hôte local).
  - [ ] OK    Fix: 

- **`targetName`**
  - EN: Network Target Name.
  - FR: Nom de la cible réseau.
  - [ ] OK    Fix: 

- **`tcpPort`**
  - EN: TCP Receive Port of the Processor.
  - FR: Port de réception TCP du processeur.
  - [ ] OK    Fix: 

- **`trackingEnabled`**
  - EN: Enable or Disable Incoming Tracking data processing.
  - FR: Activer ou désactiver le traitement des données de tracking entrantes.
  - [ ] OK    Fix: 

- **`trackingFlipX`**
  - EN: Invert Axis of Tracking X Coordinate.
  - FR: Inverser l'axe de la coordonnée X du tracking.
  - [ ] OK    Fix: 

- **`trackingFlipY`**
  - EN: Invert Axis of Tracking Y Coordinate.
  - FR: Inverser l'axe de la coordonnée Y du tracking.
  - [ ] OK    Fix: 

- **`trackingFlipZ`**
  - EN: Invert Axis of Tracking Z Coordinate.
  - FR: Inverser l'axe de la coordonnée Z du tracking.
  - [ ] OK    Fix: 

- **`trackingOffsetX`**
  - EN: Offset Tracking X Coordinate.
  - FR: Décalage de la coordonnée X du tracking.
  - [ ] OK    Fix: 

- **`trackingOffsetY`**
  - EN: Offset Tracking Y Coordinate.
  - FR: Décalage de la coordonnée Y du tracking.
  - [ ] OK    Fix: 

- **`trackingOffsetZ`**
  - EN: Offset Tracking Z Coordinate.
  - FR: Décalage de la coordonnée Z du tracking.
  - [ ] OK    Fix: 

- **`trackingOscPath`**
  - EN: OSC Path for the Tracking in OSC Mode (starts with a /)
  - FR: Chemin OSC pour le Tracking en mode OSC (commence par /)
  - [ ] OK    Fix: 

- **`trackingPort`**
  - EN: Specify the Port to receive Tracking data.
  - FR: Spécifier le port de réception des données de tracking.
  - [ ] OK    Fix: 

- **`trackingProtocol`**
  - EN: Select the type of Tracking Protocol.
  - FR: Sélectionner le type de protocole de tracking.
  - [ ] OK    Fix: 

- **`trackingScaleX`**
  - EN: Scale Tracking X Coordinate.
  - FR: Échelle de la coordonnée X du tracking.
  - [ ] OK    Fix: 

- **`trackingScaleY`**
  - EN: Scale Tracking Y Coordinate.
  - FR: Échelle de la coordonnée Y du tracking.
  - [ ] OK    Fix: 

- **`trackingScaleZ`**
  - EN: Scale Tracking Z Coordinate.
  - FR: Échelle de la coordonnée Z du tracking.
  - [ ] OK    Fix: 

- **`txEnable`**
  - EN: Enable or Disable Data Transmission.
  - FR: Activer ou désactiver la transmission de données.
  - [ ] OK    Fix: 

- **`txPort`**
  - EN: Transmit Port for this Target.
  - FR: Port de transmission pour cette cible.
  - [ ] OK    Fix: 

- **`udpPort`**
  - EN: UDP Receive Port of the Processor.
  - FR: Port de réception UDP du processeur.
  - [ ] OK    Fix: 

## `network.labels`

- **`admMapping`**
  - EN: Mapping:
  - FR: Assignation :
  - [ ] OK    Fix: 

- **`currentIPv4`**
  - EN: Current IPv4:
  - FR: IPv4 actuelle :
  - [ ] OK    Fix: 

- **`interface`**
  - EN: Network Interface:
  - FR: Interface réseau :
  - [ ] OK    Fix: 

- **`localhost`**
  - EN: Localhost (127.0.0.1)
  - FR: Localhost (127.0.0.1)
  - [ ] OK    Fix: 

- **`mqttHost`**
  - EN: Host:
  - FR: Hôte :
  - [ ] OK    Fix: 

- **`mqttJsonQ`**
  - EN: Q:
  - FR: Q :
  - [ ] OK    Fix: 

- **`mqttJsonX`**
  - EN: X:
  - FR: X :
  - [ ] OK    Fix: 

- **`mqttJsonY`**
  - EN: Y:
  - FR: Y :
  - [ ] OK    Fix: 

- **`mqttJsonZ`**
  - EN: Z:
  - FR: Z :
  - [ ] OK    Fix: 

- **`mqttTagIds`**
  - EN: Tag IDs...
  - FR: IDs Tag...
  - [ ] OK    Fix: 

- **`mqttTopic`**
  - EN: Topic:
  - FR: Sujet :
  - [ ] OK    Fix: 

- **`notAvailable`**
  - EN: Not available
  - FR: Non disponible
  - [ ] OK    Fix: 

- **`offsetX`**
  - EN: Offset X:
  - FR: Décalage X :
  - [ ] OK    Fix: 

- **`offsetY`**
  - EN: Offset Y:
  - FR: Décalage Y :
  - [ ] OK    Fix: 

- **`offsetZ`**
  - EN: Offset Z:
  - FR: Décalage Z :
  - [ ] OK    Fix: 

- **`oscPath`**
  - EN: OSC Path:
  - FR: Chemin OSC :
  - [ ] OK    Fix: 

- **`oscQuery`**
  - EN: OSC Query:
  - FR: OSC Query :
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Protocol:
  - FR: Protocole :
  - [ ] OK    Fix: 

- **`psnInterface`**
  - EN: PSN Interface:
  - FR: Interface PSN :
  - [ ] OK    Fix: 

- **`rxPort`**
  - EN: Rx Port:
  - FR: Port Rx :
  - [ ] OK    Fix: 

- **`scaleX`**
  - EN: Scale X:
  - FR: Échelle X :
  - [ ] OK    Fix: 

- **`scaleY`**
  - EN: Scale Y:
  - FR: Échelle Y :
  - [ ] OK    Fix: 

- **`scaleZ`**
  - EN: Scale Z:
  - FR: Échelle Z :
  - [ ] OK    Fix: 

- **`tcpPort`**
  - EN: TCP Port:
  - FR: Port TCP :
  - [ ] OK    Fix: 

- **`udpPort`**
  - EN: UDP Port:
  - FR: Port UDP :
  - [ ] OK    Fix: 

## `network.messages`

- **`configExported`**
  - EN: Network configuration exported.
  - FR: Configuration réseau exportée.
  - [ ] OK    Fix: 

- **`configImported`**
  - EN: Network configuration imported.
  - FR: Configuration réseau importée.
  - [ ] OK    Fix: 

- **`configLoadedFromBackup`**
  - EN: Network configuration loaded from backup.
  - FR: Configuration réseau chargée depuis la sauvegarde.
  - [ ] OK    Fix: 

- **`configNotFound`**
  - EN: Network config file not found.
  - FR: Fichier de configuration réseau introuvable.
  - [ ] OK    Fix: 

- **`configReloaded`**
  - EN: Network configuration reloaded.
  - FR: Configuration réseau rechargée.
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Network configuration saved.
  - FR: Configuration réseau enregistrée.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - FR: Erreur : {error}
  - [ ] OK    Fix: 

- **`findDeviceSent`**
  - EN: Find Device command sent.
  - FR: Commande Trouver l'appareil envoyée.
  - [ ] OK    Fix: 

- **`maxTargetsReached`**
  - EN: Maximum Number of Targets/Servers Reached.
  - FR: Nombre maximum de cibles/serveurs atteint.
  - [ ] OK    Fix: 

- **`noBackupFound`**
  - EN: No backup files found.
  - FR: Aucun fichier de sauvegarde trouvé.
  - [ ] OK    Fix: 

- **`onlyOneRemote`**
  - EN: Only one REMOTE connection is allowed.
  - FR: Une seule connexion TÉLÉCOMMANDE est autorisée.
  - [ ] OK    Fix: 

- **`oscManagerError`**
  - EN: Error: OSC Manager not available
  - FR: Erreur : Gestionnaire OSC non disponible
  - [ ] OK    Fix: 

- **`passwordEmpty`**
  - EN: Password cannot be empty.
  - FR: Le mot de passe ne peut pas être vide.
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - FR: Veuillez d'abord sélectionner un dossier projet dans Config Système.
  - [ ] OK    Fix: 

## `network.protocols`

- **`admOsc`**
  - EN: ADM-OSC
  - FR: ADM-OSC
  - [ ] OK    Fix: 

- **`disabled`**
  - EN: DISABLED
  - FR: DÉSACTIVÉ
  - [ ] OK    Fix: 

- **`mqtt`**
  - EN: MQTT
  - FR: MQTT
  - [ ] OK    Fix: 

- **`osc`**
  - EN: OSC
  - FR: OSC
  - [ ] OK    Fix: 

- **`psn`**
  - EN: PosiStageNet (PSN)
  - FR: PosiStageNet (PSN)
  - [ ] OK    Fix: 

- **`qlab`**
  - EN: QLab
  - FR: QLab
  - [ ] OK    Fix: 

- **`remote`**
  - EN: Remote
  - FR: Télécommande
  - [ ] OK    Fix: 

- **`rttrp`**
  - EN: RTTrP
  - FR: RTTrP
  - [ ] OK    Fix: 

- **`tcp`**
  - EN: TCP
  - FR: TCP
  - [ ] OK    Fix: 

- **`udp`**
  - EN: UDP
  - FR: UDP
  - [ ] OK    Fix: 

## `network.sections`

- **`admOsc`**
  - EN: ADM-OSC Mappings
  - FR: Assignations ADM-OSC
  - [ ] OK    Fix: 

- **`connections`**
  - EN: Network Connections
  - FR: Connexions réseau
  - [ ] OK    Fix: 

- **`network`**
  - EN: Network
  - FR: Réseau
  - [ ] OK    Fix: 

- **`tracking`**
  - EN: Tracking
  - FR: Suivi
  - [ ] OK    Fix: 

## `network.table`

- **`defaultTarget`**
  - EN: Target {num}
  - FR: Cible {num}
  - [ ] OK    Fix: 

- **`ipv4Address`**
  - EN: IPv4 Address
  - FR: Adresse IPv4
  - [ ] OK    Fix: 

- **`mode`**
  - EN: Mode
  - FR: Mode
  - [ ] OK    Fix: 

- **`name`**
  - EN: Name
  - FR: Nom
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Protocol
  - FR: Protocole
  - [ ] OK    Fix: 

- **`rx`**
  - EN: Rx
  - FR: Rx
  - [ ] OK    Fix: 

- **`tx`**
  - EN: Tx
  - FR: Tx
  - [ ] OK    Fix: 

- **`txPort`**
  - EN: Tx Port
  - FR: Port Tx
  - [ ] OK    Fix: 

## `network.toggles`

- **`disabled`**
  - EN: Disabled
  - FR: Désactivé
  - [ ] OK    Fix: 

- **`enabled`**
  - EN: Enabled
  - FR: Activé
  - [ ] OK    Fix: 

- **`flipXOff`**
  - EN: Flip X: OFF
  - FR: Inverser X : OFF
  - [ ] OK    Fix: 

- **`flipXOn`**
  - EN: Flip X: ON
  - FR: Inverser X : ON
  - [ ] OK    Fix: 

- **`flipYOff`**
  - EN: Flip Y: OFF
  - FR: Inverser Y : OFF
  - [ ] OK    Fix: 

- **`flipYOn`**
  - EN: Flip Y: ON
  - FR: Inverser Y : ON
  - [ ] OK    Fix: 

- **`flipZOff`**
  - EN: Flip Z: OFF
  - FR: Inverser Z : OFF
  - [ ] OK    Fix: 

- **`flipZOn`**
  - EN: Flip Z: ON
  - FR: Inverser Z : ON
  - [ ] OK    Fix: 

- **`off`**
  - EN: OFF
  - FR: OFF
  - [ ] OK    Fix: 

- **`on`**
  - EN: ON
  - FR: ON
  - [ ] OK    Fix: 

- **`oscFilterAcceptAll`**
  - EN: OSC Filter: Accept All
  - FR: Filtre OSC : Accepter tout
  - [ ] OK    Fix: 

- **`oscFilterRegisteredOnly`**
  - EN: OSC Filter: Registered Only
  - FR: Filtre OSC : Enregistrés uniquement
  - [ ] OK    Fix: 

- **`trackingOff`**
  - EN: Tracking: OFF
  - FR: Suivi : OFF
  - [ ] OK    Fix: 

- **`trackingOn`**
  - EN: Tracking: ON
  - FR: Suivi : ON
  - [ ] OK    Fix: 

## `networkLog`

- **`windowTitle`**
  - EN: Network Log
  - FR: Journal réseau
  - [ ] OK    Fix: 

## `networkLog.columns`

- **`address`**
  - EN: Address
  - FR: Adresse
  - [ ] OK    Fix: 

- **`arguments`**
  - EN: Arguments
  - FR: Arguments
  - [ ] OK    Fix: 

- **`direction`**
  - EN: Dir
  - FR: Dir
  - [ ] OK    Fix: 

- **`ip`**
  - EN: IP
  - FR: IP
  - [ ] OK    Fix: 

- **`origin`**
  - EN: Origin
  - FR: Origine
  - [ ] OK    Fix: 

- **`port`**
  - EN: Port
  - FR: Port
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Protocol
  - FR: Protocole
  - [ ] OK    Fix: 

- **`time`**
  - EN: Time
  - FR: Heure
  - [ ] OK    Fix: 

- **`transport`**
  - EN: Trans
  - FR: Trans
  - [ ] OK    Fix: 

## `networkLog.controls`

- **`clear`**
  - EN: CLEAR
  - FR: EFFACER
  - [ ] OK    Fix: 

- **`export`**
  - EN: EXPORT
  - FR: EXPORTER
  - [ ] OK    Fix: 

- **`hideHeartbeat`**
  - EN: Hide Heartbeat
  - FR: Masquer Heartbeat
  - [ ] OK    Fix: 

- **`logging`**
  - EN: Logging
  - FR: Journalisation
  - [ ] OK    Fix: 

## `networkLog.dialogs`

- **`exportCompleteMessage`**
  - EN: Log exported to: {path}
  - FR: Journal exporté vers : {path}
  - [ ] OK    Fix: 

- **`exportCompleteTitle`**
  - EN: Export Complete
  - FR: Exportation terminée
  - [ ] OK    Fix: 

- **`exportFailedMessage`**
  - EN: Could not write to file: {path}
  - FR: Impossible d'écrire dans le fichier : {path}
  - [ ] OK    Fix: 

- **`exportFailedTitle`**
  - EN: Export Failed
  - FR: Échec de l'exportation
  - [ ] OK    Fix: 

## `networkLog.exportMenu`

- **`exportAll`**
  - EN: Export All
  - FR: Exporter tout
  - [ ] OK    Fix: 

- **`exportFiltered`**
  - EN: Export Filtered
  - FR: Exporter filtré
  - [ ] OK    Fix: 

## `networkLog.filterModes`

- **`clientIp`**
  - EN: Client IP
  - FR: IP client
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Protocol
  - FR: Protocole
  - [ ] OK    Fix: 

- **`rejected`**
  - EN: Rejected
  - FR: Rejeté
  - [ ] OK    Fix: 

- **`tcpUdp`**
  - EN: TCP/UDP
  - FR: TCP/UDP
  - [ ] OK    Fix: 

## `networkLog.filterToggles`

- **`incoming`**
  - EN: Incoming
  - FR: Entrant
  - [ ] OK    Fix: 

- **`outgoing`**
  - EN: Outgoing
  - FR: Sortant
  - [ ] OK    Fix: 

- **`tcp`**
  - EN: TCP
  - FR: TCP
  - [ ] OK    Fix: 

- **`udp`**
  - EN: UDP
  - FR: UDP
  - [ ] OK    Fix: 

## `networkLog.status`

- **`rejected`**
  - EN: REJECTED
  - FR: REJETÉ
  - [ ] OK    Fix: 

## `outputs.arrayModes`

- **`absolute`**
  - EN: ABSOLUTE
  - FR: ABSOLU
  - [ ] OK    Fix: 

- **`array`**
  - EN: Array
  - FR: Groupe
  - [ ] OK    Fix: 

- **`off`**
  - EN: OFF
  - FR: DÉSACTIVÉ
  - [ ] OK    Fix: 

- **`relative`**
  - EN: RELATIVE
  - FR: RELATIF
  - [ ] OK    Fix: 

- **`single`**
  - EN: Single
  - FR: Individuel
  - [ ] OK    Fix: 

## `outputs.buttons`

- **`arrayHidden`**
  - EN: Array Hidden on Map
  - FR: Groupe masqué sur la carte
  - [ ] OK    Fix: 

- **`arrayVisible`**
  - EN: Array Visible on Map
  - FR: Groupe visible sur la carte
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - FR: Exporter
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - FR: Importer
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Backup
  - FR: Recharger sauvegarde
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Output Config
  - FR: Recharger config sorties
  - [ ] OK    Fix: 

- **`speakerHidden`**
  - EN: Speaker Hidden on Map
  - FR: Enceinte masquée sur la carte
  - [ ] OK    Fix: 

- **`speakerVisible`**
  - EN: Speaker Visible on Map
  - FR: Enceinte visible sur la carte
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Output Config
  - FR: Enregistrer config sorties
  - [ ] OK    Fix: 

- **`wizardOfOutZ`**
  - EN: Wizard of OutZ...
  - FR: Assistant OutZ...
  - [ ] OK    Fix: 

## `outputs.coordModes`

- **`xyz`**
  - EN: XYZ
  - FR: XYZ
  - [ ] OK    Fix: 

## `outputs.dialogs`

- **`export`**
  - EN: Export Output Configuration
  - FR: Exporter configuration des sorties
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import Output Configuration
  - FR: Importer configuration des sorties
  - [ ] OK    Fix: 

## `outputs.help`

- **`angleOff`**
  - EN: Output Channel Will Not Amplify Objects in this Angle in Front of it. (changes may affect the rest of the array)
  - FR: Le canal de sortie n'amplifiera pas les objets dans cet angle devant lui. (les modifications peuvent affecter le reste du groupe)
  - [ ] OK    Fix: 

- **`angleOn`**
  - EN: Output Channel Will Amplify Objects in this Angle in its Back. (changes may affect the rest of the array)
  - FR: Le canal de sortie amplifiera les objets dans cet angle derrière lui. (les modifications peuvent affecter le reste du groupe)
  - [ ] OK    Fix: 

- **`applyToArray`**
  - EN: Apply Changes to the rest of the Array (Absolute value or Relative changes).
  - FR: Appliquer les modifications au reste du groupe (valeur absolue ou modifications relatives).
  - [ ] OK    Fix: 

- **`arraySelector`**
  - EN: Selected Output Channel is part of Array.
  - FR: Le canal de sortie sélectionné fait partie d'un groupe.
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Output Channel Attenuation. (changes may affect the rest of the array)
  - FR: Atténuation du canal de sortie. (les modifications peuvent affecter le reste du groupe)
  - [ ] OK    Fix: 

- **`channelSelector`**
  - EN: Output Channel Number and Selection.
  - FR: Numéro et sélection du canal de sortie.
  - [ ] OK    Fix: 

- **`coordMode`**
  - EN: Coordinate display mode: Cartesian (X/Y/Z), Cylindrical (radius/azimuth/height), or Spherical (radius/azimuth/elevation).
  - FR: Mode d'affichage des coordonnées : cartésien (X/Y/Z), cylindrique (rayon/azimut/hauteur) ou sphérique (rayon/azimut/élévation).
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Output Channel Delay (positive values) or Latency Compensation (negative values). (changes may affect the rest of the array)
  - FR: Délai du canal de sortie (valeurs positives) ou compensation de latence (valeurs négatives). (les modifications peuvent affecter le reste du groupe)
  - [ ] OK    Fix: 

- **`directional`**
  - EN: Output Channel Directional Control. Drag to change orientation, Shift+drag for Angle Off, Alt+drag for Angle On. (changes may affect the rest of the array)
  - FR: Contrôle directionnel du canal de sortie. Glisser pour changer l'orientation, Maj+glisser pour l'angle Off, Alt+glisser pour l'angle On. (les modifications peuvent affecter le reste du groupe)
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Ratio of Distance Attenuation for Selected Output. (changes may affect the rest of the array)
  - FR: Ratio d'atténuation par la distance pour la sortie sélectionnée. (les modifications peuvent affecter le reste du groupe)
  - [ ] OK    Fix: 

- **`eqBandToggle`**
  - EN: Toggle Band {band} on/off. When off the band is bypassed.
  - FR: Activer/désactiver la bande {band}. Désactivée, la bande est contournée.
  - [ ] OK    Fix: 

- **`eqEnable`**
  - EN: Enable or disable EQ processing for this output.
  - FR: Activer ou désactiver l'égalisation pour cette sortie.
  - [ ] OK    Fix: 

- **`eqFlatten`**
  - EN: Long press to reset all EQ bands to their default values.
  - FR: Appui long pour réinitialiser toutes les bandes EQ.
  - [ ] OK    Fix: 

- **`eqFreq`**
  - EN: Output EQ Band {band} frequency (20 Hz - 20 kHz).
  - FR: Fréquence EQ sortie bande {band} (20 Hz - 20 kHz).
  - [ ] OK    Fix: 

- **`eqGain`**
  - EN: Output EQ Band {band} gain (-24 to +24 dB).
  - FR: Gain EQ sortie bande {band} (-24 à +24 dB).
  - [ ] OK    Fix: 

- **`eqQ`**
  - EN: Output EQ Band {band} Q factor (0.1 - 10.0).
  - FR: Facteur Q EQ sortie bande {band} (0.1 - 10.0).
  - [ ] OK    Fix: 

- **`eqResetBand`**
  - EN: Long press to reset Band {band} to its default values.
  - FR: Appui long pour réinitialiser la bande {band}.
  - [ ] OK    Fix: 

- **`eqShape`**
  - EN: Output EQ Band {band} filter shape.
  - FR: Forme du filtre EQ sortie bande {band}.
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Output Configuration to file (with file explorer window).
  - FR: Exporter la configuration des sorties vers un fichier (avec fenêtre de l'explorateur).
  - [ ] OK    Fix: 

- **`floorReflections`**
  - EN: Enable or Disable the Floor Reflections for this Speaker.
  - FR: Activer ou désactiver les réflexions du sol pour cette enceinte.
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: Loss of High Frequency Depending on Distance from Object to Output. (changes may affect the rest of the array)
  - FR: Perte de hautes fréquences en fonction de la distance de l'objet à la sortie. (les modifications peuvent affecter le reste du groupe)
  - [ ] OK    Fix: 

- **`hParallax`**
  - EN: Horizontal Distance from Speaker to 'Targeted' Listener. (changes may affect the rest of the array)
  - FR: Distance horizontale de l'enceinte à l'auditeur « ciblé ». (les modifications peuvent affecter le reste du groupe)
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Output Configuration from file (with file explorer window).
  - FR: Importer la configuration des sorties depuis un fichier (avec fenêtre de l'explorateur).
  - [ ] OK    Fix: 

- **`liveSource`**
  - EN: Disables Live Source Attenuation for Selected Output. (changes may affect the rest of the array)
  - FR: Désactive l'atténuation de source live pour la sortie sélectionnée. (les modifications peuvent affecter le reste du groupe)
  - [ ] OK    Fix: 

- **`mapVisibility`**
  - EN: Make Visible or Hide The Selected Output on the Map.
  - FR: Rendre visible ou masquer la sortie sélectionnée sur la carte.
  - [ ] OK    Fix: 

- **`minLatency`**
  - EN: Disables Minimal Latency Mode for Selected Output. (changes may affect the rest of the array)
  - FR: Désactive le mode latence minimale pour la sortie sélectionnée. (les modifications peuvent affecter le reste du groupe)
  - [ ] OK    Fix: 

- **`nameEditor`**
  - EN: Displayed Output Channel Name (editable).
  - FR: Nom affiché du canal de sortie (modifiable).
  - [ ] OK    Fix: 

- **`pitch`**
  - EN: Output Channel Vertical Orientation used to Determine which Objects get amplified. (changes may affect the rest of the array)
  - FR: Orientation verticale du canal de sortie utilisée pour déterminer quels objets sont amplifiés. (les modifications peuvent affecter le reste du groupe)
  - [ ] OK    Fix: 

- **`position1`**
  - EN: Output Channel {name} ({unit}).
  - FR: Canal de sortie {name} ({unit}).
  - [ ] OK    Fix: 

- **`position2`**
  - EN: Output Channel {name} ({unit}).
  - FR: Canal de sortie {name} ({unit}).
  - [ ] OK    Fix: 

- **`position3`**
  - EN: Output Channel {name} ({unit}).
  - FR: Canal de sortie {name} ({unit}).
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Output Configuration from backup file.
  - FR: Recharger la configuration des sorties depuis le fichier de sauvegarde.
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Output Configuration from file.
  - FR: Recharger la configuration des sorties depuis un fichier.
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Output Configuration to file (with backup).
  - FR: Enregistrer la configuration des sorties dans un fichier (avec sauvegarde).
  - [ ] OK    Fix: 

- **`vParallax`**
  - EN: Vertical Distance from Speaker to 'Targeted' Listener. Positive when the Speaker is Below the head of the Listener. (changes may affect the rest of the array)
  - FR: Distance verticale de l'enceinte à l'auditeur « ciblé ». Positif lorsque l'enceinte est en dessous de la tête de l'auditeur. (les modifications peuvent affecter le reste du groupe)
  - [ ] OK    Fix: 

- **`wizardOfOutZ`**
  - EN: Open Wizard of OutZ to Position Speaker Arrays Conveniently.
  - FR: Ouvrir le Wizard of OutZ pour positionner les groupes d'enceintes facilement.
  - [ ] OK    Fix: 

## `outputs.labels`

- **`angleOff`**
  - EN: Angle Off:
  - FR: Angle Off :
  - [ ] OK    Fix: 

- **`angleOn`**
  - EN: Angle On:
  - FR: Angle On :
  - [ ] OK    Fix: 

- **`applyToArray`**
  - EN: Apply to Array:
  - FR: Appliquer au groupe :
  - [ ] OK    Fix: 

- **`array`**
  - EN: Array:
  - FR: Groupe :
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Attenuation:
  - FR: Atténuation :
  - [ ] OK    Fix: 

- **`coordinates`**
  - EN: Coordinates:
  - FR: Coordonnées :
  - [ ] OK    Fix: 

- **`delay`**
  - EN: Delay:
  - FR: Délai :
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Delay/Latency:
  - FR: Délai/Latence :
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Distance Atten:
  - FR: Atténuation distance :
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: HF Damping:
  - FR: Atténuation HF :
  - [ ] OK    Fix: 

- **`hParallax`**
  - EN: Horizontal Parallax:
  - FR: Parallaxe horizontal :
  - [ ] OK    Fix: 

- **`latency`**
  - EN: Latency:
  - FR: Latence :
  - [ ] OK    Fix: 

- **`name`**
  - EN: Name:
  - FR: Nom :
  - [ ] OK    Fix: 

- **`orientation`**
  - EN: Orientation:
  - FR: Orientation :
  - [ ] OK    Fix: 

- **`pitch`**
  - EN: Pitch:
  - FR: Inclinaison :
  - [ ] OK    Fix: 

- **`positionX`**
  - EN: Position X:
  - FR: Position X :
  - [ ] OK    Fix: 

- **`positionY`**
  - EN: Position Y:
  - FR: Position Y :
  - [ ] OK    Fix: 

- **`positionZ`**
  - EN: Position Z:
  - FR: Position Z :
  - [ ] OK    Fix: 

- **`vParallax`**
  - EN: Vertical Parallax:
  - FR: Parallaxe vertical :
  - [ ] OK    Fix: 

## `outputs.messages`

- **`assignedToArray`**
  - EN: Output {num} assigned to Array {array}
  - FR: Sortie {num} assignée au Groupe {array}
  - [ ] OK    Fix: 

- **`backupLoaded`**
  - EN: Output configuration loaded from backup.
  - FR: Configuration des sorties chargée depuis la sauvegarde.
  - [ ] OK    Fix: 

- **`configExported`**
  - EN: Output configuration exported.
  - FR: Configuration des sorties exportée.
  - [ ] OK    Fix: 

- **`configImported`**
  - EN: Output configuration imported.
  - FR: Configuration des sorties importée.
  - [ ] OK    Fix: 

- **`configLoaded`**
  - EN: Output configuration loaded.
  - FR: Configuration des sorties chargée.
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Output configuration saved.
  - FR: Configuration des sorties enregistrée.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - FR: Erreur : {error}
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - FR: Veuillez d'abord sélectionner un dossier projet dans Config Système.
  - [ ] OK    Fix: 

- **`setToSingle`**
  - EN: Output {num} set to Single
  - FR: Sortie {num} définie en Individuel
  - [ ] OK    Fix: 

## `outputs.tabs`

- **`eq`**
  - EN: Output EQ
  - FR: EQ de sortie
  - [ ] OK    Fix: 

- **`parameters`**
  - EN: Output Parameters
  - FR: Paramètres de sortie
  - [ ] OK    Fix: 

## `outputs.toggles`

- **`floorReflectionsOff`**
  - EN: Floor Reflections: OFF
  - FR: Réflexions sol : NON
  - [ ] OK    Fix: 

- **`floorReflectionsOn`**
  - EN: Floor Reflections: ON
  - FR: Réflexions sol : OUI
  - [ ] OK    Fix: 

- **`liveSourceOff`**
  - EN: Live Source Atten: OFF
  - FR: Atténuation live : NON
  - [ ] OK    Fix: 

- **`liveSourceOn`**
  - EN: Live Source Atten: ON
  - FR: Atténuation live : OUI
  - [ ] OK    Fix: 

- **`minLatencyOff`**
  - EN: Minimal Latency: OFF
  - FR: Latence min : NON
  - [ ] OK    Fix: 

- **`minLatencyOn`**
  - EN: Minimal Latency: ON
  - FR: Latence min : OUI
  - [ ] OK    Fix: 

## `outputs.units`

- **`meters`**
  - EN: m
  - FR: m
  - [ ] OK    Fix: 

- **`ms`**
  - EN: ms
  - FR: ms
  - [ ] OK    Fix: 

## `reverbs`

- **`noChannels`**
  - EN: No reverb channels configured.

Set the number of Reverb Channels in System Config.
  - FR: Aucun canal de réverbération configuré.

Définissez le nombre de canaux de réverbération dans Config Système.
  - [ ] OK    Fix: 

## `reverbs.algorithm`

- **`crossoverHigh`**
  - EN: Crossover High:
  - FR: Croisement haut :
  - [ ] OK    Fix: 

- **`crossoverLow`**
  - EN: Crossover Low:
  - FR: Croisement bas :
  - [ ] OK    Fix: 

- **`decaySection`**
  - EN: Decay
  - FR: Déclin
  - [ ] OK    Fix: 

- **`diffusion`**
  - EN: Diffusion:
  - FR: Diffusion :
  - [ ] OK    Fix: 

- **`fdn`**
  - EN: FDN
  - FR: FDN
  - [ ] OK    Fix: 

- **`fdnSection`**
  - EN: FDN
  - FR: FDN
  - [ ] OK    Fix: 

- **`ir`**
  - EN: IR
  - FR: IR
  - [ ] OK    Fix: 

- **`irFile`**
  - EN: IR File:
  - FR: Fichier IR :
  - [ ] OK    Fix: 

- **`irImport`**
  - EN: Import IR...
  - FR: Importer IR...
  - [ ] OK    Fix: 

- **`irImportSuccess`**
  - EN: IR imported: {file}
  - FR: IR importé : {file}
  - [ ] OK    Fix: 

- **`irLength`**
  - EN: IR Length:
  - FR: Longueur IR :
  - [ ] OK    Fix: 

- **`irNoProject`**
  - EN: Set a project folder first
  - FR: Définir d'abord un dossier de projet
  - [ ] OK    Fix: 

- **`irSection`**
  - EN: IR
  - FR: IR
  - [ ] OK    Fix: 

- **`irTrim`**
  - EN: IR Trim:
  - FR: Recadrage IR :
  - [ ] OK    Fix: 

- **`noFileLoaded`**
  - EN: No IR loaded
  - FR: Aucun IR chargé
  - [ ] OK    Fix: 

- **`perNodeOff`**
  - EN: Per-node IR OFF
  - FR: IR par nœud OFF
  - [ ] OK    Fix: 

- **`perNodeOn`**
  - EN: Per-node IR ON
  - FR: IR par nœud ON
  - [ ] OK    Fix: 

- **`rt60`**
  - EN: RT60:
  - FR: RT60 :
  - [ ] OK    Fix: 

- **`rt60HighMult`**
  - EN: RT60 High ×:
  - FR: RT60 Haut × :
  - [ ] OK    Fix: 

- **`rt60LowMult`**
  - EN: RT60 Low ×:
  - FR: RT60 Bas × :
  - [ ] OK    Fix: 

- **`scale`**
  - EN: Scale:
  - FR: Échelle :
  - [ ] OK    Fix: 

- **`sdn`**
  - EN: SDN
  - FR: SDN
  - [ ] OK    Fix: 

- **`sdnSection`**
  - EN: SDN
  - FR: SDN
  - [ ] OK    Fix: 

- **`size`**
  - EN: Size:
  - FR: Taille :
  - [ ] OK    Fix: 

- **`wetLevel`**
  - EN: Wet Level:
  - FR: Niveau Wet :
  - [ ] OK    Fix: 

## `reverbs.buttons`

- **`editOnMap`**
  - EN: Edit on Map
  - FR: Modifier sur la carte
  - [ ] OK    Fix: 

- **`editOnMapOn`**
  - EN: Edit on Map ON
  - FR: Modifier sur la carte ON
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - FR: Exporter
  - [ ] OK    Fix: 

- **`hiddenOnMap`**
  - EN: Reverbs Hidden on Map
  - FR: Réverbs masquées sur la carte
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - FR: Importer
  - [ ] OK    Fix: 

- **`mutePost`**
  - EN: Mute Post
  - FR: Mute Post
  - [ ] OK    Fix: 

- **`mutePostOn`**
  - EN: Mute Post ON
  - FR: Mute Post ON
  - [ ] OK    Fix: 

- **`mutePre`**
  - EN: Mute Pre
  - FR: Mute Pré
  - [ ] OK    Fix: 

- **`mutePreOn`**
  - EN: Mute Pre ON
  - FR: Mute Pré ON
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Backup
  - FR: Recharger sauvegarde
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Reverb Config
  - FR: Recharger config réverb
  - [ ] OK    Fix: 

- **`soloReverbs`**
  - EN: Solo Reverbs
  - FR: Solo Reverbs
  - [ ] OK    Fix: 

- **`soloReverbsOn`**
  - EN: Solo Reverbs ON
  - FR: Solo réverbs ON
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Reverb Config
  - FR: Enregistrer config réverb
  - [ ] OK    Fix: 

- **`visibleOnMap`**
  - EN: Reverbs Visible on Map
  - FR: Réverbs visibles sur la carte
  - [ ] OK    Fix: 

## `reverbs.coordModes`

- **`xyz`**
  - EN: XYZ
  - FR: XYZ
  - [ ] OK    Fix: 

## `reverbs.dialogs`

- **`export`**
  - EN: Export Reverb Configuration
  - FR: Exporter configuration réverb
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import Reverb Configuration
  - FR: Importer configuration réverb
  - [ ] OK    Fix: 

## `reverbs.help`

- **`algoCrossoverHigh`**
  - EN: High crossover frequency for 3-band decay (1 - 10 kHz).
  - FR: Fréquence de croisement haute pour la décroissance 3 bandes (1 - 10 kHz).
  - [ ] OK    Fix: 

- **`algoCrossoverLow`**
  - EN: Low crossover frequency for 3-band decay (50 - 500 Hz).
  - FR: Fréquence de croisement basse pour la décroissance 3 bandes (50 - 500 Hz).
  - [ ] OK    Fix: 

- **`algoDiffusion`**
  - EN: Diffusion amount controlling echo density (0 - 100%).
  - FR: Quantité de diffusion contrôlant la densité d'échos (0 - 100%).
  - [ ] OK    Fix: 

- **`algoFDN`**
  - EN: Select FDN (Feedback Delay Network) reverb algorithm.
  - FR: Sélectionner l'algorithme de réverbération FDN (Feedback Delay Network).
  - [ ] OK    Fix: 

- **`algoFDNSize`**
  - EN: FDN delay line size multiplier (0.5 - 2.0x).
  - FR: Multiplicateur de taille des lignes de délai FDN (0.5 - 2.0x).
  - [ ] OK    Fix: 

- **`algoIR`**
  - EN: Select IR (Impulse Response convolution) reverb algorithm.
  - FR: Sélectionner l'algorithme de réverbération IR (convolution par réponse impulsionnelle).
  - [ ] OK    Fix: 

- **`algoIRFile`**
  - EN: Select or import an impulse response file for convolution.
  - FR: Sélectionner ou importer un fichier de réponse impulsionnelle pour la convolution.
  - [ ] OK    Fix: 

- **`algoIRLength`**
  - EN: Maximum impulse response length (0.1 - 6.0 seconds).
  - FR: Longueur maximale de la réponse impulsionnelle (0.1 - 6.0 secondes).
  - [ ] OK    Fix: 

- **`algoIRTrim`**
  - EN: Trim the start of the impulse response (0 - 100 ms).
  - FR: Couper le début de la réponse impulsionnelle (0 - 100 ms).
  - [ ] OK    Fix: 

- **`algoPerNode`**
  - EN: Use a separate IR for each reverb node, or share one IR.
  - FR: Utiliser une IR séparée pour chaque nœud de réverbération, ou partager une seule IR.
  - [ ] OK    Fix: 

- **`algoRT60`**
  - EN: Reverb decay time RT60 (0.2 - 8.0 seconds).
  - FR: Temps de décroissance de la réverbération RT60 (0.2 - 8.0 secondes).
  - [ ] OK    Fix: 

- **`algoRT60HighMult`**
  - EN: High frequency RT60 multiplier (0.1 - 9.0x).
  - FR: Multiplicateur RT60 haute fréquence (0.1 - 9.0x).
  - [ ] OK    Fix: 

- **`algoRT60LowMult`**
  - EN: Low frequency RT60 multiplier (0.1 - 9.0x).
  - FR: Multiplicateur RT60 basse fréquence (0.1 - 9.0x).
  - [ ] OK    Fix: 

- **`algoSDN`**
  - EN: Select SDN (Scattering Delay Network) reverb algorithm.
  - FR: Sélectionner l'algorithme de réverbération SDN (Scattering Delay Network).
  - [ ] OK    Fix: 

- **`algoSDNScale`**
  - EN: SDN inter-node delay scale factor (0.5 - 4.0x).
  - FR: Facteur d'échelle du délai inter-nœuds SDN (0.5 - 4.0x).
  - [ ] OK    Fix: 

- **`algoWetLevel`**
  - EN: Wet/dry mix level for reverb output (-60 to +12 dB).
  - FR: Niveau de mix wet/dry pour la sortie de réverbération (-60 à +12 dB).
  - [ ] OK    Fix: 

- **`angleOff`**
  - EN: Angle at which no amplification occurs (0-179 degrees).
  - FR: Angle auquel aucune amplification ne se produit (0-179 degrés).
  - [ ] OK    Fix: 

- **`angleOn`**
  - EN: Angle at which amplification starts (1-180 degrees).
  - FR: Angle auquel l'amplification commence (1-180 degrés).
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Reverb channel attenuation (-92 to 0 dB).
  - FR: Atténuation du canal de réverbération (-92 à 0 dB).
  - [ ] OK    Fix: 

- **`channelSelector`**
  - EN: Reverb Channel Number and Selection.
  - FR: Numéro et sélection du canal de réverbération.
  - [ ] OK    Fix: 

- **`commonAtten`**
  - EN: Common attenuation percentage (0-100%).
  - FR: Pourcentage d'atténuation commune (0-100%).
  - [ ] OK    Fix: 

- **`coordMode`**
  - EN: Coordinate display mode: Cartesian (X/Y/Z), Cylindrical (radius/azimuth/height), or Spherical (radius/azimuth/elevation).
  - FR: Mode d'affichage des coordonnées : Cartésien (X/Y/Z), Cylindrique (rayon/azimut/hauteur), ou Sphérique (rayon/azimut/élévation).
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Reverb delay/latency compensation (-100 to +100 ms).
  - FR: Compensation de délai/latence de la réverbération (-100 à +100 ms).
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Distance attenuation for reverb return (-6.0 to 0.0 dB/m).
  - FR: Atténuation par la distance pour le retour de réverbération (-6.0 à 0.0 dB/m).
  - [ ] OK    Fix: 

- **`distanceAttenEnable`**
  - EN: Distance attenuation percentage (0-200%).
  - FR: Pourcentage d'atténuation par la distance (0-200%).
  - [ ] OK    Fix: 

- **`eqBandToggle`**
  - EN: Toggle pre-EQ Band {band} on/off. When off the band is bypassed.
  - FR: Activer/désactiver la bande pré-EQ {band}.
  - [ ] OK    Fix: 

- **`eqEnable`**
  - EN: Enable or disable EQ processing for this reverb.
  - FR: Activer ou désactiver le traitement EQ pour cette réverbération.
  - [ ] OK    Fix: 

- **`eqFlatten`**
  - EN: Long press to reset all pre-EQ bands to their default values.
  - FR: Appui long pour réinitialiser toutes les bandes pré-EQ.
  - [ ] OK    Fix: 

- **`eqFreq`**
  - EN: Pre-EQ Band {band} frequency (20 Hz - 20 kHz).
  - FR: Fréquence pré-EQ bande {band} (20 Hz - 20 kHz).
  - [ ] OK    Fix: 

- **`eqGain`**
  - EN: Pre-EQ Band {band} gain (-24 to +24 dB).
  - FR: Gain pré-EQ bande {band} (-24 à +24 dB).
  - [ ] OK    Fix: 

- **`eqQ`**
  - EN: Pre-EQ Band {band} Q factor (0.1 - 20.0).
  - FR: Facteur Q pré-EQ bande {band} (0.1 - 20.0).
  - [ ] OK    Fix: 

- **`eqResetBand`**
  - EN: Long press to reset pre-EQ Band {band} to its default values.
  - FR: Appui long pour réinitialiser la bande pré-EQ {band}.
  - [ ] OK    Fix: 

- **`eqShape`**
  - EN: Pre-EQ Band {band} filter shape.
  - FR: Forme du filtre pré-EQ bande {band}.
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Reverb Configuration to file (with file explorer window).
  - FR: Exporter la configuration de réverbération vers un fichier (avec explorateur de fichiers).
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: High frequency loss per meter (-6.0 to 0.0 dB/m).
  - FR: Perte haute fréquence par mètre (-6.0 à 0.0 dB/m).
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Reverb Configuration from file (with file explorer window).
  - FR: Importer la configuration de réverbération depuis un fichier (avec explorateur de fichiers).
  - [ ] OK    Fix: 

- **`liveSourceTooltip`**
  - EN: Enable Live Source attenuation tamer. Reduces level fluctuations from sources close to the array.
  - FR: Activer le dompteur d'atténuation Live Source. Réduit les fluctuations de niveau des sources proches du groupe.
  - [ ] OK    Fix: 

- **`mapVisibility`**
  - EN: Make Visible or Hide All Reverb Channels on the Map.
  - FR: Rendre visibles ou masquer tous les canaux de réverbération sur la carte.
  - [ ] OK    Fix: 

- **`miniLatencyTooltip`**
  - EN: Enable minimal latency mode for this reverb channel. Reduces processing delay at the cost of higher CPU usage.
  - FR: Activer le mode latence minimale pour ce canal de réverbération. Réduit le délai de traitement au prix d'une utilisation CPU plus élevée.
  - [ ] OK    Fix: 

- **`muteButton`**
  - EN: Toggle mute for this output channel's reverb return.
  - FR: Activer/désactiver le mute du retour de réverbération pour cette sortie.
  - [ ] OK    Fix: 

- **`muteMacros`**
  - EN: Quick mute operations for output channels.
  - FR: Opérations rapides de mute pour les canaux de sortie.
  - [ ] OK    Fix: 

- **`nameEditor`**
  - EN: Displayed Reverb Channel Name (editable).
  - FR: Nom affiché du canal de réverbération (modifiable).
  - [ ] OK    Fix: 

- **`orientation`**
  - EN: Reverb orientation angle (-179 to +180 degrees).
  - FR: Angle d'orientation de la réverbération (-179 à +180 degrés).
  - [ ] OK    Fix: 

- **`pitch`**
  - EN: Vertical orientation of reverb (-90 to +90 degrees).
  - FR: Orientation verticale de la réverbération (-90 à +90 degrés).
  - [ ] OK    Fix: 

- **`position1`**
  - EN: Reverb virtual source {name} ({unit}).
  - FR: Source virtuelle de réverbération {name} ({unit}).
  - [ ] OK    Fix: 

- **`position2`**
  - EN: Reverb virtual source {name} ({unit}).
  - FR: Source virtuelle de réverbération {name} ({unit}).
  - [ ] OK    Fix: 

- **`position3`**
  - EN: Reverb virtual source {name} ({unit}).
  - FR: Source virtuelle de réverbération {name} ({unit}).
  - [ ] OK    Fix: 

- **`postEqBandToggle`**
  - EN: Toggle post-EQ Band {band} on/off. When off the band is bypassed.
  - FR: Activer/désactiver la bande post-EQ {band}.
  - [ ] OK    Fix: 

- **`postEqEnable`**
  - EN: Enable or disable post-processing EQ.
  - FR: Activer ou désactiver l'EQ de post-traitement.
  - [ ] OK    Fix: 

- **`postEqFlatten`**
  - EN: Long press to reset all post-EQ bands to their default values.
  - FR: Appui long pour réinitialiser toutes les bandes post-EQ.
  - [ ] OK    Fix: 

- **`postEqFreq`**
  - EN: Post-EQ Band {band} frequency (20 Hz - 20 kHz).
  - FR: Fréquence de la bande {band} du post-EQ (20 Hz - 20 kHz).
  - [ ] OK    Fix: 

- **`postEqGain`**
  - EN: Post-EQ Band {band} gain (-24 to +24 dB).
  - FR: Gain de la bande {band} du post-EQ (-24 à +24 dB).
  - [ ] OK    Fix: 

- **`postEqQ`**
  - EN: Post-EQ Band {band} Q factor (0.1 - 20.0).
  - FR: Facteur Q de la bande {band} du post-EQ (0.1 - 20.0).
  - [ ] OK    Fix: 

- **`postEqResetBand`**
  - EN: Long press to reset post-EQ Band {band} to its default values.
  - FR: Appui long pour réinitialiser la bande post-EQ {band}.
  - [ ] OK    Fix: 

- **`postEqShape`**
  - EN: Post-EQ Band {band} filter shape.
  - FR: Forme du filtre de la bande {band} du post-EQ.
  - [ ] OK    Fix: 

- **`postExpAttack`**
  - EN: Post-expander attack time (0.1 - 50 ms).
  - FR: Temps d'attaque du post-expanseur (0.1 - 50 ms).
  - [ ] OK    Fix: 

- **`postExpBypass`**
  - EN: Bypass or enable the post-expander on reverb returns.
  - FR: Contourner ou activer le post-expanseur sur les retours de réverbération.
  - [ ] OK    Fix: 

- **`postExpRatio`**
  - EN: Post-expander ratio (1:1 to 1:8).
  - FR: Ratio du post-expanseur (1:1 à 1:8).
  - [ ] OK    Fix: 

- **`postExpRelease`**
  - EN: Post-expander release time (50 - 2000 ms).
  - FR: Temps de relâchement du post-expanseur (50 - 2000 ms).
  - [ ] OK    Fix: 

- **`postExpThreshold`**
  - EN: Post-expander threshold (-80 to -10 dB).
  - FR: Seuil du post-expanseur (-80 à -10 dB).
  - [ ] OK    Fix: 

- **`preCompAttack`**
  - EN: Pre-compressor attack time (0.1 - 100 ms).
  - FR: Temps d'attaque du pré-compresseur (0.1 - 100 ms).
  - [ ] OK    Fix: 

- **`preCompBypass`**
  - EN: Bypass or enable the pre-compressor on reverb feeds.
  - FR: Contourner ou activer le pré-compresseur sur les envois de réverbération.
  - [ ] OK    Fix: 

- **`preCompRatio`**
  - EN: Pre-compressor ratio (1:1 to 20:1).
  - FR: Ratio du pré-compresseur (1:1 à 20:1).
  - [ ] OK    Fix: 

- **`preCompRelease`**
  - EN: Pre-compressor release time (10 - 1000 ms).
  - FR: Temps de relâchement du pré-compresseur (10 - 1000 ms).
  - [ ] OK    Fix: 

- **`preCompThreshold`**
  - EN: Pre-compressor threshold (-60 to 0 dB).
  - FR: Seuil du pré-compresseur (-60 à 0 dB).
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Reverb Configuration from backup file.
  - FR: Recharger la configuration de réverbération depuis le fichier de sauvegarde.
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Reverb Configuration from file.
  - FR: Recharger la configuration de réverbération depuis un fichier.
  - [ ] OK    Fix: 

- **`returnOffset1`**
  - EN: Reverb return offset {name} ({unit}).
  - FR: Décalage de retour de réverbération {name} ({unit}).
  - [ ] OK    Fix: 

- **`returnOffset2`**
  - EN: Reverb return offset {name} ({unit}).
  - FR: Décalage de retour de réverbération {name} ({unit}).
  - [ ] OK    Fix: 

- **`returnOffset3`**
  - EN: Reverb return offset {name} ({unit}).
  - FR: Décalage de retour de réverbération {name} ({unit}).
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Reverb Configuration to file (with backup).
  - FR: Enregistrer la configuration de réverbération dans un fichier (avec sauvegarde).
  - [ ] OK    Fix: 

## `reverbs.labels`

- **`angleOff`**
  - EN: Angle Off:
  - FR: Angle Off :
  - [ ] OK    Fix: 

- **`angleOn`**
  - EN: Angle On:
  - FR: Angle On :
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Attenuation:
  - FR: Atténuation :
  - [ ] OK    Fix: 

- **`commonAtten`**
  - EN: Common Atten:
  - FR: Atténuation commune :
  - [ ] OK    Fix: 

- **`coordinates`**
  - EN: Coord:
  - FR: Coord :
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Delay/Latency:
  - FR: Délai/Latence :
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Distance Atten:
  - FR: Atténuation distance :
  - [ ] OK    Fix: 

- **`distanceAttenPercent`**
  - EN: Distance Atten %:
  - FR: Atténuation distance % :
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: HF Damping:
  - FR: Atténuation HF :
  - [ ] OK    Fix: 

- **`muteMacro`**
  - EN: Mute Macro:
  - FR: Macro mute :
  - [ ] OK    Fix: 

- **`name`**
  - EN: Name:
  - FR: Nom :
  - [ ] OK    Fix: 

- **`orientation`**
  - EN: Orientation:
  - FR: Orientation :
  - [ ] OK    Fix: 

- **`outputMutes`**
  - EN: Output Mutes:
  - FR: Mutes sorties :
  - [ ] OK    Fix: 

- **`pitch`**
  - EN: Pitch:
  - FR: Inclinaison :
  - [ ] OK    Fix: 

- **`positionX`**
  - EN: Position X:
  - FR: Position X :
  - [ ] OK    Fix: 

- **`positionY`**
  - EN: Position Y:
  - FR: Position Y :
  - [ ] OK    Fix: 

- **`positionZ`**
  - EN: Position Z:
  - FR: Position Z :
  - [ ] OK    Fix: 

- **`returnOffsetX`**
  - EN: Return Offset X:
  - FR: Décalage retour X :
  - [ ] OK    Fix: 

- **`returnOffsetY`**
  - EN: Return Offset Y:
  - FR: Décalage retour Y :
  - [ ] OK    Fix: 

- **`returnOffsetZ`**
  - EN: Return Offset Z:
  - FR: Décalage retour Z :
  - [ ] OK    Fix: 

## `reverbs.messages`

- **`backupLoaded`**
  - EN: Reverb configuration loaded from backup.
  - FR: Configuration réverb chargée depuis la sauvegarde.
  - [ ] OK    Fix: 

- **`configExported`**
  - EN: Reverb configuration exported.
  - FR: Configuration réverb exportée.
  - [ ] OK    Fix: 

- **`configImported`**
  - EN: Reverb configuration imported.
  - FR: Configuration réverb importée.
  - [ ] OK    Fix: 

- **`configLoaded`**
  - EN: Reverb configuration loaded.
  - FR: Configuration réverb chargée.
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Reverb configuration saved.
  - FR: Configuration réverb enregistrée.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - FR: Erreur : {error}
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - FR: Veuillez d'abord sélectionner un dossier projet dans Config Système.
  - [ ] OK    Fix: 

## `reverbs.muteMacros`

- **`invertMutes`**
  - EN: INVERT MUTES
  - FR: INVERSER MUTES
  - [ ] OK    Fix: 

- **`muteAll`**
  - EN: MUTE ALL
  - FR: MUTER TOUT
  - [ ] OK    Fix: 

- **`muteArray`**
  - EN: MUTE ARRAY
  - FR: MUTER GROUPE
  - [ ] OK    Fix: 

- **`muteEven`**
  - EN: MUTE EVEN
  - FR: MUTER PAIRS
  - [ ] OK    Fix: 

- **`muteOdd`**
  - EN: MUTE ODD
  - FR: MUTER IMPAIRS
  - [ ] OK    Fix: 

- **`select`**
  - EN: Mute Macro Select
  - FR: Sélection macro mute
  - [ ] OK    Fix: 

- **`unmuteAll`**
  - EN: UNMUTE ALL
  - FR: DÉMUTER TOUT
  - [ ] OK    Fix: 

- **`unmuteArray`**
  - EN: UNMUTE ARRAY
  - FR: DÉMUTER GROUPE
  - [ ] OK    Fix: 

## `reverbs.postProcessing`

- **`attack`**
  - EN: Attack:
  - FR: Attaque :
  - [ ] OK    Fix: 

- **`expander`**
  - EN: Expander
  - FR: Expanseur
  - [ ] OK    Fix: 

- **`expanderOff`**
  - EN: Expander OFF
  - FR: Expanseur OFF
  - [ ] OK    Fix: 

- **`expanderOn`**
  - EN: Expander ON
  - FR: Expanseur ON
  - [ ] OK    Fix: 

- **`ratio`**
  - EN: Ratio:
  - FR: Ratio :
  - [ ] OK    Fix: 

- **`release`**
  - EN: Release:
  - FR: Relâchement :
  - [ ] OK    Fix: 

- **`threshold`**
  - EN: Threshold:
  - FR: Seuil :
  - [ ] OK    Fix: 

## `reverbs.preProcessing`

- **`attack`**
  - EN: Attack:
  - FR: Attaque :
  - [ ] OK    Fix: 

- **`compressor`**
  - EN: Compressor
  - FR: Compresseur
  - [ ] OK    Fix: 

- **`compressorOff`**
  - EN: Compressor OFF
  - FR: Compresseur OFF
  - [ ] OK    Fix: 

- **`compressorOn`**
  - EN: Compressor ON
  - FR: Compresseur ON
  - [ ] OK    Fix: 

- **`ratio`**
  - EN: Ratio:
  - FR: Ratio :
  - [ ] OK    Fix: 

- **`release`**
  - EN: Release:
  - FR: Relâchement :
  - [ ] OK    Fix: 

- **`threshold`**
  - EN: Threshold:
  - FR: Seuil :
  - [ ] OK    Fix: 

## `reverbs.sections`

- **`reverbFeed`**
  - EN: Reverb Feed
  - FR: Entrée réverb
  - [ ] OK    Fix: 

- **`reverbReturn`**
  - EN: Reverb Return
  - FR: Retour réverb
  - [ ] OK    Fix: 

## `reverbs.tabs`

- **`algorithm`**
  - EN: Algorithm
  - FR: Algorithme
  - [ ] OK    Fix: 

- **`channelParams`**
  - EN: Channel Parameters
  - FR: Paramètres du canal
  - [ ] OK    Fix: 

- **`postProcessing`**
  - EN: Post-Processing
  - FR: Post-traitement
  - [ ] OK    Fix: 

- **`preProcessing`**
  - EN: Pre-Processing
  - FR: Pré-traitement
  - [ ] OK    Fix: 

## `reverbs.toggles`

- **`liveSourceOff`**
  - EN: Live Source Atten OFF
  - FR: Atténuation live NON
  - [ ] OK    Fix: 

- **`liveSourceOn`**
  - EN: Live Source Atten ON
  - FR: Atténuation live OUI
  - [ ] OK    Fix: 

- **`minLatencyOff`**
  - EN: Minimal Latency OFF
  - FR: Latence min NON
  - [ ] OK    Fix: 

- **`minLatencyOn`**
  - EN: Minimal Latency ON
  - FR: Latence min OUI
  - [ ] OK    Fix: 

## `sampler.buttons`

- **`copy`**
  - EN: Copy
  - FR: Copier
  - [ ] OK    Fix: 

- **`copyCell`**
  - EN: Copy Cell
  - FR: Copier cellule
  - [ ] OK    Fix: 

- **`copySet`**
  - EN: Copy Set
  - FR: Copier set
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - FR: Exporter
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - FR: Importer
  - [ ] OK    Fix: 

- **`paste`**
  - EN: Paste
  - FR: Coller
  - [ ] OK    Fix: 

- **`pasteCell`**
  - EN: Paste Cell
  - FR: Coller cellule
  - [ ] OK    Fix: 

- **`pasteSet`**
  - EN: Paste Set
  - FR: Coller set
  - [ ] OK    Fix: 

## `sampler.cell`

- **`attenuation`**
  - EN: Attenuation (dB)
  - FR: Atténuation (dB)
  - [ ] OK    Fix: 

- **`clear`**
  - EN: Clear
  - FR: Effacer
  - [ ] OK    Fix: 

- **`inOut`**
  - EN: In/Out (ms)
  - FR: Entr/Sort (ms)
  - [ ] OK    Fix: 

- **`load`**
  - EN: Load
  - FR: Charger
  - [ ] OK    Fix: 

- **`loadTitle`**
  - EN: Load Sample
  - FR: Charger un sample
  - [ ] OK    Fix: 

- **`offset`**
  - EN: Offset (m)
  - FR: Décal. (m)
  - [ ] OK    Fix: 

- **`preview`**
  - EN: Preview
  - FR: Pré-écoute
  - [ ] OK    Fix: 

- **`previewStop`**
  - EN: Stop
  - FR: Arrêt
  - [ ] OK    Fix: 

## `sampler.grid`

- **`help`**
  - EN: Click = select
Shift = multi
Ctrl = set toggle
DblClick = load
  - FR: Clic=sélection | Shift=multi | Ctrl=basculer set | DblClic=charger
  - [ ] OK    Fix: 

## `sampler.labels`

- **`lightpadZone`**
  - EN: Lightpad Zone
  - FR: Zone Lightpad
  - [ ] OK    Fix: 

- **`selectZone`**
  - EN: Select Zone
  - FR: Sélectionner Zone
  - [ ] OK    Fix: 

## `sampler.lightpadZone`

- **`none`**
  - EN: None
  - FR: Aucune
  - [ ] OK    Fix: 

## `sampler.press`

- **`height`**
  - EN: Height
  - FR: Hauteur
  - [ ] OK    Fix: 

- **`hf`**
  - EN: HF Shelf
  - FR: HF Shelf
  - [ ] OK    Fix: 

- **`level`**
  - EN: Level
  - FR: Niveau
  - [ ] OK    Fix: 

- **`xy`**
  - EN: Position XY
  - FR: Position XY
  - [ ] OK    Fix: 

## `sampler.remote`

- **`gridLayout`**
  - EN: Grid Layout
  - FR: Disposition de la grille
  - [ ] OK    Fix: 

## `sampler.section`

- **`actions`**
  - EN: ACTIONS
  - FR: ACTIONS
  - [ ] OK    Fix: 

- **`cell`**
  - EN: CELL PROPERTIES
  - FR: PROPRIÉTÉS DE CELLULE
  - [ ] OK    Fix: 

- **`pressure`**
  - EN: PRESSURE MAPPINGS
  - FR: MAPPAGES DE PRESSION
  - [ ] OK    Fix: 

- **`set`**
  - EN: SET MANAGEMENT
  - FR: GESTION DES SETS
  - [ ] OK    Fix: 

## `sampler.set`

- **`copy`**
  - EN: (copy)
  - FR: (copie)
  - [ ] OK    Fix: 

- **`default`**
  - EN: Set
  - FR: Set
  - [ ] OK    Fix: 

- **`level`**
  - EN: Level (dB)
  - FR: Niveau (dB)
  - [ ] OK    Fix: 

- **`pos`**
  - EN: Position (m)
  - FR: Position (m)
  - [ ] OK    Fix: 

- **`rename`**
  - EN: Rename
  - FR: Renommer
  - [ ] OK    Fix: 

- **`roundRobin`**
  - EN: Round-Robin
  - FR: Round-Robin
  - [ ] OK    Fix: 

- **`sequential`**
  - EN: Sequential
  - FR: Séquentiel
  - [ ] OK    Fix: 

## `sampler.tooltips`

- **`addSet`**
  - EN: Create a new set. If cells are selected they will be assigned to it.
  - FR: Créer un nouveau set. Si des cellules sont sélectionnées, elles lui seront assignées.
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Cell attenuation in dB (0 = no attenuation, -60 = silent)
  - FR: Atténuation de la cellule en dB (0 = pas d'atténuation, -60 = silence)
  - [ ] OK    Fix: 

- **`clear`**
  - EN: Remove the sample from the selected cell (long press)
  - FR: Retirer l'échantillon de la cellule sélectionnée (appui long)
  - [ ] OK    Fix: 

- **`copy`**
  - EN: Copy selected cell or active set to clipboard
  - FR: Copier la cellule sélectionnée ou le set actif dans le presse-papiers
  - [ ] OK    Fix: 

- **`deleteSet`**
  - EN: Delete the active set (long press)
  - FR: Supprimer le set actif (appui long)
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export sampler configuration to file
  - FR: Exporter la configuration du sampler vers un fichier
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import sampler configuration from file
  - FR: Importer la configuration du sampler depuis un fichier
  - [ ] OK    Fix: 

- **`inOut`**
  - EN: Set the In/Out time range in milliseconds. Drag between thumbs to shift both.
  - FR: Définir les points d'entrée et de sortie en millisecondes. Glisser entre les poignées pour déplacer les deux.
  - [ ] OK    Fix: 

- **`load`**
  - EN: Load a sample file into the selected cell
  - FR: Charger un fichier d'échantillon dans la cellule sélectionnée
  - [ ] OK    Fix: 

- **`offset`**
  - EN: Offset position in meters (X, Y, Z) relative to the set position
  - FR: Décalage de position en mètres (X, Y, Z) par rapport à la position du set
  - [ ] OK    Fix: 

- **`paste`**
  - EN: Paste clipboard data to selected cell or active set
  - FR: Coller les données du presse-papiers dans la cellule sélectionnée ou le set actif
  - [ ] OK    Fix: 

- **`playMode`**
  - EN: Toggle between Sequential and Round-Robin playback
  - FR: Basculer entre lecture séquentielle et Round-Robin
  - [ ] OK    Fix: 

- **`pressCurve`**
  - EN: Pressure response curve (0 = concave, 0.5 = linear, 1 = convex)
  - FR: Courbe de réponse à la pression (0 = concave, 0,5 = linéaire, 1 = convexe)
  - [ ] OK    Fix: 

- **`pressDir`**
  - EN: Toggle pressure direction: + = more pressure increases, - = decreases
  - FR: Basculer la direction de pression : + = plus de pression augmente, - = diminue
  - [ ] OK    Fix: 

- **`pressHeight`**
  - EN: Map finger pressure to vertical position (Z)
  - FR: Mapper la pression du doigt sur la position verticale (Z)
  - [ ] OK    Fix: 

- **`pressHF`**
  - EN: Map finger pressure to high-frequency shelf attenuation
  - FR: Mapper la pression du doigt sur l'atténuation du shelving aigu
  - [ ] OK    Fix: 

- **`pressLevel`**
  - EN: Map finger pressure to output level
  - FR: Mapper la pression du doigt sur le niveau de sortie
  - [ ] OK    Fix: 

- **`pressXY`**
  - EN: Map finger pressure to XY position movement
  - FR: Mapper la pression du doigt sur le déplacement de position XY
  - [ ] OK    Fix: 

- **`pressXYScale`**
  - EN: Sensitivity: how far the source moves per pressure step
  - FR: Sensibilité : à quelle distance la source se déplace par pas de pression
  - [ ] OK    Fix: 

- **`preview`**
  - EN: Preview the loaded sample
  - FR: Pré-écouter l'échantillon chargé
  - [ ] OK    Fix: 

- **`renameSet`**
  - EN: Rename the active set
  - FR: Renommer le set actif
  - [ ] OK    Fix: 

- **`setLevel`**
  - EN: Set output level in dB (0 = unity, -60 = silent)
  - FR: Définir le niveau de sortie en dB (0 = unité, -60 = silence)
  - [ ] OK    Fix: 

- **`setPos`**
  - EN: Set base position in meters (X, Y, Z)
  - FR: Définir la position de base en mètres (X, Y, Z)
  - [ ] OK    Fix: 

## `sampler.zone`

- **`remotePad`**
  - EN: Pad {num}
  - FR: Pad {num}
  - [ ] OK    Fix: 

- **`selectRemotePad`**
  - EN: Select Remote Pad
  - FR: Sélectionner un pad de la télécommande
  - [ ] OK    Fix: 

## `setAllInputs`

- **`warning`**
  - EN: Changes will apply to ALL inputs
  - FR: Les modifications s'appliqueront à TOUTES les entrées
  - [ ] OK    Fix: 

- **`windowTitle`**
  - EN: Set All Inputs
  - FR: Appliquer à toutes les entrées
  - [ ] OK    Fix: 

## `setAllInputs.buttons`

- **`all1d`**
  - EN: All 1/d
  - FR: Tous 1/d
  - [ ] OK    Fix: 

- **`allLog`**
  - EN: All Log
  - FR: Tous Log
  - [ ] OK    Fix: 

- **`closeWindow`**
  - EN: CLOSE WINDOW
  - FR: FERMER LA FENÊTRE
  - [ ] OK    Fix: 

- **`flipXyzOff`**
  - EN: Flip XYZ > OFF
  - FR: Inverser XYZ > OFF
  - [ ] OK    Fix: 

- **`resetDirectivity`**
  - EN: Reset directivity
  - FR: Réinit. directivité
  - [ ] OK    Fix: 

- **`turnOffJitterLfo`**
  - EN: Turn OFF jitter & LFO
  - FR: Désact. jitter & LFO
  - [ ] OK    Fix: 

- **`turnOffLiveSource`**
  - EN: Turn OFF Live source atten.
  - FR: Désact. atténuation Live Source
  - [ ] OK    Fix: 

## `setAllInputs.coordinateModes`

- **`rThetaPhi`**
  - EN: r θ φ
  - FR: r θ φ
  - [ ] OK    Fix: 

- **`rThetaZ`**
  - EN: r θ Z
  - FR: r θ Z
  - [ ] OK    Fix: 

- **`xyz`**
  - EN: XYZ
  - FR: XYZ
  - [ ] OK    Fix: 

## `setAllInputs.labels`

- **`common`**
  - EN: common
  - FR: commun
  - [ ] OK    Fix: 

- **`constraintPositions`**
  - EN: Constraint positions:
  - FR: Contraintes de position :
  - [ ] OK    Fix: 

- **`coordinateMode`**
  - EN: Coordinate mode:
  - FR: Mode de coordonnées :
  - [ ] OK    Fix: 

- **`dbPerMeter`**
  - EN: dB/m
  - FR: dB/m
  - [ ] OK    Fix: 

- **`distanceAttenuation`**
  - EN: Distance attenuation
  - FR: Atténuation par distance
  - [ ] OK    Fix: 

- **`floorReflections`**
  - EN: Floor Reflections:
  - FR: Réflexions du sol :
  - [ ] OK    Fix: 

- **`fringe`**
  - EN: Fringe:
  - FR: Marge :
  - [ ] OK    Fix: 

- **`heightFactor`**
  - EN: Height factor:
  - FR: Facteur de hauteur :
  - [ ] OK    Fix: 

- **`minimalLatency`**
  - EN: Minimal Latency:
  - FR: Latence minimale :
  - [ ] OK    Fix: 

- **`muteMacros`**
  - EN: Mute macros:
  - FR: Macros de mute :
  - [ ] OK    Fix: 

- **`ratio`**
  - EN: ratio
  - FR: ratio
  - [ ] OK    Fix: 

- **`sidelines`**
  - EN: Sidelines:
  - FR: Lignes latérales :
  - [ ] OK    Fix: 

## `snapshot`

- **`qlabExportDone`**
  - EN: QLab export complete: {count} cues created
  - FR: Export QLab terminé : {count} cues créés
  - [ ] OK    Fix: 

- **`qlabExportStarted`**
  - EN: Writing {count} cues to QLab...
  - FR: Écriture de {count} cues vers QLab...
  - [ ] OK    Fix: 

- **`qlabGroupName`**
  - EN: Snapshot "{name}"
  - FR: Snapshot « {name} »
  - [ ] OK    Fix: 

- **`qlabMemoText`**
  - EN: Run either of the following cues to recall or update this snapshot
  - FR: Lancer l'un des cues suivants pour rappeler ou mettre à jour ce snapshot
  - [ ] OK    Fix: 

- **`qlabNoTarget`**
  - EN: No QLab target configured
  - FR: Aucune cible QLab configurée
  - [ ] OK    Fix: 

- **`qlabReloadName`**
  - EN: Reload "{name}"
  - FR: Rappeler "{name}"
  - [ ] OK    Fix: 

- **`qlabUpdateName`**
  - EN: Update "{name}"
  - FR: Mettre à jour "{name}"
  - [ ] OK    Fix: 

## `snapshotScope`

- **`all`**
  - EN: ALL
  - FR: TOUT
  - [ ] OK    Fix: 

- **`applyScope`**
  - EN: Apply scope:
  - FR: Appliquer la portée :
  - [ ] OK    Fix: 

- **`autoPreselectModified`**
  - EN: Auto-preselect modified parameters
  - FR: Présélectionner les paramètres modifiés
  - [ ] OK    Fix: 

- **`title`**
  - EN: Snapshot Scope: {name}
  - FR: Portée du Snapshot : {name}
  - [ ] OK    Fix: 

- **`whenRecalling`**
  - EN: When Recalling
  - FR: Au rappel
  - [ ] OK    Fix: 

- **`whenSaving`**
  - EN: When Saving
  - FR: À l'enregistrement
  - [ ] OK    Fix: 

- **`windowTitle`**
  - EN: Input Snapshot Scope
  - FR: Portée du Snapshot d'Entrée
  - [ ] OK    Fix: 

- **`writeSnapshotLoadCue`**
  - EN: Write Snapshot Load Cue to QLab
  - FR: Ecrire un cue de chargement dans QLab
  - [ ] OK    Fix: 

- **`writeSnapshotLoadCueTooltip`**
  - EN: Also create a QLab cue to load this snapshot via OSC
  - FR: Créer aussi un cue QLab pour charger ce snapshot via OSC
  - [ ] OK    Fix: 

- **`writeToQLab`**
  - EN: Write to QLab
  - FR: Exporter vers QLab
  - [ ] OK    Fix: 

- **`writeToQLabTooltip`**
  - EN: Export scope to QLab instead of saving to file
  - FR: Exporter le scope vers QLab au lieu de sauvegarder dans un fichier
  - [ ] OK    Fix: 

## `snapshotScope.buttons`

- **`cancel`**
  - EN: Cancel
  - FR: Annuler
  - [ ] OK    Fix: 

- **`clearChanges`**
  - EN: Clear Changes
  - FR: Effacer les changements
  - [ ] OK    Fix: 

- **`ok`**
  - EN: OK
  - FR: OK
  - [ ] OK    Fix: 

- **`selectModified`**
  - EN: Select Modified
  - FR: Sélectionner modifiés
  - [ ] OK    Fix: 

## `snapshotScope.sections`

- **`attenuation`**
  - EN: Attenuation
  - FR: Atténuation
  - [ ] OK    Fix: 

- **`automOtion`**
  - EN: AutomOtion
  - FR: AutomOtion
  - [ ] OK    Fix: 

- **`directivity`**
  - EN: Directivity
  - FR: Directivité
  - [ ] OK    Fix: 

- **`hackoustics`**
  - EN: Hackoustics
  - FR: Hackoustique
  - [ ] OK    Fix: 

- **`input`**
  - EN: Input
  - FR: Entrée
  - [ ] OK    Fix: 

- **`lfo`**
  - EN: LFO
  - FR: LFO
  - [ ] OK    Fix: 

- **`liveSource`**
  - EN: Live Source
  - FR: Source Live
  - [ ] OK    Fix: 

- **`mutes`**
  - EN: Mutes
  - FR: Mutes
  - [ ] OK    Fix: 

- **`position`**
  - EN: Position
  - FR: Position
  - [ ] OK    Fix: 

## `statusBar`

- **`displayLabel`**
  - EN: Display:
  - FR: Affichage :
  - [ ] OK    Fix: 

- **`helpMode`**
  - EN: Help
  - FR: Aide
  - [ ] OK    Fix: 

- **`oscMode`**
  - EN: OSC
  - FR: OSC
  - [ ] OK    Fix: 

## `systemConfig.algorithms`

- **`inputBuffer`**
  - EN: InputBuffer (read-time delays)
  - FR: InputBuffer (délais en lecture)
  - [ ] OK    Fix: 

- **`outputBuffer`**
  - EN: OutputBuffer (write-time delays)
  - FR: OutputBuffer (délais en écriture)
  - [ ] OK    Fix: 

## `systemConfig.binauralOutput`

- **`select`**
  - EN: Select...
  - FR: Sélectionner...
  - [ ] OK    Fix: 

## `systemConfig.buttons`

- **`audioPatch`**
  - EN: Audio Interface and Patching Window
  - FR: Interface audio et routage
  - [ ] OK    Fix: 

- **`binauralOff`**
  - EN: Binaural: OFF
  - FR: Binaural : OFF
  - [ ] OK    Fix: 

- **`binauralOn`**
  - EN: Binaural: ON
  - FR: Binaural : ON
  - [ ] OK    Fix: 

- **`clearSolo`**
  - EN: Clear Solo
  - FR: Clear Solo
  - [ ] OK    Fix: 

- **`copySystemInfo`**
  - EN: Copy System Info
  - FR: Copier les infos système
  - [ ] OK    Fix: 

- **`diagnosticsCollapsed`**
  - EN: Diagnostics  [+]
  - FR: Diagnostic  [+]
  - [ ] OK    Fix: 

- **`diagnosticsExpanded`**
  - EN: Diagnostics  [-]
  - FR: Diagnostic  [-]
  - [ ] OK    Fix: 

- **`exportLogs`**
  - EN: Export Logs
  - FR: Exporter les journaux
  - [ ] OK    Fix: 

- **`exportSystem`**
  - EN: Export System Configuration
  - FR: Exporter configuration système
  - [ ] OK    Fix: 

- **`importSystem`**
  - EN: Import System Configuration
  - FR: Importer configuration système
  - [ ] OK    Fix: 

- **`levelMeter`**
  - EN: Level Meters
  - FR: Level Meters
  - [ ] OK    Fix: 

- **`openLogFolder`**
  - EN: Open Log Folder
  - FR: Ouvrir le dossier des journaux
  - [ ] OK    Fix: 

- **`processingOff`**
  - EN: Processing: OFF
  - FR: Traitement : OFF
  - [ ] OK    Fix: 

- **`processingOn`**
  - EN: Processing: ON
  - FR: Traitement : ON
  - [ ] OK    Fix: 

- **`quickLongPressOff`**
  - EN: Normal
  - FR: Normal
  - [ ] OK    Fix: 

- **`quickLongPressOn`**
  - EN: Quick
  - FR: Rapide
  - [ ] OK    Fix: 

- **`reloadComplete`**
  - EN: Reload Complete Configuration
  - FR: Recharger configuration complète
  - [ ] OK    Fix: 

- **`reloadCompleteBackup`**
  - EN: Reload Complete Config. Backup
  - FR: Recharger config. complète (backup)
  - [ ] OK    Fix: 

- **`reloadSystem`**
  - EN: Reload System Configuration
  - FR: Recharger configuration système
  - [ ] OK    Fix: 

- **`reloadSystemBackup`**
  - EN: Reload System Config. Backup
  - FR: Recharger config. système (backup)
  - [ ] OK    Fix: 

- **`reportIssue`**
  - EN: Report Issue
  - FR: Signaler un problème
  - [ ] OK    Fix: 

- **`selectProjectFolder`**
  - EN: Select Project Folder
  - FR: Sélectionner dossier projet
  - [ ] OK    Fix: 

- **`setup`**
  - EN: Setup
  - FR: Config.
  - [ ] OK    Fix: 

- **`soloModeMulti`**
  - EN: Solo: Multi
  - FR: Solo : Multi
  - [ ] OK    Fix: 

- **`soloModeSingle`**
  - EN: Solo: Single
  - FR: Solo : Simple
  - [ ] OK    Fix: 

- **`storeComplete`**
  - EN: Store Complete Configuration
  - FR: Enregistrer configuration complète
  - [ ] OK    Fix: 

- **`storeSystem`**
  - EN: Store System Configuration
  - FR: Enregistrer configuration système
  - [ ] OK    Fix: 

## `systemConfig.colorSchemes`

- **`black`**
  - EN: Black
  - FR: Noir
  - [ ] OK    Fix: 

- **`default`**
  - EN: Default (Dark Gray)
  - FR: Défaut (gris foncé)
  - [ ] OK    Fix: 

- **`light`**
  - EN: Light
  - FR: Clair
  - [ ] OK    Fix: 

## `systemConfig.controller`

- **`lightpad`**
  - EN: Lightpad
  - FR: Lightpad
  - [ ] OK    Fix: 

- **`off`**
  - EN: Off
  - FR: Désactivé
  - [ ] OK    Fix: 

- **`remote`**
  - EN: Remote
  - FR: Télécommande
  - [ ] OK    Fix: 

## `systemConfig.devices`

- **`off`**
  - EN: Off
  - FR: Off
  - [ ] OK    Fix: 

## `systemConfig.dialogs`

- **`exportSystemConfig`**
  - EN: Export System Configuration
  - FR: Exporter configuration système
  - [ ] OK    Fix: 

- **`importSystemConfig`**
  - EN: Import System Configuration
  - FR: Importer configuration système
  - [ ] OK    Fix: 

- **`reduce`**
  - EN: Reduce
  - FR: Réduire
  - [ ] OK    Fix: 

- **`reduceInputChannels.message`**
  - EN: Reducing from {current} to {new} input channels will remove settings for channels {start} to {end}.

This cannot be undone.
  - FR: Réduire de {current} à {new} canaux d'entrée supprimera les paramètres des canaux {start} à {end}.

Cette action est irréversible.
  - [ ] OK    Fix: 

- **`reduceInputChannels.title`**
  - EN: Reduce Input Channels?
  - FR: Réduire les canaux d'entrée ?
  - [ ] OK    Fix: 

- **`reduceOutputChannels.message`**
  - EN: Reducing from {current} to {new} output channels will remove settings for channels {start} to {end}.

This cannot be undone.
  - FR: Réduire de {current} à {new} canaux de sortie supprimera les paramètres des canaux {start} à {end}.

Cette action est irréversible.
  - [ ] OK    Fix: 

- **`reduceOutputChannels.title`**
  - EN: Reduce Output Channels?
  - FR: Réduire les canaux de sortie ?
  - [ ] OK    Fix: 

- **`reduceReverbChannels.message`**
  - EN: Reducing from {current} to {new} reverb channels will remove settings for channels {start} to {end}.

This cannot be undone.
  - FR: Réduire de {current} à {new} canaux de réverb supprimera les paramètres des canaux {start} à {end}.

Cette action est irréversible.
  - [ ] OK    Fix: 

- **`reduceReverbChannels.title`**
  - EN: Reduce Reverb Channels?
  - FR: Réduire les canaux de réverb ?
  - [ ] OK    Fix: 

- **`selectProjectFolder`**
  - EN: Select Project Folder
  - FR: Sélectionner dossier projet
  - [ ] OK    Fix: 

## `systemConfig.help`

- **`algorithm`**
  - EN: Select the rendering algorithm from the menu.
  - FR: Sélectionnez l'algorithme de rendu dans le menu.
  - [ ] OK    Fix: 

- **`audioPatch`**
  - EN: Opens the Audio Interface and Patching Window.
  - FR: Ouvre la fenêtre d'interface audio et de routage.
  - [ ] OK    Fix: 

- **`binauralAngle`**
  - EN: Horizontal rotation of binaural listener view (degrees, 0 = facing stage).
  - FR: Rotation horizontale du point de vue de l'auditeur binaural (degrés, 0 = face à la scène).
  - [ ] OK    Fix: 

- **`binauralAtten`**
  - EN: Overall level offset for binaural output (dB).
  - FR: Décalage de niveau global pour la sortie binaurale (dB).
  - [ ] OK    Fix: 

- **`binauralDelay`**
  - EN: Additional delay for binaural output (milliseconds).
  - FR: Délai supplémentaire pour la sortie binaurale (millisecondes).
  - [ ] OK    Fix: 

- **`binauralDistance`**
  - EN: Distance of binaural listener from stage origin (meters).
  - FR: Distance de l'auditeur binaural par rapport à l'origine de la scène (mètres).
  - [ ] OK    Fix: 

- **`binauralEnable`**
  - EN: Enable or disable the binaural renderer processing.
  - FR: Activer ou désactiver le traitement du moteur de rendu binaural.
  - [ ] OK    Fix: 

- **`binauralOutput`**
  - EN: Select output channel pair for binaural monitoring. Off disables binaural output.
  - FR: Sélectionner la paire de canaux de sortie pour le monitoring binaural. Off désactive la sortie binaurale.
  - [ ] OK    Fix: 

- **`colorScheme`**
  - EN: Select the color scheme: Default (dark gray), Black (pure black for OLED displays), or Light (daytime use).
  - FR: Sélectionnez le thème couleur : Défaut (gris foncé), Noir (noir pur pour écrans OLED), ou Clair (utilisation de jour).
  - [ ] OK    Fix: 

- **`copySystemInfo`**
  - EN: Copy detailed system information to the clipboard for support requests.
  - FR: Copier les informations détaillées du système dans le presse-papiers pour les demandes de support.
  - [ ] OK    Fix: 

- **`diagnosticsToggle`**
  - EN: Long-press to show or hide the diagnostic tools (export logs, open log folder, copy system info, report issue).
  - FR: Appui long pour afficher ou masquer les outils de diagnostic (export des journaux, ouverture du dossier de logs, copie des infos système, rapport d'incident).
  - [ ] OK    Fix: 

- **`dialsAndButtons`**
  - EN: Select the hardware controller for dials and buttons: Stream Deck+ or XenceLabs Quick Keys.
  - FR: Sélectionner le contrôleur matériel pour les boutons et molettes : Stream Deck+ ou XenceLabs Quick Keys.
  - [ ] OK    Fix: 

- **`domeElevation`**
  - EN: Dome elevation angle: 180 = hemisphere, 360 = full sphere.
  - FR: Angle d'élévation du dôme : 180 = hémisphère, 360 = sphère complète.
  - [ ] OK    Fix: 

- **`exportLogs`**
  - EN: Export diagnostic logs to a zip file for debugging or support.
  - FR: Exporter les journaux de diagnostic dans un fichier zip pour le débogage ou le support.
  - [ ] OK    Fix: 

- **`exportSystem`**
  - EN: Export System Configuration to file (with file explorer window).
  - FR: Exporte la configuration système vers un fichier (avec explorateur de fichiers).
  - [ ] OK    Fix: 

- **`haasEffect`**
  - EN: Hass Effect to apply to the system. Will take into account the Latency Compensations (System, Input and Output).
  - FR: Effet Haas à appliquer au système. Prend en compte les compensations de latence (système, entrée et sortie).
  - [ ] OK    Fix: 

- **`importSystem`**
  - EN: Import System Configuration from file (with file explorer window).
  - FR: Importe la configuration système depuis un fichier (avec explorateur de fichiers).
  - [ ] OK    Fix: 

- **`inputChannels`**
  - EN: Number of Input Channels.
  - FR: Nombre de canaux d'entrée.
  - [ ] OK    Fix: 

- **`language`**
  - EN: Select the user interface language. Changes take full effect after restarting the application.
  - FR: Sélectionnez la langue de l'interface. Les changements prennent plein effet après redémarrage de l'application.
  - [ ] OK    Fix: 

- **`lightpadSetup`**
  - EN: Display the connected Roli Lightpads and allow for splitting them in 4 smaller pads.
  - FR: Affiche les Roli Lightpads connectés et permet de les diviser en 4 pads plus petits.
  - [ ] OK    Fix: 

- **`masterLevel`**
  - EN: Master Level (affects all outputs).
  - FR: Niveau master (affecte toutes les sorties).
  - [ ] OK    Fix: 

- **`openLogFolder`**
  - EN: Open the application's log folder in the system file explorer.
  - FR: Ouvrir le dossier des journaux de l'application dans l'explorateur de fichiers.
  - [ ] OK    Fix: 

- **`originCenter`**
  - EN: Set origin to center of stage volume. Typical for Spherical Dome setups.
  - FR: Place l'origine au centre du volume de la scène. Typique pour les configurations Dôme Sphérique.
  - [ ] OK    Fix: 

- **`originCenterGround`**
  - EN: Set origin to center of stage at ground level. Typical for Surround or Cylindrical setups.
  - FR: Place l'origine au centre de la scène au niveau du sol. Typique pour les configurations Surround ou Cylindriques.
  - [ ] OK    Fix: 

- **`originDepth`**
  - EN: Origin Y offset from stage center (0 = centered, negative = front/downstage).
  - FR: Décalage Y de l'origine depuis le centre de la scène (0 = centré, négatif = avant/devant scène).
  - [ ] OK    Fix: 

- **`originDismissInputs`**
  - EN: Long press to dismiss and keep current input positions.
  - FR: Appui long pour ignorer et conserver les positions actuelles des entrées.
  - [ ] OK    Fix: 

- **`originDismissOutputs`**
  - EN: Long press to dismiss and keep current output positions.
  - FR: Appui long pour ignorer et conserver les positions actuelles des sorties.
  - [ ] OK    Fix: 

- **`originDismissReverbs`**
  - EN: Long press to dismiss and keep current reverb positions.
  - FR: Appui long pour ignorer et conserver les positions actuelles des réverbs.
  - [ ] OK    Fix: 

- **`originFront`**
  - EN: Set origin to front center of stage. Typical for frontal stages.
  - FR: Place l'origine au centre avant de la scène. Typique pour les scènes frontales.
  - [ ] OK    Fix: 

- **`originHeight`**
  - EN: Origin Z offset from floor (0 = floor level, positive = above floor).
  - FR: Décalage Z de l'origine depuis le sol (0 = niveau du sol, positif = au-dessus du sol).
  - [ ] OK    Fix: 

- **`originShiftInputs`**
  - EN: Long press to shift all input positions by the origin change.
  - FR: Appui long pour décaler toutes les positions d'entrée selon le changement d'origine.
  - [ ] OK    Fix: 

- **`originShiftOutputs`**
  - EN: Long press to shift all output positions by the origin change.
  - FR: Appui long pour décaler toutes les positions de sortie selon le changement d'origine.
  - [ ] OK    Fix: 

- **`originShiftReverbs`**
  - EN: Long press to shift all reverb positions by the origin change.
  - FR: Appui long pour décaler toutes les positions de réverb selon le changement d'origine.
  - [ ] OK    Fix: 

- **`originWidth`**
  - EN: Origin X offset from stage center (0 = centered, negative = left).
  - FR: Décalage X de l'origine depuis le centre de la scène (0 = centré, négatif = gauche).
  - [ ] OK    Fix: 

- **`outputChannels`**
  - EN: Number of Output Channels.
  - FR: Nombre de canaux de sortie.
  - [ ] OK    Fix: 

- **`positionControl`**
  - EN: Select the hardware controller for position control: Space Mouse, Joystick, or Game Pad.
  - FR: Sélectionner le contrôleur matériel pour le contrôle de position : Space Mouse, Joystick ou manette de jeu.
  - [ ] OK    Fix: 

- **`processing`**
  - EN: Lock all I/O parameters and start the DSP. Long press to stop the DSP.
  - FR: Verrouille tous les paramètres E/S et démarre le DSP. Appui long pour arrêter le DSP.
  - [ ] OK    Fix: 

- **`quickLongPress`**
  - EN: Long press duration. Instead of confirmation windows this software uses long presses.
  - FR: Durée de l'appui long. Au lieu de fenêtres de confirmation, ce logiciel utilise des appuis longs.
  - [ ] OK    Fix: 

- **`reloadComplete`**
  - EN: Reload Complete Configuration from files.
  - FR: Recharge la configuration complète depuis les fichiers.
  - [ ] OK    Fix: 

- **`reloadCompleteBackup`**
  - EN: Reload Complete Configuration from backup files.
  - FR: Recharge la configuration complète depuis les fichiers de sauvegarde.
  - [ ] OK    Fix: 

- **`reloadSystem`**
  - EN: Reload System Configuration from file.
  - FR: Recharge la configuration système depuis le fichier.
  - [ ] OK    Fix: 

- **`reloadSystemBackup`**
  - EN: Reload System Configuration from backup file.
  - FR: Recharge la configuration système depuis le fichier de sauvegarde.
  - [ ] OK    Fix: 

- **`remoteSetup`**
  - EN: Select the number of pads in the Remote's XY Pads tab.
  - FR: Sélectionner le nombre de pads dans l'onglet XY Pads du Remote.
  - [ ] OK    Fix: 

- **`reportIssue`**
  - EN: Open the WFS-DIY GitHub issues page in your default browser.
  - FR: Ouvrir la page des problèmes GitHub de WFS-DIY dans votre navigateur par défaut.
  - [ ] OK    Fix: 

- **`reverbChannels`**
  - EN: Number of Reverb Channels.
  - FR: Nombre de canaux de réverbération.
  - [ ] OK    Fix: 

- **`sampler`**
  - EN: Enable or disable the Sampler feature for input channels.
  - FR: Activer ou désactiver la fonction Sampler pour les canaux d'entrée. Sélectionner le contrôleur : Lightpad ou Télécommande.
  - [ ] OK    Fix: 

- **`selectProjectFolder`**
  - EN: Select the Location of the Current Project Folder where to store files.
  - FR: Sélectionnez l'emplacement du dossier de projet actuel pour stocker les fichiers.
  - [ ] OK    Fix: 

- **`showLocation`**
  - EN: Location of the current show.
  - FR: Lieu du spectacle actuel.
  - [ ] OK    Fix: 

- **`showName`**
  - EN: Name of the current show.
  - FR: Nom du spectacle actuel.
  - [ ] OK    Fix: 

- **`soloMode`**
  - EN: Single: one input at a time. Multi: multiple inputs simultaneously.
  - FR: Simple : une entrée à la fois. Multi : plusieurs entrées simultanément.
  - [ ] OK    Fix: 

- **`speedOfSound`**
  - EN: Speed of Sound (related to the temperature).
  - FR: Vitesse du son (liée à la température).
  - [ ] OK    Fix: 

- **`stageDepth`**
  - EN: Depth of the stage in meters (Box shape only).
  - FR: Profondeur de la scène en mètres (forme Boîte uniquement).
  - [ ] OK    Fix: 

- **`stageDiameter`**
  - EN: Diameter of the stage in meters (Cylinder and Dome shapes).
  - FR: Diamètre de la scène en mètres (formes Cylindre et Dôme).
  - [ ] OK    Fix: 

- **`stageDismiss`**
  - EN: Long press to dismiss and keep current input positions.
  - FR: Appui long pour ignorer et conserver les positions actuelles des entrées.
  - [ ] OK    Fix: 

- **`stageFit`**
  - EN: Long press to move outlier inputs back within the new stage bounds.
  - FR: Appui long pour ramener les entrées hors limites à l'intérieur des nouvelles dimensions de la scène.
  - [ ] OK    Fix: 

- **`stageHeight`**
  - EN: Height of the stage in meters (Box and Cylinder shapes).
  - FR: Hauteur de la scène en mètres (formes Boîte et Cylindre).
  - [ ] OK    Fix: 

- **`stageScale`**
  - EN: Long press to scale all input positions proportionally to the new stage dimensions.
  - FR: Appui long pour mettre à l'échelle toutes les positions d'entrée proportionnellement aux nouvelles dimensions de la scène.
  - [ ] OK    Fix: 

- **`stageShape`**
  - EN: Select the shape of the stage (Box, Cylinder, or Dome).
  - FR: Sélectionnez la forme de la scène (Boîte, Cylindre ou Dôme).
  - [ ] OK    Fix: 

- **`stageWidth`**
  - EN: Width of the stage in meters (Box shape only).
  - FR: Largeur de la scène en mètres (forme Boîte uniquement).
  - [ ] OK    Fix: 

- **`storeComplete`**
  - EN: Store Complete Configuration to files (with backup).
  - FR: Enregistre la configuration complète dans des fichiers (avec sauvegarde).
  - [ ] OK    Fix: 

- **`storeSystem`**
  - EN: Store System Configuration to file (with backup).
  - FR: Enregistre la configuration système dans un fichier (avec sauvegarde).
  - [ ] OK    Fix: 

- **`systemLatency`**
  - EN: Total latency of the system (Mixing board & Computer) / Specific Input and Output Latency/Delay can be set in the respective Input and Output settings.
  - FR: Latence totale du système (console de mixage et ordinateur) / La latence/délai spécifique des entrées et sorties peut être définie dans les paramètres respectifs.
  - [ ] OK    Fix: 

- **`temperature`**
  - EN: Temperature (gives the Speed of Sound).
  - FR: Température (détermine la vitesse du son).
  - [ ] OK    Fix: 

## `systemConfig.labels`

- **`algorithm`**
  - EN: Algorithm:
  - FR: Algorithme :
  - [ ] OK    Fix: 

- **`binauralAngle`**
  - EN: Listener Angle:
  - FR: Angle auditeur :
  - [ ] OK    Fix: 

- **`binauralAtten`**
  - EN: Binaural Level:
  - FR: Niveau binaural :
  - [ ] OK    Fix: 

- **`binauralDelay`**
  - EN: Binaural Delay:
  - FR: Délai binaural :
  - [ ] OK    Fix: 

- **`binauralDistance`**
  - EN: Listener Distance:
  - FR: Distance auditeur :
  - [ ] OK    Fix: 

- **`binauralOutput`**
  - EN: Binaural Output:
  - FR: Sortie binaurale :
  - [ ] OK    Fix: 

- **`clickToSplit`**
  - EN: Click to split
  - FR: Cliquer pour diviser
  - [ ] OK    Fix: 

- **`colorScheme`**
  - EN: Color Scheme:
  - FR: Thème couleur :
  - [ ] OK    Fix: 

- **`dialsAndButtons`**
  - EN: Dials and Buttons:
  - FR: Boutons et molettes :
  - [ ] OK    Fix: 

- **`domeElevation`**
  - EN: Elevation:
  - FR: Élévation :
  - [ ] OK    Fix: 

- **`haasEffect`**
  - EN: Haas Effect:
  - FR: Effet Haas :
  - [ ] OK    Fix: 

- **`inputChannels`**
  - EN: Input Channels:
  - FR: Canaux d'entrée :
  - [ ] OK    Fix: 

- **`language`**
  - EN: Language:
  - FR: Langue :
  - [ ] OK    Fix: 

- **`lightpadArrangement`**
  - EN: Lightpad Arrangement
  - FR: Arrangement Lightpad
  - [ ] OK    Fix: 

- **`masterLevel`**
  - EN: Master Level:
  - FR: Niveau master :
  - [ ] OK    Fix: 

- **`originDepth`**
  - EN: Origin Depth:
  - FR: Origine profondeur :
  - [ ] OK    Fix: 

- **`originHeight`**
  - EN: Origin Height:
  - FR: Origine hauteur :
  - [ ] OK    Fix: 

- **`originWidth`**
  - EN: Origin Width:
  - FR: Origine largeur :
  - [ ] OK    Fix: 

- **`outputChannels`**
  - EN: Output Channels:
  - FR: Canaux de sortie :
  - [ ] OK    Fix: 

- **`positionControl`**
  - EN: Position Control:
  - FR: Contrôle de position :
  - [ ] OK    Fix: 

- **`quickLongPress`**
  - EN: Long Press:
  - FR: Appui long :
  - [ ] OK    Fix: 

- **`reverbChannels`**
  - EN: Reverb Channels:
  - FR: Canaux de réverb :
  - [ ] OK    Fix: 

- **`sampler`**
  - EN: Sampler:
  - FR: Sampler :
  - [ ] OK    Fix: 

- **`showLocation`**
  - EN: Location:
  - FR: Lieu :
  - [ ] OK    Fix: 

- **`showName`**
  - EN: Name:
  - FR: Nom :
  - [ ] OK    Fix: 

- **`speedOfSound`**
  - EN: Speed of Sound:
  - FR: Vitesse du son :
  - [ ] OK    Fix: 

- **`split`**
  - EN: Split
  - FR: Divisé
  - [ ] OK    Fix: 

- **`stageDepth`**
  - EN: Depth:
  - FR: Profondeur :
  - [ ] OK    Fix: 

- **`stageDiameter`**
  - EN: Diameter:
  - FR: Diamètre :
  - [ ] OK    Fix: 

- **`stageHeight`**
  - EN: Height:
  - FR: Hauteur :
  - [ ] OK    Fix: 

- **`stageShape`**
  - EN: Stage Shape:
  - FR: Forme de scène :
  - [ ] OK    Fix: 

- **`stageWidth`**
  - EN: Width:
  - FR: Largeur :
  - [ ] OK    Fix: 

- **`systemLatency`**
  - EN: System Latency:
  - FR: Latence système :
  - [ ] OK    Fix: 

- **`temperature`**
  - EN: Temperature:
  - FR: Température :
  - [ ] OK    Fix: 

- **`updateAvailable`**
  - EN: Update {version} available
  - FR: Mise à jour {version} disponible
  - [ ] OK    Fix: 

- **`version`**
  - EN: Version {version}
  - FR: Version {version}
  - [ ] OK    Fix: 

## `systemConfig.messages`

- **`configLoaded`**
  - EN: Complete configuration loaded.
  - FR: Configuration complète chargée.
  - [ ] OK    Fix: 

- **`configLoadedFromBackup`**
  - EN: Configuration loaded from backup.
  - FR: Configuration chargée depuis la sauvegarde.
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Complete configuration saved.
  - FR: Configuration complète enregistrée.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - FR: Erreur : {error}
  - [ ] OK    Fix: 

- **`languageChanged`**
  - EN: Language changed to: {language} (requires restart for full effect)
  - FR: Langue changée en : {language} (nécessite un redémarrage pour prendre plein effet)
  - [ ] OK    Fix: 

- **`logDirNotFound`**
  - EN: Log directory not found
  - FR: Dossier des journaux introuvable
  - [ ] OK    Fix: 

- **`logsExported`**
  - EN: Logs exported to {path}
  - FR: Journaux exportés vers {path}
  - [ ] OK    Fix: 

- **`logsExportFailed`**
  - EN: Failed to export logs
  - FR: Échec de l'exportation des journaux
  - [ ] OK    Fix: 

- **`noBackupFilesFound`**
  - EN: No backup files found.
  - FR: Aucun fichier de sauvegarde trouvé.
  - [ ] OK    Fix: 

- **`partialLoad`**
  - EN: Partial load: {error}
  - FR: Chargement partiel : {error}
  - [ ] OK    Fix: 

- **`partialLoadFromBackup`**
  - EN: Partial load from backup: {error}
  - FR: Chargement partiel depuis sauvegarde : {error}
  - [ ] OK    Fix: 

- **`restartBody`**
  - EN: Please restart the application for the language change to take full effect.
  - FR: Veuillez redémarrer l'application pour que le changement de langue prenne pleinement effet.
  - [ ] OK    Fix: 

- **`restartTitle`**
  - EN: Restart Required
  - FR: Redémarrage nécessaire
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder first.
  - FR: Veuillez d'abord sélectionner un dossier de projet.
  - [ ] OK    Fix: 

- **`selectLogExportFolder`**
  - EN: Select destination for log export
  - FR: Sélectionner la destination pour l'exportation des journaux
  - [ ] OK    Fix: 

- **`systemConfigExported`**
  - EN: System configuration exported.
  - FR: Configuration système exportée.
  - [ ] OK    Fix: 

- **`systemConfigFileNotFound`**
  - EN: System configuration file not found.
  - FR: Fichier de configuration système introuvable.
  - [ ] OK    Fix: 

- **`systemConfigImported`**
  - EN: System configuration imported.
  - FR: Configuration système importée.
  - [ ] OK    Fix: 

- **`systemConfigLoaded`**
  - EN: System configuration loaded.
  - FR: Configuration système chargée.
  - [ ] OK    Fix: 

- **`systemConfigLoadedFromBackup`**
  - EN: System configuration loaded from backup.
  - FR: Configuration système chargée depuis la sauvegarde.
  - [ ] OK    Fix: 

- **`systemConfigSaved`**
  - EN: System configuration saved.
  - FR: Configuration système enregistrée.
  - [ ] OK    Fix: 

- **`systemInfoCopied`**
  - EN: System info copied to clipboard
  - FR: Informations système copiées dans le presse-papiers
  - [ ] OK    Fix: 

## `systemConfig.sections`

- **`binauralRenderer`**
  - EN: Binaural Renderer
  - FR: Rendu Binaural
  - [ ] OK    Fix: 

- **`controllers`**
  - EN: Controllers
  - FR: Controllers
  - [ ] OK    Fix: 

- **`io`**
  - EN: I/O
  - FR: E/S
  - [ ] OK    Fix: 

- **`master`**
  - EN: Master Section
  - FR: Section Master
  - [ ] OK    Fix: 

- **`show`**
  - EN: Show
  - FR: Spectacle
  - [ ] OK    Fix: 

- **`stage`**
  - EN: Stage
  - FR: Scène
  - [ ] OK    Fix: 

- **`ui`**
  - EN: UI
  - FR: Interface
  - [ ] OK    Fix: 

- **`wfsProcessor`**
  - EN: WFS Processor
  - FR: Processeur WFS
  - [ ] OK    Fix: 

## `systemConfig.stageShapes`

- **`box`**
  - EN: Box
  - FR: Boîte
  - [ ] OK    Fix: 

- **`cylinder`**
  - EN: Cylinder
  - FR: Cylindre
  - [ ] OK    Fix: 

- **`dome`**
  - EN: Dome
  - FR: Dôme
  - [ ] OK    Fix: 

## `tabs`

- **`clusters`**
  - EN: Clusters
  - FR: Clusters
  - [ ] OK    Fix: 

- **`inputs`**
  - EN: Inputs
  - FR: Entrées
  - [ ] OK    Fix: 

- **`map`**
  - EN: Map
  - FR: Carte
  - [ ] OK    Fix: 

- **`network`**
  - EN: Network
  - FR: Réseau
  - [ ] OK    Fix: 

- **`outputs`**
  - EN: Outputs
  - FR: Sorties
  - [ ] OK    Fix: 

- **`reverb`**
  - EN: Reverb
  - FR: Réverb
  - [ ] OK    Fix: 

- **`systemConfig`**
  - EN: System Config
  - FR: Configuration
  - [ ] OK    Fix: 

## `touchscreens`

- **`button`**
  - EN: Setup
  - FR: Configurer
  - [ ] OK    Fix: 

- **`label`**
  - EN: Touchscreen
  - FR: Écran tactile
  - [ ] OK    Fix: 

## `units`

- **`celsius`**
  - EN: °C
  - FR: °C
  - [ ] OK    Fix: 

- **`decibelPerMeter`**
  - EN: dB/m
  - FR: dB/m
  - [ ] OK    Fix: 

- **`decibels`**
  - EN: dB
  - FR: dB
  - [ ] OK    Fix: 

- **`degrees`**
  - EN: °
  - FR: °
  - [ ] OK    Fix: 

- **`meters`**
  - EN: m
  - FR: m
  - [ ] OK    Fix: 

- **`metersPerSecond`**
  - EN: m/s
  - FR: m/s
  - [ ] OK    Fix: 

- **`milliseconds`**
  - EN: ms
  - FR: ms
  - [ ] OK    Fix: 

## `wizard.buttons`

- **`back`**
  - EN: Back
  - FR: Précédent
  - [ ] OK    Fix: 

- **`close`**
  - EN: Close
  - FR: Fermer
  - [ ] OK    Fix: 

- **`done`**
  - EN: Done
  - FR: Terminer
  - [ ] OK    Fix: 

- **`gettingStarted`**
  - EN: Getting Started
  - FR: Démarrage
  - [ ] OK    Fix: 

- **`gettingStartedHelp`**
  - EN: Help cards guiding you through the first parameters to adjust when starting a new project
  - FR: Fiches d'aide vous guidant à travers les premiers paramètres à ajuster lors du démarrage d'un nouveau projet
  - [ ] OK    Fix: 

- **`next`**
  - EN: Next
  - FR: Suivant
  - [ ] OK    Fix: 

- **`skip`**
  - EN: Skip
  - FR: Passer
  - [ ] OK    Fix: 

## `wizard.steps`

- **`audioDevice.description`**
  - EN: Select your audio driver and device, set the sample rate and buffer size. Check the patch routing and test your outputs. Close this window when done.
  - FR: Sélectionnez votre pilote et périphérique audio, réglez la fréquence d'échantillonnage et la taille du tampon. Vérifiez le routage et testez vos sorties. Fermez cette fenêtre quand c'est fait.
  - [ ] OK    Fix: 

- **`audioDevice.title`**
  - EN: Configure the Audio Interface
  - FR: Configurer l'interface audio
  - [ ] OK    Fix: 

- **`audioInterface.description`**
  - EN: Click the button above or press Next to open the Audio Interface window.
  - FR: Cliquez sur le bouton ci-dessus ou appuyez sur Suivant pour ouvrir la fenêtre de l'interface audio.
  - [ ] OK    Fix: 

- **`audioInterface.title`**
  - EN: Open the Audio Interface
  - FR: Ouvrir l'interface audio
  - [ ] OK    Fix: 

- **`configureOutputs.description`**
  - EN: Use the array presets and geometry tools to calculate speaker positions for your arrays. Close this window when done.
  - FR: Utilisez les préréglages et les outils de géométrie pour calculer les positions des haut-parleurs. Fermez cette fenêtre quand c'est fait.
  - [ ] OK    Fix: 

- **`configureOutputs.title`**
  - EN: Configure Output Positions
  - FR: Configurer les positions de sortie
  - [ ] OK    Fix: 

- **`exploreInputs.description`**
  - EN: Click an input on the map to select it, or lasso several to move them together. Drag to position your sources. Zoom with the mouse wheel or pinch gesture, pan with right-drag or two-finger drag. Add inputs, group them into clusters, and shape your sound field. You can also control positions with a keyboard, SpaceMouse, or other external controllers. Have fun!
  - FR: Cliquez sur une entrée sur la carte pour la sélectionner, ou utilisez le lasso pour en sélectionner plusieurs. Faites glisser pour positionner vos sources. Zoomez avec la molette ou le pincement, déplacez la vue avec un clic droit ou deux doigts. Ajoutez des entrées, regroupez-les en clusters et façonnez votre champ sonore. Vous pouvez aussi contrôler les positions avec un clavier, SpaceMouse ou d'autres contrôleurs. Amusez-vous !
  - [ ] OK    Fix: 

- **`exploreInputs.title`**
  - EN: Start Creating!
  - FR: Commencez à créer !
  - [ ] OK    Fix: 

- **`inputChannels.description`**
  - EN: How many audio sources will you be spatializing?
Set the number of input channels to match your source count.
  - FR: Combien de sources audio allez-vous spatialiser ?
Définissez le nombre de canaux d'entrée en fonction de vos sources.
  - [ ] OK    Fix: 

- **`inputChannels.title`**
  - EN: Set Input Channels
  - FR: Définir les canaux d'entrée
  - [ ] OK    Fix: 

- **`originPoint.description`**
  - EN: The origin is the reference point for all coordinates. Use the preset buttons or enter custom values. 'Front' places it at the audience edge.
  - FR: L'origine est le point de référence pour toutes les coordonnées. Utilisez les boutons de présélection ou entrez des valeurs personnalisées. 'Front' le place au bord du public.
  - [ ] OK    Fix: 

- **`originPoint.title`**
  - EN: Set the Origin Point
  - FR: Définir le point d'origine
  - [ ] OK    Fix: 

- **`outputChannels.description`**
  - EN: Set the number of output channels to match your speaker array.
Each output corresponds to one physical speaker.
  - FR: Définissez le nombre de canaux de sortie en fonction de votre réseau de haut-parleurs.
Chaque sortie correspond à un haut-parleur physique.
  - [ ] OK    Fix: 

- **`outputChannels.title`**
  - EN: Set Output Channels
  - FR: Définir les canaux de sortie
  - [ ] OK    Fix: 

- **`projectFolder.description`**
  - EN: Choose a folder to store your WFS project files. This will hold configurations, snapshots, IR files, and samples. Click the button to open the folder selector.
  - FR: Choisissez un dossier pour stocker vos fichiers de projet WFS. Il contiendra les configurations, snapshots, fichiers IR et samples. Cliquez sur le bouton pour ouvrir le sélecteur de dossier.
  - [ ] OK    Fix: 

- **`projectFolder.title`**
  - EN: Select a Project Folder
  - FR: Sélectionner un dossier de projet
  - [ ] OK    Fix: 

- **`reverbChannels.description`**
  - EN: Reverb channels add room simulation. Set to 0 if you don't need reverb.
  - FR: Les canaux de réverbération ajoutent une simulation de salle. Mettez 0 si vous n'avez pas besoin de réverbération.
  - [ ] OK    Fix: 

- **`reverbChannels.title`**
  - EN: Set Reverb Channels
  - FR: Définir les canaux de réverbération
  - [ ] OK    Fix: 

- **`stageConfig.description`**
  - EN: Set the shape and dimensions of your performance space. Choose box, cylinder, or dome, then enter the size in meters.
  - FR: Définissez la forme et les dimensions de votre espace de performance. Choisissez boîte, cylindre ou dôme, puis entrez les dimensions en mètres.
  - [ ] OK    Fix: 

- **`stageConfig.title`**
  - EN: Define the Stage
  - FR: Définir la scène
  - [ ] OK    Fix: 

- **`startProcessing.description`**
  - EN: You're all set! Long-press the Processing button to start the WFS engine. You can also start the Binaural Renderer for headphone monitoring.
  - FR: Vous êtes prêt ! Appuyez longuement sur le bouton Processing pour démarrer le moteur WFS. Vous pouvez aussi démarrer le rendu binaural pour l'écoute au casque.
  - [ ] OK    Fix: 

- **`startProcessing.title`**
  - EN: Start the WFS Engine
  - FR: Démarrer le moteur WFS
  - [ ] OK    Fix: 

- **`wizardOfOutZ.description`**
  - EN: Click the Wizard of OutZ button or press Next to open the output array helper.
  - FR: Cliquez sur le bouton Wizard of OutZ ou appuyez sur Suivant pour ouvrir l'assistant de positionnement.
  - [ ] OK    Fix: 

- **`wizardOfOutZ.title`**
  - EN: Position Your Outputs
  - FR: Positionner vos sorties
  - [ ] OK    Fix: 


