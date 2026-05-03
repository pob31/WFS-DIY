# Proofreading checklist — Chinese (中文)

Locale: `zh`  |  Total keys: 1520  |  Source: `Resources/lang/en.json` vs `Resources/lang/zh.json`

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
  - ZH: 已应用
  - [ ] OK    Fix: 

- **`atHead`**
  - EN: (no undone records — at the head)
  - ZH: (无已撤销记录 — 位于开头)
  - [ ] OK    Fix: 

- **`batch`**
  - EN: batch {id}
  - ZH: 批次 {id}
  - [ ] OK    Fix: 

- **`cursorLabel`**
  - EN: ◂  cursor (↑ applied  /  ↓ undone, redoable)
  - ZH: ◂  光标 (↑ 已应用  /  ↓ 已撤销，可重做)
  - [ ] OK    Fix: 

- **`noChanges`**
  - EN: No AI changes yet.
  - ZH: 暂无 AI 修改。
  - [ ] OK    Fix: 

- **`of`**
  - EN: of
  - ZH: /
  - [ ] OK    Fix: 

- **`stepBack`**
  - EN: ⏮ Step Back
  - ZH: ⏮ 上一步
  - [ ] OK    Fix: 

- **`stepForward`**
  - EN: Step Forward ⏭
  - ZH: 下一步 ⏭
  - [ ] OK    Fix: 

- **`undone`**
  - EN: undone
  - ZH: 已撤销
  - [ ] OK    Fix: 

- **`windowTitle`**
  - EN: AI Change History
  - ZH: AI 修改历史
  - [ ] OK    Fix: 

## `ai.server`

- **`copyUrlConfirm`**
  - EN: MCP URL copied to clipboard: {url}
  - ZH: MCP URL 已复制到剪贴板: {url}
  - [ ] OK    Fix: 

- **`label`**
  - EN: MCP Server:
  - ZH: MCP 服务器:
  - [ ] OK    Fix: 

- **`openHistory`**
  - EN: Open AI History
  - ZH: 打开 AI 历史
  - [ ] OK    Fix: 

- **`urlButtonRunning`**
  - EN: http://{host}:{port}/mcp
  - ZH: http://{host}:{port}/mcp
  - [ ] OK    Fix: 

- **`urlButtonStopped`**
  - EN: (server not running)
  - ZH: (服务器未运行)
  - [ ] OK    Fix: 

## `ai.tier`

- **`aiOff`**
  - EN: AI: OFF
  - ZH: AI: OFF
  - [ ] OK    Fix: 

- **`aiOn`**
  - EN: AI: ON
  - ZH: AI: ON
  - [ ] OK    Fix: 

- **`gateAllowed`**
  - EN: AI critical actions: ALLOWED
  - ZH: AI 关键操作: 已允许
  - [ ] OK    Fix: 

- **`gateBlocked`**
  - EN: AI critical actions: blocked
  - ZH: AI 关键操作: 已阻止
  - [ ] OK    Fix: 

- **`tier2AutoOff`**
  - EN: Tier 2 auto-confirm: off
  - ZH: 二级自动确认: 关
  - [ ] OK    Fix: 

- **`tier2AutoOn`**
  - EN: Tier 2 auto-confirm: ON (5 min)
  - ZH: 二级自动确认: ON (5 分钟)
  - [ ] OK    Fix: 

## `ai.toast`

- **`clientPrefix`**
  - EN: Claude
  - ZH: Claude
  - [ ] OK    Fix: 

- **`header`**
  - EN: AI changes
  - ZH: AI 修改
  - [ ] OK    Fix: 

- **`moreOlder`**
  - EN: …and {count} older
  - ZH: …以及更早的 {count} 项
  - [ ] OK    Fix: 

## `ai.tooltips`

- **`aiToggle`**
  - EN: Master switch for the whole MCP integration. When OFF every AI tool call is refused; when ON, normal tier handling applies (the critical-actions toggle still controls destructive calls separately).
  - ZH: 整个 MCP 集成的主开关。OFF 时拒绝每个 AI 工具调用; ON 时应用正常的分级处理 (关键操作开关单独控制破坏性调用)。
  - [ ] OK    Fix: 

- **`gate`**
  - EN: Allow destructive AI actions (channel-count changes, port reconfigure, runDSP, etc.) AND skip the per-call confirm handshake for less-destructive tier-2 actions while open. Acts as a superset of the Tier 2 auto-confirm toggle. The red fill drains over 5 minutes, then auto-blocks again.
  - ZH: 允许破坏性 AI 操作 (通道数更改、端口重新配置、runDSP 等)，并在打开时跳过较少破坏性的二级操作的逐次确认握手。作为二级自动确认开关的超集。红色填充在 5 分钟内消失，然后自动重新阻止。
  - [ ] OK    Fix: 

- **`openHistory`**
  - EN: Open the AI History window: scrollable timeline of every recent AI change with per-row undo / redo and a step-by-step cursor.
  - ZH: 打开 AI 历史窗口: 每个最近 AI 更改的可滚动时间线，每行支持撤销/重做和分步光标。
  - [ ] OK    Fix: 

- **`urlButton`**
  - EN: Click to copy the MCP server URL. Useful for Claude Code (claude mcp add wfs-diy <URL> -t http) or any MCP client that takes a URL. Claude Desktop instead uses the JSON config snippet — open the (?) help card.
  - ZH: 点击复制 MCP 服务器 URL。适用于 Claude Code (claude mcp add wfs-diy <URL> -t http) 或任何接受 URL 的 MCP 客户端。Claude Desktop 改为使用 JSON 配置片段 — 打开 (?) 帮助卡。
  - [ ] OK    Fix: 

## `ai.undo`

- **`errorPrefix`**
  - EN: AI {verb}: {message}
  - ZH: AI {verb}: {message}
  - [ ] OK    Fix: 

- **`successPrefix`**
  - EN: AI {verb}: {description}
  - ZH: AI {verb}: {description}
  - [ ] OK    Fix: 

- **`verbRedo`**
  - EN: redo
  - ZH: 重做
  - [ ] OK    Fix: 

- **`verbUndo`**
  - EN: undo
  - ZH: 撤销
  - [ ] OK    Fix: 

## `arrayHelper.acoustic`

- **`distanceAtten`**
  - EN: Distance Atten (%):
  - ZH: 距离衰减 (%):
  - [ ] OK    Fix: 

- **`floorReflections`**
  - EN: Floor Reflections
  - ZH: 地板反射
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: HF Damping (dB/m):
  - ZH: 高频衰减 (dB/m):
  - [ ] OK    Fix: 

- **`highCut`**
  - EN: High Cut (Hz):
  - ZH: 高切 (Hz):
  - [ ] OK    Fix: 

- **`hParallax`**
  - EN: H Parallax (m):
  - ZH: 水平视差 (m):
  - [ ] OK    Fix: 

- **`liveSource`**
  - EN: Live Source
  - ZH: 现场声源
  - [ ] OK    Fix: 

- **`lowCut`**
  - EN: Low Cut (Hz):
  - ZH: 低切 (Hz):
  - [ ] OK    Fix: 

- **`vParallax`**
  - EN: V Parallax (m):
  - ZH: 垂直视差 (m):
  - [ ] OK    Fix: 

## `arrayHelper.buttons`

- **`apply`**
  - EN: Apply
  - ZH: 应用
  - [ ] OK    Fix: 

- **`close`**
  - EN: Close
  - ZH: 关闭
  - [ ] OK    Fix: 

## `arrayHelper.errors`

- **`noPositions`**
  - EN: No positions to apply. Check geometry parameters.
  - ZH: 没有可应用的位置。请检查几何参数。
  - [ ] OK    Fix: 

- **`notEnoughOutputs`**
  - EN: Not enough output channels! Need {count} starting from {start}
  - ZH: 输出通道不足! 从 {start} 开始需要 {count} 个
  - [ ] OK    Fix: 

- **`prefix`**
  - EN: Error: 
  - ZH: 错误: 
  - [ ] OK    Fix: 

- **`speakerCountZero`**
  - EN: Number of speakers must be greater than 0
  - ZH: 扬声器数量必须大于0
  - [ ] OK    Fix: 

## `arrayHelper.geometry`

- **`backFacing`**
  - EN: Back Facing
  - ZH: 朝后
  - [ ] OK    Fix: 

- **`centerSpacing`**
  - EN: Center + Spacing
  - ZH: 中心+间距
  - [ ] OK    Fix: 

- **`centerX`**
  - EN: Center X (m):
  - ZH: 中心X (m):
  - [ ] OK    Fix: 

- **`centerY`**
  - EN: Center Y (m):
  - ZH: 中心Y (m):
  - [ ] OK    Fix: 

- **`endpoints`**
  - EN: Endpoints
  - ZH: 端点
  - [ ] OK    Fix: 

- **`endX`**
  - EN: End X (m):
  - ZH: 终点X (m):
  - [ ] OK    Fix: 

- **`endY`**
  - EN: End Y (m):
  - ZH: 终点Y (m):
  - [ ] OK    Fix: 

- **`facingInward`**
  - EN: Facing Inward
  - ZH: 朝内
  - [ ] OK    Fix: 

- **`facingOutward`**
  - EN: Facing Outward
  - ZH: 朝外
  - [ ] OK    Fix: 

- **`frontFacing`**
  - EN: Front Facing
  - ZH: 朝前
  - [ ] OK    Fix: 

- **`nPairs`**
  - EN: N Pairs:
  - ZH: 配对数:
  - [ ] OK    Fix: 

- **`nSpeakers`**
  - EN: N Speakers:
  - ZH: 扬声器数:
  - [ ] OK    Fix: 

- **`orientation`**
  - EN: Orientation (deg):
  - ZH: 朝向 (度):
  - [ ] OK    Fix: 

- **`radius`**
  - EN: Radius (m):
  - ZH: 半径 (m):
  - [ ] OK    Fix: 

- **`sag`**
  - EN: Sag (m):
  - ZH: 下垂 (m):
  - [ ] OK    Fix: 

- **`spacing`**
  - EN: Spacing (m):
  - ZH: 间距 (m):
  - [ ] OK    Fix: 

- **`startAngle`**
  - EN: Start Angle (deg):
  - ZH: 起始角度 (度):
  - [ ] OK    Fix: 

- **`startX`**
  - EN: Start X (m):
  - ZH: 起点X (m):
  - [ ] OK    Fix: 

- **`startY`**
  - EN: Start Y (m):
  - ZH: 起点Y (m):
  - [ ] OK    Fix: 

- **`width`**
  - EN: Width (m):
  - ZH: 宽度 (m):
  - [ ] OK    Fix: 

- **`yEnd`**
  - EN: Y End (m):
  - ZH: Y终点 (m):
  - [ ] OK    Fix: 

- **`yStart`**
  - EN: Y Start (m):
  - ZH: Y起点 (m):
  - [ ] OK    Fix: 

- **`zHeight`**
  - EN: Z Height (m):
  - ZH: Z高度 (m):
  - [ ] OK    Fix: 

## `arrayHelper.presets`

- **`circle`**
  - EN: Circle
  - ZH: 圆形
  - [ ] OK    Fix: 

- **`delayLine`**
  - EN: Delay Line
  - ZH: 延迟线
  - [ ] OK    Fix: 

- **`label`**
  - EN: Preset:
  - ZH: 预设:
  - [ ] OK    Fix: 

- **`mainRoomStraight`**
  - EN: Main Flown Array Straight
  - ZH: 主吊挂阵列直线
  - [ ] OK    Fix: 

- **`nearFieldCurved`**
  - EN: Near Field Array Curved
  - ZH: 近场阵列弯曲
  - [ ] OK    Fix: 

- **`nearFieldStraight`**
  - EN: Near Field Array Straight
  - ZH: 近场阵列直线
  - [ ] OK    Fix: 

- **`subBass`**
  - EN: Sub Bass
  - ZH: 低音炮
  - [ ] OK    Fix: 

- **`surround`**
  - EN: Surround
  - ZH: 环绕
  - [ ] OK    Fix: 

## `arrayHelper.preview`

- **`audience`**
  - EN: Audience
  - ZH: 观众
  - [ ] OK    Fix: 

## `arrayHelper.sections`

- **`acousticDefaults`**
  - EN: Acoustic Defaults
  - ZH: 声学默认值
  - [ ] OK    Fix: 

- **`geometry`**
  - EN: Geometry
  - ZH: 几何
  - [ ] OK    Fix: 

- **`target`**
  - EN: Target
  - ZH: 目标
  - [ ] OK    Fix: 

## `arrayHelper.status`

- **`applied`**
  - EN: Applied {count} speakers to Array {array}. Ready for next array.
  - ZH: 已将 {count} 个扬声器应用到阵列 {array}。准备下一个阵列。
  - [ ] OK    Fix: 

- **`calculated`**
  - EN: Calculated {count} positions
  - ZH: 已计算 {count} 个位置
  - [ ] OK    Fix: 

- **`ready`**
  - EN: Ready
  - ZH: 就绪
  - [ ] OK    Fix: 

## `arrayHelper.target`

- **`array`**
  - EN: Array:
  - ZH: 阵列:
  - [ ] OK    Fix: 

- **`arrayPrefix`**
  - EN: Array
  - ZH: 阵列
  - [ ] OK    Fix: 

- **`startingOutput`**
  - EN: Starting Output:
  - ZH: 起始输出:
  - [ ] OK    Fix: 

## `arrayHelper.window`

- **`contentName`**
  - EN: Output Array Helper
  - ZH: 输出阵列助手
  - [ ] OK    Fix: 

- **`title`**
  - EN: Wizard of OutZ
  - ZH: OutZ向导
  - [ ] OK    Fix: 

## `audioPatch`

- **`windowTitle`**
  - EN: Audio Interface and Patching
  - ZH: 音频接口和路由
  - [ ] OK    Fix: 

## `audioPatch.buttons`

- **`hold`**
  - EN: Hold
  - ZH: 保持
  - [ ] OK    Fix: 

- **`unpatchAll`**
  - EN: Unpatch All
  - ZH: 断开全部
  - [ ] OK    Fix: 

## `audioPatch.deviceSettings`

- **`buttons.controlPanel`**
  - EN: Control Panel
  - ZH: 控制面板
  - [ ] OK    Fix: 

- **`buttons.resetDevice`**
  - EN: Reset Device
  - ZH: 重置设备
  - [ ] OK    Fix: 

- **`labels.bufferSize`**
  - EN: Audio buffer size:
  - ZH: 音频缓冲区大小:
  - [ ] OK    Fix: 

- **`labels.device`**
  - EN: Device:
  - ZH: 设备:
  - [ ] OK    Fix: 

- **`labels.deviceType`**
  - EN: Audio device type:
  - ZH: 音频设备类型:
  - [ ] OK    Fix: 

- **`labels.sampleRate`**
  - EN: Sample rate:
  - ZH: 采样率:
  - [ ] OK    Fix: 

- **`noDevice`**
  - EN: No Device
  - ZH: 无设备
  - [ ] OK    Fix: 

- **`notConfigured`**
  - EN: Not configured
  - ZH: 未配置
  - [ ] OK    Fix: 

## `audioPatch.labels`

- **`interfaceInput`**
  - EN: Audio Interface Input
  - ZH: 音频接口输入
  - [ ] OK    Fix: 

- **`interfaceOutput`**
  - EN: Audio Interface Output
  - ZH: 音频接口输出
  - [ ] OK    Fix: 

- **`processorInputs`**
  - EN: Processor Inputs
  - ZH: 处理器输入
  - [ ] OK    Fix: 

- **`processorOutputs`**
  - EN: Processor Outputs
  - ZH: 处理器输出
  - [ ] OK    Fix: 

## `audioPatch.messages`

- **`chooseTestSignal`**
  - EN: Choose a Test Signal to Enable Testing
  - ZH: 选择测试信号以启用测试模式
  - [ ] OK    Fix: 

## `audioPatch.modes`

- **`patching`**
  - EN: Patching
  - ZH: 路由
  - [ ] OK    Fix: 

- **`scrolling`**
  - EN: Scrolling
  - ZH: 滚动
  - [ ] OK    Fix: 

- **`testing`**
  - EN: Testing
  - ZH: 测试
  - [ ] OK    Fix: 

## `audioPatch.tabs`

- **`deviceSettings`**
  - EN: Device Settings
  - ZH: 设备设置
  - [ ] OK    Fix: 

- **`inputPatch`**
  - EN: Input Patch
  - ZH: 输入路由
  - [ ] OK    Fix: 

- **`outputPatch`**
  - EN: Output Patch
  - ZH: 输出路由
  - [ ] OK    Fix: 

## `audioPatch.testSignal`

- **`labels.frequency`**
  - EN: Frequency:
  - ZH: 频率:
  - [ ] OK    Fix: 

- **`labels.level`**
  - EN: Level:
  - ZH: 电平:
  - [ ] OK    Fix: 

- **`labels.signal`**
  - EN: Signal:
  - ZH: 信号:
  - [ ] OK    Fix: 

- **`types.diracPulse`**
  - EN: Dirac Pulse
  - ZH: 狄拉克脉冲
  - [ ] OK    Fix: 

- **`types.off`**
  - EN: Off
  - ZH: 关闭
  - [ ] OK    Fix: 

- **`types.pinkNoise`**
  - EN: Pink Noise
  - ZH: 粉红噪声
  - [ ] OK    Fix: 

- **`types.pulse`**
  - EN: Pulse
  - ZH: 脉冲
  - [ ] OK    Fix: 

- **`types.sweep`**
  - EN: Sweep
  - ZH: 扫频
  - [ ] OK    Fix: 

- **`types.tone`**
  - EN: Tone
  - ZH: 音调
  - [ ] OK    Fix: 

## `clusters.help`

- **`attenuationSlider`**
  - EN: Adjust attenuation of all cluster inputs (dB).
  - ZH: 调整集群所有输入的衰减（dB）。
  - [ ] OK    Fix: 

- **`exportPresets`**
  - EN: Export all 16 LFO presets to an XML file.
  - ZH: 将全部 16 个 LFO 预设导出到 XML 文件。
  - [ ] OK    Fix: 

- **`importPresets`**
  - EN: Import LFO presets from an XML file.
  - ZH: 从 XML 文件导入 LFO 预设。
  - [ ] OK    Fix: 

- **`inputsVisibility`**
  - EN: Show or hide this cluster's inputs on the Map tab. Hide propagates to new members; removing an input restores its visibility.
  - ZH: 在地图选项卡中显示或隐藏此集群的输入。隐藏会应用于新成员；移除输入将恢复其可见性。
  - [ ] OK    Fix: 

- **`lfoActiveButton`**
  - EN: Enable or Disable the Periodic Movement of the Cluster (LFO).
  - ZH: 启用或禁用集群的周期性运动（LFO）。
  - [ ] OK    Fix: 

- **`lfoAmplitudeRotSlider`**
  - EN: Maximum Rotation Angle (-360 to 360 Degrees).
  - ZH: 最大旋转角度 (-360 至 360 度)。
  - [ ] OK    Fix: 

- **`lfoAmplitudeScaleSlider`**
  - EN: Maximum Scale Factor (0.1x to 10x, Logarithmic).
  - ZH: 最大缩放因子 (0.1× 至 10×，对数)。
  - [ ] OK    Fix: 

- **`lfoAmplitudeXSlider`**
  - EN: Width of Movement in Relation to Cluster Reference Position.
  - ZH: 相对于集群参考位置的宽度运动幅度。
  - [ ] OK    Fix: 

- **`lfoAmplitudeYSlider`**
  - EN: Depth of Movement in Relation to Cluster Reference Position.
  - ZH: 相对于集群参考位置的深度运动幅度。
  - [ ] OK    Fix: 

- **`lfoAmplitudeZSlider`**
  - EN: Height of Movement in Relation to Cluster Reference Position.
  - ZH: 相对于集群参考位置的高度运动幅度。
  - [ ] OK    Fix: 

- **`lfoPeriodDial`**
  - EN: Base Period of the Cluster Movement.
  - ZH: 集群运动的基础周期。
  - [ ] OK    Fix: 

- **`lfoPhaseDial`**
  - EN: Global Phase Offset of the Cluster Movement.
  - ZH: 集群运动的全局相位偏移。
  - [ ] OK    Fix: 

- **`lfoPhaseRotDial`**
  - EN: Phase Offset of the Cluster Rotation.
  - ZH: 集群旋转的相位偏移。
  - [ ] OK    Fix: 

- **`lfoPhaseScaleDial`**
  - EN: Phase Offset of the Cluster Scaling.
  - ZH: 集群缩放的相位偏移。
  - [ ] OK    Fix: 

- **`lfoPhaseXDial`**
  - EN: Phase Offset of the Cluster Movement in Width.
  - ZH: 宽度方向上集群运动的相位偏移。
  - [ ] OK    Fix: 

- **`lfoPhaseYDial`**
  - EN: Phase Offset of the Cluster Movement in Depth.
  - ZH: 深度方向上集群运动的相位偏移。
  - [ ] OK    Fix: 

- **`lfoPhaseZDial`**
  - EN: Phase Offset of the Cluster Movement in Height.
  - ZH: 高度方向上集群运动的相位偏移。
  - [ ] OK    Fix: 

- **`lfoRateRotSlider`**
  - EN: Faster or Slower Rotation in Relation to Base Period.
  - ZH: 相对于基础周期的更快或更慢的旋转。
  - [ ] OK    Fix: 

- **`lfoRateScaleSlider`**
  - EN: Faster or Slower Scaling in Relation to Base Period.
  - ZH: 相对于基础周期的更快或更慢的缩放。
  - [ ] OK    Fix: 

- **`lfoRateXSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Width.
  - ZH: 宽度方向上相对于基础周期的更快或更慢的运动。
  - [ ] OK    Fix: 

- **`lfoRateYSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Depth.
  - ZH: 深度方向上相对于基础周期的更快或更慢的运动。
  - [ ] OK    Fix: 

- **`lfoRateZSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Height.
  - ZH: 高度方向上相对于基础周期的更快或更慢的运动。
  - [ ] OK    Fix: 

- **`lfoShapeRotSelector`**
  - EN: Rotation Behaviour of the Cluster.
  - ZH: 集群的旋转行为。
  - [ ] OK    Fix: 

- **`lfoShapeScaleSelector`**
  - EN: Scale Behaviour of the Cluster.
  - ZH: 集群的缩放行为。
  - [ ] OK    Fix: 

- **`lfoShapeXSelector`**
  - EN: Movement Behaviour of the Cluster in Width.
  - ZH: 宽度方向上集群运动的行为。
  - [ ] OK    Fix: 

- **`lfoShapeYSelector`**
  - EN: Movement Behaviour of the Cluster in Depth.
  - ZH: 深度方向上集群运动的行为。
  - [ ] OK    Fix: 

- **`lfoShapeZSelector`**
  - EN: Movement Behaviour of the Cluster in Height.
  - ZH: 高度方向上集群运动的行为。
  - [ ] OK    Fix: 

- **`planeSelector`**
  - EN: Select the plane for rotation and scale operations (XY, XZ, YZ).
  - ZH: 选择用于旋转和缩放操作的平面（XY、XZ、YZ）。
  - [ ] OK    Fix: 

- **`positionJoystick`**
  - EN: Move all cluster inputs in X/Y. Hold and drag to translate.
  - ZH: 在 X/Y 方向移动集群的所有输入。按住并拖动以平移。
  - [ ] OK    Fix: 

- **`presetTile`**
  - EN: Click: recall preset. Double-click: recall + start. Middle/right-click: store current LFO.
  - ZH: 点击：调用预设。双击：调用 + 启动。中键/右键点击：保存当前 LFO。
  - [ ] OK    Fix: 

- **`qlabPreset`**
  - EN: Create a QLab network cue to recall the last selected LFO preset for the current cluster.
  - ZH: 创建 QLab 网络提示，以调用当前集群最后选择的 LFO 预设。
  - [ ] OK    Fix: 

- **`referenceMode`**
  - EN: Select the reference point for cluster transforms: First Input or Barycenter.
  - ZH: 选择集群变换的参考点：第一个输入或重心。
  - [ ] OK    Fix: 

- **`rotationDial`**
  - EN: Rotate all cluster inputs around the reference point in the selected plane.
  - ZH: 在选定平面内围绕参考点旋转集群的所有输入。
  - [ ] OK    Fix: 

- **`scaleJoystick`**
  - EN: Scale cluster inputs relative to the reference point in the selected plane.
  - ZH: 在选定平面内相对于参考点缩放集群输入。
  - [ ] OK    Fix: 

- **`stopAllLFO`**
  - EN: Stop LFO on all 10 clusters.
  - ZH: 停止全部 10 个集群上的 LFO。
  - [ ] OK    Fix: 

- **`zSlider`**
  - EN: Move all cluster inputs along the Z axis (height).
  - ZH: 沿 Z 轴（高度）移动集群的所有输入。
  - [ ] OK    Fix: 

## `clusters.labels`

- **`assignedInputs`**
  - EN: Assigned Inputs
  - ZH: 已分配输入
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Atten
  - ZH: 衰减
  - [ ] OK    Fix: 

- **`controls`**
  - EN: Controls
  - ZH: 控制
  - [ ] OK    Fix: 

- **`inputPrefix`**
  - EN: Input
  - ZH: 输入
  - [ ] OK    Fix: 

- **`position`**
  - EN: Position
  - ZH: 位置
  - [ ] OK    Fix: 

- **`posPrefix`**
  - EN: Pos:
  - ZH: 位置:
  - [ ] OK    Fix: 

- **`reference`**
  - EN: Reference:
  - ZH: 参考:
  - [ ] OK    Fix: 

- **`rotation`**
  - EN: Rotation
  - ZH: 旋转
  - [ ] OK    Fix: 

- **`scale`**
  - EN: Scale
  - ZH: 缩放
  - [ ] OK    Fix: 

- **`x`**
  - EN: X:
  - ZH: X:
  - [ ] OK    Fix: 

- **`y`**
  - EN: Y:
  - ZH: Y:
  - [ ] OK    Fix: 

- **`z`**
  - EN: Z
  - ZH: Z
  - [ ] OK    Fix: 

## `clusters.lfo`

- **`labels.amplitude`**
  - EN: Amplitude:
  - ZH: 幅度：
  - [ ] OK    Fix: 

- **`labels.angle`**
  - EN: Angle:
  - ZH: 角度：
  - [ ] OK    Fix: 

- **`labels.period`**
  - EN: Period:
  - ZH: 周期：
  - [ ] OK    Fix: 

- **`labels.phase`**
  - EN: Phase:
  - ZH: 相位：
  - [ ] OK    Fix: 

- **`labels.rate`**
  - EN: Rate:
  - ZH: 速率：
  - [ ] OK    Fix: 

- **`labels.ratio`**
  - EN: Ratio:
  - ZH: 比率：
  - [ ] OK    Fix: 

- **`labels.rot`**
  - EN: Rotation
  - ZH: 旋转
  - [ ] OK    Fix: 

- **`labels.scaleLfo`**
  - EN: Scale
  - ZH: 缩放
  - [ ] OK    Fix: 

- **`labels.section`**
  - EN: LFO
  - ZH: LFO
  - [ ] OK    Fix: 

- **`labels.x`**
  - EN: X
  - ZH: X
  - [ ] OK    Fix: 

- **`labels.y`**
  - EN: Y
  - ZH: Y
  - [ ] OK    Fix: 

- **`labels.z`**
  - EN: Z
  - ZH: Z
  - [ ] OK    Fix: 

## `clusters.osc`

- **`lfoActive`**
  - EN: /wfs/cluster/lfoActive <id> <0|1>
  - ZH: /wfs/cluster/lfoActive <id> <0|1>
  - [ ] OK    Fix: 

- **`lfoAmplitudeRot`**
  - EN: /wfs/cluster/lfoAmplitudeRot <id> <degrees>
  - ZH: /wfs/cluster/lfoAmplitudeRot <id> <degrees>
  - [ ] OK    Fix: 

- **`lfoAmplitudeScale`**
  - EN: /wfs/cluster/lfoAmplitudeScale <id> <factor>
  - ZH: /wfs/cluster/lfoAmplitudeScale <id> <factor>
  - [ ] OK    Fix: 

- **`lfoAmplitudeX`**
  - EN: /wfs/cluster/lfoAmplitudeX <id> <meters>
  - ZH: /wfs/cluster/lfoAmplitudeX <id> <meters>
  - [ ] OK    Fix: 

- **`lfoAmplitudeY`**
  - EN: /wfs/cluster/lfoAmplitudeY <id> <meters>
  - ZH: /wfs/cluster/lfoAmplitudeY <id> <meters>
  - [ ] OK    Fix: 

- **`lfoAmplitudeZ`**
  - EN: /wfs/cluster/lfoAmplitudeZ <id> <meters>
  - ZH: /wfs/cluster/lfoAmplitudeZ <id> <meters>
  - [ ] OK    Fix: 

- **`lfoPeriod`**
  - EN: /wfs/cluster/lfoPeriod <id> <seconds>
  - ZH: /wfs/cluster/lfoPeriod <id> <秒>
  - [ ] OK    Fix: 

- **`lfoPhase`**
  - EN: /wfs/cluster/lfoPhase <id> <degrees>
  - ZH: /wfs/cluster/lfoPhase <id> <度>
  - [ ] OK    Fix: 

- **`lfoPhaseRot`**
  - EN: /wfs/cluster/lfoPhaseRot <id> <degrees>
  - ZH: /wfs/cluster/lfoPhaseRot <id> <degrees>
  - [ ] OK    Fix: 

- **`lfoPhaseScale`**
  - EN: /wfs/cluster/lfoPhaseScale <id> <degrees>
  - ZH: /wfs/cluster/lfoPhaseScale <id> <degrees>
  - [ ] OK    Fix: 

- **`lfoPhaseX`**
  - EN: /wfs/cluster/lfoPhaseX <id> <degrees>
  - ZH: /wfs/cluster/lfoPhaseX <id> <degrees>
  - [ ] OK    Fix: 

- **`lfoPhaseY`**
  - EN: /wfs/cluster/lfoPhaseY <id> <degrees>
  - ZH: /wfs/cluster/lfoPhaseY <id> <degrees>
  - [ ] OK    Fix: 

- **`lfoPhaseZ`**
  - EN: /wfs/cluster/lfoPhaseZ <id> <degrees>
  - ZH: /wfs/cluster/lfoPhaseZ <id> <degrees>
  - [ ] OK    Fix: 

- **`lfoPresetRecall`**
  - EN: /wfs/cluster/lfoPresetRecall <clusterId> <presetNumber>
  - ZH: /wfs/cluster/lfoPresetRecall <clusterId> <预设编号>
  - [ ] OK    Fix: 

- **`lfoRateRot`**
  - EN: /wfs/cluster/lfoRateRot <id> <multiplier>
  - ZH: /wfs/cluster/lfoRateRot <id> <multiplier>
  - [ ] OK    Fix: 

- **`lfoRateScale`**
  - EN: /wfs/cluster/lfoRateScale <id> <multiplier>
  - ZH: /wfs/cluster/lfoRateScale <id> <multiplier>
  - [ ] OK    Fix: 

- **`lfoRateX`**
  - EN: /wfs/cluster/lfoRateX <id> <multiplier>
  - ZH: /wfs/cluster/lfoRateX <id> <multiplier>
  - [ ] OK    Fix: 

- **`lfoRateY`**
  - EN: /wfs/cluster/lfoRateY <id> <multiplier>
  - ZH: /wfs/cluster/lfoRateY <id> <multiplier>
  - [ ] OK    Fix: 

- **`lfoRateZ`**
  - EN: /wfs/cluster/lfoRateZ <id> <multiplier>
  - ZH: /wfs/cluster/lfoRateZ <id> <multiplier>
  - [ ] OK    Fix: 

- **`lfoShapeRot`**
  - EN: /wfs/cluster/lfoShapeRot <id> <0-8>
  - ZH: /wfs/cluster/lfoShapeRot <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoShapeScale`**
  - EN: /wfs/cluster/lfoShapeScale <id> <0-8>
  - ZH: /wfs/cluster/lfoShapeScale <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoShapeX`**
  - EN: /wfs/cluster/lfoShapeX <id> <0-8>
  - ZH: /wfs/cluster/lfoShapeX <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoShapeY`**
  - EN: /wfs/cluster/lfoShapeY <id> <0-8>
  - ZH: /wfs/cluster/lfoShapeY <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoShapeZ`**
  - EN: /wfs/cluster/lfoShapeZ <id> <0-8>
  - ZH: /wfs/cluster/lfoShapeZ <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoStopAll`**
  - EN: /wfs/cluster/lfoStopAll
  - ZH: /wfs/cluster/lfoStopAll
  - [ ] OK    Fix: 

