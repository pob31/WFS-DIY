# Proofreading checklist — Portuguese (Português)

Locale: `pt`  |  Total keys: 1520  |  Source: `Resources/lang/en.json` vs `Resources/lang/pt.json`

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
  - PT: aplicado
  - [ ] OK    Fix: 

- **`atHead`**
  - EN: (no undone records — at the head)
  - PT: (nenhum registo anulado — no topo)
  - [ ] OK    Fix: 

- **`batch`**
  - EN: batch {id}
  - PT: lote {id}
  - [ ] OK    Fix: 

- **`cursorLabel`**
  - EN: ◂  cursor (↑ applied  /  ↓ undone, redoable)
  - PT: ◂  cursor (↑ aplicado  /  ↓ anulado, recuperável)
  - [ ] OK    Fix: 

- **`noChanges`**
  - EN: No AI changes yet.
  - PT: Sem alterações da IA por enquanto.
  - [ ] OK    Fix: 

- **`of`**
  - EN: of
  - PT: de
  - [ ] OK    Fix: 

- **`stepBack`**
  - EN: ⏮ Step Back
  - PT: ⏮ Recuar
  - [ ] OK    Fix: 

- **`stepForward`**
  - EN: Step Forward ⏭
  - PT: Avançar ⏭
  - [ ] OK    Fix: 

- **`undone`**
  - EN: undone
  - PT: anulado
  - [ ] OK    Fix: 

- **`windowTitle`**
  - EN: AI Change History
  - PT: Histórico de alterações da IA
  - [ ] OK    Fix: 

## `ai.server`

- **`copyUrlConfirm`**
  - EN: MCP URL copied to clipboard: {url}
  - PT: URL MCP copiado para a área de transferência: {url}
  - [ ] OK    Fix: 

- **`label`**
  - EN: MCP Server:
  - PT: Servidor MCP:
  - [ ] OK    Fix: 

- **`openHistory`**
  - EN: Open AI History
  - PT: Abrir histórico da IA
  - [ ] OK    Fix: 

- **`urlButtonRunning`**
  - EN: http://{host}:{port}/mcp
  - PT: http://{host}:{port}/mcp
  - [ ] OK    Fix: 

- **`urlButtonStopped`**
  - EN: (server not running)
  - PT: (servidor parado)
  - [ ] OK    Fix: 

## `ai.tier`

- **`aiOff`**
  - EN: AI: OFF
  - PT: IA: OFF
  - [ ] OK    Fix: 

- **`aiOn`**
  - EN: AI: ON
  - PT: IA: ON
  - [ ] OK    Fix: 

- **`gateAllowed`**
  - EN: AI critical actions: ALLOWED
  - PT: Ações críticas da IA: PERMITIDAS
  - [ ] OK    Fix: 

- **`gateBlocked`**
  - EN: AI critical actions: blocked
  - PT: Ações críticas da IA: bloqueadas
  - [ ] OK    Fix: 

- **`tier2AutoOff`**
  - EN: Tier 2 auto-confirm: off
  - PT: Auto-confirmação nível 2: off
  - [ ] OK    Fix: 

- **`tier2AutoOn`**
  - EN: Tier 2 auto-confirm: ON (5 min)
  - PT: Auto-confirmação nível 2: ON (5 min)
  - [ ] OK    Fix: 

## `ai.toast`

- **`clientPrefix`**
  - EN: Claude
  - PT: Claude
  - [ ] OK    Fix: 

- **`header`**
  - EN: AI changes
  - PT: Alterações da IA
  - [ ] OK    Fix: 

- **`moreOlder`**
  - EN: …and {count} older
  - PT: …e mais {count} antigas
  - [ ] OK    Fix: 

## `ai.tooltips`

- **`aiToggle`**
  - EN: Master switch for the whole MCP integration. When OFF every AI tool call is refused; when ON, normal tier handling applies (the critical-actions toggle still controls destructive calls separately).
  - PT: Interruptor principal para toda a integração MCP. Quando OFF, cada chamada de ferramenta IA é recusada; quando ON, aplica-se a gestão normal por níveis (o interruptor de ações críticas controla separadamente as chamadas destrutivas).
  - [ ] OK    Fix: 

- **`gate`**
  - EN: Allow destructive AI actions (channel-count changes, port reconfigure, runDSP, etc.) AND skip the per-call confirm handshake for less-destructive tier-2 actions while open. Acts as a superset of the Tier 2 auto-confirm toggle. The red fill drains over 5 minutes, then auto-blocks again.
  - PT: Permitir ações IA destrutivas (alterações no número de canais, reconfiguração de portas, runDSP, etc.) E ignorar a confirmação por chamada para ações de tier-2 menos destrutivas enquanto estiver aberta. Atua como sobreconjunto do interruptor de auto-confirmação de Tier 2. O preenchimento vermelho esvazia em 5 minutos, depois bloqueia automaticamente.
  - [ ] OK    Fix: 

- **`openHistory`**
  - EN: Open the AI History window: scrollable timeline of every recent AI change with per-row undo / redo and a step-by-step cursor.
  - PT: Abrir a janela Histórico de IA: cronologia rolável de cada alteração recente da IA com anular/refazer por linha e cursor passo a passo.
  - [ ] OK    Fix: 

- **`urlButton`**
  - EN: Click to copy the MCP server URL. Useful for Claude Code (claude mcp add wfs-diy <URL> -t http) or any MCP client that takes a URL. Claude Desktop instead uses the JSON config snippet — open the (?) help card.
  - PT: Clicar para copiar o URL do servidor MCP. Útil para Claude Code (claude mcp add wfs-diy <URL> -t http) ou qualquer cliente MCP que aceite um URL. Claude Desktop usa antes o trecho de configuração JSON — abrir o cartão de ajuda (?).
  - [ ] OK    Fix: 

## `ai.undo`

- **`errorPrefix`**
  - EN: AI {verb}: {message}
  - PT: IA {verb}: {message}
  - [ ] OK    Fix: 

- **`successPrefix`**
  - EN: AI {verb}: {description}
  - PT: IA {verb}: {description}
  - [ ] OK    Fix: 

- **`verbRedo`**
  - EN: redo
  - PT: repetição
  - [ ] OK    Fix: 

- **`verbUndo`**
  - EN: undo
  - PT: anulação
  - [ ] OK    Fix: 

## `arrayHelper.acoustic`

- **`distanceAtten`**
  - EN: Distance Atten (%):
  - PT: Aten. Distância (%):
  - [ ] OK    Fix: 

- **`floorReflections`**
  - EN: Floor Reflections
  - PT: Reflexões do Chão
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: HF Damping (dB/m):
  - PT: Amortecimento HF (dB/m):
  - [ ] OK    Fix: 

- **`highCut`**
  - EN: High Cut (Hz):
  - PT: Corte Agudo (Hz):
  - [ ] OK    Fix: 

- **`hParallax`**
  - EN: H Parallax (m):
  - PT: Paralaxe H (m):
  - [ ] OK    Fix: 

- **`liveSource`**
  - EN: Live Source
  - PT: Fonte Ao Vivo
  - [ ] OK    Fix: 

- **`lowCut`**
  - EN: Low Cut (Hz):
  - PT: Corte Grave (Hz):
  - [ ] OK    Fix: 

- **`vParallax`**
  - EN: V Parallax (m):
  - PT: Paralaxe V (m):
  - [ ] OK    Fix: 

## `arrayHelper.buttons`

- **`apply`**
  - EN: Apply
  - PT: Aplicar
  - [ ] OK    Fix: 

- **`close`**
  - EN: Close
  - PT: Fechar
  - [ ] OK    Fix: 

## `arrayHelper.errors`

- **`noPositions`**
  - EN: No positions to apply. Check geometry parameters.
  - PT: Sem posições para aplicar. Verifique os parâmetros de geometria.
  - [ ] OK    Fix: 

- **`notEnoughOutputs`**
  - EN: Not enough output channels! Need {count} starting from {start}
  - PT: Canais de saída insuficientes! Necessários {count} a partir de {start}
  - [ ] OK    Fix: 

- **`prefix`**
  - EN: Error: 
  - PT: Erro: 
  - [ ] OK    Fix: 

- **`speakerCountZero`**
  - EN: Number of speakers must be greater than 0
  - PT: O número de altifalantes deve ser maior que 0
  - [ ] OK    Fix: 

## `arrayHelper.geometry`

- **`backFacing`**
  - EN: Back Facing
  - PT: Virado para Trás
  - [ ] OK    Fix: 

- **`centerSpacing`**
  - EN: Center + Spacing
  - PT: Centro + Espaçamento
  - [ ] OK    Fix: 

- **`centerX`**
  - EN: Center X (m):
  - PT: Centro X (m):
  - [ ] OK    Fix: 

- **`centerY`**
  - EN: Center Y (m):
  - PT: Centro Y (m):
  - [ ] OK    Fix: 

- **`endpoints`**
  - EN: Endpoints
  - PT: Pontos Finais
  - [ ] OK    Fix: 

- **`endX`**
  - EN: End X (m):
  - PT: Fim X (m):
  - [ ] OK    Fix: 

- **`endY`**
  - EN: End Y (m):
  - PT: Fim Y (m):
  - [ ] OK    Fix: 

- **`facingInward`**
  - EN: Facing Inward
  - PT: Virado para Dentro
  - [ ] OK    Fix: 

- **`facingOutward`**
  - EN: Facing Outward
  - PT: Virado para Fora
  - [ ] OK    Fix: 

- **`frontFacing`**
  - EN: Front Facing
  - PT: Virado para Frente
  - [ ] OK    Fix: 

- **`nPairs`**
  - EN: N Pairs:
  - PT: N Pares:
  - [ ] OK    Fix: 

- **`nSpeakers`**
  - EN: N Speakers:
  - PT: N Altifalantes:
  - [ ] OK    Fix: 

- **`orientation`**
  - EN: Orientation (deg):
  - PT: Orientação (graus):
  - [ ] OK    Fix: 

- **`radius`**
  - EN: Radius (m):
  - PT: Raio (m):
  - [ ] OK    Fix: 

- **`sag`**
  - EN: Sag (m):
  - PT: Curvatura (m):
  - [ ] OK    Fix: 

- **`spacing`**
  - EN: Spacing (m):
  - PT: Espaçamento (m):
  - [ ] OK    Fix: 

- **`startAngle`**
  - EN: Start Angle (deg):
  - PT: Ângulo Inicial (graus):
  - [ ] OK    Fix: 

- **`startX`**
  - EN: Start X (m):
  - PT: Início X (m):
  - [ ] OK    Fix: 

- **`startY`**
  - EN: Start Y (m):
  - PT: Início Y (m):
  - [ ] OK    Fix: 

- **`width`**
  - EN: Width (m):
  - PT: Largura (m):
  - [ ] OK    Fix: 

- **`yEnd`**
  - EN: Y End (m):
  - PT: Y Fim (m):
  - [ ] OK    Fix: 

- **`yStart`**
  - EN: Y Start (m):
  - PT: Y Início (m):
  - [ ] OK    Fix: 

- **`zHeight`**
  - EN: Z Height (m):
  - PT: Altura Z (m):
  - [ ] OK    Fix: 

## `arrayHelper.presets`

- **`circle`**
  - EN: Circle
  - PT: Círculo
  - [ ] OK    Fix: 

- **`delayLine`**
  - EN: Delay Line
  - PT: Linha de Atraso
  - [ ] OK    Fix: 

- **`label`**
  - EN: Preset:
  - PT: Predefinição:
  - [ ] OK    Fix: 

- **`mainRoomStraight`**
  - EN: Main Flown Array Straight
  - PT: Array Suspenso Principal Reto
  - [ ] OK    Fix: 

- **`nearFieldCurved`**
  - EN: Near Field Array Curved
  - PT: Array Campo Próximo Curvo
  - [ ] OK    Fix: 

- **`nearFieldStraight`**
  - EN: Near Field Array Straight
  - PT: Array Campo Próximo Reto
  - [ ] OK    Fix: 

- **`subBass`**
  - EN: Sub Bass
  - PT: Sub Grave
  - [ ] OK    Fix: 

- **`surround`**
  - EN: Surround
  - PT: Surround
  - [ ] OK    Fix: 

## `arrayHelper.preview`

- **`audience`**
  - EN: Audience
  - PT: Audiência
  - [ ] OK    Fix: 

## `arrayHelper.sections`

- **`acousticDefaults`**
  - EN: Acoustic Defaults
  - PT: Padrões Acústicos
  - [ ] OK    Fix: 

- **`geometry`**
  - EN: Geometry
  - PT: Geometria
  - [ ] OK    Fix: 

- **`target`**
  - EN: Target
  - PT: Destino
  - [ ] OK    Fix: 

## `arrayHelper.status`

- **`applied`**
  - EN: Applied {count} speakers to Array {array}. Ready for next array.
  - PT: Aplicados {count} altifalantes ao Array {array}. Pronto para próximo array.
  - [ ] OK    Fix: 

- **`calculated`**
  - EN: Calculated {count} positions
  - PT: Calculadas {count} posições
  - [ ] OK    Fix: 

- **`ready`**
  - EN: Ready
  - PT: Pronto
  - [ ] OK    Fix: 

## `arrayHelper.target`

- **`array`**
  - EN: Array:
  - PT: Array:
  - [ ] OK    Fix: 

- **`arrayPrefix`**
  - EN: Array
  - PT: Array
  - [ ] OK    Fix: 

- **`startingOutput`**
  - EN: Starting Output:
  - PT: Saída Inicial:
  - [ ] OK    Fix: 

## `arrayHelper.window`

- **`contentName`**
  - EN: Output Array Helper
  - PT: Assistente de Array de Saída
  - [ ] OK    Fix: 

- **`title`**
  - EN: Wizard of OutZ
  - PT: Assistente OutZ
  - [ ] OK    Fix: 

## `audioPatch`

- **`windowTitle`**
  - EN: Audio Interface and Patching
  - PT: Interface de Áudio e Patch
  - [ ] OK    Fix: 

## `audioPatch.buttons`

- **`hold`**
  - EN: Hold
  - PT: Manter
  - [ ] OK    Fix: 

- **`unpatchAll`**
  - EN: Unpatch All
  - PT: Desligar Todos
  - [ ] OK    Fix: 

## `audioPatch.deviceSettings`

- **`buttons.controlPanel`**
  - EN: Control Panel
  - PT: Painel de Controlo
  - [ ] OK    Fix: 

- **`buttons.resetDevice`**
  - EN: Reset Device
  - PT: Reiniciar Dispositivo
  - [ ] OK    Fix: 

- **`labels.bufferSize`**
  - EN: Audio buffer size:
  - PT: Tamanho do buffer de áudio:
  - [ ] OK    Fix: 

- **`labels.device`**
  - EN: Device:
  - PT: Dispositivo:
  - [ ] OK    Fix: 

- **`labels.deviceType`**
  - EN: Audio device type:
  - PT: Tipo de dispositivo áudio:
  - [ ] OK    Fix: 

- **`labels.sampleRate`**
  - EN: Sample rate:
  - PT: Taxa de amostragem:
  - [ ] OK    Fix: 

- **`noDevice`**
  - EN: No Device
  - PT: Sem Dispositivo
  - [ ] OK    Fix: 

- **`notConfigured`**
  - EN: Not configured
  - PT: Não configurado
  - [ ] OK    Fix: 

## `audioPatch.labels`

- **`interfaceInput`**
  - EN: Audio Interface Input
  - PT: Entrada da interface de áudio
  - [ ] OK    Fix: 

- **`interfaceOutput`**
  - EN: Audio Interface Output
  - PT: Saída da interface de áudio
  - [ ] OK    Fix: 

- **`processorInputs`**
  - EN: Processor Inputs
  - PT: Entradas do processador
  - [ ] OK    Fix: 

- **`processorOutputs`**
  - EN: Processor Outputs
  - PT: Saídas do processador
  - [ ] OK    Fix: 

## `audioPatch.messages`

- **`chooseTestSignal`**
  - EN: Choose a Test Signal to Enable Testing
  - PT: Escolha um Sinal de Teste para Ativar o Modo de Teste
  - [ ] OK    Fix: 

## `audioPatch.modes`

- **`patching`**
  - EN: Patching
  - PT: Patch
  - [ ] OK    Fix: 

- **`scrolling`**
  - EN: Scrolling
  - PT: Deslocamento
  - [ ] OK    Fix: 

- **`testing`**
  - EN: Testing
  - PT: Teste
  - [ ] OK    Fix: 

## `audioPatch.tabs`

- **`deviceSettings`**
  - EN: Device Settings
  - PT: Definições do Dispositivo
  - [ ] OK    Fix: 

- **`inputPatch`**
  - EN: Input Patch
  - PT: Patch de Entrada
  - [ ] OK    Fix: 

- **`outputPatch`**
  - EN: Output Patch
  - PT: Patch de Saída
  - [ ] OK    Fix: 

## `audioPatch.testSignal`

- **`labels.frequency`**
  - EN: Frequency:
  - PT: Frequência:
  - [ ] OK    Fix: 

- **`labels.level`**
  - EN: Level:
  - PT: Nível:
  - [ ] OK    Fix: 

- **`labels.signal`**
  - EN: Signal:
  - PT: Sinal:
  - [ ] OK    Fix: 

- **`types.diracPulse`**
  - EN: Dirac Pulse
  - PT: Impulso Dirac
  - [ ] OK    Fix: 

- **`types.off`**
  - EN: Off
  - PT: Desligado
  - [ ] OK    Fix: 

- **`types.pinkNoise`**
  - EN: Pink Noise
  - PT: Ruído Rosa
  - [ ] OK    Fix: 

- **`types.pulse`**
  - EN: Pulse
  - PT: Impulso
  - [ ] OK    Fix: 

- **`types.sweep`**
  - EN: Sweep
  - PT: Varrimento
  - [ ] OK    Fix: 

- **`types.tone`**
  - EN: Tone
  - PT: Tom
  - [ ] OK    Fix: 

## `clusters.help`

- **`attenuationSlider`**
  - EN: Adjust attenuation of all cluster inputs (dB).
  - PT: Ajustar a atenuação de todas as entradas do cluster (dB).
  - [ ] OK    Fix: 

- **`exportPresets`**
  - EN: Export all 16 LFO presets to an XML file.
  - PT: Exportar os 16 presets LFO para um ficheiro XML.
  - [ ] OK    Fix: 

- **`importPresets`**
  - EN: Import LFO presets from an XML file.
  - PT: Importar presets LFO a partir de um ficheiro XML.
  - [ ] OK    Fix: 

- **`inputsVisibility`**
  - EN: Show or hide this cluster's inputs on the Map tab. Hide propagates to new members; removing an input restores its visibility.
  - PT: Mostrar ou ocultar as entradas deste cluster no Mapa. Ocultar propaga para novos membros; remover uma entrada restaura a sua visibilidade.
  - [ ] OK    Fix: 

- **`lfoActiveButton`**
  - EN: Enable or Disable the Periodic Movement of the Cluster (LFO).
  - PT: Ativar ou desativar o movimento periódico do cluster (LFO).
  - [ ] OK    Fix: 

- **`lfoAmplitudeRotSlider`**
  - EN: Maximum Rotation Angle (-360 to 360 Degrees).
  - PT: Ângulo de rotação máximo (-360 a 360 graus).
  - [ ] OK    Fix: 

- **`lfoAmplitudeScaleSlider`**
  - EN: Maximum Scale Factor (0.1x to 10x, Logarithmic).
  - PT: Fator de escala máximo (0,1× a 10×, logarítmico).
  - [ ] OK    Fix: 

- **`lfoAmplitudeXSlider`**
  - EN: Width of Movement in Relation to Cluster Reference Position.
  - PT: Largura do movimento em relação à posição de referência do cluster.
  - [ ] OK    Fix: 

- **`lfoAmplitudeYSlider`**
  - EN: Depth of Movement in Relation to Cluster Reference Position.
  - PT: Profundidade do movimento em relação à posição de referência do cluster.
  - [ ] OK    Fix: 

- **`lfoAmplitudeZSlider`**
  - EN: Height of Movement in Relation to Cluster Reference Position.
  - PT: Altura do movimento em relação à posição de referência do cluster.
  - [ ] OK    Fix: 

- **`lfoPeriodDial`**
  - EN: Base Period of the Cluster Movement.
  - PT: Período base do movimento do cluster.
  - [ ] OK    Fix: 

- **`lfoPhaseDial`**
  - EN: Global Phase Offset of the Cluster Movement.
  - PT: Desfasamento global do movimento do cluster.
  - [ ] OK    Fix: 

- **`lfoPhaseRotDial`**
  - EN: Phase Offset of the Cluster Rotation.
  - PT: Desfasamento da rotação do cluster.
  - [ ] OK    Fix: 

- **`lfoPhaseScaleDial`**
  - EN: Phase Offset of the Cluster Scaling.
  - PT: Desfasamento do redimensionamento do cluster.
  - [ ] OK    Fix: 

- **`lfoPhaseXDial`**
  - EN: Phase Offset of the Cluster Movement in Width.
  - PT: Desfasamento do movimento do cluster em largura.
  - [ ] OK    Fix: 

- **`lfoPhaseYDial`**
  - EN: Phase Offset of the Cluster Movement in Depth.
  - PT: Desfasamento do movimento do cluster em profundidade.
  - [ ] OK    Fix: 

- **`lfoPhaseZDial`**
  - EN: Phase Offset of the Cluster Movement in Height.
  - PT: Desfasamento do movimento do cluster em altura.
  - [ ] OK    Fix: 

- **`lfoRateRotSlider`**
  - EN: Faster or Slower Rotation in Relation to Base Period.
  - PT: Rotação mais rápida ou mais lenta em relação ao período base.
  - [ ] OK    Fix: 

- **`lfoRateScaleSlider`**
  - EN: Faster or Slower Scaling in Relation to Base Period.
  - PT: Redimensionamento mais rápido ou mais lento em relação ao período base.
  - [ ] OK    Fix: 

- **`lfoRateXSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Width.
  - PT: Movimento mais rápido ou mais lento em relação ao período base, em largura.
  - [ ] OK    Fix: 

- **`lfoRateYSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Depth.
  - PT: Movimento mais rápido ou mais lento em relação ao período base, em profundidade.
  - [ ] OK    Fix: 

- **`lfoRateZSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Height.
  - PT: Movimento mais rápido ou mais lento em relação ao período base, em altura.
  - [ ] OK    Fix: 

- **`lfoShapeRotSelector`**
  - EN: Rotation Behaviour of the Cluster.
  - PT: Comportamento de rotação do cluster.
  - [ ] OK    Fix: 

- **`lfoShapeScaleSelector`**
  - EN: Scale Behaviour of the Cluster.
  - PT: Comportamento de escala do cluster.
  - [ ] OK    Fix: 

- **`lfoShapeXSelector`**
  - EN: Movement Behaviour of the Cluster in Width.
  - PT: Comportamento do movimento do cluster em largura.
  - [ ] OK    Fix: 

- **`lfoShapeYSelector`**
  - EN: Movement Behaviour of the Cluster in Depth.
  - PT: Comportamento do movimento do cluster em profundidade.
  - [ ] OK    Fix: 

- **`lfoShapeZSelector`**
  - EN: Movement Behaviour of the Cluster in Height.
  - PT: Comportamento do movimento do cluster em altura.
  - [ ] OK    Fix: 

- **`planeSelector`**
  - EN: Select the plane for rotation and scale operations (XY, XZ, YZ).
  - PT: Selecionar o plano para as operações de rotação e escala (XY, XZ, YZ).
  - [ ] OK    Fix: 

- **`positionJoystick`**
  - EN: Move all cluster inputs in X/Y. Hold and drag to translate.
  - PT: Mover todas as entradas do cluster em X/Y. Manter pressionado e arrastar para transladar.
  - [ ] OK    Fix: 

- **`presetTile`**
  - EN: Click: recall preset. Double-click: recall + start. Middle/right-click: store current LFO.
  - PT: Clique: recuperar preset. Duplo clique: recuperar + iniciar. Clique do meio/direito: guardar o LFO atual.
  - [ ] OK    Fix: 

- **`qlabPreset`**
  - EN: Create a QLab network cue to recall the last selected LFO preset for the current cluster.
  - PT: Criar uma cue de rede QLab para recuperar o último preset LFO selecionado para o cluster atual.
  - [ ] OK    Fix: 

- **`referenceMode`**
  - EN: Select the reference point for cluster transforms: First Input or Barycenter.
  - PT: Selecione o ponto de referência para as transformações do cluster: Primeira Entrada ou Baricentro.
  - [ ] OK    Fix: 

- **`rotationDial`**
  - EN: Rotate all cluster inputs around the reference point in the selected plane.
  - PT: Rodar todas as entradas do cluster em torno do ponto de referência no plano selecionado.
  - [ ] OK    Fix: 

- **`scaleJoystick`**
  - EN: Scale cluster inputs relative to the reference point in the selected plane.
  - PT: Escalar as entradas do cluster relativamente ao ponto de referência no plano selecionado.
  - [ ] OK    Fix: 

- **`stopAllLFO`**
  - EN: Stop LFO on all 10 clusters.
  - PT: Parar o LFO em todos os 10 clusters.
  - [ ] OK    Fix: 

- **`zSlider`**
  - EN: Move all cluster inputs along the Z axis (height).
  - PT: Mover todas as entradas do cluster ao longo do eixo Z (altura).
  - [ ] OK    Fix: 

## `clusters.labels`

- **`assignedInputs`**
  - EN: Assigned Inputs
  - PT: Entradas Atribuídas
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Atten
  - PT: Aten.
  - [ ] OK    Fix: 

- **`controls`**
  - EN: Controls
  - PT: Controlos
  - [ ] OK    Fix: 

- **`inputPrefix`**
  - EN: Input
  - PT: Entrada
  - [ ] OK    Fix: 

- **`position`**
  - EN: Position
  - PT: Posição
  - [ ] OK    Fix: 

- **`posPrefix`**
  - EN: Pos:
  - PT: Pos:
  - [ ] OK    Fix: 

- **`reference`**
  - EN: Reference:
  - PT: Referência:
  - [ ] OK    Fix: 

- **`rotation`**
  - EN: Rotation
  - PT: Rotação
  - [ ] OK    Fix: 

- **`scale`**
  - EN: Scale
  - PT: Escala
  - [ ] OK    Fix: 

- **`x`**
  - EN: X:
  - PT: X:
  - [ ] OK    Fix: 

- **`y`**
  - EN: Y:
  - PT: Y:
  - [ ] OK    Fix: 

- **`z`**
  - EN: Z
  - PT: Z
  - [ ] OK    Fix: 

## `clusters.lfo`

- **`labels.amplitude`**
  - EN: Amplitude:
  - PT: Amplitude:
  - [ ] OK    Fix: 

- **`labels.angle`**
  - EN: Angle:
  - PT: Ângulo:
  - [ ] OK    Fix: 

- **`labels.period`**
  - EN: Period:
  - PT: Período:
  - [ ] OK    Fix: 

- **`labels.phase`**
  - EN: Phase:
  - PT: Fase:
  - [ ] OK    Fix: 

- **`labels.rate`**
  - EN: Rate:
  - PT: Velocidade:
  - [ ] OK    Fix: 

- **`labels.ratio`**
  - EN: Ratio:
  - PT: Razão:
  - [ ] OK    Fix: 

- **`labels.rot`**
  - EN: Rotation
  - PT: Rotação
  - [ ] OK    Fix: 

- **`labels.scaleLfo`**
  - EN: Scale
  - PT: Escala
  - [ ] OK    Fix: 

- **`labels.section`**
  - EN: LFO
  - PT: LFO
  - [ ] OK    Fix: 

- **`labels.x`**
  - EN: X
  - PT: X
  - [ ] OK    Fix: 

- **`labels.y`**
  - EN: Y
  - PT: Y
  - [ ] OK    Fix: 

- **`labels.z`**
  - EN: Z
  - PT: Z
  - [ ] OK    Fix: 

## `clusters.osc`

- **`lfoActive`**
  - EN: /wfs/cluster/lfoActive <id> <0|1>
  - PT: /wfs/cluster/lfoActive <id> <0|1>
  - [ ] OK    Fix: 

- **`lfoAmplitudeRot`**
  - EN: /wfs/cluster/lfoAmplitudeRot <id> <degrees>
  - PT: /wfs/cluster/lfoAmplitudeRot <id> <graus>
  - [ ] OK    Fix: 

- **`lfoAmplitudeScale`**
  - EN: /wfs/cluster/lfoAmplitudeScale <id> <factor>
  - PT: /wfs/cluster/lfoAmplitudeScale <id> <fator>
  - [ ] OK    Fix: 

- **`lfoAmplitudeX`**
  - EN: /wfs/cluster/lfoAmplitudeX <id> <meters>
  - PT: /wfs/cluster/lfoAmplitudeX <id> <metros>
  - [ ] OK    Fix: 

- **`lfoAmplitudeY`**
  - EN: /wfs/cluster/lfoAmplitudeY <id> <meters>
  - PT: /wfs/cluster/lfoAmplitudeY <id> <metros>
  - [ ] OK    Fix: 

- **`lfoAmplitudeZ`**
  - EN: /wfs/cluster/lfoAmplitudeZ <id> <meters>
  - PT: /wfs/cluster/lfoAmplitudeZ <id> <metros>
  - [ ] OK    Fix: 

- **`lfoPeriod`**
  - EN: /wfs/cluster/lfoPeriod <id> <seconds>
  - PT: /wfs/cluster/lfoPeriod <id> <segundos>
  - [ ] OK    Fix: 

- **`lfoPhase`**
  - EN: /wfs/cluster/lfoPhase <id> <degrees>
  - PT: /wfs/cluster/lfoPhase <id> <graus>
  - [ ] OK    Fix: 

- **`lfoPhaseRot`**
  - EN: /wfs/cluster/lfoPhaseRot <id> <degrees>
  - PT: /wfs/cluster/lfoPhaseRot <id> <graus>
  - [ ] OK    Fix: 

- **`lfoPhaseScale`**
  - EN: /wfs/cluster/lfoPhaseScale <id> <degrees>
  - PT: /wfs/cluster/lfoPhaseScale <id> <graus>
  - [ ] OK    Fix: 

- **`lfoPhaseX`**
  - EN: /wfs/cluster/lfoPhaseX <id> <degrees>
  - PT: /wfs/cluster/lfoPhaseX <id> <graus>
  - [ ] OK    Fix: 

- **`lfoPhaseY`**
  - EN: /wfs/cluster/lfoPhaseY <id> <degrees>
  - PT: /wfs/cluster/lfoPhaseY <id> <graus>
  - [ ] OK    Fix: 

- **`lfoPhaseZ`**
  - EN: /wfs/cluster/lfoPhaseZ <id> <degrees>
  - PT: /wfs/cluster/lfoPhaseZ <id> <graus>
  - [ ] OK    Fix: 

- **`lfoPresetRecall`**
  - EN: /wfs/cluster/lfoPresetRecall <clusterId> <presetNumber>
  - PT: /wfs/cluster/lfoPresetRecall <clusterId> <numeroPreset>
  - [ ] OK    Fix: 

- **`lfoRateRot`**
  - EN: /wfs/cluster/lfoRateRot <id> <multiplier>
  - PT: /wfs/cluster/lfoRateRot <id> <multiplicador>
  - [ ] OK    Fix: 

- **`lfoRateScale`**
  - EN: /wfs/cluster/lfoRateScale <id> <multiplier>
  - PT: /wfs/cluster/lfoRateScale <id> <multiplicador>
  - [ ] OK    Fix: 

- **`lfoRateX`**
  - EN: /wfs/cluster/lfoRateX <id> <multiplier>
  - PT: /wfs/cluster/lfoRateX <id> <multiplicador>
  - [ ] OK    Fix: 

- **`lfoRateY`**
  - EN: /wfs/cluster/lfoRateY <id> <multiplier>
  - PT: /wfs/cluster/lfoRateY <id> <multiplicador>
  - [ ] OK    Fix: 

- **`lfoRateZ`**
  - EN: /wfs/cluster/lfoRateZ <id> <multiplier>
  - PT: /wfs/cluster/lfoRateZ <id> <multiplicador>
  - [ ] OK    Fix: 

- **`lfoShapeRot`**
  - EN: /wfs/cluster/lfoShapeRot <id> <0-8>
  - PT: /wfs/cluster/lfoShapeRot <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoShapeScale`**
  - EN: /wfs/cluster/lfoShapeScale <id> <0-8>
  - PT: /wfs/cluster/lfoShapeScale <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoShapeX`**
  - EN: /wfs/cluster/lfoShapeX <id> <0-8>
  - PT: /wfs/cluster/lfoShapeX <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoShapeY`**
  - EN: /wfs/cluster/lfoShapeY <id> <0-8>
  - PT: /wfs/cluster/lfoShapeY <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoShapeZ`**
  - EN: /wfs/cluster/lfoShapeZ <id> <0-8>
  - PT: /wfs/cluster/lfoShapeZ <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoStopAll`**
  - EN: /wfs/cluster/lfoStopAll
  - PT: /wfs/cluster/lfoStopAll
  - [ ] OK    Fix: 

