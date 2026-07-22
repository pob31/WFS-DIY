# Proofreading checklist — Portuguese (Português)

Locale: `pt`  |  Total keys: 687  |  Source: `Resources/lang/en.json` vs `Resources/lang/pt.json`

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

## `audioPatch.dialogs`

- **`unpatchInputsMessage`**
  - EN: Are you sure you want to remove all input patches?
  - PT: Tem a certeza que deseja remover todos os patches de entrada?
  - [ ] OK    Fix: 

- **`unpatchInputsTitle`**
  - EN: Unpatch All Inputs
  - PT: Desligar Todas as Entradas
  - [ ] OK    Fix: 

- **`unpatchOutputsMessage`**
  - EN: Are you sure you want to remove all output patches?
  - PT: Tem a certeza que deseja remover todos os patches de saída?
  - [ ] OK    Fix: 

- **`unpatchOutputsTitle`**
  - EN: Unpatch All Outputs
  - PT: Desligar Todas as Saídas
  - [ ] OK    Fix: 

## `audioPatch.messages`

- **`chooseTestSignal`**
  - EN: Choose a Test Signal to Enable Testing
  - PT: Escolha um Sinal de Teste para Ativar o Modo de Teste
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
  - EN: Select the reference point for cluster transforms: First Input, Barycenter, or Shared Position (all members coincide; scale and rotation apply to per-input offsets).
  - PT: Selecione o ponto de referência para as transformações do cluster: First Input, Baricentro ou Shared Position (todos os membros coincidem; a escala e a rotação aplicam-se aos offsets individuais).
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

## `common`

- **`add`**
  - EN: Add
  - PT: Adicionar
  - [ ] OK    Fix: 

- **`all`**
  - EN: All
  - PT: Todos
  - [ ] OK    Fix: 

- **`apply`**
  - EN: Apply
  - PT: Aplicar
  - [ ] OK    Fix: 

- **`cancel`**
  - EN: Cancel
  - PT: Cancelar
  - [ ] OK    Fix: 

- **`close`**
  - EN: Close
  - PT: Fechar
  - [ ] OK    Fix: 

- **`delete`**
  - EN: Delete
  - PT: Eliminar
  - [ ] OK    Fix: 

- **`disable`**
  - EN: Disable
  - PT: Desativar
  - [ ] OK    Fix: 

- **`edit`**
  - EN: Edit
  - PT: Editar
  - [ ] OK    Fix: 

- **`enable`**
  - EN: Enable
  - PT: Ativar
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - PT: Exportar
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - PT: Importar
  - [ ] OK    Fix: 

- **`load`**
  - EN: Load
  - PT: Carregar
  - [ ] OK    Fix: 

- **`no`**
  - EN: No
  - PT: Não
  - [ ] OK    Fix: 

- **`none`**
  - EN: None
  - PT: Nenhum
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

- **`reload`**
  - EN: Reload
  - PT: Recarregar
  - [ ] OK    Fix: 

- **`remove`**
  - EN: Remove
  - PT: Remover
  - [ ] OK    Fix: 

- **`reset`**
  - EN: Reset
  - PT: Reiniciar
  - [ ] OK    Fix: 

- **`save`**
  - EN: Save
  - PT: Guardar
  - [ ] OK    Fix: 

- **`select`**
  - EN: Select
  - PT: Selecionar
  - [ ] OK    Fix: 

- **`store`**
  - EN: Store
  - PT: Armazenar
  - [ ] OK    Fix: 

- **`yes`**
  - EN: Yes
  - PT: Sim
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

## `help.admOsc`

- **`body`**
  - EN: ADM-OSC is a protocol aiming to improve interoperability for spatial sound. It sends Cartesian positions (X, Y, Z) or polar values (AED for Azimuth, Elevation, Distance) from the console or from a DAW's automation curves.\nData is sent normalised:\n- between -1.0 and 1.0 for X, Y and Z;\n- between 0.0 to 1.0 for distance,\n- between -180° to 180° for Azimuth\n- between -90° to 90° for elevation.\nThe origin point can be moved and the mapping can also be adjusted in different segments for the inner and outer parts of the stage.\nWhen dragging the handles on the graphs, holding the shift key will apply symmetrical adjustments on the opposite side.
  - PT: Mapeamentos ADM-OSC\n\nADM-OSC é um protocolo que visa melhorar a interoperabilidade do som espacial. Envia posições cartesianas (X, Y, Z) ou valores polares (AED para Azimute, Elevação, Distância) do console ou das curvas de automação de uma DAW.\nOs dados são enviados normalizados:\n- entre -1.0 e 1.0 para X, Y e Z;\n- entre 0.0 e 1.0 para distância,\n- entre -180° e 180° para Azimute\n- entre -90° e 90° para elevação.\nO ponto de origem pode ser movido e o mapeamento pode ser ajustado em diferentes segmentos para as partes interna e externa do palco.\nAo arrastar as alças nos gráficos, segurar a tecla Shift aplica ajustes simétricos no lado oposto.
  - [ ] OK    Fix: 

- **`title`**
  - EN: ADM-OSC Mappings
  - PT: ADM-OSC Mappings
  - [ ] OK    Fix: 

## `help.automOtion`

- **`body`**
  - EN: One time movement can also be programmed and triggered manually or triggered by sound level.\nThe coordinates are either relative from the start position or absolute relative to the origin point.\nThe input can either stay at the end position or revert to the starting position.\nInput position cannot be changed while the input is moving this way, but interacting with the input will change the position offset of the input instead.\nFor audio level triggering select the sound level above which the movement will start. When the sound drops below the reset level the movement will be rearmed, ready to be triggered again. New movement automation will not be possible while the input is moving.
  - PT: Movimentos únicos podem ser programados e disparados manualmente ou por nível de som.\nAs coordenadas são relativas da posição inicial ou absolutas em relação ao ponto de origem.\nA entrada pode ficar na posição final ou voltar à posição inicial.\nA posição não pode ser alterada durante o movimento, mas a interação mudará o deslocamento de posição.\nPara disparo por nível de áudio, selecione o limiar. Quando o som cair abaixo do nível de reinício, o movimento será rearmado.
  - [ ] OK    Fix: 

- **`title`**
  - EN: AutomOtion
  - PT: AutomOtion
  - [ ] OK    Fix: 

## `help.binaural`

- **`body`**
  - EN: The Binaural renderer is used for:\n- listening to a rough spatial mix on headphones,\n- creating a mix for stereo output,\n- listening to a single soloed track through the spatial processing.\nThis may take the place of your master mix if it's only feeding headphones and media mix.\nThe position of the listening position may be adjusted in depth from the origin point and in orientation. Delay and level settings allow you to eventually match the sound at the FOH position.
  - PT: Renderizador Binaural\n\nO Binaural Renderer é usado para:\n- ouvir uma mixagem espacial aproximada em fones de ouvido,\n- criar uma mixagem para saída estéreo,\n- ouvir uma faixa solo através do processamento espacial.\nPode substituir sua mixagem master se alimentar apenas fones e mixagem de mídia.\nA posição de escuta pode ser ajustada em profundidade a partir do ponto de origem e em orientação. As configurações de atraso e nível permitem igualar o som na posição FOH.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Binaural Renderer
  - PT: Binaural Renderer
  - [ ] OK    Fix: 

## `help.clusters`

- **`body`**
  - EN: Clusters are groups of inputs that can be manipulated and animated as a whole.\nEach input can only be part of one cluster.\nEach cluster can only have one input with tracking fully enabled. Then this input will become the reference points for the cluster.\nIf no input with tracking is part of the cluster then there are two modes for the reference point of the cluster. Either the first input assigned in the list becomes the reference or the barycentre, in other words the center of gravity or the middle of the shape formed by the assigned inputs.\nAll inputs of the clusters can be moved by dragging the reference point. The individual inputs (other than a first input that would be a reference point) can still be adjusted individually. Dragging an input with tracking activated that is also a reference point for a cluster will affect its position offset and the position of the other inputs of the cluster normally.\nAll inputs in a cluster can be rotated or scaled around the reference point.\nAll clusters can be assigned an animation via an LFO. The positions X, Y and Z, the rotation and scale of the cluster can be controlled. The LFO has a period and a phase setting. Each individual parameter has shape, amplitude, rate and phase. The LFO settings can be assigned to pads for a quick recall. A right click will store the LFO parameters to a pad. Double clicking the top of the pad will allow to edit the name of the preset. Clicking or tapping a pad will recall the settings whether the LFO is running or not, but it will not start it if is isn't. A double click/tap will load and start the LFO.\nAll input clusters share the same set of LFO presets.
  - PT: Clusters são grupos de entradas que podem ser manipulados e animados como um conjunto.\nCada entrada só pode pertencer a um cluster.\nCada cluster pode ter apenas uma entrada com tracking totalmente habilitado, que se torna o ponto de referência.\nSe não houver entrada com tracking, há dois modos de referência: a primeira entrada atribuída ou o baricentro das entradas.\nTodas as entradas podem ser movidas arrastando o ponto de referência. Entradas individuais podem ser ajustadas separadamente. Arrastar uma entrada com tracking ativado que também é ponto de referência afetará seu deslocamento de posição e a posição das outras entradas do cluster normalmente.\nTodas as entradas de um cluster podem ser rotacionadas ou escaladas ao redor do ponto de referência.\nTodos os clusters podem receber animação via LFO. Posições X, Y, Z, rotação e escala podem ser controladas. Configurações LFO podem ser atribuídas a pads. Um clique direito armazenará os parâmetros LFO em um pad. Duplo clique no topo do pad permite editar o nome do preset. Um clique ou toque recupera as configurações esteja o LFO rodando ou não, mas não o iniciará. Um duplo clique/toque carregará e iniciará o LFO.\nTodos os clusters compartilham o mesmo conjunto de presets LFO.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Clusters
  - PT: Clusters
  - [ ] OK    Fix: 

## `help.diagnostics`