## `clusters.planes`

- **`xy`**
  - EN: XY
  - ZH: XY
  - [ ] OK    Fix: 

- **`xz`**
  - EN: XZ
  - ZH: XZ
  - [ ] OK    Fix: 

- **`yz`**
  - EN: YZ
  - ZH: YZ
  - [ ] OK    Fix: 

## `clusters.presets`

- **`export`**
  - EN: Export
  - ZH: 导出
  - [ ] OK    Fix: 

- **`exportDialog`**
  - EN: Export LFO Presets
  - ZH: 导出 LFO 预设
  - [ ] OK    Fix: 

- **`exported`**
  - EN: LFO presets exported.
  - ZH: LFO 预设已导出。
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - ZH: 导入
  - [ ] OK    Fix: 

- **`importDialog`**
  - EN: Import LFO Presets
  - ZH: 导入 LFO 预设
  - [ ] OK    Fix: 

- **`imported`**
  - EN: LFO presets imported.
  - ZH: LFO 预设已导入。
  - [ ] OK    Fix: 

- **`recalled`**
  - EN: LFO preset recalled from tile {n}.
  - ZH: 已从方格 {n} 调用 LFO 预设。
  - [ ] OK    Fix: 

- **`stopAll`**
  - EN: Stop All
  - ZH: 全部停止
  - [ ] OK    Fix: 

- **`stored`**
  - EN: LFO preset stored in tile {n}.
  - ZH: LFO 预设已保存到方格 {n}。
  - [ ] OK    Fix: 

## `clusters.referenceMode`

- **`barycenter`**
  - EN: Barycenter
  - ZH: 重心
  - [ ] OK    Fix: 

- **`firstInput`**
  - EN: First Input
  - ZH: 第一输入
  - [ ] OK    Fix: 

## `clusters.status`

- **`noInputs`**
  - EN: No inputs assigned
  - ZH: 未分配输入
  - [ ] OK    Fix: 

- **`trackedMarker`**
  - EN: [T]
  - ZH: [T]
  - [ ] OK    Fix: 

- **`tracking`**
  - EN: Tracking: Input {num} (overrides reference)
  - ZH: 追踪: 输入 {num} (覆盖参考)
  - [ ] OK    Fix: 

## `clusters.toggles`

- **`inputsHidden`**
  - EN: Inputs: Hidden
  - ZH: 输入：隐藏
  - [ ] OK    Fix: 

- **`inputsVisible`**
  - EN: Inputs: Visible
  - ZH: 输入：可见
  - [ ] OK    Fix: 

- **`lfoOff`**
  - EN: L.F.O: OFF
  - ZH: L.F.O: 关
  - [ ] OK    Fix: 

- **`lfoOn`**
  - EN: L.F.O: ON
  - ZH: L.F.O: 开
  - [ ] OK    Fix: 

## `common`

- **`cancel`**
  - EN: Cancel
  - ZH: 取消
  - [ ] OK    Fix: 

- **`off`**
  - EN: OFF
  - ZH: 关
  - [ ] OK    Fix: 

- **`ok`**
  - EN: OK
  - ZH: 确定
  - [ ] OK    Fix: 

- **`on`**
  - EN: ON
  - ZH: 开
  - [ ] OK    Fix: 

## `eq.buttons`

- **`flattenEQ`**
  - EN: Flatten EQ
  - ZH: 重置EQ
  - [ ] OK    Fix: 

- **`resetBand`**
  - EN: Reset
  - ZH: 重置
  - [ ] OK    Fix: 

## `eq.filterTypes`

- **`allPass`**
  - EN: All-Pass
  - ZH: 全通
  - [ ] OK    Fix: 

- **`bandPass`**
  - EN: Band Pass
  - ZH: 带通
  - [ ] OK    Fix: 

- **`highCut`**
  - EN: High Cut
  - ZH: 高切
  - [ ] OK    Fix: 

- **`highShelf`**
  - EN: High Shelf
  - ZH: 高频搁架
  - [ ] OK    Fix: 

- **`lowCut`**
  - EN: Low Cut
  - ZH: 低切
  - [ ] OK    Fix: 

- **`lowShelf`**
  - EN: Low Shelf
  - ZH: 低频搁架
  - [ ] OK    Fix: 

- **`peakNotch`**
  - EN: Peak/Notch
  - ZH: 峰值/陷波
  - [ ] OK    Fix: 

## `eq.labels`

- **`band`**
  - EN: Band
  - ZH: 频段
  - [ ] OK    Fix: 

- **`freq`**
  - EN: Freq:
  - ZH: 频率:
  - [ ] OK    Fix: 

- **`gain`**
  - EN: Gain
  - ZH: 增益
  - [ ] OK    Fix: 

- **`q`**
  - EN: Q
  - ZH: Q值
  - [ ] OK    Fix: 

## `eq.status`

- **`off`**
  - EN: EQ OFF
  - ZH: EQ 关
  - [ ] OK    Fix: 

- **`on`**
  - EN: EQ ON
  - ZH: EQ 开
  - [ ] OK    Fix: 

## `fileManager.dialogs`

- **`selectProjectFolder`**
  - EN: Select Project Folder
  - ZH: 选择项目文件夹
  - [ ] OK    Fix: 

## `fileManager.errors`

- **`backupNotFound`**
  - EN: Backup not found
  - ZH: 未找到备份
  - [ ] OK    Fix: 

- **`configStateInvalid`**
  - EN: Config state is invalid
  - ZH: 配置状态无效
  - [ ] OK    Fix: 

- **`failedApply`**
  - EN: Failed to apply: {sections}
  - ZH: 应用失败：{sections}
  - [ ] OK    Fix: 

- **`failedCreateFolder`**
  - EN: Failed to create project folder: {path}
  - ZH: 创建项目文件夹失败：{path}
  - [ ] OK    Fix: 

- **`failedCreateValueTree`**
  - EN: Failed to create ValueTree from XML: {path}
  - ZH: 从 XML 创建 ValueTree 失败：{path}
  - [ ] OK    Fix: 

- **`failedCreateXML`**
  - EN: Failed to create XML from state
  - ZH: 从状态创建 XML 失败
  - [ ] OK    Fix: 

- **`failedParseXML`**
  - EN: Failed to parse XML file: {path}
  - ZH: 解析 XML 文件失败：{path}
  - [ ] OK    Fix: 

- **`failedWriteFile`**
  - EN: Failed to write file: {path}
  - ZH: 写入文件失败：{path}
  - [ ] OK    Fix: 

- **`fileNotFound`**
  - EN: File not found: {path}
  - ZH: 未找到文件：{path}
  - [ ] OK    Fix: 

- **`invalidConfigStructure`**
  - EN: Invalid configuration file structure
  - ZH: 配置文件结构无效
  - [ ] OK    Fix: 

- **`noInputDataInFile`**
  - EN: No input data found in file
  - ZH: 文件中未找到输入数据
  - [ ] OK    Fix: 

- **`noInputDataInSnapshot`**
  - EN: No input data in snapshot
  - ZH: 快照中没有输入数据
  - [ ] OK    Fix: 

- **`noLFOPresetDataInFile`**
  - EN: No LFO preset data found in file
  - ZH: 文件中未找到LFO预设数据
  - [ ] OK    Fix: 

- **`noNetworkDataInFile`**
  - EN: No network data found in file
  - ZH: 文件中未找到网络数据
  - [ ] OK    Fix: 

- **`noNetworkSections`**
  - EN: No network sections found in file
  - ZH: 文件中未找到网络部分
  - [ ] OK    Fix: 

- **`noOutputDataInFile`**
  - EN: No output data found in file
  - ZH: 文件中未找到输出数据
  - [ ] OK    Fix: 

- **`noProjectFolder`**
  - EN: No project folder specified
  - ZH: 未指定项目文件夹
  - [ ] OK    Fix: 

- **`noReverbDataInFile`**
  - EN: No reverb data found in file
  - ZH: 文件中未找到混响数据
  - [ ] OK    Fix: 

- **`noSystemDataInFile`**
  - EN: No valid system data found in file: {path}
  - ZH: 文件中未找到有效的系统数据：{path}
  - [ ] OK    Fix: 

- **`noValidProjectFolder`**
  - EN: No valid project folder
  - ZH: 没有有效的项目文件夹
  - [ ] OK    Fix: 

- **`prefixInputs`**
  - EN: Inputs: 
  - ZH: 输入：
  - [ ] OK    Fix: 

- **`prefixNetwork`**
  - EN: Network: 
  - ZH: 网络：
  - [ ] OK    Fix: 

- **`prefixOutputs`**
  - EN: Outputs: 
  - ZH: 输出：
  - [ ] OK    Fix: 

- **`prefixReverbs`**
  - EN: Reverbs: 
  - ZH: 混响：
  - [ ] OK    Fix: 

- **`prefixSystem`**
  - EN: System: 
  - ZH: 系统：
  - [ ] OK    Fix: 

- **`snapshotNotFound`**
  - EN: Snapshot not found
  - ZH: 未找到快照
  - [ ] OK    Fix: 

- **`snapshotNotFoundNamed`**
  - EN: Snapshot not found: {name}
  - ZH: 未找到快照：{name}
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
  - ZH: ADM-OSC是一种旨在提高空间声音互操作性的协议。它从调音台或DAW的自动化曲线发送笛卡尔坐标（X、Y、Z）或极坐标值（AED：方位角、仰角、距离）。
数据以归一化方式发送：
- X、Y和Z在-1.0到1.0之间
- 距离在0.0到1.0之间
- 方位角在-180°到180°之间
- 仰角在-90°到90°之间
原点可以移动，映射也可以针对舞台的内部和外部区域进行不同的分段调整。
拖动图表上的控制点时，按住Shift键将在对侧应用对称调整。
  - [ ] OK    Fix: 

- **`title`**
  - EN: ADM-OSC Mappings
  - ZH: ADM-OSC映射
  - [ ] OK    Fix: 

## `help.automOtion`

- **`body`**
  - EN: One time movement can also be programmed and triggered manually or triggered by sound level.
The coordinates are either relative from the start position or absolute relative to the origin point.
The input can either stay at the end position or revert to the starting position.
Input position cannot be changed while the input is moving this way, but interacting with the input will change the position offset of the input instead.
For audio level triggering select the sound level above which the movement will start. When the sound drops below the reset level the movement will be rearmed, ready to be triggered again. New movement automation will not be possible while the input is moving.
  - ZH: 一次性运动可以编程并手动触发或通过声音电平触发。
坐标可以是相对于起始位置的相对值或相对于原点的绝对值。
输入可以停留在终点位置或返回起始位置。
移动过程中无法更改位置，但与输入的交互将更改位置偏移。
对于音频电平触发，选择运动开始的阈值。当声音降到重置电平以下时，运动将重新准备就绪。
  - [ ] OK    Fix: 

- **`title`**
  - EN: AutomOtion
  - ZH: AutomOtion
  - [ ] OK    Fix: 

## `help.binaural`

- **`body`**
  - EN: The Binaural renderer is used for:
- listening to a rough spatial mix on headphones,
- creating a mix for stereo output,
- listening to a single soloed track through the spatial processing.
This may take the place of your master mix if it's only feeding headphones and media mix.
The position of the listening position may be adjusted in depth from the origin point and in orientation. Delay and level settings allow you to eventually match the sound at the FOH position.
  - ZH: 双耳渲染器用于：
- 通过耳机收听粗略的空间混音
- 创建立体声输出混音
- 通过空间处理收听单独的独奏轨道
如果仅为耳机和媒体混音供电，它可以替代您的主混音。
收听位置可以从原点调整深度和方向。延迟和电平设置可以匹配FOH位置的声音。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Binaural Renderer
  - ZH: 双耳渲染器
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
  - ZH: 集群是可以作为整体进行操作和动画的输入组。
每个输入只能属于一个集群。
每个集群只能有一个完全启用跟踪的输入，该输入成为集群的参考点。
如果没有跟踪输入，参考点有两种模式：列表中第一个分配的输入或分配输入的重心。
拖动参考点可以移动所有输入。单个输入仍然可以单独调整。拖动启用了跟踪且同时是参考点的输入将正常影响其位置偏移和集群其他输入的位置。
集群中的所有输入都可以围绕参考点旋转或缩放。
所有集群都可以通过LFO分配动画。可以控制X、Y、Z位置、旋转和缩放。LFO设置可以分配给触控板以快速调用。右键点击将LFO参数存储到触控板。双击触控板顶部可以编辑预设名称。点击或轻触触控板将调用设置，无论LFO是否正在运行，但不会启动已停止的LFO。双击/双触将加载并启动LFO。
所有集群共享相同的LFO预设集。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Clusters
  - ZH: 集群
  - [ ] OK    Fix: 

## `help.floorReflections`

- **`body`**
  - EN: Simulating the floor reflections improves the naturalness of the sound. We don't expect sounds to play in a soundproof anechoic chamber. This setting helps recreate the expected floor reflections.
The level of the floor reflections can be adjusted as well as the low cut and the high frequency shelf filters. Diffusion adds a bit of chaos to simulate the unevenness of the floor.
  - ZH: 模拟地板反射可以提高声音的自然度。我们不期望声音在隔音消声室中播放。此设置有助于重建预期的地板反射。
地板反射的电平可以调整，同时还有低切和高频搁架滤波器。扩散增加一些随机性来模拟地板的不平整。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Floor Reflections
  - ZH: 地板反射
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
  - ZH: 渐变图允许根据X、Y位置控制衰减、高度和高频滤波（以1kHz为中心的平滑斜坡搁架滤波器）。例如，可以在进入某个区域时淡出声音，在远离舞台前方时应用高频衰减，或自动调整站在高台上的演员的高度。
有三个图层：衰减、高度和HF搁架。它们可以开/关切换和隐藏。
每个图层有白色和黑色的映射控制来调整效果范围。曲线设置调整过渡。
每个图层可以有可编辑的形状（矩形、椭圆或多边形），使用纯灰色、线性渐变或径向渐变。
创建多边形时点击每个角，双击关闭形状。
双击现有点将其删除，双击边添加新点。
形状和图层可以复制到其他图层。
设置存储在输入文件中。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Gradient Maps
  - ZH: 渐变图
  - [ ] OK    Fix: 

## `help.inputAdvanced`

- **`body`**
  - EN: - Sidelines and Fringe enable muting when an input comes close to the limits of a rectangular stage (except downstage).
- Tracking can be enabled and the ID of the tracker can be selected. The amount of smoothing on the position can also be adjusted.
- Maximum Speed can be engaged and the speed limit adjusted. The system will apply a gradual acceleration and deceleration when the movement starts and finishes. When Path is enabled, the system will track the path followed by the input and will not go in a straight line to the final position. This is especially handy if movements have to be operated manually.
- Height Factor lets you work in 2D, when set to 0%, or full 3D, when set at 100%, and anything in between. It's the ratio of the height in the level and delay calculations. If you wish to use floor reflections, set it to 100% and use the parallax correction of the output parameters.
  - ZH: - 边线和过渡区允许在输入接近矩形舞台边界时静音 (观众侧除外)。
- 可以启用跟踪并选择跟踪器 ID。位置平滑度也可以调整。
- 可以启用最大速度并调整速度限制。系统将在运动开始和结束时应用渐进加速和减速。当路径模式激活时，系统将跟随输入所走的路径，而不是直线移动到最终位置。如果运动需要手动操作，这特别有用。
- 高度因子允许在 0% 时以 2D 工作，在 100% 时以完整 3D 工作，以及介于两者之间的所有值。这是高度在电平和延迟计算中的比率。如果您希望使用地板反射，请将其设置为 100%，并使用输出参数中的视差校正。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Advanced Controls
  - ZH: 高级控制
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
  - ZH: 输入具有多种设置，以适应需要逼真扩声或声音设计创意工具的不同情况。
- 输入电平可以调整。
- 输入可以延迟，或可以尝试考虑特定延迟（无线传输或数字效果的数字处理）并进行补偿，以更好地对齐放大和声学声音。
- 可以启用最小延迟来代替声学优先。这会尝试尽可能快地通过系统输出声音。系统扫描此输入到输出的发送以获取最低延迟，并从所有延迟中减去它，同时绕过额外的Haas效应。
- 位置（位置和偏移）可以用直角坐标、圆柱坐标或球坐标给出，与舞台形状或其他通道无关。
- 位置可以在直角坐标中约束到舞台尺寸，或在极坐标中约束到特定距离范围。
- 翻转将在原点周围取给定坐标的对称位置。
- 操纵杆和垂直滑块允许相对位置控制。
- 输入可以分配到群组以进行协调运动。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Inputs Basic Parameters
  - ZH: 输入基本参数
  - [ ] OK    Fix: 

## `help.inputHF`

- **`body`**
  - EN: When talking away, the timbre of a voice will sound less bright. Reproducing this was the initial goal here even though we usually want to have support for voices when not addressing the audience or in bi-frontal configurations. This can be put to good use for creative effects such as having a brighter reverb on a dampened direct sound.
The orientation of the input in azimuth and in pitch can be set as well as the angle where the high frequencies will not be filtered.
The HF Shelf will set the maximum attenuation in the back of the input. There is a smooth fade (like a cosine curve) from full brightness in front to damped at the rear.
  - ZH: 当说话者转身时，声音的音色会变得不那么明亮。再现这一点是这里的最初目标，尽管我们通常希望在不面向观众或双面配置中为声音提供支持。这可以用于创意效果，如在衰减的直达声上有更明亮的混响。
输入的方位角和俯仰角方向可以设置，以及高频不被过滤的角度。
HF Shelf将设置输入背面的最大衰减。从前面的完全明亮到后面的衰减有一个平滑的渐变（如余弦曲线）。
  - [ ] OK    Fix: 

- **`title`**
  - EN: High Frequency Directivity
  - ZH: 高频指向性
  - [ ] OK    Fix: 

## `help.inputLevel`

- **`body`**
  - EN: There are two level attenuation models. One where the level decreases with the distance by a given ratio in dB/m. Otherwise the level is halved each time the distance doubles. This might be more realistic, but it may be too loud close to the source or not give enough focus. The former may be less physically accurate, it offers usually better control to have a more even and stable mix.
  - ZH: 有两种电平衰减模型。一种是电平随距离按给定的dB/m比率降低。另一种是距离每翻倍电平减半。后者可能更逼真，但在声源附近可能太响或不能提供足够的聚焦。前者物理上可能不太精确，但通常能提供更好的控制，使混音更均匀稳定。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Level Adjustments
  - ZH: 电平调整
  - [ ] OK    Fix: 

## `help.inputMutes`

- **`body`**
  - EN: You can set for each output array a specific attenuation for the selected input.
You can mute each send to any output individually. There are macros to speed up the process.
  - ZH: 您可以为每个输出阵列设置所选输入的特定衰减。
您可以单独静音到任何输出的每个发送。有宏可以加速这个过程。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Array Attenuation and Output Mutes
  - ZH: 阵列衰减和输出静音
  - [ ] OK    Fix: 

## `help.lfo`

- **`body`**
  - EN: Input position can be automated. The LFO can control X, Y and Z position coordinates individually as well as the rotation of HF directivity (gyrophone).
Adjust the global period and phase for the LFO.
For X, Y and Z coordinates select a shape, amplitude, rate and phase as desired. A circle in the XY plane would have sine for shape for X and for Y and an offset of ±90° between the two. A square would be the same but with keystone shapes.
Input position can be moved while the LFO is running.
  - ZH: 输入位置可以自动化。LFO可以单独控制X、Y、Z坐标以及HF指向性的旋转（陀螺仪）。
调整LFO的全局周期和相位。
为X、Y、Z分别选择波形、幅度、速率和相位。XY平面的圆形运动使用正弦波形，X和Y之间偏移±90°。正方形使用梯形波形。
LFO运行时可以移动输入位置。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Input LFO
  - ZH: 输入LFO
  - [ ] OK    Fix: 

## `help.liveSource`

- **`body`**
  - EN: A loud source on stage may not need much reinforcement through the nearby speakers. Imagine an opera singer near the edge of the stage. Normally the level distribution would make the level louder near the input position. But if it is already loud enough we should be able to not over-amplify it. This function manages this.
The radius and shape describe how to attenuate the level for a specific source for speakers within the radius of influence of this input. There are various shapes: a V shaped linear effect; a U shape for fast decrease within the radius; a tight V effect or a mix of the previous (sine).
The attenuation can be constant or level dependent like a local compression reacting to transients and to average RMS level.
  - ZH: 舞台上的大音量声源可能不需要通过附近的扬声器进行增强。想象一位靠近舞台边缘的歌剧歌手。通常电平分配会使输入位置附近的电平更大。但如果已经足够大，我们就不应该过度放大。此功能管理这一点。
半径和形状描述了如何为此输入影响半径内的扬声器衰减电平。有多种形状：V形线性效果；U形用于快速衰减；窄V形或前述的混合（正弦）。
衰减可以是恒定的，也可以是电平依赖的，类似于对瞬态和平均RMS电平做出反应的本地压缩。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Live Source Tamer
  - ZH: 现场声源抑制器
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
  - ZH: - 左键点击输入或集群可拖动移动。
- Shift+左键点击可添加或移除选择中的输入。
- 左键拖动绘制选择矩形。
- 双击重置位置偏移。
- 长按无移动将切换到所选输入的标签页。
- 点击输入以外的区域清除选择。
- 右键拖动平移地图视图。两指拖动也可以。
- 滚轮缩放。两指捏合也可以。
- 中键点击重置视图。
- 方向键移动X/Y，PageUp/Down调整高度。
- 第二根手指可以旋转指向性和调整高度。
- 集群上的第二根手指可以旋转和缩放。
- 输入、输出阵列和混响节点可以隐藏。
- 输入可以锁定以防止选择和移动。
- 混响节点可以移动。Ctrl/Cmd对称移动节点对。
- 启用时显示现场声源抑制器半径。
- 可以在地图上显示音频电平。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Map
  - ZH: 地图
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
  - ZH: MCP 服务器允许 AI 助手 (Claude Desktop、Claude Code、带有自定义连接器的 ChatGPT) 通过本地网络连接读取和写入此 WFS-DIY 会话的参数。

AI 可以做的事情:
• 读取实时状态: 通道数、名称、位置、衰减、EQ、快照、集群、整个参数表面。
• 移动声源、重命名通道、设置集群分配、调整阵列布局、放置输出和混响。
• 通过预先准备的提示模板运行引导工作流 (系统调音、定位故障排查、快照管理)。

此行的操作员控制:
• AI: ON / OFF — 主开关。OFF 时拒绝每个 AI 工具调用; ON 时 AI 按照下面的规则工作。
• AI 关键操作: 已阻止 / 已允许 — 破坏性操作 (删除快照、重置 DSP、更改通道数) 默认被阻止。点击允许 10 分钟; 红色填充随窗口到期消失，然后自动重新阻止。
• 打开 AI 历史 — 每个最近 AI 更改的可滚动时间线。
• MCP URL 按钮将服务器 URL 复制到剪贴板，供直接接受 URL 的 AI 客户端使用。

操作员意识:
• 每个 AI 操作都带有来源标签记录。AI 历史窗口显示完整时间线; 每行的 × 撤销操作及其依赖项。
• 如果您手动调整了 AI 刚刚移动的参数，AI 会被通知，不会盲目重试。您始终拥有最终决定权。
• Cmd/Ctrl+Alt+Z 和 Cmd/Ctrl+Alt+Y 快捷键撤销和重做最后的 AI 更改，不影响您的手动编辑 (使用普通 Ctrl+Z)。

要将此服务器添加到 Claude Desktop:
  1. 打开设置 → 开发者 → 编辑配置。
  2. 将下面的 JSON 片段粘贴到 claude_desktop_config.json (如果已经有 mcpServers 块，请合并)。
  3. 重新启动 Claude Desktop。服务器在工具菜单中显示为 'wfs-diy'。

要添加到 Claude Code，运行:
  claude mcp add wfs-diy <url> -t http

如果切换网络接口或服务器回退到不同端口，URL 会更改。此行的 URL 按钮始终反映实时 URL。
  - [ ] OK    Fix: 

- **`copyButton`**
  - EN: Copy Config
  - ZH: 复制配置
  - [ ] OK    Fix: 

- **`copyConfirm`**
  - EN: MCP config JSON copied to clipboard
  - ZH: MCP 配置 JSON 已复制到剪贴板
  - [ ] OK    Fix: 

- **`title`**
  - EN: AI / MCP Server
  - ZH: AI / MCP 服务器
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
  - ZH: 系统可以通过多种网络协议（UDP或TCP）使用OSC进行通信。可以启用OSC Query让客户端发现可用的OSC路径并订阅参数更新。
显示与所选网络接口对应的本地机器IP。传入TCP和UDP端口以及OSC Query端口。
有几个专用OSC客户端：
- Remote：用于多点触控操作和远程控制的Android应用程序。
- QLab：可以发送数据，也可以直接从应用程序编程。
- ADM-OSC：从调音台和DAW控制（参见专用帮助）。
数据可以过滤。日志窗口显示进出数据。
还有一个定位功能用于查找丢失的Android平板电脑。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Network
  - ZH: 网络
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
  - ZH: 有几个参数帮助您调整以适应声学声音。
这些参数大多为整个阵列设置，除非此输出的传播模式被关闭。特定设置后也可以选择相对变化。
- 方向和On/Off角度定义每个扬声器将放大哪些输入。默认情况下扬声器面向观众，背对舞台。绿色扇区中的输入将被放大，但扬声器前方红色扇区中的不会。两个扇区之间有渐变。对于低音炮，完全打开到最大将允许所有输入被包含。
- HF阻尼模拟距离造成的高频损失。
- 距离衰减百分比定义应用多少计算的衰减。对于低音炮，降到50%可能是明智的。
- 最小延迟启用扫描最小计算延迟。
- 现场声源衰减启用附近输入的电平降低。
- 地板反射切换是否将反射应用于此输出的信号，如低音炮和飞行阵列...
  - [ ] OK    Fix: 

- **`title`**
  - EN: Advanced Parameters
  - ZH: 高级参数
  - [ ] OK    Fix: 

## `help.outputs`

- **`body`**
  - EN: The design of the WFS system has to do with the right choice of equipment and their position. Here is a guide to help you with the design and the tuning of your arrays.
An array is a line (straight or curved) of speakers. This is one of the most important concepts in WFS adapted to sound reinforcement and for creative sound design.
A rule of thumb would be that each listener should hear three speakers of an array to have enough psycho-acoustic cues to feel the direction each sound is coming from. There will be a sweet spot to find between the distance between the speakers and the listeners, their spacing and coverage angle. This is especially true for short range arrays, a.k.a front fills. Speakers with a 120° coverage angle can be spaced by the same distance between the array and the first row of listeners. The number of speakers also depends on the sound pressure level. When placed as a flown array, trapezoidal/asymmetric horns with wide coverage angle (120°) below axis and narrow coverage (60°) on axis will give good coverage for seat rows a bit too far from the ground array and will have enough throw to reach 20m or 30m away while avoiding the walls of the venue where they would create reflections that give away the position of the speakers to the ears of the listeners. Most of the time for larger venues coaxial (elliptical or conical horns) have not enough reach and require one or more delay lines. They are more suited for smaller venues with few rows of seats.
The positioning of the speakers in the system can be done through the 'Wizard of OutZ' and the editable presets it has.
  - ZH: WFS系统的设计涉及正确选择设备及其定位。这是帮助您进行阵列设计和调谐的指南。
阵列是一条（直线或弯曲的）扬声器线。这是WFS中最重要的概念之一。
经验法则是每个听众应该听到阵列的三个扬声器，以获得足够的心理声学线索来感知每个声音的方向。扬声器与听众的距离、间距和覆盖角度之间存在最佳点。120°覆盖角的扬声器可以按阵列与第一排之间的相同距离间隔。数量还取决于声压级。作为飞行阵列，轴下宽角(120°)、轴上窄角(60°)的梯形/非对称号角将提供良好的覆盖和20-30m的到达距离，同时避免墙壁反射。同轴扬声器通常对大型场所没有足够的到达距离，需要延迟线。
定位可以通过'Wizard of OutZ'及其可编辑预设完成。
  - [ ] OK    Fix: 

- **`title`**
  - EN: WFS Array Design
  - ZH: WFS阵列设计
  - [ ] OK    Fix: 

## `help.overview`

- **`body`**
  - EN: This WFS spatial sound processor aims to be a tool for natural sound reinforcement and also a creative tool opening new avenues for writing sound in space.
Some parameters are straightforward: place sound (Map, Tracking, Speed Limiting, Gradient Maps...), work its shape (Attenuation Profile) and its acoustic presence (Directivity, Floor Reflections), give it a one-off motion (AutomOtion) or a repetitive movement (L.F.O). In some cases amplification should be limited around loud sources on stage (Live Source Tamer). All of these functionalities can be stored and recalled internally or with the help of QLab. On the other side the system allows real-time interaction to trigger and move samples, move large clusters of inputs manually or thanks to easily recallable LFO presets.
  - ZH: 这款WFS空间声音处理器旨在成为自然扩声的工具，同时也是一个创意工具，为空间中的声音书写开辟新途径。
一些参数很直观：放置声音（地图、追踪、速度限制、渐变地图...），塑造其形状（衰减曲线）和声学存在感（指向性、地板反射），赋予其一次性运动（AutomOtion）或重复运动（L.F.O）。在某些情况下应限制舞台上大声源周围的放大（Live Source Tamer）。所有这些功能都可以在内部或借助QLab存储和调用。此外系统允许实时交互来触发和移动采样，手动或通过易于调用的LFO预设来移动大型输入群组。
  - [ ] OK    Fix: 

- **`dontShow`**
  - EN: Don't show this again
  - ZH: 不再显示
  - [ ] OK    Fix: 

