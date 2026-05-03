# Proofreading checklist — Spanish (Español)

Locale: `es`  |  Total keys: 1520  |  Source: `Resources/lang/en.json` vs `Resources/lang/es.json`

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
  - ES: aplicado
  - [ ] OK    Fix: 

- **`atHead`**
  - EN: (no undone records — at the head)
  - ES: (sin registros deshechos — al inicio)
  - [ ] OK    Fix: 

- **`batch`**
  - EN: batch {id}
  - ES: lote {id}
  - [ ] OK    Fix: 

- **`cursorLabel`**
  - EN: ◂  cursor (↑ applied  /  ↓ undone, redoable)
  - ES: ◂  cursor (↑ aplicado  /  ↓ deshecho, rehacible)
  - [ ] OK    Fix: 

- **`noChanges`**
  - EN: No AI changes yet.
  - ES: Aún no hay cambios de IA.
  - [ ] OK    Fix: 

- **`of`**
  - EN: of
  - ES: de
  - [ ] OK    Fix: 

- **`stepBack`**
  - EN: ⏮ Step Back
  - ES: ⏮ Atrás
  - [ ] OK    Fix: 

- **`stepForward`**
  - EN: Step Forward ⏭
  - ES: Adelante ⏭
  - [ ] OK    Fix: 

- **`undone`**
  - EN: undone
  - ES: deshecho
  - [ ] OK    Fix: 

- **`windowTitle`**
  - EN: AI Change History
  - ES: Historial de cambios de IA
  - [ ] OK    Fix: 

## `ai.server`

- **`copyUrlConfirm`**
  - EN: MCP URL copied to clipboard: {url}
  - ES: URL MCP copiada al portapapeles: {url}
  - [ ] OK    Fix: 

- **`label`**
  - EN: MCP Server:
  - ES: Servidor MCP:
  - [ ] OK    Fix: 

- **`openHistory`**
  - EN: Open AI History
  - ES: Abrir historial de IA
  - [ ] OK    Fix: 

- **`urlButtonRunning`**
  - EN: http://{host}:{port}/mcp
  - ES: http://{host}:{port}/mcp
  - [ ] OK    Fix: 

- **`urlButtonStopped`**
  - EN: (server not running)
  - ES: (servidor detenido)
  - [ ] OK    Fix: 

## `ai.tier`

- **`aiOff`**
  - EN: AI: OFF
  - ES: IA: OFF
  - [ ] OK    Fix: 

- **`aiOn`**
  - EN: AI: ON
  - ES: IA: ON
  - [ ] OK    Fix: 

- **`gateAllowed`**
  - EN: AI critical actions: ALLOWED
  - ES: Acciones IA críticas: PERMITIDAS
  - [ ] OK    Fix: 

- **`gateBlocked`**
  - EN: AI critical actions: blocked
  - ES: Acciones IA críticas: bloqueadas
  - [ ] OK    Fix: 

- **`tier2AutoOff`**
  - EN: Tier 2 auto-confirm: off
  - ES: Auto-confirmación nivel 2: off
  - [ ] OK    Fix: 

- **`tier2AutoOn`**
  - EN: Tier 2 auto-confirm: ON (5 min)
  - ES: Auto-confirmación nivel 2: ON (5 min)
  - [ ] OK    Fix: 

## `ai.toast`

- **`clientPrefix`**
  - EN: Claude
  - ES: Claude
  - [ ] OK    Fix: 

- **`header`**
  - EN: AI changes
  - ES: Cambios de IA
  - [ ] OK    Fix: 

- **`moreOlder`**
  - EN: …and {count} older
  - ES: …y {count} más antiguos
  - [ ] OK    Fix: 

## `ai.tooltips`

- **`aiToggle`**
  - EN: Master switch for the whole MCP integration. When OFF every AI tool call is refused; when ON, normal tier handling applies (the critical-actions toggle still controls destructive calls separately).
  - ES: Interruptor principal para toda la integración MCP. En OFF, cada llamada de herramienta de IA se rechaza; en ON, se aplica la gestión normal por niveles (el interruptor de acciones críticas controla las llamadas destructivas por separado).
  - [ ] OK    Fix: 

- **`gate`**
  - EN: Allow destructive AI actions (channel-count changes, port reconfigure, runDSP, etc.) AND skip the per-call confirm handshake for less-destructive tier-2 actions while open. Acts as a superset of the Tier 2 auto-confirm toggle. The red fill drains over 5 minutes, then auto-blocks again.
  - ES: Permitir acciones de IA destructivas (cambios de número de canales, reconfiguración de puertos, runDSP, etc.) Y omitir el handshake de confirmación por llamada para acciones de tier 2 menos destructivas mientras esté abierto. Actúa como superconjunto del interruptor de auto-confirmación de Tier 2. El relleno rojo se descarga durante 5 minutos, luego se bloquea de nuevo automáticamente.
  - [ ] OK    Fix: 

- **`openHistory`**
  - EN: Open the AI History window: scrollable timeline of every recent AI change with per-row undo / redo and a step-by-step cursor.
  - ES: Abrir la ventana Historial de IA: cronología desplazable de cada cambio de IA reciente con deshacer/rehacer por fila y cursor paso a paso.
  - [ ] OK    Fix: 

- **`urlButton`**
  - EN: Click to copy the MCP server URL. Useful for Claude Code (claude mcp add wfs-diy <URL> -t http) or any MCP client that takes a URL. Claude Desktop instead uses the JSON config snippet — open the (?) help card.
  - ES: Hacer clic para copiar la URL del servidor MCP. Útil para Claude Code (claude mcp add wfs-diy <URL> -t http) o cualquier cliente MCP que acepte una URL. Claude Desktop usa en su lugar el fragmento de configuración JSON — abrir la tarjeta de ayuda (?).
  - [ ] OK    Fix: 

## `ai.undo`

- **`errorPrefix`**
  - EN: AI {verb}: {message}
  - ES: IA {verb}: {message}
  - [ ] OK    Fix: 

- **`successPrefix`**
  - EN: AI {verb}: {description}
  - ES: IA {verb}: {description}
  - [ ] OK    Fix: 

- **`verbRedo`**
  - EN: redo
  - ES: rehacer
  - [ ] OK    Fix: 

- **`verbUndo`**
  - EN: undo
  - ES: deshacer
  - [ ] OK    Fix: 

## `arrayHelper.acoustic`

- **`distanceAtten`**
  - EN: Distance Atten (%):
  - ES: Atenuación distancia (%):
  - [ ] OK    Fix: 

- **`floorReflections`**
  - EN: Floor Reflections
  - ES: Reflexiones suelo
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: HF Damping (dB/m):
  - ES: Atenuación HF (dB/m):
  - [ ] OK    Fix: 

- **`highCut`**
  - EN: High Cut (Hz):
  - ES: Corte alto (Hz):
  - [ ] OK    Fix: 

- **`hParallax`**
  - EN: H Parallax (m):
  - ES: Paralaje H (m):
  - [ ] OK    Fix: 

- **`liveSource`**
  - EN: Live Source
  - ES: Fuente en vivo
  - [ ] OK    Fix: 

- **`lowCut`**
  - EN: Low Cut (Hz):
  - ES: Corte bajo (Hz):
  - [ ] OK    Fix: 

- **`vParallax`**
  - EN: V Parallax (m):
  - ES: Paralaje V (m):
  - [ ] OK    Fix: 

## `arrayHelper.buttons`

- **`apply`**
  - EN: Apply
  - ES: Aplicar
  - [ ] OK    Fix: 

- **`close`**
  - EN: Close
  - ES: Cerrar
  - [ ] OK    Fix: 

## `arrayHelper.errors`

- **`noPositions`**
  - EN: No positions to apply. Check geometry parameters.
  - ES: No hay posiciones para aplicar. Verifique los parámetros de geometría.
  - [ ] OK    Fix: 

- **`notEnoughOutputs`**
  - EN: Not enough output channels! Need {count} starting from {start}
  - ES: ¡No hay suficientes canales de salida! Se necesitan {count} desde {start}
  - [ ] OK    Fix: 

- **`prefix`**
  - EN: Error: 
  - ES: Error: 
  - [ ] OK    Fix: 

- **`speakerCountZero`**
  - EN: Number of speakers must be greater than 0
  - ES: El número de altavoces debe ser mayor que 0
  - [ ] OK    Fix: 

## `arrayHelper.geometry`

- **`backFacing`**
  - EN: Back Facing
  - ES: Frente atrás
  - [ ] OK    Fix: 

- **`centerSpacing`**
  - EN: Center + Spacing
  - ES: Centro + Espaciado
  - [ ] OK    Fix: 

- **`centerX`**
  - EN: Center X (m):
  - ES: Centro X (m):
  - [ ] OK    Fix: 

- **`centerY`**
  - EN: Center Y (m):
  - ES: Centro Y (m):
  - [ ] OK    Fix: 

- **`endpoints`**
  - EN: Endpoints
  - ES: Puntos extremos
  - [ ] OK    Fix: 

- **`endX`**
  - EN: End X (m):
  - ES: Fin X (m):
  - [ ] OK    Fix: 

- **`endY`**
  - EN: End Y (m):
  - ES: Fin Y (m):
  - [ ] OK    Fix: 

- **`facingInward`**
  - EN: Facing Inward
  - ES: Orientado hacia adentro
  - [ ] OK    Fix: 

- **`facingOutward`**
  - EN: Facing Outward
  - ES: Orientado hacia afuera
  - [ ] OK    Fix: 

- **`frontFacing`**
  - EN: Front Facing
  - ES: Frente adelante
  - [ ] OK    Fix: 

- **`nPairs`**
  - EN: N Pairs:
  - ES: N Pares:
  - [ ] OK    Fix: 

- **`nSpeakers`**
  - EN: N Speakers:
  - ES: N Altavoces:
  - [ ] OK    Fix: 

- **`orientation`**
  - EN: Orientation (deg):
  - ES: Orientación (deg):
  - [ ] OK    Fix: 

- **`radius`**
  - EN: Radius (m):
  - ES: Radio (m):
  - [ ] OK    Fix: 

- **`sag`**
  - EN: Sag (m):
  - ES: Curvatura (m):
  - [ ] OK    Fix: 

- **`spacing`**
  - EN: Spacing (m):
  - ES: Espaciado (m):
  - [ ] OK    Fix: 

- **`startAngle`**
  - EN: Start Angle (deg):
  - ES: Ángulo inicial (deg):
  - [ ] OK    Fix: 

- **`startX`**
  - EN: Start X (m):
  - ES: Inicio X (m):
  - [ ] OK    Fix: 

- **`startY`**
  - EN: Start Y (m):
  - ES: Inicio Y (m):
  - [ ] OK    Fix: 

- **`width`**
  - EN: Width (m):
  - ES: Ancho (m):
  - [ ] OK    Fix: 

- **`yEnd`**
  - EN: Y End (m):
  - ES: Y Fin (m):
  - [ ] OK    Fix: 

- **`yStart`**
  - EN: Y Start (m):
  - ES: Y Inicio (m):
  - [ ] OK    Fix: 

- **`zHeight`**
  - EN: Z Height (m):
  - ES: Altura Z (m):
  - [ ] OK    Fix: 

## `arrayHelper.presets`

- **`circle`**
  - EN: Circle
  - ES: Círculo
  - [ ] OK    Fix: 

- **`delayLine`**
  - EN: Delay Line
  - ES: Línea de retardo
  - [ ] OK    Fix: 

- **`label`**
  - EN: Preset:
  - ES: Preajuste:
  - [ ] OK    Fix: 

- **`mainRoomStraight`**
  - EN: Main Flown Array Straight
  - ES: Array Principal Suspendido Recto
  - [ ] OK    Fix: 

- **`nearFieldCurved`**
  - EN: Near Field Array Curved
  - ES: Línea curva campo cercano
  - [ ] OK    Fix: 

- **`nearFieldStraight`**
  - EN: Near Field Array Straight
  - ES: Línea recta campo cercano
  - [ ] OK    Fix: 

- **`subBass`**
  - EN: Sub Bass
  - ES: Subgraves
  - [ ] OK    Fix: 

- **`surround`**
  - EN: Surround
  - ES: Surround
  - [ ] OK    Fix: 

## `arrayHelper.preview`

- **`audience`**
  - EN: Audience
  - ES: Audiencia
  - [ ] OK    Fix: 

## `arrayHelper.sections`

- **`acousticDefaults`**
  - EN: Acoustic Defaults
  - ES: Ajustes acústicos
  - [ ] OK    Fix: 

- **`geometry`**
  - EN: Geometry
  - ES: Geometría
  - [ ] OK    Fix: 

- **`target`**
  - EN: Target
  - ES: Destino
  - [ ] OK    Fix: 

## `arrayHelper.status`

- **`applied`**
  - EN: Applied {count} speakers to Array {array}. Ready for next array.
  - ES: {count} altavoces aplicados al Grupo {array}. Listo para el siguiente.
  - [ ] OK    Fix: 

- **`calculated`**
  - EN: Calculated {count} positions
  - ES: {count} posiciones calculadas
  - [ ] OK    Fix: 

- **`ready`**
  - EN: Ready
  - ES: Listo
  - [ ] OK    Fix: 

## `arrayHelper.target`

- **`array`**
  - EN: Array:
  - ES: Grupo:
  - [ ] OK    Fix: 

- **`arrayPrefix`**
  - EN: Array
  - ES: Grupo
  - [ ] OK    Fix: 

- **`startingOutput`**
  - EN: Starting Output:
  - ES: Salida inicial:
  - [ ] OK    Fix: 

## `arrayHelper.window`

- **`contentName`**
  - EN: Output Array Helper
  - ES: Asistente de posicionamiento de salidas
  - [ ] OK    Fix: 

- **`title`**
  - EN: Wizard of OutZ
  - ES: Asistente OutZ
  - [ ] OK    Fix: 

## `audioPatch`

- **`windowTitle`**
  - EN: Audio Interface and Patching
  - ES: Interfaz de audio y enrutamiento
  - [ ] OK    Fix: 

## `audioPatch.buttons`

- **`hold`**
  - EN: Hold
  - ES: Mantener
  - [ ] OK    Fix: 

- **`unpatchAll`**
  - EN: Unpatch All
  - ES: Desconectar todo
  - [ ] OK    Fix: 

## `audioPatch.deviceSettings`

- **`buttons.controlPanel`**
  - EN: Control Panel
  - ES: Panel de control
  - [ ] OK    Fix: 

- **`buttons.resetDevice`**
  - EN: Reset Device
  - ES: Reiniciar dispositivo
  - [ ] OK    Fix: 

- **`labels.bufferSize`**
  - EN: Audio buffer size:
  - ES: Tamaño del buffer de audio:
  - [ ] OK    Fix: 

- **`labels.device`**
  - EN: Device:
  - ES: Dispositivo:
  - [ ] OK    Fix: 

- **`labels.deviceType`**
  - EN: Audio device type:
  - ES: Tipo de dispositivo de audio:
  - [ ] OK    Fix: 

- **`labels.sampleRate`**
  - EN: Sample rate:
  - ES: Frecuencia de muestreo:
  - [ ] OK    Fix: 

- **`noDevice`**
  - EN: No Device
  - ES: Sin dispositivo
  - [ ] OK    Fix: 

- **`notConfigured`**
  - EN: Not configured
  - ES: No configurado
  - [ ] OK    Fix: 

## `audioPatch.labels`

- **`interfaceInput`**
  - EN: Audio Interface Input
  - ES: Entrada de interfaz de audio
  - [ ] OK    Fix: 

- **`interfaceOutput`**
  - EN: Audio Interface Output
  - ES: Salida de interfaz de audio
  - [ ] OK    Fix: 

- **`processorInputs`**
  - EN: Processor Inputs
  - ES: Entradas del procesador
  - [ ] OK    Fix: 

- **`processorOutputs`**
  - EN: Processor Outputs
  - ES: Salidas del procesador
  - [ ] OK    Fix: 

## `audioPatch.messages`

- **`chooseTestSignal`**
  - EN: Choose a Test Signal to Enable Testing
  - ES: Elija una señal de prueba para activar el test
  - [ ] OK    Fix: 

## `audioPatch.modes`

- **`patching`**
  - EN: Patching
  - ES: Enrutamiento
  - [ ] OK    Fix: 

- **`scrolling`**
  - EN: Scrolling
  - ES: Desplazamiento
  - [ ] OK    Fix: 

- **`testing`**
  - EN: Testing
  - ES: Prueba
  - [ ] OK    Fix: 

## `audioPatch.tabs`

- **`deviceSettings`**
  - EN: Device Settings
  - ES: Ajustes de dispositivo
  - [ ] OK    Fix: 

- **`inputPatch`**
  - EN: Input Patch
  - ES: Patch de entradas
  - [ ] OK    Fix: 

- **`outputPatch`**
  - EN: Output Patch
  - ES: Patch de salidas
  - [ ] OK    Fix: 

## `audioPatch.testSignal`

- **`labels.frequency`**
  - EN: Frequency:
  - ES: Frecuencia:
  - [ ] OK    Fix: 

- **`labels.level`**
  - EN: Level:
  - ES: Nivel:
  - [ ] OK    Fix: 

- **`labels.signal`**
  - EN: Signal:
  - ES: Señal:
  - [ ] OK    Fix: 

- **`types.diracPulse`**
  - EN: Dirac Pulse
  - ES: Impulso Dirac
  - [ ] OK    Fix: 

- **`types.off`**
  - EN: Off
  - ES: Off
  - [ ] OK    Fix: 

- **`types.pinkNoise`**
  - EN: Pink Noise
  - ES: Ruido rosa
  - [ ] OK    Fix: 

- **`types.pulse`**
  - EN: Pulse
  - ES: Impulso
  - [ ] OK    Fix: 

- **`types.sweep`**
  - EN: Sweep
  - ES: Barrido
  - [ ] OK    Fix: 

- **`types.tone`**
  - EN: Tone
  - ES: Tono
  - [ ] OK    Fix: 

## `clusters.help`

- **`attenuationSlider`**
  - EN: Adjust attenuation of all cluster inputs (dB).
  - ES: Ajustar la atenuación de todas las entradas del cluster (dB).
  - [ ] OK    Fix: 

- **`exportPresets`**
  - EN: Export all 16 LFO presets to an XML file.
  - ES: Exportar los 16 presets LFO a un archivo XML.
  - [ ] OK    Fix: 

- **`importPresets`**
  - EN: Import LFO presets from an XML file.
  - ES: Importar presets LFO desde un archivo XML.
  - [ ] OK    Fix: 

- **`inputsVisibility`**
  - EN: Show or hide this cluster's inputs on the Map tab. Hide propagates to new members; removing an input restores its visibility.
  - ES: Mostrar u ocultar las entradas de este cluster en el Mapa. Ocultar se propaga a nuevos miembros; retirar una entrada restaura su visibilidad.
  - [ ] OK    Fix: 

- **`lfoActiveButton`**
  - EN: Enable or Disable the Periodic Movement of the Cluster (LFO).
  - ES: Activar o desactivar el movimiento periódico del cluster (LFO).
  - [ ] OK    Fix: 

- **`lfoAmplitudeRotSlider`**
  - EN: Maximum Rotation Angle (-360 to 360 Degrees).
  - ES: Ángulo de rotación máximo (-360 a 360 grados).
  - [ ] OK    Fix: 

- **`lfoAmplitudeScaleSlider`**
  - EN: Maximum Scale Factor (0.1x to 10x, Logarithmic).
  - ES: Factor de escala máximo (0,1× a 10×, logarítmico).
  - [ ] OK    Fix: 

- **`lfoAmplitudeXSlider`**
  - EN: Width of Movement in Relation to Cluster Reference Position.
  - ES: Anchura del movimiento respecto a la posición de referencia del cluster.
  - [ ] OK    Fix: 

- **`lfoAmplitudeYSlider`**
  - EN: Depth of Movement in Relation to Cluster Reference Position.
  - ES: Profundidad del movimiento respecto a la posición de referencia del cluster.
  - [ ] OK    Fix: 

- **`lfoAmplitudeZSlider`**
  - EN: Height of Movement in Relation to Cluster Reference Position.
  - ES: Altura del movimiento respecto a la posición de referencia del cluster.
  - [ ] OK    Fix: 

- **`lfoPeriodDial`**
  - EN: Base Period of the Cluster Movement.
  - ES: Período base del movimiento del cluster.
  - [ ] OK    Fix: 

- **`lfoPhaseDial`**
  - EN: Global Phase Offset of the Cluster Movement.
  - ES: Desfase global del movimiento del cluster.
  - [ ] OK    Fix: 

- **`lfoPhaseRotDial`**
  - EN: Phase Offset of the Cluster Rotation.
  - ES: Desplazamiento de fase de la rotación del cluster.
  - [ ] OK    Fix: 

- **`lfoPhaseScaleDial`**
  - EN: Phase Offset of the Cluster Scaling.
  - ES: Desplazamiento de fase del escalado del cluster.
  - [ ] OK    Fix: 

- **`lfoPhaseXDial`**
  - EN: Phase Offset of the Cluster Movement in Width.
  - ES: Desplazamiento de fase del movimiento del cluster en anchura.
  - [ ] OK    Fix: 

- **`lfoPhaseYDial`**
  - EN: Phase Offset of the Cluster Movement in Depth.
  - ES: Desplazamiento de fase del movimiento del cluster en profundidad.
  - [ ] OK    Fix: 

- **`lfoPhaseZDial`**
  - EN: Phase Offset of the Cluster Movement in Height.
  - ES: Desplazamiento de fase del movimiento del cluster en altura.
  - [ ] OK    Fix: 

- **`lfoRateRotSlider`**
  - EN: Faster or Slower Rotation in Relation to Base Period.
  - ES: Rotación más rápida o lenta respecto al período base.
  - [ ] OK    Fix: 

- **`lfoRateScaleSlider`**
  - EN: Faster or Slower Scaling in Relation to Base Period.
  - ES: Escalado más rápido o lento respecto al período base.
  - [ ] OK    Fix: 

- **`lfoRateXSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Width.
  - ES: Movimiento más rápido o lento respecto al período base, en anchura.
  - [ ] OK    Fix: 

- **`lfoRateYSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Depth.
  - ES: Movimiento más rápido o lento respecto al período base, en profundidad.
  - [ ] OK    Fix: 

- **`lfoRateZSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Height.
  - ES: Movimiento más rápido o lento respecto al período base, en altura.
  - [ ] OK    Fix: 

- **`lfoShapeRotSelector`**
  - EN: Rotation Behaviour of the Cluster.
  - ES: Comportamiento de rotación del cluster.
  - [ ] OK    Fix: 

- **`lfoShapeScaleSelector`**
  - EN: Scale Behaviour of the Cluster.
  - ES: Comportamiento de escala del cluster.
  - [ ] OK    Fix: 

- **`lfoShapeXSelector`**
  - EN: Movement Behaviour of the Cluster in Width.
  - ES: Comportamiento del movimiento del cluster en anchura.
  - [ ] OK    Fix: 

- **`lfoShapeYSelector`**
  - EN: Movement Behaviour of the Cluster in Depth.
  - ES: Comportamiento del movimiento del cluster en profundidad.
  - [ ] OK    Fix: 

- **`lfoShapeZSelector`**
  - EN: Movement Behaviour of the Cluster in Height.
  - ES: Comportamiento del movimiento del cluster en altura.
  - [ ] OK    Fix: 

- **`planeSelector`**
  - EN: Select the plane for rotation and scale operations (XY, XZ, YZ).
  - ES: Seleccionar el plano para las operaciones de rotación y escala (XY, XZ, YZ).
  - [ ] OK    Fix: 

- **`positionJoystick`**
  - EN: Move all cluster inputs in X/Y. Hold and drag to translate.
  - ES: Mover todas las entradas del cluster en X/Y. Mantener y arrastrar para trasladar.
  - [ ] OK    Fix: 

- **`presetTile`**
  - EN: Click: recall preset. Double-click: recall + start. Middle/right-click: store current LFO.
  - ES: Clic: recuperar preset. Doble clic: recuperar + iniciar. Clic medio/derecho: guardar el LFO actual.
  - [ ] OK    Fix: 

- **`qlabPreset`**
  - EN: Create a QLab network cue to recall the last selected LFO preset for the current cluster.
  - ES: Crear una cue de red QLab para recuperar el último preset LFO seleccionado para el cluster actual.
  - [ ] OK    Fix: 

- **`referenceMode`**
  - EN: Select the reference point for cluster transforms: First Input or Barycenter.
  - ES: Seleccione el punto de referencia para las transformaciones del cluster: Primera Entrada o Baricentro.
  - [ ] OK    Fix: 

- **`rotationDial`**
  - EN: Rotate all cluster inputs around the reference point in the selected plane.
  - ES: Rotar todas las entradas del cluster alrededor del punto de referencia en el plano seleccionado.
  - [ ] OK    Fix: 

- **`scaleJoystick`**
  - EN: Scale cluster inputs relative to the reference point in the selected plane.
  - ES: Escalar las entradas del cluster respecto al punto de referencia en el plano seleccionado.
  - [ ] OK    Fix: 

- **`stopAllLFO`**
  - EN: Stop LFO on all 10 clusters.
  - ES: Detener el LFO en los 10 clusters.
  - [ ] OK    Fix: 

- **`zSlider`**
  - EN: Move all cluster inputs along the Z axis (height).
  - ES: Mover todas las entradas del cluster a lo largo del eje Z (altura).
  - [ ] OK    Fix: 

## `clusters.labels`

- **`assignedInputs`**
  - EN: Assigned Inputs
  - ES: Entradas asignadas
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Atten
  - ES: Atenuar
  - [ ] OK    Fix: 

- **`controls`**
  - EN: Controls
  - ES: Controles
  - [ ] OK    Fix: 

- **`inputPrefix`**
  - EN: Input
  - ES: Entrada
  - [ ] OK    Fix: 

- **`position`**
  - EN: Position
  - ES: Posición
  - [ ] OK    Fix: 

- **`posPrefix`**
  - EN: Pos:
  - ES: Pos:
  - [ ] OK    Fix: 

- **`reference`**
  - EN: Reference:
  - ES: Referencia:
  - [ ] OK    Fix: 

- **`rotation`**
  - EN: Rotation
  - ES: Rotación
  - [ ] OK    Fix: 

- **`scale`**
  - EN: Scale
  - ES: Escala
  - [ ] OK    Fix: 

- **`x`**
  - EN: X:
  - ES: X:
  - [ ] OK    Fix: 

- **`y`**
  - EN: Y:
  - ES: Y:
  - [ ] OK    Fix: 

- **`z`**
  - EN: Z
  - ES: Z
  - [ ] OK    Fix: 

## `clusters.lfo`

- **`labels.amplitude`**
  - EN: Amplitude:
  - ES: Amplitud:
  - [ ] OK    Fix: 

- **`labels.angle`**
  - EN: Angle:
  - ES: Ángulo:
  - [ ] OK    Fix: 

- **`labels.period`**
  - EN: Period:
  - ES: Período:
  - [ ] OK    Fix: 

- **`labels.phase`**
  - EN: Phase:
  - ES: Fase:
  - [ ] OK    Fix: 

- **`labels.rate`**
  - EN: Rate:
  - ES: Velocidad:
  - [ ] OK    Fix: 

- **`labels.ratio`**
  - EN: Ratio:
  - ES: Ratio:
  - [ ] OK    Fix: 

- **`labels.rot`**
  - EN: Rotation
  - ES: Rotación
  - [ ] OK    Fix: 

- **`labels.scaleLfo`**
  - EN: Scale
  - ES: Escala
  - [ ] OK    Fix: 

- **`labels.section`**
  - EN: LFO
  - ES: LFO
  - [ ] OK    Fix: 

- **`labels.x`**
  - EN: X
  - ES: X
  - [ ] OK    Fix: 

- **`labels.y`**
  - EN: Y
  - ES: Y
  - [ ] OK    Fix: 

- **`labels.z`**
  - EN: Z
  - ES: Z
  - [ ] OK    Fix: 

## `clusters.osc`

- **`lfoActive`**
  - EN: /wfs/cluster/lfoActive <id> <0|1>
  - ES: /wfs/cluster/lfoActive <id> <0|1>
  - [ ] OK    Fix: 

- **`lfoAmplitudeRot`**
  - EN: /wfs/cluster/lfoAmplitudeRot <id> <degrees>
  - ES: /wfs/cluster/lfoAmplitudeRot <id> <grados>
  - [ ] OK    Fix: 

- **`lfoAmplitudeScale`**
  - EN: /wfs/cluster/lfoAmplitudeScale <id> <factor>
  - ES: /wfs/cluster/lfoAmplitudeScale <id> <factor>
  - [ ] OK    Fix: 

- **`lfoAmplitudeX`**
  - EN: /wfs/cluster/lfoAmplitudeX <id> <meters>
  - ES: /wfs/cluster/lfoAmplitudeX <id> <metros>
  - [ ] OK    Fix: 

- **`lfoAmplitudeY`**
  - EN: /wfs/cluster/lfoAmplitudeY <id> <meters>
  - ES: /wfs/cluster/lfoAmplitudeY <id> <metros>
  - [ ] OK    Fix: 

- **`lfoAmplitudeZ`**
  - EN: /wfs/cluster/lfoAmplitudeZ <id> <meters>
  - ES: /wfs/cluster/lfoAmplitudeZ <id> <metros>
  - [ ] OK    Fix: 

- **`lfoPeriod`**
  - EN: /wfs/cluster/lfoPeriod <id> <seconds>
  - ES: /wfs/cluster/lfoPeriod <id> <segundos>
  - [ ] OK    Fix: 

- **`lfoPhase`**
  - EN: /wfs/cluster/lfoPhase <id> <degrees>
  - ES: /wfs/cluster/lfoPhase <id> <grados>
  - [ ] OK    Fix: 

- **`lfoPhaseRot`**
  - EN: /wfs/cluster/lfoPhaseRot <id> <degrees>
  - ES: /wfs/cluster/lfoPhaseRot <id> <grados>
  - [ ] OK    Fix: 

- **`lfoPhaseScale`**
  - EN: /wfs/cluster/lfoPhaseScale <id> <degrees>
  - ES: /wfs/cluster/lfoPhaseScale <id> <grados>
  - [ ] OK    Fix: 

- **`lfoPhaseX`**
  - EN: /wfs/cluster/lfoPhaseX <id> <degrees>
  - ES: /wfs/cluster/lfoPhaseX <id> <grados>
  - [ ] OK    Fix: 

- **`lfoPhaseY`**
  - EN: /wfs/cluster/lfoPhaseY <id> <degrees>
  - ES: /wfs/cluster/lfoPhaseY <id> <grados>
  - [ ] OK    Fix: 

- **`lfoPhaseZ`**
  - EN: /wfs/cluster/lfoPhaseZ <id> <degrees>
  - ES: /wfs/cluster/lfoPhaseZ <id> <grados>
  - [ ] OK    Fix: 

- **`lfoPresetRecall`**
  - EN: /wfs/cluster/lfoPresetRecall <clusterId> <presetNumber>
  - ES: /wfs/cluster/lfoPresetRecall <clusterId> <numeroPreset>
  - [ ] OK    Fix: 

- **`lfoRateRot`**
  - EN: /wfs/cluster/lfoRateRot <id> <multiplier>
  - ES: /wfs/cluster/lfoRateRot <id> <multiplicador>
  - [ ] OK    Fix: 

- **`lfoRateScale`**
  - EN: /wfs/cluster/lfoRateScale <id> <multiplier>
  - ES: /wfs/cluster/lfoRateScale <id> <multiplicador>
  - [ ] OK    Fix: 

- **`lfoRateX`**
  - EN: /wfs/cluster/lfoRateX <id> <multiplier>
  - ES: /wfs/cluster/lfoRateX <id> <multiplicador>
  - [ ] OK    Fix: 

- **`lfoRateY`**
  - EN: /wfs/cluster/lfoRateY <id> <multiplier>
  - ES: /wfs/cluster/lfoRateY <id> <multiplicador>
  - [ ] OK    Fix: 

- **`lfoRateZ`**
  - EN: /wfs/cluster/lfoRateZ <id> <multiplier>
  - ES: /wfs/cluster/lfoRateZ <id> <multiplicador>
  - [ ] OK    Fix: 

- **`lfoShapeRot`**
  - EN: /wfs/cluster/lfoShapeRot <id> <0-8>
  - ES: /wfs/cluster/lfoShapeRot <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoShapeScale`**
  - EN: /wfs/cluster/lfoShapeScale <id> <0-8>
  - ES: /wfs/cluster/lfoShapeScale <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoShapeX`**
  - EN: /wfs/cluster/lfoShapeX <id> <0-8>
  - ES: /wfs/cluster/lfoShapeX <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoShapeY`**
  - EN: /wfs/cluster/lfoShapeY <id> <0-8>
  - ES: /wfs/cluster/lfoShapeY <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoShapeZ`**
  - EN: /wfs/cluster/lfoShapeZ <id> <0-8>
  - ES: /wfs/cluster/lfoShapeZ <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoStopAll`**
  - EN: /wfs/cluster/lfoStopAll
  - ES: /wfs/cluster/lfoStopAll
  - [ ] OK    Fix: 