- **`body`**
  - EN: The diagnostic tools are hidden by default: long-press the Diagnostics button below to show or hide them. They appear automatically when the previous session did not shut down cleanly.\nTo send feedback or report a problem, click Report Issue: it opens the project's GitHub issue tracker in your browser. Describe what happened, what you expected and the steps to reproduce it, then attach the exported diagnostic data.\nExport Logs copies the useful data to a WFS-DIY-logs folder at the location you choose: the logs of the current and up to five previous sessions, plus the application settings file. Attach this folder (or a zip of it) to your report.\nThe session logs contain start-up information (application version, operating system, CPU, channel counts), project loading, network activity and errors. No audio is ever recorded.\nOpen Log Folder shows the raw session logs on disk (the WFS-DIY/logs folder in the user application data directory), useful to find a specific session.\nCopy System Info puts a short summary on the clipboard — application version, operating system, CPU and the current audio device with its sample rate and buffer size — ready to paste into an issue.
  - PT: As ferramentas de diagnóstico estão ocultas por predefinição: uma pressão longa no botão Diagnostics mostra-as ou oculta-as. Aparecem automaticamente se a sessão anterior não terminou corretamente.\nPara enviar comentários ou reportar um problema, clique em «Reportar problema» (Report Issue): o issue tracker GitHub do projeto abre no navegador. Descreva o que aconteceu, o que esperava e os passos para reproduzir, e anexe os dados de diagnóstico exportados.\n«Exportar logs» (Export Logs) copia os dados úteis para uma pasta WFS-DIY-logs no local que escolher: os logs da sessão atual e de até cinco sessões anteriores, mais o ficheiro de definições da aplicação. Anexe esta pasta (ou um zip dela) ao seu relatório.\nOs logs de sessão contêm informações de arranque (versão da aplicação, sistema operativo, CPU, número de canais), o carregamento de projetos, a atividade de rede e os erros. Nunca é gravado áudio.\n«Abrir pasta de logs» (Open Log Folder) mostra os logs em bruto no disco (pasta WFS-DIY/logs nos dados de aplicação do utilizador), útil para encontrar uma sessão específica.\n«Copiar info do sistema» (Copy System Info) coloca um breve resumo na área de transferência — versão da aplicação, sistema operativo, CPU e o dispositivo de áudio atual com a sua taxa de amostragem e tamanho do buffer — pronto a colar num issue.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Diagnostics & Feedback
  - PT: Diagnóstico e feedback
  - [ ] OK    Fix: 

## `help.floorReflections`

- **`body`**
  - EN: Simulating the floor reflections improves the naturalness of the sound. We don't expect sounds to play in a soundproof anechoic chamber. This setting helps recreate the expected floor reflections.\nThe level of the floor reflections can be adjusted as well as the low cut and the high frequency shelf filters. Diffusion adds a bit of chaos to simulate the unevenness of the floor.
  - PT: Reflexões do Chão\n\nSimular as reflexões do chão melhora a naturalidade do som. Não esperamos que sons sejam reproduzidos em uma câmara anecoica à prova de som. Esta configuração ajuda a recriar as reflexões do chão esperadas.\nO nível das reflexões do chão pode ser ajustado, assim como os filtros de corte baixo e shelf de altas frequências. A difusão adiciona um pouco de caos para simular as irregularidades do chão.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Floor Reflections
  - PT: Floor Reflections
  - [ ] OK    Fix: 

## `help.gradientMap`

- **`body`**
  - EN: Gradient maps allow to control attenuation, height and high frequency filtering (shelf with a smooth slope centered at 1kHz) depending on the X, Y position. For example, you can fade out a sound when entering a certain zone, you can have high frequency roll-off when moving away from the front of the stage, you can automatically adjust the height of an actor even when they are standing on elevated platforms without having to control height manually.\nThere are three layers for attenuation, height and HF shelf. They can be toggled on and off and they also can be hidden. The focused layer will look dimmed if disabled. Unfocused layers will look dimmed if active and only the shape outlines will be visible if they are deactivated.\nEach layer has a mapping control for white and black to adjust the range of the effect. The curve setting adjusts the transition.\nEach layer can have editable shapes (rectangle, ellipse or polygon) with either a single shade of grey, a linear gradient or a radial gradient. End points of the gradients can be adjusted.\nWhen creating a polygon click for each corner. Double-clicking will create a last corner and close the shape.\nDouble-clicking an existing point on a rectangle or a polygon will remove this corner. Double-clicking on a side will add a new point.\nThe scale and rotation of each shape can be edited for its center or from the origin point.\nWhen enabled the corner points of the rectangles and polygons can also be edited individually.\nShapes and layers can be copied to another layer on the same input or any other input.\nGradient map settings are stored in the input files.
  - PT: Mapas de Gradiente\n\nOs mapas de gradiente permitem controlar atenuação, altura e filtragem de alta frequência (shelf com inclinação suave centrada em 1kHz) dependendo da posição X, Y. Por exemplo, você pode atenuar um som ao entrar em uma zona, aplicar roll-off de altas frequências ao se afastar da frente do palco ou ajustar automaticamente a altura de um ator em plataformas elevadas.\nHá três camadas: atenuação, altura e shelf HF. Podem ser ativadas/desativadas e ocultadas.\nCada camada tem controles de mapeamento branco e preto para ajustar o alcance do efeito. A configuração de curva ajusta a transição.\nCada camada pode ter formas editáveis (retângulo, elipse ou polígono) com cinza uniforme, gradiente linear ou radial.\nAo criar um polígono, clique para cada vértice. Duplo clique fecha a forma.\nDuplo clique em um ponto o remove. Duplo clique em um lado adiciona um ponto.\nFormas e camadas podem ser copiadas.\nAs configurações são armazenadas nos arquivos de entrada.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Gradient Maps
  - PT: Gradient Maps
  - [ ] OK    Fix: 

## `help.inputAdvanced`

- **`body`**
  - EN: - Sidelines and Fringe enable muting when an input comes close to the limits of a rectangular stage (except downstage).\n- Tracking can be enabled and the ID of the tracker can be selected. The amount of smoothing on the position can also be adjusted.\n- Maximum Speed can be engaged and the speed limit adjusted. The system will apply a gradual acceleration and deceleration when the movement starts and finishes. When Path is enabled, the system will track the path followed by the input and will not go in a straight line to the final position. This is especially handy if movements have to be operated manually.\n- Height Factor lets you work in 2D, when set to 0%, or full 3D, when set at 100%, and anything in between. It's the ratio of the height in the level and delay calculations. If you wish to use floor reflections, set it to 100% and use the parallax correction of the output parameters.
  - PT: - As Linhas laterais e a Zona de transição permitem o silenciamento quando uma entrada se aproxima das margens de um palco retangular (exceto a margem da frente, lado do público).\n- O Tracking pode ser ativado e o ID do tracker selecionado. A suavização da posição também pode ser ajustada.\n- A Velocidade Máxima pode ser ativada e o limite de velocidade ajustado. O sistema aplicará uma aceleração e desaceleração graduais no início e no fim do movimento. Quando o modo Trajetória está ativo, o sistema seguirá o caminho percorrido pela entrada e não irá em linha reta para a posição final. É particularmente útil se os movimentos forem operados manualmente.\n- O Fator de Altura permite trabalhar em 2D, quando definido a 0%, ou em 3D completo, quando definido a 100%, e tudo no meio. É a proporção da altura nos cálculos de nível e atraso. Se quiser usar Floor Reflections, defina-o a 100% e use a correção de paralaxe nos parâmetros de saída.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Advanced Controls
  - PT: Controles avançados
  - [ ] OK    Fix: 

## `help.inputBasic`

- **`body`**
  - EN: Inputs have a wide variety of settings to account for different situations necessitating realistic sound reinforcement or creative tools for sound design.\n- Input level can be adjusted.\n- Inputs can be delayed or they can try to take into account specific latency (digital processing of wireless transmission or digital effects) and compensate for it to better align the amplification and the acoustic sound.\n- Minimal Latency can be toggled instead of Acoustic Precedence. On the other hand this tries to let the sound out through the system as soon as possible. The system scans this input's feeds to the outputs for lowest delay and subtracts it from all delays and bypasses additional Haas effect. Here the idea would be to beat the acoustic sound on stage to try and place a sound in a slightly different position by altering the location first perceived.\n- The location (position and offset) for any input can be given in Cartesian, Cylindrical or Spherical coordinates independently from the stage shape or other channels.\n- The position can be constrained to the dimensions of the stage in Cartesian coordinates or to a specific distance range in polar coordinates.\n- Flip will take symmetrical position for the given coordinate around the origin point.\n- The joystick and vertical slider allow relative control of the position.\n- Inputs can be assigned to a cluster to group them for coordinated movements.
  - PT: As entradas dispõem de uma grande variedade de configurações para se adaptar a diferentes situações que requerem reforço sonoro realista ou ferramentas criativas para design sonoro.\n- O nível de entrada pode ser ajustado.\n- As entradas podem ser atrasadas ou tentar considerar uma latência específica (processamento digital de transmissão sem fio ou efeitos digitais) e compensá-la para alinhar melhor a amplificação e o som acústico.\n- A Minimal Latency pode ser ativada em vez da Acoustic Precedence. Isso tenta enviar o som através do sistema o mais rápido possível. O sistema analisa os envios desta entrada para as saídas buscando o menor atraso e o subtrai de todos os atrasos, contornando o efeito Haas adicional.\n- A posição (posição e offset) pode ser dada em coordenadas Cartesianas, Cilíndricas ou Esféricas independentemente da forma do palco ou outros canais.\n- A posição pode ser restringida às dimensões do palco em coordenadas Cartesianas ou a uma faixa de distância específica em coordenadas polares.\n- Flip tomará a posição simétrica para a coordenada dada ao redor do ponto de origem.\n- O joystick e o controle deslizante vertical permitem controle relativo da posição.\n- As entradas podem ser atribuídas a um cluster para agrupá-las em movimentos coordenados.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Inputs Basic Parameters
  - PT: Parâmetros básicos de entradas
  - [ ] OK    Fix: 