- **`title`**
  - EN: System Overview
  - ZH: 系统概览
  - [ ] OK    Fix: 

## `help.parallax`

- **`body`**
  - EN: Each speaker points more or less clearly at a listener. To calculate the delay for an input for each speaker, let's consider the distance from the input to this listener, we can also calculate the distance the sound from the speaker to this listener. To match the time of arrival of both we need to apply the difference in the aforementioned distances as a delay for this input and this speaker. This gives greater stability when inputs are moved on stage and especially when they move upstage, away from the edge of the stage. This also can enable the synthesis of floor reflections. This setting can be tuned for finer adjustments, rather than simply measured. Use your ears!
  - ZH: 每个扬声器或多或少地指向一个听众。为了计算每个扬声器的输入延迟，考虑输入到这个听众的距离，我们还可以计算扬声器到这个听众的声音距离。为了匹配两者的到达时间，我们需要将上述距离差作为延迟应用。这在舞台上移动输入时提供更大的稳定性，特别是当它们远离舞台边缘时。这也可以实现地板反射的合成。此设置可以微调，而不是简单地测量。相信您的耳朵！
  - [ ] OK    Fix: 

- **`title`**
  - EN: Parallax Correction
  - ZH: 视差校正
  - [ ] OK    Fix: 

## `help.reverb`

- **`body`**
  - EN: Reverb helps blur the actual reflections of the speakers in the acoustic space. Without some reverb the listener might perceive the sound missing some depth because the reflections of the speakers in the acoustic of the room gives away their positions and all sounds might feel as coming from the plane of the speakers.
Place the reverb node according to the number of channels and the geometry of the stage and listening area, and the positions of speakers. If necessary the return position can be offset from the feed position.
Other parameters are very similar to Outputs' and Inputs'.
  - ZH: 混响有助于模糊扬声器的实际反射。
根据通道数和舞台形状放置混响节点。
其他参数与输出和输入类似。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb
  - ZH: 混响
  - [ ] OK    Fix: 

## `help.reverbAlgo`

- **`body`**
  - EN: There are three built-in reverb types inside this spatial sound processor:
- SDN (Scattered Delay Network): The sound bounces between every reverb node which act as reflecting surfaces. This reverb algorithm favours an odd number of nodes that should not have too much symmetry. This helps reduce the artifacts of the reverb or any metallic sounding resonance.
- FDN (Feedback Delay Network): Each reverb node functions as a separate reverb processor with a classical reverb algorithm. Place nodes around the stage and eventually around the audience. Inputs are sent to each node in a similar fashion as with outputs. The return signal from the reverb is then distributed like other inputs to all outputs.
- IR (Impulse Response): This is a classical convolution reverb. You can load audio samples as impulse responses. Each reverb node can share the same IR or they can have different ones. Place nodes around the stage and eventually around the audience.
The node positions can be adjusted directly on the map. The Ctrl/Cmd key will move a reverb node pair in symmetry.
  - ZH: 此空间声音处理器内置三种混响类型：
- SDN（散射延迟网络）：声音在每个混响节点之间反射，节点充当反射面。该算法适合使用对称性较少的奇数个节点，以减少伪影或金属声共振。
- FDN（反馈延迟网络）：每个节点作为独立的混响处理器运行，使用经典混响算法。在舞台周围和观众周围放置节点。
- IR（脉冲响应）：经典卷积混响。可以加载音频样本作为脉冲响应。每个节点可以共享相同的IR或使用不同的IR。
节点位置可以直接在地图上调整。Ctrl/Cmd键可以对称移动混响节点对。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Algorithms
  - ZH: 混响算法
  - [ ] OK    Fix: 

## `help.reverbFeed`

- **`body`**
  - EN: Pre-processing sending of the input channels to the reverb nodes.
- Orientation and On/Off Angles define what inputs each reverb node will receive. Inputs in the green sector will be picked-up, but not the ones in front in the red sector. There is a fade between both sectors.
- HF Damping simulates the loss of high frequency with distance.
- Distance Attenuation percentage allows you to define if more or less of the attenuation calculated from the distance and parameters of the input is applied.
- Minimal Latency toggles whether the output is scanned for the smallest calculated delay and also if the setting once engaged on an input will reduce the delay for that input through this reverb node.
- Live Source Attenuation toggles the reduction of level of nearby input if they have this setting enabled.
  - ZH: 输入通道到混响节点的预处理发送。
- 方向和On/Off角度定义每个节点接收的输入。
- HF衰减模拟高频随距离的损失。
- 距离衰减百分比定义应用的衰减。
- 最小延迟切换是否使用最小计算延迟。
- 现场声源衰减切换附近输入的电平降低。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Feed
  - ZH: 混响馈送
  - [ ] OK    Fix: 

## `help.reverbPostProc`

- **`body`**
  - EN: Includes a 4-band EQ and an Expander that looks at the signal entering the reverb processor to reduce long reverb tails when the inputs are quiet.
  - ZH: 包含4频段EQ和扩展器，监视进入混响处理器的信号，在输入安静时减少长混响尾音。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Post-Processing
  - ZH: 混响后处理
  - [ ] OK    Fix: 

## `help.reverbPreProc`

- **`body`**
  - EN: Includes a 4-band EQ and a Compressor to remove transients which might excite the reverb processor a bit too much.
  - ZH: 包含4频段EQ和压缩器，用于去除可能过度激励混响处理器的瞬态信号。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Pre-Processing
  - ZH: 混响前处理
  - [ ] OK    Fix: 

## `help.reverbReturn`

- **`body`**
  - EN: Post-processing sending to the speakers.
- Distance Attenuation defines the level drop per meter to the speakers.
- Common Attenuation will look for the lowest attenuation and keep only a percentage of this level drop and apply this across all other outputs.
- Mutes and Mute Macros allow to prevent a reverb channel from feeding an output. For instance it may not be necessary to send reverb to sub-bass speakers.
  - ZH: 到扬声器的后处理发送。
- 距离衰减定义每米的电平下降。
- 共同衰减保持最低衰减的百分比。
- 静音防止混响通道馈送到输出。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Return
  - ZH: 混响返回
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
  - ZH: 采样器允许触发采样并实时与其交互。
在轨道上启用时，采样器将始终替代实时输入。
多个采样器可以分配给不同的输入并单独触发。
使用采样器：
- 选择Roli Lightpad或已连接的Android Remote应用中的触控板。
- 将采样添加到网格中的不同方块。调整相对起始位置、电平以及入/出点。按住Shift键点击可选择多个采样。
- 创建采样集：选中的采样将被添加到新集中。创建后可按住Ctrl/Cmd点击方块来添加或删除。每个集可以重命名，可设置固定顺序或随机顺序。每个集有衰减设置和基础位置。
- 按下Lightpad或触控板触发采样。压力可映射到电平、高度和高频滤波。手指移动像操纵杆一样移动声音。
松开触控板将停止采样。
采样器设置存储在输入文件中。
方块和集可以复制、导出和导入。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Sampler
  - ZH: 采样器
  - [ ] OK    Fix: 

## `help.sessionData`

- **`body`**
  - EN: When starting a session select the working folder where the system will place the files and eventual audio files. For new projects create a new folder. When reloading a previous session navigate to the corresponding folder.
Each section has a separate xml file (System configuration, Network, Outputs, Reverbs, Inputs) and backups. Additionally convolution reverb impulse responses and audio samples will be stored in sub-directories.
Each section can be stored and recalled individually or as a whole.
Each section can also export and import files from other projects.
  - ZH: 启动会话时，选择系统将放置文件和音频文件的工作文件夹。对于新项目，创建新文件夹。重新加载之前的会话时，导航到相应的文件夹。
每个部分都有单独的xml文件（系统配置、网络、输出、混响、输入）和备份。卷积混响脉冲响应和音频样本将存储在子目录中。
每个部分可以单独或整体存储和调用。
每个部分也可以从其他项目导出和导入文件。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Session Data
  - ZH: 会话数据
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
  - ZH: 快照存储输入参数，但可以有一个范围，在演出期间调用。
范围告诉系统存储或调用哪些数据。
有几种方法：
- 仅在本地文件中记录所需数据。过滤器在存储时应用。
- 在本地文件中记录所有数据和过滤器。过滤器在调用时应用。
- 在QLab cue中记录所有范围内的数据。不建议用于大型配置。
范围可以显示并自动预选手动更改的参数。更改的参数用黄色标记。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Input Snapshots and Scope
  - ZH: 输入快照和范围
  - [ ] OK    Fix: 

## `help.tracking`

- **`body`**
  - EN: Tracking allows to follow the position in 2D or 3D of actors and musicians. There are several tracking solutions based on UWB (UltraWide Band) tags, 3D camera, computer vision systems and Infra-Red LED's followed by IR sensitive cameras.
This application allows you to receive tracking data from several protocols: OSC, MQTT, PosiStageNet/PSN, RTTrP.
You can select which protocol you're using and enter its settings. You may also adjust the mapping (offset, scaling and orientation).
Each input has a toggle to enable tracking, an ID to select which tracking marker it should follow and a smoothing algorithm to reduce jittery movements.
  - ZH: 跟踪允许追踪演员和音乐家的2D或3D位置。有多种跟踪解决方案，包括UWB标签、3D摄像头、计算机视觉系统和红外LED配合IR敏感摄像头。
此应用程序可以从多种协议接收跟踪数据：OSC、MQTT、PosiStageNet/PSN、RTTrP。
您可以选择使用的协议并输入其设置。还可以调整映射（偏移、缩放和方向）。
每个输入都有一个启用跟踪的开关、一个选择跟踪标记的ID和一个减少抖动的平滑算法。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Tracking
  - ZH: 跟踪
  - [ ] OK    Fix: 

## `help.tuning`

- **`body`**
  - EN: System tuning for WFS is different from standard PA tuning. It can proceed as follows:
- Start with the flown array muted. Set the desired levels for the near-field speakers when listening to them in the first row. Adjust the high frequency shelf attenuation so the near-field speakers are not too bright and draw too much attention.
- Mute the near-field array and unmute the flown array, find a suitable level towards the back of the house.
- Unmute both arrays, adjust the delay of the flown array to bring the sound down to the correct height in the lower rows. Adjust the levels, HF shelf/distance ratio and vertical and horizontal parallax for each array to have consistent level wherever your inputs are on stage. This is especially important if your inputs are actors, singers or acoustic music instruments on stage. You may also test adding some Haas effect to delay the whole system if you feel the WFS is too early or adding comb filtering with the acoustic sound.
You may follow a different workflow for tuning or go for different cues.
  - ZH: WFS的系统调谐不同于标准PA调谐。可以按以下步骤进行：
- 从飞行阵列静音开始。在第一排聆听近场扬声器时设置所需的电平。调整高频搁架衰减使近场扬声器不至于太亮。
- 静音近场阵列并取消飞行阵列的静音，在场地后方找到合适的电平。
- 取消两个阵列的静音，调整飞行阵列的延迟以在较低的排将声音降到正确的高度。调整每个阵列的电平、HF搁架/距离比以及垂直和水平视差，以在输入位于舞台任何位置时实现一致的电平。
您可以采用不同的工作流程进行调谐，或针对不同情况采用不同的设置。
  - [ ] OK    Fix: 

- **`title`**
  - EN: System Tuning
  - ZH: 系统调谐
  - [ ] OK    Fix: 

## `inputs`

- **`arrayPrefix`**
  - EN: Array
  - ZH: 阵列
  - [ ] OK    Fix: 

## `inputs.buttons`

- **`deleteSnapshot`**
  - EN: Delete Snapshot
  - ZH: 删除快照
  - [ ] OK    Fix: 

- **`editScope`**
  - EN: Edit Scope
  - ZH: 编辑范围
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - ZH: 导出
  - [ ] OK    Fix: 

- **`hiddenOnMap`**
  - EN: Input Hidden on Map
  - ZH: 输入在地图上隐藏
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - ZH: 导入
  - [ ] OK    Fix: 

- **`lockOnMap`**
  - EN: Lock on Map
  - ZH: 在地图上锁定
  - [ ] OK    Fix: 

- **`pauseAll`**
  - EN: Pause All
  - ZH: 全部暂停
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Backup
  - ZH: 加载备份
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Input Config
  - ZH: 重新加载输入配置
  - [ ] OK    Fix: 

- **`reloadSnapshot`**
  - EN: Reload Snapshot
  - ZH: 加载快照
  - [ ] OK    Fix: 

- **`reloadWithoutScope`**
  - EN: Reload Without Scope
  - ZH: 无过滤重新加载
  - [ ] OK    Fix: 

- **`resumeAll`**
  - EN: Resume All
  - ZH: 全部恢复
  - [ ] OK    Fix: 

- **`samplerOff`**
  - EN: Sampler: OFF
  - ZH: 采样器: 关
  - [ ] OK    Fix: 

- **`samplerOn`**
  - EN: Sampler: ON
  - ZH: 采样器: 开
  - [ ] OK    Fix: 

- **`setAllInputs`**
  - EN: Set all Inputs...
  - ZH: 设置所有输入...
  - [ ] OK    Fix: 

- **`solo`**
  - EN: Solo
  - ZH: 独听
  - [ ] OK    Fix: 

- **`soloModeMulti`**
  - EN: Multi
  - ZH: 多选
  - [ ] OK    Fix: 

- **`soloModeSingle`**
  - EN: Single
  - ZH: 单选
  - [ ] OK    Fix: 

- **`stopAll`**
  - EN: Stop All
  - ZH: 全部停止
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Input Config
  - ZH: 保存输入配置
  - [ ] OK    Fix: 

- **`storeSnapshot`**
  - EN: Store Snapshot
  - ZH: 保存快照
  - [ ] OK    Fix: 

- **`updateSnapshot`**
  - EN: Update Snapshot
  - ZH: 更新快照
  - [ ] OK    Fix: 

- **`visibleOnMap`**
  - EN: Input Visible on Map
  - ZH: 输入在地图上可见
  - [ ] OK    Fix: 

## `inputs.clusters`

- **`clusterPrefix`**
  - EN: Cluster
  - ZH: 集群
  - [ ] OK    Fix: 

- **`single`**
  - EN: Single
  - ZH: 单独
  - [ ] OK    Fix: 

## `inputs.coordinates`

- **`xyz`**
  - EN: XYZ
  - ZH: XYZ
  - [ ] OK    Fix: 

## `inputs.dialogs`

- **`exportConfig`**
  - EN: Export Input Configuration
  - ZH: 导出输入配置
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Input Configuration
  - ZH: 导入输入配置
  - [ ] OK    Fix: 

- **`selectChannel`**
  - EN: Select Channel
  - ZH: 选择通道
  - [ ] OK    Fix: 

- **`snapshotNameLabel`**
  - EN: Name:
  - ZH: 名称：
  - [ ] OK    Fix: 

- **`storeSnapshotMessage`**
  - EN: Enter a name for the new snapshot:
  - ZH: 请输入新快照的名称：
  - [ ] OK    Fix: 

- **`storeSnapshotTitle`**
  - EN: Store New Snapshot
  - ZH: 保存新快照
  - [ ] OK    Fix: 

- **`trackingConflictCluster`**
  - EN: Input {current} has tracking enabled, but Input {existing} in Cluster {cluster} is already tracked.

Only one tracked input per cluster is allowed.
  - ZH: 输入 {current} 已启用追踪，但集群 {cluster} 中的输入 {existing} 已被追踪。

每个集群仅允许一个追踪输入。
  - [ ] OK    Fix: 

- **`trackingConflictContinue`**
  - EN: Continue (disable tracking)
  - ZH: 继续（禁用追踪）
  - [ ] OK    Fix: 

- **`trackingConflictSwitch`**
  - EN: Input {existing} in Cluster {cluster} already has tracking enabled.

Only one tracked input per cluster is allowed.

Do you want to disable tracking on Input {existing} and enable it on Input {to}?
  - ZH: 集群 {cluster} 中的输入 {existing} 已启用追踪。

每个集群仅允许一个追踪输入。

是否禁用输入 {existing} 的追踪并启用输入 {to} 的追踪？
  - [ ] OK    Fix: 

- **`trackingConflictTitle`**
  - EN: Tracking Conflict
  - ZH: 追踪冲突
  - [ ] OK    Fix: 

- **`trackingConflictYes`**
  - EN: Yes, switch tracking
  - ZH: 是，切换追踪
  - [ ] OK    Fix: 

## `inputs.gradientMap`

- **`buttons.copyLayer`**
  - EN: Copy Layer
  - ZH: 复制图层
  - [ ] OK    Fix: 

- **`buttons.copyShape`**
  - EN: Copy Shape
  - ZH: 复制形状
  - [ ] OK    Fix: 

- **`buttons.delete`**
  - EN: Delete
  - ZH: 删除
  - [ ] OK    Fix: 

- **`buttons.enable`**
  - EN: On
  - ZH: 开
  - [ ] OK    Fix: 

- **`buttons.lock`**
  - EN: Lock
  - ZH: 锁定
  - [ ] OK    Fix: 

- **`buttons.pasteLayer`**
  - EN: Paste Layer
  - ZH: 粘贴图层
  - [ ] OK    Fix: 

- **`buttons.pasteShape`**
  - EN: Paste Shape
  - ZH: 粘贴形状
  - [ ] OK    Fix: 

- **`header.attenuationLayer`**
  - EN: Attenuation Layer
  - ZH: 衰减图层
  - [ ] OK    Fix: 

- **`header.heightLayer`**
  - EN: Height Layer
  - ZH: 高度图层
  - [ ] OK    Fix: 

- **`header.hfShelfLayer`**
  - EN: HF Shelf Layer
  - ZH: 高频搁架图层
  - [ ] OK    Fix: 

- **`help.blackValue`**
  - EN: Parameter value mapped to black (0.00–1.00)
  - ZH: 映射到黑色的参数值（0.00–1.00）
  - [ ] OK    Fix: 

- **`help.blur`**
  - EN: Edge blur in meters
  - ZH: 边缘模糊（米）
  - [ ] OK    Fix: 

- **`help.copy`**
  - EN: Copy selected shape or layer
  - ZH: 复制选中的形状或图层
  - [ ] OK    Fix: 

- **`help.curve`**
  - EN: Gamma curve (-1 to 1, 0 = linear)
  - ZH: 伽马曲线（-1到1，0 = 线性）
  - [ ] OK    Fix: 

- **`help.ellipseTool`**
  - EN: Draw ellipse
  - ZH: 绘制椭圆
  - [ ] OK    Fix: 

- **`help.fillTool`**
  - EN: Apply uniform fill to shape
  - ZH: 对形状应用均匀填充
  - [ ] OK    Fix: 

- **`help.fillValue`**
  - EN: Fill brightness (0 = black, 1 = white)
  - ZH: 填充亮度（0 = 黑色，1 = 白色）
  - [ ] OK    Fix: 

- **`help.layerEnable`**
  - EN: Enable/disable layer (affects output and OSC)
  - ZH: 启用/禁用图层（影响输出和OSC）
  - [ ] OK    Fix: 

- **`help.layerSelect`**
  - EN: Select this layer for editing
  - ZH: 选择此图层进行编辑
  - [ ] OK    Fix: 

- **`help.layerVisible`**
  - EN: Show/hide layer preview on canvas
  - ZH: 在画布上显示/隐藏图层预览
  - [ ] OK    Fix: 

- **`help.linGradTool`**
  - EN: Apply linear gradient to shape
  - ZH: 对形状应用线性渐变
  - [ ] OK    Fix: 

- **`help.paste`**
  - EN: Paste shape or layer from clipboard
  - ZH: 从剪贴板粘贴形状或图层
  - [ ] OK    Fix: 

- **`help.polygonTool`**
  - EN: Draw polygon (double-click to close)
  - ZH: 绘制多边形（双击关闭）
  - [ ] OK    Fix: 

- **`help.radGradTool`**
  - EN: Apply radial gradient to shape
  - ZH: 对形状应用径向渐变
  - [ ] OK    Fix: 

- **`help.rectTool`**
  - EN: Draw rectangle
  - ZH: 绘制矩形
  - [ ] OK    Fix: 

- **`help.selectTool`**
  - EN: Select and move shapes
  - ZH: 选择和移动形状
  - [ ] OK    Fix: 

- **`help.shapeDelete`**
  - EN: Delete selected shape(s)
  - ZH: 删除选中的形状
  - [ ] OK    Fix: 

- **`help.shapeEnable`**
  - EN: Enable/disable shape
  - ZH: 启用/禁用形状
  - [ ] OK    Fix: 

- **`help.shapeLock`**
  - EN: Lock shape position
  - ZH: 锁定形状位置
  - [ ] OK    Fix: 

- **`help.whiteValue`**
  - EN: Parameter value mapped to white (0.00–1.00)
  - ZH: 映射到白色的参数值（0.00–1.00）
  - [ ] OK    Fix: 

- **`hints.darkMaxAtten`**
  - EN: Dark = max attenuation | Light = none
  - ZH: 暗 = 最大衰减 | 亮 = 无
  - [ ] OK    Fix: 

- **`hints.darkMaxHeight`**
  - EN: Dark = max height | Light = ground
  - ZH: 暗 = 最大高度 | 亮 = 地面
  - [ ] OK    Fix: 

- **`hints.darkMaxHF`**
  - EN: Dark = max HF shelf | Light = none
  - ZH: 暗 = 最大高频搁架 | 亮 = 无
  - [ ] OK    Fix: 

- **`hints.polygonClose`**
  - EN: Double-click to close polygon
  - ZH: 双击关闭多边形
  - [ ] OK    Fix: 

- **`hints.whiteMaxAtten`**
  - EN: White = max attenuation | Black = none
  - ZH: 白 = 最大衰减 | 黑 = 无
  - [ ] OK    Fix: 

- **`hints.whiteMaxHeight`**
  - EN: White = max height | Black = ground
  - ZH: 白 = 最大高度 | 黑 = 地面
  - [ ] OK    Fix: 

- **`hints.whiteMaxHF`**
  - EN: White = max HF shelf | Black = none
  - ZH: 白 = 最大高频搁架 | 黑 = 无
  - [ ] OK    Fix: 

- **`labels.black`**
  - EN: Black:
  - ZH: 黑色：
  - [ ] OK    Fix: 

- **`labels.blur`**
  - EN: Blur:
  - ZH: 模糊：
  - [ ] OK    Fix: 

- **`labels.center`**
  - EN: Center:
  - ZH: 中心：
  - [ ] OK    Fix: 

- **`labels.curve`**
  - EN: Curve:
  - ZH: 曲线：
  - [ ] OK    Fix: 

- **`labels.edge`**
  - EN: Edge:
  - ZH: 边缘：
  - [ ] OK    Fix: 

- **`labels.end`**
  - EN: End:
  - ZH: 终点：
  - [ ] OK    Fix: 

- **`labels.fill`**
  - EN: Fill:
  - ZH: 填充：
  - [ ] OK    Fix: 

- **`labels.name`**
  - EN: Name:
  - ZH: 名称：
  - [ ] OK    Fix: 

- **`labels.start`**
  - EN: Start:
  - ZH: 起点：
  - [ ] OK    Fix: 

- **`labels.white`**
  - EN: White:
  - ZH: 白色：
  - [ ] OK    Fix: 

- **`layers.attenuation`**
  - EN: Attenuation
  - ZH: 衰减
  - [ ] OK    Fix: 

- **`layers.height`**
  - EN: Height
  - ZH: 高度
  - [ ] OK    Fix: 

- **`layers.hfShelf`**
  - EN: HF Shelf
  - ZH: 高频搁架
  - [ ] OK    Fix: 

- **`tools.editPoints`**
  - EN: Edit Points
  - ZH: 编辑点
  - [ ] OK    Fix: 

- **`tools.ellipse`**
  - EN: Ellipse
  - ZH: 椭圆
  - [ ] OK    Fix: 

- **`tools.fill`**
  - EN: Fill
  - ZH: 填充
  - [ ] OK    Fix: 

- **`tools.linGrad`**
  - EN: Linear Grad.
  - ZH: 线性渐变
  - [ ] OK    Fix: 

- **`tools.polygon`**
  - EN: Polygon
  - ZH: 多边形
  - [ ] OK    Fix: 

- **`tools.radGrad`**
  - EN: Radial Grad.
  - ZH: 径向渐变
  - [ ] OK    Fix: 

- **`tools.rect`**
  - EN: Rectangle
  - ZH: 矩形
  - [ ] OK    Fix: 

- **`tools.select`**
  - EN: Select
  - ZH: 选择
  - [ ] OK    Fix: 

- **`warnings.heightRatioZero`**
  - EN: Height Ratio is 0% — increase it for height to take effect
  - ZH: 高度比率为0% — 请增大以使高度生效
  - [ ] OK    Fix: 

## `inputs.help`

- **`admMapping`**
  - EN: Assign this input to an ADM-OSC mapping for position receive/transmit.
  - ZH: 将此输入分配到ADM-OSC映射，用于位置接收/发送。
  - [ ] OK    Fix: 

- **`arrayAttenDial`**
  - EN: Attenuation for Array {num} (-60 to 0 dB).
  - ZH: 阵列 {num} 的衰减 (-60 至 0 dB)。
  - [ ] OK    Fix: 

- **`attenuationLawButton`**
  - EN: Attenuation Law Model (Linear Decrease of Volume with Distance Between Object and Speaker or Squared).
  - ZH: 衰减规律模型（对象与扬声器之间随距离线性降低音量）。
  - [ ] OK    Fix: 

- **`attenuationSlider`**
  - EN: Input Channel Attenuation.
  - ZH: 输入通道衰减。
  - [ ] OK    Fix: 

- **`channelSelector`**
  - EN: Input Channel Number and Selection.
  - ZH: 输入通道编号与选择。
  - [ ] OK    Fix: 

- **`clusterSelector`**
  - EN: Object is Part of a Cluster.
  - ZH: 对象所属集群。
  - [ ] OK    Fix: 

- **`commonAttenDial`**
  - EN: Percentage of the Common Part of the Attenuation for selected Object Relative to All Outputs.
  - ZH: 选定对象相对于所有输出的共同衰减百分比。
  - [ ] OK    Fix: 

- **`constraintDistanceButton`**
  - EN: Limit Position to Distance Range from Origin (for Cylindrical/Spherical modes).
  - ZH: 限制位置在距原点的距离范围内 (用于圆柱/球面模式)。
  - [ ] OK    Fix: 

- **`constraintXButton`**
  - EN: Limit Position to the Bounds of the Stage in Width.
  - ZH: 限制位置在舞台宽度范围内。
  - [ ] OK    Fix: 

- **`constraintYButton`**
  - EN: Limit Position to the Bounds of the Stage in Depth.
  - ZH: 限制位置在舞台深度范围内。
  - [ ] OK    Fix: 

- **`constraintZButton`**
  - EN: Limit Position to the Bounds of the Stage in Height.
  - ZH: 限制位置在舞台高度范围内。
  - [ ] OK    Fix: 

- **`coordModeSelector`**
  - EN: Coordinate display mode: Cartesian (X/Y/Z), Cylindrical (radius/azimuth/height), or Spherical (radius/azimuth/elevation).
  - ZH: 坐标显示模式：笛卡尔坐标 (X/Y/Z)、柱坐标 (半径/方位角/高度) 或球坐标 (半径/方位角/仰角)。
  - [ ] OK    Fix: 

- **`delayLatencySlider`**
  - EN: Input Channel Delay (positive values) or Latency Compensation (negative values).
  - ZH: 输入通道延迟 (正值) 或延迟补偿 (负值)。
  - [ ] OK    Fix: 

- **`deleteSnapshot`**
  - EN: Delete Selected Input Snapshot With Confirmation.
  - ZH: 删除选定的快照。
  - [ ] OK    Fix: 

- **`directivitySlider`**
  - EN: How Wide is the Brightness of The Object.
  - ZH: 对象亮度锥的宽度。
  - [ ] OK    Fix: 

- **`distanceAttenDial`**
  - EN: Attenuation per Meter Between Object and Speaker.
  - ZH: 对象与扬声器之间每米衰减量。
  - [ ] OK    Fix: 

- **`distanceMaxEditor`**
  - EN: Maximum Distance from Origin in Meters.
  - ZH: 距原点的最大距离 (米)。
  - [ ] OK    Fix: 

- **`distanceMinEditor`**
  - EN: Minimum Distance from Origin in Meters.
  - ZH: 距原点的最小距离 (米)。
  - [ ] OK    Fix: 

- **`distanceRangeSlider`**
  - EN: Set Minimum and Maximum Distance from Origin.
  - ZH: 设置距原点的最小和最大距离。
  - [ ] OK    Fix: 

- **`distanceRatioDial`**
  - EN: Attenuation Ratio for Squared Model.
  - ZH: 平方模型的衰减比率。
  - [ ] OK    Fix: 

- **`editScope`**
  - EN: Open Selected Input Snapshot Scope Window.
  - ZH: 编辑快照的范围过滤器。
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Input Configuration to file (with file explorer window).
  - ZH: 将输入配置导出到文件（打开文件浏览器窗口）。
  - [ ] OK    Fix: 

- **`flipXButton`**
  - EN: X will be Symetrical to the Origin. Keyboard Nudging will be Inverted.
  - ZH: X 将相对于原点对称。键盘微调将反转。
  - [ ] OK    Fix: 

- **`flipYButton`**
  - EN: Y will be Symetrical to the Origin. Keyboard Nudging will be Inverted.
  - ZH: Y 将相对于原点对称。键盘微调将反转。
  - [ ] OK    Fix: 

- **`flipZButton`**
  - EN: Z will be Symetrical to the Origin. Keyboard Nudging will be Inverted.
  - ZH: Z 将相对于原点对称。键盘微调将反转。
  - [ ] OK    Fix: 

- **`frActiveButton`**
  - EN: Enable Simulated Floor Reflections for the Object.
  - ZH: 为对象启用模拟地板反射。
  - [ ] OK    Fix: 

- **`frAttenuationSlider`**
  - EN: Attenuation of the Simulated Floor Reflections for the Object.
  - ZH: 对象的模拟地板反射的衰减。
  - [ ] OK    Fix: 

- **`frDiffusionDial`**
  - EN: Diffusion Effect of the Simulated Floor Reflections for the Object.
  - ZH: 对象的模拟地板反射的扩散效果。
  - [ ] OK    Fix: 

- **`frHighShelfActiveButton`**
  - EN: Enable High Shelf Filter for Floor Reflections.
  - ZH: 为地板反射启用高搁架滤波器。
  - [ ] OK    Fix: 

- **`frHighShelfFreqSlider`**
  - EN: High Shelf Frequency for Floor Reflections.
  - ZH: 地板反射的高搁架频率。
  - [ ] OK    Fix: 

- **`frHighShelfGainSlider`**
  - EN: High Shelf Gain for Floor Reflections.
  - ZH: 地板反射的高搁架增益。
  - [ ] OK    Fix: 

- **`frHighShelfSlopeSlider`**
  - EN: High Shelf Slope for Floor Reflections.
  - ZH: 地板反射的高搁架斜率。
  - [ ] OK    Fix: 

- **`frLowCutActiveButton`**
  - EN: Enable Low Cut Filter for Floor Reflections.
  - ZH: 为地板反射启用低切滤波器。
  - [ ] OK    Fix: 