## `clusters.planes`

- **`xy`**
  - EN: XY
  - PT: XY
  - [ ] OK    Fix: 

- **`xz`**
  - EN: XZ
  - PT: XZ
  - [ ] OK    Fix: 

- **`yz`**
  - EN: YZ
  - PT: YZ
  - [ ] OK    Fix: 

## `clusters.presets`

- **`export`**
  - EN: Export
  - PT: Exportar
  - [ ] OK    Fix: 

- **`exportDialog`**
  - EN: Export LFO Presets
  - PT: Exportar presets LFO
  - [ ] OK    Fix: 

- **`exported`**
  - EN: LFO presets exported.
  - PT: Presets LFO exportados.
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - PT: Importar
  - [ ] OK    Fix: 

- **`importDialog`**
  - EN: Import LFO Presets
  - PT: Importar presets LFO
  - [ ] OK    Fix: 

- **`imported`**
  - EN: LFO presets imported.
  - PT: Presets LFO importados.
  - [ ] OK    Fix: 

- **`recalled`**
  - EN: LFO preset recalled from tile {n}.
  - PT: Preset LFO recuperado da casa {n}.
  - [ ] OK    Fix: 

- **`stopAll`**
  - EN: Stop All
  - PT: Parar tudo
  - [ ] OK    Fix: 

- **`stored`**
  - EN: LFO preset stored in tile {n}.
  - PT: Preset LFO guardado na casa {n}.
  - [ ] OK    Fix: 

## `clusters.referenceMode`

- **`barycenter`**
  - EN: Barycenter
  - PT: Baricentro
  - [ ] OK    Fix: 

- **`firstInput`**
  - EN: First Input
  - PT: Primeira Entrada
  - [ ] OK    Fix: 

## `clusters.status`

- **`noInputs`**
  - EN: No inputs assigned
  - PT: Sem entradas atribuídas
  - [ ] OK    Fix: 

- **`trackedMarker`**
  - EN: [T]
  - PT: [T]
  - [ ] OK    Fix: 

- **`tracking`**
  - EN: Tracking: Input {num} (overrides reference)
  - PT: Rastreamento: Entrada {num} (sobrepõe referência)
  - [ ] OK    Fix: 

## `clusters.toggles`

- **`inputsHidden`**
  - EN: Inputs: Hidden
  - PT: Entradas: Ocultas
  - [ ] OK    Fix: 

- **`inputsVisible`**
  - EN: Inputs: Visible
  - PT: Entradas: Visíveis
  - [ ] OK    Fix: 

- **`lfoOff`**
  - EN: L.F.O: OFF
  - PT: L.F.O: OFF
  - [ ] OK    Fix: 

- **`lfoOn`**
  - EN: L.F.O: ON
  - PT: L.F.O: ON
  - [ ] OK    Fix: 

## `common`

- **`cancel`**
  - EN: Cancel
  - PT: Cancelar
  - [ ] OK    Fix: 

- **`off`**
  - EN: OFF
  - PT: DESLIGADO
  - [ ] OK    Fix: 

- **`ok`**
  - EN: OK
  - PT: OK
  - [ ] OK    Fix: 

- **`on`**
  - EN: ON
  - PT: LIGADO
  - [ ] OK    Fix: 

## `eq.buttons`

- **`flattenEQ`**
  - EN: Flatten EQ
  - PT: Reiniciar EQ
  - [ ] OK    Fix: 

- **`resetBand`**
  - EN: Reset
  - PT: Reiniciar
  - [ ] OK    Fix: 

## `eq.filterTypes`

- **`allPass`**
  - EN: All-Pass
  - PT: Passa-tudo
  - [ ] OK    Fix: 

- **`bandPass`**
  - EN: Band Pass
  - PT: Passa-Banda
  - [ ] OK    Fix: 

- **`highCut`**
  - EN: High Cut
  - PT: Corte Agudo
  - [ ] OK    Fix: 

- **`highShelf`**
  - EN: High Shelf
  - PT: Prateleira Agudo
  - [ ] OK    Fix: 

- **`lowCut`**
  - EN: Low Cut
  - PT: Corte Grave
  - [ ] OK    Fix: 

- **`lowShelf`**
  - EN: Low Shelf
  - PT: Prateleira Grave
  - [ ] OK    Fix: 

- **`peakNotch`**
  - EN: Peak/Notch
  - PT: Pico/Notch
  - [ ] OK    Fix: 

## `eq.labels`

- **`band`**
  - EN: Band
  - PT: Banda
  - [ ] OK    Fix: 

- **`freq`**
  - EN: Freq:
  - PT: Freq:
  - [ ] OK    Fix: 

- **`gain`**
  - EN: Gain
  - PT: Ganho
  - [ ] OK    Fix: 

- **`q`**
  - EN: Q
  - PT: Q
  - [ ] OK    Fix: 

## `eq.status`

- **`off`**
  - EN: EQ OFF
  - PT: EQ DESLIGADO
  - [ ] OK    Fix: 

- **`on`**
  - EN: EQ ON
  - PT: EQ LIGADO
  - [ ] OK    Fix: 

## `fileManager.dialogs`

- **`selectProjectFolder`**
  - EN: Select Project Folder
  - PT: Selecionar pasta de projeto
  - [ ] OK    Fix: 

## `fileManager.errors`

- **`backupNotFound`**
  - EN: Backup not found
  - PT: Cópia de segurança não encontrada
  - [ ] OK    Fix: 

- **`configStateInvalid`**
  - EN: Config state is invalid
  - PT: Estado de configuração inválido
  - [ ] OK    Fix: 

- **`failedApply`**
  - EN: Failed to apply: {sections}
  - PT: Falha ao aplicar: {sections}
  - [ ] OK    Fix: 

- **`failedCreateFolder`**
  - EN: Failed to create project folder: {path}
  - PT: Falha ao criar a pasta do projeto: {path}
  - [ ] OK    Fix: 

- **`failedCreateValueTree`**
  - EN: Failed to create ValueTree from XML: {path}
  - PT: Falha ao criar o ValueTree a partir do XML: {path}
  - [ ] OK    Fix: 

- **`failedCreateXML`**
  - EN: Failed to create XML from state
  - PT: Falha ao criar o XML a partir do estado
  - [ ] OK    Fix: 

- **`failedParseXML`**
  - EN: Failed to parse XML file: {path}
  - PT: Falha ao analisar o ficheiro XML: {path}
  - [ ] OK    Fix: 

- **`failedWriteFile`**
  - EN: Failed to write file: {path}
  - PT: Falha ao escrever o ficheiro: {path}
  - [ ] OK    Fix: 

- **`fileNotFound`**
  - EN: File not found: {path}
  - PT: Ficheiro não encontrado: {path}
  - [ ] OK    Fix: 

- **`invalidConfigStructure`**
  - EN: Invalid configuration file structure
  - PT: Estrutura do ficheiro de configuração inválida
  - [ ] OK    Fix: 

- **`noInputDataInFile`**
  - EN: No input data found in file
  - PT: Não foram encontrados dados de entrada no ficheiro
  - [ ] OK    Fix: 

- **`noInputDataInSnapshot`**
  - EN: No input data in snapshot
  - PT: Sem dados de entrada no snapshot
  - [ ] OK    Fix: 

- **`noLFOPresetDataInFile`**
  - EN: No LFO preset data found in file
  - PT: Nenhum dado de preset LFO encontrado no ficheiro
  - [ ] OK    Fix: 

- **`noNetworkDataInFile`**
  - EN: No network data found in file
  - PT: Não foram encontrados dados de rede no ficheiro
  - [ ] OK    Fix: 

- **`noNetworkSections`**
  - EN: No network sections found in file
  - PT: Não foram encontradas secções de rede no ficheiro
  - [ ] OK    Fix: 

- **`noOutputDataInFile`**
  - EN: No output data found in file
  - PT: Não foram encontrados dados de saída no ficheiro
  - [ ] OK    Fix: 

- **`noProjectFolder`**
  - EN: No project folder specified
  - PT: Nenhuma pasta de projeto especificada
  - [ ] OK    Fix: 

- **`noReverbDataInFile`**
  - EN: No reverb data found in file
  - PT: Não foram encontrados dados de reverberação no ficheiro
  - [ ] OK    Fix: 

- **`noSystemDataInFile`**
  - EN: No valid system data found in file: {path}
  - PT: Não foram encontrados dados de sistema válidos no ficheiro: {path}
  - [ ] OK    Fix: 

- **`noValidProjectFolder`**
  - EN: No valid project folder
  - PT: Nenhuma pasta de projeto válida
  - [ ] OK    Fix: 

- **`prefixInputs`**
  - EN: Inputs: 
  - PT: Entradas: 
  - [ ] OK    Fix: 

- **`prefixNetwork`**
  - EN: Network: 
  - PT: Rede: 
  - [ ] OK    Fix: 

- **`prefixOutputs`**
  - EN: Outputs: 
  - PT: Saídas: 
  - [ ] OK    Fix: 

- **`prefixReverbs`**
  - EN: Reverbs: 
  - PT: Reverberações: 
  - [ ] OK    Fix: 

- **`prefixSystem`**
  - EN: System: 
  - PT: Sistema: 
  - [ ] OK    Fix: 

- **`snapshotNotFound`**
  - EN: Snapshot not found
  - PT: Snapshot não encontrado
  - [ ] OK    Fix: 

- **`snapshotNotFoundNamed`**
  - EN: Snapshot not found: {name}
  - PT: Snapshot não encontrado: {name}
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
  - PT: ADM-OSC é um protocolo que visa melhorar a interoperabilidade do som espacial. Envia posições cartesianas (X, Y, Z) ou valores polares (AED para Azimute, Elevação, Distância) do console ou das curvas de automação de uma DAW.
Os dados são enviados normalizados:
- entre -1.0 e 1.0 para X, Y e Z;
- entre 0.0 e 1.0 para distância,
- entre -180° e 180° para Azimute
- entre -90° e 90° para elevação.
O ponto de origem pode ser movido e o mapeamento pode ser ajustado em diferentes segmentos para as partes interna e externa do palco.
Ao arrastar as alças nos gráficos, segurar a tecla Shift aplica ajustes simétricos no lado oposto.
  - [ ] OK    Fix: 

- **`title`**
  - EN: ADM-OSC Mappings
  - PT: Mapeamentos ADM-OSC
  - [ ] OK    Fix: 

## `help.automOtion`

- **`body`**
  - EN: One time movement can also be programmed and triggered manually or triggered by sound level.
The coordinates are either relative from the start position or absolute relative to the origin point.
The input can either stay at the end position or revert to the starting position.
Input position cannot be changed while the input is moving this way, but interacting with the input will change the position offset of the input instead.
For audio level triggering select the sound level above which the movement will start. When the sound drops below the reset level the movement will be rearmed, ready to be triggered again. New movement automation will not be possible while the input is moving.
  - PT: Movimentos únicos podem ser programados e disparados manualmente ou por nível de som.
As coordenadas são relativas da posição inicial ou absolutas em relação ao ponto de origem.
A entrada pode ficar na posição final ou voltar à posição inicial.
A posição não pode ser alterada durante o movimento, mas a interação mudará o deslocamento de posição.
Para disparo por nível de áudio, selecione o limiar. Quando o som cair abaixo do nível de reinício, o movimento será rearmado.
  - [ ] OK    Fix: 

- **`title`**
  - EN: AutomOtion
  - PT: AutomOtion
  - [ ] OK    Fix: 

## `help.binaural`

- **`body`**
  - EN: The Binaural renderer is used for:
- listening to a rough spatial mix on headphones,
- creating a mix for stereo output,
- listening to a single soloed track through the spatial processing.
This may take the place of your master mix if it's only feeding headphones and media mix.
The position of the listening position may be adjusted in depth from the origin point and in orientation. Delay and level settings allow you to eventually match the sound at the FOH position.
  - PT: O renderizador binaural é usado para:
- ouvir uma mixagem espacial aproximada em fones de ouvido,
- criar uma mixagem para saída estéreo,
- ouvir uma faixa solo através do processamento espacial.
Pode substituir sua mixagem master se alimentar apenas fones e mixagem de mídia.
A posição de escuta pode ser ajustada em profundidade a partir do ponto de origem e em orientação. As configurações de atraso e nível permitem igualar o som na posição FOH.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Binaural Renderer
  - PT: Renderizador Binaural
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
  - PT: Clusters são grupos de entradas que podem ser manipulados e animados como um conjunto.
Cada entrada só pode pertencer a um cluster.
Cada cluster pode ter apenas uma entrada com tracking totalmente habilitado, que se torna o ponto de referência.
Se não houver entrada com tracking, há dois modos de referência: a primeira entrada atribuída ou o baricentro das entradas.
Todas as entradas podem ser movidas arrastando o ponto de referência. Entradas individuais podem ser ajustadas separadamente. Arrastar uma entrada com tracking ativado que também é ponto de referência afetará seu deslocamento de posição e a posição das outras entradas do cluster normalmente.
Todas as entradas de um cluster podem ser rotacionadas ou escaladas ao redor do ponto de referência.
Todos os clusters podem receber animação via LFO. Posições X, Y, Z, rotação e escala podem ser controladas. Configurações LFO podem ser atribuídas a pads. Um clique direito armazenará os parâmetros LFO em um pad. Duplo clique no topo do pad permite editar o nome do preset. Um clique ou toque recupera as configurações esteja o LFO rodando ou não, mas não o iniciará. Um duplo clique/toque carregará e iniciará o LFO.
Todos os clusters compartilham o mesmo conjunto de presets LFO.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Clusters
  - PT: Clusters
  - [ ] OK    Fix: 

## `help.floorReflections`

- **`body`**
  - EN: Simulating the floor reflections improves the naturalness of the sound. We don't expect sounds to play in a soundproof anechoic chamber. This setting helps recreate the expected floor reflections.
The level of the floor reflections can be adjusted as well as the low cut and the high frequency shelf filters. Diffusion adds a bit of chaos to simulate the unevenness of the floor.
  - PT: Simular as reflexões do chão melhora a naturalidade do som. Não esperamos que sons sejam reproduzidos em uma câmara anecoica à prova de som. Esta configuração ajuda a recriar as reflexões do chão esperadas.
O nível das reflexões do chão pode ser ajustado, assim como os filtros de corte baixo e shelf de altas frequências. A difusão adiciona um pouco de caos para simular as irregularidades do chão.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Floor Reflections
  - PT: Reflexões do Chão
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
  - PT: Os mapas de gradiente permitem controlar atenuação, altura e filtragem de alta frequência (shelf com inclinação suave centrada em 1kHz) dependendo da posição X, Y. Por exemplo, você pode atenuar um som ao entrar em uma zona, aplicar roll-off de altas frequências ao se afastar da frente do palco ou ajustar automaticamente a altura de um ator em plataformas elevadas.
Há três camadas: atenuação, altura e shelf HF. Podem ser ativadas/desativadas e ocultadas.
Cada camada tem controles de mapeamento branco e preto para ajustar o alcance do efeito. A configuração de curva ajusta a transição.
Cada camada pode ter formas editáveis (retângulo, elipse ou polígono) com cinza uniforme, gradiente linear ou radial.
Ao criar um polígono, clique para cada vértice. Duplo clique fecha a forma.
Duplo clique em um ponto o remove. Duplo clique em um lado adiciona um ponto.
Formas e camadas podem ser copiadas.
As configurações são armazenadas nos arquivos de entrada.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Gradient Maps
  - PT: Mapas de Gradiente
  - [ ] OK    Fix: 

## `help.inputAdvanced`

- **`body`**
  - EN: - Sidelines and Fringe enable muting when an input comes close to the limits of a rectangular stage (except downstage).
- Tracking can be enabled and the ID of the tracker can be selected. The amount of smoothing on the position can also be adjusted.
- Maximum Speed can be engaged and the speed limit adjusted. The system will apply a gradual acceleration and deceleration when the movement starts and finishes. When Path is enabled, the system will track the path followed by the input and will not go in a straight line to the final position. This is especially handy if movements have to be operated manually.
- Height Factor lets you work in 2D, when set to 0%, or full 3D, when set at 100%, and anything in between. It's the ratio of the height in the level and delay calculations. If you wish to use floor reflections, set it to 100% and use the parallax correction of the output parameters.
  - PT: - As Linhas laterais e a Zona de transição permitem o silenciamento quando uma entrada se aproxima das margens de um palco retangular (exceto a margem da frente, lado do público).
- O Tracking pode ser ativado e o ID do tracker selecionado. A suavização da posição também pode ser ajustada.
- A Velocidade Máxima pode ser ativada e o limite de velocidade ajustado. O sistema aplicará uma aceleração e desaceleração graduais no início e no fim do movimento. Quando o modo Trajetória está ativo, o sistema seguirá o caminho percorrido pela entrada e não irá em linha reta para a posição final. É particularmente útil se os movimentos forem operados manualmente.
- O Fator de Altura permite trabalhar em 2D, quando definido a 0%, ou em 3D completo, quando definido a 100%, e tudo no meio. É a proporção da altura nos cálculos de nível e atraso. Se quiser usar reflexões de chão, defina-o a 100% e use a correção de paralaxe nos parâmetros de saída.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Advanced Controls
  - PT: Controles avançados
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
  - PT: As entradas dispõem de uma grande variedade de configurações para se adaptar a diferentes situações que requerem reforço sonoro realista ou ferramentas criativas para design sonoro.
- O nível de entrada pode ser ajustado.
- As entradas podem ser atrasadas ou tentar considerar uma latência específica (processamento digital de transmissão sem fio ou efeitos digitais) e compensá-la para alinhar melhor a amplificação e o som acústico.
- A Latência Mínima pode ser ativada em vez da Precedência Acústica. Isso tenta enviar o som através do sistema o mais rápido possível. O sistema analisa os envios desta entrada para as saídas buscando o menor atraso e o subtrai de todos os atrasos, contornando o efeito Haas adicional.
- A posição (posição e offset) pode ser dada em coordenadas Cartesianas, Cilíndricas ou Esféricas independentemente da forma do palco ou outros canais.
- A posição pode ser restringida às dimensões do palco em coordenadas Cartesianas ou a uma faixa de distância específica em coordenadas polares.
- Flip tomará a posição simétrica para a coordenada dada ao redor do ponto de origem.
- O joystick e o controle deslizante vertical permitem controle relativo da posição.
- As entradas podem ser atribuídas a um cluster para agrupá-las em movimentos coordenados.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Inputs Basic Parameters
  - PT: Parâmetros básicos de entradas
  - [ ] OK    Fix: 

## `help.inputHF`

- **`body`**
  - EN: When talking away, the timbre of a voice will sound less bright. Reproducing this was the initial goal here even though we usually want to have support for voices when not addressing the audience or in bi-frontal configurations. This can be put to good use for creative effects such as having a brighter reverb on a dampened direct sound.
The orientation of the input in azimuth and in pitch can be set as well as the angle where the high frequencies will not be filtered.
The HF Shelf will set the maximum attenuation in the back of the input. There is a smooth fade (like a cosine curve) from full brightness in front to damped at the rear.
  - PT: Ao falar virando-se, o timbre de uma voz soa menos brilhante. Reproduzir isso era o objetivo inicial aqui, embora geralmente queiramos ter suporte para vozes quando não se dirigem ao público ou em configurações bi-frontais. Isso pode ser usado para efeitos criativos como ter uma reverberação mais brilhante sobre um som direto atenuado.
A orientação da entrada em azimute e em elevação pode ser definida assim como o ângulo onde as altas frequências não serão filtradas.
O HF Shelf definirá a atenuação máxima na parte traseira da entrada. Há um fade suave (como uma curva cosseno) do brilho total na frente à atenuação atrás.
  - [ ] OK    Fix: 

- **`title`**
  - EN: High Frequency Directivity
  - PT: Diretividade de alta frequência
  - [ ] OK    Fix: 

## `help.inputLevel`

- **`body`**
  - EN: There are two level attenuation models. One where the level decreases with the distance by a given ratio in dB/m. Otherwise the level is halved each time the distance doubles. This might be more realistic, but it may be too loud close to the source or not give enough focus. The former may be less physically accurate, it offers usually better control to have a more even and stable mix.
  - PT: Existem dois modelos de atenuação de nível. Um onde o nível diminui com a distância por uma proporção dada em dB/m. Alternativamente o nível é reduzido pela metade cada vez que a distância dobra. Este último pode ser mais realista, mas pode ser muito alto perto da fonte ou não dar foco suficiente. O primeiro pode ser menos preciso fisicamente, mas geralmente oferece melhor controle para uma mixagem mais uniforme e estável.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Level Adjustments
  - PT: Ajustes de nível
  - [ ] OK    Fix: 

## `help.inputMutes`

- **`body`**
  - EN: You can set for each output array a specific attenuation for the selected input.
You can mute each send to any output individually. There are macros to speed up the process.
  - PT: Você pode definir para cada array de saída uma atenuação específica para a entrada selecionada.
Você pode mutar cada envio para qualquer saída individualmente. Há macros disponíveis para acelerar o processo.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Array Attenuation and Output Mutes
  - PT: Atenuação por array e mutes de saída
  - [ ] OK    Fix: 

## `help.lfo`

- **`body`**
  - EN: Input position can be automated. The LFO can control X, Y and Z position coordinates individually as well as the rotation of HF directivity (gyrophone).
Adjust the global period and phase for the LFO.
For X, Y and Z coordinates select a shape, amplitude, rate and phase as desired. A circle in the XY plane would have sine for shape for X and for Y and an offset of ±90° between the two. A square would be the same but with keystone shapes.
Input position can be moved while the LFO is running.
  - PT: A posição de entrada pode ser automatizada. O LFO pode controlar as coordenadas X, Y e Z individualmente e a rotação da diretividade HF (girofone).
Ajuste o período e a fase globais do LFO.
Para X, Y e Z selecione forma, amplitude, taxa e fase. Um círculo no plano XY usaria forma senoidal para X e Y com ±90° de defasagem.
A posição pode ser movida enquanto o LFO está funcionando.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Input LFO
  - PT: LFO de Entrada
  - [ ] OK    Fix: 

## `help.liveSource`

- **`body`**
  - EN: A loud source on stage may not need much reinforcement through the nearby speakers. Imagine an opera singer near the edge of the stage. Normally the level distribution would make the level louder near the input position. But if it is already loud enough we should be able to not over-amplify it. This function manages this.
The radius and shape describe how to attenuate the level for a specific source for speakers within the radius of influence of this input. There are various shapes: a V shaped linear effect; a U shape for fast decrease within the radius; a tight V effect or a mix of the previous (sine).
The attenuation can be constant or level dependent like a local compression reacting to transients and to average RMS level.
  - PT: Uma fonte potente no palco pode não precisar de reforço através dos alto-falantes próximos. Imagine um cantor de ópera perto da borda do palco. Normalmente a distribuição de nível tornaria o som mais alto perto da posição de entrada. Mas se já estiver alto o suficiente, não devemos sobre-amplificar. Esta função gerencia isso.
O raio e a forma descrevem como atenuar o nível para os alto-falantes dentro do raio de influência desta fonte. Há várias formas: um efeito linear em V; um U para diminuição rápida; um V apertado ou uma mistura dos anteriores (seno).
A atenuação pode ser constante ou dependente de nível, como uma compressão local que reage a transientes e ao nível RMS médio.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Live Source Tamer
  - PT: Atenuador de Fonte Ao Vivo
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
  - PT: - Clique esquerdo em uma entrada ou cluster para movê-lo arrastando.
- Clique esquerdo com Shift adiciona ou remove entradas da seleção.
- Clique esquerdo arrastado desenha um retângulo de seleção.
- Duplo clique redefine o deslocamento de posição.
- Clique longo sem movimento alterna para a aba da entrada selecionada.
- Clique fora de qualquer entrada limpa a seleção.
- Clique direito arrastado desloca a vista do mapa. Arrasto com dois dedos também.
- A roda do mouse faz zoom. Pinça com dois dedos também.
- Clique do botão do meio redefine a vista.
- Setas movem X/Y, PageUp/Down a altura.
- Um segundo dedo pode rotacionar a diretividade e ajustar a altura.
- Em clusters, um segundo dedo pode rotacionar e escalar.
- Entradas, arrays de saída e nós de reverberação podem ser ocultados.
- Entradas podem ser bloqueadas.
- Nós de reverberação podem ser movidos. Ctrl/Cmd move pares em simetria.
- O raio do Live Source Tamer é exibido quando ativado.
- Os níveis de áudio podem ser exibidos no mapa.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Map
  - PT: Mapa
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
  - PT: O servidor MCP permite a um assistente de IA (Claude Desktop, Claude Code, ChatGPT com conetores personalizados) ler e escrever os parâmetros desta sessão WFS-DIY através de uma ligação de rede local.

O que a IA pode fazer:
• Ler o estado em direto: número de canais, nomes, posições, atenuações, EQ, snapshots, clusters, toda a superfície de parâmetros.
• Mover fontes, renomear canais, definir atribuições de clusters, ajustar a disposição de arrays, posicionar saídas e reverberações.
• Executar fluxos guiados (assistentes de afinação do sistema, resolução de problemas de localização, gestão de snapshots) através de modelos de prompt preparados.

Controlos do operador nesta linha:
• IA: ON / OFF — interruptor principal. Quando OFF, cada chamada IA é recusada; quando ON, a IA opera segundo as regras abaixo.
• Ações IA críticas: bloqueadas / PERMITIDAS — as ações destrutivas (eliminar snapshots, repor DSP, alterar o número de canais) estão bloqueadas por defeito. Clicar para as permitir durante 10 minutos; o preenchimento vermelho esvazia ao expirar a janela, depois bloqueia automaticamente.
• Abrir Histórico IA — cronologia rolável de cada alteração recente da IA com anular/refazer por linha.
• O botão URL MCP copia o URL do servidor para a área de transferência para clientes IA que aceitem um URL diretamente.

Vigilância do operador:
• Cada ação IA é registada com etiquetas de origem. A janela Histórico IA mostra a cronologia completa; o × por linha reverte uma ação com as suas dependências.
• Se ajustar manualmente um parâmetro que a IA acabou de mover, a IA é notificada e não retentará às cegas. Tem sempre a última palavra.
• Os atalhos Cmd/Ctrl+Alt+Z e Cmd/Ctrl+Alt+Y anulam e refazem a última alteração da IA sem afetar as suas edições manuais (que usam Ctrl+Z normal).

Para adicionar este servidor ao Claude Desktop:
  1. Abrir Definições → Programador → Editar configuração.
  2. Colar o trecho JSON abaixo em claude_desktop_config.json (fundir no bloco mcpServers existente, se já tiver um).
  3. Reiniciar o Claude Desktop. O servidor aparece como 'wfs-diy' no menu de ferramentas.

Para adicionar ao Claude Code, executar:
  claude mcp add wfs-diy <url> -t http

O URL muda se mudar de interface de rede ou se o servidor recorrer a uma porta diferente. O botão URL nesta linha reflete sempre o URL em direto.
  - [ ] OK    Fix: 

- **`copyButton`**
  - EN: Copy Config
  - PT: Copiar configuração
  - [ ] OK    Fix: 

- **`copyConfirm`**
  - EN: MCP config JSON copied to clipboard
  - PT: Configuração MCP JSON copiada para a área de transferência
  - [ ] OK    Fix: 

- **`title`**
  - EN: AI / MCP Server
  - PT: Servidor IA / MCP
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
  - PT: O sistema pode comunicar através de vários protocolos de rede (UDP ou TCP) usando OSC. OSC Query pode ser habilitado para permitir que os clientes descubram os caminhos OSC possíveis e se inscrevam em atualizações de parâmetros.
O IP da máquina local correspondente à interface de rede selecionada é mostrado. As portas TCP e UDP de entrada e a porta OSC Query.
Existem alguns clientes OSC especializados como:
- Remote para a aplicação Android para operação multitoque e controle remoto.
- QLab que pode enviar dados e ser programado diretamente pela aplicação.
- ADM-OSC para controle de consoles e DAW (ver ajuda específica).
Os dados podem ser filtrados. Uma janela de Log mostra os dados de entrada e saída.
Há também uma função de localização para encontrar tablets Android perdidos.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Network
  - PT: Rede
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
  - PT: Existem alguns parâmetros para ajudá-lo a se ajustar ao som acústico.
A maioria destes parâmetros é definida para arrays inteiros a menos que o modo de propagação esteja desativado para esta saída. Mudança relativa também pode ser selecionada após uma configuração específica.
- Orientação e Ângulos On/Off definem quais entradas cada alto-falante amplificará. Por padrão os alto-falantes apontam para o público, de costas para o palco. Entradas no setor verde serão amplificadas, mas não as que estão na frente do alto-falante, no setor vermelho. Há um fade entre os dois setores. Para sub-graves, abrir completamente ao máximo permitirá incluir todas as entradas.
- A Atenuação HF simula a perda de altas frequências com a distância.
- A porcentagem de Atenuação por Distância define quanta da atenuação calculada é aplicada. Para sub-graves pode ser prudente reduzir para 50%.
- A Latência Mínima ativa a varredura do menor atraso calculado.
- A Atenuação Live Source ativa a redução de nível de entradas próximas.
- As Reflexões de Piso ativam se as reflexões são aplicadas ao sinal para esta saída como sub-graves e arrays suspensos...
  - [ ] OK    Fix: 

- **`title`**
  - EN: Advanced Parameters
  - PT: Parâmetros avançados
  - [ ] OK    Fix: 

## `help.outputs`

- **`body`**
  - EN: The design of the WFS system has to do with the right choice of equipment and their position. Here is a guide to help you with the design and the tuning of your arrays.
An array is a line (straight or curved) of speakers. This is one of the most important concepts in WFS adapted to sound reinforcement and for creative sound design.
A rule of thumb would be that each listener should hear three speakers of an array to have enough psycho-acoustic cues to feel the direction each sound is coming from. There will be a sweet spot to find between the distance between the speakers and the listeners, their spacing and coverage angle. This is especially true for short range arrays, a.k.a front fills. Speakers with a 120° coverage angle can be spaced by the same distance between the array and the first row of listeners. The number of speakers also depends on the sound pressure level. When placed as a flown array, trapezoidal/asymmetric horns with wide coverage angle (120°) below axis and narrow coverage (60°) on axis will give good coverage for seat rows a bit too far from the ground array and will have enough throw to reach 20m or 30m away while avoiding the walls of the venue where they would create reflections that give away the position of the speakers to the ears of the listeners. Most of the time for larger venues coaxial (elliptical or conical horns) have not enough reach and require one or more delay lines. They are more suited for smaller venues with few rows of seats.
The positioning of the speakers in the system can be done through the 'Wizard of OutZ' and the editable presets it has.
  - PT: O design do sistema WFS tem a ver com a escolha certa de equipamentos e seu posicionamento. Aqui está um guia para ajudá-lo com o design e afinação dos seus arrays.
Um array é uma linha (reta ou curva) de alto-falantes. Este é um dos conceitos mais importantes em WFS adaptado ao reforço sonoro e design sonoro criativo.
Como regra geral, cada ouvinte deve ouvir três alto-falantes de um array para ter pistas psicoacústicas suficientes para sentir a direção de cada som. Haverá um ponto ótimo entre a distância entre os alto-falantes e os ouvintes, seu espaçamento e ângulo de cobertura. Alto-falantes com ângulo de cobertura de 120° podem ser espaçados pela mesma distância entre o array e a primeira fila. O número também depende do nível de pressão sonora. Como array suspenso, cornetas trapezoidais/assimétricas com ângulo amplo (120°) abaixo do eixo e estreito (60°) no eixo darão boa cobertura e alcance de 20-30m evitando reflexões nas paredes. Alto-falantes coaxiais geralmente não têm alcance suficiente para grandes locais e requerem linhas de atraso.
O posicionamento pode ser feito através do 'Wizard of OutZ' e seus presets editáveis.
  - [ ] OK    Fix: 

- **`title`**
  - EN: WFS Array Design
  - PT: Design de arrays WFS
  - [ ] OK    Fix: 

## `help.overview`

- **`body`**
  - EN: This WFS spatial sound processor aims to be a tool for natural sound reinforcement and also a creative tool opening new avenues for writing sound in space.