## `help.inputHF`

- **`body`**
  - EN: When talking away, the timbre of a voice will sound less bright. Reproducing this was the initial goal here even though we usually want to have support for voices when not addressing the audience or in bi-frontal configurations. This can be put to good use for creative effects such as having a brighter reverb on a dampened direct sound.\nThe orientation of the input in azimuth and in pitch can be set as well as the angle where the high frequencies will not be filtered.\nThe HF Shelf will set the maximum attenuation in the back of the input. There is a smooth fade (like a cosine curve) from full brightness in front to damped at the rear.
  - PT: Ao falar virando-se, o timbre de uma voz soa menos brilhante. Reproduzir isso era o objetivo inicial aqui, embora geralmente queiramos ter suporte para vozes quando não se dirigem ao público ou em configurações bi-frontais. Isso pode ser usado para efeitos criativos como ter uma reverberação mais brilhante sobre um som direto atenuado.\nA orientação da entrada em azimute e em elevação pode ser definida assim como o ângulo onde as altas frequências não serão filtradas.\nO HF Shelf definirá a atenuação máxima na parte traseira da entrada. Há um fade suave (como uma curva cosseno) do brilho total na frente à atenuação atrás.
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
  - EN: You can set for each output array a specific attenuation for the selected input.\nYou can mute each send to any output individually. There are macros to speed up the process.
  - PT: Atenuação por array e mutes de saída\n\nVocê pode definir para cada array de saída uma atenuação específica para a entrada selecionada.\nVocê pode mutar cada envio para qualquer saída individualmente. Há macros disponíveis para acelerar o processo.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Array Attenuation and Output Mutes
  - PT: Array Attenuation and Output Mutes
  - [ ] OK    Fix: 

## `help.lfo`

- **`body`**
  - EN: Input position can be automated. The LFO can control X, Y and Z position coordinates individually as well as the rotation of HF directivity (gyrophone).\nAdjust the global period and phase for the LFO.\nFor X, Y and Z coordinates select a shape, amplitude, rate and phase as desired. A circle in the XY plane would have sine for shape for X and for Y and an offset of ±90° between the two. A square would be the same but with keystone shapes.\nInput position can be moved while the LFO is running.
  - PT: A posição de entrada pode ser automatizada. O LFO pode controlar as coordenadas X, Y e Z individualmente e a rotação da diretividade HF (girofone).\nAjuste o período e a fase globais do LFO.\nPara X, Y e Z selecione forma, amplitude, taxa e fase. Um círculo no plano XY usaria forma senoidal para X e Y com ±90° de defasagem.\nA posição pode ser movida enquanto o LFO está funcionando.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Input LFO
  - PT: LFO de Entrada
  - [ ] OK    Fix: 

## `help.liveSource`

- **`body`**
  - EN: A loud source on stage may not need much reinforcement through the nearby speakers. Imagine an opera singer near the edge of the stage. Normally the level distribution would make the level louder near the input position. But if it is already loud enough we should be able to not over-amplify it. This function manages this.\nThe radius and shape describe how to attenuate the level for a specific source for speakers within the radius of influence of this input. There are various shapes: a V shaped linear effect; a U shape for fast decrease within the radius; a tight V effect or a mix of the previous (sine).\nThe attenuation can be constant or level dependent like a local compression reacting to transients and to average RMS level.
  - PT: Atenuador de Fonte Ao Vivo\n\nUma fonte potente no palco pode não precisar de reforço através dos alto-falantes próximos. Imagine um cantor de ópera perto da borda do palco. Normalmente a distribuição de nível tornaria o som mais alto perto da posição de entrada. Mas se já estiver alto o suficiente, não devemos sobre-amplificar. Esta função gerencia isso.\nO raio e a forma descrevem como atenuar o nível para os alto-falantes dentro do raio de influência desta fonte. Há várias formas: um efeito linear em V; um U para diminuição rápida; um V apertado ou uma mistura dos anteriores (seno).\nA atenuação pode ser constante ou dependente de nível, como uma compressão local que reage a transientes e ao nível RMS médio.
  - [ ] OK    Fix: 

- **`legendAttenuation`**
  - EN: attenuation
  - PT: atenuação
  - [ ] OK    Fix: 

- **`legendLinear`**
  - EN: linear
  - PT: linear
  - [ ] OK    Fix: 

- **`legendLog`**
  - EN: log
  - PT: log
  - [ ] OK    Fix: 

- **`legendMaxAttenuation`**
  - EN: maximum attenuation
  - PT: atenuação máxima
  - [ ] OK    Fix: 

- **`legendNoAttenuation`**
  - EN: no attenuation
  - PT: sem atenuação
  - [ ] OK    Fix: 

- **`legendPosition`**
  - EN: position of the source
  - PT: posição da fonte
  - [ ] OK    Fix: 

- **`legendRadius`**
  - EN: radius
  - PT: raio
  - [ ] OK    Fix: 

- **`legendSine`**
  - EN: sine
  - PT: seno
  - [ ] OK    Fix: 

- **`legendSquare`**
  - EN: square x²
  - PT: quadrado x²
  - [ ] OK    Fix: 

- **`title`**
  - EN: Live Source Tamer
  - PT: Live Source Tamer
  - [ ] OK    Fix: 

## `help.map`

- **`body`**
  - EN: - A left click on an input or a cluster will allow to move it by dragging it. A single finger touch will do the same.\n- A left click with the shift key pressed will add or remove inputs to the selection. A double tap and drag will act the same way.\n- A left click drag will draw a selection rectangle to select multiple inputs and clusters at the same time.\n- A left double-click or tap will reset the position offset of the input.\n- A long left click or press with no movement will switch to the input tab with the focus on the selected input on release.\n- A left click away from any input will clear the selection.\n- A right click and drag will pan the view of the map. A two finger drag with no selected input or cluster will do the same if your operating system supports multitouch.\n- The mouse wheel will zoom in and out. A two finger pinch with no selected input or cluster will also zoom in and out.\n- A middle click will reset the view to fit the stage on the map display. There is also a dedicated set of buttons to reset the view to fit all inputs and to fit the stage respectively.\n- Selected inputs and clusters can also be moved with the arrow keys for X and Y and with the PageUp and PageDown keys for height. Hardware controllers can be used too.\n- When an input is touched, a second finger nearby can rotate the input directivity and adjust the height by pinching if your operating system allows multitouch interaction.\n- When a cluster is touched, a second finger nearby can rotate the cluster and scale it by pinching.\n- Inputs, output arrays and the reverb nodes can be hidden on the map.\n- Inputs can also be locked to prevent selecting and moving them on the map. They will still be moved by clusters, network commands, tracking and hardware controllers.\n- All reverb nodes can be moved on the map if this is enabled on the reverb tab. Holding the Ctrl/Cmd key will move each pair of reverb nodes in symmetry.\n- Inputs with offsets, LFO or with speed regulation will have a temporary position marker. But the point of interaction will remain the normal marker.\n- The Live Source Tamer radius will be displayed around input when activated.\n- There is a toggle to display the audio level for the inputs and outputs on the map tab, that's active when the audio processing is running.
  - PT: - Clique esquerdo em uma entrada ou cluster para movê-lo arrastando.\n- Clique esquerdo com Shift adiciona ou remove entradas da seleção.\n- Clique esquerdo arrastado desenha um retângulo de seleção.\n- Duplo clique redefine o deslocamento de posição.\n- Clique longo sem movimento alterna para a aba da entrada selecionada.\n- Clique fora de qualquer entrada limpa a seleção.\n- Clique direito arrastado desloca a vista do mapa. Arrasto com dois dedos também.\n- A roda do mouse faz zoom. Pinça com dois dedos também.\n- Clique do botão do meio redefine a vista.\n- Setas movem X/Y, PageUp/Down a altura.\n- Um segundo dedo pode rotacionar a diretividade e ajustar a altura.\n- Em clusters, um segundo dedo pode rotacionar e escalar.\n- Entradas, arrays de saída e nós de reverberação podem ser ocultados.\n- Entradas podem ser bloqueadas.\n- Nós de reverberação podem ser movidos. Ctrl/Cmd move pares em simetria.\n- O raio do Live Source Tamer é exibido quando ativado.\n- Os níveis de áudio podem ser exibidos no mapa.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Map
  - PT: Mapa
  - [ ] OK    Fix: 

## `help.mcp`