- **`frLowCutFreqSlider`**
  - EN: Low Cut Frequency for Floor Reflections.
  - ZH: 地板反射的低切频率。
  - [ ] OK    Fix: 

- **`heightFactorDial`**
  - EN: Take Elevation of Object into Account Fully, Partially or Not.
  - ZH: 完全、部分或不考虑对象的高度。
  - [ ] OK    Fix: 

- **`hfShelfSlider`**
  - EN: How Much Brightness is lost in the Back of the Object, Out of its Brightness Cone.
  - ZH: 对象后方、亮度锥外损失的亮度量。
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Input Configuration from file (with file explorer window).
  - ZH: 从文件导入输入配置（打开文件浏览器窗口）。
  - [ ] OK    Fix: 

- **`inputDirectivityDial`**
  - EN: Where is the Object pointing to in the Horizontal Plane.
  - ZH: 对象在水平面上的方向。
  - [ ] OK    Fix: 

- **`jitterSlider`**
  - EN: Sphere of Rapid Movements of the Object.
  - ZH: 对象快速移动的球。
  - [ ] OK    Fix: 

- **`lfoActiveButton`**
  - EN: Enable or Disable the Periodic Movement of the Object (LFO).
  - ZH: 启用或禁用对象的周期性运动 (LFO)。
  - [ ] OK    Fix: 

- **`lfoAmplitudeXSlider`**
  - EN: Width of Movement in Relation to Base Position of the Object.
  - ZH: 相对于对象基础位置的宽度运动幅度。
  - [ ] OK    Fix: 

- **`lfoAmplitudeYSlider`**
  - EN: Depth of Movement in Relation to Base Position of the Object.
  - ZH: 相对于对象基础位置的深度运动幅度。
  - [ ] OK    Fix: 

- **`lfoAmplitudeZSlider`**
  - EN: Height of Movement in Relation to Base Position of the Object.
  - ZH: 相对于对象基础位置的高度运动幅度。
  - [ ] OK    Fix: 

- **`lfoGyrophoneSelector`**
  - EN: Rotation of the Brightness Cone of the Object.
  - ZH: 对象亮度锥的旋转。
  - [ ] OK    Fix: 

- **`lfoPeriodDial`**
  - EN: Base Period of the Movement of the Object.
  - ZH: 对象运动的基础周期。
  - [ ] OK    Fix: 

- **`lfoPhaseDial`**
  - EN: Phase Offset of the Movement of the Object.
  - ZH: 对象运动的相位偏移。
  - [ ] OK    Fix: 

- **`lfoPhaseXDial`**
  - EN: Phase Offset of the Movement of the Object in Width.
  - ZH: 宽度方向上对象运动的相位偏移。
  - [ ] OK    Fix: 

- **`lfoPhaseYDial`**
  - EN: Phase Offset of the Movement of the Object in Depth.
  - ZH: 深度方向上对象运动的相位偏移。
  - [ ] OK    Fix: 

- **`lfoPhaseZDial`**
  - EN: Phase Offset of the Movement of the Object in Height.
  - ZH: 高度方向上对象运动的相位偏移。
  - [ ] OK    Fix: 

- **`lfoRateXSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Width.
  - ZH: 宽度方向上相对于基础周期的更快或更慢的运动。
  - [ ] OK    Fix: 

- **`lfoRateYSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Depth.
  - ZH: 深度方向上相对于基础周期的更快或更慢的运动。
  - [ ] OK    Fix: 