Some parameters are straightforward: place sound (Map, Tracking, Speed Limiting, Gradient Maps...), work its shape (Attenuation Profile) and its acoustic presence (Directivity, Floor Reflections), give it a one-off motion (AutomOtion) or a repetitive movement (L.F.O). In some cases amplification should be limited around loud sources on stage (Live Source Tamer). All of these functionalities can be stored and recalled internally or with the help of QLab. On the other side the system allows real-time interaction to trigger and move samples, move large clusters of inputs manually or thanks to easily recallable LFO presets.
  - PT: Este processador espacial WFS pretende ser uma ferramenta para reforço sonoro natural e também uma ferramenta criativa abrindo novos caminhos para escrever som no espaço.
Alguns parâmetros são diretos: posicionar som (Mapa, Tracking, Limitação de velocidade, Mapas de gradiente...), trabalhar sua forma (Perfil de atenuação) e sua presença acústica (Diretividade, Reflexões do piso), dar-lhe um movimento pontual (AutomOtion) ou repetitivo (L.F.O). Em alguns casos a amplificação deve ser limitada ao redor de fontes potentes no palco (Live Source Tamer). Todas essas funcionalidades podem ser armazenadas e recuperadas internamente ou com a ajuda do QLab. Por outro lado o sistema permite interação em tempo real para disparar e mover amostras, mover grandes clusters de entradas manualmente ou graças a presets LFO facilmente recuperáveis.
  - [ ] OK    Fix: 

- **`dontShow`**
  - EN: Don't show this again
  - PT: Não mostrar novamente
  - [ ] OK    Fix: 

- **`title`**
  - EN: System Overview
  - PT: Visão geral do sistema
  - [ ] OK    Fix: 

## `help.parallax`

- **`body`**
  - EN: Each speaker points more or less clearly at a listener. To calculate the delay for an input for each speaker, let's consider the distance from the input to this listener, we can also calculate the distance the sound from the speaker to this listener. To match the time of arrival of both we need to apply the difference in the aforementioned distances as a delay for this input and this speaker. This gives greater stability when inputs are moved on stage and especially when they move upstage, away from the edge of the stage. This also can enable the synthesis of floor reflections. This setting can be tuned for finer adjustments, rather than simply measured. Use your ears!
  - PT: Cada alto-falante aponta mais ou menos claramente para um ouvinte. Para calcular o atraso para uma entrada para cada alto-falante, consideramos a distância da entrada a este ouvinte, também podemos calcular a distância do som do alto-falante a este ouvinte. Para igualar o tempo de chegada de ambos devemos aplicar a diferença das distâncias mencionadas como atraso. Isso dá maior estabilidade quando as entradas são movidas no palco e especialmente quando se afastam da borda. Isso também pode permitir a síntese de reflexões de piso. Esta configuração pode ser ajustada finamente, em vez de simplesmente medida. Confie nos seus ouvidos!
  - [ ] OK    Fix: 

- **`title`**
  - EN: Parallax Correction
  - PT: Correção de paralaxe
  - [ ] OK    Fix: 

## `help.reverb`

- **`body`**
  - EN: Reverb helps blur the actual reflections of the speakers in the acoustic space. Without some reverb the listener might perceive the sound missing some depth because the reflections of the speakers in the acoustic of the room gives away their positions and all sounds might feel as coming from the plane of the speakers.
Place the reverb node according to the number of channels and the geometry of the stage and listening area, and the positions of speakers. If necessary the return position can be offset from the feed position.
Other parameters are very similar to Outputs' and Inputs'.
  - PT: A reverberação ajuda a difundir as reflexões dos alto-falantes.
Posicione os nós de acordo com a geometria do palco.
Outros parâmetros são semelhantes aos de Saídas e Entradas.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb
  - PT: Reverberação
  - [ ] OK    Fix: 

## `help.reverbAlgo`

- **`body`**
  - EN: There are three built-in reverb types inside this spatial sound processor:
- SDN (Scattered Delay Network): The sound bounces between every reverb node which act as reflecting surfaces. This reverb algorithm favours an odd number of nodes that should not have too much symmetry. This helps reduce the artifacts of the reverb or any metallic sounding resonance.
- FDN (Feedback Delay Network): Each reverb node functions as a separate reverb processor with a classical reverb algorithm. Place nodes around the stage and eventually around the audience. Inputs are sent to each node in a similar fashion as with outputs. The return signal from the reverb is then distributed like other inputs to all outputs.
- IR (Impulse Response): This is a classical convolution reverb. You can load audio samples as impulse responses. Each reverb node can share the same IR or they can have different ones. Place nodes around the stage and eventually around the audience.
The node positions can be adjusted directly on the map. The Ctrl/Cmd key will move a reverb node pair in symmetry.
  - PT: Três tipos de reverberação estão integrados neste processador de som espacial:
- SDN (Scattered Delay Network): O som reflete entre cada nó de reverberação que atua como superfícies refletoras. Este algoritmo favorece um número ímpar de nós sem muita simetria, para reduzir artefatos ou ressonâncias metálicas.
- FDN (Feedback Delay Network): Cada nó funciona como um processador de reverberação separado com um algoritmo clássico. Posicione nós ao redor do palco e eventualmente ao redor do público.
- IR (Resposta ao Impulso): Reverberação por convolução clássica. Você pode carregar amostras de áudio como respostas ao impulso. Cada nó pode compartilhar a mesma IR ou usar diferentes.
As posições dos nós podem ser ajustadas diretamente no mapa. A tecla Ctrl/Cmd move um par de nós em simetria.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Algorithms
  - PT: Algoritmos de Reverberação
  - [ ] OK    Fix: 

## `help.reverbFeed`

- **`body`**
  - EN: Pre-processing sending of the input channels to the reverb nodes.
- Orientation and On/Off Angles define what inputs each reverb node will receive. Inputs in the green sector will be picked-up, but not the ones in front in the red sector. There is a fade between both sectors.
- HF Damping simulates the loss of high frequency with distance.
- Distance Attenuation percentage allows you to define if more or less of the attenuation calculated from the distance and parameters of the input is applied.
- Minimal Latency toggles whether the output is scanned for the smallest calculated delay and also if the setting once engaged on an input will reduce the delay for that input through this reverb node.
- Live Source Attenuation toggles the reduction of level of nearby input if they have this setting enabled.
  - PT: Envio de pré-processamento dos canais de entrada para os nós.
- Orientação e ângulos On/Off definem quais entradas cada nó recebe.
- Amortecimento HF simula a perda de alta frequência.
- Percentagem de atenuação de distância define a atenuação aplicada.
- Latência mínima determina se o menor atraso é usado.
- Atenuação de fonte ao vivo reduz o nível de entradas próximas.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Feed
  - PT: Alimentação de Reverberação
  - [ ] OK    Fix: 

## `help.reverbPostProc`

- **`body`**
  - EN: Includes a 4-band EQ and an Expander that looks at the signal entering the reverb processor to reduce long reverb tails when the inputs are quiet.
  - PT: Inclui um EQ de 4 bandas e um Expansor que observa o sinal entrando no processador de reverberação para reduzir caudas longas de reverberação quando as entradas estão em silêncio.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Post-Processing
  - PT: Pós-Processamento de Reverberação
  - [ ] OK    Fix: 

## `help.reverbPreProc`

- **`body`**
  - EN: Includes a 4-band EQ and a Compressor to remove transients which might excite the reverb processor a bit too much.
  - PT: Inclui um EQ de 4 bandas e um Compressor para remover transientes que poderiam excitar demais o processador de reverberação.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Pre-Processing
  - PT: Pré-Processamento de Reverberação
  - [ ] OK    Fix: 

## `help.reverbReturn`

- **`body`**
  - EN: Post-processing sending to the speakers.
- Distance Attenuation defines the level drop per meter to the speakers.
- Common Attenuation will look for the lowest attenuation and keep only a percentage of this level drop and apply this across all other outputs.
- Mutes and Mute Macros allow to prevent a reverb channel from feeding an output. For instance it may not be necessary to send reverb to sub-bass speakers.
  - PT: Pós-processamento enviado aos alto-falantes.
- Atenuação de Distância define a queda de nível por metro.
- Atenuação Comum mantém uma percentagem da menor atenuação.
- Mutes impedem um canal de reverberação de alimentar uma saída.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Return
  - PT: Retorno de Reverberação
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
  - PT: O sampler permite disparar amostras e interagir com elas em tempo real.
Quando habilitado em uma faixa, o sampler substitui a entrada ao vivo em todos os momentos.
Vários samplers podem ser atribuídos a diferentes entradas e disparados individualmente.
Para usar o sampler:
- Selecione um Roli Lightpad ou um pad no app Android Remote conectado.
- Adicione amostras aos diferentes blocos na grade. Ajuste a posição inicial relativa, o nível e os pontos de entrada e saída. Várias amostras podem ser selecionadas segurando Shift enquanto clica.
- Crie conjuntos de amostras: as amostras selecionadas serão adicionadas a novos conjuntos. Podem ser adicionadas ou removidas segurando Ctrl/Cmd enquanto clica nos blocos. Cada conjunto pode ser renomeado e ter uma sequência fixa ou aleatória. Cada conjunto tem uma configuração de atenuação e uma posição base.
- Pressione um Lightpad ou pad para disparar uma amostra. A pressão pode ser mapeada para nível, altura e filtragem de altas frequências. O movimento do dedo move o som como um joystick.
Soltar o pad para a amostra.
As configurações do sampler são armazenadas nos arquivos de entrada.
Blocos e conjuntos podem ser copiados, exportados e importados.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Sampler
  - PT: Sampler
  - [ ] OK    Fix: 

## `help.sessionData`

- **`body`**
  - EN: When starting a session select the working folder where the system will place the files and eventual audio files. For new projects create a new folder. When reloading a previous session navigate to the corresponding folder.
Each section has a separate xml file (System configuration, Network, Outputs, Reverbs, Inputs) and backups. Additionally convolution reverb impulse responses and audio samples will be stored in sub-directories.
Each section can be stored and recalled individually or as a whole.
Each section can also export and import files from other projects.
  - PT: Ao iniciar uma sessão, selecione a pasta de trabalho onde o sistema colocará os arquivos e eventuais arquivos de áudio. Para novos projetos, crie uma nova pasta. Para recarregar uma sessão anterior, navegue até a pasta correspondente.
Cada seção tem um arquivo xml separado (Configuração do sistema, Rede, Saídas, Reverberação, Entradas) e backups. Respostas ao impulso de reverberação por convolução e amostras de áudio serão armazenadas em subdiretórios.
Cada seção pode ser armazenada e recuperada individualmente ou como um todo.
Cada seção também pode exportar e importar arquivos de outros projetos.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Session Data
  - PT: Dados da Sessão
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
  - PT: Snapshots armazenam parâmetros de entrada, mas podem ter um escopo para serem recuperados durante uma apresentação.
O Escopo indica ao sistema quais dados armazenar ou recuperar.
Vários métodos estão disponíveis:
- Gravar apenas os dados necessários em arquivos locais. O filtro é aplicado ao salvar.
- Gravar todos os dados e um filtro em arquivos locais. O filtro é aplicado ao recuperar.
- Gravar todos os dados em cues do QLab. Não recomendado para configurações grandes.
O escopo pode mostrar e pré-selecionar automaticamente os parâmetros modificados manualmente. Alterações são marcadas em amarelo.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Input Snapshots and Scope
  - PT: Snapshots de Entrada e Escopo
  - [ ] OK    Fix: 

## `help.tracking`

- **`body`**
  - EN: Tracking allows to follow the position in 2D or 3D of actors and musicians. There are several tracking solutions based on UWB (UltraWide Band) tags, 3D camera, computer vision systems and Infra-Red LED's followed by IR sensitive cameras.
This application allows you to receive tracking data from several protocols: OSC, MQTT, PosiStageNet/PSN, RTTrP.
You can select which protocol you're using and enter its settings. You may also adjust the mapping (offset, scaling and orientation).
Each input has a toggle to enable tracking, an ID to select which tracking marker it should follow and a smoothing algorithm to reduce jittery movements.
  - PT: O tracking permite seguir a posição em 2D ou 3D de atores e músicos. Existem várias soluções baseadas em tags UWB, câmeras 3D, sistemas de visão computacional e LEDs infravermelhos com câmeras sensíveis a IR.
Esta aplicação permite receber dados de tracking de vários protocolos: OSC, MQTT, PosiStageNet/PSN, RTTrP.
Você pode selecionar o protocolo utilizado e configurar suas definições. O mapeamento (offset, escala e orientação) também pode ser ajustado.
Cada entrada tem um toggle para ativar o tracking, um ID para selecionar o marcador e um algoritmo de suavização.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Tracking
  - PT: Tracking
  - [ ] OK    Fix: 

## `help.tuning`

- **`body`**
  - EN: System tuning for WFS is different from standard PA tuning. It can proceed as follows:
- Start with the flown array muted. Set the desired levels for the near-field speakers when listening to them in the first row. Adjust the high frequency shelf attenuation so the near-field speakers are not too bright and draw too much attention.
- Mute the near-field array and unmute the flown array, find a suitable level towards the back of the house.
- Unmute both arrays, adjust the delay of the flown array to bring the sound down to the correct height in the lower rows. Adjust the levels, HF shelf/distance ratio and vertical and horizontal parallax for each array to have consistent level wherever your inputs are on stage. This is especially important if your inputs are actors, singers or acoustic music instruments on stage. You may also test adding some Haas effect to delay the whole system if you feel the WFS is too early or adding comb filtering with the acoustic sound.
You may follow a different workflow for tuning or go for different cues.
  - PT: A afinação do sistema WFS é diferente da afinação PA padrão. Pode proceder assim:
- Comece com o array suspenso mutado. Defina os níveis desejados para os alto-falantes de campo próximo ouvindo-os na primeira fila. Ajuste a atenuação do shelf de alta frequência para que não sejam muito brilhantes.
- Mute o array de campo próximo e ative o array suspenso, encontre um nível adequado em direção ao fundo da sala.
- Ative ambos os arrays, ajuste o atraso do array suspenso para trazer o som à altura correta nas filas inferiores. Ajuste níveis, shelf HF/proporção de distância e paralaxe vertical e horizontal para cada array para um nível consistente onde quer que estejam suas entradas no palco.
Você pode seguir um fluxo de trabalho diferente para afinação ou buscar configurações diferentes para diferentes situações.
  - [ ] OK    Fix: 

- **`title`**
  - EN: System Tuning
  - PT: Afinação do sistema
  - [ ] OK    Fix: 

## `inputs`

- **`arrayPrefix`**
  - EN: Array
  - PT: Array
  - [ ] OK    Fix: 

## `inputs.buttons`

- **`deleteSnapshot`**
  - EN: Delete Snapshot
  - PT: Eliminar Snapshot
  - [ ] OK    Fix: 

- **`editScope`**
  - EN: Edit Scope
  - PT: Editar Âmbito
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - PT: Exportar
  - [ ] OK    Fix: 

- **`hiddenOnMap`**
  - EN: Input Hidden on Map
  - PT: Entrada Oculta no Mapa
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - PT: Importar
  - [ ] OK    Fix: 

- **`lockOnMap`**
  - EN: Lock on Map
  - PT: Bloquear no Mapa
  - [ ] OK    Fix: 

- **`pauseAll`**
  - EN: Pause All
  - PT: Pausar Tudo
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Backup
  - PT: Recarregar Backup
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Input Config
  - PT: Recarregar Config. Entrada
  - [ ] OK    Fix: 

- **`reloadSnapshot`**
  - EN: Reload Snapshot
  - PT: Recarregar Snapshot
  - [ ] OK    Fix: 

- **`reloadWithoutScope`**
  - EN: Reload Without Scope
  - PT: Recarregar sem filtro
  - [ ] OK    Fix: 

- **`resumeAll`**
  - EN: Resume All
  - PT: Retomar Tudo
  - [ ] OK    Fix: 

- **`samplerOff`**
  - EN: Sampler: OFF
  - PT: Sampler: OFF
  - [ ] OK    Fix: 

- **`samplerOn`**
  - EN: Sampler: ON
  - PT: Sampler: ON
  - [ ] OK    Fix: 

- **`setAllInputs`**
  - EN: Set all Inputs...
  - PT: Definir todas as Entradas...
  - [ ] OK    Fix: 

- **`solo`**
  - EN: Solo
  - PT: Solo
  - [ ] OK    Fix: 

- **`soloModeMulti`**
  - EN: Multi
  - PT: Multi
  - [ ] OK    Fix: 

- **`soloModeSingle`**
  - EN: Single
  - PT: Single
  - [ ] OK    Fix: 

- **`stopAll`**
  - EN: Stop All
  - PT: Parar Tudo
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Input Config
  - PT: Guardar Config. Entrada
  - [ ] OK    Fix: 

- **`storeSnapshot`**
  - EN: Store Snapshot
  - PT: Guardar Snapshot
  - [ ] OK    Fix: 

- **`updateSnapshot`**
  - EN: Update Snapshot
  - PT: Atualizar Snapshot
  - [ ] OK    Fix: 

- **`visibleOnMap`**
  - EN: Input Visible on Map
  - PT: Entrada Visível no Mapa
  - [ ] OK    Fix: 

## `inputs.clusters`

- **`clusterPrefix`**
  - EN: Cluster
  - PT: Cluster
  - [ ] OK    Fix: 

- **`single`**
  - EN: Single
  - PT: Individual
  - [ ] OK    Fix: 

## `inputs.coordinates`

- **`xyz`**
  - EN: XYZ
  - PT: XYZ
  - [ ] OK    Fix: 

## `inputs.dialogs`

- **`exportConfig`**
  - EN: Export Input Configuration
  - PT: Exportar Configuração de Entrada
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Input Configuration
  - PT: Importar Configuração de Entrada
  - [ ] OK    Fix: 

- **`selectChannel`**
  - EN: Select Channel
  - PT: Selecionar canal
  - [ ] OK    Fix: 

- **`snapshotNameLabel`**
  - EN: Name:
  - PT: Nome:
  - [ ] OK    Fix: 

- **`storeSnapshotMessage`**
  - EN: Enter a name for the new snapshot:
  - PT: Introduza um nome para o novo snapshot:
  - [ ] OK    Fix: 

- **`storeSnapshotTitle`**
  - EN: Store New Snapshot
  - PT: Guardar novo snapshot
  - [ ] OK    Fix: 

- **`trackingConflictCluster`**
  - EN: Input {current} has tracking enabled, but Input {existing} in Cluster {cluster} is already tracked.

Only one tracked input per cluster is allowed.
  - PT: A entrada {current} tem o tracking ativado, mas a entrada {existing} do cluster {cluster} já está a ser rastreada.

Apenas é permitida uma entrada com tracking por cluster.
  - [ ] OK    Fix: 

- **`trackingConflictContinue`**
  - EN: Continue (disable tracking)
  - PT: Continuar (desativar tracking)
  - [ ] OK    Fix: 

- **`trackingConflictSwitch`**
  - EN: Input {existing} in Cluster {cluster} already has tracking enabled.

Only one tracked input per cluster is allowed.

Do you want to disable tracking on Input {existing} and enable it on Input {to}?
  - PT: A entrada {existing} do cluster {cluster} já tem o tracking ativado.

Apenas é permitida uma entrada com tracking por cluster.

Deseja desativar o tracking na entrada {existing} e ativá-lo na entrada {to}?
  - [ ] OK    Fix: 

- **`trackingConflictTitle`**
  - EN: Tracking Conflict
  - PT: Conflito de tracking
  - [ ] OK    Fix: 

- **`trackingConflictYes`**
  - EN: Yes, switch tracking
  - PT: Sim, transferir tracking
  - [ ] OK    Fix: 

## `inputs.gradientMap`

- **`buttons.copyLayer`**
  - EN: Copy Layer
  - PT: Copiar camada
  - [ ] OK    Fix: 

- **`buttons.copyShape`**
  - EN: Copy Shape
  - PT: Copiar forma
  - [ ] OK    Fix: 

- **`buttons.delete`**
  - EN: Delete
  - PT: Excluir
  - [ ] OK    Fix: 

- **`buttons.enable`**
  - EN: On
  - PT: On
  - [ ] OK    Fix: 

- **`buttons.lock`**
  - EN: Lock
  - PT: Bloquear
  - [ ] OK    Fix: 

- **`buttons.pasteLayer`**
  - EN: Paste Layer
  - PT: Colar camada
  - [ ] OK    Fix: 

- **`buttons.pasteShape`**
  - EN: Paste Shape
  - PT: Colar forma
  - [ ] OK    Fix: 

- **`header.attenuationLayer`**
  - EN: Attenuation Layer
  - PT: Camada Atenuação
  - [ ] OK    Fix: 

- **`header.heightLayer`**
  - EN: Height Layer
  - PT: Camada Altura
  - [ ] OK    Fix: 

- **`header.hfShelfLayer`**
  - EN: HF Shelf Layer
  - PT: Camada Prateleira HF
  - [ ] OK    Fix: 

- **`help.blackValue`**
  - EN: Parameter value mapped to black (0.00–1.00)
  - PT: Valor do parâmetro mapeado para preto (0.00–1.00)
  - [ ] OK    Fix: 

- **`help.blur`**
  - EN: Edge blur in meters
  - PT: Desfoque de bordas em metros
  - [ ] OK    Fix: 

- **`help.copy`**
  - EN: Copy selected shape or layer
  - PT: Copiar forma ou camada selecionada
  - [ ] OK    Fix: 

- **`help.curve`**
  - EN: Gamma curve (-1 to 1, 0 = linear)
  - PT: Curva gamma (-1 a 1, 0 = linear)
  - [ ] OK    Fix: 

- **`help.ellipseTool`**
  - EN: Draw ellipse
  - PT: Desenhar elipse
  - [ ] OK    Fix: 

- **`help.fillTool`**
  - EN: Apply uniform fill to shape
  - PT: Aplicar preenchimento uniforme à forma
  - [ ] OK    Fix: 

- **`help.fillValue`**
  - EN: Fill brightness (0 = black, 1 = white)
  - PT: Brilho de preenchimento (0 = preto, 1 = branco)
  - [ ] OK    Fix: 

- **`help.layerEnable`**
  - EN: Enable/disable layer (affects output and OSC)
  - PT: Ativar/desativar camada (afeta saída e OSC)
  - [ ] OK    Fix: 

- **`help.layerSelect`**
  - EN: Select this layer for editing
  - PT: Selecionar esta camada para edição
  - [ ] OK    Fix: 

- **`help.layerVisible`**
  - EN: Show/hide layer preview on canvas
  - PT: Mostrar/ocultar pré-visualização da camada no canvas
  - [ ] OK    Fix: 

- **`help.linGradTool`**
  - EN: Apply linear gradient to shape
  - PT: Aplicar gradiente linear à forma
  - [ ] OK    Fix: 

- **`help.paste`**
  - EN: Paste shape or layer from clipboard
  - PT: Colar forma ou camada da área de transferência
  - [ ] OK    Fix: 

- **`help.polygonTool`**
  - EN: Draw polygon (double-click to close)
  - PT: Desenhar polígono (duplo clique para fechar)
  - [ ] OK    Fix: 

- **`help.radGradTool`**
  - EN: Apply radial gradient to shape
  - PT: Aplicar gradiente radial à forma
  - [ ] OK    Fix: 

- **`help.rectTool`**
  - EN: Draw rectangle
  - PT: Desenhar retângulo
  - [ ] OK    Fix: 

- **`help.selectTool`**
  - EN: Select and move shapes
  - PT: Selecionar e mover formas
  - [ ] OK    Fix: 

- **`help.shapeDelete`**
  - EN: Delete selected shape(s)
  - PT: Excluir forma(s) selecionada(s)
  - [ ] OK    Fix: 

- **`help.shapeEnable`**
  - EN: Enable/disable shape
  - PT: Ativar/desativar forma
  - [ ] OK    Fix: 

- **`help.shapeLock`**
  - EN: Lock shape position
  - PT: Bloquear posição da forma
  - [ ] OK    Fix: 

- **`help.whiteValue`**
  - EN: Parameter value mapped to white (0.00–1.00)
  - PT: Valor do parâmetro mapeado para branco (0.00–1.00)
  - [ ] OK    Fix: 

- **`hints.darkMaxAtten`**
  - EN: Dark = max attenuation | Light = none
  - PT: Escuro = atenuação máx. | Claro = nenhuma
  - [ ] OK    Fix: 

- **`hints.darkMaxHeight`**
  - EN: Dark = max height | Light = ground
  - PT: Escuro = altura máx. | Claro = chão
  - [ ] OK    Fix: 

- **`hints.darkMaxHF`**
  - EN: Dark = max HF shelf | Light = none
  - PT: Escuro = prateleira HF máx. | Claro = nenhuma
  - [ ] OK    Fix: 

- **`hints.polygonClose`**
  - EN: Double-click to close polygon
  - PT: Duplo clique para fechar o polígono
  - [ ] OK    Fix: 

- **`hints.whiteMaxAtten`**
  - EN: White = max attenuation | Black = none
  - PT: Branco = atenuação máx. | Preto = nenhuma
  - [ ] OK    Fix: 

- **`hints.whiteMaxHeight`**
  - EN: White = max height | Black = ground
  - PT: Branco = altura máx. | Preto = chão
  - [ ] OK    Fix: 

- **`hints.whiteMaxHF`**
  - EN: White = max HF shelf | Black = none
  - PT: Branco = prateleira HF máx. | Preto = nenhuma
  - [ ] OK    Fix: 

- **`labels.black`**
  - EN: Black:
  - PT: Preto:
  - [ ] OK    Fix: 

- **`labels.blur`**
  - EN: Blur:
  - PT: Desfoque:
  - [ ] OK    Fix: 

- **`labels.center`**
  - EN: Center:
  - PT: Centro:
  - [ ] OK    Fix: 

- **`labels.curve`**
  - EN: Curve:
  - PT: Curva:
  - [ ] OK    Fix: 

- **`labels.edge`**
  - EN: Edge:
  - PT: Borda:
  - [ ] OK    Fix: 

- **`labels.end`**
  - EN: End:
  - PT: Fim:
  - [ ] OK    Fix: 

- **`labels.fill`**
  - EN: Fill:
  - PT: Preench.:
  - [ ] OK    Fix: 

- **`labels.name`**
  - EN: Name:
  - PT: Nome:
  - [ ] OK    Fix: 

- **`labels.start`**
  - EN: Start:
  - PT: Início:
  - [ ] OK    Fix: 

- **`labels.white`**
  - EN: White:
  - PT: Branco:
  - [ ] OK    Fix: 

- **`layers.attenuation`**
  - EN: Attenuation
  - PT: Atenuação
  - [ ] OK    Fix: 

- **`layers.height`**
  - EN: Height
  - PT: Altura
  - [ ] OK    Fix: 

- **`layers.hfShelf`**
  - EN: HF Shelf
  - PT: Prateleira HF
  - [ ] OK    Fix: 

- **`tools.editPoints`**
  - EN: Edit Points
  - PT: Editar pontos
  - [ ] OK    Fix: 

- **`tools.ellipse`**
  - EN: Ellipse
  - PT: Elipse
  - [ ] OK    Fix: 

- **`tools.fill`**
  - EN: Fill
  - PT: Preench.
  - [ ] OK    Fix: 

- **`tools.linGrad`**
  - EN: Linear Grad.
  - PT: Grad. Linear
  - [ ] OK    Fix: 

- **`tools.polygon`**
  - EN: Polygon
  - PT: Polígono
  - [ ] OK    Fix: 

- **`tools.radGrad`**
  - EN: Radial Grad.
  - PT: Grad. Radial
  - [ ] OK    Fix: 

- **`tools.rect`**
  - EN: Rectangle
  - PT: Retângulo
  - [ ] OK    Fix: 

- **`tools.select`**
  - EN: Select
  - PT: Seleção
  - [ ] OK    Fix: 

- **`warnings.heightRatioZero`**
  - EN: Height Ratio is 0% — increase it for height to take effect
  - PT: A proporção de altura é 0% — aumente-a para que a altura tenha efeito
  - [ ] OK    Fix: 

## `inputs.help`

- **`admMapping`**
  - EN: Assign this input to an ADM-OSC mapping for position receive/transmit.
  - PT: Atribuir esta entrada a um mapeamento ADM-OSC para recepção/transmissão de posição.
  - [ ] OK    Fix: 

- **`arrayAttenDial`**
  - EN: Attenuation for Array {num} (-60 to 0 dB).
  - PT: Atenuação para o array {num} (-60 a 0 dB).
  - [ ] OK    Fix: 

- **`attenuationLawButton`**
  - EN: Attenuation Law Model (Linear Decrease of Volume with Distance Between Object and Speaker or Squared).
  - PT: Modelo da lei de atenuação (decréscimo linear do volume com a distância entre objeto e altifalante, ou quadrática).
  - [ ] OK    Fix: 

- **`attenuationSlider`**
  - EN: Input Channel Attenuation.
  - PT: Atenuação do canal de entrada.
  - [ ] OK    Fix: 

- **`channelSelector`**
  - EN: Input Channel Number and Selection.
  - PT: Número e seleção do canal de entrada.
  - [ ] OK    Fix: 

- **`clusterSelector`**
  - EN: Object is Part of a Cluster.
  - PT: O objeto faz parte de um cluster.
  - [ ] OK    Fix: 

- **`commonAttenDial`**
  - EN: Percentage of the Common Part of the Attenuation for selected Object Relative to All Outputs.
  - PT: Percentagem da parte comum da atenuação para o objeto selecionado em relação a todas as saídas.
  - [ ] OK    Fix: 

- **`constraintDistanceButton`**
  - EN: Limit Position to Distance Range from Origin (for Cylindrical/Spherical modes).
  - PT: Limitar a posição a um intervalo de distância da origem (para modos cilíndrico/esférico).
  - [ ] OK    Fix: 

- **`constraintXButton`**
  - EN: Limit Position to the Bounds of the Stage in Width.
  - PT: Limitar a posição aos limites do palco em largura.
  - [ ] OK    Fix: 

- **`constraintYButton`**
  - EN: Limit Position to the Bounds of the Stage in Depth.
  - PT: Limitar a posição aos limites do palco em profundidade.
  - [ ] OK    Fix: 

- **`constraintZButton`**
  - EN: Limit Position to the Bounds of the Stage in Height.
  - PT: Limitar a posição aos limites do palco em altura.
  - [ ] OK    Fix: 

- **`coordModeSelector`**
  - EN: Coordinate display mode: Cartesian (X/Y/Z), Cylindrical (radius/azimuth/height), or Spherical (radius/azimuth/elevation).
  - PT: Modo de exibição de coordenadas: Cartesiano (X/Y/Z), Cilíndrico (raio/azimute/altura) ou Esférico (raio/azimute/elevação).
  - [ ] OK    Fix: 

- **`delayLatencySlider`**
  - EN: Input Channel Delay (positive values) or Latency Compensation (negative values).
  - PT: Atraso do canal de entrada (valores positivos) ou compensação de latência (valores negativos).
  - [ ] OK    Fix: 

- **`deleteSnapshot`**
  - EN: Delete Selected Input Snapshot With Confirmation.
  - PT: Eliminar o snapshot de entrada selecionado com confirmação.
  - [ ] OK    Fix: 

- **`directivitySlider`**
  - EN: How Wide is the Brightness of The Object.
  - PT: Largura do cone de brilho do objeto.
  - [ ] OK    Fix: 

- **`distanceAttenDial`**
  - EN: Attenuation per Meter Between Object and Speaker.
  - PT: Atenuação por metro entre o objeto e o altifalante.
  - [ ] OK    Fix: 

- **`distanceMaxEditor`**
  - EN: Maximum Distance from Origin in Meters.
  - PT: Distância máxima da origem em metros.
  - [ ] OK    Fix: 

- **`distanceMinEditor`**
  - EN: Minimum Distance from Origin in Meters.
  - PT: Distância mínima da origem em metros.
  - [ ] OK    Fix: 

- **`distanceRangeSlider`**
  - EN: Set Minimum and Maximum Distance from Origin.
  - PT: Definir a distância mínima e máxima da origem.
  - [ ] OK    Fix: 

- **`distanceRatioDial`**
  - EN: Attenuation Ratio for Squared Model.
  - PT: Rácio de atenuação para o modelo quadrático.
  - [ ] OK    Fix: 

- **`editScope`**
  - EN: Open Selected Input Snapshot Scope Window.
  - PT: Abrir a janela de filtro do snapshot de entrada selecionado.
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Input Configuration to file (with file explorer window).
  - PT: Exportar a configuração de entrada para ficheiro (com explorador de ficheiros).
  - [ ] OK    Fix: 

- **`flipXButton`**
  - EN: X will be Symetrical to the Origin. Keyboard Nudging will be Inverted.
  - PT: X será simétrica em relação à origem. A navegação por teclado será invertida.
  - [ ] OK    Fix: 