- **`body`**
  - EN: The MCP server lets an AI assistant (Claude Desktop, Claude Code, ChatGPT with custom connectors) read and write the parameters of this WFS-DIY session over a local network connection.\n\nWhat the AI can do:\n• Read live state: channel counts, names, positions, attenuations, EQs, snapshots, clusters, the full parameter surface.\n• Move sources, rename channels, set cluster assignments, adjust the array layout, place outputs and reverbs.\n• Run guided workflows (system tuning walkthroughs, troubleshooting localization, snapshot management) via prepared prompt templates.\n\nOperator controls on this row:\n• AI: ON / OFF — master switch. When OFF every AI tool call is refused; when ON the AI works under the rules below.\n• AI critical actions: blocked / ALLOWED — the destructive actions (deleting snapshots, resetting DSP, changing channel counts) are blocked by default. Click to allow them for 10 minutes; the red fill drains as the window expires, then they auto-block again.\n• Open AI History — scrollable timeline of every recent AI change with per-row undo / redo and a step-by-step cursor.\n• The MCP URL button copies the server URL to the clipboard for AI clients that take a URL directly.\n\nOperator awareness:\n• Every AI action is recorded with origin tags. The AI History window shows the full timeline; per-row × reverses an action with its dependents.\n• If you manually adjust a parameter the AI just moved, the AI is notified and will not blindly retry. You always have the last word.\n• The Cmd/Ctrl+Alt+Z and Cmd/Ctrl+Alt+Y shortcuts undo and redo the last AI change without affecting your manual edits (which use plain Ctrl+Z as usual).\n\nTo add this server to Claude Desktop:\n  1. Open Settings → Developer → Edit Config.\n  2. Paste the JSON snippet below into claude_desktop_config.json (merge into the existing mcpServers block if you already have one).\n  3. Restart Claude Desktop. The server appears as 'wfs-diy' in the tools menu.\n\nTo add to Claude Code, run:\n  claude mcp add wfs-diy <url> -t http\n\nThe URL changes if you switch network interface or if the server falls back to a different port. The URL button on this row always reflects the live URL.
  - PT: Servidor IA / MCP\n\nO MCP Server permite a um assistente de IA (Claude Desktop, Claude Code, ChatGPT com conetores personalizados) ler e escrever os parâmetros desta sessão WFS-DIY através de uma ligação de rede local.\n\nO que a IA pode fazer:\n• Ler o estado em direto: número de canais, nomes, posições, atenuações, EQ, snapshots, clusters, toda a superfície de parâmetros.\n• Mover fontes, renomear canais, definir atribuições de clusters, ajustar a disposição de arrays, posicionar saídas e reverberações.\n• Executar fluxos guiados (assistentes de afinação do sistema, resolução de problemas de localização, gestão de snapshots) através de modelos de prompt preparados.\n\nControlos do operador nesta linha:\n• IA: ON / OFF — interruptor principal. Quando OFF, cada chamada IA é recusada; quando ON, a IA opera segundo as regras abaixo.\n• Ações IA críticas: bloqueadas / PERMITIDAS — as ações destrutivas (eliminar snapshots, repor DSP, alterar o número de canais) estão bloqueadas por defeito. Clicar para as permitir durante 10 minutos; o preenchimento vermelho esvazia ao expirar a janela, depois bloqueia automaticamente.\n• Abrir Histórico IA — cronologia rolável de cada alteração recente da IA com anular/refazer por linha.\n• O botão URL MCP copia o URL do servidor para a área de transferência para clientes IA que aceitem um URL diretamente.\n\nVigilância do operador:\n• Cada ação IA é registada com etiquetas de origem. A janela Histórico IA mostra a cronologia completa; o × por linha reverte uma ação com as suas dependências.\n• Se ajustar manualmente um parâmetro que a IA acabou de mover, a IA é notificada e não retentará às cegas. Tem sempre a última palavra.\n• Os atalhos Cmd/Ctrl+Alt+Z e Cmd/Ctrl+Alt+Y anulam e refazem a última alteração da IA sem afetar as suas edições manuais (que usam Ctrl+Z normal).\n\nPara adicionar este servidor ao Claude Desktop:\n  1. Abrir Definições → Programador → Editar configuração.\n  2. Colar o trecho JSON abaixo em claude_desktop_config.json (fundir no bloco mcpServers existente, se já tiver um).\n  3. Reiniciar o Claude Desktop. O servidor aparece como 'wfs-diy' no menu de ferramentas.\n\nPara adicionar ao Claude Code, executar:\n  claude mcp add wfs-diy <url> -t http\n\nO URL muda se mudar de Network Interface ou se o servidor recorrer a uma porta diferente. O botão URL nesta linha reflete sempre o URL em direto.
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
  - PT: AI / MCP Server
  - [ ] OK    Fix: 

## `help.network`

- **`body`**
  - EN: The system can communicate through several network protocols, (UDP or TCP) using OSC. OSC query can be enabled to allow the clients to discover the possible OSC paths and subscribe to some parameter updates.\nThe IP of the local machine corresponding to the selected network interface is shown. The incoming TCP and UDP ports as well as the OSC Query port.\nThere are a few specialised OSC clients such as:\n- Remote for the Android application for multitouch operation and for remote control.\n- QLab that can send data and that can also be programmed directly from the application.\n- ADM-OSC for control from consoles and DAW (see specific help).\nThe data can be filtered to only allow the data from the recorded IP addresses or to allow any client sending on the correct ports.\nThere is a Log window to see what data comes in or out, filter by the type of protocol, client and so on.\nThere is also a locator function to find a lost remote Android tablet. It will flash and sound the alarm on the missing device.
  - PT: O sistema pode comunicar através de vários protocolos de rede (UDP ou TCP) usando OSC. OSC Query pode ser habilitado para permitir que os clientes descubram os caminhos OSC possíveis e se inscrevam em atualizações de parâmetros.\nO IP da máquina local correspondente à interface de rede selecionada é mostrado. As portas TCP e UDP de entrada e a porta OSC Query.\nExistem alguns clientes OSC especializados como:\n- Remote para a aplicação Android para operação multitoque e controle remoto.\n- QLab que pode enviar dados e ser programado diretamente pela aplicação.\n- ADM-OSC para controle de consoles e DAW (ver ajuda específica).\nOs dados podem ser filtrados. Uma janela de Log mostra os dados de entrada e saída.\nHá também uma função de localização para encontrar tablets Android perdidos.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Network
  - PT: Rede
  - [ ] OK    Fix: 

## `help.outputAdvanced`

- **`body`**
  - EN: There are a few parameters to help you adjust to the acoustic sound.\nMost of these parameters are set for whole arrays unless the propagation mode is switched to off for this output in the array. Relative change can also be selected after a specific setting.\n- Orientation and On/Off Angles define what inputs each speaker will amplify. By default the speakers are pointing to the audience, away from the stage. Inputs in the green sector will be amplified, but not the ones in front of the speaker, in the red sector. There is a fade between both sectors. For sub-bass speakers which usually come in limited numbers and locations, opening all the way to the maximum will allow you to have all inputs possibly picked up by the subwoofers.\n- HF Damping simulates the loss of high frequency with distance. Speakers close to the listeners can have more than speakers away from the stage and the listeners.\n- Distance Attenuation percentage allows you to define if more or less of the attenuation calculated from the distance and parameters of the input is applied. Again for Sub-bass in case you only have two and don't want to lose too much level or have hot-spots it may be wise to lower this setting to 50%.\n- Minimal Latency allows or excludes this speaker from Minimal Latency processing. When allowed, the output is scanned for the smallest calculated delay and, once the setting is engaged on an input, the delay for that input through this speaker is reduced.\n- Live Source Attenuation allows or excludes this speaker from the level reduction of nearby inputs that have this setting enabled. This may not be necessary for speakers away from the audience or for sub-bass.\n- Floor Reflections allows or excludes this speaker from the reflections applied to the signal, such as sub-bass and flown arrays...
  - PT: Existem alguns parâmetros para ajudá-lo a se ajustar ao som acústico.\nA maioria destes parâmetros é definida para arrays inteiros a menos que o modo de propagação esteja desativado para esta saída. Mudança relativa também pode ser selecionada após uma configuração específica.\n- Orientação e Ângulos On/Off definem quais entradas cada alto-falante amplificará. Por padrão os alto-falantes apontam para o público, de costas para o palco. Entradas no setor verde serão amplificadas, mas não as que estão na frente do alto-falante, no setor vermelho. Há um fade entre os dois setores. Para sub-graves, abrir completamente ao máximo permitirá incluir todas as entradas.\n- A Atenuação HF simula a perda de altas frequências com a distância.\n- A porcentagem de Distance attenuation define quanta da atenuação calculada é aplicada. Para sub-graves pode ser prudente reduzir para 50%.\n- A Minimal Latency ativa a varredura do menor atraso calculado.\n- A Atenuação Live Source ativa a redução de nível de entradas próximas.\n- As Reflexões de Piso ativam se as reflexões são aplicadas ao sinal para esta saída como sub-graves e arrays suspensos...
  - [ ] OK    Fix: 

- **`title`**
  - EN: Advanced Parameters
  - PT: Parâmetros avançados
  - [ ] OK    Fix: 

## `help.outputs`

- **`body`**
  - EN: The design of the WFS system has to do with the right choice of equipment and their position. Here is a guide to help you with the design and the tuning of your arrays.\nAn array is a line (straight or curved) of speakers. This is one of the most important concepts in WFS adapted to sound reinforcement and for creative sound design.\nA rule of thumb would be that each listener should hear three speakers of an array to have enough psycho-acoustic cues to feel the direction each sound is coming from. There will be a sweet spot to find between the distance between the speakers and the listeners, their spacing and coverage angle. This is especially true for short range arrays, a.k.a front fills. Speakers with a 120° coverage angle can be spaced by the same distance between the array and the first row of listeners. The number of speakers also depends on the sound pressure level. When placed as a flown array, trapezoidal/asymmetric horns with wide coverage angle (120°) below axis and narrow coverage (60°) on axis will give good coverage for seat rows a bit too far from the ground array and will have enough throw to reach 20m or 30m away while avoiding the walls of the venue where they would create reflections that give away the position of the speakers to the ears of the listeners. Most of the time for larger venues coaxial (elliptical or conical horns) have not enough reach and require one or more delay lines. They are more suited for smaller venues with few rows of seats.\nThe positioning of the speakers in the system can be done through the 'Wizard of OutZ' and the editable presets it has.
  - PT: O design do sistema WFS tem a ver com a escolha certa de equipamentos e seu posicionamento. Aqui está um guia para ajudá-lo com o design e afinação dos seus arrays.\nUm array é uma linha (reta ou curva) de alto-falantes. Este é um dos conceitos mais importantes em WFS adaptado ao reforço sonoro e design sonoro criativo.\nComo regra geral, cada ouvinte deve ouvir três alto-falantes de um array para ter pistas psicoacústicas suficientes para sentir a direção de cada som. Haverá um ponto ótimo entre a distância entre os alto-falantes e os ouvintes, seu espaçamento e ângulo de cobertura. Alto-falantes com ângulo de cobertura de 120° podem ser espaçados pela mesma distância entre o array e a primeira fila. O número também depende do nível de pressão sonora. Como array suspenso, cornetas trapezoidais/assimétricas com ângulo amplo (120°) abaixo do eixo e estreito (60°) no eixo darão boa cobertura e alcance de 20-30m evitando reflexões nas paredes. Alto-falantes coaxiais geralmente não têm alcance suficiente para grandes locais e requerem linhas de atraso.\nO posicionamento pode ser feito através do 'Wizard of OutZ' e seus presets editáveis.
  - [ ] OK    Fix: 