## `clusters.planes`

- **`xy`**
  - EN: XY
  - ES: XY
  - [ ] OK    Fix: 

- **`xz`**
  - EN: XZ
  - ES: XZ
  - [ ] OK    Fix: 

- **`yz`**
  - EN: YZ
  - ES: YZ
  - [ ] OK    Fix: 

## `clusters.presets`

- **`export`**
  - EN: Export
  - ES: Exportar
  - [ ] OK    Fix: 

- **`exportDialog`**
  - EN: Export LFO Presets
  - ES: Exportar presets LFO
  - [ ] OK    Fix: 

- **`exported`**
  - EN: LFO presets exported.
  - ES: Presets LFO exportados.
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - ES: Importar
  - [ ] OK    Fix: 

- **`importDialog`**
  - EN: Import LFO Presets
  - ES: Importar presets LFO
  - [ ] OK    Fix: 

- **`imported`**
  - EN: LFO presets imported.
  - ES: Presets LFO importados.
  - [ ] OK    Fix: 

- **`recalled`**
  - EN: LFO preset recalled from tile {n}.
  - ES: Preset LFO recuperado de la casilla {n}.
  - [ ] OK    Fix: 

- **`stopAll`**
  - EN: Stop All
  - ES: Detener todo
  - [ ] OK    Fix: 

- **`stored`**
  - EN: LFO preset stored in tile {n}.
  - ES: Preset LFO guardado en la casilla {n}.
  - [ ] OK    Fix: 

## `clusters.referenceMode`

- **`barycenter`**
  - EN: Barycenter
  - ES: Baricentro
  - [ ] OK    Fix: 

- **`firstInput`**
  - EN: First Input
  - ES: Primera entrada
  - [ ] OK    Fix: 

## `clusters.status`

- **`noInputs`**
  - EN: No inputs assigned
  - ES: Sin entradas asignadas
  - [ ] OK    Fix: 

- **`trackedMarker`**
  - EN: [T]
  - ES: [S]
  - [ ] OK    Fix: 

- **`tracking`**
  - EN: Tracking: Input {num} (overrides reference)
  - ES: Seguimiento: Entrada {num} (anula referencia)
  - [ ] OK    Fix: 

## `clusters.toggles`

- **`inputsHidden`**
  - EN: Inputs: Hidden
  - ES: Entradas: Ocultas
  - [ ] OK    Fix: 

- **`inputsVisible`**
  - EN: Inputs: Visible
  - ES: Entradas: Visibles
  - [ ] OK    Fix: 

- **`lfoOff`**
  - EN: L.F.O: OFF
  - ES: L.F.O: APAG.
  - [ ] OK    Fix: 

- **`lfoOn`**
  - EN: L.F.O: ON
  - ES: L.F.O: ENC.
  - [ ] OK    Fix: 

## `common`

- **`cancel`**
  - EN: Cancel
  - ES: Cancelar
  - [ ] OK    Fix: 

- **`off`**
  - EN: OFF
  - ES: OFF
  - [ ] OK    Fix: 

- **`ok`**
  - EN: OK
  - ES: OK
  - [ ] OK    Fix: 

- **`on`**
  - EN: ON
  - ES: ON
  - [ ] OK    Fix: 

## `eq.buttons`

- **`flattenEQ`**
  - EN: Flatten EQ
  - ES: Restablecer EQ
  - [ ] OK    Fix: 

- **`resetBand`**
  - EN: Reset
  - ES: Restablecer
  - [ ] OK    Fix: 

## `eq.filterTypes`

- **`allPass`**
  - EN: All-Pass
  - ES: Pasa-todo
  - [ ] OK    Fix: 

- **`bandPass`**
  - EN: Band Pass
  - ES: Paso de banda
  - [ ] OK    Fix: 

- **`highCut`**
  - EN: High Cut
  - ES: Corte alto
  - [ ] OK    Fix: 

- **`highShelf`**
  - EN: High Shelf
  - ES: Estante alto
  - [ ] OK    Fix: 

- **`lowCut`**
  - EN: Low Cut
  - ES: Corte bajo
  - [ ] OK    Fix: 

- **`lowShelf`**
  - EN: Low Shelf
  - ES: Estante bajo
  - [ ] OK    Fix: 

- **`peakNotch`**
  - EN: Peak/Notch
  - ES: Pico/Muesca
  - [ ] OK    Fix: 

## `eq.labels`

- **`band`**
  - EN: Band
  - ES: Banda
  - [ ] OK    Fix: 

- **`freq`**
  - EN: Freq:
  - ES: Frec:
  - [ ] OK    Fix: 

- **`gain`**
  - EN: Gain
  - ES: Ganancia
  - [ ] OK    Fix: 

- **`q`**
  - EN: Q
  - ES: Q
  - [ ] OK    Fix: 

## `eq.status`

- **`off`**
  - EN: EQ OFF
  - ES: EQ DESACTIVADO
  - [ ] OK    Fix: 

- **`on`**
  - EN: EQ ON
  - ES: EQ ACTIVADO
  - [ ] OK    Fix: 

## `fileManager.dialogs`

- **`selectProjectFolder`**
  - EN: Select Project Folder
  - ES: Seleccionar carpeta de proyecto
  - [ ] OK    Fix: 

## `fileManager.errors`

- **`backupNotFound`**
  - EN: Backup not found
  - ES: Copia de seguridad no encontrada
  - [ ] OK    Fix: 

- **`configStateInvalid`**
  - EN: Config state is invalid
  - ES: Estado de configuración no válido
  - [ ] OK    Fix: 

- **`failedApply`**
  - EN: Failed to apply: {sections}
  - ES: No se pudo aplicar: {sections}
  - [ ] OK    Fix: 

- **`failedCreateFolder`**
  - EN: Failed to create project folder: {path}
  - ES: No se pudo crear la carpeta de proyecto: {path}
  - [ ] OK    Fix: 

- **`failedCreateValueTree`**
  - EN: Failed to create ValueTree from XML: {path}
  - ES: No se pudo crear el ValueTree desde XML: {path}
  - [ ] OK    Fix: 

- **`failedCreateXML`**
  - EN: Failed to create XML from state
  - ES: No se pudo crear el XML a partir del estado
  - [ ] OK    Fix: 

- **`failedParseXML`**
  - EN: Failed to parse XML file: {path}
  - ES: No se pudo analizar el archivo XML: {path}
  - [ ] OK    Fix: 

- **`failedWriteFile`**
  - EN: Failed to write file: {path}
  - ES: No se pudo escribir el archivo: {path}
  - [ ] OK    Fix: 

- **`fileNotFound`**
  - EN: File not found: {path}
  - ES: Archivo no encontrado: {path}
  - [ ] OK    Fix: 

- **`invalidConfigStructure`**
  - EN: Invalid configuration file structure
  - ES: Estructura de archivo de configuración no válida
  - [ ] OK    Fix: 

- **`noInputDataInFile`**
  - EN: No input data found in file
  - ES: No se encontraron datos de entrada en el archivo
  - [ ] OK    Fix: 

- **`noInputDataInSnapshot`**
  - EN: No input data in snapshot
  - ES: No hay datos de entrada en el snapshot
  - [ ] OK    Fix: 

- **`noLFOPresetDataInFile`**
  - EN: No LFO preset data found in file
  - ES: No se encontraron datos de preset LFO en el archivo
  - [ ] OK    Fix: 

- **`noNetworkDataInFile`**
  - EN: No network data found in file
  - ES: No se encontraron datos de red en el archivo
  - [ ] OK    Fix: 

- **`noNetworkSections`**
  - EN: No network sections found in file
  - ES: No se encontraron secciones de red en el archivo
  - [ ] OK    Fix: 

- **`noOutputDataInFile`**
  - EN: No output data found in file
  - ES: No se encontraron datos de salida en el archivo
  - [ ] OK    Fix: 

- **`noProjectFolder`**
  - EN: No project folder specified
  - ES: No se ha especificado ninguna carpeta de proyecto
  - [ ] OK    Fix: 

- **`noReverbDataInFile`**
  - EN: No reverb data found in file
  - ES: No se encontraron datos de reverberación en el archivo
  - [ ] OK    Fix: 

- **`noSystemDataInFile`**
  - EN: No valid system data found in file: {path}
  - ES: No se encontraron datos de sistema válidos en el archivo: {path}
  - [ ] OK    Fix: 

- **`noValidProjectFolder`**
  - EN: No valid project folder
  - ES: No hay una carpeta de proyecto válida
  - [ ] OK    Fix: 

- **`prefixInputs`**
  - EN: Inputs: 
  - ES: Entradas: 
  - [ ] OK    Fix: 

- **`prefixNetwork`**
  - EN: Network: 
  - ES: Red: 
  - [ ] OK    Fix: 

- **`prefixOutputs`**
  - EN: Outputs: 
  - ES: Salidas: 
  - [ ] OK    Fix: 

- **`prefixReverbs`**
  - EN: Reverbs: 
  - ES: Reverberaciones: 
  - [ ] OK    Fix: 

- **`prefixSystem`**
  - EN: System: 
  - ES: Sistema: 
  - [ ] OK    Fix: 

- **`snapshotNotFound`**
  - EN: Snapshot not found
  - ES: Snapshot no encontrado
  - [ ] OK    Fix: 

- **`snapshotNotFoundNamed`**
  - EN: Snapshot not found: {name}
  - ES: Snapshot no encontrado: {name}
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
  - ES: ADM-OSC es un protocolo que busca mejorar la interoperabilidad del sonido espacial. Envía posiciones cartesianas (X, Y, Z) o valores polares (AED para Azimut, Elevación, Distancia) desde la consola o las curvas de automatización de un DAW.
Los datos se envían normalizados:
- entre -1.0 y 1.0 para X, Y y Z;
- entre 0.0 y 1.0 para distancia,
- entre -180° y 180° para Azimut
- entre -90° y 90° para elevación.
El punto de origen puede moverse y el mapeo también puede ajustarse en diferentes segmentos para las partes interior y exterior del escenario.
Al arrastrar los controles en los gráficos, mantener la tecla Mayús aplica ajustes simétricos en el lado opuesto.
  - [ ] OK    Fix: 

- **`title`**
  - EN: ADM-OSC Mappings
  - ES: Mapeos ADM-OSC
  - [ ] OK    Fix: 

## `help.automOtion`

- **`body`**
  - EN: One time movement can also be programmed and triggered manually or triggered by sound level.
The coordinates are either relative from the start position or absolute relative to the origin point.
The input can either stay at the end position or revert to the starting position.
Input position cannot be changed while the input is moving this way, but interacting with the input will change the position offset of the input instead.
For audio level triggering select the sound level above which the movement will start. When the sound drops below the reset level the movement will be rearmed, ready to be triggered again. New movement automation will not be possible while the input is moving.
  - ES: Movimientos únicos pueden programarse y dispararse manualmente o por nivel de sonido.
Las coordenadas son relativas desde la posición inicial o absolutas respecto al punto de origen.
La entrada puede quedarse en la posición final o volver a la posición inicial.
La posición no puede cambiarse durante el movimiento, pero la interacción cambiará el desplazamiento de posición.
Para disparo por nivel de audio, seleccione el umbral. Cuando el sonido baje del nivel de reinicio, el movimiento se rearmará.
  - [ ] OK    Fix: 

- **`title`**
  - EN: AutomOtion
  - ES: AutomOtion
  - [ ] OK    Fix: 

## `help.binaural`

- **`body`**
  - EN: The Binaural renderer is used for:
- listening to a rough spatial mix on headphones,
- creating a mix for stereo output,
- listening to a single soloed track through the spatial processing.
This may take the place of your master mix if it's only feeding headphones and media mix.
The position of the listening position may be adjusted in depth from the origin point and in orientation. Delay and level settings allow you to eventually match the sound at the FOH position.
  - ES: El renderizador binaural se utiliza para:
- escuchar una mezcla espacial aproximada con auriculares,
- crear una mezcla para salida estéreo,
- escuchar una pista en solo a través del procesamiento espacial.
Puede sustituir su mezcla master si solo alimenta auriculares y mezcla de medios.
La posición de escucha puede ajustarse en profundidad desde el punto de origen y en orientación. Los ajustes de retardo y nivel permiten igualar el sonido en la posición FOH.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Binaural Renderer
  - ES: Renderizador Binaural
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
  - ES: Los clusters son grupos de entradas que pueden manipularse y animarse como un conjunto.
Cada entrada solo puede pertenecer a un cluster.
Cada cluster solo puede tener una entrada con tracking completamente habilitado, que se convierte en el punto de referencia.
Si no hay entrada con tracking, hay dos modos de referencia: la primera entrada asignada o el baricentro de las entradas asignadas.
Todas las entradas pueden moverse arrastrando el punto de referencia. Las entradas individuales pueden ajustarse por separado. Arrastrar una entrada con tracking activado que también es punto de referencia afectará su desplazamiento de posición y la posición de las otras entradas del cluster normalmente.
Todas las entradas de un cluster pueden rotarse o escalarse alrededor del punto de referencia.
Todos los clusters pueden recibir una animación via LFO. Las posiciones X, Y, Z, la rotación y la escala pueden controlarse. Los ajustes LFO pueden asignarse a pads. Un clic derecho almacenará los parámetros LFO en un pad. Doble clic en la parte superior del pad permite editar el nombre del preset. Un clic o toque recuerda los ajustes esté o no el LFO en marcha, pero no lo iniciará. Un doble clic/toque cargará e iniciará el LFO.
Todos los clusters comparten el mismo conjunto de presets LFO.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Clusters
  - ES: Clusters
  - [ ] OK    Fix: 

## `help.floorReflections`

- **`body`**
  - EN: Simulating the floor reflections improves the naturalness of the sound. We don't expect sounds to play in a soundproof anechoic chamber. This setting helps recreate the expected floor reflections.
The level of the floor reflections can be adjusted as well as the low cut and the high frequency shelf filters. Diffusion adds a bit of chaos to simulate the unevenness of the floor.
  - ES: Simular las reflexiones del suelo mejora la naturalidad del sonido. No esperamos que los sonidos se reproduzcan en una cámara anecoica insonorizada. Este ajuste ayuda a recrear las reflexiones del suelo esperadas.
El nivel de las reflexiones del suelo puede ajustarse, así como los filtros de corte bajo y de estante de altas frecuencias. La difusión añade un poco de caos para simular las irregularidades del suelo.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Floor Reflections
  - ES: Reflexiones del Suelo
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
  - ES: Los mapas de gradiente permiten controlar la atenuación, la altura y el filtrado de alta frecuencia (shelf con pendiente suave centrada en 1kHz) según la posición X, Y. Por ejemplo, puede atenuar un sonido al entrar en una zona, aplicar roll-off de altas frecuencias al alejarse del frente del escenario o ajustar automáticamente la altura de un actor en plataformas elevadas.
Hay tres capas: atenuación, altura y shelf HF. Pueden activarse/desactivarse y ocultarse.
Cada capa tiene controles de mapeo blanco y negro para ajustar el rango del efecto. El ajuste de curva controla la transición.
Cada capa puede tener formas editables (rectángulo, elipse o polígono) con gris uniforme, gradiente lineal o radial.
Al crear un polígono, haga clic para cada esquina. Doble clic cierra la forma.
Doble clic en un punto lo elimina. Doble clic en un lado añade un punto.
La escala y rotación pueden editarse desde el centro o desde el origen.
Formas y capas pueden copiarse a otra capa.
Los ajustes se almacenan en los archivos de entrada.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Gradient Maps
  - ES: Mapas de Gradiente
  - [ ] OK    Fix: 

## `help.inputAdvanced`

- **`body`**
  - EN: - Sidelines and Fringe enable muting when an input comes close to the limits of a rectangular stage (except downstage).
- Tracking can be enabled and the ID of the tracker can be selected. The amount of smoothing on the position can also be adjusted.
- Maximum Speed can be engaged and the speed limit adjusted. The system will apply a gradual acceleration and deceleration when the movement starts and finishes. When Path is enabled, the system will track the path followed by the input and will not go in a straight line to the final position. This is especially handy if movements have to be operated manually.
- Height Factor lets you work in 2D, when set to 0%, or full 3D, when set at 100%, and anything in between. It's the ratio of the height in the level and delay calculations. If you wish to use floor reflections, set it to 100% and use the parallax correction of the output parameters.
  - ES: - Las líneas laterales y la zona de transición permiten silenciar cuando una entrada se acerca a los bordes de un escenario rectangular (excepto el borde del público).
- El Tracking puede activarse y seleccionarse el ID del tracker. El suavizado de la posición también puede ajustarse.
- La Velocidad Máxima puede activarse y ajustarse el límite de velocidad. El sistema aplicará una aceleración y desaceleración graduales al inicio y al final del movimiento. Cuando el modo Trayectoria está activo, el sistema seguirá el camino tomado por la entrada en lugar de ir en línea recta a la posición final. Esto es especialmente útil si los movimientos deben operarse manualmente.
- El Factor de Altura permite trabajar en 2D, ajustado al 0%, o en 3D completo, ajustado al 100%, y todo lo intermedio. Es la proporción de la altura en los cálculos de nivel y retardo. Si desea usar reflexiones de suelo, ajústelo al 100% y use la corrección de paralaje en los parámetros de salida.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Advanced Controls
  - ES: Controles avanzados
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
  - ES: Las entradas disponen de una gran variedad de ajustes para adaptarse a diferentes situaciones que requieren sonorización realista o herramientas creativas para diseño sonoro.
- El nivel de entrada puede ajustarse.
- Las entradas pueden retrasarse o intentar tener en cuenta una latencia específica (procesamiento digital de transmisión inalámbrica o efectos digitales) y compensarla para alinear mejor la amplificación y el sonido acústico.
- La Latencia Mínima puede activarse en lugar de la Precedencia Acústica. Esto intenta sacar el sonido a través del sistema lo más rápido posible. El sistema escanea los envíos de esta entrada a las salidas buscando el menor retardo y lo sustrae de todos los retardos, evitando el efecto Haas adicional.
- La posición (posición y offset) puede darse en coordenadas Cartesianas, Cilíndricas o Esféricas independientemente de la forma del escenario u otros canales.
- La posición puede restringirse a las dimensiones del escenario en coordenadas Cartesianas o a un rango de distancia específico en coordenadas polares.
- Flip tomará la posición simétrica para la coordenada dada alrededor del punto de origen.
- El joystick y el deslizador vertical permiten control relativo de la posición.
- Las entradas pueden asignarse a un cluster para agruparlas en movimientos coordinados.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Inputs Basic Parameters
  - ES: Parámetros básicos de entradas
  - [ ] OK    Fix: 

## `help.inputHF`

- **`body`**
  - EN: When talking away, the timbre of a voice will sound less bright. Reproducing this was the initial goal here even though we usually want to have support for voices when not addressing the audience or in bi-frontal configurations. This can be put to good use for creative effects such as having a brighter reverb on a dampened direct sound.
The orientation of the input in azimuth and in pitch can be set as well as the angle where the high frequencies will not be filtered.
The HF Shelf will set the maximum attenuation in the back of the input. There is a smooth fade (like a cosine curve) from full brightness in front to damped at the rear.
  - ES: Al hablar girándose, el timbre de una voz suena menos brillante. Reproducir esto era el objetivo inicial aquí, aunque generalmente queremos tener soporte para voces que no se dirigen al público o en configuraciones bi-frontales. Esto puede usarse para efectos creativos como tener una reverberación más brillante sobre un sonido directo atenuado.
La orientación de la entrada en azimut y en elevación puede ajustarse así como el ángulo donde las altas frecuencias no se filtrarán.
El HF Shelf establecerá la atenuación máxima en la parte trasera de la entrada. Hay un fundido suave (como una curva coseno) desde brillo pleno delante hasta atenuado detrás.
  - [ ] OK    Fix: 

- **`title`**
  - EN: High Frequency Directivity
  - ES: Directividad de alta frecuencia
  - [ ] OK    Fix: 

## `help.inputLevel`

- **`body`**
  - EN: There are two level attenuation models. One where the level decreases with the distance by a given ratio in dB/m. Otherwise the level is halved each time the distance doubles. This might be more realistic, but it may be too loud close to the source or not give enough focus. The former may be less physically accurate, it offers usually better control to have a more even and stable mix.
  - ES: Existen dos modelos de atenuación de nivel. Uno donde el nivel disminuye con la distancia según una relación dada en dB/m. Alternativamente el nivel se reduce a la mitad cada vez que la distancia se duplica. Este último puede ser más realista, pero puede ser demasiado fuerte cerca de la fuente o no dar suficiente enfoque. El primero puede ser menos preciso físicamente, pero generalmente ofrece mejor control para una mezcla más uniforme y estable.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Level Adjustments
  - ES: Ajustes de nivel
  - [ ] OK    Fix: 

## `help.inputMutes`

- **`body`**
  - EN: You can set for each output array a specific attenuation for the selected input.
You can mute each send to any output individually. There are macros to speed up the process.
  - ES: Puede establecer para cada array de salida una atenuación específica para la entrada seleccionada.
Puede silenciar cada envío a cualquier salida individualmente. Hay macros disponibles para acelerar el proceso.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Array Attenuation and Output Mutes
  - ES: Atenuación por array y mutes de salida
  - [ ] OK    Fix: 

## `help.lfo`

- **`body`**
  - EN: Input position can be automated. The LFO can control X, Y and Z position coordinates individually as well as the rotation of HF directivity (gyrophone).
Adjust the global period and phase for the LFO.
For X, Y and Z coordinates select a shape, amplitude, rate and phase as desired. A circle in the XY plane would have sine for shape for X and for Y and an offset of ±90° between the two. A square would be the same but with keystone shapes.
Input position can be moved while the LFO is running.
  - ES: La posición de entrada puede automatizarse. El LFO puede controlar las coordenadas X, Y y Z individualmente así como la rotación de la directividad HF (girófono).
Ajuste el período y la fase globales del LFO.
Para X, Y y Z seleccione forma, amplitud, tasa y fase. Un círculo en el plano XY usaría forma sinusoidal para X e Y con ±90° de desfase. Un cuadrado sería igual pero con formas trapezoidales.
La posición puede moverse mientras el LFO está funcionando.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Input LFO
  - ES: LFO de Entrada
  - [ ] OK    Fix: 

## `help.liveSource`

- **`body`**
  - EN: A loud source on stage may not need much reinforcement through the nearby speakers. Imagine an opera singer near the edge of the stage. Normally the level distribution would make the level louder near the input position. But if it is already loud enough we should be able to not over-amplify it. This function manages this.
The radius and shape describe how to attenuate the level for a specific source for speakers within the radius of influence of this input. There are various shapes: a V shaped linear effect; a U shape for fast decrease within the radius; a tight V effect or a mix of the previous (sine).
The attenuation can be constant or level dependent like a local compression reacting to transients and to average RMS level.
  - ES: Una fuente potente en el escenario puede no necesitar refuerzo a través de los altavoces cercanos. Imagine un cantante de ópera cerca del borde del escenario. Normalmente la distribución de nivel haría el sonido más fuerte cerca de la posición de entrada. Pero si ya es suficientemente fuerte, no deberíamos sobre-amplificarlo. Esta función gestiona esto.
El radio y la forma describen cómo atenuar el nivel para los altavoces dentro del radio de influencia de esta fuente. Hay varias formas: un efecto lineal en V; una U para disminución rápida; una V ajustada o una mezcla de los anteriores (seno).
La atenuación puede ser constante o dependiente del nivel, como una compresión local que reacciona a transitorios y al nivel RMS promedio.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Live Source Tamer
  - ES: Atenuador de Fuente en Vivo
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
  - ES: - Clic izquierdo en una entrada o cluster permite moverlo arrastrando.
- Clic izquierdo con Mayús añade o quita entradas de la selección.
- Clic izquierdo arrastrado dibuja un rectángulo de selección.
- Doble clic restablece el desplazamiento de posición.
- Clic largo sin movimiento cambia a la pestaña de la entrada seleccionada.
- Clic fuera de cualquier entrada limpia la selección.
- Clic derecho arrastrado desplaza la vista del mapa. Arrastre con dos dedos también.
- La rueda del ratón hace zoom. Pellizco con dos dedos también.
- Clic del botón central restablece la vista.
- Las flechas mueven X/Y, RePág/AvPág la altura.
- Un segundo dedo puede rotar la directividad y ajustar la altura.
- En clusters, un segundo dedo puede rotar y escalar.
- Las entradas, arrays de salida y nodos de reverberación pueden ocultarse.
- Las entradas pueden bloquearse.
- Los nodos de reverberación pueden moverse. Ctrl/Cmd mueve pares en simetría.
- El radio del Live Source Tamer se muestra cuando está activado.
- Los niveles de audio pueden mostrarse en el mapa.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Map
  - ES: Mapa
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
  - ES: El servidor MCP permite que un asistente de IA (Claude Desktop, Claude Code, ChatGPT con conectores personalizados) lea y escriba los parámetros de esta sesión de WFS-DIY a través de una conexión de red local.

Lo que la IA puede hacer:
• Leer el estado en vivo: número de canales, nombres, posiciones, atenuaciones, EQ, snapshots, clusters, toda la superficie de parámetros.
• Mover fuentes, renombrar canales, establecer asignaciones de clusters, ajustar el diseño de arrays, colocar salidas y reverberaciones.
• Ejecutar flujos guiados (asistentes de afinación de sistema, resolución de localización, gestión de snapshots) mediante plantillas de prompts preparadas.

Controles del operador en esta fila:
• IA: ON / OFF — interruptor principal. En OFF, cada llamada de herramienta de IA se rechaza; en ON, la IA trabaja según las reglas siguientes.
• Acciones críticas de IA: bloqueadas / PERMITIDAS — las acciones destructivas (eliminar snapshots, restablecer DSP, cambiar el número de canales) están bloqueadas por defecto. Hacer clic para permitirlas durante 10 minutos; el relleno rojo se descarga al expirar la ventana, luego se bloquean automáticamente de nuevo.
• Abrir Historial de IA — cronología desplazable de cada cambio de IA reciente con deshacer/rehacer por fila y cursor paso a paso.
• El botón URL MCP copia la URL del servidor al portapapeles para clientes de IA que acepten una URL directamente.

Conciencia del operador:
• Cada acción de IA se registra con etiquetas de origen. La ventana de Historial de IA muestra la cronología completa; la × por fila revierte una acción con sus dependencias.
• Si ajusta manualmente un parámetro que la IA acaba de mover, la IA es notificada y no reintentará a ciegas. Siempre tiene la última palabra.
• Los atajos Cmd/Ctrl+Alt+Z y Cmd/Ctrl+Alt+Y deshacen y rehacen el último cambio de IA sin afectar a sus ediciones manuales (que usan Ctrl+Z normal).

Para añadir este servidor a Claude Desktop:
  1. Abrir Configuración → Desarrollador → Editar configuración.
  2. Pegar el fragmento JSON siguiente en claude_desktop_config.json (fusionar en el bloque mcpServers existente si ya tiene uno).
  3. Reiniciar Claude Desktop. El servidor aparece como 'wfs-diy' en el menú de herramientas.

Para añadir a Claude Code, ejecutar:
  claude mcp add wfs-diy <url> -t http

La URL cambia si cambia de interfaz de red o si el servidor cae en un puerto diferente. El botón URL en esta fila siempre refleja la URL en vivo.
  - [ ] OK    Fix: 

- **`copyButton`**
  - EN: Copy Config
  - ES: Copiar configuración
  - [ ] OK    Fix: 

- **`copyConfirm`**
  - EN: MCP config JSON copied to clipboard
  - ES: Configuración MCP JSON copiada al portapapeles
  - [ ] OK    Fix: 

- **`title`**
  - EN: AI / MCP Server
  - ES: Servidor IA / MCP
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
  - ES: El sistema puede comunicarse a través de varios protocolos de red (UDP o TCP) usando OSC. La consulta OSC puede habilitarse para que los clientes descubran las rutas OSC posibles y se suscriban a actualizaciones de parámetros.
Se muestra la IP de la máquina local correspondiente a la interfaz de red seleccionada. Los puertos TCP y UDP entrantes así como el puerto OSC Query.
Hay algunos clientes OSC especializados como:
- Remote para la aplicación Android para operación multitáctil y control remoto.
- QLab que puede enviar datos y programarse directamente desde la aplicación.
- ADM-OSC para control desde consolas y DAW (ver ayuda específica).
Los datos pueden filtrarse. Una ventana de Log muestra los datos entrantes y salientes.
También hay una función de localización para encontrar tabletas Android perdidas.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Network
  - ES: Red
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
  - ES: Hay algunos parámetros para ayudarle a ajustar al sonido acústico.
La mayoría de estos parámetros se establecen para arrays completos a menos que el modo de propagación esté desactivado para esta salida. También puede seleccionarse un cambio relativo después de un ajuste específico.
- Orientación y Ángulos On/Off definen qué entradas amplificará cada altavoz. Por defecto los altavoces apuntan al público, de espaldas al escenario. Las entradas en el sector verde se amplificarán, pero no las que están frente al altavoz, en el sector rojo. Hay un fundido entre ambos sectores. Para subgraves, abrir completamente al máximo permitirá que todas las entradas sean captadas.
- La Atenuación HF simula la pérdida de altas frecuencias con la distancia.
- El porcentaje de Atenuación por Distancia define cuánta de la atenuación calculada se aplica. Para subgraves puede ser conveniente reducir al 50%.
- La Latencia Mínima activa el escaneo del menor retardo calculado.
- La Atenuación Live Source activa la reducción de nivel de entradas cercanas.
- Las Reflexiones de Suelo activan si las reflexiones se aplican a la señal para esta salida como subgraves y arrays colgados...
  - [ ] OK    Fix: 

- **`title`**
  - EN: Advanced Parameters
  - ES: Parámetros avanzados
  - [ ] OK    Fix: 

## `help.outputs`

- **`body`**
  - EN: The design of the WFS system has to do with the right choice of equipment and their position. Here is a guide to help you with the design and the tuning of your arrays.
An array is a line (straight or curved) of speakers. This is one of the most important concepts in WFS adapted to sound reinforcement and for creative sound design.
A rule of thumb would be that each listener should hear three speakers of an array to have enough psycho-acoustic cues to feel the direction each sound is coming from. There will be a sweet spot to find between the distance between the speakers and the listeners, their spacing and coverage angle. This is especially true for short range arrays, a.k.a front fills. Speakers with a 120° coverage angle can be spaced by the same distance between the array and the first row of listeners. The number of speakers also depends on the sound pressure level. When placed as a flown array, trapezoidal/asymmetric horns with wide coverage angle (120°) below axis and narrow coverage (60°) on axis will give good coverage for seat rows a bit too far from the ground array and will have enough throw to reach 20m or 30m away while avoiding the walls of the venue where they would create reflections that give away the position of the speakers to the ears of the listeners. Most of the time for larger venues coaxial (elliptical or conical horns) have not enough reach and require one or more delay lines. They are more suited for smaller venues with few rows of seats.
The positioning of the speakers in the system can be done through the 'Wizard of OutZ' and the editable presets it has.
  - ES: El diseño del sistema WFS tiene que ver con la elección correcta del equipamiento y su posicionamiento. Aquí hay una guía para ayudarle con el diseño y la afinación de sus arrays.
Un array es una línea (recta o curva) de altavoces. Este es uno de los conceptos más importantes en WFS adaptado a la sonorización y al diseño sonoro creativo.
Como regla general, cada oyente debería escuchar tres altavoces de un array para tener suficientes pistas psicoacústicas para sentir la dirección de cada sonido. Habrá un punto óptimo entre la distancia entre los altavoces y los oyentes, su espaciado y ángulo de cobertura. Los altavoces con ángulo de cobertura de 120° pueden espaciarse la misma distancia entre el array y la primera fila. El número también depende del nivel de presión sonora. Como array colgado, bocinas trapezoidales/asimétricas con ángulo ancho (120°) bajo eje y estrecho (60°) en eje darán buena cobertura y alcance de 20-30m evitando reflexiones en las paredes. Los altavoces coaxiales generalmente no tienen suficiente alcance para grandes recintos y requieren líneas de retardo.
El posicionamiento puede hacerse a través del 'Wizard of OutZ' y sus presets editables.
  - [ ] OK    Fix: 

- **`title`**
  - EN: WFS Array Design
  - ES: Diseño de arrays WFS
  - [ ] OK    Fix: 