- **`flipYButton`**
  - EN: Y will be Symetrical to the Origin. Keyboard Nudging will be Inverted.
  - PT: Y será simétrica em relação à origem. A navegação por teclado será invertida.
  - [ ] OK    Fix: 

- **`flipZButton`**
  - EN: Z will be Symetrical to the Origin. Keyboard Nudging will be Inverted.
  - PT: Z será simétrica em relação à origem. A navegação por teclado será invertida.
  - [ ] OK    Fix: 

- **`frActiveButton`**
  - EN: Enable Simulated Floor Reflections for the Object.
  - PT: Ativar as reflexões de chão simuladas para o objeto.
  - [ ] OK    Fix: 

- **`frAttenuationSlider`**
  - EN: Attenuation of the Simulated Floor Reflections for the Object.
  - PT: Atenuação das reflexões de chão simuladas para o objeto.
  - [ ] OK    Fix: 

- **`frDiffusionDial`**
  - EN: Diffusion Effect of the Simulated Floor Reflections for the Object.
  - PT: Efeito de difusão das reflexões de chão simuladas para o objeto.
  - [ ] OK    Fix: 

- **`frHighShelfActiveButton`**
  - EN: Enable High Shelf Filter for Floor Reflections.
  - PT: Ativar o filtro shelving agudo para as reflexões de chão.
  - [ ] OK    Fix: 

- **`frHighShelfFreqSlider`**
  - EN: High Shelf Frequency for Floor Reflections.
  - PT: Frequência do shelving agudo para as reflexões de chão.
  - [ ] OK    Fix: 

- **`frHighShelfGainSlider`**
  - EN: High Shelf Gain for Floor Reflections.
  - PT: Ganho do shelving agudo para as reflexões de chão.
  - [ ] OK    Fix: 

- **`frHighShelfSlopeSlider`**
  - EN: High Shelf Slope for Floor Reflections.
  - PT: Inclinação do shelving agudo para as reflexões de chão.
  - [ ] OK    Fix: 

- **`frLowCutActiveButton`**
  - EN: Enable Low Cut Filter for Floor Reflections.
  - PT: Ativar o filtro corte-baixo para as reflexões de chão.
  - [ ] OK    Fix: 

- **`frLowCutFreqSlider`**
  - EN: Low Cut Frequency for Floor Reflections.
  - PT: Frequência do corte-baixo para as reflexões de chão.
  - [ ] OK    Fix: 

- **`heightFactorDial`**
  - EN: Take Elevation of Object into Account Fully, Partially or Not.
  - PT: Considerar a elevação do objeto totalmente, parcialmente ou nada.
  - [ ] OK    Fix: 

- **`hfShelfSlider`**
  - EN: How Much Brightness is lost in the Back of the Object, Out of its Brightness Cone.
  - PT: Quanto brilho se perde atrás do objeto, fora do seu cone de brilho.
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Input Configuration from file (with file explorer window).
  - PT: Importar a configuração de entrada a partir de ficheiro (com explorador de ficheiros).
  - [ ] OK    Fix: 

- **`inputDirectivityDial`**
  - EN: Where is the Object pointing to in the Horizontal Plane.
  - PT: Direção do objeto no plano horizontal.
  - [ ] OK    Fix: 

- **`jitterSlider`**
  - EN: Sphere of Rapid Movements of the Object.
  - PT: Esfera de movimentos rápidos do objeto.
  - [ ] OK    Fix: 

- **`lfoActiveButton`**
  - EN: Enable or Disable the Periodic Movement of the Object (LFO).
  - PT: Ativar ou desativar o movimento periódico do objeto (LFO).
  - [ ] OK    Fix: 

- **`lfoAmplitudeXSlider`**
  - EN: Width of Movement in Relation to Base Position of the Object.
  - PT: Largura do movimento em relação à posição base do objeto.
  - [ ] OK    Fix: 

- **`lfoAmplitudeYSlider`**
  - EN: Depth of Movement in Relation to Base Position of the Object.
  - PT: Profundidade do movimento em relação à posição base do objeto.
  - [ ] OK    Fix: 

- **`lfoAmplitudeZSlider`**
  - EN: Height of Movement in Relation to Base Position of the Object.
  - PT: Altura do movimento em relação à posição base do objeto.
  - [ ] OK    Fix: 

- **`lfoGyrophoneSelector`**
  - EN: Rotation of the Brightness Cone of the Object.
  - PT: Rotação do cone de brilho do objeto.
  - [ ] OK    Fix: 

- **`lfoPeriodDial`**
  - EN: Base Period of the Movement of the Object.
  - PT: Período base do movimento do objeto.
  - [ ] OK    Fix: 

- **`lfoPhaseDial`**
  - EN: Phase Offset of the Movement of the Object.
  - PT: Desfasamento do movimento do objeto.
  - [ ] OK    Fix: 

- **`lfoPhaseXDial`**
  - EN: Phase Offset of the Movement of the Object in Width.
  - PT: Desfasamento do movimento do objeto em largura.
  - [ ] OK    Fix: 

- **`lfoPhaseYDial`**
  - EN: Phase Offset of the Movement of the Object in Depth.
  - PT: Desfasamento do movimento do objeto em profundidade.
  - [ ] OK    Fix: 

- **`lfoPhaseZDial`**
  - EN: Phase Offset of the Movement of the Object in Height.
  - PT: Desfasamento do movimento do objeto em altura.
  - [ ] OK    Fix: 

- **`lfoRateXSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Width.
  - PT: Movimento mais rápido ou mais lento em relação ao período base, em largura.
  - [ ] OK    Fix: 

- **`lfoRateYSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Depth.
  - PT: Movimento mais rápido ou mais lento em relação ao período base, em profundidade.
  - [ ] OK    Fix: 