- **`title`**
  - EN: WFS Array Design
  - PT: Design de arrays WFS
  - [ ] OK    Fix: 

## `help.overview`

- **`body`**
  - EN: This WFS spatial sound processor aims to be a tool for natural sound reinforcement and also a creative tool opening new avenues for writing sound in space.\nSome parameters are straightforward: place sound (Map, Tracking, Speed Limiting, Gradient Maps...), work its shape (Attenuation Profile) and its acoustic presence (Directivity, Floor Reflections), give it a one-off motion (AutomOtion) or a repetitive movement (L.F.O). In some cases amplification should be limited around loud sources on stage (Live Source Tamer). All of these functionalities can be stored and recalled internally or with the help of QLab. On the other side the system allows real-time interaction to trigger and move samples, move large clusters of inputs manually or thanks to easily recallable LFO presets.
  - PT: Este processador espacial WFS pretende ser uma ferramenta para reforço sonoro natural e também uma ferramenta criativa abrindo novos caminhos para escrever som no espaço.\nAlguns parâmetros são diretos: posicionar som (Mapa, Tracking, Limitação de velocidade, Gradient Maps...), trabalhar sua forma (Perfil de atenuação) e sua presença acústica (Diretividade, Reflexões do piso), dar-lhe um movimento pontual (AutomOtion) ou repetitivo (L.F.O). Em alguns casos a amplificação deve ser limitada ao redor de fontes potentes no palco (Live Source Tamer). Todas essas funcionalidades podem ser armazenadas e recuperadas internamente ou com a ajuda do QLab. Por outro lado o sistema permite interação em tempo real para disparar e mover amostras, mover grandes clusters de entradas manualmente ou graças a presets LFO facilmente recuperáveis.
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
  - EN: Reverb helps blur the actual reflections of the speakers in the acoustic space. Without some reverb the listener might perceive the sound missing some depth because the reflections of the speakers in the acoustic of the room gives away their positions and all sounds might feel as coming from the plane of the speakers.\nPlace the reverb node according to the number of channels and the geometry of the stage and listening area, and the positions of speakers. If necessary the return position can be offset from the feed position.\nOther parameters are very similar to Outputs' and Inputs'.
  - PT: A reverberação ajuda a difundir as reflexões dos alto-falantes.\nPosicione os nós de acordo com a geometria do palco.\nOutros parâmetros são semelhantes aos de Saídas e Entradas.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb
  - PT: Reverberação
  - [ ] OK    Fix: 

## `help.reverbAlgo`

- **`body`**
  - EN: There are three built-in reverb types inside this spatial sound processor:\n- SDN (Scattered Delay Network): The sound bounces between every reverb node which act as reflecting surfaces. This reverb algorithm favours an odd number of nodes that should not have too much symmetry. This helps reduce the artifacts of the reverb or any metallic sounding resonance.\n- FDN (Feedback Delay Network): Each reverb node functions as a separate reverb processor with a classical reverb algorithm. Place nodes around the stage and eventually around the audience. Inputs are sent to each node in a similar fashion as with outputs. The return signal from the reverb is then distributed like other inputs to all outputs.\n- IR (Impulse Response): This is a classical convolution reverb. You can load audio samples as impulse responses. Each reverb node can share the same IR or they can have different ones. Place nodes around the stage and eventually around the audience.\nThe node positions can be adjusted directly on the map. The Ctrl/Cmd key will move a reverb node pair in symmetry.
  - PT: Três tipos de reverberação estão integrados neste processador de som espacial:\n- SDN (Scattered Delay Network): O som reflete entre cada nó de reverberação que atua como superfícies refletoras. Este algoritmo favorece um número ímpar de nós sem muita simetria, para reduzir artefatos ou ressonâncias metálicas.\n- FDN (Feedback Delay Network): Cada nó funciona como um processador de reverberação separado com um algoritmo clássico. Posicione nós ao redor do palco e eventualmente ao redor do público.\n- IR (Resposta ao Impulso): Reverberação por convolução clássica. Você pode carregar amostras de áudio como respostas ao impulso. Cada nó pode compartilhar a mesma IR ou usar diferentes.\nAs posições dos nós podem ser ajustadas diretamente no mapa. A tecla Ctrl/Cmd move um par de nós em simetria.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Algorithms
  - PT: Algoritmos de Reverberação
  - [ ] OK    Fix: 

## `help.reverbFeed`

- **`body`**
  - EN: Pre-processing sending of the input channels to the reverb nodes.\n- Orientation and On/Off Angles define what inputs each reverb node will receive. Inputs in the green sector will be picked-up, but not the ones in front in the red sector. There is a fade between both sectors.\n- HF Damping simulates the loss of high frequency with distance.\n- Distance Attenuation percentage allows you to define if more or less of the attenuation calculated from the distance and parameters of the input is applied.\n- Minimal Latency toggles whether the output is scanned for the smallest calculated delay and also if the setting once engaged on an input will reduce the delay for that input through this reverb node.\n- Live Source Attenuation toggles the reduction of level of nearby input if they have this setting enabled.
  - PT: Alimentação de Reverberação\n\nEnvio de pré-processamento dos Input Channels para os nós.\n- Orientação e ângulos On/Off definem quais entradas cada nó recebe.\n- Amortecimento HF simula a perda de alta frequência.\n- Percentagem de atenuação de distância define a atenuação aplicada.\n- Minimal Latency determina se o menor atraso é usado.\n- Atenuação de Live Source reduz o nível de entradas próximas.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Feed
  - PT: Reverb Feed
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
  - EN: Post-processing sending to the speakers.\n- Distance Attenuation defines the level drop per meter to the speakers.\n- Common Attenuation will look for the lowest attenuation and keep only a percentage of this level drop and apply this across all other outputs.\n- Mutes and Mute Macros allow to prevent a reverb channel from feeding an output. For instance it may not be necessary to send reverb to sub-bass speakers.
  - PT: Retorno de Reverberação\n\nPós-processamento enviado aos alto-falantes.\n- Atenuação de Distância define a queda de nível por metro.\n- Atenuação Comum mantém uma percentagem da menor atenuação.\n- Mutes impedem um canal de reverberação de alimentar uma saída.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Return
  - PT: Reverb Return
  - [ ] OK    Fix: 

## `help.sampler`

- **`body`**
  - EN: The sampler allows to trigger samples and interact with them in real time.\nThe sampler when enabled on a track will replace the live input at all times.\nSeveral samplers can be assigned to different inputs and triggered individually.\nTo use the sampler:\n- Select a Roli Lightpad or a pad on the connected Android Remote app.\n- Add samples to the different tiles in the grid to the left. Adjust their relative starting position and their level and eventually their in and out points. Several samples can be selected using the shift key while clicking.\n- Create sets of samples: selected samples will be added to new sets. Samples can be added or removed after the creation of a set by holding Ctrl/Cmd while clicking on the tiles. Each set can be renamed. Each set can either have a fixed sequence or a random order (round robin, each sample is played once before a new random order is drawn). Each set has an attenuation setting. Each set has a base position applied to the input when selecting the set. It can be moved on the map or using external control. The sample position offset is added to the set position each time a sample is triggered.\n- Press a Roli Lightpad or a pad on the Android app to trigger a sample. The pressure applied to the pad can be mapped to any of the following controls: level, height and high frequency filtering. The sensitivity can be adjusted for each. The movement of the finger on the pad will cause the sound to move. This acts by measuring the deflection from the initial contact point like a joystick. This can be disabled. All sets have their respective settings for the interaction.\nReleasing the pad will stop the triggered sample.\nSampler settings are stored in the input files.\nFor convenience sample tiles and sets can be copied, exported and imported.
  - PT: O sampler permite disparar amostras e interagir com elas em tempo real.\nQuando habilitado em uma faixa, o sampler substitui a entrada ao vivo em todos os momentos.\nVários samplers podem ser atribuídos a diferentes entradas e disparados individualmente.\nPara usar o sampler:\n- Selecione um Roli Lightpad ou um pad no app Android Remote conectado.\n- Adicione amostras aos diferentes blocos na grade. Ajuste a posição inicial relativa, o nível e os pontos de entrada e saída. Várias amostras podem ser selecionadas segurando Shift enquanto clica.\n- Crie conjuntos de amostras: as amostras selecionadas serão adicionadas a novos conjuntos. Podem ser adicionadas ou removidas segurando Ctrl/Cmd enquanto clica nos blocos. Cada conjunto pode ser renomeado e ter uma sequência fixa ou aleatória. Cada conjunto tem uma configuração de atenuação e uma posição base.\n- Pressione um Lightpad ou pad para disparar uma amostra. A pressão pode ser mapeada para nível, altura e filtragem de altas frequências. O movimento do dedo move o som como um joystick.\nSoltar o pad para a amostra.\nAs configurações do sampler são armazenadas nos arquivos de entrada.\nBlocos e conjuntos podem ser copiados, exportados e importados.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Sampler
  - PT: Sampler
  - [ ] OK    Fix: 

## `help.sessionData`

- **`body`**
  - EN: When starting a session select the working folder where the system will place the files and eventual audio files. For new projects create a new folder. When reloading a previous session navigate to the corresponding folder.\nEach section has a separate xml file (System configuration, Network, Outputs, Reverbs, Inputs) and backups. Additionally convolution reverb impulse responses and audio samples will be stored in sub-directories.\nEach section can be stored and recalled individually or as a whole.\nEach section can also export and import files from other projects.
  - PT: Ao iniciar uma sessão, selecione a pasta de trabalho onde o sistema colocará os arquivos e eventuais arquivos de áudio. Para novos projetos, crie uma nova pasta. Para recarregar uma sessão anterior, navegue até a pasta correspondente.\nCada seção tem um arquivo xml separado (Configuração do sistema, Rede, Saídas, Reverberação, Entradas) e backups. Respostas ao impulso de reverberação por convolução e amostras de áudio serão armazenadas em subdiretórios.\nCada seção pode ser armazenada e recuperada individualmente ou como um todo.\nCada seção também pode exportar e importar arquivos de outros projetos.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Session Data
  - PT: Dados da Sessão
  - [ ] OK    Fix: 