## `help.overview`

- **`body`**
  - EN: This WFS spatial sound processor aims to be a tool for natural sound reinforcement and also a creative tool opening new avenues for writing sound in space.
Some parameters are straightforward: place sound (Map, Tracking, Speed Limiting, Gradient Maps...), work its shape (Attenuation Profile) and its acoustic presence (Directivity, Floor Reflections), give it a one-off motion (AutomOtion) or a repetitive movement (L.F.O). In some cases amplification should be limited around loud sources on stage (Live Source Tamer). All of these functionalities can be stored and recalled internally or with the help of QLab. On the other side the system allows real-time interaction to trigger and move samples, move large clusters of inputs manually or thanks to easily recallable LFO presets.
  - ES: Este procesador espacial WFS pretende ser una herramienta para la sonorización natural y también una herramienta creativa que abre nuevas vías para escribir sonido en el espacio.
Algunos parámetros son sencillos: colocar sonido (Mapa, Tracking, Limitación de velocidad, Mapas de gradiente...), trabajar su forma (Perfil de atenuación) y su presencia acústica (Directividad, Reflexiones de suelo), darle un movimiento puntual (AutomOtion) o repetitivo (L.F.O). En algunos casos la amplificación debería limitarse alrededor de fuentes potentes en el escenario (Live Source Tamer). Todas estas funcionalidades pueden almacenarse y recuperarse internamente o con ayuda de QLab. Por otro lado el sistema permite la interacción en tiempo real para disparar y mover samples, mover grandes clusters de entradas manualmente o gracias a presets LFO fácilmente recuperables.
  - [ ] OK    Fix: 

- **`dontShow`**
  - EN: Don't show this again
  - ES: No mostrar de nuevo
  - [ ] OK    Fix: 

- **`title`**
  - EN: System Overview
  - ES: Vista general del sistema
  - [ ] OK    Fix: 

## `help.parallax`

- **`body`**
  - EN: Each speaker points more or less clearly at a listener. To calculate the delay for an input for each speaker, let's consider the distance from the input to this listener, we can also calculate the distance the sound from the speaker to this listener. To match the time of arrival of both we need to apply the difference in the aforementioned distances as a delay for this input and this speaker. This gives greater stability when inputs are moved on stage and especially when they move upstage, away from the edge of the stage. This also can enable the synthesis of floor reflections. This setting can be tuned for finer adjustments, rather than simply measured. Use your ears!
  - ES: Cada altavoz apunta más o menos claramente a un oyente. Para calcular el retardo para una entrada para cada altavoz, consideramos la distancia de la entrada a este oyente, también podemos calcular la distancia del sonido del altavoz a este oyente. Para igualar el tiempo de llegada de ambos debemos aplicar la diferencia de las distancias mencionadas como retardo. Esto da mayor estabilidad cuando las entradas se mueven en el escenario y especialmente cuando se alejan del borde. Esto también puede permitir la síntesis de reflexiones de suelo. Este ajuste puede afinarse, en lugar de simplemente medirse. ¡Confíe en sus oídos!
  - [ ] OK    Fix: 

- **`title`**
  - EN: Parallax Correction
  - ES: Corrección de paralaje
  - [ ] OK    Fix: 

## `help.reverb`

- **`body`**
  - EN: Reverb helps blur the actual reflections of the speakers in the acoustic space. Without some reverb the listener might perceive the sound missing some depth because the reflections of the speakers in the acoustic of the room gives away their positions and all sounds might feel as coming from the plane of the speakers.
Place the reverb node according to the number of channels and the geometry of the stage and listening area, and the positions of speakers. If necessary the return position can be offset from the feed position.
Other parameters are very similar to Outputs' and Inputs'.
  - ES: La reverberación ayuda a difuminar las reflexiones de los altavoces.
Coloque los nodos según la geometría del escenario.
Otros parámetros son similares a los de Salidas y Entradas.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb
  - ES: Reverberación
  - [ ] OK    Fix: 

## `help.reverbAlgo`

- **`body`**
  - EN: There are three built-in reverb types inside this spatial sound processor:
- SDN (Scattered Delay Network): The sound bounces between every reverb node which act as reflecting surfaces. This reverb algorithm favours an odd number of nodes that should not have too much symmetry. This helps reduce the artifacts of the reverb or any metallic sounding resonance.
- FDN (Feedback Delay Network): Each reverb node functions as a separate reverb processor with a classical reverb algorithm. Place nodes around the stage and eventually around the audience. Inputs are sent to each node in a similar fashion as with outputs. The return signal from the reverb is then distributed like other inputs to all outputs.
- IR (Impulse Response): This is a classical convolution reverb. You can load audio samples as impulse responses. Each reverb node can share the same IR or they can have different ones. Place nodes around the stage and eventually around the audience.
The node positions can be adjusted directly on the map. The Ctrl/Cmd key will move a reverb node pair in symmetry.
  - ES: Tres tipos de reverberación están integrados en este procesador de sonido espacial:
- SDN (Scattered Delay Network): El sonido rebota entre cada nodo de reverberación que actúa como superficies reflectantes. Este algoritmo favorece un número impar de nodos sin demasiada simetría, para reducir artefactos o resonancias metálicas.
- FDN (Feedback Delay Network): Cada nodo funciona como un procesador de reverberación separado con un algoritmo clásico. Coloque nodos alrededor del escenario y eventualmente alrededor del público.
- IR (Respuesta al Impulso): Reverberación por convolución clásica. Puede cargar muestras de audio como respuestas al impulso. Cada nodo puede compartir la misma IR o usar diferentes.
Las posiciones de los nodos pueden ajustarse directamente en el mapa. La tecla Ctrl/Cmd mueve un par de nodos en simetría.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Algorithms
  - ES: Algoritmos de Reverberación
  - [ ] OK    Fix: 

## `help.reverbFeed`

- **`body`**
  - EN: Pre-processing sending of the input channels to the reverb nodes.
- Orientation and On/Off Angles define what inputs each reverb node will receive. Inputs in the green sector will be picked-up, but not the ones in front in the red sector. There is a fade between both sectors.
- HF Damping simulates the loss of high frequency with distance.
- Distance Attenuation percentage allows you to define if more or less of the attenuation calculated from the distance and parameters of the input is applied.
- Minimal Latency toggles whether the output is scanned for the smallest calculated delay and also if the setting once engaged on an input will reduce the delay for that input through this reverb node.
- Live Source Attenuation toggles the reduction of level of nearby input if they have this setting enabled.
  - ES: Envío de pre-procesamiento de los canales de entrada a los nodos.
- Orientación y ángulos On/Off definen qué entradas recibe cada nodo.
- Amortiguación HF simula la pérdida de alta frecuencia.
- Porcentaje de atenuación de distancia define la atenuación aplicada.
- Latencia mínima determina si se usa el menor retardo calculado.
- Atenuación de fuente viva reduce el nivel de entradas cercanas.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Feed
  - ES: Alimentación de Reverberación
  - [ ] OK    Fix: 

## `help.reverbPostProc`

- **`body`**
  - EN: Includes a 4-band EQ and an Expander that looks at the signal entering the reverb processor to reduce long reverb tails when the inputs are quiet.
  - ES: Incluye un EQ de 4 bandas y un Expansor que observa la señal que entra al procesador de reverberación para reducir las colas largas de reverberación cuando las entradas están en silencio.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Post-Processing
  - ES: Post-Procesamiento de Reverberación
  - [ ] OK    Fix: 

## `help.reverbPreProc`

- **`body`**
  - EN: Includes a 4-band EQ and a Compressor to remove transients which might excite the reverb processor a bit too much.
  - ES: Incluye un EQ de 4 bandas y un Compresor para eliminar transitorios que podrían excitar demasiado el procesador de reverberación.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Pre-Processing
  - ES: Pre-Procesamiento de Reverberación
  - [ ] OK    Fix: 

## `help.reverbReturn`

- **`body`**
  - EN: Post-processing sending to the speakers.
- Distance Attenuation defines the level drop per meter to the speakers.
- Common Attenuation will look for the lowest attenuation and keep only a percentage of this level drop and apply this across all other outputs.
- Mutes and Mute Macros allow to prevent a reverb channel from feeding an output. For instance it may not be necessary to send reverb to sub-bass speakers.
  - ES: Post-procesamiento enviado a los altavoces.
- Atenuación de Distancia define la caída de nivel por metro.
- Atenuación Común mantiene un porcentaje de la atenuación más baja.
- Mutes impiden que un canal de reverberación alimente una salida.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Return
  - ES: Retorno de Reverberación
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
  - ES: El sampler permite disparar muestras e interactuar con ellas en tiempo real.
Cuando está habilitado en una pista, el sampler reemplaza la entrada en vivo en todo momento.
Varios samplers pueden asignarse a diferentes entradas y dispararse individualmente.
Para usar el sampler:
- Seleccione un Roli Lightpad o un pad en la app Android Remote conectada.
- Añada muestras a las diferentes casillas de la cuadrícula. Ajuste su posición de inicio relativa, su nivel y eventualmente sus puntos de entrada y salida. Se pueden seleccionar varias muestras manteniendo Mayús mientras hace clic.
- Cree conjuntos de muestras: las muestras seleccionadas se añadirán a nuevos conjuntos. Pueden añadirse o eliminarse manteniendo Ctrl/Cmd mientras hace clic en las casillas. Cada conjunto puede renombrarse y tener una secuencia fija o aleatoria. Cada conjunto tiene un ajuste de atenuación y una posición base.
- Presione un Lightpad o pad para disparar una muestra. La presión puede mapearse a nivel, altura y filtrado de altas frecuencias. El movimiento del dedo mueve el sonido como un joystick.
Soltar el pad detiene la muestra.
Los ajustes del sampler se almacenan en los archivos de entrada.
Las casillas y conjuntos pueden copiarse, exportarse e importarse.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Sampler
  - ES: Sampler
  - [ ] OK    Fix: 

## `help.sessionData`

- **`body`**
  - EN: When starting a session select the working folder where the system will place the files and eventual audio files. For new projects create a new folder. When reloading a previous session navigate to the corresponding folder.
Each section has a separate xml file (System configuration, Network, Outputs, Reverbs, Inputs) and backups. Additionally convolution reverb impulse responses and audio samples will be stored in sub-directories.
Each section can be stored and recalled individually or as a whole.
Each section can also export and import files from other projects.
  - ES: Al iniciar una sesión, seleccione la carpeta de trabajo donde el sistema colocará los archivos y eventuales archivos de audio. Para proyectos nuevos, cree una carpeta nueva. Para recargar una sesión anterior, navegue a la carpeta correspondiente.
Cada sección tiene un archivo xml separado (Configuración del sistema, Red, Salidas, Reverberación, Entradas) y copias de seguridad. Las respuestas al impulso de reverberación por convolución y las muestras de audio se almacenarán en subdirectorios.
Cada sección puede almacenarse y recuperarse individualmente o en su totalidad.
Cada sección también puede exportar e importar archivos de otros proyectos.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Session Data
  - ES: Datos de Sesión
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
  - ES: Los snapshots almacenan parámetros de entrada, pero pueden tener un alcance para ser recuperados durante una actuación.
El Alcance indica al sistema qué datos almacenar o recuperar.
Hay varios métodos disponibles:
- Registrar solo los datos necesarios en archivos locales. El filtro se aplica al almacenar.
- Registrar todos los datos y un filtro en archivos locales. El filtro se aplica al recuperar.
- Registrar todos los datos en cues de QLab. No recomendado para configuraciones grandes.
El alcance puede mostrar y preseleccionar automáticamente los parámetros modificados manualmente. Los cambios se marcan en amarillo.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Input Snapshots and Scope
  - ES: Snapshots de Entrada y Alcance
  - [ ] OK    Fix: 

## `help.tracking`

- **`body`**
  - EN: Tracking allows to follow the position in 2D or 3D of actors and musicians. There are several tracking solutions based on UWB (UltraWide Band) tags, 3D camera, computer vision systems and Infra-Red LED's followed by IR sensitive cameras.
This application allows you to receive tracking data from several protocols: OSC, MQTT, PosiStageNet/PSN, RTTrP.
You can select which protocol you're using and enter its settings. You may also adjust the mapping (offset, scaling and orientation).
Each input has a toggle to enable tracking, an ID to select which tracking marker it should follow and a smoothing algorithm to reduce jittery movements.
  - ES: El tracking permite seguir la posición en 2D o 3D de actores y músicos. Existen varias soluciones basadas en etiquetas UWB, cámaras 3D, sistemas de visión por computadora y LEDs infrarrojos con cámaras sensibles a IR.
Esta aplicación permite recibir datos de tracking desde varios protocolos: OSC, MQTT, PosiStageNet/PSN, RTTrP.
Puede seleccionar el protocolo utilizado y configurar sus parámetros. También puede ajustar el mapeo (desplazamiento, escala y orientación).
Cada entrada tiene un toggle para habilitar el tracking, un ID para seleccionar el marcador a seguir y un algoritmo de suavizado.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Tracking
  - ES: Tracking
  - [ ] OK    Fix: 

## `help.tuning`

- **`body`**
  - EN: System tuning for WFS is different from standard PA tuning. It can proceed as follows:
- Start with the flown array muted. Set the desired levels for the near-field speakers when listening to them in the first row. Adjust the high frequency shelf attenuation so the near-field speakers are not too bright and draw too much attention.
- Mute the near-field array and unmute the flown array, find a suitable level towards the back of the house.
- Unmute both arrays, adjust the delay of the flown array to bring the sound down to the correct height in the lower rows. Adjust the levels, HF shelf/distance ratio and vertical and horizontal parallax for each array to have consistent level wherever your inputs are on stage. This is especially important if your inputs are actors, singers or acoustic music instruments on stage. You may also test adding some Haas effect to delay the whole system if you feel the WFS is too early or adding comb filtering with the acoustic sound.
You may follow a different workflow for tuning or go for different cues.
  - ES: La afinación del sistema WFS es diferente de la afinación PA estándar. Puede proceder así:
- Comience con el array colgado silenciado. Ajuste los niveles deseados para los altavoces de campo cercano escuchándolos desde la primera fila. Ajuste la atenuación del shelf de alta frecuencia para que no sean demasiado brillantes.
- Silencie el array de campo cercano y active el array colgado, encuentre un nivel adecuado hacia el fondo de la sala.
- Active ambos arrays, ajuste el retardo del array colgado para bajar el sonido a la altura correcta en las filas inferiores. Ajuste niveles, shelf HF/ratio de distancia y paralaje vertical y horizontal para cada array para un nivel consistente dondequiera que estén sus entradas en el escenario.
Puede seguir un flujo de trabajo diferente para la afinación o buscar diferentes ajustes según las situaciones.
  - [ ] OK    Fix: 

- **`title`**
  - EN: System Tuning
  - ES: Afinación del sistema
  - [ ] OK    Fix: 

## `inputs`

- **`arrayPrefix`**
  - EN: Array
  - ES: Arreglo
  - [ ] OK    Fix: 

## `inputs.buttons`

- **`deleteSnapshot`**
  - EN: Delete Snapshot
  - ES: Eliminar Snapshot
  - [ ] OK    Fix: 

- **`editScope`**
  - EN: Edit Scope
  - ES: Editar alcance
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - ES: Exportar
  - [ ] OK    Fix: 

- **`hiddenOnMap`**
  - EN: Input Hidden on Map
  - ES: Entrada oculta en mapa
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - ES: Importar
  - [ ] OK    Fix: 

- **`lockOnMap`**
  - EN: Lock on Map
  - ES: Bloquear en mapa
  - [ ] OK    Fix: 

- **`pauseAll`**
  - EN: Pause All
  - ES: Pausar todo
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Backup
  - ES: Recargar respaldo
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Input Config
  - ES: Recargar Config Entradas
  - [ ] OK    Fix: 

- **`reloadSnapshot`**
  - EN: Reload Snapshot
  - ES: Recargar Snapshot
  - [ ] OK    Fix: 

- **`reloadWithoutScope`**
  - EN: Reload Without Scope
  - ES: Recargar sin filtro
  - [ ] OK    Fix: 

- **`resumeAll`**
  - EN: Resume All
  - ES: Reanudar todo
  - [ ] OK    Fix: 

- **`samplerOff`**
  - EN: Sampler: OFF
  - ES: Sampler: OFF
  - [ ] OK    Fix: 

- **`samplerOn`**
  - EN: Sampler: ON
  - ES: Sampler: ON
  - [ ] OK    Fix: 

- **`setAllInputs`**
  - EN: Set all Inputs...
  - ES: Configurar todas las entradas...
  - [ ] OK    Fix: 

- **`solo`**
  - EN: Solo
  - ES: Solo
  - [ ] OK    Fix: 

- **`soloModeMulti`**
  - EN: Multi
  - ES: Multi
  - [ ] OK    Fix: 

- **`soloModeSingle`**
  - EN: Single
  - ES: Single
  - [ ] OK    Fix: 

- **`stopAll`**
  - EN: Stop All
  - ES: Detener todo
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Input Config
  - ES: Guardar Config Entradas
  - [ ] OK    Fix: 

- **`storeSnapshot`**
  - EN: Store Snapshot
  - ES: Guardar Snapshot
  - [ ] OK    Fix: 

- **`updateSnapshot`**
  - EN: Update Snapshot
  - ES: Actualizar Snapshot
  - [ ] OK    Fix: 

- **`visibleOnMap`**
  - EN: Input Visible on Map
  - ES: Entrada visible en mapa
  - [ ] OK    Fix: 

## `inputs.clusters`

- **`clusterPrefix`**
  - EN: Cluster
  - ES: Grupo
  - [ ] OK    Fix: 

- **`single`**
  - EN: Single
  - ES: Individual
  - [ ] OK    Fix: 

## `inputs.coordinates`

- **`xyz`**
  - EN: XYZ
  - ES: XYZ
  - [ ] OK    Fix: 

## `inputs.dialogs`

- **`exportConfig`**
  - EN: Export Input Configuration
  - ES: Exportar configuración entradas
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Input Configuration
  - ES: Importar configuración entradas
  - [ ] OK    Fix: 

- **`selectChannel`**
  - EN: Select Channel
  - ES: Seleccionar canal
  - [ ] OK    Fix: 

- **`snapshotNameLabel`**
  - EN: Name:
  - ES: Nombre:
  - [ ] OK    Fix: 

- **`storeSnapshotMessage`**
  - EN: Enter a name for the new snapshot:
  - ES: Introduzca un nombre para el nuevo snapshot:
  - [ ] OK    Fix: 

- **`storeSnapshotTitle`**
  - EN: Store New Snapshot
  - ES: Guardar nuevo snapshot
  - [ ] OK    Fix: 

- **`trackingConflictCluster`**
  - EN: Input {current} has tracking enabled, but Input {existing} in Cluster {cluster} is already tracked.

Only one tracked input per cluster is allowed.
  - ES: La entrada {current} tiene el tracking activado, pero la entrada {existing} del cluster {cluster} ya está siendo rastreada.

Solo se permite una entrada con tracking por cluster.
  - [ ] OK    Fix: 

- **`trackingConflictContinue`**
  - EN: Continue (disable tracking)
  - ES: Continuar (desactivar tracking)
  - [ ] OK    Fix: 

- **`trackingConflictSwitch`**
  - EN: Input {existing} in Cluster {cluster} already has tracking enabled.

Only one tracked input per cluster is allowed.

Do you want to disable tracking on Input {existing} and enable it on Input {to}?
  - ES: La entrada {existing} del cluster {cluster} ya tiene el tracking activado.

Solo se permite una entrada con tracking por cluster.

¿Desea desactivar el tracking en la entrada {existing} y activarlo en la entrada {to}?
  - [ ] OK    Fix: 

- **`trackingConflictTitle`**
  - EN: Tracking Conflict
  - ES: Conflicto de tracking
  - [ ] OK    Fix: 

- **`trackingConflictYes`**
  - EN: Yes, switch tracking
  - ES: Sí, transferir tracking
  - [ ] OK    Fix: 

## `inputs.gradientMap`

- **`buttons.copyLayer`**
  - EN: Copy Layer
  - ES: Copiar capa
  - [ ] OK    Fix: 

- **`buttons.copyShape`**
  - EN: Copy Shape
  - ES: Copiar forma
  - [ ] OK    Fix: 

- **`buttons.delete`**
  - EN: Delete
  - ES: Eliminar
  - [ ] OK    Fix: 

- **`buttons.enable`**
  - EN: On
  - ES: On
  - [ ] OK    Fix: 

- **`buttons.lock`**
  - EN: Lock
  - ES: Bloquear
  - [ ] OK    Fix: 

- **`buttons.pasteLayer`**
  - EN: Paste Layer
  - ES: Pegar capa
  - [ ] OK    Fix: 

- **`buttons.pasteShape`**
  - EN: Paste Shape
  - ES: Pegar forma
  - [ ] OK    Fix: 

- **`header.attenuationLayer`**
  - EN: Attenuation Layer
  - ES: Capa Atenuación
  - [ ] OK    Fix: 

- **`header.heightLayer`**
  - EN: Height Layer
  - ES: Capa Altura
  - [ ] OK    Fix: 

- **`header.hfShelfLayer`**
  - EN: HF Shelf Layer
  - ES: Capa Estante HF
  - [ ] OK    Fix: 

- **`help.blackValue`**
  - EN: Parameter value mapped to black (0.00–1.00)
  - ES: Valor del parámetro mapeado al negro (0.00–1.00)
  - [ ] OK    Fix: 

- **`help.blur`**
  - EN: Edge blur in meters
  - ES: Desenfoque de bordes en metros
  - [ ] OK    Fix: 

- **`help.copy`**
  - EN: Copy selected shape or layer
  - ES: Copiar forma o capa seleccionada
  - [ ] OK    Fix: 

- **`help.curve`**
  - EN: Gamma curve (-1 to 1, 0 = linear)
  - ES: Curva gamma (-1 a 1, 0 = lineal)
  - [ ] OK    Fix: 

- **`help.ellipseTool`**
  - EN: Draw ellipse
  - ES: Dibujar elipse
  - [ ] OK    Fix: 

- **`help.fillTool`**
  - EN: Apply uniform fill to shape
  - ES: Aplicar relleno uniforme a la forma
  - [ ] OK    Fix: 

- **`help.fillValue`**
  - EN: Fill brightness (0 = black, 1 = white)
  - ES: Brillo de relleno (0 = negro, 1 = blanco)
  - [ ] OK    Fix: 

- **`help.layerEnable`**
  - EN: Enable/disable layer (affects output and OSC)
  - ES: Activar/desactivar capa (afecta salida y OSC)
  - [ ] OK    Fix: 

- **`help.layerSelect`**
  - EN: Select this layer for editing
  - ES: Seleccionar esta capa para editar
  - [ ] OK    Fix: 

- **`help.layerVisible`**
  - EN: Show/hide layer preview on canvas
  - ES: Mostrar/ocultar vista previa de la capa en el lienzo
  - [ ] OK    Fix: 

- **`help.linGradTool`**
  - EN: Apply linear gradient to shape
  - ES: Aplicar degradado lineal a la forma
  - [ ] OK    Fix: 

- **`help.paste`**
  - EN: Paste shape or layer from clipboard
  - ES: Pegar forma o capa desde el portapapeles
  - [ ] OK    Fix: 

- **`help.polygonTool`**
  - EN: Draw polygon (double-click to close)
  - ES: Dibujar polígono (doble clic para cerrar)
  - [ ] OK    Fix: 

- **`help.radGradTool`**
  - EN: Apply radial gradient to shape
  - ES: Aplicar degradado radial a la forma
  - [ ] OK    Fix: 

- **`help.rectTool`**
  - EN: Draw rectangle
  - ES: Dibujar rectángulo
  - [ ] OK    Fix: 

- **`help.selectTool`**
  - EN: Select and move shapes
  - ES: Seleccionar y mover formas
  - [ ] OK    Fix: 

- **`help.shapeDelete`**
  - EN: Delete selected shape(s)
  - ES: Eliminar forma(s) seleccionada(s)
  - [ ] OK    Fix: 

- **`help.shapeEnable`**
  - EN: Enable/disable shape
  - ES: Activar/desactivar forma
  - [ ] OK    Fix: 

- **`help.shapeLock`**
  - EN: Lock shape position
  - ES: Bloquear posición de la forma
  - [ ] OK    Fix: 

- **`help.whiteValue`**
  - EN: Parameter value mapped to white (0.00–1.00)
  - ES: Valor del parámetro mapeado al blanco (0.00–1.00)
  - [ ] OK    Fix: 

- **`hints.darkMaxAtten`**
  - EN: Dark = max attenuation | Light = none
  - ES: Oscuro = atenuación máx. | Claro = ninguna
  - [ ] OK    Fix: 

- **`hints.darkMaxHeight`**
  - EN: Dark = max height | Light = ground
  - ES: Oscuro = altura máx. | Claro = suelo
  - [ ] OK    Fix: 

- **`hints.darkMaxHF`**
  - EN: Dark = max HF shelf | Light = none
  - ES: Oscuro = estante HF máx. | Claro = ninguno
  - [ ] OK    Fix: 

- **`hints.polygonClose`**
  - EN: Double-click to close polygon
  - ES: Doble clic para cerrar el polígono
  - [ ] OK    Fix: 

- **`hints.whiteMaxAtten`**
  - EN: White = max attenuation | Black = none
  - ES: Blanco = atenuación máx. | Negro = ninguna
  - [ ] OK    Fix: 

- **`hints.whiteMaxHeight`**
  - EN: White = max height | Black = ground
  - ES: Blanco = altura máx. | Negro = suelo
  - [ ] OK    Fix: 

- **`hints.whiteMaxHF`**
  - EN: White = max HF shelf | Black = none
  - ES: Blanco = estante HF máx. | Negro = ninguno
  - [ ] OK    Fix: 

- **`labels.black`**
  - EN: Black:
  - ES: Negro:
  - [ ] OK    Fix: 

- **`labels.blur`**
  - EN: Blur:
  - ES: Desenfoque:
  - [ ] OK    Fix: 

- **`labels.center`**
  - EN: Center:
  - ES: Centro:
  - [ ] OK    Fix: 

- **`labels.curve`**
  - EN: Curve:
  - ES: Curva:
  - [ ] OK    Fix: 

- **`labels.edge`**
  - EN: Edge:
  - ES: Borde:
  - [ ] OK    Fix: 

- **`labels.end`**
  - EN: End:
  - ES: Fin:
  - [ ] OK    Fix: 

- **`labels.fill`**
  - EN: Fill:
  - ES: Relleno:
  - [ ] OK    Fix: 

- **`labels.name`**
  - EN: Name:
  - ES: Nombre:
  - [ ] OK    Fix: 

- **`labels.start`**
  - EN: Start:
  - ES: Inicio:
  - [ ] OK    Fix: 

- **`labels.white`**
  - EN: White:
  - ES: Blanco:
  - [ ] OK    Fix: 

- **`layers.attenuation`**
  - EN: Attenuation
  - ES: Atenuación
  - [ ] OK    Fix: 

- **`layers.height`**
  - EN: Height
  - ES: Altura
  - [ ] OK    Fix: 

- **`layers.hfShelf`**
  - EN: HF Shelf
  - ES: Estante HF
  - [ ] OK    Fix: 

- **`tools.editPoints`**
  - EN: Edit Points
  - ES: Editar puntos
  - [ ] OK    Fix: 

- **`tools.ellipse`**
  - EN: Ellipse
  - ES: Elipse
  - [ ] OK    Fix: 

- **`tools.fill`**
  - EN: Fill
  - ES: Rellenar
  - [ ] OK    Fix: 

- **`tools.linGrad`**
  - EN: Linear Grad.
  - ES: Grad. Lineal
  - [ ] OK    Fix: 

- **`tools.polygon`**
  - EN: Polygon
  - ES: Polígono
  - [ ] OK    Fix: 

- **`tools.radGrad`**
  - EN: Radial Grad.
  - ES: Grad. Radial
  - [ ] OK    Fix: 

- **`tools.rect`**
  - EN: Rectangle
  - ES: Rectángulo
  - [ ] OK    Fix: 

- **`tools.select`**
  - EN: Select
  - ES: Selección
  - [ ] OK    Fix: 

- **`warnings.heightRatioZero`**
  - EN: Height Ratio is 0% — increase it for height to take effect
  - ES: La proporción de altura es 0% — auméntela para que la altura tenga efecto
  - [ ] OK    Fix: 

## `inputs.help`

- **`admMapping`**
  - EN: Assign this input to an ADM-OSC mapping for position receive/transmit.
  - ES: Asignar esta entrada a un mapeo ADM-OSC para recepción/transmisión de posición.
  - [ ] OK    Fix: 

- **`arrayAttenDial`**
  - EN: Attenuation for Array {num} (-60 to 0 dB).
  - ES: Atenuación para el array {num} (-60 a 0 dB).
  - [ ] OK    Fix: 

- **`attenuationLawButton`**
  - EN: Attenuation Law Model (Linear Decrease of Volume with Distance Between Object and Speaker or Squared).
  - ES: Modelo de ley de atenuación (disminución lineal del volumen con la distancia entre objeto y altavoz, o cuadrática).
  - [ ] OK    Fix: 

- **`attenuationSlider`**
  - EN: Input Channel Attenuation.
  - ES: Atenuación del canal de entrada.
  - [ ] OK    Fix: 

- **`channelSelector`**
  - EN: Input Channel Number and Selection.
  - ES: Número y selección del canal de entrada.
  - [ ] OK    Fix: 

- **`clusterSelector`**
  - EN: Object is Part of a Cluster.
  - ES: El objeto forma parte de un cluster.
  - [ ] OK    Fix: 

- **`commonAttenDial`**
  - EN: Percentage of the Common Part of the Attenuation for selected Object Relative to All Outputs.
  - ES: Porcentaje de la parte común de la atenuación para el objeto seleccionado en relación con todas las salidas.
  - [ ] OK    Fix: 

- **`constraintDistanceButton`**
  - EN: Limit Position to Distance Range from Origin (for Cylindrical/Spherical modes).
  - ES: Limitar la posición a un rango de distancia desde el origen (para modos cilíndrico/esférico).
  - [ ] OK    Fix: 

- **`constraintXButton`**
  - EN: Limit Position to the Bounds of the Stage in Width.
  - ES: Limitar la posición a los límites del escenario en anchura.
  - [ ] OK    Fix: 

- **`constraintYButton`**
  - EN: Limit Position to the Bounds of the Stage in Depth.
  - ES: Limitar la posición a los límites del escenario en profundidad.
  - [ ] OK    Fix: 

- **`constraintZButton`**
  - EN: Limit Position to the Bounds of the Stage in Height.
  - ES: Limitar la posición a los límites del escenario en altura.
  - [ ] OK    Fix: 

- **`coordModeSelector`**
  - EN: Coordinate display mode: Cartesian (X/Y/Z), Cylindrical (radius/azimuth/height), or Spherical (radius/azimuth/elevation).
  - ES: Modo de visualización de coordenadas: Cartesiano (X/Y/Z), Cilíndrico (radio/azimut/altura) o Esférico (radio/azimut/elevación).
  - [ ] OK    Fix: 

- **`delayLatencySlider`**
  - EN: Input Channel Delay (positive values) or Latency Compensation (negative values).
  - ES: Retardo del canal de entrada (valores positivos) o compensación de latencia (valores negativos).
  - [ ] OK    Fix: 

- **`deleteSnapshot`**
  - EN: Delete Selected Input Snapshot With Confirmation.
  - ES: Eliminar el snapshot de entrada seleccionado con confirmación.
  - [ ] OK    Fix: 

- **`directivitySlider`**
  - EN: How Wide is the Brightness of The Object.
  - ES: Anchura del cono de brillo del objeto.
  - [ ] OK    Fix: 

- **`distanceAttenDial`**
  - EN: Attenuation per Meter Between Object and Speaker.
  - ES: Atenuación por metro entre el objeto y el altavoz.
  - [ ] OK    Fix: 

- **`distanceMaxEditor`**
  - EN: Maximum Distance from Origin in Meters.
  - ES: Distancia máxima desde el origen en metros.
  - [ ] OK    Fix: 

- **`distanceMinEditor`**
  - EN: Minimum Distance from Origin in Meters.
  - ES: Distancia mínima desde el origen en metros.
  - [ ] OK    Fix: 

- **`distanceRangeSlider`**
  - EN: Set Minimum and Maximum Distance from Origin.
  - ES: Establecer la distancia mínima y máxima desde el origen.
  - [ ] OK    Fix: 

- **`distanceRatioDial`**
  - EN: Attenuation Ratio for Squared Model.
  - ES: Ratio de atenuación para el modelo cuadrático.
  - [ ] OK    Fix: 