- **`lfoRateZSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Height.
  - ZH: 高度方向上相对于基础周期的更快或更慢的运动。
  - [ ] OK    Fix: 

- **`lfoShapeXSelector`**
  - EN: Movement Behaviour of the Object in Width.
  - ZH: 宽度方向上对象运动的行为。
  - [ ] OK    Fix: 

- **`lfoShapeYSelector`**
  - EN: Movement Behaviour of the Object in Depth.
  - ZH: 深度方向上对象运动的行为。
  - [ ] OK    Fix: 

- **`lfoShapeZSelector`**
  - EN: Movement Behaviour of the Object in Height.
  - ZH: 高度方向上对象运动的行为。
  - [ ] OK    Fix: 

- **`lsActiveButton`**
  - EN: If You Need to Reduce the Level in Speakers Close to the Object. (eg. Loud Source Present on Stage)
  - ZH: 如果需要降低靠近对象的扬声器的电平 (例如：舞台上有大声音源)。
  - [ ] OK    Fix: 

- **`lsAttenuationSlider`**
  - EN: Constant Attenuation of Speakers Around the Object.
  - ZH: 对象周围扬声器的恒定衰减。
  - [ ] OK    Fix: 

- **`lsPeakEnableButton`**
  - EN: Enable or Disable the Fast (Peak) Compressor for the Live Source Tamer.
  - ZH: 启用或禁用 Live Source Tamer 的快速 (峰值) 压缩器。
  - [ ] OK    Fix: 

- **`lsPeakRatioDial`**
  - EN: Ratio to Apply the Fast Compression for Speakers Around the Object.
  - ZH: 应用于对象周围扬声器的快速压缩比率。
  - [ ] OK    Fix: 

- **`lsPeakThresholdSlider`**
  - EN: Fast Compression Threshold for Speakers Around the Object to Control Transients.
  - ZH: 对象周围扬声器的快速压缩阈值，用于控制瞬态。
  - [ ] OK    Fix: 

- **`lsRadiusSlider`**
  - EN: How Far does the Attenuation Affect The Speakers.
  - ZH: 衰减影响扬声器的距离。
  - [ ] OK    Fix: 

- **`lsShapeSelector`**
  - EN: Profile of the Attenuation Around the Object.
  - ZH: 对象周围的衰减轮廓。
  - [ ] OK    Fix: 

- **`lsSlowEnableButton`**
  - EN: Enable or Disable the Slow Compressor for the Live Source Tamer.
  - ZH: 启用或禁用 Live Source Tamer 的缓慢压缩器。
  - [ ] OK    Fix: 

- **`lsSlowRatioDial`**
  - EN: Ratio to Apply the Slow Compression for Speakers Around the Object.
  - ZH: 应用于对象周围扬声器的缓慢压缩比率。
  - [ ] OK    Fix: 

- **`lsSlowThresholdSlider`**
  - EN: Slow Compression Threshold for Speakers Around the Object to Control Sustained Level.
  - ZH: 对象周围扬声器的缓慢压缩阈值，用于控制持续电平。
  - [ ] OK    Fix: 

- **`mapLockButton`**
  - EN: Prevent Interaction on the Map Tab
  - ZH: 防止在地图标签页上交互。
  - [ ] OK    Fix: 

- **`mapVisibilityButton`**
  - EN: Make Visible or Hide The Selected Input on the Map
  - ZH: 在地图上显示或隐藏选定的输入。
  - [ ] OK    Fix: 

- **`maxSpeedActiveButton`**
  - EN: Enable or Disable Speed Limiting for Object.
  - ZH: 启用或禁用对象的速度限制。
  - [ ] OK    Fix: 

- **`maxSpeedDial`**
  - EN: Maximum Speed Limit for Object.
  - ZH: 对象的最大速度限制。
  - [ ] OK    Fix: 

- **`minimalLatencyButton`**
  - EN: Select between Acoustic Precedence and Minimal Latency for Amplification Precedence.
  - ZH: 在声学优先和最小延迟之间为放大优先级选择。
  - [ ] OK    Fix: 

- **`muteButton`**
  - EN: Mute Output {num} for this Object.
  - ZH: 为此对象静音输出 {num}。
  - [ ] OK    Fix: 

- **`muteMacrosSelector`**
  - EN: Mute Macros for Fast Muting and Unmuting of Arrays.
  - ZH: 用于快速静音和取消静音阵列的静音宏。
  - [ ] OK    Fix: 

- **`muteReverbSendsButton`**
  - EN: Mute sends from this input to all reverb channels.
  - ZH: 静音从此输入到所有混响通道的发送。
  - [ ] OK    Fix: 

- **`nameEditor`**
  - EN: Displayed Input Channel Name (editable).
  - ZH: 显示的输入通道名称（可编辑）。
  - [ ] OK    Fix: 

- **`nudgeHint1`**
  - EN:  Nudge with Left and Right Arrow Keys.
  - ZH:  使用左右箭头键调整。
  - [ ] OK    Fix: 

- **`nudgeHint2`**
  - EN:  Nudge with Up and Down Arrow Keys.
  - ZH:  使用上下箭头键调整。
  - [ ] OK    Fix: 

- **`nudgeHint3`**
  - EN:  Nudge with Page Up and Page Down Keys.
  - ZH:  使用Page Up/Down调整。
  - [ ] OK    Fix: 

- **`offset1`**
  - EN: Object Offset {name} ({unit}). Adjusted when Tracking is Enabled.
  - ZH: 对象偏移{name}（{unit}）。启用跟踪时自动调整。
  - [ ] OK    Fix: 

- **`offset2`**
  - EN: Object Offset {name} ({unit}). Adjusted when Tracking is Enabled.
  - ZH: 对象偏移{name}（{unit}）。启用跟踪时自动调整。
  - [ ] OK    Fix: 

- **`offset3`**
  - EN: Object Offset {name} ({unit}). Adjusted when Tracking is Enabled.
  - ZH: 对象偏移{name}（{unit}）。启用跟踪时自动调整。
  - [ ] OK    Fix: 

- **`otomoAbsRelButton`**
  - EN: Select Relative or Absolute Coordinates of Displacement.
  - ZH: 选择相对或绝对的位移坐标。
  - [ ] OK    Fix: 

- **`otomoCoordMode`**
  - EN: Coordinate display mode for AutomOtion destinations: Cartesian (X/Y/Z), Cylindrical (r/θ/Z), or Spherical (r/θ/φ).
  - ZH: AutomOtion目标坐标显示模式：笛卡尔（X/Y/Z）、柱面（r/θ/Z）或球面（r/θ/φ）。
  - [ ] OK    Fix: 

- **`otomoCurveDial`**
  - EN: Bend the Path to the Left (Negative) or Right (Positive) of the Direction of Travel.
  - ZH: 将路径向运动方向的左 (负) 或右 (正) 弯曲。
  - [ ] OK    Fix: 

- **`otomoDest1`**
  - EN: Relative or Absolute Destination {name} ({unit}).
  - ZH: 相对或绝对目标{name}（{unit}）。
  - [ ] OK    Fix: 

- **`otomoDest2`**
  - EN: Relative or Absolute Destination {name} ({unit}).
  - ZH: 相对或绝对目标{name}（{unit}）。
  - [ ] OK    Fix: 

- **`otomoDest3`**
  - EN: Relative or Absolute Destination {name} ({unit}).
  - ZH: 相对或绝对目标{name}（{unit}）。
  - [ ] OK    Fix: 

- **`otomoDurationDial`**
  - EN: Duration of the Movement in Seconds (0.1s to 1 hour).
  - ZH: 运动持续时间 (秒，0.1 秒至 1 小时)。
  - [ ] OK    Fix: 

- **`otomoPauseButton`**
  - EN: Pause and Resume the Movement.
  - ZH: 暂停和恢复运动。
  - [ ] OK    Fix: 

- **`otomoPauseResumeAllButton`**
  - EN: Pause or Resume All Active Movements Globally.
  - ZH: 全局暂停或恢复所有活动运动。
  - [ ] OK    Fix: 

- **`otomoResetDial`**
  - EN: Set the Reset Level for the Automatic Trigger.
  - ZH: 设置自动触发的重置电平。
  - [ ] OK    Fix: 

- **`otomoSpeedProfileDial`**
  - EN: Constant Speed or Gradual Acceleration and Slow Down at the Start and the End of the Movement.
  - ZH: 运动开始和结束时的恒定速度或渐进加速和减速。
  - [ ] OK    Fix: 

- **`otomoStartButton`**
  - EN: Start the Movement Manually.
  - ZH: 手动启动运动。
  - [ ] OK    Fix: 

- **`otomoStayReturnButton`**
  - EN: At the End of the Movement, should the Source Stay or Return to the Original Position.
  - ZH: 运动结束时，源是停留还是返回原始位置。
  - [ ] OK    Fix: 

- **`otomoStopAllButton`**
  - EN: Stop All Active Movements Globally.
  - ZH: 全局停止所有活动运动。
  - [ ] OK    Fix: 

- **`otomoStopButton`**
  - EN: Stop the Movement.
  - ZH: 停止运动。
  - [ ] OK    Fix: 

- **`otomoThresholdDial`**
  - EN: Set the Threshold for the Automatic Trigger of the Movement.
  - ZH: 设置运动自动触发的阈值。
  - [ ] OK    Fix: 

- **`otomoTriggerButton`**
  - EN: Manual Start of Displacement or Automatic Trigger on the Audio Level.
  - ZH: 手动启动位移或基于音频电平的自动触发。
  - [ ] OK    Fix: 

- **`pathModeButton`**
  - EN: Enable Path Mode to Follow Drawn Movement Paths Instead of Direct Lines.
  - ZH: 启用路径模式以跟随绘制的运动路径而不是直线。
  - [ ] OK    Fix: 

- **`position1`**
  - EN: Object {name} ({unit}).
  - ZH: 对象{name}（{unit}）。
  - [ ] OK    Fix: 

- **`position2`**
  - EN: Object {name} ({unit}).
  - ZH: 对象{name}（{unit}）。
  - [ ] OK    Fix: 

- **`position3`**
  - EN: Object {name} ({unit}).
  - ZH: 对象{name}（{unit}）。
  - [ ] OK    Fix: 

- **`positionJoystick`**
  - EN: Drag to adjust X/Y position in real-time. Returns to center on release.
  - ZH: 拖动调整 X/Y 实时位置。松开后回到中心。
  - [ ] OK    Fix: 

- **`positionZSlider`**
  - EN: Drag to adjust Z (height) position in real-time. Returns to center on release.
  - ZH: 拖动调整 Z（高度）实时位置。松开后回到中心。
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Input Configuration from backup file.
  - ZH: 从备份文件重新加载输入配置。
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Input Configuration from file.
  - ZH: 从文件重新加载输入配置。
  - [ ] OK    Fix: 

- **`reloadSnapshot`**
  - EN: Reload Selected Input Snapshot for All Objects Taking the Scope into Account.
  - ZH: 重新加载当前选定的快照。
  - [ ] OK    Fix: 

- **`reloadWithoutScope`**
  - EN: Reload Selected Input Snapshot for All Objects Without the Scope.
  - ZH: 重新加载快照，忽略范围过滤器。
  - [ ] OK    Fix: 

- **`sidelinesActiveButton`**
  - EN: Enable Automatic Muting when Source Approaches Stage Edges. Does Not Apply to Downstage (Front) Edge.
  - ZH: 当源接近舞台边缘时启用自动静音。不适用于前 (观众) 边缘。
  - [ ] OK    Fix: 

- **`sidelinesFringeDial`**
  - EN: Fringe Zone Size in Meters. Outer Half is Full Mute, Inner Half Fades Linearly.
  - ZH: 过渡区大小 (米)。外半部分完全静音，内半部分线性淡出。
  - [ ] OK    Fix: 

- **`snapshotSelector`**
  - EN: Select Input Snapshot Without Loading.
  - ZH: 选择要加载的快照。
  - [ ] OK    Fix: 

- **`solo`**
  - EN: Listen to Binaural Rendering of this channel.
  - ZH: 收听此通道的双耳渲染。
  - [ ] OK    Fix: 

- **`soloMode`**
  - EN: Single: one input at a time. Multi: multiple inputs simultaneously.
  - ZH: 单选：一次只听一个输入。多选：可同时听多个输入。
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Input Configuration to file (with backup).
  - ZH: 将输入配置保存到文件（含备份）。
  - [ ] OK    Fix: 

- **`storeSnapshot`**
  - EN: Store new Input Snapshot for All Objects.
  - ZH: 将当前输入参数保存为快照。
  - [ ] OK    Fix: 

- **`tiltSlider`**
  - EN: Where is the Object pointing to in the Vertical Plane.
  - ZH: 对象在垂直面上的方向。
  - [ ] OK    Fix: 

- **`trackingActiveButton`**
  - EN: Enable or Disable Tracking for Object.
  - ZH: 启用或禁用对象的跟踪。
  - [ ] OK    Fix: 

- **`trackingIdSelector`**
  - EN: Tracker ID for Object.
  - ZH: 对象的跟踪器 ID。
  - [ ] OK    Fix: 

- **`trackingSmoothDial`**
  - EN: Smoothing of Tracking Data for Object.
  - ZH: 对象跟踪数据的平滑度。
  - [ ] OK    Fix: 

- **`updateSnapshot`**
  - EN: Update Selected Input Snapshot (with backup).
  - ZH: 使用当前参数更新选定的快照。
  - [ ] OK    Fix: 

## `inputs.labels`

- **`admMapping`**
  - EN: ADM:
  - ZH: ADM:
  - [ ] OK    Fix: 

- **`amplitudeX`**
  - EN: Amplitude X:
  - ZH: 振幅X:
  - [ ] OK    Fix: 

- **`amplitudeY`**
  - EN: Amplitude Y:
  - ZH: 振幅Y:
  - [ ] OK    Fix: 

- **`amplitudeZ`**
  - EN: Amplitude Z:
  - ZH: 振幅Z:
  - [ ] OK    Fix: 

- **`arrayAttenuation`**
  - EN: Array Attenuation:
  - ZH: 阵列衰减:
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Attenuation:
  - ZH: 衰减:
  - [ ] OK    Fix: 

- **`attenuationLaw`**
  - EN: Attenuation Law:
  - ZH: 衰减曲线:
  - [ ] OK    Fix: 

- **`cluster`**
  - EN: Cluster:
  - ZH: 集群:
  - [ ] OK    Fix: 

- **`commonAtten`**
  - EN: Common Atten:
  - ZH: 公共衰减:
  - [ ] OK    Fix: 

- **`coord`**
  - EN: Coord:
  - ZH: 坐标:
  - [ ] OK    Fix: 

- **`curve`**
  - EN: Curve:
  - ZH: 曲线:
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Delay/Latency:
  - ZH: 延迟/延时:
  - [ ] OK    Fix: 

- **`destX`**
  - EN: Dest. X:
  - ZH: 目标X:
  - [ ] OK    Fix: 

- **`destY`**
  - EN: Dest. Y:
  - ZH: 目标Y:
  - [ ] OK    Fix: 

- **`destZ`**
  - EN: Dest. Z:
  - ZH: 目标Z:
  - [ ] OK    Fix: 

- **`diffusion`**
  - EN: Diffusion:
  - ZH: 扩散:
  - [ ] OK    Fix: 

- **`directivity`**
  - EN: Directivity:
  - ZH: 指向性:
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Distance Atten:
  - ZH: 距离衰减:
  - [ ] OK    Fix: 

- **`distanceRatio`**
  - EN: Distance Ratio:
  - ZH: 距离比:
  - [ ] OK    Fix: 

- **`duration`**
  - EN: Duration:
  - ZH: 持续时间:
  - [ ] OK    Fix: 

- **`frequency`**
  - EN: Frequency:
  - ZH: 频率:
  - [ ] OK    Fix: 

- **`fringe`**
  - EN: Fringe:
  - ZH: 边缘:
  - [ ] OK    Fix: 

- **`gain`**
  - EN: Gain:
  - ZH: 增益:
  - [ ] OK    Fix: 

- **`gyrophone`**
  - EN: Gyrophone:
  - ZH: 旋转声源:
  - [ ] OK    Fix: 

- **`heightFactor`**
  - EN: Height Factor:
  - ZH: 高度因子:
  - [ ] OK    Fix: 

- **`hfShelf`**
  - EN: HF Shelf:
  - ZH: 高频搁架:
  - [ ] OK    Fix: 

- **`jitter`**
  - EN: Jitter:
  - ZH: 抖动:
  - [ ] OK    Fix: 

- **`max`**
  - EN: Max:
  - ZH: 最大:
  - [ ] OK    Fix: 

- **`maxSpeed`**
  - EN: Max Speed:
  - ZH: 最大速度:
  - [ ] OK    Fix: 

- **`min`**
  - EN: Min:
  - ZH: 最小:
  - [ ] OK    Fix: 

- **`muteMacros`**
  - EN: Mute Macros:
  - ZH: 静音宏:
  - [ ] OK    Fix: 

- **`name`**
  - EN: Name:
  - ZH: 名称:
  - [ ] OK    Fix: 

- **`offsetX`**
  - EN: Offset X:
  - ZH: 偏移X:
  - [ ] OK    Fix: 

- **`offsetY`**
  - EN: Offset Y:
  - ZH: 偏移Y:
  - [ ] OK    Fix: 

- **`offsetZ`**
  - EN: Offset Z:
  - ZH: 偏移Z:
  - [ ] OK    Fix: 

- **`outX`**
  - EN: Out X:
  - ZH: 输出X:
  - [ ] OK    Fix: 

- **`outY`**
  - EN: Out Y:
  - ZH: 输出Y:
  - [ ] OK    Fix: 

- **`outZ`**
  - EN: Out Z:
  - ZH: 输出Z:
  - [ ] OK    Fix: 

- **`peakRatio`**
  - EN: Peak Ratio:
  - ZH: 峰值比:
  - [ ] OK    Fix: 

- **`peakThreshold`**
  - EN: Peak Threshold:
  - ZH: 峰值阈值:
  - [ ] OK    Fix: 

- **`period`**
  - EN: Period:
  - ZH: 周期:
  - [ ] OK    Fix: 

- **`phase`**
  - EN: Phase:
  - ZH: 相位:
  - [ ] OK    Fix: 

- **`phaseX`**
  - EN: Phase X:
  - ZH: 相位X:
  - [ ] OK    Fix: 

- **`phaseY`**
  - EN: Phase Y:
  - ZH: 相位Y:
  - [ ] OK    Fix: 

- **`phaseZ`**
  - EN: Phase Z:
  - ZH: 相位Z:
  - [ ] OK    Fix: 

- **`positionX`**
  - EN: Position X:
  - ZH: 位置X:
  - [ ] OK    Fix: 

- **`positionY`**
  - EN: Position Y:
  - ZH: 位置Y:
  - [ ] OK    Fix: 

- **`positionZ`**
  - EN: Position Z:
  - ZH: 位置Z:
  - [ ] OK    Fix: 

- **`radius`**
  - EN: Radius:
  - ZH: 半径:
  - [ ] OK    Fix: 

- **`rateX`**
  - EN: Rate X:
  - ZH: 速率X:
  - [ ] OK    Fix: 

- **`rateY`**
  - EN: Rate Y:
  - ZH: 速率Y:
  - [ ] OK    Fix: 

- **`rateZ`**
  - EN: Rate Z:
  - ZH: 速率Z:
  - [ ] OK    Fix: 

- **`reset`**
  - EN: Reset:
  - ZH: 重置:
  - [ ] OK    Fix: 

- **`rotation`**
  - EN: Rotation:
  - ZH: 旋转:
  - [ ] OK    Fix: 

- **`shape`**
  - EN: Shape:
  - ZH: 形状:
  - [ ] OK    Fix: 

- **`shapeX`**
  - EN: Shape X:
  - ZH: 形状X:
  - [ ] OK    Fix: 

- **`shapeY`**
  - EN: Shape Y:
  - ZH: 形状Y:
  - [ ] OK    Fix: 

- **`shapeZ`**
  - EN: Shape Z:
  - ZH: 形状Z:
  - [ ] OK    Fix: 

- **`slope`**
  - EN: Slope:
  - ZH: 斜率:
  - [ ] OK    Fix: 

- **`slowRatio`**
  - EN: Slow Ratio:
  - ZH: 慢速比:
  - [ ] OK    Fix: 

- **`slowThreshold`**
  - EN: Slow Threshold:
  - ZH: 慢速阈值:
  - [ ] OK    Fix: 

- **`speedProfile`**
  - EN: Speed Profile:
  - ZH: 速度曲线:
  - [ ] OK    Fix: 

- **`threshold`**
  - EN: Threshold:
  - ZH: 阈值:
  - [ ] OK    Fix: 

- **`tilt`**
  - EN: Tilt:
  - ZH: 倾斜:
  - [ ] OK    Fix: 

- **`trackingId`**
  - EN: Tracking ID:
  - ZH: 追踪ID:
  - [ ] OK    Fix: 

- **`trackingSmooth`**
  - EN: Tracking Smooth:
  - ZH: 追踪平滑:
  - [ ] OK    Fix: 

- **`xyJoystick`**
  - EN: X/Y
  - ZH: X/Y
  - [ ] OK    Fix: 

- **`zSlider`**
  - EN: Z
  - ZH: Z
  - [ ] OK    Fix: 

## `inputs.lfo`

- **`gyrophone.antiClockwise`**
  - EN: Anti-Clockwise
  - ZH: 逆时针
  - [ ] OK    Fix: 

- **`gyrophone.clockwise`**
  - EN: Clockwise
  - ZH: 顺时针
  - [ ] OK    Fix: 

- **`gyrophone.off`**
  - EN: OFF
  - ZH: 关
  - [ ] OK    Fix: 

- **`shapes.exp`**
  - EN: exp
  - ZH: 指数
  - [ ] OK    Fix: 

- **`shapes.keystone`**
  - EN: keystone
  - ZH: 梯形
  - [ ] OK    Fix: 

- **`shapes.log`**
  - EN: log
  - ZH: 对数
  - [ ] OK    Fix: 

- **`shapes.off`**
  - EN: OFF
  - ZH: 关
  - [ ] OK    Fix: 

- **`shapes.random`**
  - EN: random
  - ZH: 随机
  - [ ] OK    Fix: 

- **`shapes.sawtooth`**
  - EN: sawtooth
  - ZH: 锯齿
  - [ ] OK    Fix: 

- **`shapes.sine`**
  - EN: sine
  - ZH: 正弦
  - [ ] OK    Fix: 

- **`shapes.square`**
  - EN: square
  - ZH: 方波
  - [ ] OK    Fix: 

- **`shapes.triangle`**
  - EN: triangle
  - ZH: 三角
  - [ ] OK    Fix: 

## `inputs.liveSource`

- **`linear`**
  - EN: linear
  - ZH: 线性
  - [ ] OK    Fix: 

- **`log`**
  - EN: log
  - ZH: 对数
  - [ ] OK    Fix: 

- **`sine`**
  - EN: sine
  - ZH: 正弦
  - [ ] OK    Fix: 

## `inputs.messages`

- **`assignedCluster`**
  - EN: Input {channel} assigned to Cluster {cluster}
  - ZH: 输入 {channel} 分配到集群 {cluster}
  - [ ] OK    Fix: 

- **`backupLoaded`**
  - EN: Input configuration loaded from backup.
  - ZH: 已从备份加载输入配置。
  - [ ] OK    Fix: 

- **`configExported`**
  - EN: Input configuration exported.
  - ZH: 输入配置已导出。
  - [ ] OK    Fix: 

- **`configImported`**
  - EN: Input configuration imported.
  - ZH: 输入配置已导入。
  - [ ] OK    Fix: 

- **`configLoaded`**
  - EN: Input configuration loaded.
  - ZH: 输入配置已加载。
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Input configuration saved.
  - ZH: 输入配置已保存。
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - ZH: 错误: {error}
  - [ ] OK    Fix: 

- **`noSnapshotSelected`**
  - EN: No snapshot selected.
  - ZH: 未选择快照。
  - [ ] OK    Fix: 

- **`scopeConfigured`**
  - EN: Scope configured for next snapshot.
  - ZH: 已为下一个快照配置范围。
  - [ ] OK    Fix: 

- **`scopeSaved`**
  - EN: Snapshot scope saved.
  - ZH: 快照范围已保存。
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - ZH: 请先在系统配置中选择项目文件夹。
  - [ ] OK    Fix: 

- **`setSingle`**
  - EN: Input {channel} set to Single
  - ZH: 输入 {channel} 设置为单独
  - [ ] OK    Fix: 

- **`snapshotDeleted`**
  - EN: Snapshot '{name}' deleted.
  - ZH: 快照 '{name}' 已删除。
  - [ ] OK    Fix: 

- **`snapshotLoaded`**
  - EN: Snapshot '{name}' loaded.
  - ZH: 快照 '{name}' 已加载。
  - [ ] OK    Fix: 

- **`snapshotLoadedWithoutScope`**
  - EN: Snapshot '{name}' loaded (without scope).
  - ZH: 快照 '{name}' 已加载（无范围）。
  - [ ] OK    Fix: 

- **`snapshotStored`**
  - EN: Snapshot '{name}' stored.
  - ZH: 快照 '{name}' 已保存。
  - [ ] OK    Fix: 

- **`snapshotUpdated`**
  - EN: Snapshot '{name}' updated.
  - ZH: 快照 '{name}' 已更新。
  - [ ] OK    Fix: 

- **`trackingDisabled`**
  - EN: Tracking disabled for Input {channel}
  - ZH: 输入 {channel} 的追踪已禁用
  - [ ] OK    Fix: 

- **`trackingSwitched`**
  - EN: Tracking switched from Input {from} to Input {to}
  - ZH: 追踪已从输入 {from} 切换到输入 {to}
  - [ ] OK    Fix: 

## `inputs.muteMacros`

- **`invertMutes`**
  - EN: INVERT MUTES
  - ZH: 反转静音
  - [ ] OK    Fix: 

- **`muteAll`**
  - EN: MUTE ALL
  - ZH: 全部静音
  - [ ] OK    Fix: 

- **`muteArrayPrefix`**
  - EN: MUTE ARRAY
  - ZH: 静音阵列
  - [ ] OK    Fix: 

- **`muteEven`**
  - EN: MUTE EVEN
  - ZH: 静音偶数
  - [ ] OK    Fix: 

- **`muteOdd`**
  - EN: MUTE ODD
  - ZH: 静音奇数
  - [ ] OK    Fix: 

- **`selectMacro`**
  - EN: Select Macro...
  - ZH: 选择宏...
  - [ ] OK    Fix: 

- **`unmuteAll`**
  - EN: UNMUTE ALL
  - ZH: 全部解除静音
  - [ ] OK    Fix: 

- **`unmuteArrayPrefix`**
  - EN: UNMUTE ARRAY
  - ZH: 解除阵列静音
  - [ ] OK    Fix: 

## `inputs.prefixes`

- **`delay`**
  - EN: Delay:
  - ZH: 延迟:
  - [ ] OK    Fix: 

## `inputs.sections`

- **`automotion`**
  - EN: AutomOtion
  - ZH: AutomOtion (自动移动)
  - [ ] OK    Fix: 

## `inputs.snapshots`

- **`selectSnapshot`**
  - EN: Select Snapshot...
  - ZH: 选择快照...
  - [ ] OK    Fix: 

## `inputs.tabs`

- **`gradientMaps`**
  - EN: Gradient Maps
  - ZH: 渐变图
  - [ ] OK    Fix: 

- **`inputParams`**
  - EN: Input Parameters
  - ZH: 输入参数
  - [ ] OK    Fix: 

- **`liveSourceHackoustics`**
  - EN: Live Source & Hackoustics
  - ZH: 现场声源 & Hackoustics (虚拟声学)
  - [ ] OK    Fix: 

- **`movements`**
  - EN: Movements
  - ZH: 运动
  - [ ] OK    Fix: 

- **`sampler`**
  - EN: Sampler
  - ZH: Sampler
  - [ ] OK    Fix: 

- **`visualisation`**
  - EN: Visualisation
  - ZH: 可视化
  - [ ] OK    Fix: 

## `inputs.toggles`

- **`absolute`**
  - EN: Absolute
  - ZH: 绝对
  - [ ] OK    Fix: 

- **`acousticPrecedence`**
  - EN: Acoustic Precedence
  - ZH: 声学优先
  - [ ] OK    Fix: 

- **`attenuationLawLog`**
  - EN: Log
  - ZH: 对数
  - [ ] OK    Fix: 

- **`constraintROff`**
  - EN: Constraint R: OFF
  - ZH: 约束R: 关
  - [ ] OK    Fix: 

- **`constraintROn`**
  - EN: Constraint R: ON
  - ZH: 约束R: 开
  - [ ] OK    Fix: 

- **`constraintXOff`**
  - EN: Constraint X: OFF
  - ZH: 约束X: 关
  - [ ] OK    Fix: 

- **`constraintXOn`**
  - EN: Constraint X: ON
  - ZH: 约束X: 开
  - [ ] OK    Fix: 

- **`constraintYOff`**
  - EN: Constraint Y: OFF
  - ZH: 约束Y: 关
  - [ ] OK    Fix: 

- **`constraintYOn`**
  - EN: Constraint Y: ON
  - ZH: 约束Y: 开
  - [ ] OK    Fix: 

- **`constraintZOff`**
  - EN: Constraint Z: OFF
  - ZH: 约束Z: 关
  - [ ] OK    Fix: 

- **`constraintZOn`**
  - EN: Constraint Z: ON
  - ZH: 约束Z: 开
  - [ ] OK    Fix: 

- **`flipXOff`**
  - EN: Flip X: OFF
  - ZH: 翻转X: 关
  - [ ] OK    Fix: 

- **`flipXOn`**
  - EN: Flip X: ON
  - ZH: 翻转X: 开
  - [ ] OK    Fix: 

- **`flipYOff`**
  - EN: Flip Y: OFF
  - ZH: 翻转Y: 关
  - [ ] OK    Fix: 

- **`flipYOn`**
  - EN: Flip Y: ON
  - ZH: 翻转Y: 开
  - [ ] OK    Fix: 

- **`flipZOff`**
  - EN: Flip Z: OFF
  - ZH: 翻转Z: 关
  - [ ] OK    Fix: 

- **`flipZOn`**
  - EN: Flip Z: ON
  - ZH: 翻转Z: 开
  - [ ] OK    Fix: 

- **`floorReflectionsOff`**
  - EN: Floor Reflections: OFF
  - ZH: 地板反射: 关
  - [ ] OK    Fix: 

- **`floorReflectionsOn`**
  - EN: Floor Reflections: ON
  - ZH: 地板反射: 开
  - [ ] OK    Fix: 

- **`highShelfOff`**
  - EN: High Shelf: OFF
  - ZH: 高频搁架: 关
  - [ ] OK    Fix: 

- **`highShelfOn`**
  - EN: High Shelf: ON
  - ZH: 高频搁架: 开
  - [ ] OK    Fix: 

- **`lfoOff`**
  - EN: L.F.O: OFF
  - ZH: L.F.O: 关
  - [ ] OK    Fix: 

- **`lfoOn`**
  - EN: L.F.O: ON
  - ZH: L.F.O: 开
  - [ ] OK    Fix: 

- **`liveSourceTamerOff`**
  - EN: Live Source Tamer: OFF
  - ZH: 驯服器: OFF
  - [ ] OK    Fix: 

- **`liveSourceTamerOn`**
  - EN: Live Source Tamer: ON
  - ZH: 驯服器: ON
  - [ ] OK    Fix: 

- **`lowCutOff`**
  - EN: Low Cut: OFF
  - ZH: 低切: 关
  - [ ] OK    Fix: 

- **`lowCutOn`**
  - EN: Low Cut: ON
  - ZH: 低切: 开
  - [ ] OK    Fix: 

- **`lsPeakOff`**
  - EN: Peak: OFF
  - ZH: 峰值: OFF
  - [ ] OK    Fix: 

- **`lsPeakOn`**
  - EN: Peak: ON
  - ZH: 峰值: ON
  - [ ] OK    Fix: 

- **`lsSlowOff`**
  - EN: Slow: OFF
  - ZH: 缓慢: OFF
  - [ ] OK    Fix: 

- **`lsSlowOn`**
  - EN: Slow: ON
  - ZH: 缓慢: ON
  - [ ] OK    Fix: 

- **`manual`**
  - EN: Manual
  - ZH: 手动
  - [ ] OK    Fix: 

- **`maxSpeedOff`**
  - EN: Max Speed: OFF
  - ZH: 最大速度: 关
  - [ ] OK    Fix: 

- **`maxSpeedOn`**
  - EN: Max Speed: ON
  - ZH: 最大速度: 开
  - [ ] OK    Fix: 

- **`minimalLatency`**
  - EN: Minimal Latency
  - ZH: 最小延时
  - [ ] OK    Fix: 

- **`pathModeOff`**
  - EN: Path Mode: OFF
  - ZH: 路径模式: 关
  - [ ] OK    Fix: 

- **`pathModeOn`**
  - EN: Path Mode: ON
  - ZH: 路径模式: 开
  - [ ] OK    Fix: 

- **`relative`**
  - EN: Relative
  - ZH: 相对
  - [ ] OK    Fix: 

- **`return`**
  - EN: Return
  - ZH: 返回
  - [ ] OK    Fix: 

- **`reverbSendsMuted`**
  - EN: Sends to Reverbs: Muted
  - ZH: 混响发送: 已静音
  - [ ] OK    Fix: 

- **`reverbSendsUnmuted`**
  - EN: Sends to Reverbs: Unmuted
  - ZH: 混响发送: 已启用
  - [ ] OK    Fix: 

- **`sidelinesOff`**
  - EN: Sidelines Off
  - ZH: 边线 关
  - [ ] OK    Fix: 

- **`sidelinesOn`**
  - EN: Sidelines On
  - ZH: 边线 开
  - [ ] OK    Fix: 

- **`stay`**
  - EN: Stay
  - ZH: 停留
  - [ ] OK    Fix: 

- **`trackingOff`**
  - EN: Tracking: OFF
  - ZH: 追踪: 关
  - [ ] OK    Fix: 

- **`trackingOn`**
  - EN: Tracking: ON
  - ZH: 追踪: 开
  - [ ] OK    Fix: 

- **`triggered`**
  - EN: Triggered
  - ZH: 触发
  - [ ] OK    Fix: 

## `inputs.visualisation`

- **`delay`**
  - EN: delay
  - ZH: 延迟
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: HF
damping
  - ZH: 高频
衰减
  - [ ] OK    Fix: 

- **`level`**
  - EN: level
  - ZH: 电平
  - [ ] OK    Fix: 

## `levelMeter`

- **`inputs`**
  - EN: Inputs
  - ZH: 输入
  - [ ] OK    Fix: 

- **`outputs`**
  - EN: Outputs
  - ZH: 输出
  - [ ] OK    Fix: 

- **`windowTitle`**
  - EN: Level Meters
  - ZH: 电平表
  - [ ] OK    Fix: 

## `levelMeter.buttons`

- **`clearSolo`**
  - EN: Clear Solo
  - ZH: 清除独奏
  - [ ] OK    Fix: 

- **`soloModeMulti`**
  - EN: Multi
  - ZH: 多选
  - [ ] OK    Fix: 

- **`soloModeSingle`**
  - EN: Single
  - ZH: 单选
  - [ ] OK    Fix: 

## `levelMeter.tooltips`

- **`clearSolo`**
  - EN: Disengage all solo toggles
  - ZH: 取消所有独奏
  - [ ] OK    Fix: 

- **`solo`**
  - EN: Display the contribution of the input to all outputs in the Level Meter display (in Single mode) and play binaural render of soloed inputs
  - ZH: 在电平表显示中显示输入对所有输出的贡献（单独模式下）并播放独奏输入的双耳渲染
  - [ ] OK    Fix: 

- **`soloMode`**
  - EN: Single: one input at a time. Multi: multiple inputs simultaneously.
  - ZH: 单选：一次只听一个输入。多选：可同时听多个输入。
  - [ ] OK    Fix: 

## `map`

- **`detachedMessage`**
  - EN: The map is displayed in a separate window.
  - ZH: 地图正在单独的窗口中显示。
  - [ ] OK    Fix: 

- **`reattach`**
  - EN: Re-attach Map
  - ZH: 重新附加地图
  - [ ] OK    Fix: 

## `map.buttons`

- **`fitInputs`**
  - EN: Fit All Inputs to Screen
  - ZH: 适配所有输入到屏幕
  - [ ] OK    Fix: 

- **`fitStage`**
  - EN: Fit Stage to Screen
  - ZH: 适配舞台到屏幕
  - [ ] OK    Fix: 

- **`hideLevels`**
  - EN: Hide Levels
  - ZH: 隐藏电平
  - [ ] OK    Fix: 

- **`showLevels`**
  - EN: Show Levels
  - ZH: 显示电平
  - [ ] OK    Fix: 

## `map.labels`

- **`reverbPrefix`**
  - EN: R
  - ZH: 混
  - [ ] OK    Fix: 

## `map.messages`

- **`assignedCluster`**
  - EN: Input {channel} assigned to Cluster {cluster}
  - ZH: 输入 {channel} 已分配到集群 {cluster}
  - [ ] OK    Fix: 

- **`assignedClusterMulti`**
  - EN: {count} inputs assigned to Cluster {cluster}
  - ZH: {count} 个输入已分配到集群 {cluster}
  - [ ] OK    Fix: 

- **`clusterBrokenUp`**
  - EN: Cluster {cluster} broken up
  - ZH: 集群 {cluster} 已解散
  - [ ] OK    Fix: 

- **`setSingle`**
  - EN: Input {channel} removed from cluster
  - ZH: 输入 {channel} 已从集群中移除
  - [ ] OK    Fix: 

- **`setSingleMulti`**
  - EN: {count} inputs removed from clusters
  - ZH: {count} 个输入已从集群中移除
  - [ ] OK    Fix: 

## `map.tooltips`

- **`detach`**
  - EN: Detach the map into a separate window for dual-screen setups
  - ZH: 将地图分离到单独窗口以适配双屏设置
  - [ ] OK    Fix: 

- **`fitInputs`**
  - EN: Adjust the zoom and panning to fit all visible inputs on the display
  - ZH: 调整缩放和平移以显示所有可见输入
  - [ ] OK    Fix: 

- **`fitStage`**
  - EN: Adjust the zoom and panning to fit the stage on the display
  - ZH: 调整缩放和平移以显示舞台
  - [ ] OK    Fix: 

- **`levels`**
  - EN: Display the levels for inputs and outputs on the map
  - ZH: 在地图上显示输入和输出的电平
  - [ ] OK    Fix: 

## `network.buttons`

- **`add`**
  - EN: ADD
  - ZH: 添加
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - ZH: 导出
  - [ ] OK    Fix: 

- **`findMyRemote`**
  - EN: Find My Remote
  - ZH: 查找遥控器
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - ZH: 导入
  - [ ] OK    Fix: 

- **`openLogWindow`**
  - EN: Open Log Window
  - ZH: 打开日志窗口
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Backup
  - ZH: 加载备份
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Network Config
  - ZH: 重新加载网络配置
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Network Config
  - ZH: 保存网络配置
  - [ ] OK    Fix: 

## `network.dialogs`

- **`exportConfig`**
  - EN: Export Network Configuration
  - ZH: 导出网络配置
  - [ ] OK    Fix: 

- **`findMyRemoteMessage`**
  - EN: Enter the password for your remote device:
  - ZH: 输入遥控设备的密码：
  - [ ] OK    Fix: 

- **`findMyRemotePassword`**
  - EN: Password:
  - ZH: 密码：
  - [ ] OK    Fix: 

- **`findMyRemoteTitle`**
  - EN: Find My Remote
  - ZH: 查找遥控器
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Network Configuration
  - ZH: 导入网络配置
  - [ ] OK    Fix: 

- **`removeTargetMessage`**
  - EN: Delete target '{name}'?
  - ZH: 删除目标 '{name}'？
  - [ ] OK    Fix: 

- **`removeTargetTitle`**
  - EN: Remove Target
  - ZH: 移除目标
  - [ ] OK    Fix: 

- **`trackingConflictsContinue`**
  - EN: Continue
  - ZH: 继续
  - [ ] OK    Fix: 

- **`trackingConflictsMessage`**
  - EN: 
Only one tracked input per cluster is allowed. If you continue, tracking will be kept only for the first input in each cluster.
  - ZH: 
每个集群仅允许一个追踪输入。如果继续，每个集群将仅保留第一个输入的追踪。
  - [ ] OK    Fix: 

- **`trackingConflictsTitle`**
  - EN: Tracking Conflicts Detected
  - ZH: 检测到追踪冲突
  - [ ] OK    Fix: 

## `network.help`

- **`addTarget`**
  - EN: Add new network target.
  - ZH: 添加新的网络目标。
  - [ ] OK    Fix: 

- **`admMapping`**
  - EN: Select an ADM-OSC mapping to configure. Cart = Cartesian (xyz), Polar = spherical (aed).
  - ZH: 选择要配置的 ADM-OSC 映射。Cart = 笛卡尔 (xyz)，Polar = 球坐标 (aed)。
  - [ ] OK    Fix: 

- **`admMappingPanel`**
  - EN: Drag dots to edit mapping. Click axis title to swap, click Flip to invert. Hold Shift to edit both sides symmetrically.
  - ZH: 拖动点以编辑映射。点击轴标题进行交换，点击 Flip 进行反转。按住 Shift 以对称编辑两侧。
  - [ ] OK    Fix: 

- **`currentIP`**
  - EN: IP address of the Processor.
  - ZH: 处理器的 IP 地址。
  - [ ] OK    Fix: 

- **`dataMode`**
  - EN: Select UDP or TCP data transmission.
  - ZH: 选择 UDP 或 TCP 数据传输。
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Network Configuration to file.
  - ZH: 将网络配置导出到文件。
  - [ ] OK    Fix: 

- **`findMyRemote`**
  - EN: Make your Remote Flash and Buzz to Find it.
  - ZH: 让遥控器闪烁和振动以便找到它。
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Network Configuration from file.
  - ZH: 从文件导入网络配置。
  - [ ] OK    Fix: 

- **`networkInterface`**
  - EN: Select the Network Interface.
  - ZH: 选择网络接口。
  - [ ] OK    Fix: 

- **`openLogWindow`**
  - EN: Open Network Logging window.
  - ZH: 打开网络日志窗口。
  - [ ] OK    Fix: 

- **`oscQueryEnable`**
  - EN: Enable/disable OSC Query server for automatic parameter discovery via HTTP/WebSocket.
  - ZH: 启用/禁用 OSC Query 服务器，通过 HTTP/WebSocket 自动发现参数。
  - [ ] OK    Fix: 

- **`oscQueryPort`**
  - EN: HTTP port for OSC Query discovery. Other apps can browse parameters at http://localhost:<port>/
  - ZH: OSC Query 发现的 HTTP 端口。其他应用可在 http://localhost:<端口>/ 浏览参数。
  - [ ] OK    Fix: 

- **`oscSourceFilter`**
  - EN: Filter incoming OSC: Accept All sources or only Registered connections with Rx enabled.
  - ZH: 过滤传入 OSC：接受所有来源或仅接受已注册且启用 Rx 的连接。
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Select the Protocol: DISABLED, OSC, REMOTE, or ADM-OSC.
  - ZH: 选择协议：DISABLED、OSC、REMOTE 或 ADM-OSC。
  - [ ] OK    Fix: 

- **`psnInterface`**
  - EN: Network interface for PSN multicast reception
  - ZH: 用于 PSN 多播接收的网络接口
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Network Configuration from backup file.
  - ZH: 从备份文件重新加载网络配置。
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Network Configuration from file.
  - ZH: 从文件重新加载网络配置。
  - [ ] OK    Fix: 

- **`removeTarget`**
  - EN: Delete this Network Target.
  - ZH: 删除此网络目标。
  - [ ] OK    Fix: 

- **`rxEnable`**
  - EN: Enable or Disable Data Reception.
  - ZH: 启用或禁用数据接收。
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Network Configuration to file.
  - ZH: 将网络配置保存到文件。
  - [ ] OK    Fix: 

- **`targetIP`**
  - EN: IP Address of the Target (use 127.0.0.1 for local host).
  - ZH: 目标 IP 地址（本机使用 127.0.0.1）。
  - [ ] OK    Fix: 

- **`targetName`**
  - EN: Network Target Name.
  - ZH: 网络目标名称。
  - [ ] OK    Fix: 

- **`tcpPort`**
  - EN: TCP Receive Port of the Processor.
  - ZH: 处理器的 TCP 接收端口。
  - [ ] OK    Fix: 

- **`trackingEnabled`**
  - EN: Enable or Disable Incoming Tracking data processing.
  - ZH: 启用或禁用传入追踪数据处理。
  - [ ] OK    Fix: 

- **`trackingFlipX`**
  - EN: Invert Axis of Tracking X Coordinate.
  - ZH: 反转追踪 X 坐标轴。
  - [ ] OK    Fix: 

- **`trackingFlipY`**
  - EN: Invert Axis of Tracking Y Coordinate.
  - ZH: 反转追踪 Y 坐标轴。
  - [ ] OK    Fix: 

- **`trackingFlipZ`**
  - EN: Invert Axis of Tracking Z Coordinate.
  - ZH: 反转追踪 Z 坐标轴。
  - [ ] OK    Fix: 

- **`trackingOffsetX`**
  - EN: Offset Tracking X Coordinate.
  - ZH: 追踪 X 坐标偏移。
  - [ ] OK    Fix: 

- **`trackingOffsetY`**
  - EN: Offset Tracking Y Coordinate.
  - ZH: 追踪 Y 坐标偏移。
  - [ ] OK    Fix: 

- **`trackingOffsetZ`**
  - EN: Offset Tracking Z Coordinate.
  - ZH: 追踪 Z 坐标偏移。
  - [ ] OK    Fix: 

- **`trackingOscPath`**
  - EN: OSC Path for the Tracking in OSC Mode (starts with a /)
  - ZH: OSC 模式下追踪的 OSC 路径（以 / 开头）
  - [ ] OK    Fix: 

- **`trackingPort`**
  - EN: Specify the Port to receive Tracking data.
  - ZH: 指定接收追踪数据的端口。
  - [ ] OK    Fix: 

- **`trackingProtocol`**
  - EN: Select the type of Tracking Protocol.
  - ZH: 选择追踪协议类型。
  - [ ] OK    Fix: 

- **`trackingScaleX`**
  - EN: Scale Tracking X Coordinate.
  - ZH: 追踪 X 坐标缩放。
  - [ ] OK    Fix: 

- **`trackingScaleY`**
  - EN: Scale Tracking Y Coordinate.
  - ZH: 追踪 Y 坐标缩放。
  - [ ] OK    Fix: 

- **`trackingScaleZ`**
  - EN: Scale Tracking Z Coordinate.
  - ZH: 追踪 Z 坐标缩放。
  - [ ] OK    Fix: 

- **`txEnable`**
  - EN: Enable or Disable Data Transmission.
  - ZH: 启用或禁用数据发送。
  - [ ] OK    Fix: 

- **`txPort`**
  - EN: Transmit Port for this Target.
  - ZH: 此目标的发送端口。
  - [ ] OK    Fix: 

- **`udpPort`**
  - EN: UDP Receive Port of the Processor.
  - ZH: 处理器的 UDP 接收端口。
  - [ ] OK    Fix: 

## `network.labels`

- **`admMapping`**
  - EN: Mapping:
  - ZH: 映射:
  - [ ] OK    Fix: 

- **`currentIPv4`**
  - EN: Current IPv4:
  - ZH: 当前IPv4:
  - [ ] OK    Fix: 

- **`interface`**
  - EN: Network Interface:
  - ZH: 网络接口:
  - [ ] OK    Fix: 

- **`localhost`**
  - EN: Localhost (127.0.0.1)
  - ZH: 本机 (127.0.0.1)
  - [ ] OK    Fix: 

- **`mqttHost`**
  - EN: Host:
  - ZH: 主机:
  - [ ] OK    Fix: 

- **`mqttJsonQ`**
  - EN: Q:
  - ZH: Q:
  - [ ] OK    Fix: 

- **`mqttJsonX`**
  - EN: X:
  - ZH: X:
  - [ ] OK    Fix: 

- **`mqttJsonY`**
  - EN: Y:
  - ZH: Y:
  - [ ] OK    Fix: 

- **`mqttJsonZ`**
  - EN: Z:
  - ZH: Z:
  - [ ] OK    Fix: 

- **`mqttTagIds`**
  - EN: Tag IDs...
  - ZH: 标签 ID...
  - [ ] OK    Fix: 

- **`mqttTopic`**
  - EN: Topic:
  - ZH: 主题:
  - [ ] OK    Fix: 

- **`notAvailable`**
  - EN: Not available
  - ZH: 不可用
  - [ ] OK    Fix: 

- **`offsetX`**
  - EN: Offset X:
  - ZH: 偏移X:
  - [ ] OK    Fix: 

- **`offsetY`**
  - EN: Offset Y:
  - ZH: 偏移Y:
  - [ ] OK    Fix: 

- **`offsetZ`**
  - EN: Offset Z:
  - ZH: 偏移Z:
  - [ ] OK    Fix: 

- **`oscPath`**
  - EN: OSC Path:
  - ZH: OSC 路径:
  - [ ] OK    Fix: 

- **`oscQuery`**
  - EN: OSC Query:
  - ZH: OSC查询:
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Protocol:
  - ZH: 协议:
  - [ ] OK    Fix: 

- **`psnInterface`**
  - EN: PSN Interface:
  - ZH: PSN 接口:
  - [ ] OK    Fix: 

- **`rxPort`**
  - EN: Rx Port:
  - ZH: 接收端口:
  - [ ] OK    Fix: 

- **`scaleX`**
  - EN: Scale X:
  - ZH: 缩放X:
  - [ ] OK    Fix: 

- **`scaleY`**
  - EN: Scale Y:
  - ZH: 缩放Y:
  - [ ] OK    Fix: 

- **`scaleZ`**
  - EN: Scale Z:
  - ZH: 缩放Z:
  - [ ] OK    Fix: 

- **`tcpPort`**
  - EN: TCP Port:
  - ZH: TCP端口:
  - [ ] OK    Fix: 

- **`udpPort`**
  - EN: UDP Port:
  - ZH: UDP端口:
  - [ ] OK    Fix: 

## `network.messages`

- **`configExported`**
  - EN: Network configuration exported.
  - ZH: 网络配置已导出。
  - [ ] OK    Fix: 

- **`configImported`**
  - EN: Network configuration imported.
  - ZH: 网络配置已导入。
  - [ ] OK    Fix: 

- **`configLoadedFromBackup`**
  - EN: Network configuration loaded from backup.
  - ZH: 已从备份加载网络配置。
  - [ ] OK    Fix: 

- **`configNotFound`**
  - EN: Network config file not found.
  - ZH: 未找到网络配置文件。
  - [ ] OK    Fix: 

- **`configReloaded`**
  - EN: Network configuration reloaded.
  - ZH: 网络配置已重新加载。
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Network configuration saved.
  - ZH: 网络配置已保存。
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - ZH: 错误: {error}
  - [ ] OK    Fix: 

- **`findDeviceSent`**
  - EN: Find Device command sent.
  - ZH: 查找设备命令已发送。
  - [ ] OK    Fix: 

- **`maxTargetsReached`**
  - EN: Maximum Number of Targets/Servers Reached.
  - ZH: 已达到最大目标/服务器数量。
  - [ ] OK    Fix: 

- **`noBackupFound`**
  - EN: No backup files found.
  - ZH: 未找到备份文件。
  - [ ] OK    Fix: 

- **`onlyOneRemote`**
  - EN: Only one REMOTE connection is allowed.
  - ZH: 只允许一个遥控连接。
  - [ ] OK    Fix: 

- **`oscManagerError`**
  - EN: Error: OSC Manager not available
  - ZH: 错误：OSC 管理器不可用
  - [ ] OK    Fix: 

- **`passwordEmpty`**
  - EN: Password cannot be empty.
  - ZH: 密码不能为空。
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - ZH: 请先在系统配置中选择项目文件夹。
  - [ ] OK    Fix: 

## `network.protocols`

- **`admOsc`**
  - EN: ADM-OSC
  - ZH: ADM-OSC
  - [ ] OK    Fix: 

- **`disabled`**
  - EN: DISABLED
  - ZH: 禁用
  - [ ] OK    Fix: 

- **`mqtt`**
  - EN: MQTT
  - ZH: MQTT
  - [ ] OK    Fix: 

- **`osc`**
  - EN: OSC
  - ZH: OSC
  - [ ] OK    Fix: 

- **`psn`**
  - EN: PosiStageNet (PSN)
  - ZH: PosiStageNet (PSN)
  - [ ] OK    Fix: 

- **`qlab`**
  - EN: QLab
  - ZH: QLab
  - [ ] OK    Fix: 

- **`remote`**
  - EN: Remote
  - ZH: 遥控
  - [ ] OK    Fix: 

- **`rttrp`**
  - EN: RTTrP
  - ZH: RTTrP
  - [ ] OK    Fix: 

- **`tcp`**
  - EN: TCP
  - ZH: TCP
  - [ ] OK    Fix: 

- **`udp`**
  - EN: UDP
  - ZH: UDP
  - [ ] OK    Fix: 

## `network.sections`

- **`admOsc`**
  - EN: ADM-OSC Mappings
  - ZH: ADM-OSC 映射
  - [ ] OK    Fix: 

- **`connections`**
  - EN: Network Connections
  - ZH: 网络连接
  - [ ] OK    Fix: 

- **`network`**
  - EN: Network
  - ZH: 网络
  - [ ] OK    Fix: 

- **`tracking`**
  - EN: Tracking
  - ZH: 追踪
  - [ ] OK    Fix: 

## `network.table`

- **`defaultTarget`**
  - EN: Target {num}
  - ZH: 目标 {num}
  - [ ] OK    Fix: 

- **`ipv4Address`**
  - EN: IPv4 Address
  - ZH: IPv4地址
  - [ ] OK    Fix: 

- **`mode`**
  - EN: Mode
  - ZH: 模式
  - [ ] OK    Fix: 

- **`name`**
  - EN: Name
  - ZH: 名称
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Protocol
  - ZH: 协议
  - [ ] OK    Fix: 

- **`rx`**
  - EN: Rx
  - ZH: 接收
  - [ ] OK    Fix: 

- **`tx`**
  - EN: Tx
  - ZH: 发送
  - [ ] OK    Fix: 

- **`txPort`**
  - EN: Tx Port
  - ZH: 发送端口
  - [ ] OK    Fix: 

## `network.toggles`

- **`disabled`**
  - EN: Disabled
  - ZH: 已禁用
  - [ ] OK    Fix: 

- **`enabled`**
  - EN: Enabled
  - ZH: 已启用
  - [ ] OK    Fix: 

- **`flipXOff`**
  - EN: Flip X: OFF
  - ZH: 翻转X: 关
  - [ ] OK    Fix: 

- **`flipXOn`**
  - EN: Flip X: ON
  - ZH: 翻转X: 开
  - [ ] OK    Fix: 

- **`flipYOff`**
  - EN: Flip Y: OFF
  - ZH: 翻转Y: 关
  - [ ] OK    Fix: 

- **`flipYOn`**
  - EN: Flip Y: ON
  - ZH: 翻转Y: 开
  - [ ] OK    Fix: 

- **`flipZOff`**
  - EN: Flip Z: OFF
  - ZH: 翻转Z: 关
  - [ ] OK    Fix: 

- **`flipZOn`**
  - EN: Flip Z: ON
  - ZH: 翻转Z: 开
  - [ ] OK    Fix: 

- **`off`**
  - EN: OFF
  - ZH: 关
  - [ ] OK    Fix: 

- **`on`**
  - EN: ON
  - ZH: 开
  - [ ] OK    Fix: 

- **`oscFilterAcceptAll`**
  - EN: OSC Filter: Accept All
  - ZH: OSC过滤: 接受全部
  - [ ] OK    Fix: 

- **`oscFilterRegisteredOnly`**
  - EN: OSC Filter: Registered Only
  - ZH: OSC过滤: 仅已注册
  - [ ] OK    Fix: 

- **`trackingOff`**
  - EN: Tracking: OFF
  - ZH: 追踪: 关
  - [ ] OK    Fix: 

- **`trackingOn`**
  - EN: Tracking: ON
  - ZH: 追踪: 开
  - [ ] OK    Fix: 

## `networkLog`

- **`windowTitle`**
  - EN: Network Log
  - ZH: 网络日志
  - [ ] OK    Fix: 

## `networkLog.columns`

- **`address`**
  - EN: Address
  - ZH: 地址
  - [ ] OK    Fix: 

- **`arguments`**
  - EN: Arguments
  - ZH: 参数
  - [ ] OK    Fix: 

- **`direction`**
  - EN: Dir
  - ZH: 方向
  - [ ] OK    Fix: 

- **`ip`**
  - EN: IP
  - ZH: IP
  - [ ] OK    Fix: 

- **`origin`**
  - EN: Origin
  - ZH: 来源
  - [ ] OK    Fix: 

- **`port`**
  - EN: Port
  - ZH: 端口
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Protocol
  - ZH: 协议
  - [ ] OK    Fix: 

- **`time`**
  - EN: Time
  - ZH: 时间
  - [ ] OK    Fix: 

- **`transport`**
  - EN: Trans
  - ZH: 传输
  - [ ] OK    Fix: 

## `networkLog.controls`

- **`clear`**
  - EN: CLEAR
  - ZH: 清除
  - [ ] OK    Fix: 

- **`export`**
  - EN: EXPORT
  - ZH: 导出
  - [ ] OK    Fix: 

- **`hideHeartbeat`**
  - EN: Hide Heartbeat
  - ZH: 隐藏心跳
  - [ ] OK    Fix: 

- **`logging`**
  - EN: Logging
  - ZH: 日志记录
  - [ ] OK    Fix: 

## `networkLog.dialogs`

- **`exportCompleteMessage`**
  - EN: Log exported to: {path}
  - ZH: 日志已导出到: {path}
  - [ ] OK    Fix: 

- **`exportCompleteTitle`**
  - EN: Export Complete
  - ZH: 导出完成
  - [ ] OK    Fix: 

- **`exportFailedMessage`**
  - EN: Could not write to file: {path}
  - ZH: 无法写入文件: {path}
  - [ ] OK    Fix: 

- **`exportFailedTitle`**
  - EN: Export Failed
  - ZH: 导出失败
  - [ ] OK    Fix: 

## `networkLog.exportMenu`

- **`exportAll`**
  - EN: Export All
  - ZH: 导出全部
  - [ ] OK    Fix: 

- **`exportFiltered`**
  - EN: Export Filtered
  - ZH: 导出已过滤
  - [ ] OK    Fix: 

## `networkLog.filterModes`

- **`clientIp`**
  - EN: Client IP
  - ZH: 客户端IP
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Protocol
  - ZH: 协议
  - [ ] OK    Fix: 

- **`rejected`**
  - EN: Rejected
  - ZH: 已拒绝
  - [ ] OK    Fix: 

- **`tcpUdp`**
  - EN: TCP/UDP
  - ZH: TCP/UDP
  - [ ] OK    Fix: 

## `networkLog.filterToggles`

- **`incoming`**
  - EN: Incoming
  - ZH: 传入
  - [ ] OK    Fix: 

- **`outgoing`**
  - EN: Outgoing
  - ZH: 传出
  - [ ] OK    Fix: 

- **`tcp`**
  - EN: TCP
  - ZH: TCP
  - [ ] OK    Fix: 

- **`udp`**
  - EN: UDP
  - ZH: UDP
  - [ ] OK    Fix: 

## `networkLog.status`

- **`rejected`**
  - EN: REJECTED
  - ZH: 已拒绝
  - [ ] OK    Fix: 

## `outputs.arrayModes`

- **`absolute`**
  - EN: ABSOLUTE
  - ZH: 绝对
  - [ ] OK    Fix: 

- **`array`**
  - EN: Array
  - ZH: 阵列
  - [ ] OK    Fix: 

- **`off`**
  - EN: OFF
  - ZH: 关
  - [ ] OK    Fix: 

- **`relative`**
  - EN: RELATIVE
  - ZH: 相对
  - [ ] OK    Fix: 

- **`single`**
  - EN: Single
  - ZH: 单独
  - [ ] OK    Fix: 

## `outputs.buttons`

- **`arrayHidden`**
  - EN: Array Hidden on Map
  - ZH: 阵列在地图上隐藏
  - [ ] OK    Fix: 

- **`arrayVisible`**
  - EN: Array Visible on Map
  - ZH: 阵列在地图上可见
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - ZH: 导出
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - ZH: 导入
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Backup
  - ZH: 加载备份
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Output Config
  - ZH: 重新加载输出配置
  - [ ] OK    Fix: 

- **`speakerHidden`**
  - EN: Speaker Hidden on Map
  - ZH: 扬声器在地图上隐藏
  - [ ] OK    Fix: 

- **`speakerVisible`**
  - EN: Speaker Visible on Map
  - ZH: 扬声器在地图上可见
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Output Config
  - ZH: 保存输出配置
  - [ ] OK    Fix: 

- **`wizardOfOutZ`**
  - EN: Wizard of OutZ...
  - ZH: OutZ向导...
  - [ ] OK    Fix: 

## `outputs.coordModes`

- **`xyz`**
  - EN: XYZ
  - ZH: XYZ
  - [ ] OK    Fix: 

## `outputs.dialogs`

- **`export`**
  - EN: Export Output Configuration
  - ZH: 导出输出配置
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import Output Configuration
  - ZH: 导入输出配置
  - [ ] OK    Fix: 

## `outputs.help`

- **`angleOff`**
  - EN: Output Channel Will Not Amplify Objects in this Angle in Front of it. (changes may affect the rest of the array)
  - ZH: 输出通道将不放大此角度内位于其前方的对象。（更改可能影响阵列的其余部分）
  - [ ] OK    Fix: 

- **`angleOn`**
  - EN: Output Channel Will Amplify Objects in this Angle in its Back. (changes may affect the rest of the array)
  - ZH: 输出通道将放大此角度内位于其后方的对象。（更改可能影响阵列的其余部分）
  - [ ] OK    Fix: 

- **`applyToArray`**
  - EN: Apply Changes to the rest of the Array (Absolute value or Relative changes).
  - ZH: 将更改应用到阵列的其余部分（绝对值或相对更改）。
  - [ ] OK    Fix: 

- **`arraySelector`**
  - EN: Selected Output Channel is part of Array.
  - ZH: 选定的输出通道属于阵列的一部分。
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Output Channel Attenuation. (changes may affect the rest of the array)
  - ZH: 输出通道衰减。（更改可能影响阵列的其余部分）
  - [ ] OK    Fix: 

- **`channelSelector`**
  - EN: Output Channel Number and Selection.
  - ZH: 输出通道编号和选择。
  - [ ] OK    Fix: 

- **`coordMode`**
  - EN: Coordinate display mode: Cartesian (X/Y/Z), Cylindrical (radius/azimuth/height), or Spherical (radius/azimuth/elevation).
  - ZH: 坐标显示模式：笛卡尔坐标（X/Y/Z）、柱面坐标（半径/方位角/高度）或球面坐标（半径/方位角/仰角）。
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Output Channel Delay (positive values) or Latency Compensation (negative values). (changes may affect the rest of the array)
  - ZH: 输出通道延迟（正值）或延迟补偿（负值）。（更改可能影响阵列的其余部分）
  - [ ] OK    Fix: 

- **`directional`**
  - EN: Output Channel Directional Control. Drag to change orientation, Shift+drag for Angle Off, Alt+drag for Angle On. (changes may affect the rest of the array)
  - ZH: 输出通道方向控制。拖动改变朝向，Shift+拖动调整 Angle Off，Alt+拖动调整 Angle On。（更改可能影响阵列的其余部分）
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Ratio of Distance Attenuation for Selected Output. (changes may affect the rest of the array)
  - ZH: 所选输出的距离衰减比率。（更改可能影响阵列的其余部分）
  - [ ] OK    Fix: 

- **`eqBandToggle`**
  - EN: Toggle Band {band} on/off. When off the band is bypassed.
  - ZH: 开启/关闭频段{band}。关闭时频段被旁路。
  - [ ] OK    Fix: 

- **`eqEnable`**
  - EN: Enable or disable EQ processing for this output.
  - ZH: 启用或禁用此输出的EQ处理。
  - [ ] OK    Fix: 

- **`eqFlatten`**
  - EN: Long press to reset all EQ bands to their default values.
  - ZH: 长按重置所有EQ频段为默认值。
  - [ ] OK    Fix: 

- **`eqFreq`**
  - EN: Output EQ Band {band} frequency (20 Hz - 20 kHz).
  - ZH: 输出EQ频段{band}频率 (20 Hz - 20 kHz)。
  - [ ] OK    Fix: 

- **`eqGain`**
  - EN: Output EQ Band {band} gain (-24 to +24 dB).
  - ZH: 输出EQ频段{band}增益 (-24至+24 dB)。
  - [ ] OK    Fix: 

- **`eqQ`**
  - EN: Output EQ Band {band} Q factor (0.1 - 10.0).
  - ZH: 输出EQ频段{band} Q因子 (0.1 - 10.0)。
  - [ ] OK    Fix: 

- **`eqResetBand`**
  - EN: Long press to reset Band {band} to its default values.
  - ZH: 长按重置频段{band}为默认值。
  - [ ] OK    Fix: 

- **`eqShape`**
  - EN: Output EQ Band {band} filter shape.
  - ZH: 输出EQ频段{band}滤波器形状。
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Output Configuration to file (with file explorer window).
  - ZH: 将输出配置导出到文件（通过文件浏览器窗口）。
  - [ ] OK    Fix: 

- **`floorReflections`**
  - EN: Enable or Disable the Floor Reflections for this Speaker.
  - ZH: 启用或禁用此扬声器的地板反射。
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: Loss of High Frequency Depending on Distance from Object to Output. (changes may affect the rest of the array)
  - ZH: 根据对象到输出的距离产生的高频损失。（更改可能影响阵列的其余部分）
  - [ ] OK    Fix: 

- **`hParallax`**
  - EN: Horizontal Distance from Speaker to 'Targeted' Listener. (changes may affect the rest of the array)
  - ZH: 扬声器到“目标”听众的水平距离。（更改可能影响阵列的其余部分）
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Output Configuration from file (with file explorer window).
  - ZH: 从文件导入输出配置（通过文件浏览器窗口）。
  - [ ] OK    Fix: 

- **`liveSource`**
  - EN: Disables Live Source Attenuation for Selected Output. (changes may affect the rest of the array)
  - ZH: 禁用所选输出的现场声源衰减。（更改可能影响阵列的其余部分）
  - [ ] OK    Fix: 

- **`mapVisibility`**
  - EN: Make Visible or Hide The Selected Output on the Map.
  - ZH: 在地图上显示或隐藏选定的输出。
  - [ ] OK    Fix: 

- **`minLatency`**
  - EN: Disables Minimal Latency Mode for Selected Output. (changes may affect the rest of the array)
  - ZH: 禁用所选输出的最小延迟模式。（更改可能影响阵列的其余部分）
  - [ ] OK    Fix: 

- **`nameEditor`**
  - EN: Displayed Output Channel Name (editable).
  - ZH: 显示的输出通道名称（可编辑）。
  - [ ] OK    Fix: 

- **`pitch`**
  - EN: Output Channel Vertical Orientation used to Determine which Objects get amplified. (changes may affect the rest of the array)
  - ZH: 用于确定哪些对象被放大的输出通道垂直朝向。（更改可能影响阵列的其余部分）
  - [ ] OK    Fix: 

- **`position1`**
  - EN: Output Channel {name} ({unit}).
  - ZH: 输出通道{name}（{unit}）。
  - [ ] OK    Fix: 

- **`position2`**
  - EN: Output Channel {name} ({unit}).
  - ZH: 输出通道{name}（{unit}）。
  - [ ] OK    Fix: 

- **`position3`**
  - EN: Output Channel {name} ({unit}).
  - ZH: 输出通道{name}（{unit}）。
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Output Configuration from backup file.
  - ZH: 从备份文件重新加载输出配置。
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Output Configuration from file.
  - ZH: 从文件重新加载输出配置。
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Output Configuration to file (with backup).
  - ZH: 将输出配置存储到文件（含备份）。
  - [ ] OK    Fix: 

- **`vParallax`**
  - EN: Vertical Distance from Speaker to 'Targeted' Listener. Positive when the Speaker is Below the head of the Listener. (changes may affect the rest of the array)
  - ZH: 扬声器到“目标”听众的垂直距离。当扬声器位于听众头部下方时为正值。（更改可能影响阵列的其余部分）
  - [ ] OK    Fix: 

- **`wizardOfOutZ`**
  - EN: Open Wizard of OutZ to Position Speaker Arrays Conveniently.
  - ZH: 打开 Wizard of OutZ 以便捷地定位扬声器阵列。
  - [ ] OK    Fix: 

## `outputs.labels`

- **`angleOff`**
  - EN: Angle Off:
  - ZH: 关闭角度:
  - [ ] OK    Fix: 

- **`angleOn`**
  - EN: Angle On:
  - ZH: 开启角度:
  - [ ] OK    Fix: 

- **`applyToArray`**
  - EN: Apply to Array:
  - ZH: 应用到阵列:
  - [ ] OK    Fix: 

- **`array`**
  - EN: Array:
  - ZH: 阵列:
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Attenuation:
  - ZH: 衰减:
  - [ ] OK    Fix: 

- **`coordinates`**
  - EN: Coordinates:
  - ZH: 坐标:
  - [ ] OK    Fix: 

- **`delay`**
  - EN: Delay:
  - ZH: 延迟:
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Delay/Latency:
  - ZH: 延迟/延时:
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Distance Atten:
  - ZH: 距离衰减:
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: HF Damping:
  - ZH: 高频衰减:
  - [ ] OK    Fix: 

- **`hParallax`**
  - EN: Horizontal Parallax:
  - ZH: 水平视差:
  - [ ] OK    Fix: 

- **`latency`**
  - EN: Latency:
  - ZH: 延时:
  - [ ] OK    Fix: 

- **`name`**
  - EN: Name:
  - ZH: 名称:
  - [ ] OK    Fix: 

- **`orientation`**
  - EN: Orientation:
  - ZH: 朝向:
  - [ ] OK    Fix: 

- **`pitch`**
  - EN: Pitch:
  - ZH: 俯仰:
  - [ ] OK    Fix: 

- **`positionX`**
  - EN: Position X:
  - ZH: 位置X:
  - [ ] OK    Fix: 

- **`positionY`**
  - EN: Position Y:
  - ZH: 位置Y:
  - [ ] OK    Fix: 

- **`positionZ`**
  - EN: Position Z:
  - ZH: 位置Z:
  - [ ] OK    Fix: 

- **`vParallax`**
  - EN: Vertical Parallax:
  - ZH: 垂直视差:
  - [ ] OK    Fix: 

## `outputs.messages`

- **`assignedToArray`**
  - EN: Output {num} assigned to Array {array}
  - ZH: 输出 {num} 分配到阵列 {array}
  - [ ] OK    Fix: 

- **`backupLoaded`**
  - EN: Output configuration loaded from backup.
  - ZH: 已从备份加载输出配置。
  - [ ] OK    Fix: 

- **`configExported`**
  - EN: Output configuration exported.
  - ZH: 输出配置已导出。
  - [ ] OK    Fix: 

- **`configImported`**
  - EN: Output configuration imported.
  - ZH: 输出配置已导入。
  - [ ] OK    Fix: 

- **`configLoaded`**
  - EN: Output configuration loaded.
  - ZH: 输出配置已加载。
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Output configuration saved.
  - ZH: 输出配置已保存。
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - ZH: 错误: {error}
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - ZH: 请先在系统配置中选择项目文件夹。
  - [ ] OK    Fix: 

- **`setToSingle`**
  - EN: Output {num} set to Single
  - ZH: 输出 {num} 设置为单独
  - [ ] OK    Fix: 

## `outputs.tabs`

- **`eq`**
  - EN: Output EQ
  - ZH: 输出EQ
  - [ ] OK    Fix: 

- **`parameters`**
  - EN: Output Parameters
  - ZH: 输出参数
  - [ ] OK    Fix: 

## `outputs.toggles`

- **`floorReflectionsOff`**
  - EN: Floor Reflections: OFF
  - ZH: 地板反射: 关
  - [ ] OK    Fix: 

- **`floorReflectionsOn`**
  - EN: Floor Reflections: ON
  - ZH: 地板反射: 开
  - [ ] OK    Fix: 

- **`liveSourceOff`**
  - EN: Live Source Atten: OFF
  - ZH: 现场声源衰减: 关
  - [ ] OK    Fix: 

- **`liveSourceOn`**
  - EN: Live Source Atten: ON
  - ZH: 现场声源衰减: 开
  - [ ] OK    Fix: 

- **`minLatencyOff`**
  - EN: Minimal Latency: OFF
  - ZH: 最小延时: 关
  - [ ] OK    Fix: 

- **`minLatencyOn`**
  - EN: Minimal Latency: ON
  - ZH: 最小延时: 开
  - [ ] OK    Fix: 

## `outputs.units`

- **`meters`**
  - EN: m
  - ZH: m
  - [ ] OK    Fix: 

- **`ms`**
  - EN: ms
  - ZH: ms
  - [ ] OK    Fix: 

## `reverbs`

- **`noChannels`**
  - EN: No reverb channels configured.

Set the number of Reverb Channels in System Config.
  - ZH: 未配置混响通道。

请在系统配置中设置混响通道数量。
  - [ ] OK    Fix: 

## `reverbs.algorithm`

- **`crossoverHigh`**
  - EN: Crossover High:
  - ZH: 分频点高:
  - [ ] OK    Fix: 

- **`crossoverLow`**
  - EN: Crossover Low:
  - ZH: 分频点低:
  - [ ] OK    Fix: 

- **`decaySection`**
  - EN: Decay
  - ZH: 衰减
  - [ ] OK    Fix: 

- **`diffusion`**
  - EN: Diffusion:
  - ZH: 扩散:
  - [ ] OK    Fix: 

- **`fdn`**
  - EN: FDN
  - ZH: FDN
  - [ ] OK    Fix: 

- **`fdnSection`**
  - EN: FDN
  - ZH: FDN
  - [ ] OK    Fix: 

- **`ir`**
  - EN: IR
  - ZH: IR
  - [ ] OK    Fix: 

- **`irFile`**
  - EN: IR File:
  - ZH: IR文件:
  - [ ] OK    Fix: 

- **`irImport`**
  - EN: Import IR...
  - ZH: 导入IR...
  - [ ] OK    Fix: 

- **`irImportSuccess`**
  - EN: IR imported: {file}
  - ZH: IR已导入: {file}
  - [ ] OK    Fix: 

- **`irLength`**
  - EN: IR Length:
  - ZH: IR长度:
  - [ ] OK    Fix: 

- **`irNoProject`**
  - EN: Set a project folder first
  - ZH: 请先设置项目文件夹
  - [ ] OK    Fix: 

- **`irSection`**
  - EN: IR
  - ZH: IR
  - [ ] OK    Fix: 

- **`irTrim`**
  - EN: IR Trim:
  - ZH: IR修剪:
  - [ ] OK    Fix: 

- **`noFileLoaded`**
  - EN: No IR loaded
  - ZH: 未加载IR
  - [ ] OK    Fix: 

- **`perNodeOff`**
  - EN: Per-node IR OFF
  - ZH: 逐节点IR 关
  - [ ] OK    Fix: 

- **`perNodeOn`**
  - EN: Per-node IR ON
  - ZH: 逐节点IR 开
  - [ ] OK    Fix: 

- **`rt60`**
  - EN: RT60:
  - ZH: RT60:
  - [ ] OK    Fix: 

- **`rt60HighMult`**
  - EN: RT60 High ×:
  - ZH: RT60 高频 ×:
  - [ ] OK    Fix: 

- **`rt60LowMult`**
  - EN: RT60 Low ×:
  - ZH: RT60 低频 ×:
  - [ ] OK    Fix: 

- **`scale`**
  - EN: Scale:
  - ZH: 比例:
  - [ ] OK    Fix: 

- **`sdn`**
  - EN: SDN
  - ZH: SDN
  - [ ] OK    Fix: 

- **`sdnSection`**
  - EN: SDN
  - ZH: SDN
  - [ ] OK    Fix: 

- **`size`**
  - EN: Size:
  - ZH: 大小:
  - [ ] OK    Fix: 

- **`wetLevel`**
  - EN: Wet Level:
  - ZH: 湿信号电平:
  - [ ] OK    Fix: 

## `reverbs.buttons`

- **`editOnMap`**
  - EN: Edit on Map
  - ZH: 在地图上编辑
  - [ ] OK    Fix: 

- **`editOnMapOn`**
  - EN: Edit on Map ON
  - ZH: 在地图上编辑 ON
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - ZH: 导出
  - [ ] OK    Fix: 

- **`hiddenOnMap`**
  - EN: Reverbs Hidden on Map
  - ZH: 混响在地图上隐藏
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - ZH: 导入
  - [ ] OK    Fix: 

- **`mutePost`**
  - EN: Mute Post
  - ZH: Mute Post
  - [ ] OK    Fix: 

- **`mutePostOn`**
  - EN: Mute Post ON
  - ZH: 后级静音 开
  - [ ] OK    Fix: 

- **`mutePre`**
  - EN: Mute Pre
  - ZH: Mute Pre
  - [ ] OK    Fix: 

- **`mutePreOn`**
  - EN: Mute Pre ON
  - ZH: 前级静音 开
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Backup
  - ZH: 加载备份
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Reverb Config
  - ZH: 重新加载混响配置
  - [ ] OK    Fix: 

- **`soloReverbs`**
  - EN: Solo Reverbs
  - ZH: Solo Reverbs
  - [ ] OK    Fix: 

- **`soloReverbsOn`**
  - EN: Solo Reverbs ON
  - ZH: 独奏混响 开
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Reverb Config
  - ZH: 保存混响配置
  - [ ] OK    Fix: 

- **`visibleOnMap`**
  - EN: Reverbs Visible on Map
  - ZH: 混响在地图上可见
  - [ ] OK    Fix: 

## `reverbs.coordModes`

- **`xyz`**
  - EN: XYZ
  - ZH: XYZ
  - [ ] OK    Fix: 

## `reverbs.dialogs`

- **`export`**
  - EN: Export Reverb Configuration
  - ZH: 导出混响配置
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import Reverb Configuration
  - ZH: 导入混响配置
  - [ ] OK    Fix: 

## `reverbs.help`

- **`algoCrossoverHigh`**
  - EN: High crossover frequency for 3-band decay (1 - 10 kHz).
  - ZH: 3频段衰减的高分频点（1 - 10 kHz）。
  - [ ] OK    Fix: 

- **`algoCrossoverLow`**
  - EN: Low crossover frequency for 3-band decay (50 - 500 Hz).
  - ZH: 3频段衰减的低分频点（50 - 500 Hz）。
  - [ ] OK    Fix: 

- **`algoDiffusion`**
  - EN: Diffusion amount controlling echo density (0 - 100%).
  - ZH: 控制回声密度的扩散量（0 - 100%）。
  - [ ] OK    Fix: 

- **`algoFDN`**
  - EN: Select FDN (Feedback Delay Network) reverb algorithm.
  - ZH: 选择FDN（Feedback Delay Network）混响算法。
  - [ ] OK    Fix: 

- **`algoFDNSize`**
  - EN: FDN delay line size multiplier (0.5 - 2.0x).
  - ZH: FDN延迟线大小乘数（0.5 - 2.0x）。
  - [ ] OK    Fix: 

- **`algoIR`**
  - EN: Select IR (Impulse Response convolution) reverb algorithm.
  - ZH: 选择IR（脉冲响应卷积）混响算法。
  - [ ] OK    Fix: 

- **`algoIRFile`**
  - EN: Select or import an impulse response file for convolution.
  - ZH: 选择或导入用于卷积的脉冲响应文件。
  - [ ] OK    Fix: 

- **`algoIRLength`**
  - EN: Maximum impulse response length (0.1 - 6.0 seconds).
  - ZH: 脉冲响应最大长度（0.1 - 6.0秒）。
  - [ ] OK    Fix: 

- **`algoIRTrim`**
  - EN: Trim the start of the impulse response (0 - 100 ms).
  - ZH: 修剪脉冲响应的开头（0 - 100 ms）。
  - [ ] OK    Fix: 

- **`algoPerNode`**
  - EN: Use a separate IR for each reverb node, or share one IR.
  - ZH: 为每个混响节点使用单独的IR，或共享一个IR。
  - [ ] OK    Fix: 

- **`algoRT60`**
  - EN: Reverb decay time RT60 (0.2 - 8.0 seconds).
  - ZH: 混响衰减时间RT60（0.2 - 8.0秒）。
  - [ ] OK    Fix: 

- **`algoRT60HighMult`**
  - EN: High frequency RT60 multiplier (0.1 - 9.0x).
  - ZH: 高频RT60乘数（0.1 - 9.0x）。
  - [ ] OK    Fix: 

- **`algoRT60LowMult`**
  - EN: Low frequency RT60 multiplier (0.1 - 9.0x).
  - ZH: 低频RT60乘数（0.1 - 9.0x）。
  - [ ] OK    Fix: 

- **`algoSDN`**
  - EN: Select SDN (Scattering Delay Network) reverb algorithm.
  - ZH: 选择SDN（Scattering Delay Network）混响算法。
  - [ ] OK    Fix: 

- **`algoSDNScale`**
  - EN: SDN inter-node delay scale factor (0.5 - 4.0x).
  - ZH: SDN节点间延迟比例因子（0.5 - 4.0x）。
  - [ ] OK    Fix: 

- **`algoWetLevel`**
  - EN: Wet/dry mix level for reverb output (-60 to +12 dB).
  - ZH: 混响输出的湿/干混合电平（-60至+12 dB）。
  - [ ] OK    Fix: 

- **`angleOff`**
  - EN: Angle at which no amplification occurs (0-179 degrees).
  - ZH: 没有放大的角度（0-179度）。
  - [ ] OK    Fix: 

- **`angleOn`**
  - EN: Angle at which amplification starts (1-180 degrees).
  - ZH: 放大开始的角度（1-180度）。
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Reverb channel attenuation (-92 to 0 dB).
  - ZH: 混响通道衰减（-92至0 dB）。
  - [ ] OK    Fix: 

- **`channelSelector`**
  - EN: Reverb Channel Number and Selection.
  - ZH: 混响通道编号和选择。
  - [ ] OK    Fix: 

- **`commonAtten`**
  - EN: Common attenuation percentage (0-100%).
  - ZH: 公共衰减百分比（0-100%）。
  - [ ] OK    Fix: 

- **`coordMode`**
  - EN: Coordinate display mode: Cartesian (X/Y/Z), Cylindrical (radius/azimuth/height), or Spherical (radius/azimuth/elevation).
  - ZH: 坐标显示模式：笛卡尔坐标（X/Y/Z）、圆柱坐标（半径/方位角/高度）或球坐标（半径/方位角/仰角）。
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Reverb delay/latency compensation (-100 to +100 ms).
  - ZH: 混响延迟/延时补偿（-100至+100 ms）。
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Distance attenuation for reverb return (-6.0 to 0.0 dB/m).
  - ZH: 混响返回的距离衰减（-6.0至0.0 dB/m）。
  - [ ] OK    Fix: 

- **`distanceAttenEnable`**
  - EN: Distance attenuation percentage (0-200%).
  - ZH: 距离衰减百分比（0-200%）。
  - [ ] OK    Fix: 

- **`eqBandToggle`**
  - EN: Toggle pre-EQ Band {band} on/off. When off the band is bypassed.
  - ZH: 开启/关闭前置EQ频段{band}。
  - [ ] OK    Fix: 

- **`eqEnable`**
  - EN: Enable or disable EQ processing for this reverb.
  - ZH: 启用或禁用此混响的EQ处理。
  - [ ] OK    Fix: 

- **`eqFlatten`**
  - EN: Long press to reset all pre-EQ bands to their default values.
  - ZH: 长按重置所有前置EQ频段为默认值。
  - [ ] OK    Fix: 

- **`eqFreq`**
  - EN: Pre-EQ Band {band} frequency (20 Hz - 20 kHz).
  - ZH: 前置EQ频段{band}频率 (20 Hz - 20 kHz)。
  - [ ] OK    Fix: 

- **`eqGain`**
  - EN: Pre-EQ Band {band} gain (-24 to +24 dB).
  - ZH: 前置EQ频段{band}增益 (-24至+24 dB)。
  - [ ] OK    Fix: 

- **`eqQ`**
  - EN: Pre-EQ Band {band} Q factor (0.1 - 20.0).
  - ZH: 前置EQ频段{band} Q因子 (0.1 - 20.0)。
  - [ ] OK    Fix: 

- **`eqResetBand`**
  - EN: Long press to reset pre-EQ Band {band} to its default values.
  - ZH: 长按重置前置EQ频段{band}为默认值。
  - [ ] OK    Fix: 

- **`eqShape`**
  - EN: Pre-EQ Band {band} filter shape.
  - ZH: 前置EQ频段{band}滤波器形状。
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Reverb Configuration to file (with file explorer window).
  - ZH: 将混响配置导出到文件（使用文件浏览器）。
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: High frequency loss per meter (-6.0 to 0.0 dB/m).
  - ZH: 每米高频损耗（-6.0至0.0 dB/m）。
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Reverb Configuration from file (with file explorer window).
  - ZH: 从文件导入混响配置（使用文件浏览器）。
  - [ ] OK    Fix: 

- **`liveSourceTooltip`**
  - EN: Enable Live Source attenuation tamer. Reduces level fluctuations from sources close to the array.
  - ZH: 启用现场声源衰减调节器。减少靠近阵列的声源的电平波动。
  - [ ] OK    Fix: 

- **`mapVisibility`**
  - EN: Make Visible or Hide All Reverb Channels on the Map.
  - ZH: 在地图上显示或隐藏所有混响通道。
  - [ ] OK    Fix: 

- **`miniLatencyTooltip`**
  - EN: Enable minimal latency mode for this reverb channel. Reduces processing delay at the cost of higher CPU usage.
  - ZH: 启用此混响通道的最小延迟模式。以更高 CPU 使用为代价减少处理延迟。
  - [ ] OK    Fix: 

- **`muteButton`**
  - EN: Toggle mute for this output channel's reverb return.
  - ZH: 切换此输出通道的混响返回静音。
  - [ ] OK    Fix: 

- **`muteMacros`**
  - EN: Quick mute operations for output channels.
  - ZH: 输出通道的快速静音操作。
  - [ ] OK    Fix: 

- **`nameEditor`**
  - EN: Displayed Reverb Channel Name (editable).
  - ZH: 显示的混响通道名称（可编辑）。
  - [ ] OK    Fix: 

- **`orientation`**
  - EN: Reverb orientation angle (-179 to +180 degrees).
  - ZH: 混响朝向角度（-179至+180度）。
  - [ ] OK    Fix: 

- **`pitch`**
  - EN: Vertical orientation of reverb (-90 to +90 degrees).
  - ZH: 混响的垂直朝向（-90至+90度）。
  - [ ] OK    Fix: 

- **`position1`**
  - EN: Reverb virtual source {name} ({unit}).
  - ZH: 混响虚拟声源{name}（{unit}）。
  - [ ] OK    Fix: 

- **`position2`**
  - EN: Reverb virtual source {name} ({unit}).
  - ZH: 混响虚拟声源{name}（{unit}）。
  - [ ] OK    Fix: 

- **`position3`**
  - EN: Reverb virtual source {name} ({unit}).
  - ZH: 混响虚拟声源{name}（{unit}）。
  - [ ] OK    Fix: 

- **`postEqBandToggle`**
  - EN: Toggle post-EQ Band {band} on/off. When off the band is bypassed.
  - ZH: 开启/关闭后置EQ频段{band}。
  - [ ] OK    Fix: 

- **`postEqEnable`**
  - EN: Enable or disable post-processing EQ.
  - ZH: 启用或禁用后处理EQ。
  - [ ] OK    Fix: 

- **`postEqFlatten`**
  - EN: Long press to reset all post-EQ bands to their default values.
  - ZH: 长按重置所有后置EQ频段为默认值。
  - [ ] OK    Fix: 

- **`postEqFreq`**
  - EN: Post-EQ Band {band} frequency (20 Hz - 20 kHz).
  - ZH: 后处理EQ频段{band}频率（20 Hz - 20 kHz）。
  - [ ] OK    Fix: 

- **`postEqGain`**
  - EN: Post-EQ Band {band} gain (-24 to +24 dB).
  - ZH: 后处理EQ频段{band}增益（-24至+24 dB）。
  - [ ] OK    Fix: 

- **`postEqQ`**
  - EN: Post-EQ Band {band} Q factor (0.1 - 20.0).
  - ZH: 后处理EQ频段{band} Q值（0.1 - 20.0）。
  - [ ] OK    Fix: 

- **`postEqResetBand`**
  - EN: Long press to reset post-EQ Band {band} to its default values.
  - ZH: 长按重置后置EQ频段{band}为默认值。
  - [ ] OK    Fix: 

- **`postEqShape`**
  - EN: Post-EQ Band {band} filter shape.
  - ZH: 后处理EQ频段{band}滤波器形状。
  - [ ] OK    Fix: 

- **`postExpAttack`**
  - EN: Post-expander attack time (0.1 - 50 ms).
  - ZH: 后扩展器起音时间（0.1 - 50 ms）。
  - [ ] OK    Fix: 

- **`postExpBypass`**
  - EN: Bypass or enable the post-expander on reverb returns.
  - ZH: 旁通或启用混响返回上的后扩展器。
  - [ ] OK    Fix: 

- **`postExpRatio`**
  - EN: Post-expander ratio (1:1 to 1:8).
  - ZH: 后扩展器比率（1:1至1:8）。
  - [ ] OK    Fix: 

- **`postExpRelease`**
  - EN: Post-expander release time (50 - 2000 ms).
  - ZH: 后扩展器释放时间（50 - 2000 ms）。
  - [ ] OK    Fix: 

- **`postExpThreshold`**
  - EN: Post-expander threshold (-80 to -10 dB).
  - ZH: 后扩展器阈值（-80至-10 dB）。
  - [ ] OK    Fix: 

- **`preCompAttack`**
  - EN: Pre-compressor attack time (0.1 - 100 ms).
  - ZH: 预压缩器起音时间（0.1 - 100 ms）。
  - [ ] OK    Fix: 

- **`preCompBypass`**
  - EN: Bypass or enable the pre-compressor on reverb feeds.
  - ZH: 旁通或启用混响发送上的预压缩器。
  - [ ] OK    Fix: 

- **`preCompRatio`**
  - EN: Pre-compressor ratio (1:1 to 20:1).
  - ZH: 预压缩器比率（1:1至20:1）。
  - [ ] OK    Fix: 

- **`preCompRelease`**
  - EN: Pre-compressor release time (10 - 1000 ms).
  - ZH: 预压缩器释放时间（10 - 1000 ms）。
  - [ ] OK    Fix: 

- **`preCompThreshold`**
  - EN: Pre-compressor threshold (-60 to 0 dB).
  - ZH: 预压缩器阈值（-60至0 dB）。
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Reverb Configuration from backup file.
  - ZH: 从备份文件重新加载混响配置。
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Reverb Configuration from file.
  - ZH: 从文件重新加载混响配置。
  - [ ] OK    Fix: 

- **`returnOffset1`**
  - EN: Reverb return offset {name} ({unit}).
  - ZH: 混响返回偏移{name}（{unit}）。
  - [ ] OK    Fix: 

- **`returnOffset2`**
  - EN: Reverb return offset {name} ({unit}).
  - ZH: 混响返回偏移{name}（{unit}）。
  - [ ] OK    Fix: 

- **`returnOffset3`**
  - EN: Reverb return offset {name} ({unit}).
  - ZH: 混响返回偏移{name}（{unit}）。
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Reverb Configuration to file (with backup).
  - ZH: 将混响配置存储到文件（含备份）。
  - [ ] OK    Fix: 

## `reverbs.labels`

- **`angleOff`**
  - EN: Angle Off:
  - ZH: 关闭角度:
  - [ ] OK    Fix: 

- **`angleOn`**
  - EN: Angle On:
  - ZH: 开启角度:
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Attenuation:
  - ZH: 衰减:
  - [ ] OK    Fix: 

- **`commonAtten`**
  - EN: Common Atten:
  - ZH: 公共衰减:
  - [ ] OK    Fix: 

- **`coordinates`**
  - EN: Coord:
  - ZH: 坐标:
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Delay/Latency:
  - ZH: 延迟/延时:
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Distance Atten:
  - ZH: 距离衰减:
  - [ ] OK    Fix: 

- **`distanceAttenPercent`**
  - EN: Distance Atten %:
  - ZH: 距离衰减%:
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: HF Damping:
  - ZH: 高频衰减:
  - [ ] OK    Fix: 

- **`muteMacro`**
  - EN: Mute Macro:
  - ZH: 静音宏:
  - [ ] OK    Fix: 

- **`name`**
  - EN: Name:
  - ZH: 名称:
  - [ ] OK    Fix: 

- **`orientation`**
  - EN: Orientation:
  - ZH: 朝向:
  - [ ] OK    Fix: 

- **`outputMutes`**
  - EN: Output Mutes:
  - ZH: 输出静音:
  - [ ] OK    Fix: 

- **`pitch`**
  - EN: Pitch:
  - ZH: 俯仰:
  - [ ] OK    Fix: 

- **`positionX`**
  - EN: Position X:
  - ZH: 位置X:
  - [ ] OK    Fix: 

- **`positionY`**
  - EN: Position Y:
  - ZH: 位置Y:
  - [ ] OK    Fix: 

- **`positionZ`**
  - EN: Position Z:
  - ZH: 位置Z:
  - [ ] OK    Fix: 

- **`returnOffsetX`**
  - EN: Return Offset X:
  - ZH: 返回偏移X:
  - [ ] OK    Fix: 

- **`returnOffsetY`**
  - EN: Return Offset Y:
  - ZH: 返回偏移Y:
  - [ ] OK    Fix: 

- **`returnOffsetZ`**
  - EN: Return Offset Z:
  - ZH: 返回偏移Z:
  - [ ] OK    Fix: 

## `reverbs.messages`

- **`backupLoaded`**
  - EN: Reverb configuration loaded from backup.
  - ZH: 已从备份加载混响配置。
  - [ ] OK    Fix: 

- **`configExported`**
  - EN: Reverb configuration exported.
  - ZH: 混响配置已导出。
  - [ ] OK    Fix: 

- **`configImported`**
  - EN: Reverb configuration imported.
  - ZH: 混响配置已导入。
  - [ ] OK    Fix: 

- **`configLoaded`**
  - EN: Reverb configuration loaded.
  - ZH: 混响配置已加载。
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Reverb configuration saved.
  - ZH: 混响配置已保存。
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - ZH: 错误: {error}
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - ZH: 请先在系统配置中选择项目文件夹。
  - [ ] OK    Fix: 

## `reverbs.muteMacros`

- **`invertMutes`**
  - EN: INVERT MUTES
  - ZH: 反转静音
  - [ ] OK    Fix: 

- **`muteAll`**
  - EN: MUTE ALL
  - ZH: 全部静音
  - [ ] OK    Fix: 

- **`muteArray`**
  - EN: MUTE ARRAY
  - ZH: 静音阵列
  - [ ] OK    Fix: 

- **`muteEven`**
  - EN: MUTE EVEN
  - ZH: 静音偶数
  - [ ] OK    Fix: 

- **`muteOdd`**
  - EN: MUTE ODD
  - ZH: 静音奇数
  - [ ] OK    Fix: 

- **`select`**
  - EN: Mute Macro Select
  - ZH: 选择静音宏
  - [ ] OK    Fix: 

- **`unmuteAll`**
  - EN: UNMUTE ALL
  - ZH: 全部解除静音
  - [ ] OK    Fix: 

- **`unmuteArray`**
  - EN: UNMUTE ARRAY
  - ZH: 解除阵列静音
  - [ ] OK    Fix: 

## `reverbs.postProcessing`

- **`attack`**
  - EN: Attack:
  - ZH: 起音:
  - [ ] OK    Fix: 

- **`expander`**
  - EN: Expander
  - ZH: 扩展器
  - [ ] OK    Fix: 

- **`expanderOff`**
  - EN: Expander OFF
  - ZH: 扩展器 关
  - [ ] OK    Fix: 

- **`expanderOn`**
  - EN: Expander ON
  - ZH: 扩展器 开
  - [ ] OK    Fix: 

- **`ratio`**
  - EN: Ratio:
  - ZH: 比率:
  - [ ] OK    Fix: 

- **`release`**
  - EN: Release:
  - ZH: 释放:
  - [ ] OK    Fix: 

- **`threshold`**
  - EN: Threshold:
  - ZH: 阈值:
  - [ ] OK    Fix: 

## `reverbs.preProcessing`

- **`attack`**
  - EN: Attack:
  - ZH: 起音:
  - [ ] OK    Fix: 

- **`compressor`**
  - EN: Compressor
  - ZH: 压缩器
  - [ ] OK    Fix: 

- **`compressorOff`**
  - EN: Compressor OFF
  - ZH: 压缩器 关
  - [ ] OK    Fix: 

- **`compressorOn`**
  - EN: Compressor ON
  - ZH: 压缩器 开
  - [ ] OK    Fix: 

- **`ratio`**
  - EN: Ratio:
  - ZH: 比率:
  - [ ] OK    Fix: 

- **`release`**
  - EN: Release:
  - ZH: 释放:
  - [ ] OK    Fix: 

- **`threshold`**
  - EN: Threshold:
  - ZH: 阈值:
  - [ ] OK    Fix: 

## `reverbs.sections`

- **`reverbFeed`**
  - EN: Reverb Feed
  - ZH: 混响发送
  - [ ] OK    Fix: 

- **`reverbReturn`**
  - EN: Reverb Return
  - ZH: 混响返回
  - [ ] OK    Fix: 

## `reverbs.tabs`

- **`algorithm`**
  - EN: Algorithm
  - ZH: 算法
  - [ ] OK    Fix: 

- **`channelParams`**
  - EN: Channel Parameters
  - ZH: 通道参数
  - [ ] OK    Fix: 

- **`postProcessing`**
  - EN: Post-Processing
  - ZH: 后处理
  - [ ] OK    Fix: 

- **`preProcessing`**
  - EN: Pre-Processing
  - ZH: 前处理
  - [ ] OK    Fix: 

## `reverbs.toggles`

- **`liveSourceOff`**
  - EN: Live Source Atten OFF
  - ZH: 现场声源衰减 关
  - [ ] OK    Fix: 

- **`liveSourceOn`**
  - EN: Live Source Atten ON
  - ZH: 现场声源衰减 开
  - [ ] OK    Fix: 

- **`minLatencyOff`**
  - EN: Minimal Latency OFF
  - ZH: 最小延时 关
  - [ ] OK    Fix: 

- **`minLatencyOn`**
  - EN: Minimal Latency ON
  - ZH: 最小延时 开
  - [ ] OK    Fix: 

## `sampler.buttons`

- **`copy`**
  - EN: Copy
  - ZH: 复制
  - [ ] OK    Fix: 

- **`copyCell`**
  - EN: Copy Cell
  - ZH: 复制单元
  - [ ] OK    Fix: 

- **`copySet`**
  - EN: Copy Set
  - ZH: 复制集合
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - ZH: 导出
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - ZH: 导入
  - [ ] OK    Fix: 

- **`paste`**
  - EN: Paste
  - ZH: 粘贴
  - [ ] OK    Fix: 

- **`pasteCell`**
  - EN: Paste Cell
  - ZH: 粘贴单元
  - [ ] OK    Fix: 

- **`pasteSet`**
  - EN: Paste Set
  - ZH: 粘贴集合
  - [ ] OK    Fix: 

## `sampler.cell`

- **`attenuation`**
  - EN: Attenuation (dB)
  - ZH: 衰减 (dB)
  - [ ] OK    Fix: 

- **`clear`**
  - EN: Clear
  - ZH: 清除
  - [ ] OK    Fix: 

- **`inOut`**
  - EN: In/Out (ms)
  - ZH: 淡入/淡出 (ms)
  - [ ] OK    Fix: 

- **`load`**
  - EN: Load
  - ZH: 加载
  - [ ] OK    Fix: 

- **`loadTitle`**
  - EN: Load Sample
  - ZH: 加载采样
  - [ ] OK    Fix: 

- **`offset`**
  - EN: Offset (m)
  - ZH: 偏移 (m)
  - [ ] OK    Fix: 

- **`preview`**
  - EN: Preview
  - ZH: 试听
  - [ ] OK    Fix: 

- **`previewStop`**
  - EN: Stop
  - ZH: 停止
  - [ ] OK    Fix: 

## `sampler.grid`

- **`help`**
  - EN: Click = select
Shift = multi
Ctrl = set toggle
DblClick = load
  - ZH: 单击=选择 | Shift=多选 | Ctrl=切换集合 | 双击=加载
  - [ ] OK    Fix: 

## `sampler.labels`

- **`lightpadZone`**
  - EN: Lightpad Zone
  - ZH: Lightpad 区域
  - [ ] OK    Fix: 

- **`selectZone`**
  - EN: Select Zone
  - ZH: 选择区域
  - [ ] OK    Fix: 

## `sampler.lightpadZone`

- **`none`**
  - EN: None
  - ZH: 无
  - [ ] OK    Fix: 

## `sampler.press`

- **`height`**
  - EN: Height
  - ZH: 高度
  - [ ] OK    Fix: 

- **`hf`**
  - EN: HF Shelf
  - ZH: HF Shelf
  - [ ] OK    Fix: 

- **`level`**
  - EN: Level
  - ZH: 电平
  - [ ] OK    Fix: 

- **`xy`**
  - EN: Position XY
  - ZH: Position XY
  - [ ] OK    Fix: 

## `sampler.remote`

- **`gridLayout`**
  - EN: Grid Layout
  - ZH: 网格布局
  - [ ] OK    Fix: 

## `sampler.section`

- **`actions`**
  - EN: ACTIONS
  - ZH: 操作
  - [ ] OK    Fix: 

- **`cell`**
  - EN: CELL PROPERTIES
  - ZH: 单元属性
  - [ ] OK    Fix: 

- **`pressure`**
  - EN: PRESSURE MAPPINGS
  - ZH: 压力映射
  - [ ] OK    Fix: 

- **`set`**
  - EN: SET MANAGEMENT
  - ZH: 集合管理
  - [ ] OK    Fix: 

## `sampler.set`

- **`copy`**
  - EN: (copy)
  - ZH: (副本)
  - [ ] OK    Fix: 

- **`default`**
  - EN: Set
  - ZH: 集合
  - [ ] OK    Fix: 

- **`level`**
  - EN: Level (dB)
  - ZH: 电平 (dB)
  - [ ] OK    Fix: 

- **`pos`**
  - EN: Position (m)
  - ZH: 位置 (m)
  - [ ] OK    Fix: 

- **`rename`**
  - EN: Rename
  - ZH: 重命名
  - [ ] OK    Fix: 

- **`roundRobin`**
  - EN: Round-Robin
  - ZH: 轮询
  - [ ] OK    Fix: 

- **`sequential`**
  - EN: Sequential
  - ZH: 顺序
  - [ ] OK    Fix: 

## `sampler.tooltips`

- **`addSet`**
  - EN: Create a new set. If cells are selected they will be assigned to it.
  - ZH: 创建新集合。如果选择了单元格，它们将被分配给它。
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Cell attenuation in dB (0 = no attenuation, -60 = silent)
  - ZH: 单元格衰减 (dB) (0 = 无衰减，-60 = 静音)
  - [ ] OK    Fix: 

- **`clear`**
  - EN: Remove the sample from the selected cell (long press)
  - ZH: 从所选单元格中删除样本 (长按)
  - [ ] OK    Fix: 

- **`copy`**
  - EN: Copy selected cell or active set to clipboard
  - ZH: 将所选单元格或活动集合复制到剪贴板
  - [ ] OK    Fix: 

- **`deleteSet`**
  - EN: Delete the active set (long press)
  - ZH: 删除活动集合 (长按)
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export sampler configuration to file
  - ZH: 将采样器配置导出到文件
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import sampler configuration from file
  - ZH: 从文件导入采样器配置
  - [ ] OK    Fix: 

- **`inOut`**
  - EN: Set the In/Out time range in milliseconds. Drag between thumbs to shift both.
  - ZH: 以毫秒设置入/出时间范围。在拇指之间拖动以同时移动两者。
  - [ ] OK    Fix: 

- **`load`**
  - EN: Load a sample file into the selected cell
  - ZH: 将样本文件加载到所选单元格
  - [ ] OK    Fix: 

- **`offset`**
  - EN: Offset position in meters (X, Y, Z) relative to the set position
  - ZH: 相对于集合位置的位置偏移 (米) (X、Y、Z)
  - [ ] OK    Fix: 

- **`paste`**
  - EN: Paste clipboard data to selected cell or active set
  - ZH: 将剪贴板数据粘贴到所选单元格或活动集合
  - [ ] OK    Fix: 

- **`playMode`**
  - EN: Toggle between Sequential and Round-Robin playback
  - ZH: 在顺序和轮询播放之间切换
  - [ ] OK    Fix: 

- **`pressCurve`**
  - EN: Pressure response curve (0 = concave, 0.5 = linear, 1 = convex)
  - ZH: 压力响应曲线 (0 = 凹，0.5 = 线性，1 = 凸)
  - [ ] OK    Fix: 

- **`pressDir`**
  - EN: Toggle pressure direction: + = more pressure increases, - = decreases
  - ZH: 切换压力方向: + = 更大压力增加，- = 减小
  - [ ] OK    Fix: 

- **`pressHeight`**
  - EN: Map finger pressure to vertical position (Z)
  - ZH: 将手指压力映射到垂直位置 (Z)
  - [ ] OK    Fix: 

- **`pressHF`**
  - EN: Map finger pressure to high-frequency shelf attenuation
  - ZH: 将手指压力映射到高搁架衰减
  - [ ] OK    Fix: 

- **`pressLevel`**
  - EN: Map finger pressure to output level
  - ZH: 将手指压力映射到输出电平
  - [ ] OK    Fix: 

- **`pressXY`**
  - EN: Map finger pressure to XY position movement
  - ZH: 将手指压力映射到 XY 位置移动
  - [ ] OK    Fix: 

- **`pressXYScale`**
  - EN: Sensitivity: how far the source moves per pressure step
  - ZH: 灵敏度: 每个压力步骤源移动的距离
  - [ ] OK    Fix: 

- **`preview`**
  - EN: Preview the loaded sample
  - ZH: 预听已加载的样本
  - [ ] OK    Fix: 

- **`renameSet`**
  - EN: Rename the active set
  - ZH: 重命名活动集合
  - [ ] OK    Fix: 

- **`setLevel`**
  - EN: Set output level in dB (0 = unity, -60 = silent)
  - ZH: 以 dB 设置输出电平 (0 = 单位，-60 = 静音)
  - [ ] OK    Fix: 

- **`setPos`**
  - EN: Set base position in meters (X, Y, Z)
  - ZH: 以米设置基础位置 (X、Y、Z)
  - [ ] OK    Fix: 

## `sampler.zone`

- **`remotePad`**
  - EN: Pad {num}
  - ZH: Pad {num}
  - [ ] OK    Fix: 

- **`selectRemotePad`**
  - EN: Select Remote Pad
  - ZH: 选择遥控器板
  - [ ] OK    Fix: 

## `setAllInputs`

- **`warning`**
  - EN: Changes will apply to ALL inputs
  - ZH: 更改将应用于所有输入
  - [ ] OK    Fix: 

- **`windowTitle`**
  - EN: Set All Inputs
  - ZH: 设置所有输入
  - [ ] OK    Fix: 

## `setAllInputs.buttons`

- **`all1d`**
  - EN: All 1/d
  - ZH: 全部1/d
  - [ ] OK    Fix: 

- **`allLog`**
  - EN: All Log
  - ZH: 全部对数
  - [ ] OK    Fix: 

- **`closeWindow`**
  - EN: CLOSE WINDOW
  - ZH: 关闭窗口
  - [ ] OK    Fix: 

- **`flipXyzOff`**
  - EN: Flip XYZ > OFF
  - ZH: 翻转XYZ > 关
  - [ ] OK    Fix: 

- **`resetDirectivity`**
  - EN: Reset directivity
  - ZH: 重置指向性
  - [ ] OK    Fix: 

- **`turnOffJitterLfo`**
  - EN: Turn OFF jitter & LFO
  - ZH: 关闭抖动和LFO
  - [ ] OK    Fix: 

- **`turnOffLiveSource`**
  - EN: Turn OFF Live source atten.
  - ZH: 关闭现场声源衰减
  - [ ] OK    Fix: 

## `setAllInputs.coordinateModes`

- **`rThetaPhi`**
  - EN: r θ φ
  - ZH: r θ φ
  - [ ] OK    Fix: 

- **`rThetaZ`**
  - EN: r θ Z
  - ZH: r θ Z
  - [ ] OK    Fix: 

- **`xyz`**
  - EN: XYZ
  - ZH: XYZ
  - [ ] OK    Fix: 

## `setAllInputs.labels`

- **`common`**
  - EN: common
  - ZH: 公共
  - [ ] OK    Fix: 

- **`constraintPositions`**
  - EN: Constraint positions:
  - ZH: 约束位置:
  - [ ] OK    Fix: 

- **`coordinateMode`**
  - EN: Coordinate mode:
  - ZH: 坐标模式:
  - [ ] OK    Fix: 

- **`dbPerMeter`**
  - EN: dB/m
  - ZH: dB/m
  - [ ] OK    Fix: 

- **`distanceAttenuation`**
  - EN: Distance attenuation
  - ZH: 距离衰减
  - [ ] OK    Fix: 

- **`floorReflections`**
  - EN: Floor Reflections:
  - ZH: 地板反射:
  - [ ] OK    Fix: 

- **`fringe`**
  - EN: Fringe:
  - ZH: 边缘:
  - [ ] OK    Fix: 

- **`heightFactor`**
  - EN: Height factor:
  - ZH: 高度因子:
  - [ ] OK    Fix: 

- **`minimalLatency`**
  - EN: Minimal Latency:
  - ZH: 最小延时:
  - [ ] OK    Fix: 

- **`muteMacros`**
  - EN: Mute macros:
  - ZH: 静音宏:
  - [ ] OK    Fix: 

- **`ratio`**
  - EN: ratio
  - ZH: 比率
  - [ ] OK    Fix: 

- **`sidelines`**
  - EN: Sidelines:
  - ZH: 边线:
  - [ ] OK    Fix: 

## `snapshot`

- **`qlabExportDone`**
  - EN: QLab export complete: {count} cues created
  - ZH: QLab 导出完成：已创建 {count} 个 Cue
  - [ ] OK    Fix: 

- **`qlabExportStarted`**
  - EN: Writing {count} cues to QLab...
  - ZH: 正在向 QLab 写入 {count} 个 Cue...
  - [ ] OK    Fix: 

- **`qlabGroupName`**
  - EN: Snapshot "{name}"
  - ZH: 快照 "{name}"
  - [ ] OK    Fix: 

- **`qlabMemoText`**
  - EN: Run either of the following cues to recall or update this snapshot
  - ZH: 运行以下任一 Cue 来加载或更新此快照
  - [ ] OK    Fix: 

- **`qlabNoTarget`**
  - EN: No QLab target configured
  - ZH: 未配置QLab目标
  - [ ] OK    Fix: 

- **`qlabReloadName`**
  - EN: Reload "{name}"
  - ZH: 加载 "{name}"
  - [ ] OK    Fix: 

- **`qlabUpdateName`**
  - EN: Update "{name}"
  - ZH: 更新 "{name}"
  - [ ] OK    Fix: 

## `snapshotScope`

- **`all`**
  - EN: ALL
  - ZH: 全部
  - [ ] OK    Fix: 

- **`applyScope`**
  - EN: Apply scope:
  - ZH: 应用范围:
  - [ ] OK    Fix: 

- **`autoPreselectModified`**
  - EN: Auto-preselect modified parameters
  - ZH: 自动预选已修改的参数
  - [ ] OK    Fix: 

- **`title`**
  - EN: Snapshot Scope: {name}
  - ZH: 快照范围: {name}
  - [ ] OK    Fix: 

- **`whenRecalling`**
  - EN: When Recalling
  - ZH: 调用时
  - [ ] OK    Fix: 

- **`whenSaving`**
  - EN: When Saving
  - ZH: 保存时
  - [ ] OK    Fix: 

- **`windowTitle`**
  - EN: Input Snapshot Scope
  - ZH: 输入快照范围
  - [ ] OK    Fix: 

- **`writeSnapshotLoadCue`**
  - EN: Write Snapshot Load Cue to QLab
  - ZH: 将快照加载提示写入 QLab
  - [ ] OK    Fix: 

- **`writeSnapshotLoadCueTooltip`**
  - EN: Also create a QLab cue to load this snapshot via OSC
  - ZH: 同时创建一个通过 OSC 加载此快照的 QLab 提示
  - [ ] OK    Fix: 

- **`writeToQLab`**
  - EN: Write to QLab
  - ZH: 写入 QLab
  - [ ] OK    Fix: 

- **`writeToQLabTooltip`**
  - EN: Export scope to QLab instead of saving to file
  - ZH: 将范围导出到 QLab 而非保存到文件
  - [ ] OK    Fix: 

## `snapshotScope.buttons`

- **`cancel`**
  - EN: Cancel
  - ZH: 取消
  - [ ] OK    Fix: 

- **`clearChanges`**
  - EN: Clear Changes
  - ZH: 清除更改
  - [ ] OK    Fix: 

- **`ok`**
  - EN: OK
  - ZH: OK
  - [ ] OK    Fix: 

- **`selectModified`**
  - EN: Select Modified
  - ZH: 选择已修改的
  - [ ] OK    Fix: 

## `snapshotScope.sections`

- **`attenuation`**
  - EN: Attenuation
  - ZH: 衰减
  - [ ] OK    Fix: 

- **`automOtion`**
  - EN: AutomOtion
  - ZH: AutomOtion (自动移动)
  - [ ] OK    Fix: 

- **`directivity`**
  - EN: Directivity
  - ZH: 指向性
  - [ ] OK    Fix: 

- **`hackoustics`**
  - EN: Hackoustics
  - ZH: Hackoustics (虚拟声学)
  - [ ] OK    Fix: 

- **`input`**
  - EN: Input
  - ZH: 输入
  - [ ] OK    Fix: 

- **`lfo`**
  - EN: LFO
  - ZH: LFO
  - [ ] OK    Fix: 

- **`liveSource`**
  - EN: Live Source
  - ZH: 现场声源
  - [ ] OK    Fix: 

- **`mutes`**
  - EN: Mutes
  - ZH: 静音
  - [ ] OK    Fix: 

- **`position`**
  - EN: Position
  - ZH: 位置
  - [ ] OK    Fix: 

## `statusBar`

- **`displayLabel`**
  - EN: Display:
  - ZH: 显示:
  - [ ] OK    Fix: 

- **`helpMode`**
  - EN: Help
  - ZH: 帮助
  - [ ] OK    Fix: 

- **`oscMode`**
  - EN: OSC
  - ZH: OSC
  - [ ] OK    Fix: 

## `systemConfig.algorithms`

- **`inputBuffer`**
  - EN: InputBuffer (read-time delays)
  - ZH: InputBuffer (读取时延迟)
  - [ ] OK    Fix: 

- **`outputBuffer`**
  - EN: OutputBuffer (write-time delays)
  - ZH: OutputBuffer (写入时延迟)
  - [ ] OK    Fix: 

## `systemConfig.binauralOutput`

- **`select`**
  - EN: Select...
  - ZH: 选择...
  - [ ] OK    Fix: 

## `systemConfig.buttons`

- **`audioPatch`**
  - EN: Audio Interface and Patching Window
  - ZH: 音频接口和路由窗口
  - [ ] OK    Fix: 

- **`binauralOff`**
  - EN: Binaural: OFF
  - ZH: 双耳: 关
  - [ ] OK    Fix: 

- **`binauralOn`**
  - EN: Binaural: ON
  - ZH: 双耳: 开
  - [ ] OK    Fix: 

- **`clearSolo`**
  - EN: Clear Solo
  - ZH: 清除独听
  - [ ] OK    Fix: 

- **`copySystemInfo`**
  - EN: Copy System Info
  - ZH: 复制系统信息
  - [ ] OK    Fix: 

- **`diagnosticsCollapsed`**
  - EN: Diagnostics  [+]
  - ZH: 诊断  [+]
  - [ ] OK    Fix: 

- **`diagnosticsExpanded`**
  - EN: Diagnostics  [-]
  - ZH: 诊断  [-]
  - [ ] OK    Fix: 

- **`exportLogs`**
  - EN: Export Logs
  - ZH: 导出日志
  - [ ] OK    Fix: 

- **`exportSystem`**
  - EN: Export System Configuration
  - ZH: 导出系统配置
  - [ ] OK    Fix: 

- **`importSystem`**
  - EN: Import System Configuration
  - ZH: 导入系统配置
  - [ ] OK    Fix: 

- **`levelMeter`**
  - EN: Level Meters
  - ZH: 电平表
  - [ ] OK    Fix: 

- **`openLogFolder`**
  - EN: Open Log Folder
  - ZH: 打开日志文件夹
  - [ ] OK    Fix: 

- **`processingOff`**
  - EN: Processing: OFF
  - ZH: 处理: 关
  - [ ] OK    Fix: 

- **`processingOn`**
  - EN: Processing: ON
  - ZH: 处理: 开
  - [ ] OK    Fix: 

- **`quickLongPressOff`**
  - EN: Normal
  - ZH: 正常
  - [ ] OK    Fix: 

- **`quickLongPressOn`**
  - EN: Quick
  - ZH: 快速
  - [ ] OK    Fix: 

- **`reloadComplete`**
  - EN: Reload Complete Configuration
  - ZH: 重新加载完整配置
  - [ ] OK    Fix: 

- **`reloadCompleteBackup`**
  - EN: Reload Complete Config. Backup
  - ZH: 从备份加载完整配置
  - [ ] OK    Fix: 

- **`reloadSystem`**
  - EN: Reload System Configuration
  - ZH: 重新加载系统配置
  - [ ] OK    Fix: 

- **`reloadSystemBackup`**
  - EN: Reload System Config. Backup
  - ZH: 从备份加载系统配置
  - [ ] OK    Fix: 

- **`reportIssue`**
  - EN: Report Issue
  - ZH: 报告问题
  - [ ] OK    Fix: 

- **`selectProjectFolder`**
  - EN: Select Project Folder
  - ZH: 选择项目文件夹
  - [ ] OK    Fix: 

- **`setup`**
  - EN: Setup
  - ZH: 设置
  - [ ] OK    Fix: 

- **`soloModeMulti`**
  - EN: Solo: Multi
  - ZH: 独听: 多选
  - [ ] OK    Fix: 

- **`soloModeSingle`**
  - EN: Solo: Single
  - ZH: 独听: 单选
  - [ ] OK    Fix: 

- **`storeComplete`**
  - EN: Store Complete Configuration
  - ZH: 保存完整配置
  - [ ] OK    Fix: 

- **`storeSystem`**
  - EN: Store System Configuration
  - ZH: 保存系统配置
  - [ ] OK    Fix: 

## `systemConfig.colorSchemes`

- **`black`**
  - EN: Black
  - ZH: 黑色
  - [ ] OK    Fix: 

- **`default`**
  - EN: Default (Dark Gray)
  - ZH: 默认 (深灰色)
  - [ ] OK    Fix: 

- **`light`**
  - EN: Light
  - ZH: 浅色
  - [ ] OK    Fix: 

## `systemConfig.controller`

- **`lightpad`**
  - EN: Lightpad
  - ZH: Lightpad
  - [ ] OK    Fix: 

- **`off`**
  - EN: Off
  - ZH: 关闭
  - [ ] OK    Fix: 

- **`remote`**
  - EN: Remote
  - ZH: 遥控器
  - [ ] OK    Fix: 

## `systemConfig.devices`

- **`off`**
  - EN: Off
  - ZH: 关
  - [ ] OK    Fix: 

## `systemConfig.dialogs`

- **`exportSystemConfig`**
  - EN: Export System Configuration
  - ZH: 导出系统配置
  - [ ] OK    Fix: 

- **`importSystemConfig`**
  - EN: Import System Configuration
  - ZH: 导入系统配置
  - [ ] OK    Fix: 

- **`reduce`**
  - EN: Reduce
  - ZH: 减少
  - [ ] OK    Fix: 

- **`reduceInputChannels.message`**
  - EN: Reducing from {current} to {new} input channels will remove settings for channels {start} to {end}.

This cannot be undone.
  - ZH: 从 {current} 减少到 {new} 个输入通道将删除通道 {start} 到 {end} 的设置。

此操作无法撤销。
  - [ ] OK    Fix: 

- **`reduceInputChannels.title`**
  - EN: Reduce Input Channels?
  - ZH: 减少输入通道?
  - [ ] OK    Fix: 

- **`reduceOutputChannels.message`**
  - EN: Reducing from {current} to {new} output channels will remove settings for channels {start} to {end}.

This cannot be undone.
  - ZH: 从 {current} 减少到 {new} 个输出通道将删除通道 {start} 到 {end} 的设置。

此操作无法撤销。
  - [ ] OK    Fix: 

- **`reduceOutputChannels.title`**
  - EN: Reduce Output Channels?
  - ZH: 减少输出通道?
  - [ ] OK    Fix: 

- **`reduceReverbChannels.message`**
  - EN: Reducing from {current} to {new} reverb channels will remove settings for channels {start} to {end}.

This cannot be undone.
  - ZH: 从 {current} 减少到 {new} 个混响通道将删除通道 {start} 到 {end} 的设置。

此操作无法撤销。
  - [ ] OK    Fix: 

- **`reduceReverbChannels.title`**
  - EN: Reduce Reverb Channels?
  - ZH: 减少混响通道?
  - [ ] OK    Fix: 

- **`selectProjectFolder`**
  - EN: Select Project Folder
  - ZH: 选择项目文件夹
  - [ ] OK    Fix: 

## `systemConfig.help`

- **`algorithm`**
  - EN: Select the rendering algorithm from the menu.
  - ZH: 从菜单中选择渲染算法。
  - [ ] OK    Fix: 

- **`audioPatch`**
  - EN: Opens the Audio Interface and Patching Window.
  - ZH: 打开音频接口和路由窗口。
  - [ ] OK    Fix: 

- **`binauralAngle`**
  - EN: Horizontal rotation of binaural listener view (degrees, 0 = facing stage).
  - ZH: 双声道听者视角的水平旋转 (度，0 = 面向舞台)。
  - [ ] OK    Fix: 

- **`binauralAtten`**
  - EN: Overall level offset for binaural output (dB).
  - ZH: 双声道输出的整体电平偏移 (dB)。
  - [ ] OK    Fix: 

- **`binauralDelay`**
  - EN: Additional delay for binaural output (milliseconds).
  - ZH: 双声道输出的额外延迟 (毫秒)。
  - [ ] OK    Fix: 

- **`binauralDistance`**
  - EN: Distance of binaural listener from stage origin (meters).
  - ZH: 双声道听者距舞台原点的距离 (米)。
  - [ ] OK    Fix: 

- **`binauralEnable`**
  - EN: Enable or disable the binaural renderer processing.
  - ZH: 启用或禁用双声道渲染器处理。
  - [ ] OK    Fix: 

- **`binauralOutput`**
  - EN: Select output channel pair for binaural monitoring. Off disables binaural output.
  - ZH: 选择用于双声道监听的输出通道对。Off 禁用双声道输出。
  - [ ] OK    Fix: 

- **`colorScheme`**
  - EN: Select the color scheme: Default (dark gray), Black (pure black for OLED displays), or Light (daytime use).
  - ZH: 选择配色方案: 默认 (深灰色)、黑色 (适用于OLED显示屏) 或浅色 (日间使用)。
  - [ ] OK    Fix: 

- **`copySystemInfo`**
  - EN: Copy detailed system information to the clipboard for support requests.
  - ZH: 将详细的系统信息复制到剪贴板以用于支持请求。
  - [ ] OK    Fix: 

- **`diagnosticsToggle`**
  - EN: Long-press to show or hide the diagnostic tools (export logs, open log folder, copy system info, report issue).
  - ZH: 长按以显示或隐藏诊断工具 (导出日志、打开日志文件夹、复制系统信息、报告问题)。
  - [ ] OK    Fix: 

- **`dialsAndButtons`**
  - EN: Select the hardware controller for dials and buttons: Stream Deck+ or XenceLabs Quick Keys.
  - ZH: 为旋钮和按钮选择硬件控制器: Stream Deck+ 或 XenceLabs Quick Keys。
  - [ ] OK    Fix: 

- **`domeElevation`**
  - EN: Dome elevation angle: 180 = hemisphere, 360 = full sphere.
  - ZH: 穹顶仰角: 180 = 半球, 360 = 全球。
  - [ ] OK    Fix: 

- **`exportLogs`**
  - EN: Export diagnostic logs to a zip file for debugging or support.
  - ZH: 将诊断日志导出为 zip 文件以进行调试或支持。
  - [ ] OK    Fix: 

- **`exportSystem`**
  - EN: Export System Configuration to file (with file explorer window).
  - ZH: 将系统配置导出到文件 (使用文件浏览器)。
  - [ ] OK    Fix: 

- **`haasEffect`**
  - EN: Hass Effect to apply to the system. Will take into account the Latency Compensations (System, Input and Output).
  - ZH: 应用于系统的哈斯效应。将考虑延迟补偿 (系统、输入和输出)。
  - [ ] OK    Fix: 

- **`importSystem`**
  - EN: Import System Configuration from file (with file explorer window).
  - ZH: 从文件导入系统配置 (使用文件浏览器)。
  - [ ] OK    Fix: 

- **`inputChannels`**
  - EN: Number of Input Channels.
  - ZH: 输入通道数量。
  - [ ] OK    Fix: 

- **`language`**
  - EN: Select the user interface language. Changes take full effect after restarting the application.
  - ZH: 选择用户界面语言。重启应用程序后更改将完全生效。
  - [ ] OK    Fix: 

- **`lightpadSetup`**
  - EN: Display the connected Roli Lightpads and allow for splitting them in 4 smaller pads.
  - ZH: 显示已连接的Roli Lightpad并允许将其分成4个较小的触控板。
  - [ ] OK    Fix: 

- **`masterLevel`**
  - EN: Master Level (affects all outputs).
  - ZH: 主电平 (影响所有输出)。
  - [ ] OK    Fix: 

- **`openLogFolder`**
  - EN: Open the application's log folder in the system file explorer.
  - ZH: 在系统文件资源管理器中打开应用程序的日志文件夹。
  - [ ] OK    Fix: 

- **`originCenter`**
  - EN: Set origin to center of stage volume. Typical for Spherical Dome setups.
  - ZH: 将原点设置在舞台体积中心。适用于球形穹顶设置。
  - [ ] OK    Fix: 

- **`originCenterGround`**
  - EN: Set origin to center of stage at ground level. Typical for Surround or Cylindrical setups.
  - ZH: 将原点设置在舞台中心地面。适用于环绕声或圆柱形设置。
  - [ ] OK    Fix: 

- **`originDepth`**
  - EN: Origin Y offset from stage center (0 = centered, negative = front/downstage).
  - ZH: 原点Y偏移量 (0 = 居中, 负值 = 前方/舞台前)。
  - [ ] OK    Fix: 

- **`originDismissInputs`**
  - EN: Long press to dismiss and keep current input positions.
  - ZH: 长按以忽略并保持当前输入位置。
  - [ ] OK    Fix: 

- **`originDismissOutputs`**
  - EN: Long press to dismiss and keep current output positions.
  - ZH: 长按以忽略并保持当前输出位置。
  - [ ] OK    Fix: 

- **`originDismissReverbs`**
  - EN: Long press to dismiss and keep current reverb positions.
  - ZH: 长按以忽略并保持当前混响位置。
  - [ ] OK    Fix: 

- **`originFront`**
  - EN: Set origin to front center of stage. Typical for frontal stages.
  - ZH: 将原点设置在舞台前方中心。适用于正面舞台。
  - [ ] OK    Fix: 

- **`originHeight`**
  - EN: Origin Z offset from floor (0 = floor level, positive = above floor).
  - ZH: 原点Z偏移量 (0 = 地面, 正值 = 地面以上)。
  - [ ] OK    Fix: 

- **`originShiftInputs`**
  - EN: Long press to shift all input positions by the origin change.
  - ZH: 长按以根据原点变化移动所有输入位置。
  - [ ] OK    Fix: 

- **`originShiftOutputs`**
  - EN: Long press to shift all output positions by the origin change.
  - ZH: 长按以根据原点变化移动所有输出位置。
  - [ ] OK    Fix: 

- **`originShiftReverbs`**
  - EN: Long press to shift all reverb positions by the origin change.
  - ZH: 长按以根据原点变化移动所有混响位置。
  - [ ] OK    Fix: 

- **`originWidth`**
  - EN: Origin X offset from stage center (0 = centered, negative = left).
  - ZH: 原点X偏移量 (0 = 居中, 负值 = 左侧)。
  - [ ] OK    Fix: 

- **`outputChannels`**
  - EN: Number of Output Channels.
  - ZH: 输出通道数量。
  - [ ] OK    Fix: 

- **`positionControl`**
  - EN: Select the hardware controller for position control: Space Mouse, Joystick, or Game Pad.
  - ZH: 为位置控制选择硬件控制器: Space Mouse、操纵杆或游戏手柄。
  - [ ] OK    Fix: 

- **`processing`**
  - EN: Lock all I/O parameters and start the DSP. Long press to stop the DSP.
  - ZH: 锁定所有输入/输出参数并启动DSP。长按停止DSP。
  - [ ] OK    Fix: 

- **`quickLongPress`**
  - EN: Long press duration. Instead of confirmation windows this software uses long presses.
  - ZH: 长按持续时间。此软件使用长按代替确认窗口。
  - [ ] OK    Fix: 

- **`reloadComplete`**
  - EN: Reload Complete Configuration from files.
  - ZH: 从文件重新加载完整配置。
  - [ ] OK    Fix: 

- **`reloadCompleteBackup`**
  - EN: Reload Complete Configuration from backup files.
  - ZH: 从备份文件重新加载完整配置。
  - [ ] OK    Fix: 

- **`reloadSystem`**
  - EN: Reload System Configuration from file.
  - ZH: 从文件重新加载系统配置。
  - [ ] OK    Fix: 

- **`reloadSystemBackup`**
  - EN: Reload System Configuration from backup file.
  - ZH: 从备份文件重新加载系统配置。
  - [ ] OK    Fix: 

- **`remoteSetup`**
  - EN: Select the number of pads in the Remote's XY Pads tab.
  - ZH: 选择Remote XY Pads选项卡中的触控板数量。
  - [ ] OK    Fix: 

- **`reportIssue`**
  - EN: Open the WFS-DIY GitHub issues page in your default browser.
  - ZH: 在默认浏览器中打开 WFS-DIY 的 GitHub 问题页面。
  - [ ] OK    Fix: 

- **`reverbChannels`**
  - EN: Number of Reverb Channels.
  - ZH: 混响通道数量。
  - [ ] OK    Fix: 

- **`sampler`**
  - EN: Enable or disable the Sampler feature for input channels.
  - ZH: 启用或禁用输入通道的采样器功能。选择控制器: Lightpad 或遥控器。
  - [ ] OK    Fix: 

- **`selectProjectFolder`**
  - EN: Select the Location of the Current Project Folder where to store files.
  - ZH: 选择当前项目文件夹的存储位置。
  - [ ] OK    Fix: 

- **`showLocation`**
  - EN: Location of the current show.
  - ZH: 当前演出的地点。
  - [ ] OK    Fix: 

- **`showName`**
  - EN: Name of the current show.
  - ZH: 当前演出的名称。
  - [ ] OK    Fix: 

- **`soloMode`**
  - EN: Single: one input at a time. Multi: multiple inputs simultaneously.
  - ZH: 单选：一次只听一个输入。多选：可同时听多个输入。
  - [ ] OK    Fix: 

- **`speedOfSound`**
  - EN: Speed of Sound (related to the temperature).
  - ZH: 声速 (与温度相关)。
  - [ ] OK    Fix: 

- **`stageDepth`**
  - EN: Depth of the stage in meters (Box shape only).
  - ZH: 舞台深度 (米) (仅限立方体形状)。
  - [ ] OK    Fix: 

- **`stageDiameter`**
  - EN: Diameter of the stage in meters (Cylinder and Dome shapes).
  - ZH: 舞台直径 (米) (圆柱体和穹顶形状)。
  - [ ] OK    Fix: 

- **`stageDismiss`**
  - EN: Long press to dismiss and keep current input positions.
  - ZH: 长按以忽略并保持当前输入位置。
  - [ ] OK    Fix: 

- **`stageFit`**
  - EN: Long press to move outlier inputs back within the new stage bounds.
  - ZH: 长按以将超出范围的输入移回新舞台边界内。
  - [ ] OK    Fix: 

- **`stageHeight`**
  - EN: Height of the stage in meters (Box and Cylinder shapes).
  - ZH: 舞台高度 (米) (立方体和圆柱体形状)。
  - [ ] OK    Fix: 

- **`stageScale`**
  - EN: Long press to scale all input positions proportionally to the new stage dimensions.
  - ZH: 长按以按比例缩放所有输入位置至新舞台尺寸。
  - [ ] OK    Fix: 

- **`stageShape`**
  - EN: Select the shape of the stage (Box, Cylinder, or Dome).
  - ZH: 选择舞台形状 (立方体、圆柱体或穹顶)。
  - [ ] OK    Fix: 

- **`stageWidth`**
  - EN: Width of the stage in meters (Box shape only).
  - ZH: 舞台宽度 (米) (仅限立方体形状)。
  - [ ] OK    Fix: 

- **`storeComplete`**
  - EN: Store Complete Configuration to files (with backup).
  - ZH: 将完整配置保存到文件 (含备份)。
  - [ ] OK    Fix: 

- **`storeSystem`**
  - EN: Store System Configuration to file (with backup).
  - ZH: 将系统配置保存到文件 (含备份)。
  - [ ] OK    Fix: 

- **`systemLatency`**
  - EN: Total latency of the system (Mixing board & Computer) / Specific Input and Output Latency/Delay can be set in the respective Input and Output settings.
  - ZH: 系统总延迟 (调音台和计算机) / 特定输入和输出延迟可在各自设置中设定。
  - [ ] OK    Fix: 

- **`temperature`**
  - EN: Temperature (gives the Speed of Sound).
  - ZH: 温度 (决定声速)。
  - [ ] OK    Fix: 

## `systemConfig.labels`

- **`algorithm`**
  - EN: Algorithm:
  - ZH: 算法:
  - [ ] OK    Fix: 

- **`binauralAngle`**
  - EN: Listener Angle:
  - ZH: 听者角度:
  - [ ] OK    Fix: 

- **`binauralAtten`**
  - EN: Binaural Level:
  - ZH: 双耳电平:
  - [ ] OK    Fix: 

- **`binauralDelay`**
  - EN: Binaural Delay:
  - ZH: 双耳延迟:
  - [ ] OK    Fix: 

- **`binauralDistance`**
  - EN: Listener Distance:
  - ZH: 听者距离:
  - [ ] OK    Fix: 

- **`binauralOutput`**
  - EN: Binaural Output:
  - ZH: 双耳输出:
  - [ ] OK    Fix: 

- **`clickToSplit`**
  - EN: Click to split
  - ZH: 点击以拆分
  - [ ] OK    Fix: 

- **`colorScheme`**
  - EN: Color Scheme:
  - ZH: 配色方案:
  - [ ] OK    Fix: 

- **`dialsAndButtons`**
  - EN: Dials and Buttons:
  - ZH: 旋钮和按钮:
  - [ ] OK    Fix: 

- **`domeElevation`**
  - EN: Elevation:
  - ZH: 穹顶角度:
  - [ ] OK    Fix: 

- **`haasEffect`**
  - EN: Haas Effect:
  - ZH: 哈斯效应:
  - [ ] OK    Fix: 

- **`inputChannels`**
  - EN: Input Channels:
  - ZH: 输入通道:
  - [ ] OK    Fix: 

- **`language`**
  - EN: Language:
  - ZH: 语言:
  - [ ] OK    Fix: 

- **`lightpadArrangement`**
  - EN: Lightpad Arrangement
  - ZH: Lightpad 布局
  - [ ] OK    Fix: 

- **`masterLevel`**
  - EN: Master Level:
  - ZH: 主电平:
  - [ ] OK    Fix: 

- **`originDepth`**
  - EN: Origin Depth:
  - ZH: 原点深度:
  - [ ] OK    Fix: 

- **`originHeight`**
  - EN: Origin Height:
  - ZH: 原点高度:
  - [ ] OK    Fix: 

- **`originWidth`**
  - EN: Origin Width:
  - ZH: 原点宽度:
  - [ ] OK    Fix: 

- **`outputChannels`**
  - EN: Output Channels:
  - ZH: 输出通道:
  - [ ] OK    Fix: 

- **`positionControl`**
  - EN: Position Control:
  - ZH: 位置控制:
  - [ ] OK    Fix: 

- **`quickLongPress`**
  - EN: Long Press:
  - ZH: 长按:
  - [ ] OK    Fix: 

- **`reverbChannels`**
  - EN: Reverb Channels:
  - ZH: 混响通道:
  - [ ] OK    Fix: 

- **`sampler`**
  - EN: Sampler:
  - ZH: 采样器:
  - [ ] OK    Fix: 

- **`showLocation`**
  - EN: Location:
  - ZH: 地点:
  - [ ] OK    Fix: 

- **`showName`**
  - EN: Name:
  - ZH: 名称:
  - [ ] OK    Fix: 

- **`speedOfSound`**
  - EN: Speed of Sound:
  - ZH: 声速:
  - [ ] OK    Fix: 

- **`split`**
  - EN: Split
  - ZH: 拆分
  - [ ] OK    Fix: 

- **`stageDepth`**
  - EN: Depth:
  - ZH: 深度:
  - [ ] OK    Fix: 

- **`stageDiameter`**
  - EN: Diameter:
  - ZH: 直径:
  - [ ] OK    Fix: 

- **`stageHeight`**
  - EN: Height:
  - ZH: 高度:
  - [ ] OK    Fix: 

- **`stageShape`**
  - EN: Stage Shape:
  - ZH: 舞台形状:
  - [ ] OK    Fix: 

- **`stageWidth`**
  - EN: Width:
  - ZH: 宽度:
  - [ ] OK    Fix: 

- **`systemLatency`**
  - EN: System Latency:
  - ZH: 系统延迟:
  - [ ] OK    Fix: 

- **`temperature`**
  - EN: Temperature:
  - ZH: 温度:
  - [ ] OK    Fix: 

- **`updateAvailable`**
  - EN: Update {version} available
  - ZH: 更新 {version} 可用
  - [ ] OK    Fix: 

- **`version`**
  - EN: Version {version}
  - ZH: 版本 {version}
  - [ ] OK    Fix: 

## `systemConfig.messages`

- **`configLoaded`**
  - EN: Complete configuration loaded.
  - ZH: 完整配置已加载。
  - [ ] OK    Fix: 

- **`configLoadedFromBackup`**
  - EN: Configuration loaded from backup.
  - ZH: 已从备份加载配置。
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Complete configuration saved.
  - ZH: 完整配置已保存。
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - ZH: 错误: {error}
  - [ ] OK    Fix: 

- **`languageChanged`**
  - EN: Language changed to: {language} (requires restart for full effect)
  - ZH: 语言已更改为: {language} (需要重启才能完全生效)
  - [ ] OK    Fix: 

- **`logDirNotFound`**
  - EN: Log directory not found
  - ZH: 未找到日志目录
  - [ ] OK    Fix: 

- **`logsExported`**
  - EN: Logs exported to {path}
  - ZH: 日志已导出到 {path}
  - [ ] OK    Fix: 

- **`logsExportFailed`**
  - EN: Failed to export logs
  - ZH: 导出日志失败
  - [ ] OK    Fix: 

- **`noBackupFilesFound`**
  - EN: No backup files found.
  - ZH: 未找到备份文件。
  - [ ] OK    Fix: 

- **`partialLoad`**
  - EN: Partial load: {error}
  - ZH: 部分加载: {error}
  - [ ] OK    Fix: 

- **`partialLoadFromBackup`**
  - EN: Partial load from backup: {error}
  - ZH: 从备份部分加载: {error}
  - [ ] OK    Fix: 

- **`restartBody`**
  - EN: Please restart the application for the language change to take full effect.
  - ZH: 请重启应用程序以使语言更改完全生效。
  - [ ] OK    Fix: 

- **`restartTitle`**
  - EN: Restart Required
  - ZH: 需要重启
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder first.
  - ZH: 请先选择项目文件夹。
  - [ ] OK    Fix: 

- **`selectLogExportFolder`**
  - EN: Select destination for log export
  - ZH: 选择日志导出目标文件夹
  - [ ] OK    Fix: 

- **`systemConfigExported`**
  - EN: System configuration exported.
  - ZH: 系统配置已导出。
  - [ ] OK    Fix: 

- **`systemConfigFileNotFound`**
  - EN: System configuration file not found.
  - ZH: 未找到系统配置文件。
  - [ ] OK    Fix: 

- **`systemConfigImported`**
  - EN: System configuration imported.
  - ZH: 系统配置已导入。
  - [ ] OK    Fix: 

- **`systemConfigLoaded`**
  - EN: System configuration loaded.
  - ZH: 系统配置已加载。
  - [ ] OK    Fix: 

- **`systemConfigLoadedFromBackup`**
  - EN: System configuration loaded from backup.
  - ZH: 已从备份加载系统配置。
  - [ ] OK    Fix: 

- **`systemConfigSaved`**
  - EN: System configuration saved.
  - ZH: 系统配置已保存。
  - [ ] OK    Fix: 

- **`systemInfoCopied`**
  - EN: System info copied to clipboard
  - ZH: 系统信息已复制到剪贴板
  - [ ] OK    Fix: 

## `systemConfig.sections`

- **`binauralRenderer`**
  - EN: Binaural Renderer
  - ZH: 双耳渲染器
  - [ ] OK    Fix: 

- **`controllers`**
  - EN: Controllers
  - ZH: Controllers
  - [ ] OK    Fix: 

- **`io`**
  - EN: I/O
  - ZH: 输入/输出
  - [ ] OK    Fix: 

- **`master`**
  - EN: Master Section
  - ZH: 主控区
  - [ ] OK    Fix: 

- **`show`**
  - EN: Show
  - ZH: 演出
  - [ ] OK    Fix: 

- **`stage`**
  - EN: Stage
  - ZH: 舞台
  - [ ] OK    Fix: 

- **`ui`**
  - EN: UI
  - ZH: 界面
  - [ ] OK    Fix: 

- **`wfsProcessor`**
  - EN: WFS Processor
  - ZH: WFS处理器
  - [ ] OK    Fix: 

## `systemConfig.stageShapes`

- **`box`**
  - EN: Box
  - ZH: 立方体
  - [ ] OK    Fix: 

- **`cylinder`**
  - EN: Cylinder
  - ZH: 圆柱体
  - [ ] OK    Fix: 

- **`dome`**
  - EN: Dome
  - ZH: 穹顶
  - [ ] OK    Fix: 

## `tabs`

- **`clusters`**
  - EN: Clusters
  - ZH: 集群
  - [ ] OK    Fix: 

- **`inputs`**
  - EN: Inputs
  - ZH: 输入
  - [ ] OK    Fix: 

- **`map`**
  - EN: Map
  - ZH: 地图
  - [ ] OK    Fix: 

- **`network`**
  - EN: Network
  - ZH: 网络
  - [ ] OK    Fix: 

- **`outputs`**
  - EN: Outputs
  - ZH: 输出
  - [ ] OK    Fix: 

- **`reverb`**
  - EN: Reverb
  - ZH: 混响
  - [ ] OK    Fix: 

- **`systemConfig`**
  - EN: System Config
  - ZH: 系统配置
  - [ ] OK    Fix: 

## `touchscreens`

- **`button`**
  - EN: Setup
  - ZH: 设置
  - [ ] OK    Fix: 

- **`label`**
  - EN: Touchscreen
  - ZH: 触摸屏
  - [ ] OK    Fix: 

## `units`

- **`celsius`**
  - EN: °C
  - ZH: °C
  - [ ] OK    Fix: 

- **`decibelPerMeter`**
  - EN: dB/m
  - ZH: dB/m
  - [ ] OK    Fix: 

- **`decibels`**
  - EN: dB
  - ZH: dB
  - [ ] OK    Fix: 

- **`degrees`**
  - EN: °
  - ZH: °
  - [ ] OK    Fix: 

- **`meters`**
  - EN: m
  - ZH: m
  - [ ] OK    Fix: 

- **`metersPerSecond`**
  - EN: m/s
  - ZH: m/s
  - [ ] OK    Fix: 

- **`milliseconds`**
  - EN: ms
  - ZH: ms
  - [ ] OK    Fix: 

## `wizard.buttons`

- **`back`**
  - EN: Back
  - ZH: 上一步
  - [ ] OK    Fix: 

- **`close`**
  - EN: Close
  - ZH: 关闭
  - [ ] OK    Fix: 

- **`done`**
  - EN: Done
  - ZH: 完成
  - [ ] OK    Fix: 

- **`gettingStarted`**
  - EN: Getting Started
  - ZH: 入门指南
  - [ ] OK    Fix: 

- **`gettingStartedHelp`**
  - EN: Help cards guiding you through the first parameters to adjust when starting a new project
  - ZH: 帮助卡片，引导您完成新项目启动时需要调整的第一批参数
  - [ ] OK    Fix: 

- **`next`**
  - EN: Next
  - ZH: 下一步
  - [ ] OK    Fix: 

- **`skip`**
  - EN: Skip
  - ZH: 跳过
  - [ ] OK    Fix: 

## `wizard.steps`

- **`audioDevice.description`**
  - EN: Select your audio driver and device, set the sample rate and buffer size. Check the patch routing and test your outputs. Close this window when done.
  - ZH: 选择音频驱动程序和设备，设置采样率和缓冲区大小。检查补丁路由并测试输出。完成后关闭此窗口。
  - [ ] OK    Fix: 

- **`audioDevice.title`**
  - EN: Configure the Audio Interface
  - ZH: 配置音频接口
  - [ ] OK    Fix: 

- **`audioInterface.description`**
  - EN: Click the button above or press Next to open the Audio Interface window.
  - ZH: 点击上方按钮或按下一步打开音频接口窗口。
  - [ ] OK    Fix: 

- **`audioInterface.title`**
  - EN: Open the Audio Interface
  - ZH: 打开音频接口
  - [ ] OK    Fix: 

- **`configureOutputs.description`**
  - EN: Use the array presets and geometry tools to calculate speaker positions for your arrays. Close this window when done.
  - ZH: 使用阵列预设和几何工具计算扬声器位置。完成后关闭此窗口。
  - [ ] OK    Fix: 

- **`configureOutputs.title`**
  - EN: Configure Output Positions
  - ZH: 配置输出位置
  - [ ] OK    Fix: 

- **`exploreInputs.description`**
  - EN: Click an input on the map to select it, or lasso several to move them together. Drag to position your sources. Zoom with the mouse wheel or pinch gesture, pan with right-drag or two-finger drag. Add inputs, group them into clusters, and shape your sound field. You can also control positions with a keyboard, SpaceMouse, or other external controllers. Have fun!
  - ZH: 点击地图上的输入进行选择，或用套索选择多个。拖动定位您的声源。用鼠标滚轮或捏合手势缩放，用右键拖动或双指拖动平移。添加输入，将它们分组为集群，塑造您的声场。您还可以用键盘、SpaceMouse或其他控制器控制位置。尽情享受！
  - [ ] OK    Fix: 

- **`exploreInputs.title`**
  - EN: Start Creating!
  - ZH: 开始创作！
  - [ ] OK    Fix: 

- **`inputChannels.description`**
  - EN: How many audio sources will you be spatializing?
Set the number of input channels to match your source count.
  - ZH: 您将空间化多少个音频源？
根据源数量设置输入通道数。
  - [ ] OK    Fix: 

- **`inputChannels.title`**
  - EN: Set Input Channels
  - ZH: 设置输入通道数
  - [ ] OK    Fix: 

- **`originPoint.description`**
  - EN: The origin is the reference point for all coordinates. Use the preset buttons or enter custom values. 'Front' places it at the audience edge.
  - ZH: 原点是所有坐标的参考点。使用预设按钮或输入自定义值。'Front'将其放置在观众边缘。
  - [ ] OK    Fix: 

- **`originPoint.title`**
  - EN: Set the Origin Point
  - ZH: 设置原点
  - [ ] OK    Fix: 

- **`outputChannels.description`**
  - EN: Set the number of output channels to match your speaker array.
Each output corresponds to one physical speaker.
  - ZH: 根据您的扬声器阵列设置输出通道数。
每个输出对应一个物理扬声器。
  - [ ] OK    Fix: 

- **`outputChannels.title`**
  - EN: Set Output Channels
  - ZH: 设置输出通道数
  - [ ] OK    Fix: 

- **`projectFolder.description`**
  - EN: Choose a folder to store your WFS project files. This will hold configurations, snapshots, IR files, and samples. Click the button to open the folder selector.
  - ZH: 选择一个文件夹来存储您的WFS项目文件。它将保存配置、快照、IR文件和采样。点击按钮打开文件夹选择器。
  - [ ] OK    Fix: 

- **`projectFolder.title`**
  - EN: Select a Project Folder
  - ZH: 选择项目文件夹
  - [ ] OK    Fix: 

- **`reverbChannels.description`**
  - EN: Reverb channels add room simulation. Set to 0 if you don't need reverb.
  - ZH: 混响通道添加房间模拟。如果不需要混响，请设置为0。
  - [ ] OK    Fix: 

- **`reverbChannels.title`**
  - EN: Set Reverb Channels
  - ZH: 设置混响通道数
  - [ ] OK    Fix: 

- **`stageConfig.description`**
  - EN: Set the shape and dimensions of your performance space. Choose box, cylinder, or dome, then enter the size in meters.
  - ZH: 设置演出空间的形状和尺寸。选择箱体、圆柱或穹顶，然后以米为单位输入尺寸。
  - [ ] OK    Fix: 

- **`stageConfig.title`**
  - EN: Define the Stage
  - ZH: 定义舞台
  - [ ] OK    Fix: 

- **`startProcessing.description`**
  - EN: You're all set! Long-press the Processing button to start the WFS engine. You can also start the Binaural Renderer for headphone monitoring.
  - ZH: 一切就绪！长按Processing按钮启动WFS引擎。您也可以启动双耳渲染器进行耳机监听。
  - [ ] OK    Fix: 

- **`startProcessing.title`**
  - EN: Start the WFS Engine
  - ZH: 启动WFS引擎
  - [ ] OK    Fix: 

- **`wizardOfOutZ.description`**
  - EN: Click the Wizard of OutZ button or press Next to open the output array helper.
  - ZH: 点击Wizard of OutZ按钮或按下一步打开定位助手。
  - [ ] OK    Fix: 

- **`wizardOfOutZ.title`**
  - EN: Position Your Outputs
  - ZH: 定位输出
  - [ ] OK    Fix: 