## `help.shortcuts`

- **`body`**
  - EN: *H* opens the help card closest to the pointer.\n*I*, *O* and *R* open the Input, Output and Reverb tabs respectively; for a few seconds afterwards you can type a channel number to select it (confirm with *Enter*).\n*N* opens the Network tab.\n*C* opens the Clusters tab.\n*M* opens the Map tab.\n*Spacebar* scrolls to the next channel and *Shift+Spacebar* to the previous one in the Input, Output and Reverb tabs. On the Clusters tab they cycle through the clusters.\n*Ctrl/Cmd* while adjusting a parameter of an output channel that is part of an array adjusts the parameter for the selected channel only, temporarily disabling the propagation to the rest of the array.\n*F1* to *F10* assign inputs to the corresponding cluster in the Input and Map tabs, assign outputs to the corresponding array in the Output tab, and select the corresponding cluster in the Clusters tab. *F11* sets the channel back to Single.\n*Shift* while adjusting a parameter of an input that is part of a cluster adjusts this parameter for the other inputs of the cluster in relative mode: the variation affects all inputs of the cluster, but relative offsets are kept. *Ctrl/Cmd+Shift* changes the parameter in absolute mode: the value becomes identical across all inputs of the cluster.\n*Ctrl/Cmd+Z* undoes the last change; *Ctrl/Cmd+Y* or *Ctrl/Cmd+Shift+Z* redoes it.
  - PT: *H* abre o cartão de ajuda mais próximo do ponteiro.\n*I*, *O* e *R* abrem respetivamente os separadores Inputs (entradas), Outputs (saídas) e Reverb; durante alguns segundos pode depois digitar um número de canal para o selecionar (confirme com *Enter*).\n*N* abre o separador Network (rede).\n*C* abre o separador Clusters.\n*M* abre o separador Map (mapa).\nA *barra de espaço* passa ao canal seguinte e *Shift+Espaço* ao anterior nos separadores Inputs, Outputs e Reverb. No separador Clusters percorrem os clusters.\n*Ctrl/Cmd* enquanto ajusta um parâmetro de uma saída que faz parte de um array ajusta o parâmetro apenas para o canal selecionado, desativando temporariamente a propagação ao resto do array.\n*F1* a *F10* atribuem as entradas ao cluster correspondente nos separadores Inputs e Map, atribuem as saídas ao array correspondente no separador Outputs e selecionam o cluster correspondente no separador Clusters. *F11* devolve o canal a Single.\n*Shift* enquanto ajusta um parâmetro de uma entrada que faz parte de um cluster ajusta esse parâmetro para as outras entradas do cluster em modo relativo: a variação afeta todas as entradas do cluster, mas as diferenças relativas são mantidas. *Ctrl/Cmd+Shift* altera o parâmetro em modo absoluto: o valor torna-se idêntico em todas as entradas do cluster.\n*Ctrl/Cmd+Z* anula a última alteração; *Ctrl/Cmd+Y* ou *Ctrl/Cmd+Shift+Z* volta a aplicá-la.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Keyboard Shortcuts
  - PT: Atalhos de teclado
  - [ ] OK    Fix: 

## `help.snapshotScope`

- **`body`**
  - EN: Snapshots store input parameters, but can have a scope to be recalled during a performance. They can have between all parameters for all inputs and only one parameter for a single channel. They can be updated and renamed for convenience.\nThe Scope tells the system what data to store or recall. It's the opposite of 'safe' parameters.\nThere are several ways to do this in this application:\n- Record only the needed data in local files. The scope filter is applied when storing the data. A recall cue can be created in QLab to trigger the reading of the local file.\n- Record all data and a filter in local files. The scope filter is applied when recalling the data. This allows to eventually recall all data not taking into account the scope filter. This may come in handy when a complete configuration should be recalled during rehearsal for example. A recall cue can be created in QLab to trigger the reading of the local file.\n- Record all data in scope in QLab cues. This should not be used to recall all parameters for large configurations since QLab may stall when recalling so much data.\nThe scope can show and automatically pre-select the parameters that have been manually changed (local UI, hardware controllers, remote Android application). Changed parameters are marked with a yellow mark.
  - PT: Snapshots armazenam parâmetros de entrada, mas podem ter um escopo para serem recuperados durante uma apresentação.\nO Escopo indica ao sistema quais dados armazenar ou recuperar.\nVários métodos estão disponíveis:\n- Gravar apenas os dados necessários em arquivos locais. O filtro é aplicado ao salvar.\n- Gravar todos os dados e um filtro em arquivos locais. O filtro é aplicado ao recuperar.\n- Gravar todos os dados em cues do QLab. Não recomendado para configurações grandes.\nO escopo pode mostrar e pré-selecionar automaticamente os parâmetros modificados manualmente. Alterações são marcadas em amarelo.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Input Snapshots and Scope
  - PT: Snapshots de Entrada e Escopo
  - [ ] OK    Fix: 

## `help.tracking`

- **`body`**
  - EN: Tracking allows to follow the position in 2D or 3D of actors and musicians. There are several tracking solutions based on UWB (UltraWide Band) tags, 3D camera, computer vision systems and Infra-Red LED's followed by IR sensitive cameras.\nThis application allows you to receive tracking data from several protocols: OSC, MQTT, PosiStageNet/PSN, RTTrP.\nYou can select which protocol you're using and enter its settings. You may also adjust the mapping (offset, scaling and orientation).\nEach input has a toggle to enable tracking, an ID to select which tracking marker it should follow and a smoothing algorithm to reduce jittery movements.
  - PT: O tracking permite seguir a posição em 2D ou 3D de atores e músicos. Existem várias soluções baseadas em tags UWB, câmeras 3D, sistemas de visão computacional e LEDs infravermelhos com câmeras sensíveis a IR.\nEsta aplicação permite receber dados de tracking de vários protocolos: OSC, MQTT, PosiStageNet/PSN, RTTrP.\nVocê pode selecionar o protocolo utilizado e configurar suas definições. O mapeamento (offset, escala e orientação) também pode ser ajustado.\nCada entrada tem um toggle para ativar o tracking, um ID para selecionar o marcador e um algoritmo de suavização.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Tracking
  - PT: Tracking
  - [ ] OK    Fix: 

## `help.tuning`

- **`body`**
  - EN: System tuning for WFS is different from standard PA tuning. It can proceed as follows:\n- Start with the flown array muted. Set the desired levels for the near-field speakers when listening to them in the first row. Adjust the high frequency shelf attenuation so the near-field speakers are not too bright and draw too much attention.\n- Mute the near-field array and unmute the flown array, find a suitable level towards the back of the house.\n- Unmute both arrays, adjust the delay of the flown array to bring the sound down to the correct height in the lower rows. Adjust the levels, HF shelf/distance ratio and vertical and horizontal parallax for each array to have consistent level wherever your inputs are on stage. This is especially important if your inputs are actors, singers or acoustic music instruments on stage. You may also test adding some Haas effect to delay the whole system if you feel the WFS is too early or adding comb filtering with the acoustic sound.\nYou may follow a different workflow for tuning or go for different cues.
  - PT: A afinação do sistema WFS é diferente da afinação PA padrão. Pode proceder assim:\n- Comece com o array suspenso mutado. Defina os níveis desejados para os alto-falantes de campo próximo ouvindo-os na primeira fila. Ajuste a atenuação do shelf de alta frequência para que não sejam muito brilhantes.\n- Mute o array de campo próximo e ative o array suspenso, encontre um nível adequado em direção ao fundo da sala.\n- Ative ambos os arrays, ajuste o atraso do array suspenso para trazer o som à altura correta nas filas inferiores. Ajuste níveis, shelf HF/proporção de distância e paralaxe vertical e horizontal para cada array para um nível consistente onde quer que estejam suas entradas no palco.\nVocê pode seguir um fluxo de trabalho diferente para afinação ou buscar configurações diferentes para diferentes situações.
  - [ ] OK    Fix: 

- **`title`**
  - EN: System Tuning
  - PT: Afinação do sistema
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
  - EN: Input {current} has tracking enabled, but Input {existing} in Cluster {cluster} is already tracked.\n\nOnly one tracked input per cluster is allowed.
  - PT: A entrada {current} tem o tracking ativado, mas a entrada {existing} do cluster {cluster} já está a ser rastreada.\n\nApenas é permitida uma entrada com tracking por cluster.
  - [ ] OK    Fix: 

- **`trackingConflictContinue`**
  - EN: Continue (disable tracking)
  - PT: Continuar (desativar tracking)
  - [ ] OK    Fix: 

- **`trackingConflictSwitch`**
  - EN: Input {existing} in Cluster {cluster} already has tracking enabled.\n\nOnly one tracked input per cluster is allowed.\n\nDo you want to disable tracking on Input {existing} and enable it on Input {to}?
  - PT: A entrada {existing} do cluster {cluster} já tem o tracking ativado.\n\nApenas é permitida uma entrada com tracking por cluster.\n\nDeseja desativar o tracking na entrada {existing} e ativá-lo na entrada {to}?
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
  - PT: Modelo da Attenuation Law (decréscimo linear do volume com a distância entre objeto e altifalante, ou quadrática).
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
  - PT: Ativar as Floor Reflections simuladas para o objeto.
  - [ ] OK    Fix: 

- **`frAttenuationSlider`**
  - EN: Attenuation of the Simulated Floor Reflections for the Object.
  - PT: Atenuação das Floor Reflections simuladas para o objeto.
  - [ ] OK    Fix: 