- **`editScope`**
  - EN: Open Selected Input Snapshot Scope Window.
  - ES: Abrir la ventana de filtro del snapshot de entrada seleccionado.
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Input Configuration to file (with file explorer window).
  - ES: Exportar la configuración de entrada a archivo (con explorador de archivos).
  - [ ] OK    Fix: 

- **`flipXButton`**
  - EN: X will be Symetrical to the Origin. Keyboard Nudging will be Inverted.
  - ES: X será simétrica al origen. La navegación por teclado se invertirá.
  - [ ] OK    Fix: 

- **`flipYButton`**
  - EN: Y will be Symetrical to the Origin. Keyboard Nudging will be Inverted.
  - ES: Y será simétrica al origen. La navegación por teclado se invertirá.
  - [ ] OK    Fix: 

- **`flipZButton`**
  - EN: Z will be Symetrical to the Origin. Keyboard Nudging will be Inverted.
  - ES: Z será simétrica al origen. La navegación por teclado se invertirá.
  - [ ] OK    Fix: 

- **`frActiveButton`**
  - EN: Enable Simulated Floor Reflections for the Object.
  - ES: Activar las reflexiones de suelo simuladas para el objeto.
  - [ ] OK    Fix: 

- **`frAttenuationSlider`**
  - EN: Attenuation of the Simulated Floor Reflections for the Object.
  - ES: Atenuación de las reflexiones de suelo simuladas para el objeto.
  - [ ] OK    Fix: 

- **`frDiffusionDial`**
  - EN: Diffusion Effect of the Simulated Floor Reflections for the Object.
  - ES: Efecto de difusión de las reflexiones de suelo simuladas para el objeto.
  - [ ] OK    Fix: 

- **`frHighShelfActiveButton`**
  - EN: Enable High Shelf Filter for Floor Reflections.
  - ES: Activar el filtro shelving agudo para las reflexiones de suelo.
  - [ ] OK    Fix: 

- **`frHighShelfFreqSlider`**
  - EN: High Shelf Frequency for Floor Reflections.
  - ES: Frecuencia del shelving agudo para las reflexiones de suelo.
  - [ ] OK    Fix: 

- **`frHighShelfGainSlider`**
  - EN: High Shelf Gain for Floor Reflections.
  - ES: Ganancia del shelving agudo para las reflexiones de suelo.
  - [ ] OK    Fix: 

- **`frHighShelfSlopeSlider`**
  - EN: High Shelf Slope for Floor Reflections.
  - ES: Pendiente del shelving agudo para las reflexiones de suelo.
  - [ ] OK    Fix: 

- **`frLowCutActiveButton`**
  - EN: Enable Low Cut Filter for Floor Reflections.
  - ES: Activar el filtro corte bajo para las reflexiones de suelo.
  - [ ] OK    Fix: 

- **`frLowCutFreqSlider`**
  - EN: Low Cut Frequency for Floor Reflections.
  - ES: Frecuencia del corte bajo para las reflexiones de suelo.
  - [ ] OK    Fix: 

- **`heightFactorDial`**
  - EN: Take Elevation of Object into Account Fully, Partially or Not.
  - ES: Tener en cuenta la elevación del objeto totalmente, parcialmente o nada.
  - [ ] OK    Fix: 

- **`hfShelfSlider`**
  - EN: How Much Brightness is lost in the Back of the Object, Out of its Brightness Cone.
  - ES: Cuánto brillo se pierde detrás del objeto, fuera de su cono de brillo.
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Input Configuration from file (with file explorer window).
  - ES: Importar la configuración de entrada desde archivo (con explorador de archivos).
  - [ ] OK    Fix: 

- **`inputDirectivityDial`**
  - EN: Where is the Object pointing to in the Horizontal Plane.
  - ES: Dirección del objeto en el plano horizontal.
  - [ ] OK    Fix: 

- **`jitterSlider`**
  - EN: Sphere of Rapid Movements of the Object.
  - ES: Esfera de movimientos rápidos del objeto.
  - [ ] OK    Fix: 

- **`lfoActiveButton`**
  - EN: Enable or Disable the Periodic Movement of the Object (LFO).
  - ES: Activar o desactivar el movimiento periódico del objeto (LFO).
  - [ ] OK    Fix: 

- **`lfoAmplitudeXSlider`**
  - EN: Width of Movement in Relation to Base Position of the Object.
  - ES: Anchura del movimiento respecto a la posición base del objeto.
  - [ ] OK    Fix: 

- **`lfoAmplitudeYSlider`**
  - EN: Depth of Movement in Relation to Base Position of the Object.
  - ES: Profundidad del movimiento respecto a la posición base del objeto.
  - [ ] OK    Fix: 

- **`lfoAmplitudeZSlider`**
  - EN: Height of Movement in Relation to Base Position of the Object.
  - ES: Altura del movimiento respecto a la posición base del objeto.
  - [ ] OK    Fix: 

- **`lfoGyrophoneSelector`**
  - EN: Rotation of the Brightness Cone of the Object.
  - ES: Rotación del cono de brillo del objeto.
  - [ ] OK    Fix: 

- **`lfoPeriodDial`**
  - EN: Base Period of the Movement of the Object.
  - ES: Período base del movimiento del objeto.
  - [ ] OK    Fix: 

- **`lfoPhaseDial`**
  - EN: Phase Offset of the Movement of the Object.
  - ES: Desplazamiento de fase del movimiento del objeto.
  - [ ] OK    Fix: 

- **`lfoPhaseXDial`**
  - EN: Phase Offset of the Movement of the Object in Width.
  - ES: Desplazamiento de fase del movimiento del objeto en anchura.
  - [ ] OK    Fix: 

- **`lfoPhaseYDial`**
  - EN: Phase Offset of the Movement of the Object in Depth.
  - ES: Desplazamiento de fase del movimiento del objeto en profundidad.
  - [ ] OK    Fix: 

- **`lfoPhaseZDial`**
  - EN: Phase Offset of the Movement of the Object in Height.
  - ES: Desplazamiento de fase del movimiento del objeto en altura.
  - [ ] OK    Fix: 

- **`lfoRateXSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Width.
  - ES: Movimiento más rápido o lento respecto al período base, en anchura.
  - [ ] OK    Fix: 

- **`lfoRateYSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Depth.
  - ES: Movimiento más rápido o lento respecto al período base, en profundidad.
  - [ ] OK    Fix: 