- **`lfoRateZSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Height.
  - PT: Movimento mais rápido ou mais lento em relação ao período base, em altura.
  - [ ] OK    Fix: 

- **`lfoShapeXSelector`**
  - EN: Movement Behaviour of the Object in Width.
  - PT: Comportamento do movimento do objeto em largura.
  - [ ] OK    Fix: 

- **`lfoShapeYSelector`**
  - EN: Movement Behaviour of the Object in Depth.
  - PT: Comportamento do movimento do objeto em profundidade.
  - [ ] OK    Fix: 

- **`lfoShapeZSelector`**
  - EN: Movement Behaviour of the Object in Height.
  - PT: Comportamento do movimento do objeto em altura.
  - [ ] OK    Fix: 

- **`lsActiveButton`**
  - EN: If You Need to Reduce the Level in Speakers Close to the Object. (eg. Loud Source Present on Stage)
  - PT: Se for necessário reduzir o nível dos altifalantes próximos do objeto (p. ex., fonte alta presente no palco).
  - [ ] OK    Fix: 

- **`lsAttenuationSlider`**
  - EN: Constant Attenuation of Speakers Around the Object.
  - PT: Atenuação constante dos altifalantes à volta do objeto.
  - [ ] OK    Fix: 

- **`lsPeakEnableButton`**
  - EN: Enable or Disable the Fast (Peak) Compressor for the Live Source Tamer.
  - PT: Ativar ou desativar o compressor rápido (pico) do Domador de Source Live.
  - [ ] OK    Fix: 

- **`lsPeakRatioDial`**
  - EN: Ratio to Apply the Fast Compression for Speakers Around the Object.
  - PT: Ratio aplicado à compressão rápida para os altifalantes à volta do objeto.
  - [ ] OK    Fix: 

- **`lsPeakThresholdSlider`**
  - EN: Fast Compression Threshold for Speakers Around the Object to Control Transients.
  - PT: Limiar de compressão rápida para os altifalantes à volta do objeto, para controlar transitórios.
  - [ ] OK    Fix: 

- **`lsRadiusSlider`**
  - EN: How Far does the Attenuation Affect The Speakers.
  - PT: Até onde a atenuação afeta os altifalantes.
  - [ ] OK    Fix: 

- **`lsShapeSelector`**
  - EN: Profile of the Attenuation Around the Object.
  - PT: Perfil da atenuação à volta do objeto.
  - [ ] OK    Fix: 

- **`lsSlowEnableButton`**
  - EN: Enable or Disable the Slow Compressor for the Live Source Tamer.
  - PT: Ativar ou desativar o compressor lento do Domador de Source Live.
  - [ ] OK    Fix: 

- **`lsSlowRatioDial`**
  - EN: Ratio to Apply the Slow Compression for Speakers Around the Object.
  - PT: Ratio aplicado à compressão lenta para os altifalantes à volta do objeto.
  - [ ] OK    Fix: 

- **`lsSlowThresholdSlider`**
  - EN: Slow Compression Threshold for Speakers Around the Object to Control Sustained Level.
  - PT: Limiar de compressão lenta para os altifalantes à volta do objeto, para controlar o nível sustentado.
  - [ ] OK    Fix: 

- **`mapLockButton`**
  - EN: Prevent Interaction on the Map Tab
  - PT: Impedir a interação no separador Mapa.
  - [ ] OK    Fix: 

- **`mapVisibilityButton`**
  - EN: Make Visible or Hide The Selected Input on the Map
  - PT: Mostrar ou ocultar a entrada selecionada no mapa.
  - [ ] OK    Fix: 

- **`maxSpeedActiveButton`**
  - EN: Enable or Disable Speed Limiting for Object.
  - PT: Ativar ou desativar a limitação de velocidade para o objeto.
  - [ ] OK    Fix: 

- **`maxSpeedDial`**
  - EN: Maximum Speed Limit for Object.
  - PT: Limite de velocidade máxima para o objeto.
  - [ ] OK    Fix: 

- **`minimalLatencyButton`**
  - EN: Select between Acoustic Precedence and Minimal Latency for Amplification Precedence.
  - PT: Escolher entre precedência acústica e latência mínima para a precedência de amplificação.
  - [ ] OK    Fix: 

- **`muteButton`**
  - EN: Mute Output {num} for this Object.
  - PT: Silenciar a saída {num} para este objeto.
  - [ ] OK    Fix: 

- **`muteMacrosSelector`**
  - EN: Mute Macros for Fast Muting and Unmuting of Arrays.
  - PT: Macros de mute para silenciar e reativar rapidamente os arrays.
  - [ ] OK    Fix: 

- **`muteReverbSendsButton`**
  - EN: Mute sends from this input to all reverb channels.
  - PT: Silenciar os envios desta entrada para todos os canais de reverberação.
  - [ ] OK    Fix: 

- **`nameEditor`**
  - EN: Displayed Input Channel Name (editable).
  - PT: Nome apresentado do canal de entrada (editável).
  - [ ] OK    Fix: 

- **`nudgeHint1`**
  - EN:  Nudge with Left and Right Arrow Keys.
  - PT:  Ajustar com as teclas Esquerda e Direita.
  - [ ] OK    Fix: 

- **`nudgeHint2`**
  - EN:  Nudge with Up and Down Arrow Keys.
  - PT:  Ajustar com as teclas Cima e Baixo.
  - [ ] OK    Fix: 

- **`nudgeHint3`**
  - EN:  Nudge with Page Up and Page Down Keys.
  - PT:  Ajustar com Page Up e Page Down.
  - [ ] OK    Fix: 

- **`offset1`**
  - EN: Object Offset {name} ({unit}). Adjusted when Tracking is Enabled.
  - PT: Deslocamento {name} do objeto ({unit}). Ajustado quando o rastreamento está ativado.
  - [ ] OK    Fix: 

- **`offset2`**
  - EN: Object Offset {name} ({unit}). Adjusted when Tracking is Enabled.
  - PT: Deslocamento {name} do objeto ({unit}). Ajustado quando o rastreamento está ativado.
  - [ ] OK    Fix: 

- **`offset3`**
  - EN: Object Offset {name} ({unit}). Adjusted when Tracking is Enabled.
  - PT: Deslocamento {name} do objeto ({unit}). Ajustado quando o rastreamento está ativado.
  - [ ] OK    Fix: 

- **`otomoAbsRelButton`**
  - EN: Select Relative or Absolute Coordinates of Displacement.
  - PT: Escolher coordenadas de deslocamento relativas ou absolutas.
  - [ ] OK    Fix: 

- **`otomoCoordMode`**
  - EN: Coordinate display mode for AutomOtion destinations: Cartesian (X/Y/Z), Cylindrical (r/θ/Z), or Spherical (r/θ/φ).
  - PT: Modo de coordenadas para destinos AutomOtion: Cartesiano (X/Y/Z), Cilíndrico (r/θ/Z) ou Esférico (r/θ/φ).
  - [ ] OK    Fix: 

- **`otomoCurveDial`**
  - EN: Bend the Path to the Left (Negative) or Right (Positive) of the Direction of Travel.
  - PT: Curvar a trajetória para a esquerda (negativo) ou para a direita (positivo) do sentido de movimento.
  - [ ] OK    Fix: 

- **`otomoDest1`**
  - EN: Relative or Absolute Destination {name} ({unit}).
  - PT: Destino relativo ou absoluto {name} ({unit}).
  - [ ] OK    Fix: 

- **`otomoDest2`**
  - EN: Relative or Absolute Destination {name} ({unit}).
  - PT: Destino relativo ou absoluto {name} ({unit}).
  - [ ] OK    Fix: 

- **`otomoDest3`**
  - EN: Relative or Absolute Destination {name} ({unit}).
  - PT: Destino relativo ou absoluto {name} ({unit}).
  - [ ] OK    Fix: 

- **`otomoDurationDial`**
  - EN: Duration of the Movement in Seconds (0.1s to 1 hour).
  - PT: Duração do movimento em segundos (0,1 s a 1 hora).
  - [ ] OK    Fix: 

- **`otomoPauseButton`**
  - EN: Pause and Resume the Movement.
  - PT: Pausar e retomar o movimento.
  - [ ] OK    Fix: 

- **`otomoPauseResumeAllButton`**
  - EN: Pause or Resume All Active Movements Globally.
  - PT: Pausar ou retomar globalmente todos os movimentos ativos.
  - [ ] OK    Fix: 

- **`otomoResetDial`**
  - EN: Set the Reset Level for the Automatic Trigger.
  - PT: Definir o nível de reposição para o disparo automático.
  - [ ] OK    Fix: 

- **`otomoSpeedProfileDial`**
  - EN: Constant Speed or Gradual Acceleration and Slow Down at the Start and the End of the Movement.
  - PT: Velocidade constante ou aceleração e desaceleração graduais no início e no fim do movimento.
  - [ ] OK    Fix: 

- **`otomoStartButton`**
  - EN: Start the Movement Manually.
  - PT: Iniciar o movimento manualmente.
  - [ ] OK    Fix: 

- **`otomoStayReturnButton`**
  - EN: At the End of the Movement, should the Source Stay or Return to the Original Position.
  - PT: No fim do movimento, a fonte permanece ou regressa à posição original.
  - [ ] OK    Fix: 

- **`otomoStopAllButton`**
  - EN: Stop All Active Movements Globally.
  - PT: Parar globalmente todos os movimentos ativos.
  - [ ] OK    Fix: 

- **`otomoStopButton`**
  - EN: Stop the Movement.
  - PT: Parar o movimento.
  - [ ] OK    Fix: 

- **`otomoThresholdDial`**
  - EN: Set the Threshold for the Automatic Trigger of the Movement.
  - PT: Definir o limiar para o disparo automático do movimento.
  - [ ] OK    Fix: 

- **`otomoTriggerButton`**
  - EN: Manual Start of Displacement or Automatic Trigger on the Audio Level.
  - PT: Início manual do deslocamento ou disparo automático com base no nível de áudio.
  - [ ] OK    Fix: 

- **`pathModeButton`**
  - EN: Enable Path Mode to Follow Drawn Movement Paths Instead of Direct Lines.
  - PT: Ativar o modo Trajetória para seguir caminhos de movimento desenhados em vez de linhas diretas.
  - [ ] OK    Fix: 

- **`position1`**
  - EN: Object {name} ({unit}).
  - PT: {name} do objeto ({unit}).
  - [ ] OK    Fix: 

- **`position2`**
  - EN: Object {name} ({unit}).
  - PT: {name} do objeto ({unit}).
  - [ ] OK    Fix: 

- **`position3`**
  - EN: Object {name} ({unit}).
  - PT: {name} do objeto ({unit}).
  - [ ] OK    Fix: 

- **`positionJoystick`**
  - EN: Drag to adjust X/Y position in real-time. Returns to center on release.
  - PT: Arrastar para ajustar a posição X/Y em tempo real. Volta ao centro ao soltar.
  - [ ] OK    Fix: 

- **`positionZSlider`**
  - EN: Drag to adjust Z (height) position in real-time. Returns to center on release.
  - PT: Arrastar para ajustar a posição Z (altura) em tempo real. Volta ao centro ao soltar.
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Input Configuration from backup file.
  - PT: Recarregar a configuração de entrada a partir do ficheiro de cópia de segurança.
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Input Configuration from file.
  - PT: Recarregar a configuração de entrada a partir do ficheiro.
  - [ ] OK    Fix: 

- **`reloadSnapshot`**
  - EN: Reload Selected Input Snapshot for All Objects Taking the Scope into Account.
  - PT: Recarregar o snapshot de entrada selecionado para todos os objetos tendo em conta o filtro.
  - [ ] OK    Fix: 

- **`reloadWithoutScope`**
  - EN: Reload Selected Input Snapshot for All Objects Without the Scope.
  - PT: Recarregar o snapshot de entrada selecionado para todos os objetos sem o filtro.
  - [ ] OK    Fix: 

- **`sidelinesActiveButton`**
  - EN: Enable Automatic Muting when Source Approaches Stage Edges. Does Not Apply to Downstage (Front) Edge.
  - PT: Ativar o silenciamento automático quando a fonte se aproxima das margens do palco. Não se aplica à margem da frente (lado do público).
  - [ ] OK    Fix: 

- **`sidelinesFringeDial`**
  - EN: Fringe Zone Size in Meters. Outer Half is Full Mute, Inner Half Fades Linearly.
  - PT: Tamanho da zona de transição em metros. A metade exterior silencia totalmente, a metade interior atenua linearmente.
  - [ ] OK    Fix: 

- **`snapshotSelector`**
  - EN: Select Input Snapshot Without Loading.
  - PT: Selecionar snapshot de entrada sem carregar.
  - [ ] OK    Fix: 

- **`solo`**
  - EN: Listen to Binaural Rendering of this channel.
  - PT: Ouvir o renderização binaural deste canal.
  - [ ] OK    Fix: 

- **`soloMode`**
  - EN: Single: one input at a time. Multi: multiple inputs simultaneously.
  - PT: Único: uma entrada de cada vez. Múltiplo: várias entradas simultaneamente.
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Input Configuration to file (with backup).
  - PT: Guardar a configuração de entrada em ficheiro (com cópia de segurança).
  - [ ] OK    Fix: 

- **`storeSnapshot`**
  - EN: Store new Input Snapshot for All Objects.
  - PT: Guardar novo snapshot de entrada para todos os objetos.
  - [ ] OK    Fix: 

- **`tiltSlider`**
  - EN: Where is the Object pointing to in the Vertical Plane.
  - PT: Direção do objeto no plano vertical.
  - [ ] OK    Fix: 

- **`trackingActiveButton`**
  - EN: Enable or Disable Tracking for Object.
  - PT: Ativar ou desativar o tracking para o objeto.
  - [ ] OK    Fix: 

- **`trackingIdSelector`**
  - EN: Tracker ID for Object.
  - PT: ID do tracker para o objeto.
  - [ ] OK    Fix: 

- **`trackingSmoothDial`**
  - EN: Smoothing of Tracking Data for Object.
  - PT: Suavização dos dados de tracking para o objeto.
  - [ ] OK    Fix: 

- **`updateSnapshot`**
  - EN: Update Selected Input Snapshot (with backup).
  - PT: Atualizar o snapshot de entrada selecionado (com cópia de segurança).
  - [ ] OK    Fix: 

## `inputs.labels`

- **`admMapping`**
  - EN: ADM:
  - PT: ADM:
  - [ ] OK    Fix: 

- **`amplitudeX`**
  - EN: Amplitude X:
  - PT: Amplitude X:
  - [ ] OK    Fix: 

- **`amplitudeY`**
  - EN: Amplitude Y:
  - PT: Amplitude Y:
  - [ ] OK    Fix: 

- **`amplitudeZ`**
  - EN: Amplitude Z:
  - PT: Amplitude Z:
  - [ ] OK    Fix: 

- **`arrayAttenuation`**
  - EN: Array Attenuation:
  - PT: Atenuação de Array:
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Attenuation:
  - PT: Atenuação:
  - [ ] OK    Fix: 

- **`attenuationLaw`**
  - EN: Attenuation Law:
  - PT: Lei de Atenuação:
  - [ ] OK    Fix: 

- **`cluster`**
  - EN: Cluster:
  - PT: Cluster:
  - [ ] OK    Fix: 

- **`commonAtten`**
  - EN: Common Atten:
  - PT: Aten. Comum:
  - [ ] OK    Fix: 

- **`coord`**
  - EN: Coord:
  - PT: Coord.:
  - [ ] OK    Fix: 

- **`curve`**
  - EN: Curve:
  - PT: Curva:
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Delay/Latency:
  - PT: Atraso/Latência:
  - [ ] OK    Fix: 

- **`destX`**
  - EN: Dest. X:
  - PT: Dest. X:
  - [ ] OK    Fix: 

- **`destY`**
  - EN: Dest. Y:
  - PT: Dest. Y:
  - [ ] OK    Fix: 

- **`destZ`**
  - EN: Dest. Z:
  - PT: Dest. Z:
  - [ ] OK    Fix: 

- **`diffusion`**
  - EN: Diffusion:
  - PT: Difusão:
  - [ ] OK    Fix: 

- **`directivity`**
  - EN: Directivity:
  - PT: Diretividade:
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Distance Atten:
  - PT: Aten. Distância:
  - [ ] OK    Fix: 

- **`distanceRatio`**
  - EN: Distance Ratio:
  - PT: Rácio de Distância:
  - [ ] OK    Fix: 

- **`duration`**
  - EN: Duration:
  - PT: Duração:
  - [ ] OK    Fix: 

- **`frequency`**
  - EN: Frequency:
  - PT: Frequência:
  - [ ] OK    Fix: 

- **`fringe`**
  - EN: Fringe:
  - PT: Margem:
  - [ ] OK    Fix: 

- **`gain`**
  - EN: Gain:
  - PT: Ganho:
  - [ ] OK    Fix: 

- **`gyrophone`**
  - EN: Gyrophone:
  - PT: Girófone:
  - [ ] OK    Fix: 

- **`heightFactor`**
  - EN: Height Factor:
  - PT: Fator de Altura:
  - [ ] OK    Fix: 

- **`hfShelf`**
  - EN: HF Shelf:
  - PT: Prateleira HF:
  - [ ] OK    Fix: 

- **`jitter`**
  - EN: Jitter:
  - PT: Tremulação:
  - [ ] OK    Fix: 

- **`max`**
  - EN: Max:
  - PT: Máx:
  - [ ] OK    Fix: 

- **`maxSpeed`**
  - EN: Max Speed:
  - PT: Velocidade Máx:
  - [ ] OK    Fix: 

- **`min`**
  - EN: Min:
  - PT: Mín:
  - [ ] OK    Fix: 

- **`muteMacros`**
  - EN: Mute Macros:
  - PT: Macros de Silenciamento:
  - [ ] OK    Fix: 

- **`name`**
  - EN: Name:
  - PT: Nome:
  - [ ] OK    Fix: 

- **`offsetX`**
  - EN: Offset X:
  - PT: Deslocamento X:
  - [ ] OK    Fix: 

- **`offsetY`**
  - EN: Offset Y:
  - PT: Deslocamento Y:
  - [ ] OK    Fix: 

- **`offsetZ`**
  - EN: Offset Z:
  - PT: Deslocamento Z:
  - [ ] OK    Fix: 

- **`outX`**
  - EN: Out X:
  - PT: Saída X:
  - [ ] OK    Fix: 

- **`outY`**
  - EN: Out Y:
  - PT: Saída Y:
  - [ ] OK    Fix: 

- **`outZ`**
  - EN: Out Z:
  - PT: Saída Z:
  - [ ] OK    Fix: 

- **`peakRatio`**
  - EN: Peak Ratio:
  - PT: Rácio de Pico:
  - [ ] OK    Fix: 

- **`peakThreshold`**
  - EN: Peak Threshold:
  - PT: Limiar de Pico:
  - [ ] OK    Fix: 

- **`period`**
  - EN: Period:
  - PT: Período:
  - [ ] OK    Fix: 

- **`phase`**
  - EN: Phase:
  - PT: Fase:
  - [ ] OK    Fix: 

- **`phaseX`**
  - EN: Phase X:
  - PT: Fase X:
  - [ ] OK    Fix: 

- **`phaseY`**
  - EN: Phase Y:
  - PT: Fase Y:
  - [ ] OK    Fix: 

- **`phaseZ`**
  - EN: Phase Z:
  - PT: Fase Z:
  - [ ] OK    Fix: 

- **`positionX`**
  - EN: Position X:
  - PT: Posição X:
  - [ ] OK    Fix: 

- **`positionY`**
  - EN: Position Y:
  - PT: Posição Y:
  - [ ] OK    Fix: 

- **`positionZ`**
  - EN: Position Z:
  - PT: Posição Z:
  - [ ] OK    Fix: 

- **`radius`**
  - EN: Radius:
  - PT: Raio:
  - [ ] OK    Fix: 

- **`rateX`**
  - EN: Rate X:
  - PT: Taxa X:
  - [ ] OK    Fix: 

- **`rateY`**
  - EN: Rate Y:
  - PT: Taxa Y:
  - [ ] OK    Fix: 

- **`rateZ`**
  - EN: Rate Z:
  - PT: Taxa Z:
  - [ ] OK    Fix: 

- **`reset`**
  - EN: Reset:
  - PT: Reiniciar:
  - [ ] OK    Fix: 

- **`rotation`**
  - EN: Rotation:
  - PT: Rotação:
  - [ ] OK    Fix: 

- **`shape`**
  - EN: Shape:
  - PT: Forma:
  - [ ] OK    Fix: 

- **`shapeX`**
  - EN: Shape X:
  - PT: Forma X:
  - [ ] OK    Fix: 

- **`shapeY`**
  - EN: Shape Y:
  - PT: Forma Y:
  - [ ] OK    Fix: 

- **`shapeZ`**
  - EN: Shape Z:
  - PT: Forma Z:
  - [ ] OK    Fix: 

- **`slope`**
  - EN: Slope:
  - PT: Inclinação:
  - [ ] OK    Fix: 

- **`slowRatio`**
  - EN: Slow Ratio:
  - PT: Rácio Lento:
  - [ ] OK    Fix: 

- **`slowThreshold`**
  - EN: Slow Threshold:
  - PT: Limiar Lento:
  - [ ] OK    Fix: 

- **`speedProfile`**
  - EN: Speed Profile:
  - PT: Perfil de Velocidade:
  - [ ] OK    Fix: 

- **`threshold`**
  - EN: Threshold:
  - PT: Limiar:
  - [ ] OK    Fix: 

- **`tilt`**
  - EN: Tilt:
  - PT: Inclinação:
  - [ ] OK    Fix: 

- **`trackingId`**
  - EN: Tracking ID:
  - PT: ID de Rastreamento:
  - [ ] OK    Fix: 

- **`trackingSmooth`**
  - EN: Tracking Smooth:
  - PT: Suavização Rastreamento:
  - [ ] OK    Fix: 

- **`xyJoystick`**
  - EN: X/Y
  - PT: X/Y
  - [ ] OK    Fix: 

- **`zSlider`**
  - EN: Z
  - PT: Z
  - [ ] OK    Fix: 

## `inputs.lfo`

- **`gyrophone.antiClockwise`**
  - EN: Anti-Clockwise
  - PT: Anti-horário
  - [ ] OK    Fix: 

- **`gyrophone.clockwise`**
  - EN: Clockwise
  - PT: Horário
  - [ ] OK    Fix: 

- **`gyrophone.off`**
  - EN: OFF
  - PT: DESLIGADO
  - [ ] OK    Fix: 

- **`shapes.exp`**
  - EN: exp
  - PT: exp
  - [ ] OK    Fix: 

- **`shapes.keystone`**
  - EN: keystone
  - PT: trapézio
  - [ ] OK    Fix: 

- **`shapes.log`**
  - EN: log
  - PT: log
  - [ ] OK    Fix: 

- **`shapes.off`**
  - EN: OFF
  - PT: DESLIGADO
  - [ ] OK    Fix: 

- **`shapes.random`**
  - EN: random
  - PT: aleatório
  - [ ] OK    Fix: 

- **`shapes.sawtooth`**
  - EN: sawtooth
  - PT: dente de serra
  - [ ] OK    Fix: 

- **`shapes.sine`**
  - EN: sine
  - PT: seno
  - [ ] OK    Fix: 

- **`shapes.square`**
  - EN: square
  - PT: quadrada
  - [ ] OK    Fix: 

- **`shapes.triangle`**
  - EN: triangle
  - PT: triângulo
  - [ ] OK    Fix: 

## `inputs.liveSource`

- **`linear`**
  - EN: linear
  - PT: linear
  - [ ] OK    Fix: 

- **`log`**
  - EN: log
  - PT: log
  - [ ] OK    Fix: 

- **`sine`**
  - EN: sine
  - PT: seno
  - [ ] OK    Fix: 

## `inputs.messages`

- **`assignedCluster`**
  - EN: Input {channel} assigned to Cluster {cluster}
  - PT: Entrada {channel} atribuída ao Cluster {cluster}
  - [ ] OK    Fix: 

- **`backupLoaded`**
  - EN: Input configuration loaded from backup.
  - PT: Configuração de entrada carregada do backup.
  - [ ] OK    Fix: 

- **`configExported`**
  - EN: Input configuration exported.
  - PT: Configuração de entrada exportada.
  - [ ] OK    Fix: 

- **`configImported`**
  - EN: Input configuration imported.
  - PT: Configuração de entrada importada.
  - [ ] OK    Fix: 

- **`configLoaded`**
  - EN: Input configuration loaded.
  - PT: Configuração de entrada carregada.
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Input configuration saved.
  - PT: Configuração de entrada guardada.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - PT: Erro: {error}
  - [ ] OK    Fix: 

- **`noSnapshotSelected`**
  - EN: No snapshot selected.
  - PT: Nenhum snapshot selecionado.
  - [ ] OK    Fix: 

- **`scopeConfigured`**
  - EN: Scope configured for next snapshot.
  - PT: Âmbito configurado para o próximo snapshot.
  - [ ] OK    Fix: 

- **`scopeSaved`**
  - EN: Snapshot scope saved.
  - PT: Âmbito do snapshot guardado.
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - PT: Por favor, selecione primeiro uma pasta de projeto em Config. Sistema.
  - [ ] OK    Fix: 

- **`setSingle`**
  - EN: Input {channel} set to Single
  - PT: Entrada {channel} definida como Individual
  - [ ] OK    Fix: 

- **`snapshotDeleted`**
  - EN: Snapshot '{name}' deleted.
  - PT: Snapshot '{name}' eliminado.
  - [ ] OK    Fix: 

- **`snapshotLoaded`**
  - EN: Snapshot '{name}' loaded.
  - PT: Snapshot '{name}' carregado.
  - [ ] OK    Fix: 

- **`snapshotLoadedWithoutScope`**
  - EN: Snapshot '{name}' loaded (without scope).
  - PT: Snapshot '{name}' carregado (sem âmbito).
  - [ ] OK    Fix: 

- **`snapshotStored`**
  - EN: Snapshot '{name}' stored.
  - PT: Snapshot '{name}' guardado.
  - [ ] OK    Fix: 

- **`snapshotUpdated`**
  - EN: Snapshot '{name}' updated.
  - PT: Snapshot '{name}' atualizado.
  - [ ] OK    Fix: 

- **`trackingDisabled`**
  - EN: Tracking disabled for Input {channel}
  - PT: Rastreamento desativado para Entrada {channel}
  - [ ] OK    Fix: 

- **`trackingSwitched`**
  - EN: Tracking switched from Input {from} to Input {to}
  - PT: Rastreamento mudou de Entrada {from} para Entrada {to}
  - [ ] OK    Fix: 

## `inputs.muteMacros`

- **`invertMutes`**
  - EN: INVERT MUTES
  - PT: INVERTER SILENCIAMENTOS
  - [ ] OK    Fix: 

- **`muteAll`**
  - EN: MUTE ALL
  - PT: SILENCIAR TODOS
  - [ ] OK    Fix: 

- **`muteArrayPrefix`**
  - EN: MUTE ARRAY
  - PT: SILENCIAR ARRAY
  - [ ] OK    Fix: 

- **`muteEven`**
  - EN: MUTE EVEN
  - PT: SILENCIAR PARES
  - [ ] OK    Fix: 

- **`muteOdd`**
  - EN: MUTE ODD
  - PT: SILENCIAR ÍMPARES
  - [ ] OK    Fix: 

- **`selectMacro`**
  - EN: Select Macro...
  - PT: Selecionar Macro...
  - [ ] OK    Fix: 

- **`unmuteAll`**
  - EN: UNMUTE ALL
  - PT: ATIVAR TODOS
  - [ ] OK    Fix: 

- **`unmuteArrayPrefix`**
  - EN: UNMUTE ARRAY
  - PT: ATIVAR ARRAY
  - [ ] OK    Fix: 

## `inputs.prefixes`

- **`delay`**
  - EN: Delay:
  - PT: Atraso:
  - [ ] OK    Fix: 

## `inputs.sections`

- **`automotion`**
  - EN: AutomOtion
  - PT: AutomOção
  - [ ] OK    Fix: 

## `inputs.snapshots`

- **`selectSnapshot`**
  - EN: Select Snapshot...
  - PT: Selecionar Snapshot...
  - [ ] OK    Fix: 

## `inputs.tabs`

- **`gradientMaps`**
  - EN: Gradient Maps
  - PT: Mapas de Gradiente
  - [ ] OK    Fix: 

- **`inputParams`**
  - EN: Input Parameters
  - PT: Parâmetros de Entrada
  - [ ] OK    Fix: 

- **`liveSourceHackoustics`**
  - EN: Live Source & Hackoustics
  - PT: Fonte ao Vivo & Hackústica
  - [ ] OK    Fix: 

- **`movements`**
  - EN: Movements
  - PT: Movimentos
  - [ ] OK    Fix: 

- **`sampler`**
  - EN: Sampler
  - PT: Sampler
  - [ ] OK    Fix: 

- **`visualisation`**
  - EN: Visualisation
  - PT: Visualização
  - [ ] OK    Fix: 

## `inputs.toggles`

- **`absolute`**
  - EN: Absolute
  - PT: Absoluto
  - [ ] OK    Fix: 

- **`acousticPrecedence`**
  - EN: Acoustic Precedence
  - PT: Precedência Acústica
  - [ ] OK    Fix: 

- **`attenuationLawLog`**
  - EN: Log
  - PT: Log
  - [ ] OK    Fix: 

- **`constraintROff`**
  - EN: Constraint R: OFF
  - PT: Restrição R: DESLIGADO
  - [ ] OK    Fix: 

- **`constraintROn`**
  - EN: Constraint R: ON
  - PT: Restrição R: LIGADO
  - [ ] OK    Fix: 

- **`constraintXOff`**
  - EN: Constraint X: OFF
  - PT: Restrição X: DESLIGADO
  - [ ] OK    Fix: 

- **`constraintXOn`**
  - EN: Constraint X: ON
  - PT: Restrição X: LIGADO
  - [ ] OK    Fix: 

- **`constraintYOff`**
  - EN: Constraint Y: OFF
  - PT: Restrição Y: DESLIGADO
  - [ ] OK    Fix: 

- **`constraintYOn`**
  - EN: Constraint Y: ON
  - PT: Restrição Y: LIGADO
  - [ ] OK    Fix: 

- **`constraintZOff`**
  - EN: Constraint Z: OFF
  - PT: Restrição Z: DESLIGADO
  - [ ] OK    Fix: 

- **`constraintZOn`**
  - EN: Constraint Z: ON
  - PT: Restrição Z: LIGADO
  - [ ] OK    Fix: 

- **`flipXOff`**
  - EN: Flip X: OFF
  - PT: Inverter X: DESLIGADO
  - [ ] OK    Fix: 

- **`flipXOn`**
  - EN: Flip X: ON
  - PT: Inverter X: LIGADO
  - [ ] OK    Fix: 

- **`flipYOff`**
  - EN: Flip Y: OFF
  - PT: Inverter Y: DESLIGADO
  - [ ] OK    Fix: 

- **`flipYOn`**
  - EN: Flip Y: ON
  - PT: Inverter Y: LIGADO
  - [ ] OK    Fix: 

- **`flipZOff`**
  - EN: Flip Z: OFF
  - PT: Inverter Z: DESLIGADO
  - [ ] OK    Fix: 

- **`flipZOn`**
  - EN: Flip Z: ON
  - PT: Inverter Z: LIGADO
  - [ ] OK    Fix: 

- **`floorReflectionsOff`**
  - EN: Floor Reflections: OFF
  - PT: Reflexões do Chão: DESLIGADO
  - [ ] OK    Fix: 

- **`floorReflectionsOn`**
  - EN: Floor Reflections: ON
  - PT: Reflexões do Chão: LIGADO
  - [ ] OK    Fix: 

- **`highShelfOff`**
  - EN: High Shelf: OFF
  - PT: Prateleira Agudo: DESLIGADO
  - [ ] OK    Fix: 

- **`highShelfOn`**
  - EN: High Shelf: ON
  - PT: Prateleira Agudo: LIGADO
  - [ ] OK    Fix: 

- **`lfoOff`**
  - EN: L.F.O: OFF
  - PT: L.F.O: DESLIGADO
  - [ ] OK    Fix: 

- **`lfoOn`**
  - EN: L.F.O: ON
  - PT: L.F.O: LIGADO
  - [ ] OK    Fix: 

- **`liveSourceTamerOff`**
  - EN: Live Source Tamer: OFF
  - PT: Domador: OFF
  - [ ] OK    Fix: 

- **`liveSourceTamerOn`**
  - EN: Live Source Tamer: ON
  - PT: Domador: ON
  - [ ] OK    Fix: 

- **`lowCutOff`**
  - EN: Low Cut: OFF
  - PT: Corte Grave: DESLIGADO
  - [ ] OK    Fix: 

- **`lowCutOn`**
  - EN: Low Cut: ON
  - PT: Corte Grave: LIGADO
  - [ ] OK    Fix: 

- **`lsPeakOff`**
  - EN: Peak: OFF
  - PT: Pico: OFF
  - [ ] OK    Fix: 

- **`lsPeakOn`**
  - EN: Peak: ON
  - PT: Pico: ON
  - [ ] OK    Fix: 

- **`lsSlowOff`**
  - EN: Slow: OFF
  - PT: Lento: OFF
  - [ ] OK    Fix: 

- **`lsSlowOn`**
  - EN: Slow: ON
  - PT: Lento: ON
  - [ ] OK    Fix: 

- **`manual`**
  - EN: Manual
  - PT: Manual
  - [ ] OK    Fix: 

- **`maxSpeedOff`**
  - EN: Max Speed: OFF
  - PT: Vel. Máx: DESLIGADO
  - [ ] OK    Fix: 

- **`maxSpeedOn`**
  - EN: Max Speed: ON
  - PT: Vel. Máx: LIGADO
  - [ ] OK    Fix: 

- **`minimalLatency`**
  - EN: Minimal Latency
  - PT: Latência Mínima
  - [ ] OK    Fix: 

- **`pathModeOff`**
  - EN: Path Mode: OFF
  - PT: Modo Caminho: DESLIGADO
  - [ ] OK    Fix: 

- **`pathModeOn`**
  - EN: Path Mode: ON
  - PT: Modo Caminho: LIGADO
  - [ ] OK    Fix: 

- **`relative`**
  - EN: Relative
  - PT: Relativo
  - [ ] OK    Fix: 

- **`return`**
  - EN: Return
  - PT: Regressar
  - [ ] OK    Fix: 

- **`reverbSendsMuted`**
  - EN: Sends to Reverbs: Muted
  - PT: Envios para Reverbs: Silenciados
  - [ ] OK    Fix: 

- **`reverbSendsUnmuted`**
  - EN: Sends to Reverbs: Unmuted
  - PT: Envios para Reverbs: Ativos
  - [ ] OK    Fix: 

- **`sidelinesOff`**
  - EN: Sidelines Off
  - PT: Linhas Laterais Desligadas
  - [ ] OK    Fix: 

- **`sidelinesOn`**
  - EN: Sidelines On
  - PT: Linhas Laterais Ligadas
  - [ ] OK    Fix: 

- **`stay`**
  - EN: Stay
  - PT: Ficar
  - [ ] OK    Fix: 

- **`trackingOff`**
  - EN: Tracking: OFF
  - PT: Rastreamento: DESLIGADO
  - [ ] OK    Fix: 

- **`trackingOn`**
  - EN: Tracking: ON
  - PT: Rastreamento: LIGADO
  - [ ] OK    Fix: 

- **`triggered`**
  - EN: Triggered
  - PT: Acionado
  - [ ] OK    Fix: 

## `inputs.visualisation`

- **`delay`**
  - EN: delay
  - PT: atraso
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: HF
damping
  - PT: atenuação
HF
  - [ ] OK    Fix: 

- **`level`**
  - EN: level
  - PT: nível
  - [ ] OK    Fix: 

## `levelMeter`

- **`inputs`**
  - EN: Inputs
  - PT: Entradas
  - [ ] OK    Fix: 

- **`outputs`**
  - EN: Outputs
  - PT: Saídas
  - [ ] OK    Fix: 

- **`windowTitle`**
  - EN: Level Meters
  - PT: Medidores de Nível
  - [ ] OK    Fix: 

## `levelMeter.buttons`

- **`clearSolo`**
  - EN: Clear Solo
  - PT: Limpar solo
  - [ ] OK    Fix: 

- **`soloModeMulti`**
  - EN: Multi
  - PT: Multi
  - [ ] OK    Fix: 

- **`soloModeSingle`**
  - EN: Single
  - PT: Single
  - [ ] OK    Fix: 

## `levelMeter.tooltips`

- **`clearSolo`**
  - EN: Disengage all solo toggles
  - PT: Desativar todos os solos
  - [ ] OK    Fix: 

- **`solo`**
  - EN: Display the contribution of the input to all outputs in the Level Meter display (in Single mode) and play binaural render of soloed inputs
  - PT: Mostrar a contribuição da entrada para todas as saídas no medidor de nível (em modo Solo único) e reproduzir renderização binaural das entradas em solo
  - [ ] OK    Fix: 

- **`soloMode`**
  - EN: Single: one input at a time. Multi: multiple inputs simultaneously.
  - PT: Único: uma entrada de cada vez. Múltiplo: várias entradas simultaneamente.
  - [ ] OK    Fix: 

## `map`

- **`detachedMessage`**
  - EN: The map is displayed in a separate window.
  - PT: O mapa está exibido em uma janela separada.
  - [ ] OK    Fix: 

- **`reattach`**
  - EN: Re-attach Map
  - PT: Reanexar mapa
  - [ ] OK    Fix: 

## `map.buttons`

- **`fitInputs`**
  - EN: Fit All Inputs to Screen
  - PT: Ajustar todas as entradas à tela
  - [ ] OK    Fix: 

- **`fitStage`**
  - EN: Fit Stage to Screen
  - PT: Ajustar palco à tela
  - [ ] OK    Fix: 

- **`hideLevels`**
  - EN: Hide Levels
  - PT: Ocultar Níveis
  - [ ] OK    Fix: 

- **`showLevels`**
  - EN: Show Levels
  - PT: Mostrar Níveis
  - [ ] OK    Fix: 

## `map.labels`

- **`reverbPrefix`**
  - EN: R
  - PT: R
  - [ ] OK    Fix: 

## `map.messages`

- **`assignedCluster`**
  - EN: Input {channel} assigned to Cluster {cluster}
  - PT: Entrada {channel} atribuída ao Cluster {cluster}
  - [ ] OK    Fix: 

- **`assignedClusterMulti`**
  - EN: {count} inputs assigned to Cluster {cluster}
  - PT: {count} entradas atribuídas ao Cluster {cluster}
  - [ ] OK    Fix: 

- **`clusterBrokenUp`**
  - EN: Cluster {cluster} broken up
  - PT: Cluster {cluster} dissolvido
  - [ ] OK    Fix: 

- **`setSingle`**
  - EN: Input {channel} removed from cluster
  - PT: Entrada {channel} removida do cluster
  - [ ] OK    Fix: 

- **`setSingleMulti`**
  - EN: {count} inputs removed from clusters
  - PT: {count} entradas removidas dos clusters
  - [ ] OK    Fix: 

## `map.tooltips`

- **`detach`**
  - EN: Detach the map into a separate window for dual-screen setups
  - PT: Destacar o mapa em uma janela separada para configurações de tela dupla
  - [ ] OK    Fix: 

- **`fitInputs`**
  - EN: Adjust the zoom and panning to fit all visible inputs on the display
  - PT: Ajustar o zoom e panorâmica para exibir todas as entradas visíveis
  - [ ] OK    Fix: 

- **`fitStage`**
  - EN: Adjust the zoom and panning to fit the stage on the display
  - PT: Ajustar o zoom e panorâmica para exibir o palco
  - [ ] OK    Fix: 

- **`levels`**
  - EN: Display the levels for inputs and outputs on the map
  - PT: Mostrar os níveis de entradas e saídas no mapa
  - [ ] OK    Fix: 

## `network.buttons`

- **`add`**
  - EN: ADD
  - PT: ADICIONAR
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - PT: Exportar
  - [ ] OK    Fix: 

- **`findMyRemote`**
  - EN: Find My Remote
  - PT: Encontrar Controlo Remoto
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - PT: Importar
  - [ ] OK    Fix: 

- **`openLogWindow`**
  - EN: Open Log Window
  - PT: Abrir Janela de Registo
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Backup
  - PT: Recarregar Backup
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Network Config
  - PT: Recarregar Config. Rede
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Network Config
  - PT: Guardar Config. Rede
  - [ ] OK    Fix: 

## `network.dialogs`

- **`exportConfig`**
  - EN: Export Network Configuration
  - PT: Exportar Configuração de Rede
  - [ ] OK    Fix: 

- **`findMyRemoteMessage`**
  - EN: Enter the password for your remote device:
  - PT: Introduza a palavra-passe do seu comando:
  - [ ] OK    Fix: 

- **`findMyRemotePassword`**
  - EN: Password:
  - PT: Palavra-passe:
  - [ ] OK    Fix: 

- **`findMyRemoteTitle`**
  - EN: Find My Remote
  - PT: Encontrar o meu comando
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Network Configuration
  - PT: Importar Configuração de Rede
  - [ ] OK    Fix: 

- **`removeTargetMessage`**
  - EN: Delete target '{name}'?
  - PT: Eliminar o destino '{name}'?
  - [ ] OK    Fix: 

- **`removeTargetTitle`**
  - EN: Remove Target
  - PT: Remover destino
  - [ ] OK    Fix: 

- **`trackingConflictsContinue`**
  - EN: Continue
  - PT: Continuar
  - [ ] OK    Fix: 

- **`trackingConflictsMessage`**
  - EN: 
Only one tracked input per cluster is allowed. If you continue, tracking will be kept only for the first input in each cluster.
  - PT: 
Apenas é permitida uma entrada com tracking por cluster. Se continuar, o tracking será mantido apenas para a primeira entrada de cada cluster.
  - [ ] OK    Fix: 

- **`trackingConflictsTitle`**
  - EN: Tracking Conflicts Detected
  - PT: Conflitos de tracking detetados
  - [ ] OK    Fix: 

## `network.help`

- **`addTarget`**
  - EN: Add new network target.
  - PT: Adicionar novo destino de rede.
  - [ ] OK    Fix: 

- **`admMapping`**
  - EN: Select an ADM-OSC mapping to configure. Cart = Cartesian (xyz), Polar = spherical (aed).
  - PT: Selecionar um mapeamento ADM-OSC para configurar. Cart = Cartesiano (xyz), Polar = esférico (aed).
  - [ ] OK    Fix: 

- **`admMappingPanel`**
  - EN: Drag dots to edit mapping. Click axis title to swap, click Flip to invert. Hold Shift to edit both sides symmetrically.
  - PT: Arrastar pontos para editar o mapeamento. Clicar no título do eixo para trocar, clicar em Flip para inverter. Manter Shift para editar ambos os lados simetricamente.
  - [ ] OK    Fix: 

- **`currentIP`**
  - EN: IP address of the Processor.
  - PT: Endereço IP do processador.
  - [ ] OK    Fix: 

- **`dataMode`**
  - EN: Select UDP or TCP data transmission.
  - PT: Selecionar transmissão de dados UDP ou TCP.
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Network Configuration to file.
  - PT: Exportar a configuração de rede para um ficheiro.
  - [ ] OK    Fix: 

- **`findMyRemote`**
  - EN: Make your Remote Flash and Buzz to Find it.
  - PT: Faça o seu comando piscar e vibrar para o encontrar.
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Network Configuration from file.
  - PT: Importar a configuração de rede a partir de um ficheiro.
  - [ ] OK    Fix: 

- **`networkInterface`**
  - EN: Select the Network Interface.
  - PT: Selecionar a interface de rede.
  - [ ] OK    Fix: 

- **`openLogWindow`**
  - EN: Open Network Logging window.
  - PT: Abrir a janela de registo de rede.
  - [ ] OK    Fix: 

- **`oscQueryEnable`**
  - EN: Enable/disable OSC Query server for automatic parameter discovery via HTTP/WebSocket.
  - PT: Ativar/desativar o servidor OSC Query para a descoberta automática de parâmetros via HTTP/WebSocket.
  - [ ] OK    Fix: 

- **`oscQueryPort`**
  - EN: HTTP port for OSC Query discovery. Other apps can browse parameters at http://localhost:<port>/
  - PT: Porta HTTP para a descoberta OSC Query. Outras aplicações podem navegar pelos parâmetros em http://localhost:<port>/
  - [ ] OK    Fix: 

- **`oscSourceFilter`**
  - EN: Filter incoming OSC: Accept All sources or only Registered connections with Rx enabled.
  - PT: Filtrar OSC recebido: aceitar todas as fontes ou apenas conexões registadas com Rx ativo.
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Select the Protocol: DISABLED, OSC, REMOTE, or ADM-OSC.
  - PT: Selecionar o protocolo: DISABLED, OSC, REMOTE ou ADM-OSC.
  - [ ] OK    Fix: 

- **`psnInterface`**
  - EN: Network interface for PSN multicast reception
  - PT: Interface de rede para a receção multicast PSN
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Network Configuration from backup file.
  - PT: Recarregar a configuração de rede a partir do ficheiro de cópia de segurança.
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Network Configuration from file.
  - PT: Recarregar a configuração de rede a partir de um ficheiro.
  - [ ] OK    Fix: 

- **`removeTarget`**
  - EN: Delete this Network Target.
  - PT: Eliminar este destino de rede.
  - [ ] OK    Fix: 

- **`rxEnable`**
  - EN: Enable or Disable Data Reception.
  - PT: Ativar ou desativar a receção de dados.
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Network Configuration to file.
  - PT: Guardar a configuração de rede num ficheiro.
  - [ ] OK    Fix: 

- **`targetIP`**
  - EN: IP Address of the Target (use 127.0.0.1 for local host).
  - PT: Endereço IP do destino (usar 127.0.0.1 para o host local).
  - [ ] OK    Fix: 

- **`targetName`**
  - EN: Network Target Name.
  - PT: Nome do destino de rede.
  - [ ] OK    Fix: 

- **`tcpPort`**
  - EN: TCP Receive Port of the Processor.
  - PT: Porta de receção TCP do processador.
  - [ ] OK    Fix: 

- **`trackingEnabled`**
  - EN: Enable or Disable Incoming Tracking data processing.
  - PT: Ativar ou desativar o processamento de dados de tracking recebidos.
  - [ ] OK    Fix: 

- **`trackingFlipX`**
  - EN: Invert Axis of Tracking X Coordinate.
  - PT: Inverter o eixo da coordenada X do tracking.
  - [ ] OK    Fix: 

- **`trackingFlipY`**
  - EN: Invert Axis of Tracking Y Coordinate.
  - PT: Inverter o eixo da coordenada Y do tracking.
  - [ ] OK    Fix: 

- **`trackingFlipZ`**
  - EN: Invert Axis of Tracking Z Coordinate.
  - PT: Inverter o eixo da coordenada Z do tracking.
  - [ ] OK    Fix: 

- **`trackingOffsetX`**
  - EN: Offset Tracking X Coordinate.
  - PT: Deslocamento da coordenada X do tracking.
  - [ ] OK    Fix: 

- **`trackingOffsetY`**
  - EN: Offset Tracking Y Coordinate.
  - PT: Deslocamento da coordenada Y do tracking.
  - [ ] OK    Fix: 

- **`trackingOffsetZ`**
  - EN: Offset Tracking Z Coordinate.
  - PT: Deslocamento da coordenada Z do tracking.
  - [ ] OK    Fix: 

- **`trackingOscPath`**
  - EN: OSC Path for the Tracking in OSC Mode (starts with a /)
  - PT: Caminho OSC para o tracking em modo OSC (começa com /)
  - [ ] OK    Fix: 

- **`trackingPort`**
  - EN: Specify the Port to receive Tracking data.
  - PT: Especificar a porta para receber dados de tracking.
  - [ ] OK    Fix: 

- **`trackingProtocol`**
  - EN: Select the type of Tracking Protocol.
  - PT: Selecionar o tipo de protocolo de tracking.
  - [ ] OK    Fix: 

- **`trackingScaleX`**
  - EN: Scale Tracking X Coordinate.
  - PT: Escala da coordenada X do tracking.
  - [ ] OK    Fix: 

- **`trackingScaleY`**
  - EN: Scale Tracking Y Coordinate.
  - PT: Escala da coordenada Y do tracking.
  - [ ] OK    Fix: 

- **`trackingScaleZ`**
  - EN: Scale Tracking Z Coordinate.
  - PT: Escala da coordenada Z do tracking.
  - [ ] OK    Fix: 

- **`txEnable`**
  - EN: Enable or Disable Data Transmission.
  - PT: Ativar ou desativar a transmissão de dados.
  - [ ] OK    Fix: 

- **`txPort`**
  - EN: Transmit Port for this Target.
  - PT: Porta de transmissão para este destino.
  - [ ] OK    Fix: 

- **`udpPort`**
  - EN: UDP Receive Port of the Processor.
  - PT: Porta de receção UDP do processador.
  - [ ] OK    Fix: 

## `network.labels`

- **`admMapping`**
  - EN: Mapping:
  - PT: Mapeamento:
  - [ ] OK    Fix: 

- **`currentIPv4`**
  - EN: Current IPv4:
  - PT: IPv4 Atual:
  - [ ] OK    Fix: 

- **`interface`**
  - EN: Network Interface:
  - PT: Interface de Rede:
  - [ ] OK    Fix: 

- **`localhost`**
  - EN: Localhost (127.0.0.1)
  - PT: Localhost (127.0.0.1)
  - [ ] OK    Fix: 

- **`mqttHost`**
  - EN: Host:
  - PT: Host:
  - [ ] OK    Fix: 

- **`mqttJsonQ`**
  - EN: Q:
  - PT: Q:
  - [ ] OK    Fix: 

- **`mqttJsonX`**
  - EN: X:
  - PT: X:
  - [ ] OK    Fix: 

- **`mqttJsonY`**
  - EN: Y:
  - PT: Y:
  - [ ] OK    Fix: 

- **`mqttJsonZ`**
  - EN: Z:
  - PT: Z:
  - [ ] OK    Fix: 

- **`mqttTagIds`**
  - EN: Tag IDs...
  - PT: IDs Tag...
  - [ ] OK    Fix: 

- **`mqttTopic`**
  - EN: Topic:
  - PT: Tópico:
  - [ ] OK    Fix: 

- **`notAvailable`**
  - EN: Not available
  - PT: Não disponível
  - [ ] OK    Fix: 

- **`offsetX`**
  - EN: Offset X:
  - PT: Deslocamento X:
  - [ ] OK    Fix: 

- **`offsetY`**
  - EN: Offset Y:
  - PT: Deslocamento Y:
  - [ ] OK    Fix: 

- **`offsetZ`**
  - EN: Offset Z:
  - PT: Deslocamento Z:
  - [ ] OK    Fix: 

- **`oscPath`**
  - EN: OSC Path:
  - PT: OSC Path:
  - [ ] OK    Fix: 

- **`oscQuery`**
  - EN: OSC Query:
  - PT: OSC Query:
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Protocol:
  - PT: Protocolo:
  - [ ] OK    Fix: 

- **`psnInterface`**
  - EN: PSN Interface:
  - PT: PSN Interface:
  - [ ] OK    Fix: 

- **`rxPort`**
  - EN: Rx Port:
  - PT: Porta Rx:
  - [ ] OK    Fix: 

- **`scaleX`**
  - EN: Scale X:
  - PT: Escala X:
  - [ ] OK    Fix: 

- **`scaleY`**
  - EN: Scale Y:
  - PT: Escala Y:
  - [ ] OK    Fix: 

- **`scaleZ`**
  - EN: Scale Z:
  - PT: Escala Z:
  - [ ] OK    Fix: 

- **`tcpPort`**
  - EN: TCP Port:
  - PT: Porta TCP:
  - [ ] OK    Fix: 

- **`udpPort`**
  - EN: UDP Port:
  - PT: Porta UDP:
  - [ ] OK    Fix: 

## `network.messages`

- **`configExported`**
  - EN: Network configuration exported.
  - PT: Configuração de rede exportada.
  - [ ] OK    Fix: 

- **`configImported`**
  - EN: Network configuration imported.
  - PT: Configuração de rede importada.
  - [ ] OK    Fix: 

- **`configLoadedFromBackup`**
  - EN: Network configuration loaded from backup.
  - PT: Configuração de rede carregada do backup.
  - [ ] OK    Fix: 

- **`configNotFound`**
  - EN: Network config file not found.
  - PT: Ficheiro de configuração de rede não encontrado.
  - [ ] OK    Fix: 

- **`configReloaded`**
  - EN: Network configuration reloaded.
  - PT: Configuração de rede recarregada.
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Network configuration saved.
  - PT: Configuração de rede guardada.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - PT: Erro: {error}
  - [ ] OK    Fix: 

- **`findDeviceSent`**
  - EN: Find Device command sent.
  - PT: Comando Encontrar dispositivo enviado.
  - [ ] OK    Fix: 

- **`maxTargetsReached`**
  - EN: Maximum Number of Targets/Servers Reached.
  - PT: Número máximo de destinos/servidores atingido.
  - [ ] OK    Fix: 

- **`noBackupFound`**
  - EN: No backup files found.
  - PT: Nenhum ficheiro de backup encontrado.
  - [ ] OK    Fix: 

- **`onlyOneRemote`**
  - EN: Only one REMOTE connection is allowed.
  - PT: Apenas uma ligação REMOTA é permitida.
  - [ ] OK    Fix: 

- **`oscManagerError`**
  - EN: Error: OSC Manager not available
  - PT: Erro: gestor OSC não disponível
  - [ ] OK    Fix: 

- **`passwordEmpty`**
  - EN: Password cannot be empty.
  - PT: A palavra-passe não pode estar vazia.
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - PT: Por favor, selecione primeiro uma pasta de projeto em Config. Sistema.
  - [ ] OK    Fix: 

## `network.protocols`

- **`admOsc`**
  - EN: ADM-OSC
  - PT: ADM-OSC
  - [ ] OK    Fix: 

- **`disabled`**
  - EN: DISABLED
  - PT: DESATIVADO
  - [ ] OK    Fix: 

- **`mqtt`**
  - EN: MQTT
  - PT: MQTT
  - [ ] OK    Fix: 

- **`osc`**
  - EN: OSC
  - PT: OSC
  - [ ] OK    Fix: 

- **`psn`**
  - EN: PosiStageNet (PSN)
  - PT: PosiStageNet (PSN)
  - [ ] OK    Fix: 

- **`qlab`**
  - EN: QLab
  - PT: QLab
  - [ ] OK    Fix: 

- **`remote`**
  - EN: Remote
  - PT: Remoto
  - [ ] OK    Fix: 

- **`rttrp`**
  - EN: RTTrP
  - PT: RTTrP
  - [ ] OK    Fix: 

- **`tcp`**
  - EN: TCP
  - PT: TCP
  - [ ] OK    Fix: 

- **`udp`**
  - EN: UDP
  - PT: UDP
  - [ ] OK    Fix: 

## `network.sections`

- **`admOsc`**
  - EN: ADM-OSC Mappings
  - PT: Mapeamentos ADM-OSC
  - [ ] OK    Fix: 

- **`connections`**
  - EN: Network Connections
  - PT: Ligações de Rede
  - [ ] OK    Fix: 

- **`network`**
  - EN: Network
  - PT: Rede
  - [ ] OK    Fix: 

- **`tracking`**
  - EN: Tracking
  - PT: Rastreamento
  - [ ] OK    Fix: 

## `network.table`

- **`defaultTarget`**
  - EN: Target {num}
  - PT: Destino {num}
  - [ ] OK    Fix: 

- **`ipv4Address`**
  - EN: IPv4 Address
  - PT: Endereço IPv4
  - [ ] OK    Fix: 

- **`mode`**
  - EN: Mode
  - PT: Modo
  - [ ] OK    Fix: 

- **`name`**
  - EN: Name
  - PT: Nome
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Protocol
  - PT: Protocolo
  - [ ] OK    Fix: 

- **`rx`**
  - EN: Rx
  - PT: Rx
  - [ ] OK    Fix: 

- **`tx`**
  - EN: Tx
  - PT: Tx
  - [ ] OK    Fix: 

- **`txPort`**
  - EN: Tx Port
  - PT: Porta Tx
  - [ ] OK    Fix: 

## `network.toggles`

- **`disabled`**
  - EN: Disabled
  - PT: Desativado
  - [ ] OK    Fix: 

- **`enabled`**
  - EN: Enabled
  - PT: Ativado
  - [ ] OK    Fix: 

- **`flipXOff`**
  - EN: Flip X: OFF
  - PT: Inverter X: DESLIGADO
  - [ ] OK    Fix: 

- **`flipXOn`**
  - EN: Flip X: ON
  - PT: Inverter X: LIGADO
  - [ ] OK    Fix: 

- **`flipYOff`**
  - EN: Flip Y: OFF
  - PT: Inverter Y: DESLIGADO
  - [ ] OK    Fix: 

- **`flipYOn`**
  - EN: Flip Y: ON
  - PT: Inverter Y: LIGADO
  - [ ] OK    Fix: 

- **`flipZOff`**
  - EN: Flip Z: OFF
  - PT: Inverter Z: DESLIGADO
  - [ ] OK    Fix: 

- **`flipZOn`**
  - EN: Flip Z: ON
  - PT: Inverter Z: LIGADO
  - [ ] OK    Fix: 

- **`off`**
  - EN: OFF
  - PT: DESLIGADO
  - [ ] OK    Fix: 

- **`on`**
  - EN: ON
  - PT: LIGADO
  - [ ] OK    Fix: 

- **`oscFilterAcceptAll`**
  - EN: OSC Filter: Accept All
  - PT: Filtro OSC: Aceitar Todos
  - [ ] OK    Fix: 

- **`oscFilterRegisteredOnly`**
  - EN: OSC Filter: Registered Only
  - PT: Filtro OSC: Apenas Registados
  - [ ] OK    Fix: 

- **`trackingOff`**
  - EN: Tracking: OFF
  - PT: Rastreamento: DESLIGADO
  - [ ] OK    Fix: 

- **`trackingOn`**
  - EN: Tracking: ON
  - PT: Rastreamento: LIGADO
  - [ ] OK    Fix: 

## `networkLog`

- **`windowTitle`**
  - EN: Network Log
  - PT: Registo de Rede
  - [ ] OK    Fix: 

## `networkLog.columns`

- **`address`**
  - EN: Address
  - PT: Endereço
  - [ ] OK    Fix: 

- **`arguments`**
  - EN: Arguments
  - PT: Argumentos
  - [ ] OK    Fix: 

- **`direction`**
  - EN: Dir
  - PT: Dir
  - [ ] OK    Fix: 

- **`ip`**
  - EN: IP
  - PT: IP
  - [ ] OK    Fix: 

- **`origin`**
  - EN: Origin
  - PT: Origem
  - [ ] OK    Fix: 

- **`port`**
  - EN: Port
  - PT: Porta
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Protocol
  - PT: Protocolo
  - [ ] OK    Fix: 

- **`time`**
  - EN: Time
  - PT: Hora
  - [ ] OK    Fix: 

- **`transport`**
  - EN: Trans
  - PT: Trans
  - [ ] OK    Fix: 

## `networkLog.controls`

- **`clear`**
  - EN: CLEAR
  - PT: LIMPAR
  - [ ] OK    Fix: 

- **`export`**
  - EN: EXPORT
  - PT: EXPORTAR
  - [ ] OK    Fix: 

- **`hideHeartbeat`**
  - EN: Hide Heartbeat
  - PT: Ocultar Heartbeat
  - [ ] OK    Fix: 

- **`logging`**
  - EN: Logging
  - PT: Registo
  - [ ] OK    Fix: 

## `networkLog.dialogs`

- **`exportCompleteMessage`**
  - EN: Log exported to: {path}
  - PT: Registo exportado para: {path}
  - [ ] OK    Fix: 

- **`exportCompleteTitle`**
  - EN: Export Complete
  - PT: Exportação Completa
  - [ ] OK    Fix: 

- **`exportFailedMessage`**
  - EN: Could not write to file: {path}
  - PT: Não foi possível escrever no ficheiro: {path}
  - [ ] OK    Fix: 

- **`exportFailedTitle`**
  - EN: Export Failed
  - PT: Exportação Falhou
  - [ ] OK    Fix: 

## `networkLog.exportMenu`

- **`exportAll`**
  - EN: Export All
  - PT: Exportar Tudo
  - [ ] OK    Fix: 

- **`exportFiltered`**
  - EN: Export Filtered
  - PT: Exportar Filtrado
  - [ ] OK    Fix: 

## `networkLog.filterModes`

- **`clientIp`**
  - EN: Client IP
  - PT: IP do Cliente
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Protocol
  - PT: Protocolo
  - [ ] OK    Fix: 

- **`rejected`**
  - EN: Rejected
  - PT: Rejeitado
  - [ ] OK    Fix: 

- **`tcpUdp`**
  - EN: TCP/UDP
  - PT: TCP/UDP
  - [ ] OK    Fix: 

## `networkLog.filterToggles`

- **`incoming`**
  - EN: Incoming
  - PT: Entrada
  - [ ] OK    Fix: 

- **`outgoing`**
  - EN: Outgoing
  - PT: Saída
  - [ ] OK    Fix: 

- **`tcp`**
  - EN: TCP
  - PT: TCP
  - [ ] OK    Fix: 

- **`udp`**
  - EN: UDP
  - PT: UDP
  - [ ] OK    Fix: 

## `networkLog.status`

- **`rejected`**
  - EN: REJECTED
  - PT: REJEITADO
  - [ ] OK    Fix: 

## `outputs.arrayModes`

- **`absolute`**
  - EN: ABSOLUTE
  - PT: ABSOLUTO
  - [ ] OK    Fix: 

- **`array`**
  - EN: Array
  - PT: Array
  - [ ] OK    Fix: 

- **`off`**
  - EN: OFF
  - PT: DESLIGADO
  - [ ] OK    Fix: 

- **`relative`**
  - EN: RELATIVE
  - PT: RELATIVO
  - [ ] OK    Fix: 

- **`single`**
  - EN: Single
  - PT: Individual
  - [ ] OK    Fix: 

## `outputs.buttons`

- **`arrayHidden`**
  - EN: Array Hidden on Map
  - PT: Array oculto no mapa
  - [ ] OK    Fix: 

- **`arrayVisible`**
  - EN: Array Visible on Map
  - PT: Array visível no mapa
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - PT: Exportar
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - PT: Importar
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Backup
  - PT: Recarregar Backup
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Output Config
  - PT: Recarregar Config. Saída
  - [ ] OK    Fix: 

- **`speakerHidden`**
  - EN: Speaker Hidden on Map
  - PT: Altifalante oculto no mapa
  - [ ] OK    Fix: 

- **`speakerVisible`**
  - EN: Speaker Visible on Map
  - PT: Altifalante Visível no Mapa
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Output Config
  - PT: Guardar Config. Saída
  - [ ] OK    Fix: 

- **`wizardOfOutZ`**
  - EN: Wizard of OutZ...
  - PT: Assistente OutZ...
  - [ ] OK    Fix: 

## `outputs.coordModes`

- **`xyz`**
  - EN: XYZ
  - PT: XYZ
  - [ ] OK    Fix: 

## `outputs.dialogs`

- **`export`**
  - EN: Export Output Configuration
  - PT: Exportar Configuração de Saída
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import Output Configuration
  - PT: Importar Configuração de Saída
  - [ ] OK    Fix: 

## `outputs.help`

- **`angleOff`**
  - EN: Output Channel Will Not Amplify Objects in this Angle in Front of it. (changes may affect the rest of the array)
  - PT: O canal de saída não amplificará objetos neste ângulo à frente dele. (as alterações podem afetar o resto do array)
  - [ ] OK    Fix: 

- **`angleOn`**
  - EN: Output Channel Will Amplify Objects in this Angle in its Back. (changes may affect the rest of the array)
  - PT: O canal de saída amplificará objetos neste ângulo atrás dele. (as alterações podem afetar o resto do array)
  - [ ] OK    Fix: 

- **`applyToArray`**
  - EN: Apply Changes to the rest of the Array (Absolute value or Relative changes).
  - PT: Aplicar alterações ao resto do array (valor absoluto ou alterações relativas).
  - [ ] OK    Fix: 

- **`arraySelector`**
  - EN: Selected Output Channel is part of Array.
  - PT: O canal de saída selecionado faz parte de um array.
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Output Channel Attenuation. (changes may affect the rest of the array)
  - PT: Atenuação do canal de saída. (as alterações podem afetar o resto do array)
  - [ ] OK    Fix: 

- **`channelSelector`**
  - EN: Output Channel Number and Selection.
  - PT: Numero e seleção do canal de saída.
  - [ ] OK    Fix: 

- **`coordMode`**
  - EN: Coordinate display mode: Cartesian (X/Y/Z), Cylindrical (radius/azimuth/height), or Spherical (radius/azimuth/elevation).
  - PT: Modo de exibição de coordenadas: cartesiano (X/Y/Z), cilíndrico (raio/azimute/altura) ou esférico (raio/azimute/elevação).
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Output Channel Delay (positive values) or Latency Compensation (negative values). (changes may affect the rest of the array)
  - PT: Atraso do canal de saída (valores positivos) ou compensação de latência (valores negativos). (as alterações podem afetar o resto do array)
  - [ ] OK    Fix: 

- **`directional`**
  - EN: Output Channel Directional Control. Drag to change orientation, Shift+drag for Angle Off, Alt+drag for Angle On. (changes may affect the rest of the array)
  - PT: Controlo direcional do canal de saída. Arrastar para alterar orientação, Shift+arrastar para Angle Off, Alt+arrastar para Angle On. (as alterações podem afetar o resto do array)
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Ratio of Distance Attenuation for Selected Output. (changes may affect the rest of the array)
  - PT: Proporção de atenuação por distância para a saída selecionada. (as alterações podem afetar o resto do array)
  - [ ] OK    Fix: 

- **`eqBandToggle`**
  - EN: Toggle Band {band} on/off. When off the band is bypassed.
  - PT: Ativar/desativar a banda {band}. Desativada, a banda é ignorada.
  - [ ] OK    Fix: 

- **`eqEnable`**
  - EN: Enable or disable EQ processing for this output.
  - PT: Ativar ou desativar o EQ para esta saída.
  - [ ] OK    Fix: 

- **`eqFlatten`**
  - EN: Long press to reset all EQ bands to their default values.
  - PT: Pressão longa para redefinir todas as bandas EQ.
  - [ ] OK    Fix: 

- **`eqFreq`**
  - EN: Output EQ Band {band} frequency (20 Hz - 20 kHz).
  - PT: Frequência EQ saída banda {band} (20 Hz - 20 kHz).
  - [ ] OK    Fix: 

- **`eqGain`**
  - EN: Output EQ Band {band} gain (-24 to +24 dB).
  - PT: Ganho EQ saída banda {band} (-24 a +24 dB).
  - [ ] OK    Fix: 

- **`eqQ`**
  - EN: Output EQ Band {band} Q factor (0.1 - 10.0).
  - PT: Fator Q EQ saída banda {band} (0.1 - 10.0).
  - [ ] OK    Fix: 

- **`eqResetBand`**
  - EN: Long press to reset Band {band} to its default values.
  - PT: Pressão longa para redefinir a banda {band}.
  - [ ] OK    Fix: 

- **`eqShape`**
  - EN: Output EQ Band {band} filter shape.
  - PT: Forma do filtro EQ saída banda {band}.
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Output Configuration to file (with file explorer window).
  - PT: Exportar a configuração de saída para ficheiro (com janela do explorador).
  - [ ] OK    Fix: 

- **`floorReflections`**
  - EN: Enable or Disable the Floor Reflections for this Speaker.
  - PT: Ativar ou desativar as reflexões do chão para este altifalante.
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: Loss of High Frequency Depending on Distance from Object to Output. (changes may affect the rest of the array)
  - PT: Perda de altas frequências em função da distância do objeto à saída. (as alterações podem afetar o resto do array)
  - [ ] OK    Fix: 

- **`hParallax`**
  - EN: Horizontal Distance from Speaker to 'Targeted' Listener. (changes may affect the rest of the array)
  - PT: Distância horizontal do altifalante ao ouvinte « alvo ». (as alterações podem afetar o resto do array)
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Output Configuration from file (with file explorer window).
  - PT: Importar a configuração de saída a partir de ficheiro (com janela do explorador).
  - [ ] OK    Fix: 

- **`liveSource`**
  - EN: Disables Live Source Attenuation for Selected Output. (changes may affect the rest of the array)
  - PT: Desativa a atenuação de fonte ao vivo para a saída selecionada. (as alterações podem afetar o resto do array)
  - [ ] OK    Fix: 

- **`mapVisibility`**
  - EN: Make Visible or Hide The Selected Output on the Map.
  - PT: Tornar visível ou ocultar a saída selecionada no mapa.
  - [ ] OK    Fix: 

- **`minLatency`**
  - EN: Disables Minimal Latency Mode for Selected Output. (changes may affect the rest of the array)
  - PT: Desativa o modo de latência mínima para a saída selecionada. (as alterações podem afetar o resto do array)
  - [ ] OK    Fix: 

- **`nameEditor`**
  - EN: Displayed Output Channel Name (editable).
  - PT: Nome exibido do canal de saída (editável).
  - [ ] OK    Fix: 

- **`pitch`**
  - EN: Output Channel Vertical Orientation used to Determine which Objects get amplified. (changes may affect the rest of the array)
  - PT: Orientação vertical do canal de saída usada para determinar quais objetos são amplificados. (as alterações podem afetar o resto do array)
  - [ ] OK    Fix: 

- **`position1`**
  - EN: Output Channel {name} ({unit}).
  - PT: Canal de saída {name} ({unit}).
  - [ ] OK    Fix: 

- **`position2`**
  - EN: Output Channel {name} ({unit}).
  - PT: Canal de saída {name} ({unit}).
  - [ ] OK    Fix: 

- **`position3`**
  - EN: Output Channel {name} ({unit}).
  - PT: Canal de saída {name} ({unit}).
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Output Configuration from backup file.
  - PT: Recarregar a configuração de saída a partir do ficheiro de backup.
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Output Configuration from file.
  - PT: Recarregar a configuração de saída a partir do ficheiro.
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Output Configuration to file (with backup).
  - PT: Guardar a configuração de saída em ficheiro (com backup).
  - [ ] OK    Fix: 

- **`vParallax`**
  - EN: Vertical Distance from Speaker to 'Targeted' Listener. Positive when the Speaker is Below the head of the Listener. (changes may affect the rest of the array)
  - PT: Distância vertical do altifalante ao ouvinte « alvo ». Positivo quando o altifalante está abaixo da cabeça do ouvinte. (as alterações podem afetar o resto do array)
  - [ ] OK    Fix: 

- **`wizardOfOutZ`**
  - EN: Open Wizard of OutZ to Position Speaker Arrays Conveniently.
  - PT: Abrir o Wizard of OutZ para posicionar arrays de altifalantes convenientemente.
  - [ ] OK    Fix: 

## `outputs.labels`

- **`angleOff`**
  - EN: Angle Off:
  - PT: Ângulo Desligado:
  - [ ] OK    Fix: 

- **`angleOn`**
  - EN: Angle On:
  - PT: Ângulo Ligado:
  - [ ] OK    Fix: 

- **`applyToArray`**
  - EN: Apply to Array:
  - PT: Aplicar ao Array:
  - [ ] OK    Fix: 

- **`array`**
  - EN: Array:
  - PT: Array:
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Attenuation:
  - PT: Atenuação:
  - [ ] OK    Fix: 

- **`coordinates`**
  - EN: Coordinates:
  - PT: Coordenadas:
  - [ ] OK    Fix: 

- **`delay`**
  - EN: Delay:
  - PT: Atraso:
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Delay/Latency:
  - PT: Atraso/Latência:
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Distance Atten:
  - PT: Aten. Distância:
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: HF Damping:
  - PT: Amortecimento HF:
  - [ ] OK    Fix: 

- **`hParallax`**
  - EN: Horizontal Parallax:
  - PT: Paralaxe Horiz.:
  - [ ] OK    Fix: 

- **`latency`**
  - EN: Latency:
  - PT: Latência:
  - [ ] OK    Fix: 

- **`name`**
  - EN: Name:
  - PT: Nome:
  - [ ] OK    Fix: 

- **`orientation`**
  - EN: Orientation:
  - PT: Orientação:
  - [ ] OK    Fix: 

- **`pitch`**
  - EN: Pitch:
  - PT: Inclinação:
  - [ ] OK    Fix: 

- **`positionX`**
  - EN: Position X:
  - PT: Posição X:
  - [ ] OK    Fix: 

- **`positionY`**
  - EN: Position Y:
  - PT: Posição Y:
  - [ ] OK    Fix: 

- **`positionZ`**
  - EN: Position Z:
  - PT: Posição Z:
  - [ ] OK    Fix: 

- **`vParallax`**
  - EN: Vertical Parallax:
  - PT: Paralaxe Vert.:
  - [ ] OK    Fix: 

## `outputs.messages`

- **`assignedToArray`**
  - EN: Output {num} assigned to Array {array}
  - PT: Saída {num} atribuída ao Array {array}
  - [ ] OK    Fix: 

- **`backupLoaded`**
  - EN: Output configuration loaded from backup.
  - PT: Configuração de saída carregada do backup.
  - [ ] OK    Fix: 

- **`configExported`**
  - EN: Output configuration exported.
  - PT: Configuração de saída exportada.
  - [ ] OK    Fix: 

- **`configImported`**
  - EN: Output configuration imported.
  - PT: Configuração de saída importada.
  - [ ] OK    Fix: 

- **`configLoaded`**
  - EN: Output configuration loaded.
  - PT: Configuração de saída carregada.
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Output configuration saved.
  - PT: Configuração de saída guardada.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - PT: Erro: {error}
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - PT: Por favor, selecione primeiro uma pasta de projeto em Config. Sistema.
  - [ ] OK    Fix: 

- **`setToSingle`**
  - EN: Output {num} set to Single
  - PT: Saída {num} definida como Individual
  - [ ] OK    Fix: 

## `outputs.tabs`

- **`eq`**
  - EN: Output EQ
  - PT: EQ de Saída
  - [ ] OK    Fix: 

- **`parameters`**
  - EN: Output Parameters
  - PT: Parâmetros de Saída
  - [ ] OK    Fix: 

## `outputs.toggles`

- **`floorReflectionsOff`**
  - EN: Floor Reflections: OFF
  - PT: Reflexões do Chão: DESLIGADO
  - [ ] OK    Fix: 

- **`floorReflectionsOn`**
  - EN: Floor Reflections: ON
  - PT: Reflexões do Chão: LIGADO
  - [ ] OK    Fix: 

- **`liveSourceOff`**
  - EN: Live Source Atten: OFF
  - PT: Aten. Fonte Ao Vivo: DESLIGADO
  - [ ] OK    Fix: 

- **`liveSourceOn`**
  - EN: Live Source Atten: ON
  - PT: Aten. Fonte Ao Vivo: LIGADO
  - [ ] OK    Fix: 

- **`minLatencyOff`**
  - EN: Minimal Latency: OFF
  - PT: Latência Mínima: DESLIGADO
  - [ ] OK    Fix: 

- **`minLatencyOn`**
  - EN: Minimal Latency: ON
  - PT: Latência Mínima: LIGADO
  - [ ] OK    Fix: 

## `outputs.units`

- **`meters`**
  - EN: m
  - PT: m
  - [ ] OK    Fix: 

- **`ms`**
  - EN: ms
  - PT: ms
  - [ ] OK    Fix: 

## `reverbs`

- **`noChannels`**
  - EN: No reverb channels configured.

Set the number of Reverb Channels in System Config.
  - PT: Nenhum canal de reverberação configurado.

Defina o número de Canais de Reverb em Config. Sistema.
  - [ ] OK    Fix: 

## `reverbs.algorithm`

- **`crossoverHigh`**
  - EN: Crossover High:
  - PT: Crossover agudo:
  - [ ] OK    Fix: 

- **`crossoverLow`**
  - EN: Crossover Low:
  - PT: Crossover grave:
  - [ ] OK    Fix: 

- **`decaySection`**
  - EN: Decay
  - PT: Decaimento
  - [ ] OK    Fix: 

- **`diffusion`**
  - EN: Diffusion:
  - PT: Difusão:
  - [ ] OK    Fix: 

- **`fdn`**
  - EN: FDN
  - PT: FDN
  - [ ] OK    Fix: 

- **`fdnSection`**
  - EN: FDN
  - PT: FDN
  - [ ] OK    Fix: 

- **`ir`**
  - EN: IR
  - PT: IR
  - [ ] OK    Fix: 

- **`irFile`**
  - EN: IR File:
  - PT: Ficheiro IR:
  - [ ] OK    Fix: 

- **`irImport`**
  - EN: Import IR...
  - PT: Importar IR...
  - [ ] OK    Fix: 

- **`irImportSuccess`**
  - EN: IR imported: {file}
  - PT: IR importado: {file}
  - [ ] OK    Fix: 

- **`irLength`**
  - EN: IR Length:
  - PT: Comprimento IR:
  - [ ] OK    Fix: 

- **`irNoProject`**
  - EN: Set a project folder first
  - PT: Defina uma pasta de projeto primeiro
  - [ ] OK    Fix: 

- **`irSection`**
  - EN: IR
  - PT: IR
  - [ ] OK    Fix: 

- **`irTrim`**
  - EN: IR Trim:
  - PT: Corte IR:
  - [ ] OK    Fix: 

- **`noFileLoaded`**
  - EN: No IR loaded
  - PT: Nenhum IR carregado
  - [ ] OK    Fix: 

- **`perNodeOff`**
  - EN: Per-node IR OFF
  - PT: IR por nó DESLIGADO
  - [ ] OK    Fix: 

- **`perNodeOn`**
  - EN: Per-node IR ON
  - PT: IR por nó LIGADO
  - [ ] OK    Fix: 

- **`rt60`**
  - EN: RT60:
  - PT: RT60:
  - [ ] OK    Fix: 

- **`rt60HighMult`**
  - EN: RT60 High ×:
  - PT: RT60 Agudo ×:
  - [ ] OK    Fix: 

- **`rt60LowMult`**
  - EN: RT60 Low ×:
  - PT: RT60 Grave ×:
  - [ ] OK    Fix: 

- **`scale`**
  - EN: Scale:
  - PT: Escala:
  - [ ] OK    Fix: 

- **`sdn`**
  - EN: SDN
  - PT: SDN
  - [ ] OK    Fix: 

- **`sdnSection`**
  - EN: SDN
  - PT: SDN
  - [ ] OK    Fix: 

- **`size`**
  - EN: Size:
  - PT: Tamanho:
  - [ ] OK    Fix: 

- **`wetLevel`**
  - EN: Wet Level:
  - PT: Nível Wet:
  - [ ] OK    Fix: 

## `reverbs.buttons`

- **`editOnMap`**
  - EN: Edit on Map
  - PT: Editar no mapa
  - [ ] OK    Fix: 

- **`editOnMapOn`**
  - EN: Edit on Map ON
  - PT: Editar no mapa ON
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - PT: Exportar
  - [ ] OK    Fix: 

- **`hiddenOnMap`**
  - EN: Reverbs Hidden on Map
  - PT: Reverbs Ocultos no Mapa
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - PT: Importar
  - [ ] OK    Fix: 

- **`mutePost`**
  - EN: Mute Post
  - PT: Mute Post
  - [ ] OK    Fix: 

- **`mutePostOn`**
  - EN: Mute Post ON
  - PT: Mute Pós ON
  - [ ] OK    Fix: 

- **`mutePre`**
  - EN: Mute Pre
  - PT: Mute Pre
  - [ ] OK    Fix: 

- **`mutePreOn`**
  - EN: Mute Pre ON
  - PT: Mute Pré ON
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Backup
  - PT: Recarregar Backup
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Reverb Config
  - PT: Recarregar Config. Reverb
  - [ ] OK    Fix: 

- **`soloReverbs`**
  - EN: Solo Reverbs
  - PT: Solo Reverbs
  - [ ] OK    Fix: 

- **`soloReverbsOn`**
  - EN: Solo Reverbs ON
  - PT: Solo Reverberações ON
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Reverb Config
  - PT: Guardar Config. Reverb
  - [ ] OK    Fix: 

- **`visibleOnMap`**
  - EN: Reverbs Visible on Map
  - PT: Reverbs Visíveis no Mapa
  - [ ] OK    Fix: 

## `reverbs.coordModes`

- **`xyz`**
  - EN: XYZ
  - PT: XYZ
  - [ ] OK    Fix: 

## `reverbs.dialogs`

- **`export`**
  - EN: Export Reverb Configuration
  - PT: Exportar Configuração de Reverb
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import Reverb Configuration
  - PT: Importar Configuração de Reverb
  - [ ] OK    Fix: 

## `reverbs.help`

- **`algoCrossoverHigh`**
  - EN: High crossover frequency for 3-band decay (1 - 10 kHz).
  - PT: Frequência de crossover agudo para decaimento de 3 bandas (1 - 10 kHz).
  - [ ] OK    Fix: 

- **`algoCrossoverLow`**
  - EN: Low crossover frequency for 3-band decay (50 - 500 Hz).
  - PT: Frequência de crossover grave para decaimento de 3 bandas (50 - 500 Hz).
  - [ ] OK    Fix: 

- **`algoDiffusion`**
  - EN: Diffusion amount controlling echo density (0 - 100%).
  - PT: Quantidade de difusão que controla a densidade de ecos (0 - 100%).
  - [ ] OK    Fix: 

- **`algoFDN`**
  - EN: Select FDN (Feedback Delay Network) reverb algorithm.
  - PT: Selecionar algoritmo de reverberação FDN (Feedback Delay Network).
  - [ ] OK    Fix: 

- **`algoFDNSize`**
  - EN: FDN delay line size multiplier (0.5 - 2.0x).
  - PT: Multiplicador do tamanho da linha de atraso FDN (0.5 - 2.0x).
  - [ ] OK    Fix: 

- **`algoIR`**
  - EN: Select IR (Impulse Response convolution) reverb algorithm.
  - PT: Selecionar algoritmo de reverberação IR (convolução por resposta impulsiva).
  - [ ] OK    Fix: 

- **`algoIRFile`**
  - EN: Select or import an impulse response file for convolution.
  - PT: Selecionar ou importar um ficheiro de resposta impulsiva para convolução.
  - [ ] OK    Fix: 

- **`algoIRLength`**
  - EN: Maximum impulse response length (0.1 - 6.0 seconds).
  - PT: Comprimento máximo da resposta impulsiva (0.1 - 6.0 segundos).
  - [ ] OK    Fix: 

- **`algoIRTrim`**
  - EN: Trim the start of the impulse response (0 - 100 ms).
  - PT: Cortar o início da resposta impulsiva (0 - 100 ms).
  - [ ] OK    Fix: 

- **`algoPerNode`**
  - EN: Use a separate IR for each reverb node, or share one IR.
  - PT: Usar uma IR separada para cada nó de reverberação, ou partilhar uma única IR.
  - [ ] OK    Fix: 

- **`algoRT60`**
  - EN: Reverb decay time RT60 (0.2 - 8.0 seconds).
  - PT: Tempo de decaimento da reverberação RT60 (0.2 - 8.0 segundos).
  - [ ] OK    Fix: 

- **`algoRT60HighMult`**
  - EN: High frequency RT60 multiplier (0.1 - 9.0x).
  - PT: Multiplicador RT60 de alta frequência (0.1 - 9.0x).
  - [ ] OK    Fix: 

- **`algoRT60LowMult`**
  - EN: Low frequency RT60 multiplier (0.1 - 9.0x).
  - PT: Multiplicador RT60 de baixa frequência (0.1 - 9.0x).
  - [ ] OK    Fix: 

- **`algoSDN`**
  - EN: Select SDN (Scattering Delay Network) reverb algorithm.
  - PT: Selecionar algoritmo de reverberação SDN (Scattering Delay Network).
  - [ ] OK    Fix: 

- **`algoSDNScale`**
  - EN: SDN inter-node delay scale factor (0.5 - 4.0x).
  - PT: Fator de escala de atraso entre nós SDN (0.5 - 4.0x).
  - [ ] OK    Fix: 

- **`algoWetLevel`**
  - EN: Wet/dry mix level for reverb output (-60 to +12 dB).
  - PT: Nível de mistura wet/dry para a saída de reverberação (-60 a +12 dB).
  - [ ] OK    Fix: 

- **`angleOff`**
  - EN: Angle at which no amplification occurs (0-179 degrees).
  - PT: Ângulo no qual nenhuma amplificação ocorre (0-179 graus).
  - [ ] OK    Fix: 

- **`angleOn`**
  - EN: Angle at which amplification starts (1-180 degrees).
  - PT: Ângulo no qual a amplificação começa (1-180 graus).
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Reverb channel attenuation (-92 to 0 dB).
  - PT: Atenuação do canal de reverberação (-92 a 0 dB).
  - [ ] OK    Fix: 

- **`channelSelector`**
  - EN: Reverb Channel Number and Selection.
  - PT: Número e seleção do canal de reverberação.
  - [ ] OK    Fix: 

- **`commonAtten`**
  - EN: Common attenuation percentage (0-100%).
  - PT: Percentagem de atenuação comum (0-100%).
  - [ ] OK    Fix: 

- **`coordMode`**
  - EN: Coordinate display mode: Cartesian (X/Y/Z), Cylindrical (radius/azimuth/height), or Spherical (radius/azimuth/elevation).
  - PT: Modo de exibição de coordenadas: Cartesiano (X/Y/Z), Cilíndrico (raio/azimute/altura), ou Esférico (raio/azimute/elevação).
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Reverb delay/latency compensation (-100 to +100 ms).
  - PT: Compensação de atraso/latência da reverberação (-100 a +100 ms).
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Distance attenuation for reverb return (-6.0 to 0.0 dB/m).
  - PT: Atenuação por distância para o retorno de reverberação (-6.0 a 0.0 dB/m).
  - [ ] OK    Fix: 

- **`distanceAttenEnable`**
  - EN: Distance attenuation percentage (0-200%).
  - PT: Percentagem de atenuação por distância (0-200%).
  - [ ] OK    Fix: 

- **`eqBandToggle`**
  - EN: Toggle pre-EQ Band {band} on/off. When off the band is bypassed.
  - PT: Ativar/desativar a banda pré-EQ {band}.
  - [ ] OK    Fix: 

- **`eqEnable`**
  - EN: Enable or disable EQ processing for this reverb.
  - PT: Ativar ou desativar o processamento EQ para esta reverberação.
  - [ ] OK    Fix: 

- **`eqFlatten`**
  - EN: Long press to reset all pre-EQ bands to their default values.
  - PT: Pressão longa para redefinir todas as bandas pré-EQ.
  - [ ] OK    Fix: 

- **`eqFreq`**
  - EN: Pre-EQ Band {band} frequency (20 Hz - 20 kHz).
  - PT: Frequência pré-EQ banda {band} (20 Hz - 20 kHz).
  - [ ] OK    Fix: 

- **`eqGain`**
  - EN: Pre-EQ Band {band} gain (-24 to +24 dB).
  - PT: Ganho pré-EQ banda {band} (-24 a +24 dB).
  - [ ] OK    Fix: 

- **`eqQ`**
  - EN: Pre-EQ Band {band} Q factor (0.1 - 20.0).
  - PT: Fator Q pré-EQ banda {band} (0.1 - 20.0).
  - [ ] OK    Fix: 

- **`eqResetBand`**
  - EN: Long press to reset pre-EQ Band {band} to its default values.
  - PT: Pressão longa para redefinir a banda pré-EQ {band}.
  - [ ] OK    Fix: 

- **`eqShape`**
  - EN: Pre-EQ Band {band} filter shape.
  - PT: Forma do filtro pré-EQ banda {band}.
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Reverb Configuration to file (with file explorer window).
  - PT: Exportar a configuração de reverberação para ficheiro (com explorador de ficheiros).
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: High frequency loss per meter (-6.0 to 0.0 dB/m).
  - PT: Perda de alta frequência por metro (-6.0 a 0.0 dB/m).
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Reverb Configuration from file (with file explorer window).
  - PT: Importar a configuração de reverberação a partir de ficheiro (com explorador de ficheiros).
  - [ ] OK    Fix: 

- **`liveSourceTooltip`**
  - EN: Enable Live Source attenuation tamer. Reduces level fluctuations from sources close to the array.
  - PT: Ativa o domador de atenuação Live Source. Reduz as flutuações de nível de fontes próximas do array.
  - [ ] OK    Fix: 

- **`mapVisibility`**
  - EN: Make Visible or Hide All Reverb Channels on the Map.
  - PT: Tornar visíveis ou ocultar todos os canais de reverberação no mapa.
  - [ ] OK    Fix: 

- **`miniLatencyTooltip`**
  - EN: Enable minimal latency mode for this reverb channel. Reduces processing delay at the cost of higher CPU usage.
  - PT: Ativa o modo de latência mínima para este canal de reverberação. Reduz o atraso de processamento à custa de maior uso de CPU.
  - [ ] OK    Fix: 

- **`muteButton`**
  - EN: Toggle mute for this output channel's reverb return.
  - PT: Ativar/desativar mudo do retorno de reverberação para esta saída.
  - [ ] OK    Fix: 

- **`muteMacros`**
  - EN: Quick mute operations for output channels.
  - PT: Operações rápidas de silenciamento para canais de saída.
  - [ ] OK    Fix: 

- **`nameEditor`**
  - EN: Displayed Reverb Channel Name (editable).
  - PT: Nome exibido do canal de reverberação (editável).
  - [ ] OK    Fix: 

- **`orientation`**
  - EN: Reverb orientation angle (-179 to +180 degrees).
  - PT: Ângulo de orientação da reverberação (-179 a +180 graus).
  - [ ] OK    Fix: 

- **`pitch`**
  - EN: Vertical orientation of reverb (-90 to +90 degrees).
  - PT: Orientação vertical da reverberação (-90 a +90 graus).
  - [ ] OK    Fix: 

- **`position1`**
  - EN: Reverb virtual source {name} ({unit}).
  - PT: Fonte virtual de reverberação {name} ({unit}).
  - [ ] OK    Fix: 

- **`position2`**
  - EN: Reverb virtual source {name} ({unit}).
  - PT: Fonte virtual de reverberação {name} ({unit}).
  - [ ] OK    Fix: 

- **`position3`**
  - EN: Reverb virtual source {name} ({unit}).
  - PT: Fonte virtual de reverberação {name} ({unit}).
  - [ ] OK    Fix: 

- **`postEqBandToggle`**
  - EN: Toggle post-EQ Band {band} on/off. When off the band is bypassed.
  - PT: Ativar/desativar a banda pós-EQ {band}.
  - [ ] OK    Fix: 

- **`postEqEnable`**
  - EN: Enable or disable post-processing EQ.
  - PT: Ativar ou desativar o EQ de pós-processamento.
  - [ ] OK    Fix: 

- **`postEqFlatten`**
  - EN: Long press to reset all post-EQ bands to their default values.
  - PT: Pressão longa para redefinir todas as bandas pós-EQ.
  - [ ] OK    Fix: 

- **`postEqFreq`**
  - EN: Post-EQ Band {band} frequency (20 Hz - 20 kHz).
  - PT: Frequência da banda {band} do post-EQ (20 Hz - 20 kHz).
  - [ ] OK    Fix: 

- **`postEqGain`**
  - EN: Post-EQ Band {band} gain (-24 to +24 dB).
  - PT: Ganho da banda {band} do post-EQ (-24 a +24 dB).
  - [ ] OK    Fix: 

- **`postEqQ`**
  - EN: Post-EQ Band {band} Q factor (0.1 - 20.0).
  - PT: Fator Q da banda {band} do post-EQ (0.1 - 20.0).
  - [ ] OK    Fix: 

- **`postEqResetBand`**
  - EN: Long press to reset post-EQ Band {band} to its default values.
  - PT: Pressão longa para redefinir a banda pós-EQ {band}.
  - [ ] OK    Fix: 

- **`postEqShape`**
  - EN: Post-EQ Band {band} filter shape.
  - PT: Forma do filtro da banda {band} do post-EQ.
  - [ ] OK    Fix: 

- **`postExpAttack`**
  - EN: Post-expander attack time (0.1 - 50 ms).
  - PT: Tempo de ataque do pós-expansor (0.1 - 50 ms).
  - [ ] OK    Fix: 

- **`postExpBypass`**
  - EN: Bypass or enable the post-expander on reverb returns.
  - PT: Contornar ou ativar o pós-expansor nos retornos de reverberação.
  - [ ] OK    Fix: 

- **`postExpRatio`**
  - EN: Post-expander ratio (1:1 to 1:8).
  - PT: Rácio do pós-expansor (1:1 a 1:8).
  - [ ] OK    Fix: 

- **`postExpRelease`**
  - EN: Post-expander release time (50 - 2000 ms).
  - PT: Tempo de libertação do pós-expansor (50 - 2000 ms).
  - [ ] OK    Fix: 

- **`postExpThreshold`**
  - EN: Post-expander threshold (-80 to -10 dB).
  - PT: Limiar do pós-expansor (-80 a -10 dB).
  - [ ] OK    Fix: 

- **`preCompAttack`**
  - EN: Pre-compressor attack time (0.1 - 100 ms).
  - PT: Tempo de ataque do pré-compressor (0.1 - 100 ms).
  - [ ] OK    Fix: 

- **`preCompBypass`**
  - EN: Bypass or enable the pre-compressor on reverb feeds.
  - PT: Contornar ou ativar o pré-compressor nos envios de reverberação.
  - [ ] OK    Fix: 

- **`preCompRatio`**
  - EN: Pre-compressor ratio (1:1 to 20:1).
  - PT: Rácio do pré-compressor (1:1 a 20:1).
  - [ ] OK    Fix: 

- **`preCompRelease`**
  - EN: Pre-compressor release time (10 - 1000 ms).
  - PT: Tempo de libertação do pré-compressor (10 - 1000 ms).
  - [ ] OK    Fix: 

- **`preCompThreshold`**
  - EN: Pre-compressor threshold (-60 to 0 dB).
  - PT: Limiar do pré-compressor (-60 a 0 dB).
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Reverb Configuration from backup file.
  - PT: Recarregar a configuração de reverberação a partir do ficheiro de backup.
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Reverb Configuration from file.
  - PT: Recarregar a configuração de reverberação a partir de ficheiro.
  - [ ] OK    Fix: 

- **`returnOffset1`**
  - EN: Reverb return offset {name} ({unit}).
  - PT: Deslocamento de retorno de reverberação {name} ({unit}).
  - [ ] OK    Fix: 

- **`returnOffset2`**
  - EN: Reverb return offset {name} ({unit}).
  - PT: Deslocamento de retorno de reverberação {name} ({unit}).
  - [ ] OK    Fix: 

- **`returnOffset3`**
  - EN: Reverb return offset {name} ({unit}).
  - PT: Deslocamento de retorno de reverberação {name} ({unit}).
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Reverb Configuration to file (with backup).
  - PT: Guardar a configuração de reverberação em ficheiro (com backup).
  - [ ] OK    Fix: 

## `reverbs.labels`

- **`angleOff`**
  - EN: Angle Off:
  - PT: Ângulo Desligado:
  - [ ] OK    Fix: 

- **`angleOn`**
  - EN: Angle On:
  - PT: Ângulo Ligado:
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Attenuation:
  - PT: Atenuação:
  - [ ] OK    Fix: 

- **`commonAtten`**
  - EN: Common Atten:
  - PT: Aten. Comum:
  - [ ] OK    Fix: 

- **`coordinates`**
  - EN: Coord:
  - PT: Coord.:
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Delay/Latency:
  - PT: Atraso/Latência:
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Distance Atten:
  - PT: Aten. Distância:
  - [ ] OK    Fix: 

- **`distanceAttenPercent`**
  - EN: Distance Atten %:
  - PT: Aten. Distância %:
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: HF Damping:
  - PT: Amortecimento HF:
  - [ ] OK    Fix: 

- **`muteMacro`**
  - EN: Mute Macro:
  - PT: Macro de Silenciamento:
  - [ ] OK    Fix: 

- **`name`**
  - EN: Name:
  - PT: Nome:
  - [ ] OK    Fix: 

- **`orientation`**
  - EN: Orientation:
  - PT: Orientação:
  - [ ] OK    Fix: 

- **`outputMutes`**
  - EN: Output Mutes:
  - PT: Silenciamentos de Saída:
  - [ ] OK    Fix: 

- **`pitch`**
  - EN: Pitch:
  - PT: Inclinação:
  - [ ] OK    Fix: 

- **`positionX`**
  - EN: Position X:
  - PT: Posição X:
  - [ ] OK    Fix: 

- **`positionY`**
  - EN: Position Y:
  - PT: Posição Y:
  - [ ] OK    Fix: 

- **`positionZ`**
  - EN: Position Z:
  - PT: Posição Z:
  - [ ] OK    Fix: 

- **`returnOffsetX`**
  - EN: Return Offset X:
  - PT: Deslocamento Retorno X:
  - [ ] OK    Fix: 

- **`returnOffsetY`**
  - EN: Return Offset Y:
  - PT: Deslocamento Retorno Y:
  - [ ] OK    Fix: 

- **`returnOffsetZ`**
  - EN: Return Offset Z:
  - PT: Deslocamento Retorno Z:
  - [ ] OK    Fix: 

## `reverbs.messages`

- **`backupLoaded`**
  - EN: Reverb configuration loaded from backup.
  - PT: Configuração de reverb carregada do backup.
  - [ ] OK    Fix: 

- **`configExported`**
  - EN: Reverb configuration exported.
  - PT: Configuração de reverb exportada.
  - [ ] OK    Fix: 

- **`configImported`**
  - EN: Reverb configuration imported.
  - PT: Configuração de reverb importada.
  - [ ] OK    Fix: 

- **`configLoaded`**
  - EN: Reverb configuration loaded.
  - PT: Configuração de reverb carregada.
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Reverb configuration saved.
  - PT: Configuração de reverb guardada.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - PT: Erro: {error}
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - PT: Por favor, selecione primeiro uma pasta de projeto em Config. Sistema.
  - [ ] OK    Fix: 

## `reverbs.muteMacros`

- **`invertMutes`**
  - EN: INVERT MUTES
  - PT: INVERTER SILENCIAMENTOS
  - [ ] OK    Fix: 

- **`muteAll`**
  - EN: MUTE ALL
  - PT: SILENCIAR TODOS
  - [ ] OK    Fix: 

- **`muteArray`**
  - EN: MUTE ARRAY
  - PT: SILENCIAR ARRAY
  - [ ] OK    Fix: 

- **`muteEven`**
  - EN: MUTE EVEN
  - PT: SILENCIAR PARES
  - [ ] OK    Fix: 

- **`muteOdd`**
  - EN: MUTE ODD
  - PT: SILENCIAR ÍMPARES
  - [ ] OK    Fix: 

- **`select`**
  - EN: Mute Macro Select
  - PT: Selecionar Macro de Silenciamento
  - [ ] OK    Fix: 

- **`unmuteAll`**
  - EN: UNMUTE ALL
  - PT: ATIVAR TODOS
  - [ ] OK    Fix: 

- **`unmuteArray`**
  - EN: UNMUTE ARRAY
  - PT: ATIVAR ARRAY
  - [ ] OK    Fix: 

## `reverbs.postProcessing`

- **`attack`**
  - EN: Attack:
  - PT: Ataque:
  - [ ] OK    Fix: 

- **`expander`**
  - EN: Expander
  - PT: Expansor
  - [ ] OK    Fix: 

- **`expanderOff`**
  - EN: Expander OFF
  - PT: Expansor DESLIGADO
  - [ ] OK    Fix: 

- **`expanderOn`**
  - EN: Expander ON
  - PT: Expansor LIGADO
  - [ ] OK    Fix: 

- **`ratio`**
  - EN: Ratio:
  - PT: Rácio:
  - [ ] OK    Fix: 

- **`release`**
  - EN: Release:
  - PT: Libertação:
  - [ ] OK    Fix: 

- **`threshold`**
  - EN: Threshold:
  - PT: Limiar:
  - [ ] OK    Fix: 

## `reverbs.preProcessing`

- **`attack`**
  - EN: Attack:
  - PT: Ataque:
  - [ ] OK    Fix: 

- **`compressor`**
  - EN: Compressor
  - PT: Compressor
  - [ ] OK    Fix: 

- **`compressorOff`**
  - EN: Compressor OFF
  - PT: Compressor DESLIGADO
  - [ ] OK    Fix: 

- **`compressorOn`**
  - EN: Compressor ON
  - PT: Compressor LIGADO
  - [ ] OK    Fix: 

- **`ratio`**
  - EN: Ratio:
  - PT: Rácio:
  - [ ] OK    Fix: 

- **`release`**
  - EN: Release:
  - PT: Libertação:
  - [ ] OK    Fix: 

- **`threshold`**
  - EN: Threshold:
  - PT: Limiar:
  - [ ] OK    Fix: 

## `reverbs.sections`

- **`reverbFeed`**
  - EN: Reverb Feed
  - PT: Envio de Reverb
  - [ ] OK    Fix: 

- **`reverbReturn`**
  - EN: Reverb Return
  - PT: Retorno de Reverb
  - [ ] OK    Fix: 

## `reverbs.tabs`

- **`algorithm`**
  - EN: Algorithm
  - PT: Algoritmo
  - [ ] OK    Fix: 

- **`channelParams`**
  - EN: Channel Parameters
  - PT: Parâmetros do Canal
  - [ ] OK    Fix: 

- **`postProcessing`**
  - EN: Post-Processing
  - PT: Pós-processamento
  - [ ] OK    Fix: 

- **`preProcessing`**
  - EN: Pre-Processing
  - PT: Pré-processamento
  - [ ] OK    Fix: 

## `reverbs.toggles`

- **`liveSourceOff`**
  - EN: Live Source Atten OFF
  - PT: Aten. Fonte Ao Vivo DESLIGADO
  - [ ] OK    Fix: 

- **`liveSourceOn`**
  - EN: Live Source Atten ON
  - PT: Aten. Fonte Ao Vivo LIGADO
  - [ ] OK    Fix: 

- **`minLatencyOff`**
  - EN: Minimal Latency OFF
  - PT: Latência Mínima DESLIGADO
  - [ ] OK    Fix: 

- **`minLatencyOn`**
  - EN: Minimal Latency ON
  - PT: Latência Mínima LIGADO
  - [ ] OK    Fix: 

## `sampler.buttons`

- **`copy`**
  - EN: Copy
  - PT: Copiar
  - [ ] OK    Fix: 

- **`copyCell`**
  - EN: Copy Cell
  - PT: Copiar célula
  - [ ] OK    Fix: 

- **`copySet`**
  - EN: Copy Set
  - PT: Copiar set
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - PT: Exportar
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - PT: Importar
  - [ ] OK    Fix: 

- **`paste`**
  - EN: Paste
  - PT: Colar
  - [ ] OK    Fix: 

- **`pasteCell`**
  - EN: Paste Cell
  - PT: Colar célula
  - [ ] OK    Fix: 

- **`pasteSet`**
  - EN: Paste Set
  - PT: Colar set
  - [ ] OK    Fix: 

## `sampler.cell`

- **`attenuation`**
  - EN: Attenuation (dB)
  - PT: Atenuação (dB)
  - [ ] OK    Fix: 

- **`clear`**
  - EN: Clear
  - PT: Limpar
  - [ ] OK    Fix: 

- **`inOut`**
  - EN: In/Out (ms)
  - PT: Ent/Saí (ms)
  - [ ] OK    Fix: 

- **`load`**
  - EN: Load
  - PT: Carregar
  - [ ] OK    Fix: 

- **`loadTitle`**
  - EN: Load Sample
  - PT: Carregar sample
  - [ ] OK    Fix: 

- **`offset`**
  - EN: Offset (m)
  - PT: Desloc. (m)
  - [ ] OK    Fix: 

- **`preview`**
  - EN: Preview
  - PT: Pré-escuta
  - [ ] OK    Fix: 

- **`previewStop`**
  - EN: Stop
  - PT: Parar
  - [ ] OK    Fix: 

## `sampler.grid`

- **`help`**
  - EN: Click = select
Shift = multi
Ctrl = set toggle
DblClick = load
  - PT: Clique=selecionar | Shift=multi | Ctrl=alternar set | DblClique=carregar
  - [ ] OK    Fix: 

## `sampler.labels`

- **`lightpadZone`**
  - EN: Lightpad Zone
  - PT: Zona Lightpad
  - [ ] OK    Fix: 

- **`selectZone`**
  - EN: Select Zone
  - PT: Selecionar zona
  - [ ] OK    Fix: 

## `sampler.lightpadZone`

- **`none`**
  - EN: None
  - PT: Nenhuma
  - [ ] OK    Fix: 

## `sampler.press`

- **`height`**
  - EN: Height
  - PT: Altura
  - [ ] OK    Fix: 

- **`hf`**
  - EN: HF Shelf
  - PT: HF Shelf
  - [ ] OK    Fix: 

- **`level`**
  - EN: Level
  - PT: Nível
  - [ ] OK    Fix: 

- **`xy`**
  - EN: Position XY
  - PT: Position XY
  - [ ] OK    Fix: 

## `sampler.remote`

- **`gridLayout`**
  - EN: Grid Layout
  - PT: Disposição da grelha
  - [ ] OK    Fix: 

## `sampler.section`

- **`actions`**
  - EN: ACTIONS
  - PT: AÇÕES
  - [ ] OK    Fix: 

- **`cell`**
  - EN: CELL PROPERTIES
  - PT: PROPRIEDADES DA CÉLULA
  - [ ] OK    Fix: 

- **`pressure`**
  - EN: PRESSURE MAPPINGS
  - PT: MAPEAMENTOS DE PRESSÃO
  - [ ] OK    Fix: 

- **`set`**
  - EN: SET MANAGEMENT
  - PT: GESTÃO DE SETS
  - [ ] OK    Fix: 

## `sampler.set`

- **`copy`**
  - EN: (copy)
  - PT: (cópia)
  - [ ] OK    Fix: 

- **`default`**
  - EN: Set
  - PT: Set
  - [ ] OK    Fix: 

- **`level`**
  - EN: Level (dB)
  - PT: Nível (dB)
  - [ ] OK    Fix: 

- **`pos`**
  - EN: Position (m)
  - PT: Posição (m)
  - [ ] OK    Fix: 

- **`rename`**
  - EN: Rename
  - PT: Renomear
  - [ ] OK    Fix: 

- **`roundRobin`**
  - EN: Round-Robin
  - PT: Round-Robin
  - [ ] OK    Fix: 

- **`sequential`**
  - EN: Sequential
  - PT: Sequencial
  - [ ] OK    Fix: 

## `sampler.tooltips`

- **`addSet`**
  - EN: Create a new set. If cells are selected they will be assigned to it.
  - PT: Criar um novo set. Se houver células selecionadas, ser-lhe-ão atribuídas.
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Cell attenuation in dB (0 = no attenuation, -60 = silent)
  - PT: Atenuação da célula em dB (0 = sem atenuação, -60 = silêncio)
  - [ ] OK    Fix: 

- **`clear`**
  - EN: Remove the sample from the selected cell (long press)
  - PT: Remover a amostra da célula selecionada (pressão longa)
  - [ ] OK    Fix: 

- **`copy`**
  - EN: Copy selected cell or active set to clipboard
  - PT: Copiar a célula selecionada ou o set ativo para a área de transferência
  - [ ] OK    Fix: 

- **`deleteSet`**
  - EN: Delete the active set (long press)
  - PT: Eliminar o set ativo (pressão longa)
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export sampler configuration to file
  - PT: Exportar a configuração do sampler para ficheiro
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import sampler configuration from file
  - PT: Importar a configuração do sampler a partir de ficheiro
  - [ ] OK    Fix: 

- **`inOut`**
  - EN: Set the In/Out time range in milliseconds. Drag between thumbs to shift both.
  - PT: Definir o intervalo de tempo Entrada/Saída em milissegundos. Arrastar entre os manípulos para mover ambos.
  - [ ] OK    Fix: 

- **`load`**
  - EN: Load a sample file into the selected cell
  - PT: Carregar um ficheiro de amostra na célula selecionada
  - [ ] OK    Fix: 

- **`offset`**
  - EN: Offset position in meters (X, Y, Z) relative to the set position
  - PT: Deslocamento de posição em metros (X, Y, Z) relativo à posição do set
  - [ ] OK    Fix: 

- **`paste`**
  - EN: Paste clipboard data to selected cell or active set
  - PT: Colar os dados da área de transferência na célula selecionada ou no set ativo
  - [ ] OK    Fix: 

- **`playMode`**
  - EN: Toggle between Sequential and Round-Robin playback
  - PT: Alternar entre reprodução Sequencial e Round-Robin
  - [ ] OK    Fix: 

- **`pressCurve`**
  - EN: Pressure response curve (0 = concave, 0.5 = linear, 1 = convex)
  - PT: Curva de resposta à pressão (0 = côncava, 0,5 = linear, 1 = convexa)
  - [ ] OK    Fix: 

- **`pressDir`**
  - EN: Toggle pressure direction: + = more pressure increases, - = decreases
  - PT: Alternar a direção da pressão: + = mais pressão aumenta, - = diminui
  - [ ] OK    Fix: 

- **`pressHeight`**
  - EN: Map finger pressure to vertical position (Z)
  - PT: Mapear a pressão do dedo na posição vertical (Z)
  - [ ] OK    Fix: 

- **`pressHF`**
  - EN: Map finger pressure to high-frequency shelf attenuation
  - PT: Mapear a pressão do dedo na atenuação do shelving agudo
  - [ ] OK    Fix: 

- **`pressLevel`**
  - EN: Map finger pressure to output level
  - PT: Mapear a pressão do dedo no nível de saída
  - [ ] OK    Fix: 

- **`pressXY`**
  - EN: Map finger pressure to XY position movement
  - PT: Mapear a pressão do dedo no deslocamento de posição XY
  - [ ] OK    Fix: 

- **`pressXYScale`**
  - EN: Sensitivity: how far the source moves per pressure step
  - PT: Sensibilidade: a que distância a fonte se desloca por cada passo de pressão
  - [ ] OK    Fix: 

- **`preview`**
  - EN: Preview the loaded sample
  - PT: Pré-escutar a amostra carregada
  - [ ] OK    Fix: 

- **`renameSet`**
  - EN: Rename the active set
  - PT: Renomear o set ativo
  - [ ] OK    Fix: 

- **`setLevel`**
  - EN: Set output level in dB (0 = unity, -60 = silent)
  - PT: Definir o nível de saída em dB (0 = unidade, -60 = silêncio)
  - [ ] OK    Fix: 

- **`setPos`**
  - EN: Set base position in meters (X, Y, Z)
  - PT: Definir a posição base em metros (X, Y, Z)
  - [ ] OK    Fix: 

## `sampler.zone`

- **`remotePad`**
  - EN: Pad {num}
  - PT: Pad {num}
  - [ ] OK    Fix: 

- **`selectRemotePad`**
  - EN: Select Remote Pad
  - PT: Selecionar Pad do Comando
  - [ ] OK    Fix: 

## `setAllInputs`

- **`warning`**
  - EN: Changes will apply to ALL inputs
  - PT: As alterações serão aplicadas a TODAS as entradas
  - [ ] OK    Fix: 

- **`windowTitle`**
  - EN: Set All Inputs
  - PT: Definir Todas as Entradas
  - [ ] OK    Fix: 

## `setAllInputs.buttons`

- **`all1d`**
  - EN: All 1/d
  - PT: Todos 1/d
  - [ ] OK    Fix: 

- **`allLog`**
  - EN: All Log
  - PT: Todos Log
  - [ ] OK    Fix: 

- **`closeWindow`**
  - EN: CLOSE WINDOW
  - PT: FECHAR JANELA
  - [ ] OK    Fix: 

- **`flipXyzOff`**
  - EN: Flip XYZ > OFF
  - PT: Inverter XYZ > DESLIGADO
  - [ ] OK    Fix: 

- **`resetDirectivity`**
  - EN: Reset directivity
  - PT: Reiniciar diretividade
  - [ ] OK    Fix: 

- **`turnOffJitterLfo`**
  - EN: Turn OFF jitter & LFO
  - PT: Desligar tremulação & LFO
  - [ ] OK    Fix: 

- **`turnOffLiveSource`**
  - EN: Turn OFF Live source atten.
  - PT: Desligar aten. Fonte Ao Vivo
  - [ ] OK    Fix: 

## `setAllInputs.coordinateModes`

- **`rThetaPhi`**
  - EN: r θ φ
  - PT: r θ φ
  - [ ] OK    Fix: 

- **`rThetaZ`**
  - EN: r θ Z
  - PT: r θ Z
  - [ ] OK    Fix: 

- **`xyz`**
  - EN: XYZ
  - PT: XYZ
  - [ ] OK    Fix: 

## `setAllInputs.labels`

- **`common`**
  - EN: common
  - PT: comum
  - [ ] OK    Fix: 

- **`constraintPositions`**
  - EN: Constraint positions:
  - PT: Restringir posições:
  - [ ] OK    Fix: 

- **`coordinateMode`**
  - EN: Coordinate mode:
  - PT: Modo de coordenadas:
  - [ ] OK    Fix: 

- **`dbPerMeter`**
  - EN: dB/m
  - PT: dB/m
  - [ ] OK    Fix: 

- **`distanceAttenuation`**
  - EN: Distance attenuation
  - PT: Atenuação de distância
  - [ ] OK    Fix: 

- **`floorReflections`**
  - EN: Floor Reflections:
  - PT: Reflexões do Chão:
  - [ ] OK    Fix: 

- **`fringe`**
  - EN: Fringe:
  - PT: Margem:
  - [ ] OK    Fix: 

- **`heightFactor`**
  - EN: Height factor:
  - PT: Fator de altura:
  - [ ] OK    Fix: 

- **`minimalLatency`**
  - EN: Minimal Latency:
  - PT: Latência Mínima:
  - [ ] OK    Fix: 

- **`muteMacros`**
  - EN: Mute macros:
  - PT: Macros de silenciamento:
  - [ ] OK    Fix: 

- **`ratio`**
  - EN: ratio
  - PT: rácio
  - [ ] OK    Fix: 

- **`sidelines`**
  - EN: Sidelines:
  - PT: Linhas laterais:
  - [ ] OK    Fix: 

## `snapshot`

- **`qlabExportDone`**
  - EN: QLab export complete: {count} cues created
  - PT: Exportação QLab concluída: {count} cues criados
  - [ ] OK    Fix: 

- **`qlabExportStarted`**
  - EN: Writing {count} cues to QLab...
  - PT: Escrevendo {count} cues no QLab...
  - [ ] OK    Fix: 

- **`qlabGroupName`**
  - EN: Snapshot "{name}"
  - PT: Captura "{name}"
  - [ ] OK    Fix: 

- **`qlabMemoText`**
  - EN: Run either of the following cues to recall or update this snapshot
  - PT: Execute qualquer um dos seguintes cues para recarregar ou atualizar este snapshot
  - [ ] OK    Fix: 

- **`qlabNoTarget`**
  - EN: No QLab target configured
  - PT: Nenhum destino QLab configurado
  - [ ] OK    Fix: 

- **`qlabReloadName`**
  - EN: Reload "{name}"
  - PT: Recarregar "{name}"
  - [ ] OK    Fix: 

- **`qlabUpdateName`**
  - EN: Update "{name}"
  - PT: Atualizar "{name}"
  - [ ] OK    Fix: 

## `snapshotScope`

- **`all`**
  - EN: ALL
  - PT: TODOS
  - [ ] OK    Fix: 

- **`applyScope`**
  - EN: Apply scope:
  - PT: Aplicar âmbito:
  - [ ] OK    Fix: 

- **`autoPreselectModified`**
  - EN: Auto-preselect modified parameters
  - PT: Pré-selecionar automaticamente os parâmetros modificados
  - [ ] OK    Fix: 

- **`title`**
  - EN: Snapshot Scope: {name}
  - PT: Âmbito do Snapshot: {name}
  - [ ] OK    Fix: 

- **`whenRecalling`**
  - EN: When Recalling
  - PT: Ao Recuperar
  - [ ] OK    Fix: 

- **`whenSaving`**
  - EN: When Saving
  - PT: Ao Guardar
  - [ ] OK    Fix: 

- **`windowTitle`**
  - EN: Input Snapshot Scope
  - PT: Âmbito do Snapshot de Entrada
  - [ ] OK    Fix: 

- **`writeSnapshotLoadCue`**
  - EN: Write Snapshot Load Cue to QLab
  - PT: Escrever cue de carregamento no QLab
  - [ ] OK    Fix: 

- **`writeSnapshotLoadCueTooltip`**
  - EN: Also create a QLab cue to load this snapshot via OSC
  - PT: Também criar um cue QLab para carregar este snapshot via OSC
  - [ ] OK    Fix: 

- **`writeToQLab`**
  - EN: Write to QLab
  - PT: Escrever no QLab
  - [ ] OK    Fix: 

- **`writeToQLabTooltip`**
  - EN: Export scope to QLab instead of saving to file
  - PT: Exportar scope para QLab em vez de guardar em ficheiro
  - [ ] OK    Fix: 

## `snapshotScope.buttons`

- **`cancel`**
  - EN: Cancel
  - PT: Cancelar
  - [ ] OK    Fix: 

- **`clearChanges`**
  - EN: Clear Changes
  - PT: Limpar alterações
  - [ ] OK    Fix: 

- **`ok`**
  - EN: OK
  - PT: OK
  - [ ] OK    Fix: 

- **`selectModified`**
  - EN: Select Modified
  - PT: Selecionar modificados
  - [ ] OK    Fix: 

## `snapshotScope.sections`

- **`attenuation`**
  - EN: Attenuation
  - PT: Atenuação
  - [ ] OK    Fix: 

- **`automOtion`**
  - EN: AutomOtion
  - PT: AutomOção
  - [ ] OK    Fix: 

- **`directivity`**
  - EN: Directivity
  - PT: Diretividade
  - [ ] OK    Fix: 

- **`hackoustics`**
  - EN: Hackoustics
  - PT: Hackústica
  - [ ] OK    Fix: 

- **`input`**
  - EN: Input
  - PT: Entrada
  - [ ] OK    Fix: 

- **`lfo`**
  - EN: LFO
  - PT: LFO
  - [ ] OK    Fix: 

- **`liveSource`**
  - EN: Live Source
  - PT: Fonte Ao Vivo
  - [ ] OK    Fix: 

- **`mutes`**
  - EN: Mutes
  - PT: Silenciamentos
  - [ ] OK    Fix: 

- **`position`**
  - EN: Position
  - PT: Posição
  - [ ] OK    Fix: 

## `statusBar`

- **`displayLabel`**
  - EN: Display:
  - PT: Exibir:
  - [ ] OK    Fix: 

- **`helpMode`**
  - EN: Help
  - PT: Ajuda
  - [ ] OK    Fix: 

- **`oscMode`**
  - EN: OSC
  - PT: OSC
  - [ ] OK    Fix: 

## `systemConfig.algorithms`

- **`inputBuffer`**
  - EN: InputBuffer (read-time delays)
  - PT: InputBuffer (atrasos de leitura)
  - [ ] OK    Fix: 

- **`outputBuffer`**
  - EN: OutputBuffer (write-time delays)
  - PT: OutputBuffer (atrasos de escrita)
  - [ ] OK    Fix: 

## `systemConfig.binauralOutput`

- **`select`**
  - EN: Select...
  - PT: Selecionar...
  - [ ] OK    Fix: 

## `systemConfig.buttons`

- **`audioPatch`**
  - EN: Audio Interface and Patching Window
  - PT: Interface de Áudio e Patch
  - [ ] OK    Fix: 

- **`binauralOff`**
  - EN: Binaural: OFF
  - PT: Binaural: DESLIGADO
  - [ ] OK    Fix: 

- **`binauralOn`**
  - EN: Binaural: ON
  - PT: Binaural: LIGADO
  - [ ] OK    Fix: 

- **`clearSolo`**
  - EN: Clear Solo
  - PT: Clear Solo
  - [ ] OK    Fix: 

- **`copySystemInfo`**
  - EN: Copy System Info
  - PT: Copiar info do sistema
  - [ ] OK    Fix: 

- **`diagnosticsCollapsed`**
  - EN: Diagnostics  [+]
  - PT: Diagnóstico  [+]
  - [ ] OK    Fix: 

- **`diagnosticsExpanded`**
  - EN: Diagnostics  [-]
  - PT: Diagnóstico  [-]
  - [ ] OK    Fix: 

- **`exportLogs`**
  - EN: Export Logs
  - PT: Exportar logs
  - [ ] OK    Fix: 

- **`exportSystem`**
  - EN: Export System Configuration
  - PT: Exportar Configuração do Sistema
  - [ ] OK    Fix: 

- **`importSystem`**
  - EN: Import System Configuration
  - PT: Importar Configuração do Sistema
  - [ ] OK    Fix: 

- **`levelMeter`**
  - EN: Level Meters
  - PT: Level Meters
  - [ ] OK    Fix: 

- **`openLogFolder`**
  - EN: Open Log Folder
  - PT: Abrir pasta de logs
  - [ ] OK    Fix: 

- **`processingOff`**
  - EN: Processing: OFF
  - PT: Processamento: DESLIGADO
  - [ ] OK    Fix: 

- **`processingOn`**
  - EN: Processing: ON
  - PT: Processamento: LIGADO
  - [ ] OK    Fix: 

- **`quickLongPressOff`**
  - EN: Normal
  - PT: Normal
  - [ ] OK    Fix: 

- **`quickLongPressOn`**
  - EN: Quick
  - PT: Rápido
  - [ ] OK    Fix: 

- **`reloadComplete`**
  - EN: Reload Complete Configuration
  - PT: Recarregar Configuração Completa
  - [ ] OK    Fix: 

- **`reloadCompleteBackup`**
  - EN: Reload Complete Config. Backup
  - PT: Recarregar Config. Completa do Backup
  - [ ] OK    Fix: 

- **`reloadSystem`**
  - EN: Reload System Configuration
  - PT: Recarregar Configuração do Sistema
  - [ ] OK    Fix: 

- **`reloadSystemBackup`**
  - EN: Reload System Config. Backup
  - PT: Recarregar Config. Sistema do Backup
  - [ ] OK    Fix: 

- **`reportIssue`**
  - EN: Report Issue
  - PT: Reportar problema
  - [ ] OK    Fix: 

- **`selectProjectFolder`**
  - EN: Select Project Folder
  - PT: Selecionar Pasta do Projeto
  - [ ] OK    Fix: 

- **`setup`**
  - EN: Setup
  - PT: Config.
  - [ ] OK    Fix: 

- **`soloModeMulti`**
  - EN: Solo: Multi
  - PT: Solo: Múltiplo
  - [ ] OK    Fix: 

- **`soloModeSingle`**
  - EN: Solo: Single
  - PT: Solo: Único
  - [ ] OK    Fix: 

- **`storeComplete`**
  - EN: Store Complete Configuration
  - PT: Guardar Configuração Completa
  - [ ] OK    Fix: 

- **`storeSystem`**
  - EN: Store System Configuration
  - PT: Guardar Configuração do Sistema
  - [ ] OK    Fix: 

## `systemConfig.colorSchemes`

- **`black`**
  - EN: Black
  - PT: Preto
  - [ ] OK    Fix: 

- **`default`**
  - EN: Default (Dark Gray)
  - PT: Padrão (Cinza Escuro)
  - [ ] OK    Fix: 

- **`light`**
  - EN: Light
  - PT: Claro
  - [ ] OK    Fix: 

## `systemConfig.controller`

- **`lightpad`**
  - EN: Lightpad
  - PT: Lightpad
  - [ ] OK    Fix: 

- **`off`**
  - EN: Off
  - PT: Desligado
  - [ ] OK    Fix: 

- **`remote`**
  - EN: Remote
  - PT: Comando
  - [ ] OK    Fix: 

## `systemConfig.devices`

- **`off`**
  - EN: Off
  - PT: Desligado
  - [ ] OK    Fix: 

## `systemConfig.dialogs`

- **`exportSystemConfig`**
  - EN: Export System Configuration
  - PT: Exportar Configuração do Sistema
  - [ ] OK    Fix: 

- **`importSystemConfig`**
  - EN: Import System Configuration
  - PT: Importar Configuração do Sistema
  - [ ] OK    Fix: 

- **`reduce`**
  - EN: Reduce
  - PT: Reduzir
  - [ ] OK    Fix: 

- **`reduceInputChannels.message`**
  - EN: Reducing from {current} to {new} input channels will remove settings for channels {start} to {end}.

This cannot be undone.
  - PT: Reduzir de {current} para {new} canais de entrada irá remover as configurações dos canais {start} a {end}.

Esta ação não pode ser desfeita.
  - [ ] OK    Fix: 

- **`reduceInputChannels.title`**
  - EN: Reduce Input Channels?
  - PT: Reduzir Canais de Entrada?
  - [ ] OK    Fix: 

- **`reduceOutputChannels.message`**
  - EN: Reducing from {current} to {new} output channels will remove settings for channels {start} to {end}.

This cannot be undone.
  - PT: Reduzir de {current} para {new} canais de saída irá remover as configurações dos canais {start} a {end}.

Esta ação não pode ser desfeita.
  - [ ] OK    Fix: 

- **`reduceOutputChannels.title`**
  - EN: Reduce Output Channels?
  - PT: Reduzir Canais de Saída?
  - [ ] OK    Fix: 

- **`reduceReverbChannels.message`**
  - EN: Reducing from {current} to {new} reverb channels will remove settings for channels {start} to {end}.

This cannot be undone.
  - PT: Reduzir de {current} para {new} canais de reverb irá remover as configurações dos canais {start} a {end}.

Esta ação não pode ser desfeita.
  - [ ] OK    Fix: 

- **`reduceReverbChannels.title`**
  - EN: Reduce Reverb Channels?
  - PT: Reduzir Canais de Reverb?
  - [ ] OK    Fix: 

- **`selectProjectFolder`**
  - EN: Select Project Folder
  - PT: Selecionar Pasta do Projeto
  - [ ] OK    Fix: 

## `systemConfig.help`

- **`algorithm`**
  - EN: Select the rendering algorithm from the menu.
  - PT: Selecione o algoritmo de renderização no menu.
  - [ ] OK    Fix: 

- **`audioPatch`**
  - EN: Opens the Audio Interface and Patching Window.
  - PT: Abre a janela de Interface de Áudio e Patch.
  - [ ] OK    Fix: 

- **`binauralAngle`**
  - EN: Horizontal rotation of binaural listener view (degrees, 0 = facing stage).
  - PT: Rotação horizontal da perspetiva do ouvinte binaural (graus, 0 = virado para o palco).
  - [ ] OK    Fix: 

- **`binauralAtten`**
  - EN: Overall level offset for binaural output (dB).
  - PT: Deslocamento de nível global para a saída binaural (dB).
  - [ ] OK    Fix: 

- **`binauralDelay`**
  - EN: Additional delay for binaural output (milliseconds).
  - PT: Atraso adicional para a saída binaural (milissegundos).
  - [ ] OK    Fix: 

- **`binauralDistance`**
  - EN: Distance of binaural listener from stage origin (meters).
  - PT: Distância do ouvinte binaural à origem do palco (metros).
  - [ ] OK    Fix: 

- **`binauralEnable`**
  - EN: Enable or disable the binaural renderer processing.
  - PT: Ativar ou desativar o processamento do renderizador binaural.
  - [ ] OK    Fix: 

- **`binauralOutput`**
  - EN: Select output channel pair for binaural monitoring. Off disables binaural output.
  - PT: Selecionar o par de canais de saída para a monitorização binaural. Off desativa a saída binaural.
  - [ ] OK    Fix: 

- **`colorScheme`**
  - EN: Select the color scheme: Default (dark gray), Black (pure black for OLED displays), or Light (daytime use).
  - PT: Selecione o esquema de cores: Padrão (cinza escuro), Preto (para ecrãs OLED) ou Claro (uso diurno).
  - [ ] OK    Fix: 

- **`copySystemInfo`**
  - EN: Copy detailed system information to the clipboard for support requests.
  - PT: Copiar informações detalhadas do sistema para a área de transferência para pedidos de suporte.
  - [ ] OK    Fix: 

- **`diagnosticsToggle`**
  - EN: Long-press to show or hide the diagnostic tools (export logs, open log folder, copy system info, report issue).
  - PT: Premir longamente para mostrar ou ocultar as ferramentas de diagnóstico (exportar registos, abrir pasta de registos, copiar info do sistema, reportar problema).
  - [ ] OK    Fix: 

- **`dialsAndButtons`**
  - EN: Select the hardware controller for dials and buttons: Stream Deck+ or XenceLabs Quick Keys.
  - PT: Selecionar o controlador físico para botões e mostradores: Stream Deck+ ou XenceLabs Quick Keys.
  - [ ] OK    Fix: 

- **`domeElevation`**
  - EN: Dome elevation angle: 180 = hemisphere, 360 = full sphere.
  - PT: Ângulo de elevação da cúpula: 180 = hemisfério, 360 = esfera completa.
  - [ ] OK    Fix: 

- **`exportLogs`**
  - EN: Export diagnostic logs to a zip file for debugging or support.
  - PT: Exportar os registos de diagnóstico para um ficheiro zip para depuração ou suporte.
  - [ ] OK    Fix: 

- **`exportSystem`**
  - EN: Export System Configuration to file (with file explorer window).
  - PT: Exportar configuração do sistema para ficheiro (com explorador de ficheiros).
  - [ ] OK    Fix: 

- **`haasEffect`**
  - EN: Hass Effect to apply to the system. Will take into account the Latency Compensations (System, Input and Output).
  - PT: Efeito Haas a aplicar ao sistema. Terá em conta as compensações de latência (Sistema, Entrada e Saída).
  - [ ] OK    Fix: 

- **`importSystem`**
  - EN: Import System Configuration from file (with file explorer window).
  - PT: Importar configuração do sistema do ficheiro (com explorador de ficheiros).
  - [ ] OK    Fix: 

- **`inputChannels`**
  - EN: Number of Input Channels.
  - PT: Número de canais de entrada.
  - [ ] OK    Fix: 

- **`language`**
  - EN: Select the user interface language. Changes take full effect after restarting the application.
  - PT: Selecione o idioma da interface. As alterações terão efeito completo após reiniciar a aplicação.
  - [ ] OK    Fix: 

- **`lightpadSetup`**
  - EN: Display the connected Roli Lightpads and allow for splitting them in 4 smaller pads.
  - PT: Exibir os Roli Lightpads conectados e permitir dividi-los em 4 pads menores.
  - [ ] OK    Fix: 

- **`masterLevel`**
  - EN: Master Level (affects all outputs).
  - PT: Nível principal (afeta todas as saídas).
  - [ ] OK    Fix: 

- **`openLogFolder`**
  - EN: Open the application's log folder in the system file explorer.
  - PT: Abrir a pasta de registos da aplicação no explorador de ficheiros do sistema.
  - [ ] OK    Fix: 

- **`originCenter`**
  - EN: Set origin to center of stage volume. Typical for Spherical Dome setups.
  - PT: Definir origem no centro do volume do palco. Típico para configurações de Cúpula Esférica.
  - [ ] OK    Fix: 

- **`originCenterGround`**
  - EN: Set origin to center of stage at ground level. Typical for Surround or Cylindrical setups.
  - PT: Definir origem no centro do palco ao nível do chão. Típico para configurações Surround ou Cilíndricas.
  - [ ] OK    Fix: 

- **`originDepth`**
  - EN: Origin Y offset from stage center (0 = centered, negative = front/downstage).
  - PT: Deslocamento Y da origem do centro do palco (0 = centrado, negativo = frente).
  - [ ] OK    Fix: 

- **`originDismissInputs`**
  - EN: Long press to dismiss and keep current input positions.
  - PT: Pressão longa para ignorar e manter as posições atuais das entradas.
  - [ ] OK    Fix: 

- **`originDismissOutputs`**
  - EN: Long press to dismiss and keep current output positions.
  - PT: Pressão longa para ignorar e manter as posições atuais das saídas.
  - [ ] OK    Fix: 

- **`originDismissReverbs`**
  - EN: Long press to dismiss and keep current reverb positions.
  - PT: Pressão longa para ignorar e manter as posições atuais dos reverbs.
  - [ ] OK    Fix: 

- **`originFront`**
  - EN: Set origin to front center of stage. Typical for frontal stages.
  - PT: Definir origem na frente central do palco. Típico para palcos frontais.
  - [ ] OK    Fix: 

- **`originHeight`**
  - EN: Origin Z offset from floor (0 = floor level, positive = above floor).
  - PT: Deslocamento Z da origem do chão (0 = nível do chão, positivo = acima do chão).
  - [ ] OK    Fix: 

- **`originShiftInputs`**
  - EN: Long press to shift all input positions by the origin change.
  - PT: Pressão longa para deslocar todas as posições de entrada conforme a mudança de origem.
  - [ ] OK    Fix: 

- **`originShiftOutputs`**
  - EN: Long press to shift all output positions by the origin change.
  - PT: Pressão longa para deslocar todas as posições de saída conforme a mudança de origem.
  - [ ] OK    Fix: 

- **`originShiftReverbs`**
  - EN: Long press to shift all reverb positions by the origin change.
  - PT: Pressão longa para deslocar todas as posições de reverb conforme a mudança de origem.
  - [ ] OK    Fix: 

- **`originWidth`**
  - EN: Origin X offset from stage center (0 = centered, negative = left).
  - PT: Deslocamento X da origem do centro do palco (0 = centrado, negativo = esquerda).
  - [ ] OK    Fix: 

- **`outputChannels`**
  - EN: Number of Output Channels.
  - PT: Número de canais de saída.
  - [ ] OK    Fix: 

- **`positionControl`**
  - EN: Select the hardware controller for position control: Space Mouse, Joystick, or Game Pad.
  - PT: Selecionar o controlador físico para o controlo de posição: Space Mouse, Joystick ou comando de jogos.
  - [ ] OK    Fix: 

- **`processing`**
  - EN: Lock all I/O parameters and start the DSP. Long press to stop the DSP.
  - PT: Bloquear todos os parâmetros de E/S e iniciar o DSP. Prima longamente para parar o DSP.
  - [ ] OK    Fix: 

- **`quickLongPress`**
  - EN: Long press duration. Instead of confirmation windows this software uses long presses.
  - PT: Duração da pressão longa. Em vez de janelas de confirmação, este software usa pressões longas.
  - [ ] OK    Fix: 

- **`reloadComplete`**
  - EN: Reload Complete Configuration from files.
  - PT: Recarregar configuração completa dos ficheiros.
  - [ ] OK    Fix: 

- **`reloadCompleteBackup`**
  - EN: Reload Complete Configuration from backup files.
  - PT: Recarregar configuração completa dos ficheiros de backup.
  - [ ] OK    Fix: 

- **`reloadSystem`**
  - EN: Reload System Configuration from file.
  - PT: Recarregar configuração do sistema do ficheiro.
  - [ ] OK    Fix: 

- **`reloadSystemBackup`**
  - EN: Reload System Configuration from backup file.
  - PT: Recarregar configuração do sistema do ficheiro de backup.
  - [ ] OK    Fix: 

- **`remoteSetup`**
  - EN: Select the number of pads in the Remote's XY Pads tab.
  - PT: Selecionar o número de pads na aba XY Pads do Remote.
  - [ ] OK    Fix: 

- **`reportIssue`**
  - EN: Open the WFS-DIY GitHub issues page in your default browser.
  - PT: Abrir a página de issues do WFS-DIY no GitHub no navegador predefinido.
  - [ ] OK    Fix: 

- **`reverbChannels`**
  - EN: Number of Reverb Channels.
  - PT: Número de canais de reverberação.
  - [ ] OK    Fix: 

- **`sampler`**
  - EN: Enable or disable the Sampler feature for input channels.
  - PT: Ativar ou desativar a função Sampler para os canais de entrada. Selecionar o controlador: Lightpad ou Comando.
  - [ ] OK    Fix: 

- **`selectProjectFolder`**
  - EN: Select the Location of the Current Project Folder where to store files.
  - PT: Selecione a localização da pasta do projeto atual para guardar ficheiros.
  - [ ] OK    Fix: 

- **`showLocation`**
  - EN: Location of the current show.
  - PT: Local do espetáculo atual.
  - [ ] OK    Fix: 

- **`showName`**
  - EN: Name of the current show.
  - PT: Nome do espetáculo atual.
  - [ ] OK    Fix: 

- **`soloMode`**
  - EN: Single: one input at a time. Multi: multiple inputs simultaneously.
  - PT: Único: uma entrada de cada vez. Múltiplo: várias entradas simultaneamente.
  - [ ] OK    Fix: 

- **`speedOfSound`**
  - EN: Speed of Sound (related to the temperature).
  - PT: Velocidade do som (relacionada com a temperatura).
  - [ ] OK    Fix: 

- **`stageDepth`**
  - EN: Depth of the stage in meters (Box shape only).
  - PT: Profundidade do palco em metros (apenas forma Caixa).
  - [ ] OK    Fix: 

- **`stageDiameter`**
  - EN: Diameter of the stage in meters (Cylinder and Dome shapes).
  - PT: Diâmetro do palco em metros (formas Cilindro e Cúpula).
  - [ ] OK    Fix: 

- **`stageDismiss`**
  - EN: Long press to dismiss and keep current input positions.
  - PT: Pressão longa para ignorar e manter as posições atuais das entradas.
  - [ ] OK    Fix: 

- **`stageFit`**
  - EN: Long press to move outlier inputs back within the new stage bounds.
  - PT: Pressão longa para mover entradas fora dos limites para dentro das novas dimensões do palco.
  - [ ] OK    Fix: 

- **`stageHeight`**
  - EN: Height of the stage in meters (Box and Cylinder shapes).
  - PT: Altura do palco em metros (formas Caixa e Cilindro).
  - [ ] OK    Fix: 

- **`stageScale`**
  - EN: Long press to scale all input positions proportionally to the new stage dimensions.
  - PT: Pressão longa para escalar todas as posições de entrada proporcionalmente às novas dimensões do palco.
  - [ ] OK    Fix: 

- **`stageShape`**
  - EN: Select the shape of the stage (Box, Cylinder, or Dome).
  - PT: Selecione a forma do palco (Caixa, Cilindro ou Cúpula).
  - [ ] OK    Fix: 

- **`stageWidth`**
  - EN: Width of the stage in meters (Box shape only).
  - PT: Largura do palco em metros (apenas forma Caixa).
  - [ ] OK    Fix: 

- **`storeComplete`**
  - EN: Store Complete Configuration to files (with backup).
  - PT: Guardar configuração completa em ficheiros (com backup).
  - [ ] OK    Fix: 

- **`storeSystem`**
  - EN: Store System Configuration to file (with backup).
  - PT: Guardar configuração do sistema em ficheiro (com backup).
  - [ ] OK    Fix: 

- **`systemLatency`**
  - EN: Total latency of the system (Mixing board & Computer) / Specific Input and Output Latency/Delay can be set in the respective Input and Output settings.
  - PT: Latência total do sistema (Mesa de mistura e Computador) / Latência específica de Entrada e Saída pode ser definida nas respetivas configurações.
  - [ ] OK    Fix: 

- **`temperature`**
  - EN: Temperature (gives the Speed of Sound).
  - PT: Temperatura (determina a velocidade do som).
  - [ ] OK    Fix: 

## `systemConfig.labels`

- **`algorithm`**
  - EN: Algorithm:
  - PT: Algoritmo:
  - [ ] OK    Fix: 

- **`binauralAngle`**
  - EN: Listener Angle:
  - PT: Ângulo ouvinte:
  - [ ] OK    Fix: 

- **`binauralAtten`**
  - EN: Binaural Level:
  - PT: Nível binaural:
  - [ ] OK    Fix: 

- **`binauralDelay`**
  - EN: Binaural Delay:
  - PT: Atraso binaural:
  - [ ] OK    Fix: 

- **`binauralDistance`**
  - EN: Listener Distance:
  - PT: Distância ouvinte:
  - [ ] OK    Fix: 

- **`binauralOutput`**
  - EN: Binaural Output:
  - PT: Saída binaural:
  - [ ] OK    Fix: 

- **`clickToSplit`**
  - EN: Click to split
  - PT: Clique para dividir
  - [ ] OK    Fix: 

- **`colorScheme`**
  - EN: Color Scheme:
  - PT: Esquema de Cores:
  - [ ] OK    Fix: 

- **`dialsAndButtons`**
  - EN: Dials and Buttons:
  - PT: Botões e mostradores:
  - [ ] OK    Fix: 

- **`domeElevation`**
  - EN: Elevation:
  - PT: Elevação da Cúpula:
  - [ ] OK    Fix: 

- **`haasEffect`**
  - EN: Haas Effect:
  - PT: Efeito Haas:
  - [ ] OK    Fix: 

- **`inputChannels`**
  - EN: Input Channels:
  - PT: Canais de Entrada:
  - [ ] OK    Fix: 

- **`language`**
  - EN: Language:
  - PT: Idioma:
  - [ ] OK    Fix: 

- **`lightpadArrangement`**
  - EN: Lightpad Arrangement
  - PT: Disposição do Lightpad
  - [ ] OK    Fix: 

- **`masterLevel`**
  - EN: Master Level:
  - PT: Nível Principal:
  - [ ] OK    Fix: 

- **`originDepth`**
  - EN: Origin Depth:
  - PT: Origem Profundidade:
  - [ ] OK    Fix: 

- **`originHeight`**
  - EN: Origin Height:
  - PT: Origem Altura:
  - [ ] OK    Fix: 

- **`originWidth`**
  - EN: Origin Width:
  - PT: Origem Largura:
  - [ ] OK    Fix: 

- **`outputChannels`**
  - EN: Output Channels:
  - PT: Canais de Saída:
  - [ ] OK    Fix: 

- **`positionControl`**
  - EN: Position Control:
  - PT: Controlo de posição:
  - [ ] OK    Fix: 

- **`quickLongPress`**
  - EN: Long Press:
  - PT: Pressão longa:
  - [ ] OK    Fix: 

- **`reverbChannels`**
  - EN: Reverb Channels:
  - PT: Canais de Reverb:
  - [ ] OK    Fix: 

- **`sampler`**
  - EN: Sampler:
  - PT: Sampler:
  - [ ] OK    Fix: 

- **`showLocation`**
  - EN: Location:
  - PT: Local:
  - [ ] OK    Fix: 

- **`showName`**
  - EN: Name:
  - PT: Nome:
  - [ ] OK    Fix: 

- **`speedOfSound`**
  - EN: Speed of Sound:
  - PT: Velocidade do Som:
  - [ ] OK    Fix: 

- **`split`**
  - EN: Split
  - PT: Dividir
  - [ ] OK    Fix: 

- **`stageDepth`**
  - EN: Depth:
  - PT: Profundidade:
  - [ ] OK    Fix: 

- **`stageDiameter`**
  - EN: Diameter:
  - PT: Diâmetro:
  - [ ] OK    Fix: 

- **`stageHeight`**
  - EN: Height:
  - PT: Altura:
  - [ ] OK    Fix: 

- **`stageShape`**
  - EN: Stage Shape:
  - PT: Forma do Palco:
  - [ ] OK    Fix: 

- **`stageWidth`**
  - EN: Width:
  - PT: Largura:
  - [ ] OK    Fix: 

- **`systemLatency`**
  - EN: System Latency:
  - PT: Latência do Sistema:
  - [ ] OK    Fix: 

- **`temperature`**
  - EN: Temperature:
  - PT: Temperatura:
  - [ ] OK    Fix: 

- **`updateAvailable`**
  - EN: Update {version} available
  - PT: Atualização {version} disponível
  - [ ] OK    Fix: 

- **`version`**
  - EN: Version {version}
  - PT: Versão {version}
  - [ ] OK    Fix: 

## `systemConfig.messages`

- **`configLoaded`**
  - EN: Complete configuration loaded.
  - PT: Configuração completa carregada.
  - [ ] OK    Fix: 

- **`configLoadedFromBackup`**
  - EN: Configuration loaded from backup.
  - PT: Configuração carregada do backup.
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Complete configuration saved.
  - PT: Configuração completa guardada.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - PT: Erro: {error}
  - [ ] OK    Fix: 

- **`languageChanged`**
  - EN: Language changed to: {language} (requires restart for full effect)
  - PT: Idioma alterado para: {language} (requer reinício para efeito completo)
  - [ ] OK    Fix: 

- **`logDirNotFound`**
  - EN: Log directory not found
  - PT: Pasta de logs não encontrada
  - [ ] OK    Fix: 

- **`logsExported`**
  - EN: Logs exported to {path}
  - PT: Logs exportados para {path}
  - [ ] OK    Fix: 

- **`logsExportFailed`**
  - EN: Failed to export logs
  - PT: Falha ao exportar logs
  - [ ] OK    Fix: 

- **`noBackupFilesFound`**
  - EN: No backup files found.
  - PT: Nenhum ficheiro de backup encontrado.
  - [ ] OK    Fix: 

- **`partialLoad`**
  - EN: Partial load: {error}
  - PT: Carregamento parcial: {error}
  - [ ] OK    Fix: 

- **`partialLoadFromBackup`**
  - EN: Partial load from backup: {error}
  - PT: Carregamento parcial do backup: {error}
  - [ ] OK    Fix: 

- **`restartBody`**
  - EN: Please restart the application for the language change to take full effect.
  - PT: Por favor, reinicie o aplicativo para que a alteração de idioma tenha efeito completo.
  - [ ] OK    Fix: 

- **`restartTitle`**
  - EN: Restart Required
  - PT: Reinício necessário
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder first.
  - PT: Por favor, selecione primeiro uma pasta de projeto.
  - [ ] OK    Fix: 

- **`selectLogExportFolder`**
  - EN: Select destination for log export
  - PT: Selecionar destino para exportação de logs
  - [ ] OK    Fix: 

- **`systemConfigExported`**
  - EN: System configuration exported.
  - PT: Configuração do sistema exportada.
  - [ ] OK    Fix: 

- **`systemConfigFileNotFound`**
  - EN: System configuration file not found.
  - PT: Ficheiro de configuração do sistema não encontrado.
  - [ ] OK    Fix: 

- **`systemConfigImported`**
  - EN: System configuration imported.
  - PT: Configuração do sistema importada.
  - [ ] OK    Fix: 

- **`systemConfigLoaded`**
  - EN: System configuration loaded.
  - PT: Configuração do sistema carregada.
  - [ ] OK    Fix: 

- **`systemConfigLoadedFromBackup`**
  - EN: System configuration loaded from backup.
  - PT: Configuração do sistema carregada do backup.
  - [ ] OK    Fix: 

- **`systemConfigSaved`**
  - EN: System configuration saved.
  - PT: Configuração do sistema guardada.
  - [ ] OK    Fix: 

- **`systemInfoCopied`**
  - EN: System info copied to clipboard
  - PT: Informações do sistema copiadas para a área de transferência
  - [ ] OK    Fix: 

## `systemConfig.sections`

- **`binauralRenderer`**
  - EN: Binaural Renderer
  - PT: Renderizador Binaural
  - [ ] OK    Fix: 

- **`controllers`**
  - EN: Controllers
  - PT: Controllers
  - [ ] OK    Fix: 

- **`io`**
  - EN: I/O
  - PT: E/S
  - [ ] OK    Fix: 

- **`master`**
  - EN: Master Section
  - PT: Secção Principal
  - [ ] OK    Fix: 

- **`show`**
  - EN: Show
  - PT: Espetáculo
  - [ ] OK    Fix: 

- **`stage`**
  - EN: Stage
  - PT: Palco
  - [ ] OK    Fix: 

- **`ui`**
  - EN: UI
  - PT: Interface
  - [ ] OK    Fix: 

- **`wfsProcessor`**
  - EN: WFS Processor
  - PT: Processador WFS
  - [ ] OK    Fix: 

## `systemConfig.stageShapes`

- **`box`**
  - EN: Box
  - PT: Caixa
  - [ ] OK    Fix: 

- **`cylinder`**
  - EN: Cylinder
  - PT: Cilindro
  - [ ] OK    Fix: 

- **`dome`**
  - EN: Dome
  - PT: Cúpula
  - [ ] OK    Fix: 

## `tabs`

- **`clusters`**
  - EN: Clusters
  - PT: Clusters
  - [ ] OK    Fix: 

- **`inputs`**
  - EN: Inputs
  - PT: Entradas
  - [ ] OK    Fix: 

- **`map`**
  - EN: Map
  - PT: Mapa
  - [ ] OK    Fix: 

- **`network`**
  - EN: Network
  - PT: Rede
  - [ ] OK    Fix: 

- **`outputs`**
  - EN: Outputs
  - PT: Saídas
  - [ ] OK    Fix: 

- **`reverb`**
  - EN: Reverb
  - PT: Reverberação
  - [ ] OK    Fix: 

- **`systemConfig`**
  - EN: System Config
  - PT: Config. Sistema
  - [ ] OK    Fix: 

## `touchscreens`

- **`button`**
  - EN: Setup
  - PT: Configurar
  - [ ] OK    Fix: 

- **`label`**
  - EN: Touchscreen
  - PT: Tela sensível ao toque
  - [ ] OK    Fix: 

## `units`

- **`celsius`**
  - EN: °C
  - PT: °C
  - [ ] OK    Fix: 

- **`decibelPerMeter`**
  - EN: dB/m
  - PT: dB/m
  - [ ] OK    Fix: 

- **`decibels`**
  - EN: dB
  - PT: dB
  - [ ] OK    Fix: 

- **`degrees`**
  - EN: °
  - PT: °
  - [ ] OK    Fix: 

- **`meters`**
  - EN: m
  - PT: m
  - [ ] OK    Fix: 

- **`metersPerSecond`**
  - EN: m/s
  - PT: m/s
  - [ ] OK    Fix: 

- **`milliseconds`**
  - EN: ms
  - PT: ms
  - [ ] OK    Fix: 

## `wizard.buttons`

- **`back`**
  - EN: Back
  - PT: Anterior
  - [ ] OK    Fix: 

- **`close`**
  - EN: Close
  - PT: Fechar
  - [ ] OK    Fix: 

- **`done`**
  - EN: Done
  - PT: Concluir
  - [ ] OK    Fix: 

- **`gettingStarted`**
  - EN: Getting Started
  - PT: Começar
  - [ ] OK    Fix: 

- **`gettingStartedHelp`**
  - EN: Help cards guiding you through the first parameters to adjust when starting a new project
  - PT: Cartões de ajuda que guiam pelos primeiros parâmetros a ajustar ao iniciar um novo projeto
  - [ ] OK    Fix: 

- **`next`**
  - EN: Next
  - PT: Próximo
  - [ ] OK    Fix: 

- **`skip`**
  - EN: Skip
  - PT: Pular
  - [ ] OK    Fix: 

## `wizard.steps`

- **`audioDevice.description`**
  - EN: Select your audio driver and device, set the sample rate and buffer size. Check the patch routing and test your outputs. Close this window when done.
  - PT: Selecione seu driver e dispositivo de áudio, defina a taxa de amostragem e o tamanho do buffer. Verifique o roteamento e teste suas saídas. Feche esta janela quando terminar.
  - [ ] OK    Fix: 

- **`audioDevice.title`**
  - EN: Configure the Audio Interface
  - PT: Configurar a interface de áudio
  - [ ] OK    Fix: 

- **`audioInterface.description`**
  - EN: Click the button above or press Next to open the Audio Interface window.
  - PT: Clique no botão acima ou pressione Próximo para abrir a janela da interface de áudio.
  - [ ] OK    Fix: 

- **`audioInterface.title`**
  - EN: Open the Audio Interface
  - PT: Abrir a interface de áudio
  - [ ] OK    Fix: 

- **`configureOutputs.description`**
  - EN: Use the array presets and geometry tools to calculate speaker positions for your arrays. Close this window when done.
  - PT: Use os presets de array e ferramentas de geometria para calcular as posições dos alto-falantes. Feche esta janela quando terminar.
  - [ ] OK    Fix: 

- **`configureOutputs.title`**
  - EN: Configure Output Positions
  - PT: Configurar posições de saída
  - [ ] OK    Fix: 

- **`exploreInputs.description`**
  - EN: Click an input on the map to select it, or lasso several to move them together. Drag to position your sources. Zoom with the mouse wheel or pinch gesture, pan with right-drag or two-finger drag. Add inputs, group them into clusters, and shape your sound field. You can also control positions with a keyboard, SpaceMouse, or other external controllers. Have fun!
  - PT: Clique em uma entrada no mapa para selecioná-la, ou use o laço para selecionar várias. Arraste para posicionar suas fontes. Zoom com a roda do mouse ou gesto de pinça, desloque a vista com clique direito ou arraste com dois dedos. Adicione entradas, agrupe-as em clusters e molde seu campo sonoro. Você também pode controlar posições com teclado, SpaceMouse ou outros controladores. Divirta-se!
  - [ ] OK    Fix: 

- **`exploreInputs.title`**
  - EN: Start Creating!
  - PT: Comece a criar!
  - [ ] OK    Fix: 

- **`inputChannels.description`**
  - EN: How many audio sources will you be spatializing?
Set the number of input channels to match your source count.
  - PT: Quantas fontes de áudio você vai espacializar?
Defina o número de canais de entrada de acordo com suas fontes.
  - [ ] OK    Fix: 

- **`inputChannels.title`**
  - EN: Set Input Channels
  - PT: Definir canais de entrada
  - [ ] OK    Fix: 

- **`originPoint.description`**
  - EN: The origin is the reference point for all coordinates. Use the preset buttons or enter custom values. 'Front' places it at the audience edge.
  - PT: A origem é o ponto de referência para todas as coordenadas. Use os botões de predefinição ou insira valores personalizados. 'Front' o posiciona na borda do público.
  - [ ] OK    Fix: 

- **`originPoint.title`**
  - EN: Set the Origin Point
  - PT: Definir o ponto de origem
  - [ ] OK    Fix: 

- **`outputChannels.description`**
  - EN: Set the number of output channels to match your speaker array.
Each output corresponds to one physical speaker.
  - PT: Defina o número de canais de saída de acordo com seu array de alto-falantes.
Cada saída corresponde a um alto-falante físico.
  - [ ] OK    Fix: 

- **`outputChannels.title`**
  - EN: Set Output Channels
  - PT: Definir canais de saída
  - [ ] OK    Fix: 

- **`projectFolder.description`**
  - EN: Choose a folder to store your WFS project files. This will hold configurations, snapshots, IR files, and samples. Click the button to open the folder selector.
  - PT: Escolha uma pasta para armazenar seus arquivos de projeto WFS. Ela conterá configurações, snapshots, arquivos IR e amostras. Clique no botão para abrir o seletor de pastas.
  - [ ] OK    Fix: 

- **`projectFolder.title`**
  - EN: Select a Project Folder
  - PT: Selecionar pasta do projeto
  - [ ] OK    Fix: 

- **`reverbChannels.description`**
  - EN: Reverb channels add room simulation. Set to 0 if you don't need reverb.
  - PT: Canais de reverberação adicionam simulação de sala. Defina como 0 se não precisar de reverberação.
  - [ ] OK    Fix: 

- **`reverbChannels.title`**
  - EN: Set Reverb Channels
  - PT: Definir canais de reverberação
  - [ ] OK    Fix: 

- **`stageConfig.description`**
  - EN: Set the shape and dimensions of your performance space. Choose box, cylinder, or dome, then enter the size in meters.
  - PT: Defina a forma e as dimensões do seu espaço de apresentação. Escolha caixa, cilindro ou domo e insira as medidas em metros.
  - [ ] OK    Fix: 

- **`stageConfig.title`**
  - EN: Define the Stage
  - PT: Definir o palco
  - [ ] OK    Fix: 

- **`startProcessing.description`**
  - EN: You're all set! Long-press the Processing button to start the WFS engine. You can also start the Binaural Renderer for headphone monitoring.
  - PT: Tudo pronto! Mantenha pressionado o botão Processing para iniciar o motor WFS. Você também pode iniciar o renderizador binaural para monitoramento com fones de ouvido.
  - [ ] OK    Fix: 

- **`startProcessing.title`**
  - EN: Start the WFS Engine
  - PT: Iniciar o motor WFS
  - [ ] OK    Fix: 

- **`wizardOfOutZ.description`**
  - EN: Click the Wizard of OutZ button or press Next to open the output array helper.
  - PT: Clique no botão Wizard of OutZ ou pressione Próximo para abrir o assistente de posicionamento.
  - [ ] OK    Fix: 

- **`wizardOfOutZ.title`**
  - EN: Position Your Outputs
  - PT: Posicionar suas saídas
  - [ ] OK    Fix: 