- **`frDiffusionDial`**
  - EN: Diffusion Effect of the Simulated Floor Reflections for the Object.
  - PT: Efeito de difusão das Floor Reflections simuladas para o objeto.
  - [ ] OK    Fix: 

- **`frHighShelfActiveButton`**
  - EN: Enable High Shelf Filter for Floor Reflections.
  - PT: Ativar o filtro High Shelf para as Floor Reflections.
  - [ ] OK    Fix: 

- **`frHighShelfFreqSlider`**
  - EN: High Shelf Frequency for Floor Reflections.
  - PT: Frequência do High Shelf para as Floor Reflections.
  - [ ] OK    Fix: 

- **`frHighShelfGainSlider`**
  - EN: High Shelf Gain for Floor Reflections.
  - PT: Ganho do High Shelf para as Floor Reflections.
  - [ ] OK    Fix: 

- **`frHighShelfSlopeSlider`**
  - EN: High Shelf Slope for Floor Reflections.
  - PT: Inclinação do High Shelf para as Floor Reflections.
  - [ ] OK    Fix: 

- **`frLowCutActiveButton`**
  - EN: Enable Low Cut Filter for Floor Reflections.
  - PT: Ativar o filtro Low Cut para as Floor Reflections.
  - [ ] OK    Fix: 

- **`frLowCutFreqSlider`**
  - EN: Low Cut Frequency for Floor Reflections.
  - PT: Frequência do Low Cut para as Floor Reflections.
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
  - PT: Ativar ou desativar o compressor rápido (pico) do Live Source Tamer.
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
  - PT: Ativar ou desativar o compressor lento do Live Source Tamer.
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
  - PT: Escolher entre Acoustic Precedence e Minimal Latency para a precedência de amplificação.
  - [ ] OK    Fix: 

- **`muteButton`**
  - EN: Mute Output {num} for this Object.
  - PT: Silenciar a saída {num} para este objeto.
  - [ ] OK    Fix: 

- **`muteMacrosSelector`**
  - EN: Mute Macros for Fast Muting and Unmuting of Arrays.
  - PT: Mute Macros para silenciar e reativar rapidamente os arrays.
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

## `inputs.messages`

- **`assignedCluster`**
  - EN: Input {channel} assigned to Cluster {cluster}
  - PT: Entrada {channel} atribuída ao Cluster {cluster}
  - [ ] OK    Fix: 

- **`backupLoaded`**
  - EN: Input configuration loaded from backup.
  - PT: Configuração de entrada carregada do backup.
  - [ ] OK    Fix: 

- **`clusterEditAbsolute`**
  - EN: Ctrl+Shift edit: copying value to {count} other input(s) of Cluster {cluster}
  - PT: Edição com Ctrl+Shift: valor copiado para {count} outra(s) entrada(s) do Cluster {cluster}
  - [ ] OK    Fix: 

- **`clusterEditRelative`**
  - EN: Shift edit: applying relative change to {count} other input(s) of Cluster {cluster}
  - PT: Edição com Shift: alteração relativa aplicada a {count} outra(s) entrada(s) do Cluster {cluster}
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
  - PT: Por favor, selecione primeiro uma pasta de projeto em System Config.
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

- **`snapshotScopeUpdated`**
  - EN: Snapshot '{name}' scope updated.
  - PT: Âmbito do snapshot '{name}' atualizado.
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

## `meta`

- **`author`**
  - EN: WFS-DIY Team
  - PT: WFS-DIY Team
  - [ ] OK    Fix: 

- **`language`**
  - EN: English
  - PT: Português
  - [ ] OK    Fix: 

- **`locale`**
  - EN: en
  - PT: pt
  - [ ] OK    Fix: 

- **`version`**
  - EN: 1.0.0
  - PT: 1.0.0
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
  - EN: \nOnly one tracked input per cluster is allowed. If you continue, tracking will be kept only for the first input in each cluster.
  - PT: \nApenas é permitida uma entrada com tracking por cluster. Se continuar, o tracking será mantido apenas para a primeira entrada de cada cluster.
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

- **`admAxisSwap`**
  - EN: Which incoming ADM-OSC axis maps to this internal axis.
  - PT: Qual eixo ADM-OSC de entrada é mapeado para este eixo interno.
  - [ ] OK    Fix: 

- **`admAzFlip`**
  - EN: Invert the direction of incoming azimuth.
  - PT: Inverter a direção do azimute de entrada.
  - [ ] OK    Fix: 

- **`admAzOffset`**
  - EN: Azimuth offset (deg) applied to incoming ADM-OSC azimuth.
  - PT: Offset de azimute (graus) aplicado ao azimute ADM-OSC de entrada.
  - [ ] OK    Fix: 

- **`admBreakpoint`**
  - EN: Normalized breakpoint (0-1) for piecewise linear stretch.
  - PT: Ponto de ruptura normalizado (0-1) para alongamento linear por partes.
  - [ ] OK    Fix: 

- **`admCenterOffset`**
  - EN: Physical position (m) where normalized 0.0 maps to.
  - PT: Posição física (m) para a qual o valor normalizado 0.0 é mapeado.
  - [ ] OK    Fix: 

- **`admDistMax`**
  - EN: Maximum physical distance (m) at ADM-OSC distance=1.
  - PT: Distância física máxima (m) em ADM-OSC distance=1.
  - [ ] OK    Fix: 

- **`admDistMin`**
  - EN: Minimum physical distance (m) at ADM-OSC distance=0.
  - PT: Distância física mínima (m) em ADM-OSC distance=0.
  - [ ] OK    Fix: 

- **`admElFlip`**
  - EN: Invert the sign of incoming elevation.
  - PT: Inverter o sinal da elevação de entrada.
  - [ ] OK    Fix: 

- **`admInnerWidth`**
  - EN: Physical extent (m) from center to breakpoint.
  - PT: Extensão física (m) do centro ao ponto de ruptura.
  - [ ] OK    Fix: 

- **`admInputAssign`**
  - EN: Assign this input to an ADM-OSC mapping for receive/transmit.
  - PT: Atribuir esta entrada a um mapeamento ADM-OSC para recepção/transmissão.
  - [ ] OK    Fix: 

- **`admLinkAll`**
  - EN: Select all 6 sides at once for uniform editing.
  - PT: Selecionar todos os 6 lados de uma vez para edição uniforme.
  - [ ] OK    Fix: 

- **`admMapping`**
  - EN: Select an ADM-OSC mapping to configure. Cart = Cartesian (xyz), Polar = spherical (aed).
  - PT: Selecionar um mapeamento ADM-OSC para configurar. Cart = Cartesiano (xyz), Polar = esférico (aed).
  - [ ] OK    Fix: 

- **`admMappingPanel`**
  - EN: Drag dots to edit mapping. Click axis title to swap, click Flip to invert. Hold Shift to edit both sides symmetrically.
  - PT: Arrastar pontos para editar o mapeamento. Clicar no título do eixo para trocar, clicar em Flip para inverter. Manter Shift para editar ambos os lados simetricamente.
  - [ ] OK    Fix: 

- **`admOuterWidth`**
  - EN: Physical extent (m) from breakpoint to ±1.
  - PT: Extensão física (m) do ponto de ruptura a ±1.
  - [ ] OK    Fix: 

- **`admSideSelect`**
  - EN: Select sides to edit. Changes apply to all selected sides at once.
  - PT: Selecionar lados para editar. As alterações se aplicam a todos os lados selecionados simultaneamente.
  - [ ] OK    Fix: 

- **`admSignFlip`**
  - EN: Invert the sign of the incoming axis value.
  - PT: Inverter o sinal do valor do eixo de entrada.
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
  - PT: Selecionar a Network Interface.
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
  - PT: OSC Path para o tracking em modo OSC (começa com /)
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
  - PT: Por favor, selecione primeiro uma pasta de projeto em System Config.
  - [ ] OK    Fix: 

## `network.remote`

- **`notResponding`**
  - EN: Remote not responding — the tablet app may be outdated or unreachable
  - PT: (missing — falls back to English)
  - [ ] OK    Fix: 

- **`protocolMismatch`**
  - EN: Remote app uses protocol v{remote}, expected v{local} — update the tablet app
  - PT: (missing — falls back to English)
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

## `networkLog.status`

- **`rejected`**
  - EN: REJECTED
  - PT: REJEITADO
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
  - PT: Frequência Output EQ banda {band} (20 Hz - 20 kHz).
  - [ ] OK    Fix: 

- **`eqGain`**
  - EN: Output EQ Band {band} gain (-24 to +24 dB).
  - PT: Ganho Output EQ banda {band} (-24 a +24 dB).
  - [ ] OK    Fix: 

- **`eqQ`**
  - EN: Output EQ Band {band} Q factor (0.1 - 10.0).
  - PT: Fator Q Output EQ banda {band} (0.1 - 10.0).
  - [ ] OK    Fix: 

- **`eqResetBand`**
  - EN: Long press to reset Band {band} to its default values.
  - PT: Pressão longa para redefinir a banda {band}.
  - [ ] OK    Fix: 

- **`eqShape`**
  - EN: Output EQ Band {band} filter shape.
  - PT: Forma do filtro Output EQ banda {band}.
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Output Configuration to file (with file explorer window).
  - PT: Exportar a configuração de saída para ficheiro (com janela do explorador).
  - [ ] OK    Fix: 

- **`floorReflections`**
  - EN: Allow or exclude this speaker from Floor Reflections.
  - PT: Permitir ou excluir este altifalante das Floor Reflections.
  - [ ] OK    Fix: 

- **`hParallax`**
  - EN: Horizontal Distance from Speaker to 'Targeted' Listener. (changes may affect the rest of the array)
  - PT: Distância horizontal do altifalante ao ouvinte « alvo ». (as alterações podem afetar o resto do array)
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: Loss of High Frequency Depending on Distance from Object to Output. (changes may affect the rest of the array)
  - PT: Perda de altas frequências em função da distância do objeto à saída. (as alterações podem afetar o resto do array)
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Output Configuration from file (with file explorer window).
  - PT: Importar a configuração de saída a partir de ficheiro (com janela do explorador).
  - [ ] OK    Fix: 