- **`lfoRateZSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Height.
  - ES: Movimiento más rápido o lento respecto al período base, en altura.
  - [ ] OK    Fix: 

- **`lfoShapeXSelector`**
  - EN: Movement Behaviour of the Object in Width.
  - ES: Comportamiento del movimiento del objeto en anchura.
  - [ ] OK    Fix: 

- **`lfoShapeYSelector`**
  - EN: Movement Behaviour of the Object in Depth.
  - ES: Comportamiento del movimiento del objeto en profundidad.
  - [ ] OK    Fix: 

- **`lfoShapeZSelector`**
  - EN: Movement Behaviour of the Object in Height.
  - ES: Comportamiento del movimiento del objeto en altura.
  - [ ] OK    Fix: 

- **`lsActiveButton`**
  - EN: If You Need to Reduce the Level in Speakers Close to the Object. (eg. Loud Source Present on Stage)
  - ES: Si necesita reducir el nivel en altavoces cercanos al objeto (p. ej., fuente alta presente en el escenario).
  - [ ] OK    Fix: 

- **`lsAttenuationSlider`**
  - EN: Constant Attenuation of Speakers Around the Object.
  - ES: Atenuación constante de los altavoces alrededor del objeto.
  - [ ] OK    Fix: 

- **`lsPeakEnableButton`**
  - EN: Enable or Disable the Fast (Peak) Compressor for the Live Source Tamer.
  - ES: Activar o desactivar el compresor rápido (pico) para el Domador de Source Live.
  - [ ] OK    Fix: 

- **`lsPeakRatioDial`**
  - EN: Ratio to Apply the Fast Compression for Speakers Around the Object.
  - ES: Ratio aplicado a la compresión rápida para los altavoces alrededor del objeto.
  - [ ] OK    Fix: 

- **`lsPeakThresholdSlider`**
  - EN: Fast Compression Threshold for Speakers Around the Object to Control Transients.
  - ES: Umbral de compresión rápida para los altavoces alrededor del objeto, para controlar transitorios.
  - [ ] OK    Fix: 

- **`lsRadiusSlider`**
  - EN: How Far does the Attenuation Affect The Speakers.
  - ES: Hasta dónde afecta la atenuación a los altavoces.
  - [ ] OK    Fix: 

- **`lsShapeSelector`**
  - EN: Profile of the Attenuation Around the Object.
  - ES: Perfil de la atenuación alrededor del objeto.
  - [ ] OK    Fix: 

- **`lsSlowEnableButton`**
  - EN: Enable or Disable the Slow Compressor for the Live Source Tamer.
  - ES: Activar o desactivar el compresor lento para el Domador de Source Live.
  - [ ] OK    Fix: 

- **`lsSlowRatioDial`**
  - EN: Ratio to Apply the Slow Compression for Speakers Around the Object.
  - ES: Ratio aplicado a la compresión lenta para los altavoces alrededor del objeto.
  - [ ] OK    Fix: 

- **`lsSlowThresholdSlider`**
  - EN: Slow Compression Threshold for Speakers Around the Object to Control Sustained Level.
  - ES: Umbral de compresión lenta para los altavoces alrededor del objeto, para controlar el nivel sostenido.
  - [ ] OK    Fix: 

- **`mapLockButton`**
  - EN: Prevent Interaction on the Map Tab
  - ES: Impedir la interacción en la pestaña Mapa.
  - [ ] OK    Fix: 

- **`mapVisibilityButton`**
  - EN: Make Visible or Hide The Selected Input on the Map
  - ES: Mostrar u ocultar la entrada seleccionada en el mapa.
  - [ ] OK    Fix: 

- **`maxSpeedActiveButton`**
  - EN: Enable or Disable Speed Limiting for Object.
  - ES: Activar o desactivar la limitación de velocidad para el objeto.
  - [ ] OK    Fix: 

- **`maxSpeedDial`**
  - EN: Maximum Speed Limit for Object.
  - ES: Límite de velocidad máxima para el objeto.
  - [ ] OK    Fix: 

- **`minimalLatencyButton`**
  - EN: Select between Acoustic Precedence and Minimal Latency for Amplification Precedence.
  - ES: Elegir entre precedencia acústica y latencia mínima para la precedencia de amplificación.
  - [ ] OK    Fix: 

- **`muteButton`**
  - EN: Mute Output {num} for this Object.
  - ES: Silenciar la salida {num} para este objeto.
  - [ ] OK    Fix: 

- **`muteMacrosSelector`**
  - EN: Mute Macros for Fast Muting and Unmuting of Arrays.
  - ES: Macros de mute para silenciar y reactivar rápidamente los arrays.
  - [ ] OK    Fix: 

- **`muteReverbSendsButton`**
  - EN: Mute sends from this input to all reverb channels.
  - ES: Silenciar los envíos de esta entrada a todos los canales de reverberación.
  - [ ] OK    Fix: 

- **`nameEditor`**
  - EN: Displayed Input Channel Name (editable).
  - ES: Nombre mostrado del canal de entrada (editable).
  - [ ] OK    Fix: 

- **`nudgeHint1`**
  - EN:  Nudge with Left and Right Arrow Keys.
  - ES:  Ajustar con las teclas Izquierda y Derecha.
  - [ ] OK    Fix: 

- **`nudgeHint2`**
  - EN:  Nudge with Up and Down Arrow Keys.
  - ES:  Ajustar con las teclas Arriba y Abajo.
  - [ ] OK    Fix: 

- **`nudgeHint3`**
  - EN:  Nudge with Page Up and Page Down Keys.
  - ES:  Ajustar con Re Pág y Av Pág.
  - [ ] OK    Fix: 

- **`offset1`**
  - EN: Object Offset {name} ({unit}). Adjusted when Tracking is Enabled.
  - ES: Desplazamiento {name} del objeto ({unit}). Ajustado cuando el seguimiento está activado.
  - [ ] OK    Fix: 

- **`offset2`**
  - EN: Object Offset {name} ({unit}). Adjusted when Tracking is Enabled.
  - ES: Desplazamiento {name} del objeto ({unit}). Ajustado cuando el seguimiento está activado.
  - [ ] OK    Fix: 

- **`offset3`**
  - EN: Object Offset {name} ({unit}). Adjusted when Tracking is Enabled.
  - ES: Desplazamiento {name} del objeto ({unit}). Ajustado cuando el seguimiento está activado.
  - [ ] OK    Fix: 

- **`otomoAbsRelButton`**
  - EN: Select Relative or Absolute Coordinates of Displacement.
  - ES: Elegir coordenadas de desplazamiento relativas o absolutas.
  - [ ] OK    Fix: 

- **`otomoCoordMode`**
  - EN: Coordinate display mode for AutomOtion destinations: Cartesian (X/Y/Z), Cylindrical (r/θ/Z), or Spherical (r/θ/φ).
  - ES: Modo de coordenadas para destinos AutomOtion: Cartesiano (X/Y/Z), Cilíndrico (r/θ/Z) o Esférico (r/θ/φ).
  - [ ] OK    Fix: 

- **`otomoCurveDial`**
  - EN: Bend the Path to the Left (Negative) or Right (Positive) of the Direction of Travel.
  - ES: Curvar la trayectoria a la izquierda (negativo) o a la derecha (positivo) del sentido de movimiento.
  - [ ] OK    Fix: 

- **`otomoDest1`**
  - EN: Relative or Absolute Destination {name} ({unit}).
  - ES: Destino relativo o absoluto {name} ({unit}).
  - [ ] OK    Fix: 

- **`otomoDest2`**
  - EN: Relative or Absolute Destination {name} ({unit}).
  - ES: Destino relativo o absoluto {name} ({unit}).
  - [ ] OK    Fix: 

- **`otomoDest3`**
  - EN: Relative or Absolute Destination {name} ({unit}).
  - ES: Destino relativo o absoluto {name} ({unit}).
  - [ ] OK    Fix: 

- **`otomoDurationDial`**
  - EN: Duration of the Movement in Seconds (0.1s to 1 hour).
  - ES: Duración del movimiento en segundos (0,1 s a 1 hora).
  - [ ] OK    Fix: 

- **`otomoPauseButton`**
  - EN: Pause and Resume the Movement.
  - ES: Pausar y reanudar el movimiento.
  - [ ] OK    Fix: 

- **`otomoPauseResumeAllButton`**
  - EN: Pause or Resume All Active Movements Globally.
  - ES: Pausar o reanudar globalmente todos los movimientos activos.
  - [ ] OK    Fix: 

- **`otomoResetDial`**
  - EN: Set the Reset Level for the Automatic Trigger.
  - ES: Establecer el nivel de reinicio para el disparo automático.
  - [ ] OK    Fix: 

- **`otomoSpeedProfileDial`**
  - EN: Constant Speed or Gradual Acceleration and Slow Down at the Start and the End of the Movement.
  - ES: Velocidad constante o aceleración y desaceleración graduales al inicio y al final del movimiento.
  - [ ] OK    Fix: 

- **`otomoStartButton`**
  - EN: Start the Movement Manually.
  - ES: Iniciar el movimiento manualmente.
  - [ ] OK    Fix: 

- **`otomoStayReturnButton`**
  - EN: At the End of the Movement, should the Source Stay or Return to the Original Position.
  - ES: Al final del movimiento, la fuente permanece o vuelve a la posición original.
  - [ ] OK    Fix: 

- **`otomoStopAllButton`**
  - EN: Stop All Active Movements Globally.
  - ES: Detener globalmente todos los movimientos activos.
  - [ ] OK    Fix: 

- **`otomoStopButton`**
  - EN: Stop the Movement.
  - ES: Detener el movimiento.
  - [ ] OK    Fix: 

- **`otomoThresholdDial`**
  - EN: Set the Threshold for the Automatic Trigger of the Movement.
  - ES: Establecer el umbral para el disparo automático del movimiento.
  - [ ] OK    Fix: 

- **`otomoTriggerButton`**
  - EN: Manual Start of Displacement or Automatic Trigger on the Audio Level.
  - ES: Inicio manual del desplazamiento o disparo automático según el nivel de audio.
  - [ ] OK    Fix: 

- **`pathModeButton`**
  - EN: Enable Path Mode to Follow Drawn Movement Paths Instead of Direct Lines.
  - ES: Activar el modo Trayectoria para seguir caminos de movimiento dibujados en lugar de líneas directas.
  - [ ] OK    Fix: 

- **`position1`**
  - EN: Object {name} ({unit}).
  - ES: {name} del objeto ({unit}).
  - [ ] OK    Fix: 

- **`position2`**
  - EN: Object {name} ({unit}).
  - ES: {name} del objeto ({unit}).
  - [ ] OK    Fix: 

- **`position3`**
  - EN: Object {name} ({unit}).
  - ES: {name} del objeto ({unit}).
  - [ ] OK    Fix: 

- **`positionJoystick`**
  - EN: Drag to adjust X/Y position in real-time. Returns to center on release.
  - ES: Arrastrar para ajustar la posición X/Y en tiempo real. Vuelve al centro al soltar.
  - [ ] OK    Fix: 

- **`positionZSlider`**
  - EN: Drag to adjust Z (height) position in real-time. Returns to center on release.
  - ES: Arrastrar para ajustar la posición Z (altura) en tiempo real. Vuelve al centro al soltar.
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Input Configuration from backup file.
  - ES: Recargar la configuración de entrada desde archivo de copia de seguridad.
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Input Configuration from file.
  - ES: Recargar la configuración de entrada desde archivo.
  - [ ] OK    Fix: 

- **`reloadSnapshot`**
  - EN: Reload Selected Input Snapshot for All Objects Taking the Scope into Account.
  - ES: Recargar el snapshot de entrada seleccionado para todos los objetos teniendo en cuenta el filtro.
  - [ ] OK    Fix: 

- **`reloadWithoutScope`**
  - EN: Reload Selected Input Snapshot for All Objects Without the Scope.
  - ES: Recargar el snapshot de entrada seleccionado para todos los objetos sin el filtro.
  - [ ] OK    Fix: 

- **`sidelinesActiveButton`**
  - EN: Enable Automatic Muting when Source Approaches Stage Edges. Does Not Apply to Downstage (Front) Edge.
  - ES: Activar el silenciamiento automático cuando la fuente se acerca a los bordes del escenario. No se aplica al borde delantero (lado del público).
  - [ ] OK    Fix: 

- **`sidelinesFringeDial`**
  - EN: Fringe Zone Size in Meters. Outer Half is Full Mute, Inner Half Fades Linearly.
  - ES: Tamaño de la zona de transición en metros. La mitad exterior silencia totalmente, la mitad interior atenúa linealmente.
  - [ ] OK    Fix: 

- **`snapshotSelector`**
  - EN: Select Input Snapshot Without Loading.
  - ES: Seleccionar snapshot de entrada sin cargarlo.
  - [ ] OK    Fix: 

- **`solo`**
  - EN: Listen to Binaural Rendering of this channel.
  - ES: Escuchar el renderizado binaural de este canal.
  - [ ] OK    Fix: 

- **`soloMode`**
  - EN: Single: one input at a time. Multi: multiple inputs simultaneously.
  - ES: Único: una entrada a la vez. Múltiple: varias entradas simultáneamente.
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Input Configuration to file (with backup).
  - ES: Guardar la configuración de entrada en archivo (con copia de seguridad).
  - [ ] OK    Fix: 

- **`storeSnapshot`**
  - EN: Store new Input Snapshot for All Objects.
  - ES: Guardar nuevo snapshot de entrada para todos los objetos.
  - [ ] OK    Fix: 

- **`tiltSlider`**
  - EN: Where is the Object pointing to in the Vertical Plane.
  - ES: Dirección del objeto en el plano vertical.
  - [ ] OK    Fix: 

- **`trackingActiveButton`**
  - EN: Enable or Disable Tracking for Object.
  - ES: Activar o desactivar el tracking para el objeto.
  - [ ] OK    Fix: 

- **`trackingIdSelector`**
  - EN: Tracker ID for Object.
  - ES: ID de tracker para el objeto.
  - [ ] OK    Fix: 

- **`trackingSmoothDial`**
  - EN: Smoothing of Tracking Data for Object.
  - ES: Suavizado de los datos de tracking para el objeto.
  - [ ] OK    Fix: 

- **`updateSnapshot`**
  - EN: Update Selected Input Snapshot (with backup).
  - ES: Actualizar el snapshot de entrada seleccionado (con copia de seguridad).
  - [ ] OK    Fix: 

## `inputs.labels`

- **`admMapping`**
  - EN: ADM:
  - ES: ADM:
  - [ ] OK    Fix: 

- **`amplitudeX`**
  - EN: Amplitude X:
  - ES: Amplitud X:
  - [ ] OK    Fix: 

- **`amplitudeY`**
  - EN: Amplitude Y:
  - ES: Amplitud Y:
  - [ ] OK    Fix: 

- **`amplitudeZ`**
  - EN: Amplitude Z:
  - ES: Amplitud Z:
  - [ ] OK    Fix: 

- **`arrayAttenuation`**
  - EN: Array Attenuation:
  - ES: Atenuación de arreglo:
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Attenuation:
  - ES: Atenuación:
  - [ ] OK    Fix: 

- **`attenuationLaw`**
  - EN: Attenuation Law:
  - ES: Ley de atenuación:
  - [ ] OK    Fix: 

- **`cluster`**
  - EN: Cluster:
  - ES: Grupo:
  - [ ] OK    Fix: 

- **`commonAtten`**
  - EN: Common Atten:
  - ES: Atenuación común:
  - [ ] OK    Fix: 

- **`coord`**
  - EN: Coord:
  - ES: Coord:
  - [ ] OK    Fix: 

- **`curve`**
  - EN: Curve:
  - ES: Curva:
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Delay/Latency:
  - ES: Retardo/Latencia:
  - [ ] OK    Fix: 

- **`destX`**
  - EN: Dest. X:
  - ES: Dest. X:
  - [ ] OK    Fix: 

- **`destY`**
  - EN: Dest. Y:
  - ES: Dest. Y:
  - [ ] OK    Fix: 

- **`destZ`**
  - EN: Dest. Z:
  - ES: Dest. Z:
  - [ ] OK    Fix: 

- **`diffusion`**
  - EN: Diffusion:
  - ES: Difusión:
  - [ ] OK    Fix: 

- **`directivity`**
  - EN: Directivity:
  - ES: Directividad:
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Distance Atten:
  - ES: Atenuación distancia:
  - [ ] OK    Fix: 

- **`distanceRatio`**
  - EN: Distance Ratio:
  - ES: Ratio distancia:
  - [ ] OK    Fix: 

- **`duration`**
  - EN: Duration:
  - ES: Duración:
  - [ ] OK    Fix: 

- **`frequency`**
  - EN: Frequency:
  - ES: Frecuencia:
  - [ ] OK    Fix: 

- **`fringe`**
  - EN: Fringe:
  - ES: Borde:
  - [ ] OK    Fix: 

- **`gain`**
  - EN: Gain:
  - ES: Ganancia:
  - [ ] OK    Fix: 

- **`gyrophone`**
  - EN: Gyrophone:
  - ES: Gyrophone:
  - [ ] OK    Fix: 

- **`heightFactor`**
  - EN: Height Factor:
  - ES: Factor de altura:
  - [ ] OK    Fix: 

- **`hfShelf`**
  - EN: HF Shelf:
  - ES: Atenuación HF:
  - [ ] OK    Fix: 

- **`jitter`**
  - EN: Jitter:
  - ES: Jitter:
  - [ ] OK    Fix: 

- **`max`**
  - EN: Max:
  - ES: Máx:
  - [ ] OK    Fix: 

- **`maxSpeed`**
  - EN: Max Speed:
  - ES: Velocidad Máx:
  - [ ] OK    Fix: 

- **`min`**
  - EN: Min:
  - ES: Mín:
  - [ ] OK    Fix: 

- **`muteMacros`**
  - EN: Mute Macros:
  - ES: Macros Mute:
  - [ ] OK    Fix: 

- **`name`**
  - EN: Name:
  - ES: Nombre:
  - [ ] OK    Fix: 

- **`offsetX`**
  - EN: Offset X:
  - ES: Desplazamiento X:
  - [ ] OK    Fix: 

- **`offsetY`**
  - EN: Offset Y:
  - ES: Desplazamiento Y:
  - [ ] OK    Fix: 

- **`offsetZ`**
  - EN: Offset Z:
  - ES: Desplazamiento Z:
  - [ ] OK    Fix: 

- **`outX`**
  - EN: Out X:
  - ES: Salida X:
  - [ ] OK    Fix: 

- **`outY`**
  - EN: Out Y:
  - ES: Salida Y:
  - [ ] OK    Fix: 

- **`outZ`**
  - EN: Out Z:
  - ES: Salida Z:
  - [ ] OK    Fix: 

- **`peakRatio`**
  - EN: Peak Ratio:
  - ES: Ratio pico:
  - [ ] OK    Fix: 

- **`peakThreshold`**
  - EN: Peak Threshold:
  - ES: Umbral pico:
  - [ ] OK    Fix: 

- **`period`**
  - EN: Period:
  - ES: Período:
  - [ ] OK    Fix: 

- **`phase`**
  - EN: Phase:
  - ES: Fase:
  - [ ] OK    Fix: 

- **`phaseX`**
  - EN: Phase X:
  - ES: Fase X:
  - [ ] OK    Fix: 

- **`phaseY`**
  - EN: Phase Y:
  - ES: Fase Y:
  - [ ] OK    Fix: 

- **`phaseZ`**
  - EN: Phase Z:
  - ES: Fase Z:
  - [ ] OK    Fix: 

- **`positionX`**
  - EN: Position X:
  - ES: Posición X:
  - [ ] OK    Fix: 

- **`positionY`**
  - EN: Position Y:
  - ES: Posición Y:
  - [ ] OK    Fix: 

- **`positionZ`**
  - EN: Position Z:
  - ES: Posición Z:
  - [ ] OK    Fix: 

- **`radius`**
  - EN: Radius:
  - ES: Radio:
  - [ ] OK    Fix: 

- **`rateX`**
  - EN: Rate X:
  - ES: Tasa X:
  - [ ] OK    Fix: 

- **`rateY`**
  - EN: Rate Y:
  - ES: Tasa Y:
  - [ ] OK    Fix: 

- **`rateZ`**
  - EN: Rate Z:
  - ES: Tasa Z:
  - [ ] OK    Fix: 

- **`reset`**
  - EN: Reset:
  - ES: Restablecer:
  - [ ] OK    Fix: 

- **`rotation`**
  - EN: Rotation:
  - ES: Rotación:
  - [ ] OK    Fix: 

- **`shape`**
  - EN: Shape:
  - ES: Forma:
  - [ ] OK    Fix: 

- **`shapeX`**
  - EN: Shape X:
  - ES: Forma X:
  - [ ] OK    Fix: 

- **`shapeY`**
  - EN: Shape Y:
  - ES: Forma Y:
  - [ ] OK    Fix: 

- **`shapeZ`**
  - EN: Shape Z:
  - ES: Forma Z:
  - [ ] OK    Fix: 

- **`slope`**
  - EN: Slope:
  - ES: Pendiente:
  - [ ] OK    Fix: 

- **`slowRatio`**
  - EN: Slow Ratio:
  - ES: Ratio lento:
  - [ ] OK    Fix: 

- **`slowThreshold`**
  - EN: Slow Threshold:
  - ES: Umbral lento:
  - [ ] OK    Fix: 

- **`speedProfile`**
  - EN: Speed Profile:
  - ES: Perfil de velocidad:
  - [ ] OK    Fix: 

- **`threshold`**
  - EN: Threshold:
  - ES: Umbral:
  - [ ] OK    Fix: 

- **`tilt`**
  - EN: Tilt:
  - ES: Inclinación:
  - [ ] OK    Fix: 

- **`trackingId`**
  - EN: Tracking ID:
  - ES: ID Tracking:
  - [ ] OK    Fix: 

- **`trackingSmooth`**
  - EN: Tracking Smooth:
  - ES: Suavizado Tracking:
  - [ ] OK    Fix: 

- **`xyJoystick`**
  - EN: X/Y
  - ES: X/Y
  - [ ] OK    Fix: 

- **`zSlider`**
  - EN: Z
  - ES: Z
  - [ ] OK    Fix: 

## `inputs.lfo`

- **`gyrophone.antiClockwise`**
  - EN: Anti-Clockwise
  - ES: Anti-horario
  - [ ] OK    Fix: 

- **`gyrophone.clockwise`**
  - EN: Clockwise
  - ES: Horario
  - [ ] OK    Fix: 

- **`gyrophone.off`**
  - EN: OFF
  - ES: OFF
  - [ ] OK    Fix: 

- **`shapes.exp`**
  - EN: exp
  - ES: exp
  - [ ] OK    Fix: 

- **`shapes.keystone`**
  - EN: keystone
  - ES: trapecio
  - [ ] OK    Fix: 

- **`shapes.log`**
  - EN: log
  - ES: log
  - [ ] OK    Fix: 

- **`shapes.off`**
  - EN: OFF
  - ES: OFF
  - [ ] OK    Fix: 

- **`shapes.random`**
  - EN: random
  - ES: aleatorio
  - [ ] OK    Fix: 

- **`shapes.sawtooth`**
  - EN: sawtooth
  - ES: sierra
  - [ ] OK    Fix: 

- **`shapes.sine`**
  - EN: sine
  - ES: seno
  - [ ] OK    Fix: 

- **`shapes.square`**
  - EN: square
  - ES: cuadrada
  - [ ] OK    Fix: 

- **`shapes.triangle`**
  - EN: triangle
  - ES: triángulo
  - [ ] OK    Fix: 

## `inputs.liveSource`

- **`linear`**
  - EN: linear
  - ES: lineal
  - [ ] OK    Fix: 

- **`log`**
  - EN: log
  - ES: log
  - [ ] OK    Fix: 

- **`sine`**
  - EN: sine
  - ES: seno
  - [ ] OK    Fix: 

## `inputs.messages`

- **`assignedCluster`**
  - EN: Input {channel} assigned to Cluster {cluster}
  - ES: Entrada {channel} asignada al Grupo {cluster}
  - [ ] OK    Fix: 

- **`backupLoaded`**
  - EN: Input configuration loaded from backup.
  - ES: Configuración entradas cargada desde respaldo.
  - [ ] OK    Fix: 

- **`configExported`**
  - EN: Input configuration exported.
  - ES: Configuración entradas exportada.
  - [ ] OK    Fix: 

- **`configImported`**
  - EN: Input configuration imported.
  - ES: Configuración entradas importada.
  - [ ] OK    Fix: 

- **`configLoaded`**
  - EN: Input configuration loaded.
  - ES: Configuración entradas cargada.
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Input configuration saved.
  - ES: Configuración entradas guardada.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - ES: Error: {error}
  - [ ] OK    Fix: 

- **`noSnapshotSelected`**
  - EN: No snapshot selected.
  - ES: Ningún snapshot seleccionado.
  - [ ] OK    Fix: 

- **`scopeConfigured`**
  - EN: Scope configured for next snapshot.
  - ES: Alcance configurado para el próximo snapshot.
  - [ ] OK    Fix: 

- **`scopeSaved`**
  - EN: Snapshot scope saved.
  - ES: Alcance del snapshot guardado.
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - ES: Por favor seleccione primero una carpeta de proyecto en Config Sistema.
  - [ ] OK    Fix: 

- **`setSingle`**
  - EN: Input {channel} set to Single
  - ES: Entrada {channel} configurada como Individual
  - [ ] OK    Fix: 

- **`snapshotDeleted`**
  - EN: Snapshot '{name}' deleted.
  - ES: Snapshot '{name}' eliminado.
  - [ ] OK    Fix: 

- **`snapshotLoaded`**
  - EN: Snapshot '{name}' loaded.
  - ES: Snapshot '{name}' cargado.
  - [ ] OK    Fix: 

- **`snapshotLoadedWithoutScope`**
  - EN: Snapshot '{name}' loaded (without scope).
  - ES: Snapshot '{name}' cargado (sin alcance).
  - [ ] OK    Fix: 

- **`snapshotStored`**
  - EN: Snapshot '{name}' stored.
  - ES: Snapshot '{name}' guardado.
  - [ ] OK    Fix: 

- **`snapshotUpdated`**
  - EN: Snapshot '{name}' updated.
  - ES: Snapshot '{name}' actualizado.
  - [ ] OK    Fix: 

- **`trackingDisabled`**
  - EN: Tracking disabled for Input {channel}
  - ES: Tracking desactivado para Entrada {channel}
  - [ ] OK    Fix: 

- **`trackingSwitched`**
  - EN: Tracking switched from Input {from} to Input {to}
  - ES: Tracking cambiado de Entrada {from} a Entrada {to}
  - [ ] OK    Fix: 

## `inputs.muteMacros`

- **`invertMutes`**
  - EN: INVERT MUTES
  - ES: INVERTIR MUTES
  - [ ] OK    Fix: 

- **`muteAll`**
  - EN: MUTE ALL
  - ES: SILENCIAR TODO
  - [ ] OK    Fix: 

- **`muteArrayPrefix`**
  - EN: MUTE ARRAY
  - ES: SILENCIAR ARREGLO
  - [ ] OK    Fix: 

- **`muteEven`**
  - EN: MUTE EVEN
  - ES: SILENCIAR PARES
  - [ ] OK    Fix: 

- **`muteOdd`**
  - EN: MUTE ODD
  - ES: SILENCIAR IMPARES
  - [ ] OK    Fix: 

- **`selectMacro`**
  - EN: Select Macro...
  - ES: Seleccionar macro...
  - [ ] OK    Fix: 

- **`unmuteAll`**
  - EN: UNMUTE ALL
  - ES: ACTIVAR TODO
  - [ ] OK    Fix: 

- **`unmuteArrayPrefix`**
  - EN: UNMUTE ARRAY
  - ES: ACTIVAR ARREGLO
  - [ ] OK    Fix: 

## `inputs.prefixes`

- **`delay`**
  - EN: Delay:
  - ES: Retardo:
  - [ ] OK    Fix: 

## `inputs.sections`

- **`automotion`**
  - EN: AutomOtion
  - ES: AutomOción
  - [ ] OK    Fix: 

## `inputs.snapshots`

- **`selectSnapshot`**
  - EN: Select Snapshot...
  - ES: Seleccionar Snapshot...
  - [ ] OK    Fix: 

## `inputs.tabs`

- **`gradientMaps`**
  - EN: Gradient Maps
  - ES: Mapas de Gradiente
  - [ ] OK    Fix: 

- **`inputParams`**
  - EN: Input Parameters
  - ES: Parámetros de entrada
  - [ ] OK    Fix: 

- **`liveSourceHackoustics`**
  - EN: Live Source & Hackoustics
  - ES: Fuente en Vivo & Hackústica
  - [ ] OK    Fix: 

- **`movements`**
  - EN: Movements
  - ES: Movimientos
  - [ ] OK    Fix: 

- **`sampler`**
  - EN: Sampler
  - ES: Sampler
  - [ ] OK    Fix: 

- **`visualisation`**
  - EN: Visualisation
  - ES: Visualización
  - [ ] OK    Fix: 

## `inputs.toggles`

- **`absolute`**
  - EN: Absolute
  - ES: Absoluto
  - [ ] OK    Fix: 

- **`acousticPrecedence`**
  - EN: Acoustic Precedence
  - ES: Precedencia acústica
  - [ ] OK    Fix: 

- **`attenuationLawLog`**
  - EN: Log
  - ES: Log
  - [ ] OK    Fix: 

- **`constraintROff`**
  - EN: Constraint R: OFF
  - ES: Restricción R: OFF
  - [ ] OK    Fix: 

- **`constraintROn`**
  - EN: Constraint R: ON
  - ES: Restricción R: ON
  - [ ] OK    Fix: 

- **`constraintXOff`**
  - EN: Constraint X: OFF
  - ES: Restricción X: OFF
  - [ ] OK    Fix: 

- **`constraintXOn`**
  - EN: Constraint X: ON
  - ES: Restricción X: ON
  - [ ] OK    Fix: 

- **`constraintYOff`**
  - EN: Constraint Y: OFF
  - ES: Restricción Y: OFF
  - [ ] OK    Fix: 

- **`constraintYOn`**
  - EN: Constraint Y: ON
  - ES: Restricción Y: ON
  - [ ] OK    Fix: 

- **`constraintZOff`**
  - EN: Constraint Z: OFF
  - ES: Restricción Z: OFF
  - [ ] OK    Fix: 

- **`constraintZOn`**
  - EN: Constraint Z: ON
  - ES: Restricción Z: ON
  - [ ] OK    Fix: 

- **`flipXOff`**
  - EN: Flip X: OFF
  - ES: Invertir X: OFF
  - [ ] OK    Fix: 

- **`flipXOn`**
  - EN: Flip X: ON
  - ES: Invertir X: ON
  - [ ] OK    Fix: 

- **`flipYOff`**
  - EN: Flip Y: OFF
  - ES: Invertir Y: OFF
  - [ ] OK    Fix: 

- **`flipYOn`**
  - EN: Flip Y: ON
  - ES: Invertir Y: ON
  - [ ] OK    Fix: 

- **`flipZOff`**
  - EN: Flip Z: OFF
  - ES: Invertir Z: OFF
  - [ ] OK    Fix: 

- **`flipZOn`**
  - EN: Flip Z: ON
  - ES: Invertir Z: ON
  - [ ] OK    Fix: 

- **`floorReflectionsOff`**
  - EN: Floor Reflections: OFF
  - ES: Reflexiones de piso: OFF
  - [ ] OK    Fix: 

- **`floorReflectionsOn`**
  - EN: Floor Reflections: ON
  - ES: Reflexiones de piso: ON
  - [ ] OK    Fix: 

- **`highShelfOff`**
  - EN: High Shelf: OFF
  - ES: Shelving agudo: OFF
  - [ ] OK    Fix: 

- **`highShelfOn`**
  - EN: High Shelf: ON
  - ES: Shelving agudo: ON
  - [ ] OK    Fix: 

- **`lfoOff`**
  - EN: L.F.O: OFF
  - ES: L.F.O: APAG.
  - [ ] OK    Fix: 

- **`lfoOn`**
  - EN: L.F.O: ON
  - ES: L.F.O: ENC.
  - [ ] OK    Fix: 

- **`liveSourceTamerOff`**
  - EN: Live Source Tamer: OFF
  - ES: Domador: OFF
  - [ ] OK    Fix: 

- **`liveSourceTamerOn`**
  - EN: Live Source Tamer: ON
  - ES: Domador: ON
  - [ ] OK    Fix: 

- **`lowCutOff`**
  - EN: Low Cut: OFF
  - ES: Corte bajo: OFF
  - [ ] OK    Fix: 

- **`lowCutOn`**
  - EN: Low Cut: ON
  - ES: Corte bajo: ON
  - [ ] OK    Fix: 

- **`lsPeakOff`**
  - EN: Peak: OFF
  - ES: Pico: OFF
  - [ ] OK    Fix: 

- **`lsPeakOn`**
  - EN: Peak: ON
  - ES: Pico: ON
  - [ ] OK    Fix: 

- **`lsSlowOff`**
  - EN: Slow: OFF
  - ES: Lento: OFF
  - [ ] OK    Fix: 

- **`lsSlowOn`**
  - EN: Slow: ON
  - ES: Lento: ON
  - [ ] OK    Fix: 

- **`manual`**
  - EN: Manual
  - ES: Manual
  - [ ] OK    Fix: 

- **`maxSpeedOff`**
  - EN: Max Speed: OFF
  - ES: Velocidad máx: OFF
  - [ ] OK    Fix: 

- **`maxSpeedOn`**
  - EN: Max Speed: ON
  - ES: Velocidad máx: ON
  - [ ] OK    Fix: 

- **`minimalLatency`**
  - EN: Minimal Latency
  - ES: Latencia mínima
  - [ ] OK    Fix: 

- **`pathModeOff`**
  - EN: Path Mode: OFF
  - ES: Modo ruta: OFF
  - [ ] OK    Fix: 

- **`pathModeOn`**
  - EN: Path Mode: ON
  - ES: Modo ruta: ON
  - [ ] OK    Fix: 

- **`relative`**
  - EN: Relative
  - ES: Relativo
  - [ ] OK    Fix: 

- **`return`**
  - EN: Return
  - ES: Regresar
  - [ ] OK    Fix: 

- **`reverbSendsMuted`**
  - EN: Sends to Reverbs: Muted
  - ES: Envíos Reverb: Silenciados
  - [ ] OK    Fix: 

- **`reverbSendsUnmuted`**
  - EN: Sends to Reverbs: Unmuted
  - ES: Envíos Reverb: Activos
  - [ ] OK    Fix: 

- **`sidelinesOff`**
  - EN: Sidelines Off
  - ES: Líneas laterales Off
  - [ ] OK    Fix: 

- **`sidelinesOn`**
  - EN: Sidelines On
  - ES: Líneas laterales On
  - [ ] OK    Fix: 

- **`stay`**
  - EN: Stay
  - ES: Quedarse
  - [ ] OK    Fix: 

- **`trackingOff`**
  - EN: Tracking: OFF
  - ES: Seguimiento: OFF
  - [ ] OK    Fix: 

- **`trackingOn`**
  - EN: Tracking: ON
  - ES: Seguimiento: ON
  - [ ] OK    Fix: 

- **`triggered`**
  - EN: Triggered
  - ES: Activado
  - [ ] OK    Fix: 

## `inputs.visualisation`

- **`delay`**
  - EN: delay
  - ES: retardo
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: HF
damping
  - ES: atenuación
HF
  - [ ] OK    Fix: 

- **`level`**
  - EN: level
  - ES: nivel
  - [ ] OK    Fix: 

## `levelMeter`

- **`inputs`**
  - EN: Inputs
  - ES: Entradas
  - [ ] OK    Fix: 

- **`outputs`**
  - EN: Outputs
  - ES: Salidas
  - [ ] OK    Fix: 

- **`windowTitle`**
  - EN: Level Meters
  - ES: Medidores de nivel
  - [ ] OK    Fix: 

## `levelMeter.buttons`

- **`clearSolo`**
  - EN: Clear Solo
  - ES: Borrar solo
  - [ ] OK    Fix: 

- **`soloModeMulti`**
  - EN: Multi
  - ES: Multi
  - [ ] OK    Fix: 

- **`soloModeSingle`**
  - EN: Single
  - ES: Single
  - [ ] OK    Fix: 

## `levelMeter.tooltips`

- **`clearSolo`**
  - EN: Disengage all solo toggles
  - ES: Desactivar todos los solos
  - [ ] OK    Fix: 

- **`solo`**
  - EN: Display the contribution of the input to all outputs in the Level Meter display (in Single mode) and play binaural render of soloed inputs
  - ES: Mostrar la contribución de la entrada a todas las salidas en el medidor de niveles (en modo Solo único) y reproducir el renderizado binaural de las entradas en solo
  - [ ] OK    Fix: 

- **`soloMode`**
  - EN: Single: one input at a time. Multi: multiple inputs simultaneously.
  - ES: Único: una entrada a la vez. Múltiple: varias entradas simultáneamente.
  - [ ] OK    Fix: 

## `map`

- **`detachedMessage`**
  - EN: The map is displayed in a separate window.
  - ES: El mapa se muestra en una ventana separada.
  - [ ] OK    Fix: 

- **`reattach`**
  - EN: Re-attach Map
  - ES: Reinsertar mapa
  - [ ] OK    Fix: 

## `map.buttons`

- **`fitInputs`**
  - EN: Fit All Inputs to Screen
  - ES: Ajustar todas las entradas a pantalla
  - [ ] OK    Fix: 

- **`fitStage`**
  - EN: Fit Stage to Screen
  - ES: Ajustar escenario a pantalla
  - [ ] OK    Fix: 

- **`hideLevels`**
  - EN: Hide Levels
  - ES: Ocultar niveles
  - [ ] OK    Fix: 

- **`showLevels`**
  - EN: Show Levels
  - ES: Mostrar niveles
  - [ ] OK    Fix: 

## `map.labels`

- **`reverbPrefix`**
  - EN: R
  - ES: R
  - [ ] OK    Fix: 

## `map.messages`

- **`assignedCluster`**
  - EN: Input {channel} assigned to Cluster {cluster}
  - ES: Entrada {channel} asignada al Cluster {cluster}
  - [ ] OK    Fix: 

- **`assignedClusterMulti`**
  - EN: {count} inputs assigned to Cluster {cluster}
  - ES: {count} entradas asignadas al Cluster {cluster}
  - [ ] OK    Fix: 

- **`clusterBrokenUp`**
  - EN: Cluster {cluster} broken up
  - ES: Cluster {cluster} disuelto
  - [ ] OK    Fix: 

- **`setSingle`**
  - EN: Input {channel} removed from cluster
  - ES: Entrada {channel} eliminada del cluster
  - [ ] OK    Fix: 

- **`setSingleMulti`**
  - EN: {count} inputs removed from clusters
  - ES: {count} entradas eliminadas de los clusters
  - [ ] OK    Fix: 

## `map.tooltips`

- **`detach`**
  - EN: Detach the map into a separate window for dual-screen setups
  - ES: Separar el mapa en una ventana independiente para configuraciones de doble pantalla
  - [ ] OK    Fix: 

- **`fitInputs`**
  - EN: Adjust the zoom and panning to fit all visible inputs on the display
  - ES: Ajustar el zoom y panorámica para mostrar todas las entradas visibles
  - [ ] OK    Fix: 

- **`fitStage`**
  - EN: Adjust the zoom and panning to fit the stage on the display
  - ES: Ajustar el zoom y panorámica para mostrar el escenario
  - [ ] OK    Fix: 

- **`levels`**
  - EN: Display the levels for inputs and outputs on the map
  - ES: Mostrar los niveles de entradas y salidas en el mapa
  - [ ] OK    Fix: 

## `network.buttons`

- **`add`**
  - EN: ADD
  - ES: AÑADIR
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - ES: Exportar
  - [ ] OK    Fix: 

- **`findMyRemote`**
  - EN: Find My Remote
  - ES: Buscar mi control remoto
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - ES: Importar
  - [ ] OK    Fix: 

- **`openLogWindow`**
  - EN: Open Log Window
  - ES: Abrir ventana de registro
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Backup
  - ES: Recargar respaldo
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Network Config
  - ES: Recargar config red
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Network Config
  - ES: Guardar config red
  - [ ] OK    Fix: 

## `network.dialogs`

- **`exportConfig`**
  - EN: Export Network Configuration
  - ES: Exportar configuración de red
  - [ ] OK    Fix: 

- **`findMyRemoteMessage`**
  - EN: Enter the password for your remote device:
  - ES: Introduzca la contraseña de su mando:
  - [ ] OK    Fix: 

- **`findMyRemotePassword`**
  - EN: Password:
  - ES: Contraseña:
  - [ ] OK    Fix: 

- **`findMyRemoteTitle`**
  - EN: Find My Remote
  - ES: Encontrar mi mando
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Network Configuration
  - ES: Importar configuración de red
  - [ ] OK    Fix: 

- **`removeTargetMessage`**
  - EN: Delete target '{name}'?
  - ES: ¿Eliminar el destino '{name}'?
  - [ ] OK    Fix: 

- **`removeTargetTitle`**
  - EN: Remove Target
  - ES: Eliminar destino
  - [ ] OK    Fix: 

- **`trackingConflictsContinue`**
  - EN: Continue
  - ES: Continuar
  - [ ] OK    Fix: 

- **`trackingConflictsMessage`**
  - EN: 
Only one tracked input per cluster is allowed. If you continue, tracking will be kept only for the first input in each cluster.
  - ES: 
Solo se permite una entrada con tracking por cluster. Si continúa, el tracking se mantendrá solo para la primera entrada de cada cluster.
  - [ ] OK    Fix: 

- **`trackingConflictsTitle`**
  - EN: Tracking Conflicts Detected
  - ES: Conflictos de tracking detectados
  - [ ] OK    Fix: 

## `network.help`

- **`addTarget`**
  - EN: Add new network target.
  - ES: Añadir nuevo destino de red.
  - [ ] OK    Fix: 

- **`admMapping`**
  - EN: Select an ADM-OSC mapping to configure. Cart = Cartesian (xyz), Polar = spherical (aed).
  - ES: Seleccionar una asignación ADM-OSC a configurar. Cart = Cartesiano (xyz), Polar = esférico (aed).
  - [ ] OK    Fix: 

- **`admMappingPanel`**
  - EN: Drag dots to edit mapping. Click axis title to swap, click Flip to invert. Hold Shift to edit both sides symmetrically.
  - ES: Arrastrar puntos para editar el mapeo. Hacer clic en el título del eje para intercambiar, clic en Flip para invertir. Mantener Mayús para editar ambos lados simétricamente.
  - [ ] OK    Fix: 

- **`currentIP`**
  - EN: IP address of the Processor.
  - ES: Dirección IP del procesador.
  - [ ] OK    Fix: 

- **`dataMode`**
  - EN: Select UDP or TCP data transmission.
  - ES: Seleccionar transmisión de datos UDP o TCP.
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Network Configuration to file.
  - ES: Exportar la configuración de red a un archivo.
  - [ ] OK    Fix: 

- **`findMyRemote`**
  - EN: Make your Remote Flash and Buzz to Find it.
  - ES: Haga parpadear y vibrar su mando para encontrarlo.
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Network Configuration from file.
  - ES: Importar la configuración de red desde un archivo.
  - [ ] OK    Fix: 

- **`networkInterface`**
  - EN: Select the Network Interface.
  - ES: Seleccionar la interfaz de red.
  - [ ] OK    Fix: 

- **`openLogWindow`**
  - EN: Open Network Logging window.
  - ES: Abrir la ventana de registro de red.
  - [ ] OK    Fix: 

- **`oscQueryEnable`**
  - EN: Enable/disable OSC Query server for automatic parameter discovery via HTTP/WebSocket.
  - ES: Activar/desactivar el servidor OSC Query para el descubrimiento automático de parámetros vía HTTP/WebSocket.
  - [ ] OK    Fix: 

- **`oscQueryPort`**
  - EN: HTTP port for OSC Query discovery. Other apps can browse parameters at http://localhost:<port>/
  - ES: Puerto HTTP para el descubrimiento OSC Query. Otras aplicaciones pueden explorar los parámetros en http://localhost:<port>/
  - [ ] OK    Fix: 

- **`oscSourceFilter`**
  - EN: Filter incoming OSC: Accept All sources or only Registered connections with Rx enabled.
  - ES: Filtrar OSC entrante: aceptar todas las fuentes o solo conexiones registradas con Rx activado.
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Select the Protocol: DISABLED, OSC, REMOTE, or ADM-OSC.
  - ES: Seleccionar el protocolo: DISABLED, OSC, REMOTE o ADM-OSC.
  - [ ] OK    Fix: 

- **`psnInterface`**
  - EN: Network interface for PSN multicast reception
  - ES: Interfaz de red para la recepción multicast PSN
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Network Configuration from backup file.
  - ES: Recargar la configuración de red desde el archivo de respaldo.
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Network Configuration from file.
  - ES: Recargar la configuración de red desde un archivo.
  - [ ] OK    Fix: 

- **`removeTarget`**
  - EN: Delete this Network Target.
  - ES: Eliminar este destino de red.
  - [ ] OK    Fix: 

- **`rxEnable`**
  - EN: Enable or Disable Data Reception.
  - ES: Activar o desactivar la recepción de datos.
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Network Configuration to file.
  - ES: Guardar la configuración de red en un archivo.
  - [ ] OK    Fix: 

- **`targetIP`**
  - EN: IP Address of the Target (use 127.0.0.1 for local host).
  - ES: Dirección IP del destino (usar 127.0.0.1 para el host local).
  - [ ] OK    Fix: 

- **`targetName`**
  - EN: Network Target Name.
  - ES: Nombre del destino de red.
  - [ ] OK    Fix: 

- **`tcpPort`**
  - EN: TCP Receive Port of the Processor.
  - ES: Puerto de recepción TCP del procesador.
  - [ ] OK    Fix: 

- **`trackingEnabled`**
  - EN: Enable or Disable Incoming Tracking data processing.
  - ES: Activar o desactivar el procesamiento de datos de tracking entrantes.
  - [ ] OK    Fix: 

- **`trackingFlipX`**
  - EN: Invert Axis of Tracking X Coordinate.
  - ES: Invertir el eje de la coordenada X del tracking.
  - [ ] OK    Fix: 

- **`trackingFlipY`**
  - EN: Invert Axis of Tracking Y Coordinate.
  - ES: Invertir el eje de la coordenada Y del tracking.
  - [ ] OK    Fix: 

- **`trackingFlipZ`**
  - EN: Invert Axis of Tracking Z Coordinate.
  - ES: Invertir el eje de la coordenada Z del tracking.
  - [ ] OK    Fix: 

- **`trackingOffsetX`**
  - EN: Offset Tracking X Coordinate.
  - ES: Desplazamiento de la coordenada X del tracking.
  - [ ] OK    Fix: 

- **`trackingOffsetY`**
  - EN: Offset Tracking Y Coordinate.
  - ES: Desplazamiento de la coordenada Y del tracking.
  - [ ] OK    Fix: 

- **`trackingOffsetZ`**
  - EN: Offset Tracking Z Coordinate.
  - ES: Desplazamiento de la coordenada Z del tracking.
  - [ ] OK    Fix: 

- **`trackingOscPath`**
  - EN: OSC Path for the Tracking in OSC Mode (starts with a /)
  - ES: Ruta OSC para el tracking en modo OSC (comienza con /)
  - [ ] OK    Fix: 

- **`trackingPort`**
  - EN: Specify the Port to receive Tracking data.
  - ES: Especificar el puerto para recibir datos de tracking.
  - [ ] OK    Fix: 

- **`trackingProtocol`**
  - EN: Select the type of Tracking Protocol.
  - ES: Seleccionar el tipo de protocolo de tracking.
  - [ ] OK    Fix: 

- **`trackingScaleX`**
  - EN: Scale Tracking X Coordinate.
  - ES: Escala de la coordenada X del tracking.
  - [ ] OK    Fix: 

- **`trackingScaleY`**
  - EN: Scale Tracking Y Coordinate.
  - ES: Escala de la coordenada Y del tracking.
  - [ ] OK    Fix: 

- **`trackingScaleZ`**
  - EN: Scale Tracking Z Coordinate.
  - ES: Escala de la coordenada Z del tracking.
  - [ ] OK    Fix: 

- **`txEnable`**
  - EN: Enable or Disable Data Transmission.
  - ES: Activar o desactivar la transmisión de datos.
  - [ ] OK    Fix: 

- **`txPort`**
  - EN: Transmit Port for this Target.
  - ES: Puerto de transmisión para este destino.
  - [ ] OK    Fix: 

- **`udpPort`**
  - EN: UDP Receive Port of the Processor.
  - ES: Puerto de recepción UDP del procesador.
  - [ ] OK    Fix: 

## `network.labels`

- **`admMapping`**
  - EN: Mapping:
  - ES: Asignación:
  - [ ] OK    Fix: 

- **`currentIPv4`**
  - EN: Current IPv4:
  - ES: IPv4 actual:
  - [ ] OK    Fix: 

- **`interface`**
  - EN: Network Interface:
  - ES: Interfaz de red:
  - [ ] OK    Fix: 

- **`localhost`**
  - EN: Localhost (127.0.0.1)
  - ES: Localhost (127.0.0.1)
  - [ ] OK    Fix: 

- **`mqttHost`**
  - EN: Host:
  - ES: Host:
  - [ ] OK    Fix: 

- **`mqttJsonQ`**
  - EN: Q:
  - ES: Q:
  - [ ] OK    Fix: 

- **`mqttJsonX`**
  - EN: X:
  - ES: X:
  - [ ] OK    Fix: 

- **`mqttJsonY`**
  - EN: Y:
  - ES: Y:
  - [ ] OK    Fix: 

- **`mqttJsonZ`**
  - EN: Z:
  - ES: Z:
  - [ ] OK    Fix: 

- **`mqttTagIds`**
  - EN: Tag IDs...
  - ES: IDs Tag...
  - [ ] OK    Fix: 

- **`mqttTopic`**
  - EN: Topic:
  - ES: Tema:
  - [ ] OK    Fix: 

- **`notAvailable`**
  - EN: Not available
  - ES: No disponible
  - [ ] OK    Fix: 

- **`offsetX`**
  - EN: Offset X:
  - ES: Desplazamiento X:
  - [ ] OK    Fix: 

- **`offsetY`**
  - EN: Offset Y:
  - ES: Desplazamiento Y:
  - [ ] OK    Fix: 

- **`offsetZ`**
  - EN: Offset Z:
  - ES: Desplazamiento Z:
  - [ ] OK    Fix: 

- **`oscPath`**
  - EN: OSC Path:
  - ES: OSC Path:
  - [ ] OK    Fix: 

- **`oscQuery`**
  - EN: OSC Query:
  - ES: OSC Query:
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Protocol:
  - ES: Protocolo:
  - [ ] OK    Fix: 

- **`psnInterface`**
  - EN: PSN Interface:
  - ES: PSN Interface:
  - [ ] OK    Fix: 

- **`rxPort`**
  - EN: Rx Port:
  - ES: Puerto Rx:
  - [ ] OK    Fix: 

- **`scaleX`**
  - EN: Scale X:
  - ES: Escala X:
  - [ ] OK    Fix: 

- **`scaleY`**
  - EN: Scale Y:
  - ES: Escala Y:
  - [ ] OK    Fix: 

- **`scaleZ`**
  - EN: Scale Z:
  - ES: Escala Z:
  - [ ] OK    Fix: 

- **`tcpPort`**
  - EN: TCP Port:
  - ES: Puerto TCP:
  - [ ] OK    Fix: 

- **`udpPort`**
  - EN: UDP Port:
  - ES: Puerto UDP:
  - [ ] OK    Fix: 

## `network.messages`

- **`configExported`**
  - EN: Network configuration exported.
  - ES: Configuración de red exportada.
  - [ ] OK    Fix: 

- **`configImported`**
  - EN: Network configuration imported.
  - ES: Configuración de red importada.
  - [ ] OK    Fix: 

- **`configLoadedFromBackup`**
  - EN: Network configuration loaded from backup.
  - ES: Configuración de red cargada desde respaldo.
  - [ ] OK    Fix: 

- **`configNotFound`**
  - EN: Network config file not found.
  - ES: Archivo de configuración de red no encontrado.
  - [ ] OK    Fix: 

- **`configReloaded`**
  - EN: Network configuration reloaded.
  - ES: Configuración de red recargada.
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Network configuration saved.
  - ES: Configuración de red guardada.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - ES: Error: {error}
  - [ ] OK    Fix: 

- **`findDeviceSent`**
  - EN: Find Device command sent.
  - ES: Comando Buscar dispositivo enviado.
  - [ ] OK    Fix: 

- **`maxTargetsReached`**
  - EN: Maximum Number of Targets/Servers Reached.
  - ES: Número máximo de destinos/servidores alcanzado.
  - [ ] OK    Fix: 

- **`noBackupFound`**
  - EN: No backup files found.
  - ES: No se encontraron archivos de respaldo.
  - [ ] OK    Fix: 

- **`onlyOneRemote`**
  - EN: Only one REMOTE connection is allowed.
  - ES: Solo se permite una conexión REMOTO.
  - [ ] OK    Fix: 

- **`oscManagerError`**
  - EN: Error: OSC Manager not available
  - ES: Error: gestor OSC no disponible
  - [ ] OK    Fix: 

- **`passwordEmpty`**
  - EN: Password cannot be empty.
  - ES: La contraseña no puede estar vacía.
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - ES: Por favor seleccione primero una carpeta de proyecto en Config Sistema.
  - [ ] OK    Fix: 

## `network.protocols`

- **`admOsc`**
  - EN: ADM-OSC
  - ES: ADM-OSC
  - [ ] OK    Fix: 

- **`disabled`**
  - EN: DISABLED
  - ES: DESACTIVADO
  - [ ] OK    Fix: 

- **`mqtt`**
  - EN: MQTT
  - ES: MQTT
  - [ ] OK    Fix: 

- **`osc`**
  - EN: OSC
  - ES: OSC
  - [ ] OK    Fix: 

- **`psn`**
  - EN: PosiStageNet (PSN)
  - ES: PosiStageNet (PSN)
  - [ ] OK    Fix: 

- **`qlab`**
  - EN: QLab
  - ES: QLab
  - [ ] OK    Fix: 

- **`remote`**
  - EN: Remote
  - ES: Remoto
  - [ ] OK    Fix: 

- **`rttrp`**
  - EN: RTTrP
  - ES: RTTrP
  - [ ] OK    Fix: 

- **`tcp`**
  - EN: TCP
  - ES: TCP
  - [ ] OK    Fix: 

- **`udp`**
  - EN: UDP
  - ES: UDP
  - [ ] OK    Fix: 

## `network.sections`

- **`admOsc`**
  - EN: ADM-OSC Mappings
  - ES: Asignaciones ADM-OSC
  - [ ] OK    Fix: 

- **`connections`**
  - EN: Network Connections
  - ES: Conexiones de red
  - [ ] OK    Fix: 

- **`network`**
  - EN: Network
  - ES: Red
  - [ ] OK    Fix: 

- **`tracking`**
  - EN: Tracking
  - ES: Seguimiento
  - [ ] OK    Fix: 

## `network.table`

- **`defaultTarget`**
  - EN: Target {num}
  - ES: Destino {num}
  - [ ] OK    Fix: 

- **`ipv4Address`**
  - EN: IPv4 Address
  - ES: Dirección IPv4
  - [ ] OK    Fix: 

- **`mode`**
  - EN: Mode
  - ES: Modo
  - [ ] OK    Fix: 

- **`name`**
  - EN: Name
  - ES: Nombre
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Protocol
  - ES: Protocolo
  - [ ] OK    Fix: 

- **`rx`**
  - EN: Rx
  - ES: Rx
  - [ ] OK    Fix: 

- **`tx`**
  - EN: Tx
  - ES: Tx
  - [ ] OK    Fix: 

- **`txPort`**
  - EN: Tx Port
  - ES: Puerto Tx
  - [ ] OK    Fix: 

## `network.toggles`

- **`disabled`**
  - EN: Disabled
  - ES: Desactivado
  - [ ] OK    Fix: 

- **`enabled`**
  - EN: Enabled
  - ES: Activado
  - [ ] OK    Fix: 

- **`flipXOff`**
  - EN: Flip X: OFF
  - ES: Invertir X: OFF
  - [ ] OK    Fix: 

- **`flipXOn`**
  - EN: Flip X: ON
  - ES: Invertir X: ON
  - [ ] OK    Fix: 

- **`flipYOff`**
  - EN: Flip Y: OFF
  - ES: Invertir Y: OFF
  - [ ] OK    Fix: 

- **`flipYOn`**
  - EN: Flip Y: ON
  - ES: Invertir Y: ON
  - [ ] OK    Fix: 

- **`flipZOff`**
  - EN: Flip Z: OFF
  - ES: Invertir Z: OFF
  - [ ] OK    Fix: 

- **`flipZOn`**
  - EN: Flip Z: ON
  - ES: Invertir Z: ON
  - [ ] OK    Fix: 

- **`off`**
  - EN: OFF
  - ES: OFF
  - [ ] OK    Fix: 

- **`on`**
  - EN: ON
  - ES: ON
  - [ ] OK    Fix: 

- **`oscFilterAcceptAll`**
  - EN: OSC Filter: Accept All
  - ES: Filtro OSC: Aceptar todos
  - [ ] OK    Fix: 

- **`oscFilterRegisteredOnly`**
  - EN: OSC Filter: Registered Only
  - ES: Filtro OSC: Solo registrados
  - [ ] OK    Fix: 

- **`trackingOff`**
  - EN: Tracking: OFF
  - ES: Seguimiento: OFF
  - [ ] OK    Fix: 

- **`trackingOn`**
  - EN: Tracking: ON
  - ES: Seguimiento: ON
  - [ ] OK    Fix: 

## `networkLog`

- **`windowTitle`**
  - EN: Network Log
  - ES: Registro de red
  - [ ] OK    Fix: 

## `networkLog.columns`

- **`address`**
  - EN: Address
  - ES: Dirección
  - [ ] OK    Fix: 

- **`arguments`**
  - EN: Arguments
  - ES: Argumentos
  - [ ] OK    Fix: 

- **`direction`**
  - EN: Dir
  - ES: Dir
  - [ ] OK    Fix: 

- **`ip`**
  - EN: IP
  - ES: IP
  - [ ] OK    Fix: 

- **`origin`**
  - EN: Origin
  - ES: Origen
  - [ ] OK    Fix: 

- **`port`**
  - EN: Port
  - ES: Puerto
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Protocol
  - ES: Protocolo
  - [ ] OK    Fix: 

- **`time`**
  - EN: Time
  - ES: Hora
  - [ ] OK    Fix: 

- **`transport`**
  - EN: Trans
  - ES: Trans
  - [ ] OK    Fix: 

## `networkLog.controls`

- **`clear`**
  - EN: CLEAR
  - ES: BORRAR
  - [ ] OK    Fix: 

- **`export`**
  - EN: EXPORT
  - ES: EXPORTAR
  - [ ] OK    Fix: 

- **`hideHeartbeat`**
  - EN: Hide Heartbeat
  - ES: Ocultar Heartbeat
  - [ ] OK    Fix: 

- **`logging`**
  - EN: Logging
  - ES: Registro
  - [ ] OK    Fix: 

## `networkLog.dialogs`

- **`exportCompleteMessage`**
  - EN: Log exported to: {path}
  - ES: Registro exportado a: {path}
  - [ ] OK    Fix: 

- **`exportCompleteTitle`**
  - EN: Export Complete
  - ES: Exportación completada
  - [ ] OK    Fix: 

- **`exportFailedMessage`**
  - EN: Could not write to file: {path}
  - ES: No se pudo escribir en el archivo: {path}
  - [ ] OK    Fix: 

- **`exportFailedTitle`**
  - EN: Export Failed
  - ES: Error de exportación
  - [ ] OK    Fix: 

## `networkLog.exportMenu`

- **`exportAll`**
  - EN: Export All
  - ES: Exportar todo
  - [ ] OK    Fix: 

- **`exportFiltered`**
  - EN: Export Filtered
  - ES: Exportar filtrado
  - [ ] OK    Fix: 

## `networkLog.filterModes`

- **`clientIp`**
  - EN: Client IP
  - ES: IP cliente
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Protocol
  - ES: Protocolo
  - [ ] OK    Fix: 

- **`rejected`**
  - EN: Rejected
  - ES: Rechazado
  - [ ] OK    Fix: 

- **`tcpUdp`**
  - EN: TCP/UDP
  - ES: TCP/UDP
  - [ ] OK    Fix: 

## `networkLog.filterToggles`

- **`incoming`**
  - EN: Incoming
  - ES: Entrante
  - [ ] OK    Fix: 

- **`outgoing`**
  - EN: Outgoing
  - ES: Saliente
  - [ ] OK    Fix: 

- **`tcp`**
  - EN: TCP
  - ES: TCP
  - [ ] OK    Fix: 

- **`udp`**
  - EN: UDP
  - ES: UDP
  - [ ] OK    Fix: 

## `networkLog.status`

- **`rejected`**
  - EN: REJECTED
  - ES: RECHAZADO
  - [ ] OK    Fix: 

## `outputs.arrayModes`

- **`absolute`**
  - EN: ABSOLUTE
  - ES: ABSOLUTO
  - [ ] OK    Fix: 

- **`array`**
  - EN: Array
  - ES: Grupo
  - [ ] OK    Fix: 

- **`off`**
  - EN: OFF
  - ES: DESACTIVADO
  - [ ] OK    Fix: 

- **`relative`**
  - EN: RELATIVE
  - ES: RELATIVO
  - [ ] OK    Fix: 

- **`single`**
  - EN: Single
  - ES: Individual
  - [ ] OK    Fix: 

## `outputs.buttons`

- **`arrayHidden`**
  - EN: Array Hidden on Map
  - ES: Array oculto en el mapa
  - [ ] OK    Fix: 

- **`arrayVisible`**
  - EN: Array Visible on Map
  - ES: Array visible en el mapa
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - ES: Exportar
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - ES: Importar
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Backup
  - ES: Recargar respaldo
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Output Config
  - ES: Recargar config salidas
  - [ ] OK    Fix: 

- **`speakerHidden`**
  - EN: Speaker Hidden on Map
  - ES: Altavoz oculto en el mapa
  - [ ] OK    Fix: 

- **`speakerVisible`**
  - EN: Speaker Visible on Map
  - ES: Altavoz visible en mapa
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Output Config
  - ES: Guardar config salidas
  - [ ] OK    Fix: 

- **`wizardOfOutZ`**
  - EN: Wizard of OutZ...
  - ES: Asistente OutZ...
  - [ ] OK    Fix: 

## `outputs.coordModes`

- **`xyz`**
  - EN: XYZ
  - ES: XYZ
  - [ ] OK    Fix: 

## `outputs.dialogs`

- **`export`**
  - EN: Export Output Configuration
  - ES: Exportar configuración de salidas
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import Output Configuration
  - ES: Importar configuración de salidas
  - [ ] OK    Fix: 

## `outputs.help`

- **`angleOff`**
  - EN: Output Channel Will Not Amplify Objects in this Angle in Front of it. (changes may affect the rest of the array)
  - ES: El canal de salida no amplificará objetos en este ángulo delante de él. (los cambios pueden afectar al resto del grupo)
  - [ ] OK    Fix: 

- **`angleOn`**
  - EN: Output Channel Will Amplify Objects in this Angle in its Back. (changes may affect the rest of the array)
  - ES: El canal de salida amplificará objetos en este ángulo detrás de él. (los cambios pueden afectar al resto del grupo)
  - [ ] OK    Fix: 

- **`applyToArray`**
  - EN: Apply Changes to the rest of the Array (Absolute value or Relative changes).
  - ES: Aplicar cambios al resto del grupo (valor absoluto o cambios relativos).
  - [ ] OK    Fix: 

- **`arraySelector`**
  - EN: Selected Output Channel is part of Array.
  - ES: El canal de salida seleccionado forma parte de un grupo.
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Output Channel Attenuation. (changes may affect the rest of the array)
  - ES: Atenuación del canal de salida. (los cambios pueden afectar al resto del grupo)
  - [ ] OK    Fix: 

- **`channelSelector`**
  - EN: Output Channel Number and Selection.
  - ES: Número y selección del canal de salida.
  - [ ] OK    Fix: 

- **`coordMode`**
  - EN: Coordinate display mode: Cartesian (X/Y/Z), Cylindrical (radius/azimuth/height), or Spherical (radius/azimuth/elevation).
  - ES: Modo de visualización de coordenadas: cartesiano (X/Y/Z), cilíndrico (radio/azimut/altura) o esférico (radio/azimut/elevación).
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Output Channel Delay (positive values) or Latency Compensation (negative values). (changes may affect the rest of the array)
  - ES: Retardo del canal de salida (valores positivos) o compensación de latencia (valores negativos). (los cambios pueden afectar al resto del grupo)
  - [ ] OK    Fix: 

- **`directional`**
  - EN: Output Channel Directional Control. Drag to change orientation, Shift+drag for Angle Off, Alt+drag for Angle On. (changes may affect the rest of the array)
  - ES: Control direccional del canal de salida. Arrastrar para cambiar orientación, Mayús+arrastrar para Angle Off, Alt+arrastrar para Angle On. (los cambios pueden afectar al resto del grupo)
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Ratio of Distance Attenuation for Selected Output. (changes may affect the rest of the array)
  - ES: Ratio de atenuación por distancia para la salida seleccionada. (los cambios pueden afectar al resto del grupo)
  - [ ] OK    Fix: 

- **`eqBandToggle`**
  - EN: Toggle Band {band} on/off. When off the band is bypassed.
  - ES: Activar/desactivar la banda {band}. Desactivada, la banda se omite.
  - [ ] OK    Fix: 

- **`eqEnable`**
  - EN: Enable or disable EQ processing for this output.
  - ES: Activar o desactivar el EQ para esta salida.
  - [ ] OK    Fix: 

- **`eqFlatten`**
  - EN: Long press to reset all EQ bands to their default values.
  - ES: Pulsación larga para restablecer todas las bandas EQ.
  - [ ] OK    Fix: 

- **`eqFreq`**
  - EN: Output EQ Band {band} frequency (20 Hz - 20 kHz).
  - ES: Frecuencia EQ salida banda {band} (20 Hz - 20 kHz).
  - [ ] OK    Fix: 

- **`eqGain`**
  - EN: Output EQ Band {band} gain (-24 to +24 dB).
  - ES: Ganancia EQ salida banda {band} (-24 a +24 dB).
  - [ ] OK    Fix: 

- **`eqQ`**
  - EN: Output EQ Band {band} Q factor (0.1 - 10.0).
  - ES: Factor Q EQ salida banda {band} (0.1 - 10.0).
  - [ ] OK    Fix: 

- **`eqResetBand`**
  - EN: Long press to reset Band {band} to its default values.
  - ES: Pulsación larga para restablecer la banda {band}.
  - [ ] OK    Fix: 

- **`eqShape`**
  - EN: Output EQ Band {band} filter shape.
  - ES: Forma del filtro EQ salida banda {band}.
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Output Configuration to file (with file explorer window).
  - ES: Exportar la configuración de salidas a un archivo (con ventana del explorador).
  - [ ] OK    Fix: 

- **`floorReflections`**
  - EN: Enable or Disable the Floor Reflections for this Speaker.
  - ES: Activar o desactivar las reflexiones del suelo para este altavoz.
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: Loss of High Frequency Depending on Distance from Object to Output. (changes may affect the rest of the array)
  - ES: Pérdida de altas frecuencias según la distancia del objeto a la salida. (los cambios pueden afectar al resto del grupo)
  - [ ] OK    Fix: 

- **`hParallax`**
  - EN: Horizontal Distance from Speaker to 'Targeted' Listener. (changes may affect the rest of the array)
  - ES: Distancia horizontal del altavoz al oyente « objetivo ». (los cambios pueden afectar al resto del grupo)
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Output Configuration from file (with file explorer window).
  - ES: Importar la configuración de salidas desde un archivo (con ventana del explorador).
  - [ ] OK    Fix: 

- **`liveSource`**
  - EN: Disables Live Source Attenuation for Selected Output. (changes may affect the rest of the array)
  - ES: Desactiva la atenuación de fuente en vivo para la salida seleccionada. (los cambios pueden afectar al resto del grupo)
  - [ ] OK    Fix: 

- **`mapVisibility`**
  - EN: Make Visible or Hide The Selected Output on the Map.
  - ES: Hacer visible u ocultar la salida seleccionada en el mapa.
  - [ ] OK    Fix: 

- **`minLatency`**
  - EN: Disables Minimal Latency Mode for Selected Output. (changes may affect the rest of the array)
  - ES: Desactiva el modo de latencia mínima para la salida seleccionada. (los cambios pueden afectar al resto del grupo)
  - [ ] OK    Fix: 

- **`nameEditor`**
  - EN: Displayed Output Channel Name (editable).
  - ES: Nombre mostrado del canal de salida (editable).
  - [ ] OK    Fix: 

- **`pitch`**
  - EN: Output Channel Vertical Orientation used to Determine which Objects get amplified. (changes may affect the rest of the array)
  - ES: Orientación vertical del canal de salida utilizada para determinar qué objetos se amplifican. (los cambios pueden afectar al resto del grupo)
  - [ ] OK    Fix: 

- **`position1`**
  - EN: Output Channel {name} ({unit}).
  - ES: Canal de salida {name} ({unit}).
  - [ ] OK    Fix: 

- **`position2`**
  - EN: Output Channel {name} ({unit}).
  - ES: Canal de salida {name} ({unit}).
  - [ ] OK    Fix: 

- **`position3`**
  - EN: Output Channel {name} ({unit}).
  - ES: Canal de salida {name} ({unit}).
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Output Configuration from backup file.
  - ES: Recargar la configuración de salidas desde el archivo de respaldo.
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Output Configuration from file.
  - ES: Recargar la configuración de salidas desde un archivo.
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Output Configuration to file (with backup).
  - ES: Guardar la configuración de salidas en un archivo (con respaldo).
  - [ ] OK    Fix: 

- **`vParallax`**
  - EN: Vertical Distance from Speaker to 'Targeted' Listener. Positive when the Speaker is Below the head of the Listener. (changes may affect the rest of the array)
  - ES: Distancia vertical del altavoz al oyente « objetivo ». Positivo cuando el altavoz está debajo de la cabeza del oyente. (los cambios pueden afectar al resto del grupo)
  - [ ] OK    Fix: 

- **`wizardOfOutZ`**
  - EN: Open Wizard of OutZ to Position Speaker Arrays Conveniently.
  - ES: Abrir el Wizard of OutZ para posicionar grupos de altavoces cómodamente.
  - [ ] OK    Fix: 

## `outputs.labels`

- **`angleOff`**
  - EN: Angle Off:
  - ES: Ángulo Off:
  - [ ] OK    Fix: 

- **`angleOn`**
  - EN: Angle On:
  - ES: Ángulo On:
  - [ ] OK    Fix: 

- **`applyToArray`**
  - EN: Apply to Array:
  - ES: Aplicar al grupo:
  - [ ] OK    Fix: 

- **`array`**
  - EN: Array:
  - ES: Grupo:
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Attenuation:
  - ES: Atenuación:
  - [ ] OK    Fix: 

- **`coordinates`**
  - EN: Coordinates:
  - ES: Coordenadas:
  - [ ] OK    Fix: 

- **`delay`**
  - EN: Delay:
  - ES: Retardo:
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Delay/Latency:
  - ES: Retardo/Latencia:
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Distance Atten:
  - ES: Atenuación distancia:
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: HF Damping:
  - ES: Atenuación HF:
  - [ ] OK    Fix: 

- **`hParallax`**
  - EN: Horizontal Parallax:
  - ES: Paralaje horizontal:
  - [ ] OK    Fix: 

- **`latency`**
  - EN: Latency:
  - ES: Latencia:
  - [ ] OK    Fix: 

- **`name`**
  - EN: Name:
  - ES: Nombre:
  - [ ] OK    Fix: 

- **`orientation`**
  - EN: Orientation:
  - ES: Orientación:
  - [ ] OK    Fix: 

- **`pitch`**
  - EN: Pitch:
  - ES: Inclinación:
  - [ ] OK    Fix: 

- **`positionX`**
  - EN: Position X:
  - ES: Posición X:
  - [ ] OK    Fix: 

- **`positionY`**
  - EN: Position Y:
  - ES: Posición Y:
  - [ ] OK    Fix: 

- **`positionZ`**
  - EN: Position Z:
  - ES: Posición Z:
  - [ ] OK    Fix: 

- **`vParallax`**
  - EN: Vertical Parallax:
  - ES: Paralaje vertical:
  - [ ] OK    Fix: 

## `outputs.messages`

- **`assignedToArray`**
  - EN: Output {num} assigned to Array {array}
  - ES: Salida {num} asignada al Grupo {array}
  - [ ] OK    Fix: 

- **`backupLoaded`**
  - EN: Output configuration loaded from backup.
  - ES: Configuración de salidas cargada desde respaldo.
  - [ ] OK    Fix: 

- **`configExported`**
  - EN: Output configuration exported.
  - ES: Configuración de salidas exportada.
  - [ ] OK    Fix: 

- **`configImported`**
  - EN: Output configuration imported.
  - ES: Configuración de salidas importada.
  - [ ] OK    Fix: 

- **`configLoaded`**
  - EN: Output configuration loaded.
  - ES: Configuración de salidas cargada.
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Output configuration saved.
  - ES: Configuración de salidas guardada.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - ES: Error: {error}
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - ES: Por favor seleccione primero una carpeta de proyecto en Config Sistema.
  - [ ] OK    Fix: 

- **`setToSingle`**
  - EN: Output {num} set to Single
  - ES: Salida {num} configurada como Individual
  - [ ] OK    Fix: 

## `outputs.tabs`

- **`eq`**
  - EN: Output EQ
  - ES: EQ de salida
  - [ ] OK    Fix: 

- **`parameters`**
  - EN: Output Parameters
  - ES: Parámetros de salida
  - [ ] OK    Fix: 

## `outputs.toggles`

- **`floorReflectionsOff`**
  - EN: Floor Reflections: OFF
  - ES: Reflejos suelo: NO
  - [ ] OK    Fix: 

- **`floorReflectionsOn`**
  - EN: Floor Reflections: ON
  - ES: Reflejos suelo: SÍ
  - [ ] OK    Fix: 

- **`liveSourceOff`**
  - EN: Live Source Atten: OFF
  - ES: Atenuación live: NO
  - [ ] OK    Fix: 

- **`liveSourceOn`**
  - EN: Live Source Atten: ON
  - ES: Atenuación live: SÍ
  - [ ] OK    Fix: 

- **`minLatencyOff`**
  - EN: Minimal Latency: OFF
  - ES: Latencia mín: NO
  - [ ] OK    Fix: 

- **`minLatencyOn`**
  - EN: Minimal Latency: ON
  - ES: Latencia mín: SÍ
  - [ ] OK    Fix: 

## `outputs.units`

- **`meters`**
  - EN: m
  - ES: m
  - [ ] OK    Fix: 

- **`ms`**
  - EN: ms
  - ES: ms
  - [ ] OK    Fix: 

## `reverbs`

- **`noChannels`**
  - EN: No reverb channels configured.

Set the number of Reverb Channels in System Config.
  - ES: No hay canales de reverberación configurados.

Configure el número de canales de reverberación en Config Sistema.
  - [ ] OK    Fix: 

## `reverbs.algorithm`

- **`crossoverHigh`**
  - EN: Crossover High:
  - ES: Cruce alto:
  - [ ] OK    Fix: 

- **`crossoverLow`**
  - EN: Crossover Low:
  - ES: Cruce bajo:
  - [ ] OK    Fix: 

- **`decaySection`**
  - EN: Decay
  - ES: Decaimiento
  - [ ] OK    Fix: 

- **`diffusion`**
  - EN: Diffusion:
  - ES: Difusión:
  - [ ] OK    Fix: 

- **`fdn`**
  - EN: FDN
  - ES: FDN
  - [ ] OK    Fix: 

- **`fdnSection`**
  - EN: FDN
  - ES: FDN
  - [ ] OK    Fix: 

- **`ir`**
  - EN: IR
  - ES: IR
  - [ ] OK    Fix: 

- **`irFile`**
  - EN: IR File:
  - ES: Archivo IR:
  - [ ] OK    Fix: 

- **`irImport`**
  - EN: Import IR...
  - ES: Importar IR...
  - [ ] OK    Fix: 

- **`irImportSuccess`**
  - EN: IR imported: {file}
  - ES: IR importado: {file}
  - [ ] OK    Fix: 

- **`irLength`**
  - EN: IR Length:
  - ES: Longitud IR:
  - [ ] OK    Fix: 

- **`irNoProject`**
  - EN: Set a project folder first
  - ES: Seleccione una carpeta de proyecto primero
  - [ ] OK    Fix: 

- **`irSection`**
  - EN: IR
  - ES: IR
  - [ ] OK    Fix: 

- **`irTrim`**
  - EN: IR Trim:
  - ES: Recorte IR:
  - [ ] OK    Fix: 

- **`noFileLoaded`**
  - EN: No IR loaded
  - ES: Sin IR cargada
  - [ ] OK    Fix: 

- **`perNodeOff`**
  - EN: Per-node IR OFF
  - ES: IR por nodo OFF
  - [ ] OK    Fix: 

- **`perNodeOn`**
  - EN: Per-node IR ON
  - ES: IR por nodo ON
  - [ ] OK    Fix: 

- **`rt60`**
  - EN: RT60:
  - ES: RT60:
  - [ ] OK    Fix: 

- **`rt60HighMult`**
  - EN: RT60 High ×:
  - ES: RT60 Alto ×:
  - [ ] OK    Fix: 

- **`rt60LowMult`**
  - EN: RT60 Low ×:
  - ES: RT60 Bajo ×:
  - [ ] OK    Fix: 

- **`scale`**
  - EN: Scale:
  - ES: Escala:
  - [ ] OK    Fix: 

- **`sdn`**
  - EN: SDN
  - ES: SDN
  - [ ] OK    Fix: 

- **`sdnSection`**
  - EN: SDN
  - ES: SDN
  - [ ] OK    Fix: 

- **`size`**
  - EN: Size:
  - ES: Tamaño:
  - [ ] OK    Fix: 

- **`wetLevel`**
  - EN: Wet Level:
  - ES: Nivel Wet:
  - [ ] OK    Fix: 

## `reverbs.buttons`

- **`editOnMap`**
  - EN: Edit on Map
  - ES: Editar en el mapa
  - [ ] OK    Fix: 

- **`editOnMapOn`**
  - EN: Edit on Map ON
  - ES: Editar en el mapa ON
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - ES: Exportar
  - [ ] OK    Fix: 

- **`hiddenOnMap`**
  - EN: Reverbs Hidden on Map
  - ES: Reverbs ocultas en mapa
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - ES: Importar
  - [ ] OK    Fix: 

- **`mutePost`**
  - EN: Mute Post
  - ES: Mute Post
  - [ ] OK    Fix: 

- **`mutePostOn`**
  - EN: Mute Post ON
  - ES: Mute Post ENC.
  - [ ] OK    Fix: 

- **`mutePre`**
  - EN: Mute Pre
  - ES: Mute Pre
  - [ ] OK    Fix: 

- **`mutePreOn`**
  - EN: Mute Pre ON
  - ES: Mute Pre ENC.
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Backup
  - ES: Recargar respaldo
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Reverb Config
  - ES: Recargar config reverb
  - [ ] OK    Fix: 

- **`soloReverbs`**
  - EN: Solo Reverbs
  - ES: Solo Reverbs
  - [ ] OK    Fix: 

- **`soloReverbsOn`**
  - EN: Solo Reverbs ON
  - ES: Solo reverbs ENC.
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Reverb Config
  - ES: Guardar config reverb
  - [ ] OK    Fix: 

- **`visibleOnMap`**
  - EN: Reverbs Visible on Map
  - ES: Reverbs visibles en mapa
  - [ ] OK    Fix: 

## `reverbs.coordModes`

- **`xyz`**
  - EN: XYZ
  - ES: XYZ
  - [ ] OK    Fix: 

## `reverbs.dialogs`

- **`export`**
  - EN: Export Reverb Configuration
  - ES: Exportar configuración reverb
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import Reverb Configuration
  - ES: Importar configuración reverb
  - [ ] OK    Fix: 

## `reverbs.help`

- **`algoCrossoverHigh`**
  - EN: High crossover frequency for 3-band decay (1 - 10 kHz).
  - ES: Frecuencia de cruce alta para decaimiento de 3 bandas (1 - 10 kHz).
  - [ ] OK    Fix: 

- **`algoCrossoverLow`**
  - EN: Low crossover frequency for 3-band decay (50 - 500 Hz).
  - ES: Frecuencia de cruce baja para decaimiento de 3 bandas (50 - 500 Hz).
  - [ ] OK    Fix: 

- **`algoDiffusion`**
  - EN: Diffusion amount controlling echo density (0 - 100%).
  - ES: Cantidad de difusión que controla la densidad de ecos (0 - 100%).
  - [ ] OK    Fix: 

- **`algoFDN`**
  - EN: Select FDN (Feedback Delay Network) reverb algorithm.
  - ES: Seleccionar algoritmo de reverberación FDN (Feedback Delay Network).
  - [ ] OK    Fix: 

- **`algoFDNSize`**
  - EN: FDN delay line size multiplier (0.5 - 2.0x).
  - ES: Multiplicador del tamaño de línea de retardo FDN (0.5 - 2.0x).
  - [ ] OK    Fix: 

- **`algoIR`**
  - EN: Select IR (Impulse Response convolution) reverb algorithm.
  - ES: Seleccionar algoritmo de reverberación IR (convolución de respuesta al impulso).
  - [ ] OK    Fix: 

- **`algoIRFile`**
  - EN: Select or import an impulse response file for convolution.
  - ES: Seleccionar o importar un archivo de respuesta al impulso para convolución.
  - [ ] OK    Fix: 

- **`algoIRLength`**
  - EN: Maximum impulse response length (0.1 - 6.0 seconds).
  - ES: Longitud máxima de la respuesta al impulso (0.1 - 6.0 segundos).
  - [ ] OK    Fix: 

- **`algoIRTrim`**
  - EN: Trim the start of the impulse response (0 - 100 ms).
  - ES: Recortar el inicio de la respuesta al impulso (0 - 100 ms).
  - [ ] OK    Fix: 

- **`algoPerNode`**
  - EN: Use a separate IR for each reverb node, or share one IR.
  - ES: Usar una IR separada para cada nodo de reverberación, o compartir una sola IR.
  - [ ] OK    Fix: 

- **`algoRT60`**
  - EN: Reverb decay time RT60 (0.2 - 8.0 seconds).
  - ES: Tiempo de decaimiento de reverberación RT60 (0.2 - 8.0 segundos).
  - [ ] OK    Fix: 

- **`algoRT60HighMult`**
  - EN: High frequency RT60 multiplier (0.1 - 9.0x).
  - ES: Multiplicador RT60 de alta frecuencia (0.1 - 9.0x).
  - [ ] OK    Fix: 

- **`algoRT60LowMult`**
  - EN: Low frequency RT60 multiplier (0.1 - 9.0x).
  - ES: Multiplicador RT60 de baja frecuencia (0.1 - 9.0x).
  - [ ] OK    Fix: 

- **`algoSDN`**
  - EN: Select SDN (Scattering Delay Network) reverb algorithm.
  - ES: Seleccionar algoritmo de reverberación SDN (Scattering Delay Network).
  - [ ] OK    Fix: 

- **`algoSDNScale`**
  - EN: SDN inter-node delay scale factor (0.5 - 4.0x).
  - ES: Factor de escala de retardo entre nodos SDN (0.5 - 4.0x).
  - [ ] OK    Fix: 

- **`algoWetLevel`**
  - EN: Wet/dry mix level for reverb output (-60 to +12 dB).
  - ES: Nivel de mezcla wet/dry para la salida de reverberación (-60 a +12 dB).
  - [ ] OK    Fix: 

- **`angleOff`**
  - EN: Angle at which no amplification occurs (0-179 degrees).
  - ES: Ángulo en el que no se produce amplificación (0-179 grados).
  - [ ] OK    Fix: 

- **`angleOn`**
  - EN: Angle at which amplification starts (1-180 degrees).
  - ES: Ángulo en el que comienza la amplificación (1-180 grados).
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Reverb channel attenuation (-92 to 0 dB).
  - ES: Atenuación del canal de reverberación (-92 a 0 dB).
  - [ ] OK    Fix: 

- **`channelSelector`**
  - EN: Reverb Channel Number and Selection.
  - ES: Número y selección del canal de reverberación.
  - [ ] OK    Fix: 

- **`commonAtten`**
  - EN: Common attenuation percentage (0-100%).
  - ES: Porcentaje de atenuación común (0-100%).
  - [ ] OK    Fix: 

- **`coordMode`**
  - EN: Coordinate display mode: Cartesian (X/Y/Z), Cylindrical (radius/azimuth/height), or Spherical (radius/azimuth/elevation).
  - ES: Modo de visualización de coordenadas: Cartesiano (X/Y/Z), Cilíndrico (radio/acimut/altura), o Esférico (radio/acimut/elevación).
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Reverb delay/latency compensation (-100 to +100 ms).
  - ES: Compensación de retardo/latencia de la reverberación (-100 a +100 ms).
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Distance attenuation for reverb return (-6.0 to 0.0 dB/m).
  - ES: Atenuación por distancia para el retorno de reverberación (-6.0 a 0.0 dB/m).
  - [ ] OK    Fix: 

- **`distanceAttenEnable`**
  - EN: Distance attenuation percentage (0-200%).
  - ES: Porcentaje de atenuación por distancia (0-200%).
  - [ ] OK    Fix: 

- **`eqBandToggle`**
  - EN: Toggle pre-EQ Band {band} on/off. When off the band is bypassed.
  - ES: Activar/desactivar la banda pre-EQ {band}.
  - [ ] OK    Fix: 

- **`eqEnable`**
  - EN: Enable or disable EQ processing for this reverb.
  - ES: Activar o desactivar el procesamiento EQ para esta reverberación.
  - [ ] OK    Fix: 

- **`eqFlatten`**
  - EN: Long press to reset all pre-EQ bands to their default values.
  - ES: Pulsación larga para restablecer todas las bandas pre-EQ.
  - [ ] OK    Fix: 

- **`eqFreq`**
  - EN: Pre-EQ Band {band} frequency (20 Hz - 20 kHz).
  - ES: Frecuencia pre-EQ banda {band} (20 Hz - 20 kHz).
  - [ ] OK    Fix: 

- **`eqGain`**
  - EN: Pre-EQ Band {band} gain (-24 to +24 dB).
  - ES: Ganancia pre-EQ banda {band} (-24 a +24 dB).
  - [ ] OK    Fix: 

- **`eqQ`**
  - EN: Pre-EQ Band {band} Q factor (0.1 - 20.0).
  - ES: Factor Q pre-EQ banda {band} (0.1 - 20.0).
  - [ ] OK    Fix: 

- **`eqResetBand`**
  - EN: Long press to reset pre-EQ Band {band} to its default values.
  - ES: Pulsación larga para restablecer la banda pre-EQ {band}.
  - [ ] OK    Fix: 

- **`eqShape`**
  - EN: Pre-EQ Band {band} filter shape.
  - ES: Forma del filtro pre-EQ banda {band}.
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Reverb Configuration to file (with file explorer window).
  - ES: Exportar la configuración de reverberación a archivo (con explorador de archivos).
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: High frequency loss per meter (-6.0 to 0.0 dB/m).
  - ES: Pérdida de alta frecuencia por metro (-6.0 a 0.0 dB/m).
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Reverb Configuration from file (with file explorer window).
  - ES: Importar la configuración de reverberación desde archivo (con explorador de archivos).
  - [ ] OK    Fix: 

- **`liveSourceTooltip`**
  - EN: Enable Live Source attenuation tamer. Reduces level fluctuations from sources close to the array.
  - ES: Activar el domador de atenuación Live Source. Reduce las fluctuaciones de nivel de fuentes cercanas al array.
  - [ ] OK    Fix: 

- **`mapVisibility`**
  - EN: Make Visible or Hide All Reverb Channels on the Map.
  - ES: Hacer visibles u ocultar todos los canales de reverberación en el mapa.
  - [ ] OK    Fix: 

- **`miniLatencyTooltip`**
  - EN: Enable minimal latency mode for this reverb channel. Reduces processing delay at the cost of higher CPU usage.
  - ES: Activar el modo de latencia mínima para este canal de reverberación. Reduce el retardo de procesamiento a costa de mayor uso de CPU.
  - [ ] OK    Fix: 

- **`muteButton`**
  - EN: Toggle mute for this output channel's reverb return.
  - ES: Activar/desactivar silencio del retorno de reverberación para esta salida.
  - [ ] OK    Fix: 

- **`muteMacros`**
  - EN: Quick mute operations for output channels.
  - ES: Operaciones rápidas de silenciamiento para canales de salida.
  - [ ] OK    Fix: 

- **`nameEditor`**
  - EN: Displayed Reverb Channel Name (editable).
  - ES: Nombre mostrado del canal de reverberación (editable).
  - [ ] OK    Fix: 

- **`orientation`**
  - EN: Reverb orientation angle (-179 to +180 degrees).
  - ES: Ángulo de orientación de la reverberación (-179 a +180 grados).
  - [ ] OK    Fix: 

- **`pitch`**
  - EN: Vertical orientation of reverb (-90 to +90 degrees).
  - ES: Orientación vertical de la reverberación (-90 a +90 grados).
  - [ ] OK    Fix: 

- **`position1`**
  - EN: Reverb virtual source {name} ({unit}).
  - ES: Fuente virtual de reverberación {name} ({unit}).
  - [ ] OK    Fix: 

- **`position2`**
  - EN: Reverb virtual source {name} ({unit}).
  - ES: Fuente virtual de reverberación {name} ({unit}).
  - [ ] OK    Fix: 

- **`position3`**
  - EN: Reverb virtual source {name} ({unit}).
  - ES: Fuente virtual de reverberación {name} ({unit}).
  - [ ] OK    Fix: 

- **`postEqBandToggle`**
  - EN: Toggle post-EQ Band {band} on/off. When off the band is bypassed.
  - ES: Activar/desactivar la banda post-EQ {band}.
  - [ ] OK    Fix: 

- **`postEqEnable`**
  - EN: Enable or disable post-processing EQ.
  - ES: Activar o desactivar el EQ de post-procesamiento.
  - [ ] OK    Fix: 

- **`postEqFlatten`**
  - EN: Long press to reset all post-EQ bands to their default values.
  - ES: Pulsación larga para restablecer todas las bandas post-EQ.
  - [ ] OK    Fix: 

- **`postEqFreq`**
  - EN: Post-EQ Band {band} frequency (20 Hz - 20 kHz).
  - ES: Frecuencia de la banda {band} del post-EQ (20 Hz - 20 kHz).
  - [ ] OK    Fix: 

- **`postEqGain`**
  - EN: Post-EQ Band {band} gain (-24 to +24 dB).
  - ES: Ganancia de la banda {band} del post-EQ (-24 a +24 dB).
  - [ ] OK    Fix: 

- **`postEqQ`**
  - EN: Post-EQ Band {band} Q factor (0.1 - 20.0).
  - ES: Factor Q de la banda {band} del post-EQ (0.1 - 20.0).
  - [ ] OK    Fix: 

- **`postEqResetBand`**
  - EN: Long press to reset post-EQ Band {band} to its default values.
  - ES: Pulsación larga para restablecer la banda post-EQ {band}.
  - [ ] OK    Fix: 

- **`postEqShape`**
  - EN: Post-EQ Band {band} filter shape.
  - ES: Forma de filtro de la banda {band} del post-EQ.
  - [ ] OK    Fix: 

- **`postExpAttack`**
  - EN: Post-expander attack time (0.1 - 50 ms).
  - ES: Tiempo de ataque del post-expansor (0.1 - 50 ms).
  - [ ] OK    Fix: 

- **`postExpBypass`**
  - EN: Bypass or enable the post-expander on reverb returns.
  - ES: Omitir o activar el post-expansor en los retornos de reverberación.
  - [ ] OK    Fix: 

- **`postExpRatio`**
  - EN: Post-expander ratio (1:1 to 1:8).
  - ES: Ratio del post-expansor (1:1 a 1:8).
  - [ ] OK    Fix: 

- **`postExpRelease`**
  - EN: Post-expander release time (50 - 2000 ms).
  - ES: Tiempo de liberación del post-expansor (50 - 2000 ms).
  - [ ] OK    Fix: 

- **`postExpThreshold`**
  - EN: Post-expander threshold (-80 to -10 dB).
  - ES: Umbral del post-expansor (-80 a -10 dB).
  - [ ] OK    Fix: 

- **`preCompAttack`**
  - EN: Pre-compressor attack time (0.1 - 100 ms).
  - ES: Tiempo de ataque del precompresor (0.1 - 100 ms).
  - [ ] OK    Fix: 

- **`preCompBypass`**
  - EN: Bypass or enable the pre-compressor on reverb feeds.
  - ES: Omitir o activar el precompresor en los envíos de reverberación.
  - [ ] OK    Fix: 

- **`preCompRatio`**
  - EN: Pre-compressor ratio (1:1 to 20:1).
  - ES: Ratio del precompresor (1:1 a 20:1).
  - [ ] OK    Fix: 

- **`preCompRelease`**
  - EN: Pre-compressor release time (10 - 1000 ms).
  - ES: Tiempo de liberación del precompresor (10 - 1000 ms).
  - [ ] OK    Fix: 

- **`preCompThreshold`**
  - EN: Pre-compressor threshold (-60 to 0 dB).
  - ES: Umbral del precompresor (-60 a 0 dB).
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Reverb Configuration from backup file.
  - ES: Recargar la configuración de reverberación desde archivo de respaldo.
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Reverb Configuration from file.
  - ES: Recargar la configuración de reverberación desde archivo.
  - [ ] OK    Fix: 

- **`returnOffset1`**
  - EN: Reverb return offset {name} ({unit}).
  - ES: Desplazamiento de retorno de reverberación {name} ({unit}).
  - [ ] OK    Fix: 

- **`returnOffset2`**
  - EN: Reverb return offset {name} ({unit}).
  - ES: Desplazamiento de retorno de reverberación {name} ({unit}).
  - [ ] OK    Fix: 

- **`returnOffset3`**
  - EN: Reverb return offset {name} ({unit}).
  - ES: Desplazamiento de retorno de reverberación {name} ({unit}).
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Reverb Configuration to file (with backup).
  - ES: Guardar la configuración de reverberación en archivo (con copia de seguridad).
  - [ ] OK    Fix: 

## `reverbs.labels`

- **`angleOff`**
  - EN: Angle Off:
  - ES: Ángulo Off:
  - [ ] OK    Fix: 

- **`angleOn`**
  - EN: Angle On:
  - ES: Ángulo On:
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Attenuation:
  - ES: Atenuación:
  - [ ] OK    Fix: 

- **`commonAtten`**
  - EN: Common Atten:
  - ES: Atenuación común:
  - [ ] OK    Fix: 

- **`coordinates`**
  - EN: Coord:
  - ES: Coord:
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Delay/Latency:
  - ES: Retardo/Latencia:
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Distance Atten:
  - ES: Atenuación distancia:
  - [ ] OK    Fix: 

- **`distanceAttenPercent`**
  - EN: Distance Atten %:
  - ES: Atenuación distancia %:
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: HF Damping:
  - ES: Atenuación HF:
  - [ ] OK    Fix: 

- **`muteMacro`**
  - EN: Mute Macro:
  - ES: Macro mute:
  - [ ] OK    Fix: 

- **`name`**
  - EN: Name:
  - ES: Nombre:
  - [ ] OK    Fix: 

- **`orientation`**
  - EN: Orientation:
  - ES: Orientación:
  - [ ] OK    Fix: 

- **`outputMutes`**
  - EN: Output Mutes:
  - ES: Mutes salidas:
  - [ ] OK    Fix: 

- **`pitch`**
  - EN: Pitch:
  - ES: Inclinación:
  - [ ] OK    Fix: 

- **`positionX`**
  - EN: Position X:
  - ES: Posición X:
  - [ ] OK    Fix: 

- **`positionY`**
  - EN: Position Y:
  - ES: Posición Y:
  - [ ] OK    Fix: 

- **`positionZ`**
  - EN: Position Z:
  - ES: Posición Z:
  - [ ] OK    Fix: 

- **`returnOffsetX`**
  - EN: Return Offset X:
  - ES: Desplazamiento retorno X:
  - [ ] OK    Fix: 

- **`returnOffsetY`**
  - EN: Return Offset Y:
  - ES: Desplazamiento retorno Y:
  - [ ] OK    Fix: 

- **`returnOffsetZ`**
  - EN: Return Offset Z:
  - ES: Desplazamiento retorno Z:
  - [ ] OK    Fix: 

## `reverbs.messages`

- **`backupLoaded`**
  - EN: Reverb configuration loaded from backup.
  - ES: Configuración reverb cargada desde respaldo.
  - [ ] OK    Fix: 

- **`configExported`**
  - EN: Reverb configuration exported.
  - ES: Configuración reverb exportada.
  - [ ] OK    Fix: 

- **`configImported`**
  - EN: Reverb configuration imported.
  - ES: Configuración reverb importada.
  - [ ] OK    Fix: 

- **`configLoaded`**
  - EN: Reverb configuration loaded.
  - ES: Configuración reverb cargada.
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Reverb configuration saved.
  - ES: Configuración reverb guardada.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - ES: Error: {error}
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - ES: Por favor seleccione primero una carpeta de proyecto en Config Sistema.
  - [ ] OK    Fix: 

## `reverbs.muteMacros`

- **`invertMutes`**
  - EN: INVERT MUTES
  - ES: INVERTIR MUTES
  - [ ] OK    Fix: 

- **`muteAll`**
  - EN: MUTE ALL
  - ES: SILENCIAR TODO
  - [ ] OK    Fix: 

- **`muteArray`**
  - EN: MUTE ARRAY
  - ES: SILENCIAR GRUPO
  - [ ] OK    Fix: 

- **`muteEven`**
  - EN: MUTE EVEN
  - ES: SILENCIAR PARES
  - [ ] OK    Fix: 

- **`muteOdd`**
  - EN: MUTE ODD
  - ES: SILENCIAR IMPARES
  - [ ] OK    Fix: 

- **`select`**
  - EN: Mute Macro Select
  - ES: Selección macro mute
  - [ ] OK    Fix: 

- **`unmuteAll`**
  - EN: UNMUTE ALL
  - ES: ACTIVAR TODO
  - [ ] OK    Fix: 

- **`unmuteArray`**
  - EN: UNMUTE ARRAY
  - ES: ACTIVAR GRUPO
  - [ ] OK    Fix: 

## `reverbs.postProcessing`

- **`attack`**
  - EN: Attack:
  - ES: Ataque:
  - [ ] OK    Fix: 

- **`expander`**
  - EN: Expander
  - ES: Expansor
  - [ ] OK    Fix: 

- **`expanderOff`**
  - EN: Expander OFF
  - ES: Expansor OFF
  - [ ] OK    Fix: 

- **`expanderOn`**
  - EN: Expander ON
  - ES: Expansor ON
  - [ ] OK    Fix: 

- **`ratio`**
  - EN: Ratio:
  - ES: Ratio:
  - [ ] OK    Fix: 

- **`release`**
  - EN: Release:
  - ES: Liberación:
  - [ ] OK    Fix: 

- **`threshold`**
  - EN: Threshold:
  - ES: Umbral:
  - [ ] OK    Fix: 

## `reverbs.preProcessing`

- **`attack`**
  - EN: Attack:
  - ES: Ataque:
  - [ ] OK    Fix: 

- **`compressor`**
  - EN: Compressor
  - ES: Compresor
  - [ ] OK    Fix: 

- **`compressorOff`**
  - EN: Compressor OFF
  - ES: Compresor OFF
  - [ ] OK    Fix: 

- **`compressorOn`**
  - EN: Compressor ON
  - ES: Compresor ON
  - [ ] OK    Fix: 

- **`ratio`**
  - EN: Ratio:
  - ES: Ratio:
  - [ ] OK    Fix: 

- **`release`**
  - EN: Release:
  - ES: Liberación:
  - [ ] OK    Fix: 

- **`threshold`**
  - EN: Threshold:
  - ES: Umbral:
  - [ ] OK    Fix: 

## `reverbs.sections`

- **`reverbFeed`**
  - EN: Reverb Feed
  - ES: Entrada reverb
  - [ ] OK    Fix: 

- **`reverbReturn`**
  - EN: Reverb Return
  - ES: Retorno reverb
  - [ ] OK    Fix: 

## `reverbs.tabs`

- **`algorithm`**
  - EN: Algorithm
  - ES: Algoritmo
  - [ ] OK    Fix: 

- **`channelParams`**
  - EN: Channel Parameters
  - ES: Parámetros del canal
  - [ ] OK    Fix: 

- **`postProcessing`**
  - EN: Post-Processing
  - ES: Post-procesamiento
  - [ ] OK    Fix: 

- **`preProcessing`**
  - EN: Pre-Processing
  - ES: Pre-procesamiento
  - [ ] OK    Fix: 

## `reverbs.toggles`

- **`liveSourceOff`**
  - EN: Live Source Atten OFF
  - ES: Atenuación live NO
  - [ ] OK    Fix: 

- **`liveSourceOn`**
  - EN: Live Source Atten ON
  - ES: Atenuación live SÍ
  - [ ] OK    Fix: 

- **`minLatencyOff`**
  - EN: Minimal Latency OFF
  - ES: Latencia mín NO
  - [ ] OK    Fix: 

- **`minLatencyOn`**
  - EN: Minimal Latency ON
  - ES: Latencia mín SÍ
  - [ ] OK    Fix: 

## `sampler.buttons`

- **`copy`**
  - EN: Copy
  - ES: Copiar
  - [ ] OK    Fix: 

- **`copyCell`**
  - EN: Copy Cell
  - ES: Copiar celda
  - [ ] OK    Fix: 

- **`copySet`**
  - EN: Copy Set
  - ES: Copiar set
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - ES: Exportar
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - ES: Importar
  - [ ] OK    Fix: 

- **`paste`**
  - EN: Paste
  - ES: Pegar
  - [ ] OK    Fix: 

- **`pasteCell`**
  - EN: Paste Cell
  - ES: Pegar celda
  - [ ] OK    Fix: 

- **`pasteSet`**
  - EN: Paste Set
  - ES: Pegar set
  - [ ] OK    Fix: 

## `sampler.cell`

- **`attenuation`**
  - EN: Attenuation (dB)
  - ES: Atenuación (dB)
  - [ ] OK    Fix: 

- **`clear`**
  - EN: Clear
  - ES: Borrar
  - [ ] OK    Fix: 

- **`inOut`**
  - EN: In/Out (ms)
  - ES: Ent/Sal (ms)
  - [ ] OK    Fix: 

- **`load`**
  - EN: Load
  - ES: Cargar
  - [ ] OK    Fix: 

- **`loadTitle`**
  - EN: Load Sample
  - ES: Cargar sample
  - [ ] OK    Fix: 

- **`offset`**
  - EN: Offset (m)
  - ES: Despl. (m)
  - [ ] OK    Fix: 

- **`preview`**
  - EN: Preview
  - ES: Preescucha
  - [ ] OK    Fix: 

- **`previewStop`**
  - EN: Stop
  - ES: Detener
  - [ ] OK    Fix: 

## `sampler.grid`

- **`help`**
  - EN: Click = select
Shift = multi
Ctrl = set toggle
DblClick = load
  - ES: Clic=seleccionar | Shift=multi | Ctrl=alternar set | DblClic=cargar
  - [ ] OK    Fix: 

## `sampler.labels`

- **`lightpadZone`**
  - EN: Lightpad Zone
  - ES: Zona Lightpad
  - [ ] OK    Fix: 

- **`selectZone`**
  - EN: Select Zone
  - ES: Seleccionar zona
  - [ ] OK    Fix: 

## `sampler.lightpadZone`

- **`none`**
  - EN: None
  - ES: Ninguna
  - [ ] OK    Fix: 

## `sampler.press`

- **`height`**
  - EN: Height
  - ES: Altura
  - [ ] OK    Fix: 

- **`hf`**
  - EN: HF Shelf
  - ES: HF Shelf
  - [ ] OK    Fix: 

- **`level`**
  - EN: Level
  - ES: Nivel
  - [ ] OK    Fix: 

- **`xy`**
  - EN: Position XY
  - ES: Position XY
  - [ ] OK    Fix: 

## `sampler.remote`

- **`gridLayout`**
  - EN: Grid Layout
  - ES: Disposición de la cuadrícula
  - [ ] OK    Fix: 

## `sampler.section`

- **`actions`**
  - EN: ACTIONS
  - ES: ACCIONES
  - [ ] OK    Fix: 

- **`cell`**
  - EN: CELL PROPERTIES
  - ES: PROPIEDADES DE CELDA
  - [ ] OK    Fix: 

- **`pressure`**
  - EN: PRESSURE MAPPINGS
  - ES: MAPEOS DE PRESIÓN
  - [ ] OK    Fix: 

- **`set`**
  - EN: SET MANAGEMENT
  - ES: GESTIÓN DE SETS
  - [ ] OK    Fix: 

## `sampler.set`

- **`copy`**
  - EN: (copy)
  - ES: (copia)
  - [ ] OK    Fix: 

- **`default`**
  - EN: Set
  - ES: Set
  - [ ] OK    Fix: 

- **`level`**
  - EN: Level (dB)
  - ES: Nivel (dB)
  - [ ] OK    Fix: 

- **`pos`**
  - EN: Position (m)
  - ES: Posición (m)
  - [ ] OK    Fix: 

- **`rename`**
  - EN: Rename
  - ES: Renombrar
  - [ ] OK    Fix: 

- **`roundRobin`**
  - EN: Round-Robin
  - ES: Round-Robin
  - [ ] OK    Fix: 

- **`sequential`**
  - EN: Sequential
  - ES: Secuencial
  - [ ] OK    Fix: 

## `sampler.tooltips`

- **`addSet`**
  - EN: Create a new set. If cells are selected they will be assigned to it.
  - ES: Crear un nuevo set. Si hay celdas seleccionadas, se le asignarán.
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Cell attenuation in dB (0 = no attenuation, -60 = silent)
  - ES: Atenuación de celda en dB (0 = sin atenuación, -60 = silencio)
  - [ ] OK    Fix: 

- **`clear`**
  - EN: Remove the sample from the selected cell (long press)
  - ES: Quitar la muestra de la celda seleccionada (pulsación larga)
  - [ ] OK    Fix: 

- **`copy`**
  - EN: Copy selected cell or active set to clipboard
  - ES: Copiar la celda seleccionada o el set activo al portapapeles
  - [ ] OK    Fix: 

- **`deleteSet`**
  - EN: Delete the active set (long press)
  - ES: Eliminar el set activo (pulsación larga)
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export sampler configuration to file
  - ES: Exportar la configuración del sampler a un archivo
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import sampler configuration from file
  - ES: Importar la configuración del sampler desde un archivo
  - [ ] OK    Fix: 

- **`inOut`**
  - EN: Set the In/Out time range in milliseconds. Drag between thumbs to shift both.
  - ES: Establecer el rango de tiempo Entrada/Salida en milisegundos. Arrastrar entre los pomos para mover ambos.
  - [ ] OK    Fix: 

- **`load`**
  - EN: Load a sample file into the selected cell
  - ES: Cargar un archivo de muestra en la celda seleccionada
  - [ ] OK    Fix: 

- **`offset`**
  - EN: Offset position in meters (X, Y, Z) relative to the set position
  - ES: Desplazamiento de posición en metros (X, Y, Z) relativo a la posición del set
  - [ ] OK    Fix: 

- **`paste`**
  - EN: Paste clipboard data to selected cell or active set
  - ES: Pegar los datos del portapapeles en la celda seleccionada o el set activo
  - [ ] OK    Fix: 

- **`playMode`**
  - EN: Toggle between Sequential and Round-Robin playback
  - ES: Alternar entre reproducción Secuencial y Round-Robin
  - [ ] OK    Fix: 

- **`pressCurve`**
  - EN: Pressure response curve (0 = concave, 0.5 = linear, 1 = convex)
  - ES: Curva de respuesta de presión (0 = cóncava, 0,5 = lineal, 1 = convexa)
  - [ ] OK    Fix: 

- **`pressDir`**
  - EN: Toggle pressure direction: + = more pressure increases, - = decreases
  - ES: Alternar la dirección de presión: + = más presión aumenta, - = disminuye
  - [ ] OK    Fix: 

- **`pressHeight`**
  - EN: Map finger pressure to vertical position (Z)
  - ES: Mapear la presión del dedo a la posición vertical (Z)
  - [ ] OK    Fix: 

- **`pressHF`**
  - EN: Map finger pressure to high-frequency shelf attenuation
  - ES: Mapear la presión del dedo a la atenuación del shelving agudo
  - [ ] OK    Fix: 

- **`pressLevel`**
  - EN: Map finger pressure to output level
  - ES: Mapear la presión del dedo al nivel de salida
  - [ ] OK    Fix: 

- **`pressXY`**
  - EN: Map finger pressure to XY position movement
  - ES: Mapear la presión del dedo al desplazamiento de posición XY
  - [ ] OK    Fix: 

- **`pressXYScale`**
  - EN: Sensitivity: how far the source moves per pressure step
  - ES: Sensibilidad: cuánto se mueve la fuente por cada paso de presión
  - [ ] OK    Fix: 

- **`preview`**
  - EN: Preview the loaded sample
  - ES: Pre-escuchar la muestra cargada
  - [ ] OK    Fix: 

- **`renameSet`**
  - EN: Rename the active set
  - ES: Renombrar el set activo
  - [ ] OK    Fix: 

- **`setLevel`**
  - EN: Set output level in dB (0 = unity, -60 = silent)
  - ES: Establecer el nivel de salida en dB (0 = unidad, -60 = silencio)
  - [ ] OK    Fix: 

- **`setPos`**
  - EN: Set base position in meters (X, Y, Z)
  - ES: Establecer la posición base en metros (X, Y, Z)
  - [ ] OK    Fix: 

## `sampler.zone`

- **`remotePad`**
  - EN: Pad {num}
  - ES: Pad {num}
  - [ ] OK    Fix: 

- **`selectRemotePad`**
  - EN: Select Remote Pad
  - ES: Seleccionar Pad del Mando
  - [ ] OK    Fix: 

## `setAllInputs`

- **`warning`**
  - EN: Changes will apply to ALL inputs
  - ES: Los cambios se aplicarán a TODAS las entradas
  - [ ] OK    Fix: 

- **`windowTitle`**
  - EN: Set All Inputs
  - ES: Aplicar a todas las entradas
  - [ ] OK    Fix: 

## `setAllInputs.buttons`

- **`all1d`**
  - EN: All 1/d
  - ES: Todos 1/d
  - [ ] OK    Fix: 

- **`allLog`**
  - EN: All Log
  - ES: Todos Log
  - [ ] OK    Fix: 

- **`closeWindow`**
  - EN: CLOSE WINDOW
  - ES: CERRAR VENTANA
  - [ ] OK    Fix: 

- **`flipXyzOff`**
  - EN: Flip XYZ > OFF
  - ES: Invertir XYZ > OFF
  - [ ] OK    Fix: 

- **`resetDirectivity`**
  - EN: Reset directivity
  - ES: Reiniciar directividad
  - [ ] OK    Fix: 

- **`turnOffJitterLfo`**
  - EN: Turn OFF jitter & LFO
  - ES: Desact. jitter & LFO
  - [ ] OK    Fix: 

- **`turnOffLiveSource`**
  - EN: Turn OFF Live source atten.
  - ES: Desact. atenuación Live Source
  - [ ] OK    Fix: 

## `setAllInputs.coordinateModes`

- **`rThetaPhi`**
  - EN: r θ φ
  - ES: r θ φ
  - [ ] OK    Fix: 

- **`rThetaZ`**
  - EN: r θ Z
  - ES: r θ Z
  - [ ] OK    Fix: 

- **`xyz`**
  - EN: XYZ
  - ES: XYZ
  - [ ] OK    Fix: 

## `setAllInputs.labels`

- **`common`**
  - EN: common
  - ES: común
  - [ ] OK    Fix: 

- **`constraintPositions`**
  - EN: Constraint positions:
  - ES: Restricciones de posición:
  - [ ] OK    Fix: 

- **`coordinateMode`**
  - EN: Coordinate mode:
  - ES: Modo de coordenadas:
  - [ ] OK    Fix: 

- **`dbPerMeter`**
  - EN: dB/m
  - ES: dB/m
  - [ ] OK    Fix: 

- **`distanceAttenuation`**
  - EN: Distance attenuation
  - ES: Atenuación por distancia
  - [ ] OK    Fix: 

- **`floorReflections`**
  - EN: Floor Reflections:
  - ES: Reflexiones del suelo:
  - [ ] OK    Fix: 

- **`fringe`**
  - EN: Fringe:
  - ES: Margen:
  - [ ] OK    Fix: 

- **`heightFactor`**
  - EN: Height factor:
  - ES: Factor de altura:
  - [ ] OK    Fix: 

- **`minimalLatency`**
  - EN: Minimal Latency:
  - ES: Latencia mínima:
  - [ ] OK    Fix: 

- **`muteMacros`**
  - EN: Mute macros:
  - ES: Macros de mute:
  - [ ] OK    Fix: 

- **`ratio`**
  - EN: ratio
  - ES: ratio
  - [ ] OK    Fix: 

- **`sidelines`**
  - EN: Sidelines:
  - ES: Líneas laterales:
  - [ ] OK    Fix: 

## `snapshot`

- **`qlabExportDone`**
  - EN: QLab export complete: {count} cues created
  - ES: Exportación QLab completa: {count} cues creados
  - [ ] OK    Fix: 

- **`qlabExportStarted`**
  - EN: Writing {count} cues to QLab...
  - ES: Escribiendo {count} cues a QLab...
  - [ ] OK    Fix: 

- **`qlabGroupName`**
  - EN: Snapshot "{name}"
  - ES: Captura "{name}"
  - [ ] OK    Fix: 

- **`qlabMemoText`**
  - EN: Run either of the following cues to recall or update this snapshot
  - ES: Ejecute cualquiera de los siguientes cues para recuperar o actualizar este snapshot
  - [ ] OK    Fix: 

- **`qlabNoTarget`**
  - EN: No QLab target configured
  - ES: No hay destino QLab configurado
  - [ ] OK    Fix: 

- **`qlabReloadName`**
  - EN: Reload "{name}"
  - ES: Recargar "{name}"
  - [ ] OK    Fix: 

- **`qlabUpdateName`**
  - EN: Update "{name}"
  - ES: Actualizar "{name}"
  - [ ] OK    Fix: 

## `snapshotScope`

- **`all`**
  - EN: ALL
  - ES: TODO
  - [ ] OK    Fix: 

- **`applyScope`**
  - EN: Apply scope:
  - ES: Aplicar alcance:
  - [ ] OK    Fix: 

- **`autoPreselectModified`**
  - EN: Auto-preselect modified parameters
  - ES: Preseleccionar automáticamente parámetros modificados
  - [ ] OK    Fix: 

- **`title`**
  - EN: Snapshot Scope: {name}
  - ES: Alcance del Snapshot: {name}
  - [ ] OK    Fix: 

- **`whenRecalling`**
  - EN: When Recalling
  - ES: Al recuperar
  - [ ] OK    Fix: 

- **`whenSaving`**
  - EN: When Saving
  - ES: Al guardar
  - [ ] OK    Fix: 

- **`windowTitle`**
  - EN: Input Snapshot Scope
  - ES: Alcance del Snapshot de Entrada
  - [ ] OK    Fix: 

- **`writeSnapshotLoadCue`**
  - EN: Write Snapshot Load Cue to QLab
  - ES: Escribir cue de carga en QLab
  - [ ] OK    Fix: 

- **`writeSnapshotLoadCueTooltip`**
  - EN: Also create a QLab cue to load this snapshot via OSC
  - ES: También crear un cue de QLab para cargar este snapshot vía OSC
  - [ ] OK    Fix: 

- **`writeToQLab`**
  - EN: Write to QLab
  - ES: Escribir a QLab
  - [ ] OK    Fix: 

- **`writeToQLabTooltip`**
  - EN: Export scope to QLab instead of saving to file
  - ES: Exportar scope a QLab en lugar de guardar en archivo
  - [ ] OK    Fix: 

## `snapshotScope.buttons`

- **`cancel`**
  - EN: Cancel
  - ES: Cancelar
  - [ ] OK    Fix: 

- **`clearChanges`**
  - EN: Clear Changes
  - ES: Borrar cambios
  - [ ] OK    Fix: 

- **`ok`**
  - EN: OK
  - ES: OK
  - [ ] OK    Fix: 

- **`selectModified`**
  - EN: Select Modified
  - ES: Seleccionar modificados
  - [ ] OK    Fix: 

## `snapshotScope.sections`

- **`attenuation`**
  - EN: Attenuation
  - ES: Atenuación
  - [ ] OK    Fix: 

- **`automOtion`**
  - EN: AutomOtion
  - ES: AutomOción
  - [ ] OK    Fix: 

- **`directivity`**
  - EN: Directivity
  - ES: Directividad
  - [ ] OK    Fix: 

- **`hackoustics`**
  - EN: Hackoustics
  - ES: Hackústica
  - [ ] OK    Fix: 

- **`input`**
  - EN: Input
  - ES: Entrada
  - [ ] OK    Fix: 

- **`lfo`**
  - EN: LFO
  - ES: LFO
  - [ ] OK    Fix: 

- **`liveSource`**
  - EN: Live Source
  - ES: Fuente en Vivo
  - [ ] OK    Fix: 

- **`mutes`**
  - EN: Mutes
  - ES: Mutes
  - [ ] OK    Fix: 

- **`position`**
  - EN: Position
  - ES: Posición
  - [ ] OK    Fix: 

## `statusBar`

- **`displayLabel`**
  - EN: Display:
  - ES: Mostrar:
  - [ ] OK    Fix: 

- **`helpMode`**
  - EN: Help
  - ES: Ayuda
  - [ ] OK    Fix: 

- **`oscMode`**
  - EN: OSC
  - ES: OSC
  - [ ] OK    Fix: 

## `systemConfig.algorithms`

- **`inputBuffer`**
  - EN: InputBuffer (read-time delays)
  - ES: InputBuffer (retardos en lectura)
  - [ ] OK    Fix: 

- **`outputBuffer`**
  - EN: OutputBuffer (write-time delays)
  - ES: OutputBuffer (retardos en escritura)
  - [ ] OK    Fix: 

## `systemConfig.binauralOutput`

- **`select`**
  - EN: Select...
  - ES: Seleccionar...
  - [ ] OK    Fix: 

## `systemConfig.buttons`

- **`audioPatch`**
  - EN: Audio Interface and Patching Window
  - ES: Interfaz de audio y enrutamiento
  - [ ] OK    Fix: 

- **`binauralOff`**
  - EN: Binaural: OFF
  - ES: Binaural: OFF
  - [ ] OK    Fix: 

- **`binauralOn`**
  - EN: Binaural: ON
  - ES: Binaural: ON
  - [ ] OK    Fix: 

- **`clearSolo`**
  - EN: Clear Solo
  - ES: Clear Solo
  - [ ] OK    Fix: 

- **`copySystemInfo`**
  - EN: Copy System Info
  - ES: Copiar info del sistema
  - [ ] OK    Fix: 

- **`diagnosticsCollapsed`**
  - EN: Diagnostics  [+]
  - ES: Diagnóstico  [+]
  - [ ] OK    Fix: 

- **`diagnosticsExpanded`**
  - EN: Diagnostics  [-]
  - ES: Diagnóstico  [-]
  - [ ] OK    Fix: 

- **`exportLogs`**
  - EN: Export Logs
  - ES: Exportar registros
  - [ ] OK    Fix: 

- **`exportSystem`**
  - EN: Export System Configuration
  - ES: Exportar configuración del sistema
  - [ ] OK    Fix: 

- **`importSystem`**
  - EN: Import System Configuration
  - ES: Importar configuración del sistema
  - [ ] OK    Fix: 

- **`levelMeter`**
  - EN: Level Meters
  - ES: Level Meters
  - [ ] OK    Fix: 

- **`openLogFolder`**
  - EN: Open Log Folder
  - ES: Abrir carpeta de registros
  - [ ] OK    Fix: 

- **`processingOff`**
  - EN: Processing: OFF
  - ES: Procesamiento: OFF
  - [ ] OK    Fix: 

- **`processingOn`**
  - EN: Processing: ON
  - ES: Procesamiento: ON
  - [ ] OK    Fix: 

- **`quickLongPressOff`**
  - EN: Normal
  - ES: Normal
  - [ ] OK    Fix: 

- **`quickLongPressOn`**
  - EN: Quick
  - ES: Rápido
  - [ ] OK    Fix: 

- **`reloadComplete`**
  - EN: Reload Complete Configuration
  - ES: Recargar configuración completa
  - [ ] OK    Fix: 

- **`reloadCompleteBackup`**
  - EN: Reload Complete Config. Backup
  - ES: Recargar config. completa (respaldo)
  - [ ] OK    Fix: 

- **`reloadSystem`**
  - EN: Reload System Configuration
  - ES: Recargar configuración del sistema
  - [ ] OK    Fix: 

- **`reloadSystemBackup`**
  - EN: Reload System Config. Backup
  - ES: Recargar config. sistema (respaldo)
  - [ ] OK    Fix: 

- **`reportIssue`**
  - EN: Report Issue
  - ES: Reportar problema
  - [ ] OK    Fix: 

- **`selectProjectFolder`**
  - EN: Select Project Folder
  - ES: Seleccionar carpeta del proyecto
  - [ ] OK    Fix: 

- **`setup`**
  - EN: Setup
  - ES: Config.
  - [ ] OK    Fix: 

- **`soloModeMulti`**
  - EN: Solo: Multi
  - ES: Solo: Múltiple
  - [ ] OK    Fix: 

- **`soloModeSingle`**
  - EN: Solo: Single
  - ES: Solo: Único
  - [ ] OK    Fix: 

- **`storeComplete`**
  - EN: Store Complete Configuration
  - ES: Guardar configuración completa
  - [ ] OK    Fix: 

- **`storeSystem`**
  - EN: Store System Configuration
  - ES: Guardar configuración del sistema
  - [ ] OK    Fix: 

## `systemConfig.colorSchemes`

- **`black`**
  - EN: Black
  - ES: Negro
  - [ ] OK    Fix: 

- **`default`**
  - EN: Default (Dark Gray)
  - ES: Por defecto (gris oscuro)
  - [ ] OK    Fix: 

- **`light`**
  - EN: Light
  - ES: Claro
  - [ ] OK    Fix: 

## `systemConfig.controller`

- **`lightpad`**
  - EN: Lightpad
  - ES: Lightpad
  - [ ] OK    Fix: 

- **`off`**
  - EN: Off
  - ES: Apagado
  - [ ] OK    Fix: 

- **`remote`**
  - EN: Remote
  - ES: Mando
  - [ ] OK    Fix: 

## `systemConfig.devices`

- **`off`**
  - EN: Off
  - ES: Apagado
  - [ ] OK    Fix: 

## `systemConfig.dialogs`

- **`exportSystemConfig`**
  - EN: Export System Configuration
  - ES: Exportar configuración del sistema
  - [ ] OK    Fix: 

- **`importSystemConfig`**
  - EN: Import System Configuration
  - ES: Importar configuración del sistema
  - [ ] OK    Fix: 

- **`reduce`**
  - EN: Reduce
  - ES: Reducir
  - [ ] OK    Fix: 

- **`reduceInputChannels.message`**
  - EN: Reducing from {current} to {new} input channels will remove settings for channels {start} to {end}.

This cannot be undone.
  - ES: Reducir de {current} a {new} canales de entrada eliminará la configuración de los canales {start} a {end}.

Esta acción no se puede deshacer.
  - [ ] OK    Fix: 

- **`reduceInputChannels.title`**
  - EN: Reduce Input Channels?
  - ES: ¿Reducir canales de entrada?
  - [ ] OK    Fix: 

- **`reduceOutputChannels.message`**
  - EN: Reducing from {current} to {new} output channels will remove settings for channels {start} to {end}.

This cannot be undone.
  - ES: Reducir de {current} a {new} canales de salida eliminará la configuración de los canales {start} a {end}.

Esta acción no se puede deshacer.
  - [ ] OK    Fix: 

- **`reduceOutputChannels.title`**
  - EN: Reduce Output Channels?
  - ES: ¿Reducir canales de salida?
  - [ ] OK    Fix: 

- **`reduceReverbChannels.message`**
  - EN: Reducing from {current} to {new} reverb channels will remove settings for channels {start} to {end}.

This cannot be undone.
  - ES: Reducir de {current} a {new} canales de reverb eliminará la configuración de los canales {start} a {end}.

Esta acción no se puede deshacer.
  - [ ] OK    Fix: 

- **`reduceReverbChannels.title`**
  - EN: Reduce Reverb Channels?
  - ES: ¿Reducir canales de reverb?
  - [ ] OK    Fix: 

- **`selectProjectFolder`**
  - EN: Select Project Folder
  - ES: Seleccionar carpeta del proyecto
  - [ ] OK    Fix: 

## `systemConfig.help`

- **`algorithm`**
  - EN: Select the rendering algorithm from the menu.
  - ES: Seleccione el algoritmo de renderizado del menú.
  - [ ] OK    Fix: 

- **`audioPatch`**
  - EN: Opens the Audio Interface and Patching Window.
  - ES: Abre la ventana de interfaz de audio y enrutamiento.
  - [ ] OK    Fix: 

- **`binauralAngle`**
  - EN: Horizontal rotation of binaural listener view (degrees, 0 = facing stage).
  - ES: Rotación horizontal de la vista del oyente binaural (grados, 0 = mirando al escenario).
  - [ ] OK    Fix: 

- **`binauralAtten`**
  - EN: Overall level offset for binaural output (dB).
  - ES: Desplazamiento de nivel global para la salida binaural (dB).
  - [ ] OK    Fix: 

- **`binauralDelay`**
  - EN: Additional delay for binaural output (milliseconds).
  - ES: Retardo adicional para la salida binaural (milisegundos).
  - [ ] OK    Fix: 

- **`binauralDistance`**
  - EN: Distance of binaural listener from stage origin (meters).
  - ES: Distancia del oyente binaural desde el origen del escenario (metros).
  - [ ] OK    Fix: 

- **`binauralEnable`**
  - EN: Enable or disable the binaural renderer processing.
  - ES: Activar o desactivar el procesamiento del renderizador binaural.
  - [ ] OK    Fix: 

- **`binauralOutput`**
  - EN: Select output channel pair for binaural monitoring. Off disables binaural output.
  - ES: Seleccionar el par de canales de salida para el monitoreo binaural. Off desactiva la salida binaural.
  - [ ] OK    Fix: 

- **`colorScheme`**
  - EN: Select the color scheme: Default (dark gray), Black (pure black for OLED displays), or Light (daytime use).
  - ES: Seleccione el esquema de color: Por defecto (gris oscuro), Negro (negro puro para pantallas OLED), o Claro (uso diurno).
  - [ ] OK    Fix: 

- **`copySystemInfo`**
  - EN: Copy detailed system information to the clipboard for support requests.
  - ES: Copiar información detallada del sistema al portapapeles para solicitudes de soporte.
  - [ ] OK    Fix: 

- **`diagnosticsToggle`**
  - EN: Long-press to show or hide the diagnostic tools (export logs, open log folder, copy system info, report issue).
  - ES: Pulsación larga para mostrar u ocultar las herramientas de diagnóstico (exportar registros, abrir carpeta de registros, copiar información del sistema, reportar problema).
  - [ ] OK    Fix: 

- **`dialsAndButtons`**
  - EN: Select the hardware controller for dials and buttons: Stream Deck+ or XenceLabs Quick Keys.
  - ES: Seleccionar el controlador hardware para diales y botones: Stream Deck+ o XenceLabs Quick Keys.
  - [ ] OK    Fix: 

- **`domeElevation`**
  - EN: Dome elevation angle: 180 = hemisphere, 360 = full sphere.
  - ES: Ángulo de elevación de la cúpula: 180 = hemisferio, 360 = esfera completa.
  - [ ] OK    Fix: 

- **`exportLogs`**
  - EN: Export diagnostic logs to a zip file for debugging or support.
  - ES: Exportar los registros de diagnóstico a un archivo zip para depuración o soporte.
  - [ ] OK    Fix: 

- **`exportSystem`**
  - EN: Export System Configuration to file (with file explorer window).
  - ES: Exporta la configuración del sistema a un archivo (con explorador de archivos).
  - [ ] OK    Fix: 

- **`haasEffect`**
  - EN: Hass Effect to apply to the system. Will take into account the Latency Compensations (System, Input and Output).
  - ES: Efecto Haas a aplicar al sistema. Tiene en cuenta las compensaciones de latencia (sistema, entrada y salida).
  - [ ] OK    Fix: 

- **`importSystem`**
  - EN: Import System Configuration from file (with file explorer window).
  - ES: Importa la configuración del sistema desde un archivo (con explorador de archivos).
  - [ ] OK    Fix: 

- **`inputChannels`**
  - EN: Number of Input Channels.
  - ES: Número de canales de entrada.
  - [ ] OK    Fix: 

- **`language`**
  - EN: Select the user interface language. Changes take full effect after restarting the application.
  - ES: Seleccione el idioma de la interfaz de usuario. Los cambios tienen efecto completo después de reiniciar la aplicación.
  - [ ] OK    Fix: 

- **`lightpadSetup`**
  - EN: Display the connected Roli Lightpads and allow for splitting them in 4 smaller pads.
  - ES: Mostrar los Roli Lightpads conectados y permitir dividirlos en 4 pads más pequeños.
  - [ ] OK    Fix: 

- **`masterLevel`**
  - EN: Master Level (affects all outputs).
  - ES: Nivel master (afecta a todas las salidas).
  - [ ] OK    Fix: 

- **`openLogFolder`**
  - EN: Open the application's log folder in the system file explorer.
  - ES: Abrir la carpeta de registros de la aplicación en el explorador de archivos del sistema.
  - [ ] OK    Fix: 

- **`originCenter`**
  - EN: Set origin to center of stage volume. Typical for Spherical Dome setups.
  - ES: Establece el origen en el centro del volumen del escenario. Típico para configuraciones de Cúpula Esférica.
  - [ ] OK    Fix: 

- **`originCenterGround`**
  - EN: Set origin to center of stage at ground level. Typical for Surround or Cylindrical setups.
  - ES: Establece el origen en el centro del escenario a nivel del suelo. Típico para configuraciones Surround o Cilíndricas.
  - [ ] OK    Fix: 

- **`originDepth`**
  - EN: Origin Y offset from stage center (0 = centered, negative = front/downstage).
  - ES: Desplazamiento Y del origen desde el centro del escenario (0 = centrado, negativo = frente/proscenio).
  - [ ] OK    Fix: 

- **`originDismissInputs`**
  - EN: Long press to dismiss and keep current input positions.
  - ES: Pulsación larga para descartar y mantener las posiciones actuales de las entradas.
  - [ ] OK    Fix: 

- **`originDismissOutputs`**
  - EN: Long press to dismiss and keep current output positions.
  - ES: Pulsación larga para descartar y mantener las posiciones actuales de las salidas.
  - [ ] OK    Fix: 

- **`originDismissReverbs`**
  - EN: Long press to dismiss and keep current reverb positions.
  - ES: Pulsación larga para descartar y mantener las posiciones actuales de las reverbs.
  - [ ] OK    Fix: 

- **`originFront`**
  - EN: Set origin to front center of stage. Typical for frontal stages.
  - ES: Establece el origen en el centro frontal del escenario. Típico para escenarios frontales.
  - [ ] OK    Fix: 

- **`originHeight`**
  - EN: Origin Z offset from floor (0 = floor level, positive = above floor).
  - ES: Desplazamiento Z del origen desde el suelo (0 = nivel del suelo, positivo = sobre el suelo).
  - [ ] OK    Fix: 

- **`originShiftInputs`**
  - EN: Long press to shift all input positions by the origin change.
  - ES: Pulsación larga para desplazar todas las posiciones de entrada según el cambio de origen.
  - [ ] OK    Fix: 

- **`originShiftOutputs`**
  - EN: Long press to shift all output positions by the origin change.
  - ES: Pulsación larga para desplazar todas las posiciones de salida según el cambio de origen.
  - [ ] OK    Fix: 

- **`originShiftReverbs`**
  - EN: Long press to shift all reverb positions by the origin change.
  - ES: Pulsación larga para desplazar todas las posiciones de reverb según el cambio de origen.
  - [ ] OK    Fix: 

- **`originWidth`**
  - EN: Origin X offset from stage center (0 = centered, negative = left).
  - ES: Desplazamiento X del origen desde el centro del escenario (0 = centrado, negativo = izquierda).
  - [ ] OK    Fix: 

- **`outputChannels`**
  - EN: Number of Output Channels.
  - ES: Número de canales de salida.
  - [ ] OK    Fix: 

- **`positionControl`**
  - EN: Select the hardware controller for position control: Space Mouse, Joystick, or Game Pad.
  - ES: Seleccionar el controlador hardware para el control de posición: Space Mouse, Joystick o gamepad.
  - [ ] OK    Fix: 

- **`processing`**
  - EN: Lock all I/O parameters and start the DSP. Long press to stop the DSP.
  - ES: Bloquea todos los parámetros de E/S e inicia el DSP. Mantenga presionado para detener el DSP.
  - [ ] OK    Fix: 

- **`quickLongPress`**
  - EN: Long press duration. Instead of confirmation windows this software uses long presses.
  - ES: Duración de la pulsación larga. En lugar de ventanas de confirmación, este software utiliza pulsaciones largas.
  - [ ] OK    Fix: 

- **`reloadComplete`**
  - EN: Reload Complete Configuration from files.
  - ES: Recarga la configuración completa desde archivos.
  - [ ] OK    Fix: 

- **`reloadCompleteBackup`**
  - EN: Reload Complete Configuration from backup files.
  - ES: Recarga la configuración completa desde archivos de respaldo.
  - [ ] OK    Fix: 

- **`reloadSystem`**
  - EN: Reload System Configuration from file.
  - ES: Recarga la configuración del sistema desde archivo.
  - [ ] OK    Fix: 

- **`reloadSystemBackup`**
  - EN: Reload System Configuration from backup file.
  - ES: Recarga la configuración del sistema desde archivo de respaldo.
  - [ ] OK    Fix: 

- **`remoteSetup`**
  - EN: Select the number of pads in the Remote's XY Pads tab.
  - ES: Seleccionar el número de pads en la pestaña XY Pads del Remote.
  - [ ] OK    Fix: 

- **`reportIssue`**
  - EN: Open the WFS-DIY GitHub issues page in your default browser.
  - ES: Abrir la página de issues de WFS-DIY en GitHub en el navegador predeterminado.
  - [ ] OK    Fix: 

- **`reverbChannels`**
  - EN: Number of Reverb Channels.
  - ES: Número de canales de reverberación.
  - [ ] OK    Fix: 

- **`sampler`**
  - EN: Enable or disable the Sampler feature for input channels.
  - ES: Activar o desactivar la función Sampler para los canales de entrada. Seleccionar el controlador: Lightpad o Mando.
  - [ ] OK    Fix: 

- **`selectProjectFolder`**
  - EN: Select the Location of the Current Project Folder where to store files.
  - ES: Seleccione la ubicación de la carpeta del proyecto actual para almacenar archivos.
  - [ ] OK    Fix: 

- **`showLocation`**
  - EN: Location of the current show.
  - ES: Ubicación del show actual.
  - [ ] OK    Fix: 

- **`showName`**
  - EN: Name of the current show.
  - ES: Nombre del show actual.
  - [ ] OK    Fix: 

- **`soloMode`**
  - EN: Single: one input at a time. Multi: multiple inputs simultaneously.
  - ES: Único: una entrada a la vez. Múltiple: varias entradas simultáneamente.
  - [ ] OK    Fix: 

- **`speedOfSound`**
  - EN: Speed of Sound (related to the temperature).
  - ES: Velocidad del sonido (relacionada con la temperatura).
  - [ ] OK    Fix: 

- **`stageDepth`**
  - EN: Depth of the stage in meters (Box shape only).
  - ES: Profundidad del escenario en metros (solo forma Caja).
  - [ ] OK    Fix: 

- **`stageDiameter`**
  - EN: Diameter of the stage in meters (Cylinder and Dome shapes).
  - ES: Diámetro del escenario en metros (formas Cilindro y Cúpula).
  - [ ] OK    Fix: 

- **`stageDismiss`**
  - EN: Long press to dismiss and keep current input positions.
  - ES: Pulsación larga para descartar y mantener las posiciones actuales de las entradas.
  - [ ] OK    Fix: 

- **`stageFit`**
  - EN: Long press to move outlier inputs back within the new stage bounds.
  - ES: Pulsación larga para mover las entradas fuera de los límites dentro de las nuevas dimensiones del escenario.
  - [ ] OK    Fix: 

- **`stageHeight`**
  - EN: Height of the stage in meters (Box and Cylinder shapes).
  - ES: Altura del escenario en metros (formas Caja y Cilindro).
  - [ ] OK    Fix: 

- **`stageScale`**
  - EN: Long press to scale all input positions proportionally to the new stage dimensions.
  - ES: Pulsación larga para escalar todas las posiciones de entrada proporcionalmente a las nuevas dimensiones del escenario.
  - [ ] OK    Fix: 

- **`stageShape`**
  - EN: Select the shape of the stage (Box, Cylinder, or Dome).
  - ES: Seleccione la forma del escenario (Caja, Cilindro o Cúpula).
  - [ ] OK    Fix: 

- **`stageWidth`**
  - EN: Width of the stage in meters (Box shape only).
  - ES: Ancho del escenario en metros (solo forma Caja).
  - [ ] OK    Fix: 

- **`storeComplete`**
  - EN: Store Complete Configuration to files (with backup).
  - ES: Guarda la configuración completa en archivos (con respaldo).
  - [ ] OK    Fix: 

- **`storeSystem`**
  - EN: Store System Configuration to file (with backup).
  - ES: Guarda la configuración del sistema en un archivo (con respaldo).
  - [ ] OK    Fix: 

- **`systemLatency`**
  - EN: Total latency of the system (Mixing board & Computer) / Specific Input and Output Latency/Delay can be set in the respective Input and Output settings.
  - ES: Latencia total del sistema (mesa de mezclas y ordenador) / La latencia/retardo específico de entradas y salidas puede configurarse en sus respectivos ajustes.
  - [ ] OK    Fix: 

- **`temperature`**
  - EN: Temperature (gives the Speed of Sound).
  - ES: Temperatura (determina la velocidad del sonido).
  - [ ] OK    Fix: 

## `systemConfig.labels`

- **`algorithm`**
  - EN: Algorithm:
  - ES: Algoritmo:
  - [ ] OK    Fix: 

- **`binauralAngle`**
  - EN: Listener Angle:
  - ES: Ángulo oyente:
  - [ ] OK    Fix: 

- **`binauralAtten`**
  - EN: Binaural Level:
  - ES: Nivel binaural:
  - [ ] OK    Fix: 

- **`binauralDelay`**
  - EN: Binaural Delay:
  - ES: Retardo binaural:
  - [ ] OK    Fix: 

- **`binauralDistance`**
  - EN: Listener Distance:
  - ES: Distancia oyente:
  - [ ] OK    Fix: 

- **`binauralOutput`**
  - EN: Binaural Output:
  - ES: Salida binaural:
  - [ ] OK    Fix: 

- **`clickToSplit`**
  - EN: Click to split
  - ES: Clic para dividir
  - [ ] OK    Fix: 

- **`colorScheme`**
  - EN: Color Scheme:
  - ES: Esquema de color:
  - [ ] OK    Fix: 

- **`dialsAndButtons`**
  - EN: Dials and Buttons:
  - ES: Diales y botones:
  - [ ] OK    Fix: 

- **`domeElevation`**
  - EN: Elevation:
  - ES: Elevación:
  - [ ] OK    Fix: 

- **`haasEffect`**
  - EN: Haas Effect:
  - ES: Efecto Haas:
  - [ ] OK    Fix: 

- **`inputChannels`**
  - EN: Input Channels:
  - ES: Canales de entrada:
  - [ ] OK    Fix: 

- **`language`**
  - EN: Language:
  - ES: Idioma:
  - [ ] OK    Fix: 

- **`lightpadArrangement`**
  - EN: Lightpad Arrangement
  - ES: Disposición Lightpad
  - [ ] OK    Fix: 

- **`masterLevel`**
  - EN: Master Level:
  - ES: Nivel master:
  - [ ] OK    Fix: 

- **`originDepth`**
  - EN: Origin Depth:
  - ES: Origen profundidad:
  - [ ] OK    Fix: 

- **`originHeight`**
  - EN: Origin Height:
  - ES: Origen altura:
  - [ ] OK    Fix: 

- **`originWidth`**
  - EN: Origin Width:
  - ES: Origen ancho:
  - [ ] OK    Fix: 

- **`outputChannels`**
  - EN: Output Channels:
  - ES: Canales de salida:
  - [ ] OK    Fix: 

- **`positionControl`**
  - EN: Position Control:
  - ES: Control de posición:
  - [ ] OK    Fix: 

- **`quickLongPress`**
  - EN: Long Press:
  - ES: Pulsación larga:
  - [ ] OK    Fix: 

- **`reverbChannels`**
  - EN: Reverb Channels:
  - ES: Canales de reverb:
  - [ ] OK    Fix: 

- **`sampler`**
  - EN: Sampler:
  - ES: Sampler:
  - [ ] OK    Fix: 

- **`showLocation`**
  - EN: Location:
  - ES: Ubicación:
  - [ ] OK    Fix: 

- **`showName`**
  - EN: Name:
  - ES: Nombre:
  - [ ] OK    Fix: 

- **`speedOfSound`**
  - EN: Speed of Sound:
  - ES: Velocidad del sonido:
  - [ ] OK    Fix: 

- **`split`**
  - EN: Split
  - ES: Dividir
  - [ ] OK    Fix: 

- **`stageDepth`**
  - EN: Depth:
  - ES: Profundidad:
  - [ ] OK    Fix: 

- **`stageDiameter`**
  - EN: Diameter:
  - ES: Diámetro:
  - [ ] OK    Fix: 

- **`stageHeight`**
  - EN: Height:
  - ES: Altura:
  - [ ] OK    Fix: 

- **`stageShape`**
  - EN: Stage Shape:
  - ES: Forma del escenario:
  - [ ] OK    Fix: 

- **`stageWidth`**
  - EN: Width:
  - ES: Ancho:
  - [ ] OK    Fix: 

- **`systemLatency`**
  - EN: System Latency:
  - ES: Latencia del sistema:
  - [ ] OK    Fix: 

- **`temperature`**
  - EN: Temperature:
  - ES: Temperatura:
  - [ ] OK    Fix: 

- **`updateAvailable`**
  - EN: Update {version} available
  - ES: Actualización {version} disponible
  - [ ] OK    Fix: 

- **`version`**
  - EN: Version {version}
  - ES: Versión {version}
  - [ ] OK    Fix: 

## `systemConfig.messages`

- **`configLoaded`**
  - EN: Complete configuration loaded.
  - ES: Configuración completa cargada.
  - [ ] OK    Fix: 

- **`configLoadedFromBackup`**
  - EN: Configuration loaded from backup.
  - ES: Configuración cargada desde respaldo.
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Complete configuration saved.
  - ES: Configuración completa guardada.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - ES: Error: {error}
  - [ ] OK    Fix: 

- **`languageChanged`**
  - EN: Language changed to: {language} (requires restart for full effect)
  - ES: Idioma cambiado a: {language} (requiere reinicio para efecto completo)
  - [ ] OK    Fix: 

- **`logDirNotFound`**
  - EN: Log directory not found
  - ES: Carpeta de registros no encontrada
  - [ ] OK    Fix: 

- **`logsExported`**
  - EN: Logs exported to {path}
  - ES: Registros exportados a {path}
  - [ ] OK    Fix: 

- **`logsExportFailed`**
  - EN: Failed to export logs
  - ES: Error al exportar los registros
  - [ ] OK    Fix: 

- **`noBackupFilesFound`**
  - EN: No backup files found.
  - ES: No se encontraron archivos de respaldo.
  - [ ] OK    Fix: 

- **`partialLoad`**
  - EN: Partial load: {error}
  - ES: Carga parcial: {error}
  - [ ] OK    Fix: 

- **`partialLoadFromBackup`**
  - EN: Partial load from backup: {error}
  - ES: Carga parcial desde respaldo: {error}
  - [ ] OK    Fix: 

- **`restartBody`**
  - EN: Please restart the application for the language change to take full effect.
  - ES: Por favor, reinicie la aplicación para que el cambio de idioma surta pleno efecto.
  - [ ] OK    Fix: 

- **`restartTitle`**
  - EN: Restart Required
  - ES: Reinicio necesario
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder first.
  - ES: Por favor, seleccione primero una carpeta de proyecto.
  - [ ] OK    Fix: 

- **`selectLogExportFolder`**
  - EN: Select destination for log export
  - ES: Seleccionar destino para la exportación de registros
  - [ ] OK    Fix: 

- **`systemConfigExported`**
  - EN: System configuration exported.
  - ES: Configuración del sistema exportada.
  - [ ] OK    Fix: 

- **`systemConfigFileNotFound`**
  - EN: System configuration file not found.
  - ES: Archivo de configuración del sistema no encontrado.
  - [ ] OK    Fix: 

- **`systemConfigImported`**
  - EN: System configuration imported.
  - ES: Configuración del sistema importada.
  - [ ] OK    Fix: 

- **`systemConfigLoaded`**
  - EN: System configuration loaded.
  - ES: Configuración del sistema cargada.
  - [ ] OK    Fix: 

- **`systemConfigLoadedFromBackup`**
  - EN: System configuration loaded from backup.
  - ES: Configuración del sistema cargada desde respaldo.
  - [ ] OK    Fix: 

- **`systemConfigSaved`**
  - EN: System configuration saved.
  - ES: Configuración del sistema guardada.
  - [ ] OK    Fix: 

- **`systemInfoCopied`**
  - EN: System info copied to clipboard
  - ES: Información del sistema copiada al portapapeles
  - [ ] OK    Fix: 

## `systemConfig.sections`

- **`binauralRenderer`**
  - EN: Binaural Renderer
  - ES: Renderizador Binaural
  - [ ] OK    Fix: 

- **`controllers`**
  - EN: Controllers
  - ES: Controllers
  - [ ] OK    Fix: 

- **`io`**
  - EN: I/O
  - ES: E/S
  - [ ] OK    Fix: 

- **`master`**
  - EN: Master Section
  - ES: Sección Master
  - [ ] OK    Fix: 

- **`show`**
  - EN: Show
  - ES: Show
  - [ ] OK    Fix: 

- **`stage`**
  - EN: Stage
  - ES: Escenario
  - [ ] OK    Fix: 

- **`ui`**
  - EN: UI
  - ES: Interfaz
  - [ ] OK    Fix: 

- **`wfsProcessor`**
  - EN: WFS Processor
  - ES: Procesador WFS
  - [ ] OK    Fix: 

## `systemConfig.stageShapes`

- **`box`**
  - EN: Box
  - ES: Caja
  - [ ] OK    Fix: 

- **`cylinder`**
  - EN: Cylinder
  - ES: Cilindro
  - [ ] OK    Fix: 

- **`dome`**
  - EN: Dome
  - ES: Cúpula
  - [ ] OK    Fix: 

## `tabs`

- **`clusters`**
  - EN: Clusters
  - ES: Clusters
  - [ ] OK    Fix: 

- **`inputs`**
  - EN: Inputs
  - ES: Entradas
  - [ ] OK    Fix: 

- **`map`**
  - EN: Map
  - ES: Mapa
  - [ ] OK    Fix: 

- **`network`**
  - EN: Network
  - ES: Red
  - [ ] OK    Fix: 

- **`outputs`**
  - EN: Outputs
  - ES: Salidas
  - [ ] OK    Fix: 

- **`reverb`**
  - EN: Reverb
  - ES: Reverb
  - [ ] OK    Fix: 

- **`systemConfig`**
  - EN: System Config
  - ES: Configuración
  - [ ] OK    Fix: 

## `touchscreens`

- **`button`**
  - EN: Setup
  - ES: Configurar
  - [ ] OK    Fix: 

- **`label`**
  - EN: Touchscreen
  - ES: Pantalla táctil
  - [ ] OK    Fix: 

## `units`

- **`celsius`**
  - EN: °C
  - ES: °C
  - [ ] OK    Fix: 

- **`decibelPerMeter`**
  - EN: dB/m
  - ES: dB/m
  - [ ] OK    Fix: 

- **`decibels`**
  - EN: dB
  - ES: dB
  - [ ] OK    Fix: 

- **`degrees`**
  - EN: °
  - ES: °
  - [ ] OK    Fix: 

- **`meters`**
  - EN: m
  - ES: m
  - [ ] OK    Fix: 

- **`metersPerSecond`**
  - EN: m/s
  - ES: m/s
  - [ ] OK    Fix: 

- **`milliseconds`**
  - EN: ms
  - ES: ms
  - [ ] OK    Fix: 

## `wizard.buttons`

- **`back`**
  - EN: Back
  - ES: Anterior
  - [ ] OK    Fix: 

- **`close`**
  - EN: Close
  - ES: Cerrar
  - [ ] OK    Fix: 

- **`done`**
  - EN: Done
  - ES: Listo
  - [ ] OK    Fix: 

- **`gettingStarted`**
  - EN: Getting Started
  - ES: Inicio
  - [ ] OK    Fix: 

- **`gettingStartedHelp`**
  - EN: Help cards guiding you through the first parameters to adjust when starting a new project
  - ES: Tarjetas de ayuda que le guían a través de los primeros parámetros a ajustar al iniciar un nuevo proyecto
  - [ ] OK    Fix: 

- **`next`**
  - EN: Next
  - ES: Siguiente
  - [ ] OK    Fix: 

- **`skip`**
  - EN: Skip
  - ES: Saltar
  - [ ] OK    Fix: 

## `wizard.steps`

- **`audioDevice.description`**
  - EN: Select your audio driver and device, set the sample rate and buffer size. Check the patch routing and test your outputs. Close this window when done.
  - ES: Seleccione su controlador y dispositivo de audio, establezca la frecuencia de muestreo y el tamaño del búfer. Verifique el enrutamiento y pruebe sus salidas. Cierre esta ventana cuando termine.
  - [ ] OK    Fix: 

- **`audioDevice.title`**
  - EN: Configure the Audio Interface
  - ES: Configurar la interfaz de audio
  - [ ] OK    Fix: 

- **`audioInterface.description`**
  - EN: Click the button above or press Next to open the Audio Interface window.
  - ES: Haga clic en el botón de arriba o presione Siguiente para abrir la ventana de la interfaz de audio.
  - [ ] OK    Fix: 

- **`audioInterface.title`**
  - EN: Open the Audio Interface
  - ES: Abrir la interfaz de audio
  - [ ] OK    Fix: 

- **`configureOutputs.description`**
  - EN: Use the array presets and geometry tools to calculate speaker positions for your arrays. Close this window when done.
  - ES: Use los presets de arreglos y herramientas de geometría para calcular las posiciones de los altavoces. Cierre esta ventana cuando termine.
  - [ ] OK    Fix: 

- **`configureOutputs.title`**
  - EN: Configure Output Positions
  - ES: Configurar posiciones de salida
  - [ ] OK    Fix: 

- **`exploreInputs.description`**
  - EN: Click an input on the map to select it, or lasso several to move them together. Drag to position your sources. Zoom with the mouse wheel or pinch gesture, pan with right-drag or two-finger drag. Add inputs, group them into clusters, and shape your sound field. You can also control positions with a keyboard, SpaceMouse, or other external controllers. Have fun!
  - ES: Haga clic en una entrada en el mapa para seleccionarla, o use el lazo para seleccionar varias. Arrastre para posicionar sus fuentes. Zoom con la rueda del ratón o gesto de pellizco, desplace la vista con clic derecho o arrastre con dos dedos. Añada entradas, agrúpelas en clusters y dé forma a su campo sonoro. También puede controlar posiciones con teclado, SpaceMouse u otros controladores. ¡Diviértase!
  - [ ] OK    Fix: 

- **`exploreInputs.title`**
  - EN: Start Creating!
  - ES: ¡Empiece a crear!
  - [ ] OK    Fix: 

- **`inputChannels.description`**
  - EN: How many audio sources will you be spatializing?
Set the number of input channels to match your source count.
  - ES: ¿Cuántas fuentes de audio va a espacializar?
Establezca el número de canales de entrada según sus fuentes.
  - [ ] OK    Fix: 

- **`inputChannels.title`**
  - EN: Set Input Channels
  - ES: Configurar canales de entrada
  - [ ] OK    Fix: 

- **`originPoint.description`**
  - EN: The origin is the reference point for all coordinates. Use the preset buttons or enter custom values. 'Front' places it at the audience edge.
  - ES: El origen es el punto de referencia para todas las coordenadas. Use los botones de preselección o ingrese valores personalizados. 'Front' lo coloca en el borde del público.
  - [ ] OK    Fix: 

- **`originPoint.title`**
  - EN: Set the Origin Point
  - ES: Establecer el punto de origen
  - [ ] OK    Fix: 

- **`outputChannels.description`**
  - EN: Set the number of output channels to match your speaker array.
Each output corresponds to one physical speaker.
  - ES: Establezca el número de canales de salida según su arreglo de altavoces.
Cada salida corresponde a un altavoz físico.
  - [ ] OK    Fix: 

- **`outputChannels.title`**
  - EN: Set Output Channels
  - ES: Configurar canales de salida
  - [ ] OK    Fix: 

- **`projectFolder.description`**
  - EN: Choose a folder to store your WFS project files. This will hold configurations, snapshots, IR files, and samples. Click the button to open the folder selector.
  - ES: Elija una carpeta para almacenar los archivos de su proyecto WFS. Contendrá configuraciones, snapshots, archivos IR y samples. Haga clic en el botón para abrir el selector de carpetas.
  - [ ] OK    Fix: 

- **`projectFolder.title`**
  - EN: Select a Project Folder
  - ES: Seleccionar carpeta de proyecto
  - [ ] OK    Fix: 

- **`reverbChannels.description`**
  - EN: Reverb channels add room simulation. Set to 0 if you don't need reverb.
  - ES: Los canales de reverberación añaden simulación de sala. Establezca 0 si no necesita reverberación.
  - [ ] OK    Fix: 

- **`reverbChannels.title`**
  - EN: Set Reverb Channels
  - ES: Configurar canales de reverberación
  - [ ] OK    Fix: 

- **`stageConfig.description`**
  - EN: Set the shape and dimensions of your performance space. Choose box, cylinder, or dome, then enter the size in meters.
  - ES: Establezca la forma y dimensiones de su espacio de actuación. Elija caja, cilindro o cúpula, luego ingrese las medidas en metros.
  - [ ] OK    Fix: 

- **`stageConfig.title`**
  - EN: Define the Stage
  - ES: Definir el escenario
  - [ ] OK    Fix: 

- **`startProcessing.description`**
  - EN: You're all set! Long-press the Processing button to start the WFS engine. You can also start the Binaural Renderer for headphone monitoring.
  - ES: ¡Todo listo! Mantenga presionado el botón Processing para iniciar el motor WFS. También puede iniciar el renderizador binaural para monitoreo con auriculares.
  - [ ] OK    Fix: 

- **`startProcessing.title`**
  - EN: Start the WFS Engine
  - ES: Iniciar el motor WFS
  - [ ] OK    Fix: 

- **`wizardOfOutZ.description`**
  - EN: Click the Wizard of OutZ button or press Next to open the output array helper.
  - ES: Haga clic en el botón Wizard of OutZ o presione Siguiente para abrir el asistente de posicionamiento.
  - [ ] OK    Fix: 

- **`wizardOfOutZ.title`**
  - EN: Position Your Outputs
  - ES: Posicionar sus salidas
  - [ ] OK    Fix: 