- **`liveSource`**
  - EN: Allow or exclude this speaker from Live Source Attenuation. (may affect the rest of the array)
  - PT: Permitir ou excluir este altifalante da atenuação Live Source. (pode afetar o resto do array)
  - [ ] OK    Fix: 

- **`mapVisibility`**
  - EN: Make Visible or Hide The Selected Output on the Map.
  - PT: Tornar visível ou ocultar a saída selecionada no mapa.
  - [ ] OK    Fix: 

- **`minLatency`**
  - EN: Allow or exclude this speaker from Minimal Latency processing. (may affect the rest of the array)
  - PT: Permitir ou excluir este altifalante do processamento Minimal Latency. (pode afetar o resto do array)
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

## `outputs.messages`

- **`arrayEditSingle`**
  - EN: Ctrl edit: change applied to this output only (Array {array} not affected)
  - PT: Edição com Ctrl: alteração aplicada apenas a esta saída (Array {array} não afetado)
  - [ ] OK    Fix: 

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

- **`configNotFound`**
  - EN: Output config file not found.
  - PT: Ficheiro de configuração de saída não encontrado.
  - [ ] OK    Fix: 

- **`configReloaded`**
  - EN: Output configuration reloaded.
  - PT: Configuração de saída recarregada.
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Output configuration saved.
  - PT: Configuração de saída guardada.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - PT: Erro: {error}
  - [ ] OK    Fix: 

- **`noBackup`**
  - EN: No backup output configuration found.
  - PT: Nenhum backup de configuração de saída encontrado.
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - PT: Por favor, selecione primeiro uma pasta de projeto em System Config.
  - [ ] OK    Fix: 

- **`setToSingle`**
  - EN: Output {num} set to Single
  - PT: Saída {num} definida como Individual
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

- **`algoFDNGpu`**
  - EN: Run the FDN reverb on the GPU (adds ~20 ms to the wet path only) or on the CPU. Falls back to CPU automatically if the GPU is unavailable.
  - PT: Executa a reverb FDN na GPU (adiciona ~20 ms apenas ao sinal wet) ou na CPU. Retorna automaticamente à CPU se a GPU não estiver disponível.
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

- **`algoIRGpu`**
  - EN: Run the IR convolution on the GPU (adds ~20 ms to the wet path only) or on the CPU. Falls back to CPU automatically if the GPU is unavailable.
  - PT: Executa a convolução IR na GPU (adiciona ~20 ms apenas ao sinal wet) ou na CPU. Retorna automaticamente à CPU se a GPU não estiver disponível.
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

- **`algoSDNGpu`**
  - EN: Run the SDN reverb on the GPU (adds ~20 ms to the wet path only) or on the CPU. Falls back to CPU automatically if the GPU is unavailable.
  - PT: Executa a reverb SDN na GPU (adiciona ~20 ms apenas ao sinal wet) ou na CPU. Retorna automaticamente à CPU se a GPU não estiver disponível.
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

- **`applyToAllNodes`**
  - EN: When ON, parameter edits are applied to every reverb node at once; when OFF, only the selected node. Runtime only - only future edits propagate.
  - PT: Quando ativado, as alterações de parâmetros aplicam-se a todos os nós de reverberação ao mesmo tempo; quando desativado, apenas ao nó selecionado. Apenas em tempo de execução - só as alterações futuras se propagam.
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
  - PT: Distance attenuation para o Reverb Return (-6.0 a 0.0 dB/m).
  - [ ] OK    Fix: 

- **`distanceAttenEnable`**
  - EN: Distance attenuation percentage (0-200%).
  - PT: Percentagem de Distance attenuation (0-200%).
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
  - EN: Allow or exclude this reverb feed from Live Source Attenuation.
  - PT: Permitir ou excluir este envio de reverberação da atenuação Live Source.
  - [ ] OK    Fix: 

- **`mapVisibility`**
  - EN: Make Visible or Hide All Reverb Channels on the Map.
  - PT: Tornar visíveis ou ocultar todos os Reverb Channels no mapa.
  - [ ] OK    Fix: 

- **`miniLatencyTooltip`**
  - EN: Allow or exclude this reverb feed from Minimal Latency processing.
  - PT: Permitir ou excluir este envio de reverberação do processamento Minimal Latency.
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
  - PT: Por favor, selecione primeiro uma pasta de projeto em System Config.
  - [ ] OK    Fix: 

## `sampler`

- **`guide`**
  - EN: Select a cell on the grid to edit its properties.\nDouble-click to load a sample.\nUse Ctrl+Click to assign cells to the active set.
  - PT: Selecione uma célula na grelha para editar as suas propriedades.\nClique duas vezes para carregar uma amostra.\nUse Ctrl+Clique para atribuir células ao set ativo.
  - [ ] OK    Fix: 

## `sampler.grid`

- **`help`**
  - EN: Click = select\nShift = multi\nCtrl = set toggle\nDblClick = load
  - PT: Clique=selecionar | Shift=multi | Ctrl=alternar set | DblClique=carregar
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

- **`pressHF`**
  - EN: Map finger pressure to high-frequency shelf attenuation
  - PT: Mapear a pressão do dedo na atenuação do shelving agudo
  - [ ] OK    Fix: 

- **`pressHeight`**
  - EN: Map finger pressure to vertical position (Z)
  - PT: Mapear a pressão do dedo na posição vertical (Z)
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
  - EN: Reducing from {current} to {new} input channels will remove settings for channels {start} to {end}.\n\nThis cannot be undone.
  - PT: Reduzir de {current} para {new} canais de entrada irá remover as configurações dos canais {start} a {end}.\n\nEsta ação não pode ser desfeita.
  - [ ] OK    Fix: 

- **`reduceInputChannels.title`**
  - EN: Reduce Input Channels?
  - PT: Reduzir Input Channels?
  - [ ] OK    Fix: 

- **`reduceOutputChannels.message`**
  - EN: Reducing from {current} to {new} output channels will remove settings for channels {start} to {end}.\n\nThis cannot be undone.
  - PT: Reduzir de {current} para {new} canais de saída irá remover as configurações dos canais {start} a {end}.\n\nEsta ação não pode ser desfeita.
  - [ ] OK    Fix: 

- **`reduceOutputChannels.title`**
  - EN: Reduce Output Channels?
  - PT: Reduzir Output Channels?
  - [ ] OK    Fix: 

- **`reduceReverbChannels.message`**
  - EN: Reducing from {current} to {new} reverb channels will remove settings for channels {start} to {end}.\n\nThis cannot be undone.
  - PT: Reduzir de {current} para {new} canais de reverb irá remover as configurações dos canais {start} a {end}.\n\nEsta ação não pode ser desfeita.
  - [ ] OK    Fix: 

- **`reduceReverbChannels.title`**
  - EN: Reduce Reverb Channels?
  - PT: Reduzir Reverb Channels?
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

- **`clearSolo`**
  - EN: Clear all input solo states.
  - PT: Clear all input solo states.
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
  - EN: Select the hardware controller for dials and buttons: Stream Deck+.
  - PT: Selecionar o controlador físico para botões e mostradores: Stream Deck+.
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

- **`gpuPipelineDepth`**
  - EN: GPU pipeline depth in audio blocks. Adds depth x buffer/sample-rate of constant latency (pre-subtracted from WFS delays) and absorbs GPU stalls of the same length: deeper = immune to desktop/UI hiccups, shallower = lower latency. Applies live. Default 4.
  - PT: Profundidade do pipeline GPU em blocos de áudio. Adiciona depth x buffer/taxa de amostragem de latência constante (pré-subtraída dos atrasos WFS) e absorve interrupções da GPU da mesma duração: mais profundo = imune a engasgos do ambiente de trabalho/IU, menos profundo = menor latência. Aplicado em tempo real. Predefinição 4.
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
  - PT: Número de Input Channels.
  - [ ] OK    Fix: 

- **`language`**
  - EN: Select the user interface language. Changes take full effect after restarting the application.
  - PT: Selecione o idioma da interface. As alterações terão efeito completo após reiniciar a aplicação.
  - [ ] OK    Fix: 

- **`levelMeter`**
  - EN: Opens the Level Meter Window.
  - PT: Opens the Level Meter Window.
  - [ ] OK    Fix: 

- **`lightpadSetup`**
  - EN: Display the connected Roli Lightpads and allow for splitting them in 4 smaller pads.
  - PT: Exibir os Roli Lightpads conectados e permitir dividi-los em 4 pads menores.
  - [ ] OK    Fix: 

- **`masterLevel`**
  - EN: Master Level (affects all outputs).
  - PT: Master Level (afeta todas as saídas).
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
  - PT: Número de Output Channels.
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
  - PT: Número de Reverb Channels.
  - [ ] OK    Fix: 

- **`sampler`**
  - EN: Enable or disable the Sampler feature for input channels.
  - PT: Ativar ou desativar a função Sampler para os canais de entrada. Selecionar o controlador: Lightpad ou Comando.
  - [ ] OK    Fix: 

- **`screenReader`**
  - EN: Enable or disable screen reader announcements. When enabled, parameter names and values are announced on hover, and help text is read after a few seconds.
  - PT: Ativar ou desativar anúncios do leitor de ecrã. Quando ativado, nomes e valores de parâmetros são anunciados ao passar o rato, e texto de ajuda é lido após alguns segundos.
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
  - PT: Speed of Sound (relacionada com a temperatura).
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
  - PT: Temperatura (determina a Speed of Sound).
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

- **`logsExportFailed`**
  - EN: Failed to export logs
  - PT: Falha ao exportar logs
  - [ ] OK    Fix: 

- **`logsExported`**
  - EN: Logs exported to {path}
  - PT: Logs exportados para {path}
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
