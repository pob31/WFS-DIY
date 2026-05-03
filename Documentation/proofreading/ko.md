# Proofreading checklist — Korean (한국어)

Locale: `ko`  |  Total keys: 1520  |  Source: `Resources/lang/en.json` vs `Resources/lang/ko.json`

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
  - KO: 적용됨
  - [ ] OK    Fix: 

- **`atHead`**
  - EN: (no undone records — at the head)
  - KO: (실행 취소된 기록 없음 — 맨 앞)
  - [ ] OK    Fix: 

- **`batch`**
  - EN: batch {id}
  - KO: 배치 {id}
  - [ ] OK    Fix: 

- **`cursorLabel`**
  - EN: ◂  cursor (↑ applied  /  ↓ undone, redoable)
  - KO: ◂  커서 (↑ 적용됨  /  ↓ 실행 취소됨, 재실행 가능)
  - [ ] OK    Fix: 

- **`noChanges`**
  - EN: No AI changes yet.
  - KO: 아직 AI 변경 사항이 없습니다.
  - [ ] OK    Fix: 

- **`of`**
  - EN: of
  - KO: /
  - [ ] OK    Fix: 

- **`stepBack`**
  - EN: ⏮ Step Back
  - KO: ⏮ 뒤로
  - [ ] OK    Fix: 

- **`stepForward`**
  - EN: Step Forward ⏭
  - KO: 앞으로 ⏭
  - [ ] OK    Fix: 

- **`undone`**
  - EN: undone
  - KO: 실행 취소됨
  - [ ] OK    Fix: 

- **`windowTitle`**
  - EN: AI Change History
  - KO: AI 변경 기록
  - [ ] OK    Fix: 

## `ai.server`

- **`copyUrlConfirm`**
  - EN: MCP URL copied to clipboard: {url}
  - KO: MCP URL이 클립보드에 복사되었습니다: {url}
  - [ ] OK    Fix: 

- **`label`**
  - EN: MCP Server:
  - KO: MCP 서버:
  - [ ] OK    Fix: 

- **`openHistory`**
  - EN: Open AI History
  - KO: AI 기록 열기
  - [ ] OK    Fix: 

- **`urlButtonRunning`**
  - EN: http://{host}:{port}/mcp
  - KO: http://{host}:{port}/mcp
  - [ ] OK    Fix: 

- **`urlButtonStopped`**
  - EN: (server not running)
  - KO: (서버 미실행)
  - [ ] OK    Fix: 

## `ai.tier`

- **`aiOff`**
  - EN: AI: OFF
  - KO: AI: OFF
  - [ ] OK    Fix: 

- **`aiOn`**
  - EN: AI: ON
  - KO: AI: ON
  - [ ] OK    Fix: 

- **`gateAllowed`**
  - EN: AI critical actions: ALLOWED
  - KO: AI 중요 동작: 허용됨
  - [ ] OK    Fix: 

- **`gateBlocked`**
  - EN: AI critical actions: blocked
  - KO: AI 중요 동작: 차단됨
  - [ ] OK    Fix: 

- **`tier2AutoOff`**
  - EN: Tier 2 auto-confirm: off
  - KO: 티어 2 자동 확인: 끔
  - [ ] OK    Fix: 

- **`tier2AutoOn`**
  - EN: Tier 2 auto-confirm: ON (5 min)
  - KO: 티어 2 자동 확인: ON (5분)
  - [ ] OK    Fix: 

## `ai.toast`

- **`clientPrefix`**
  - EN: Claude
  - KO: Claude
  - [ ] OK    Fix: 

- **`header`**
  - EN: AI changes
  - KO: AI 변경
  - [ ] OK    Fix: 

- **`moreOlder`**
  - EN: …and {count} older
  - KO: …그리고 이전 {count}개
  - [ ] OK    Fix: 

## `ai.tooltips`

- **`aiToggle`**
  - EN: Master switch for the whole MCP integration. When OFF every AI tool call is refused; when ON, normal tier handling applies (the critical-actions toggle still controls destructive calls separately).
  - KO: 전체 MCP 통합의 메인 스위치. OFF일 때 모든 AI 도구 호출이 거부됩니다. ON일 때 일반 티어 처리가 적용됩니다 (중요 동작 토글은 파괴적 호출을 별도로 제어).
  - [ ] OK    Fix: 

- **`gate`**
  - EN: Allow destructive AI actions (channel-count changes, port reconfigure, runDSP, etc.) AND skip the per-call confirm handshake for less-destructive tier-2 actions while open. Acts as a superset of the Tier 2 auto-confirm toggle. The red fill drains over 5 minutes, then auto-blocks again.
  - KO: 파괴적인 AI 동작(채널 수 변경, 포트 재구성, runDSP 등)을 허용하고 열려 있는 동안 덜 파괴적인 티어 2 동작에 대해 호출별 확인 핸드셰이크를 건너뜁니다. 티어 2 자동 확인 토글의 상위 집합 역할을 합니다. 빨간색 채우기는 5분에 걸쳐 줄어들고, 그 후 자동으로 다시 차단됩니다.
  - [ ] OK    Fix: 

- **`openHistory`**
  - EN: Open the AI History window: scrollable timeline of every recent AI change with per-row undo / redo and a step-by-step cursor.
  - KO: AI 기록 창 열기: 행별 실행 취소/다시 실행과 단계별 커서가 있는 최근 모든 AI 변경의 스크롤 가능한 타임라인.
  - [ ] OK    Fix: 

- **`urlButton`**
  - EN: Click to copy the MCP server URL. Useful for Claude Code (claude mcp add wfs-diy <URL> -t http) or any MCP client that takes a URL. Claude Desktop instead uses the JSON config snippet — open the (?) help card.
  - KO: MCP 서버 URL을 복사하려면 클릭하세요. Claude Code (claude mcp add wfs-diy <URL> -t http) 또는 URL을 받는 모든 MCP 클라이언트에 유용합니다. Claude Desktop은 대신 JSON 구성 스니펫을 사용합니다 — (?) 도움말 카드를 여세요.
  - [ ] OK    Fix: 

## `ai.undo`

- **`errorPrefix`**
  - EN: AI {verb}: {message}
  - KO: AI {verb}: {message}
  - [ ] OK    Fix: 

- **`successPrefix`**
  - EN: AI {verb}: {description}
  - KO: AI {verb}: {description}
  - [ ] OK    Fix: 

- **`verbRedo`**
  - EN: redo
  - KO: 다시 실행
  - [ ] OK    Fix: 

- **`verbUndo`**
  - EN: undo
  - KO: 실행 취소
  - [ ] OK    Fix: 

## `arrayHelper.acoustic`

- **`distanceAtten`**
  - EN: Distance Atten (%):
  - KO: 거리 감쇠 (%):
  - [ ] OK    Fix: 

- **`floorReflections`**
  - EN: Floor Reflections
  - KO: 바닥 반사
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: HF Damping (dB/m):
  - KO: 고주파 감쇠 (dB/m):
  - [ ] OK    Fix: 

- **`highCut`**
  - EN: High Cut (Hz):
  - KO: 하이 컷 (Hz):
  - [ ] OK    Fix: 

- **`hParallax`**
  - EN: H Parallax (m):
  - KO: 수평 시차 (m):
  - [ ] OK    Fix: 

- **`liveSource`**
  - EN: Live Source
  - KO: 라이브 소스
  - [ ] OK    Fix: 

- **`lowCut`**
  - EN: Low Cut (Hz):
  - KO: 로우 컷 (Hz):
  - [ ] OK    Fix: 

- **`vParallax`**
  - EN: V Parallax (m):
  - KO: 수직 시차 (m):
  - [ ] OK    Fix: 

## `arrayHelper.buttons`

- **`apply`**
  - EN: Apply
  - KO: 적용
  - [ ] OK    Fix: 

- **`close`**
  - EN: Close
  - KO: 닫기
  - [ ] OK    Fix: 

## `arrayHelper.errors`

- **`noPositions`**
  - EN: No positions to apply. Check geometry parameters.
  - KO: 적용할 위치가 없습니다. 기하 매개변수를 확인하세요.
  - [ ] OK    Fix: 

- **`notEnoughOutputs`**
  - EN: Not enough output channels! Need {count} starting from {start}
  - KO: 출력 채널이 부족합니다! {start}부터 {count}개가 필요합니다
  - [ ] OK    Fix: 

- **`prefix`**
  - EN: Error: 
  - KO: 오류: 
  - [ ] OK    Fix: 

- **`speakerCountZero`**
  - EN: Number of speakers must be greater than 0
  - KO: 스피커 수는 0보다 커야 합니다
  - [ ] OK    Fix: 

## `arrayHelper.geometry`

- **`backFacing`**
  - EN: Back Facing
  - KO: 뒤쪽 방향
  - [ ] OK    Fix: 

- **`centerSpacing`**
  - EN: Center + Spacing
  - KO: 중심 + 간격
  - [ ] OK    Fix: 

- **`centerX`**
  - EN: Center X (m):
  - KO: 중심 X (m):
  - [ ] OK    Fix: 

- **`centerY`**
  - EN: Center Y (m):
  - KO: 중심 Y (m):
  - [ ] OK    Fix: 

- **`endpoints`**
  - EN: Endpoints
  - KO: 끝점
  - [ ] OK    Fix: 

- **`endX`**
  - EN: End X (m):
  - KO: 끝 X (m):
  - [ ] OK    Fix: 

- **`endY`**
  - EN: End Y (m):
  - KO: 끝 Y (m):
  - [ ] OK    Fix: 

- **`facingInward`**
  - EN: Facing Inward
  - KO: 안쪽 방향
  - [ ] OK    Fix: 

- **`facingOutward`**
  - EN: Facing Outward
  - KO: 바깥쪽 방향
  - [ ] OK    Fix: 

- **`frontFacing`**
  - EN: Front Facing
  - KO: 앞쪽 방향
  - [ ] OK    Fix: 

- **`nPairs`**
  - EN: N Pairs:
  - KO: 쌍 수:
  - [ ] OK    Fix: 

- **`nSpeakers`**
  - EN: N Speakers:
  - KO: 스피커 수:
  - [ ] OK    Fix: 

- **`orientation`**
  - EN: Orientation (deg):
  - KO: 방향 (도):
  - [ ] OK    Fix: 

- **`radius`**
  - EN: Radius (m):
  - KO: 반지름 (m):
  - [ ] OK    Fix: 

- **`sag`**
  - EN: Sag (m):
  - KO: 처짐 (m):
  - [ ] OK    Fix: 

- **`spacing`**
  - EN: Spacing (m):
  - KO: 간격 (m):
  - [ ] OK    Fix: 

- **`startAngle`**
  - EN: Start Angle (deg):
  - KO: 시작 각도 (도):
  - [ ] OK    Fix: 

- **`startX`**
  - EN: Start X (m):
  - KO: 시작 X (m):
  - [ ] OK    Fix: 

- **`startY`**
  - EN: Start Y (m):
  - KO: 시작 Y (m):
  - [ ] OK    Fix: 

- **`width`**
  - EN: Width (m):
  - KO: 너비 (m):
  - [ ] OK    Fix: 

- **`yEnd`**
  - EN: Y End (m):
  - KO: Y 끝 (m):
  - [ ] OK    Fix: 

- **`yStart`**
  - EN: Y Start (m):
  - KO: Y 시작 (m):
  - [ ] OK    Fix: 

- **`zHeight`**
  - EN: Z Height (m):
  - KO: Z 높이 (m):
  - [ ] OK    Fix: 

## `arrayHelper.presets`

- **`circle`**
  - EN: Circle
  - KO: 원형
  - [ ] OK    Fix: 

- **`delayLine`**
  - EN: Delay Line
  - KO: 딜레이 라인
  - [ ] OK    Fix: 

- **`label`**
  - EN: Preset:
  - KO: 프리셋:
  - [ ] OK    Fix: 

- **`mainRoomStraight`**
  - EN: Main Flown Array Straight
  - KO: 메인 플라잉 어레이 직선
  - [ ] OK    Fix: 

- **`nearFieldCurved`**
  - EN: Near Field Array Curved
  - KO: 근거리 어레이 곡선
  - [ ] OK    Fix: 

- **`nearFieldStraight`**
  - EN: Near Field Array Straight
  - KO: 근거리 어레이 직선
  - [ ] OK    Fix: 

- **`subBass`**
  - EN: Sub Bass
  - KO: 서브 베이스
  - [ ] OK    Fix: 

- **`surround`**
  - EN: Surround
  - KO: 서라운드
  - [ ] OK    Fix: 

## `arrayHelper.preview`

- **`audience`**
  - EN: Audience
  - KO: 관객
  - [ ] OK    Fix: 

## `arrayHelper.sections`

- **`acousticDefaults`**
  - EN: Acoustic Defaults
  - KO: 음향 기본값
  - [ ] OK    Fix: 

- **`geometry`**
  - EN: Geometry
  - KO: 기하
  - [ ] OK    Fix: 

- **`target`**
  - EN: Target
  - KO: 대상
  - [ ] OK    Fix: 

## `arrayHelper.status`

- **`applied`**
  - EN: Applied {count} speakers to Array {array}. Ready for next array.
  - KO: {count}개의 스피커가 어레이 {array}에 적용됨. 다음 어레이 준비됨.
  - [ ] OK    Fix: 

- **`calculated`**
  - EN: Calculated {count} positions
  - KO: {count}개의 위치가 계산됨
  - [ ] OK    Fix: 

- **`ready`**
  - EN: Ready
  - KO: 준비됨
  - [ ] OK    Fix: 

## `arrayHelper.target`

- **`array`**
  - EN: Array:
  - KO: 어레이:
  - [ ] OK    Fix: 

- **`arrayPrefix`**
  - EN: Array
  - KO: 어레이
  - [ ] OK    Fix: 

- **`startingOutput`**
  - EN: Starting Output:
  - KO: 시작 출력:
  - [ ] OK    Fix: 

## `arrayHelper.window`

- **`contentName`**
  - EN: Output Array Helper
  - KO: 출력 어레이 도우미
  - [ ] OK    Fix: 

- **`title`**
  - EN: Wizard of OutZ
  - KO: OutZ 마법사
  - [ ] OK    Fix: 

## `audioPatch`

- **`windowTitle`**
  - EN: Audio Interface and Patching
  - KO: 오디오 인터페이스 및 패치
  - [ ] OK    Fix: 

## `audioPatch.buttons`

- **`hold`**
  - EN: Hold
  - KO: 유지
  - [ ] OK    Fix: 

- **`unpatchAll`**
  - EN: Unpatch All
  - KO: 모두 연결 해제
  - [ ] OK    Fix: 

## `audioPatch.deviceSettings`

- **`buttons.controlPanel`**
  - EN: Control Panel
  - KO: 제어판
  - [ ] OK    Fix: 

- **`buttons.resetDevice`**
  - EN: Reset Device
  - KO: 장치 초기화
  - [ ] OK    Fix: 

- **`labels.bufferSize`**
  - EN: Audio buffer size:
  - KO: 오디오 버퍼 크기:
  - [ ] OK    Fix: 

- **`labels.device`**
  - EN: Device:
  - KO: 장치:
  - [ ] OK    Fix: 

- **`labels.deviceType`**
  - EN: Audio device type:
  - KO: 오디오 장치 유형:
  - [ ] OK    Fix: 

- **`labels.sampleRate`**
  - EN: Sample rate:
  - KO: 샘플 레이트:
  - [ ] OK    Fix: 

- **`noDevice`**
  - EN: No Device
  - KO: 장치 없음
  - [ ] OK    Fix: 

- **`notConfigured`**
  - EN: Not configured
  - KO: 구성되지 않음
  - [ ] OK    Fix: 

## `audioPatch.labels`

- **`interfaceInput`**
  - EN: Audio Interface Input
  - KO: 오디오 인터페이스 입력
  - [ ] OK    Fix: 

- **`interfaceOutput`**
  - EN: Audio Interface Output
  - KO: 오디오 인터페이스 출력
  - [ ] OK    Fix: 

- **`processorInputs`**
  - EN: Processor Inputs
  - KO: 프로세서 입력
  - [ ] OK    Fix: 

- **`processorOutputs`**
  - EN: Processor Outputs
  - KO: 프로세서 출력
  - [ ] OK    Fix: 

## `audioPatch.messages`

- **`chooseTestSignal`**
  - EN: Choose a Test Signal to Enable Testing
  - KO: 테스트 모드를 활성화하려면 테스트 신호를 선택하세요
  - [ ] OK    Fix: 

## `audioPatch.modes`

- **`patching`**
  - EN: Patching
  - KO: 패칭
  - [ ] OK    Fix: 

- **`scrolling`**
  - EN: Scrolling
  - KO: 스크롤
  - [ ] OK    Fix: 

- **`testing`**
  - EN: Testing
  - KO: 테스트
  - [ ] OK    Fix: 

## `audioPatch.tabs`

- **`deviceSettings`**
  - EN: Device Settings
  - KO: 장치 설정
  - [ ] OK    Fix: 

- **`inputPatch`**
  - EN: Input Patch
  - KO: 입력 패치
  - [ ] OK    Fix: 

- **`outputPatch`**
  - EN: Output Patch
  - KO: 출력 패치
  - [ ] OK    Fix: 

## `audioPatch.testSignal`

- **`labels.frequency`**
  - EN: Frequency:
  - KO: 주파수:
  - [ ] OK    Fix: 

- **`labels.level`**
  - EN: Level:
  - KO: 레벨:
  - [ ] OK    Fix: 

- **`labels.signal`**
  - EN: Signal:
  - KO: 신호:
  - [ ] OK    Fix: 

- **`types.diracPulse`**
  - EN: Dirac Pulse
  - KO: 디락 펄스
  - [ ] OK    Fix: 

- **`types.off`**
  - EN: Off
  - KO: 끄기
  - [ ] OK    Fix: 

- **`types.pinkNoise`**
  - EN: Pink Noise
  - KO: 핑크 노이즈
  - [ ] OK    Fix: 

- **`types.pulse`**
  - EN: Pulse
  - KO: 펄스
  - [ ] OK    Fix: 

- **`types.sweep`**
  - EN: Sweep
  - KO: 스윕
  - [ ] OK    Fix: 

- **`types.tone`**
  - EN: Tone
  - KO: 톤
  - [ ] OK    Fix: 

## `clusters.help`

- **`attenuationSlider`**
  - EN: Adjust attenuation of all cluster inputs (dB).
  - KO: 클러스터의 모든 입력의 감쇠를 조정합니다 (dB).
  - [ ] OK    Fix: 

- **`exportPresets`**
  - EN: Export all 16 LFO presets to an XML file.
  - KO: 16개의 LFO 프리셋을 모두 XML 파일로 내보냅니다.
  - [ ] OK    Fix: 

- **`importPresets`**
  - EN: Import LFO presets from an XML file.
  - KO: XML 파일에서 LFO 프리셋을 가져옵니다.
  - [ ] OK    Fix: 

- **`inputsVisibility`**
  - EN: Show or hide this cluster's inputs on the Map tab. Hide propagates to new members; removing an input restores its visibility.
  - KO: 이 클러스터의 입력을 맵 탭에서 표시하거나 숨깁니다. 숨김은 새 멤버에도 적용되며, 입력을 제거하면 다시 표시됩니다.
  - [ ] OK    Fix: 

- **`lfoActiveButton`**
  - EN: Enable or Disable the Periodic Movement of the Cluster (LFO).
  - KO: 클러스터의 주기적 움직임(LFO)을 활성화 또는 비활성화합니다.
  - [ ] OK    Fix: 

- **`lfoAmplitudeRotSlider`**
  - EN: Maximum Rotation Angle (-360 to 360 Degrees).
  - KO: 최대 회전 각도 (-360~360도).
  - [ ] OK    Fix: 

- **`lfoAmplitudeScaleSlider`**
  - EN: Maximum Scale Factor (0.1x to 10x, Logarithmic).
  - KO: 최대 스케일 계수 (0.1×~10×, 로그).
  - [ ] OK    Fix: 

- **`lfoAmplitudeXSlider`**
  - EN: Width of Movement in Relation to Cluster Reference Position.
  - KO: 클러스터 참조 위치에 대한 너비 움직임 크기.
  - [ ] OK    Fix: 

- **`lfoAmplitudeYSlider`**
  - EN: Depth of Movement in Relation to Cluster Reference Position.
  - KO: 클러스터 참조 위치에 대한 깊이 움직임 크기.
  - [ ] OK    Fix: 

- **`lfoAmplitudeZSlider`**
  - EN: Height of Movement in Relation to Cluster Reference Position.
  - KO: 클러스터 참조 위치에 대한 높이 움직임 크기.
  - [ ] OK    Fix: 

- **`lfoPeriodDial`**
  - EN: Base Period of the Cluster Movement.
  - KO: 클러스터 움직임의 기본 주기입니다.
  - [ ] OK    Fix: 

- **`lfoPhaseDial`**
  - EN: Global Phase Offset of the Cluster Movement.
  - KO: 클러스터 움직임의 전역 위상 오프셋입니다.
  - [ ] OK    Fix: 

- **`lfoPhaseRotDial`**
  - EN: Phase Offset of the Cluster Rotation.
  - KO: 클러스터 회전의 위상 오프셋.
  - [ ] OK    Fix: 

- **`lfoPhaseScaleDial`**
  - EN: Phase Offset of the Cluster Scaling.
  - KO: 클러스터 스케일링의 위상 오프셋.
  - [ ] OK    Fix: 

- **`lfoPhaseXDial`**
  - EN: Phase Offset of the Cluster Movement in Width.
  - KO: 너비에서 클러스터 움직임의 위상 오프셋.
  - [ ] OK    Fix: 

- **`lfoPhaseYDial`**
  - EN: Phase Offset of the Cluster Movement in Depth.
  - KO: 깊이에서 클러스터 움직임의 위상 오프셋.
  - [ ] OK    Fix: 

- **`lfoPhaseZDial`**
  - EN: Phase Offset of the Cluster Movement in Height.
  - KO: 높이에서 클러스터 움직임의 위상 오프셋.
  - [ ] OK    Fix: 

- **`lfoRateRotSlider`**
  - EN: Faster or Slower Rotation in Relation to Base Period.
  - KO: 기본 주기에 대한 빠르거나 느린 회전.
  - [ ] OK    Fix: 

- **`lfoRateScaleSlider`**
  - EN: Faster or Slower Scaling in Relation to Base Period.
  - KO: 기본 주기에 대한 빠르거나 느린 스케일링.
  - [ ] OK    Fix: 

- **`lfoRateXSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Width.
  - KO: 너비에서 기본 주기에 대한 빠르거나 느린 움직임.
  - [ ] OK    Fix: 

- **`lfoRateYSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Depth.
  - KO: 깊이에서 기본 주기에 대한 빠르거나 느린 움직임.
  - [ ] OK    Fix: 

- **`lfoRateZSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Height.
  - KO: 높이에서 기본 주기에 대한 빠르거나 느린 움직임.
  - [ ] OK    Fix: 

- **`lfoShapeRotSelector`**
  - EN: Rotation Behaviour of the Cluster.
  - KO: 클러스터의 회전 동작.
  - [ ] OK    Fix: 

- **`lfoShapeScaleSelector`**
  - EN: Scale Behaviour of the Cluster.
  - KO: 클러스터의 스케일 동작.
  - [ ] OK    Fix: 

- **`lfoShapeXSelector`**
  - EN: Movement Behaviour of the Cluster in Width.
  - KO: 너비에서 클러스터 움직임의 동작.
  - [ ] OK    Fix: 

- **`lfoShapeYSelector`**
  - EN: Movement Behaviour of the Cluster in Depth.
  - KO: 깊이에서 클러스터 움직임의 동작.
  - [ ] OK    Fix: 

- **`lfoShapeZSelector`**
  - EN: Movement Behaviour of the Cluster in Height.
  - KO: 높이에서 클러스터 움직임의 동작.
  - [ ] OK    Fix: 

- **`planeSelector`**
  - EN: Select the plane for rotation and scale operations (XY, XZ, YZ).
  - KO: 회전 및 크기 조정 작업의 평면을 선택합니다 (XY, XZ, YZ).
  - [ ] OK    Fix: 

- **`positionJoystick`**
  - EN: Move all cluster inputs in X/Y. Hold and drag to translate.
  - KO: 클러스터의 모든 입력을 X/Y로 이동합니다. 누른 채 드래그하여 이동합니다.
  - [ ] OK    Fix: 

- **`presetTile`**
  - EN: Click: recall preset. Double-click: recall + start. Middle/right-click: store current LFO.
  - KO: 클릭: 프리셋 불러오기. 더블 클릭: 불러오기 + 시작. 가운데/오른쪽 클릭: 현재 LFO 저장.
  - [ ] OK    Fix: 

- **`qlabPreset`**
  - EN: Create a QLab network cue to recall the last selected LFO preset for the current cluster.
  - KO: 현재 클러스터에서 마지막으로 선택한 LFO 프리셋을 불러오기 위한 QLab 네트워크 큐를 만듭니다.
  - [ ] OK    Fix: 

- **`referenceMode`**
  - EN: Select the reference point for cluster transforms: First Input or Barycenter.
  - KO: 클러스터 변환의 기준점을 선택합니다: 첫 번째 입력 또는 무게 중심.
  - [ ] OK    Fix: 

- **`rotationDial`**
  - EN: Rotate all cluster inputs around the reference point in the selected plane.
  - KO: 선택한 평면에서 기준점을 중심으로 클러스터의 모든 입력을 회전합니다.
  - [ ] OK    Fix: 

- **`scaleJoystick`**
  - EN: Scale cluster inputs relative to the reference point in the selected plane.
  - KO: 선택한 평면에서 기준점을 중심으로 클러스터 입력의 크기를 조정합니다.
  - [ ] OK    Fix: 

- **`stopAllLFO`**
  - EN: Stop LFO on all 10 clusters.
  - KO: 10개 모든 클러스터의 LFO를 중지합니다.
  - [ ] OK    Fix: 

- **`zSlider`**
  - EN: Move all cluster inputs along the Z axis (height).
  - KO: 클러스터의 모든 입력을 Z축(높이)을 따라 이동합니다.
  - [ ] OK    Fix: 

## `clusters.labels`

- **`assignedInputs`**
  - EN: Assigned Inputs
  - KO: 할당된 입력
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Atten
  - KO: 감쇠
  - [ ] OK    Fix: 

- **`controls`**
  - EN: Controls
  - KO: 컨트롤
  - [ ] OK    Fix: 

- **`inputPrefix`**
  - EN: Input
  - KO: 입력
  - [ ] OK    Fix: 

- **`position`**
  - EN: Position
  - KO: 위치
  - [ ] OK    Fix: 

- **`posPrefix`**
  - EN: Pos:
  - KO: 위치:
  - [ ] OK    Fix: 

- **`reference`**
  - EN: Reference:
  - KO: 참조:
  - [ ] OK    Fix: 

- **`rotation`**
  - EN: Rotation
  - KO: 회전
  - [ ] OK    Fix: 

- **`scale`**
  - EN: Scale
  - KO: 스케일
  - [ ] OK    Fix: 

- **`x`**
  - EN: X:
  - KO: X:
  - [ ] OK    Fix: 

- **`y`**
  - EN: Y:
  - KO: Y:
  - [ ] OK    Fix: 

- **`z`**
  - EN: Z
  - KO: Z
  - [ ] OK    Fix: 

## `clusters.lfo`

- **`labels.amplitude`**
  - EN: Amplitude:
  - KO: 진폭:
  - [ ] OK    Fix: 

- **`labels.angle`**
  - EN: Angle:
  - KO: 각도:
  - [ ] OK    Fix: 

- **`labels.period`**
  - EN: Period:
  - KO: 주기:
  - [ ] OK    Fix: 

- **`labels.phase`**
  - EN: Phase:
  - KO: 위상:
  - [ ] OK    Fix: 

- **`labels.rate`**
  - EN: Rate:
  - KO: 속도:
  - [ ] OK    Fix: 

- **`labels.ratio`**
  - EN: Ratio:
  - KO: 비율:
  - [ ] OK    Fix: 

- **`labels.rot`**
  - EN: Rotation
  - KO: 회전
  - [ ] OK    Fix: 

- **`labels.scaleLfo`**
  - EN: Scale
  - KO: 스케일
  - [ ] OK    Fix: 

- **`labels.section`**
  - EN: LFO
  - KO: LFO
  - [ ] OK    Fix: 

- **`labels.x`**
  - EN: X
  - KO: X
  - [ ] OK    Fix: 

- **`labels.y`**
  - EN: Y
  - KO: Y
  - [ ] OK    Fix: 

- **`labels.z`**
  - EN: Z
  - KO: Z
  - [ ] OK    Fix: 

## `clusters.osc`

- **`lfoActive`**
  - EN: /wfs/cluster/lfoActive <id> <0|1>
  - KO: /wfs/cluster/lfoActive <id> <0|1>
  - [ ] OK    Fix: 

- **`lfoAmplitudeRot`**
  - EN: /wfs/cluster/lfoAmplitudeRot <id> <degrees>
  - KO: /wfs/cluster/lfoAmplitudeRot <id> <degrees>
  - [ ] OK    Fix: 

- **`lfoAmplitudeScale`**
  - EN: /wfs/cluster/lfoAmplitudeScale <id> <factor>
  - KO: /wfs/cluster/lfoAmplitudeScale <id> <factor>
  - [ ] OK    Fix: 

- **`lfoAmplitudeX`**
  - EN: /wfs/cluster/lfoAmplitudeX <id> <meters>
  - KO: /wfs/cluster/lfoAmplitudeX <id> <meters>
  - [ ] OK    Fix: 

- **`lfoAmplitudeY`**
  - EN: /wfs/cluster/lfoAmplitudeY <id> <meters>
  - KO: /wfs/cluster/lfoAmplitudeY <id> <meters>
  - [ ] OK    Fix: 

- **`lfoAmplitudeZ`**
  - EN: /wfs/cluster/lfoAmplitudeZ <id> <meters>
  - KO: /wfs/cluster/lfoAmplitudeZ <id> <meters>
  - [ ] OK    Fix: 

- **`lfoPeriod`**
  - EN: /wfs/cluster/lfoPeriod <id> <seconds>
  - KO: /wfs/cluster/lfoPeriod <id> <초>
  - [ ] OK    Fix: 

- **`lfoPhase`**
  - EN: /wfs/cluster/lfoPhase <id> <degrees>
  - KO: /wfs/cluster/lfoPhase <id> <도>
  - [ ] OK    Fix: 

- **`lfoPhaseRot`**
  - EN: /wfs/cluster/lfoPhaseRot <id> <degrees>
  - KO: /wfs/cluster/lfoPhaseRot <id> <degrees>
  - [ ] OK    Fix: 

- **`lfoPhaseScale`**
  - EN: /wfs/cluster/lfoPhaseScale <id> <degrees>
  - KO: /wfs/cluster/lfoPhaseScale <id> <degrees>
  - [ ] OK    Fix: 

- **`lfoPhaseX`**
  - EN: /wfs/cluster/lfoPhaseX <id> <degrees>
  - KO: /wfs/cluster/lfoPhaseX <id> <degrees>
  - [ ] OK    Fix: 

- **`lfoPhaseY`**
  - EN: /wfs/cluster/lfoPhaseY <id> <degrees>
  - KO: /wfs/cluster/lfoPhaseY <id> <degrees>
  - [ ] OK    Fix: 

- **`lfoPhaseZ`**
  - EN: /wfs/cluster/lfoPhaseZ <id> <degrees>
  - KO: /wfs/cluster/lfoPhaseZ <id> <degrees>
  - [ ] OK    Fix: 

- **`lfoPresetRecall`**
  - EN: /wfs/cluster/lfoPresetRecall <clusterId> <presetNumber>
  - KO: /wfs/cluster/lfoPresetRecall <clusterId> <프리셋번호>
  - [ ] OK    Fix: 

- **`lfoRateRot`**
  - EN: /wfs/cluster/lfoRateRot <id> <multiplier>
  - KO: /wfs/cluster/lfoRateRot <id> <multiplier>
  - [ ] OK    Fix: 

- **`lfoRateScale`**
  - EN: /wfs/cluster/lfoRateScale <id> <multiplier>
  - KO: /wfs/cluster/lfoRateScale <id> <multiplier>
  - [ ] OK    Fix: 

- **`lfoRateX`**
  - EN: /wfs/cluster/lfoRateX <id> <multiplier>
  - KO: /wfs/cluster/lfoRateX <id> <multiplier>
  - [ ] OK    Fix: 

- **`lfoRateY`**
  - EN: /wfs/cluster/lfoRateY <id> <multiplier>
  - KO: /wfs/cluster/lfoRateY <id> <multiplier>
  - [ ] OK    Fix: 

- **`lfoRateZ`**
  - EN: /wfs/cluster/lfoRateZ <id> <multiplier>
  - KO: /wfs/cluster/lfoRateZ <id> <multiplier>
  - [ ] OK    Fix: 

- **`lfoShapeRot`**
  - EN: /wfs/cluster/lfoShapeRot <id> <0-8>
  - KO: /wfs/cluster/lfoShapeRot <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoShapeScale`**
  - EN: /wfs/cluster/lfoShapeScale <id> <0-8>
  - KO: /wfs/cluster/lfoShapeScale <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoShapeX`**
  - EN: /wfs/cluster/lfoShapeX <id> <0-8>
  - KO: /wfs/cluster/lfoShapeX <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoShapeY`**
  - EN: /wfs/cluster/lfoShapeY <id> <0-8>
  - KO: /wfs/cluster/lfoShapeY <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoShapeZ`**
  - EN: /wfs/cluster/lfoShapeZ <id> <0-8>
  - KO: /wfs/cluster/lfoShapeZ <id> <0-8>
  - [ ] OK    Fix: 

- **`lfoStopAll`**
  - EN: /wfs/cluster/lfoStopAll
  - KO: /wfs/cluster/lfoStopAll
  - [ ] OK    Fix: 

## `clusters.planes`

- **`xy`**
  - EN: XY
  - KO: XY
  - [ ] OK    Fix: 

- **`xz`**
  - EN: XZ
  - KO: XZ
  - [ ] OK    Fix: 

- **`yz`**
  - EN: YZ
  - KO: YZ
  - [ ] OK    Fix: 

## `clusters.presets`

- **`export`**
  - EN: Export
  - KO: 내보내기
  - [ ] OK    Fix: 

- **`exportDialog`**
  - EN: Export LFO Presets
  - KO: LFO 프리셋 내보내기
  - [ ] OK    Fix: 

- **`exported`**
  - EN: LFO presets exported.
  - KO: LFO 프리셋을 내보냈습니다.
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - KO: 가져오기
  - [ ] OK    Fix: 

- **`importDialog`**
  - EN: Import LFO Presets
  - KO: LFO 프리셋 가져오기
  - [ ] OK    Fix: 

- **`imported`**
  - EN: LFO presets imported.
  - KO: LFO 프리셋을 가져왔습니다.
  - [ ] OK    Fix: 

- **`recalled`**
  - EN: LFO preset recalled from tile {n}.
  - KO: LFO 프리셋을 타일 {n}에서 불러왔습니다.
  - [ ] OK    Fix: 

- **`stopAll`**
  - EN: Stop All
  - KO: 모두 중지
  - [ ] OK    Fix: 

- **`stored`**
  - EN: LFO preset stored in tile {n}.
  - KO: LFO 프리셋이 타일 {n}에 저장되었습니다.
  - [ ] OK    Fix: 

## `clusters.referenceMode`

- **`barycenter`**
  - EN: Barycenter
  - KO: 무게 중심
  - [ ] OK    Fix: 

- **`firstInput`**
  - EN: First Input
  - KO: 첫 번째 입력
  - [ ] OK    Fix: 

## `clusters.status`

- **`noInputs`**
  - EN: No inputs assigned
  - KO: 할당된 입력 없음
  - [ ] OK    Fix: 

- **`trackedMarker`**
  - EN: [T]
  - KO: [T]
  - [ ] OK    Fix: 

- **`tracking`**
  - EN: Tracking: Input {num} (overrides reference)
  - KO: 트래킹: 입력 {num} (참조 무시)
  - [ ] OK    Fix: 

## `clusters.toggles`

- **`inputsHidden`**
  - EN: Inputs: Hidden
  - KO: 입력: 숨김
  - [ ] OK    Fix: 

- **`inputsVisible`**
  - EN: Inputs: Visible
  - KO: 입력: 표시
  - [ ] OK    Fix: 

- **`lfoOff`**
  - EN: L.F.O: OFF
  - KO: L.F.O: 꺼짐
  - [ ] OK    Fix: 

- **`lfoOn`**
  - EN: L.F.O: ON
  - KO: L.F.O: 켜짐
  - [ ] OK    Fix: 

## `common`

- **`cancel`**
  - EN: Cancel
  - KO: 취소
  - [ ] OK    Fix: 

- **`off`**
  - EN: OFF
  - KO: 끄기
  - [ ] OK    Fix: 

- **`ok`**
  - EN: OK
  - KO: 확인
  - [ ] OK    Fix: 

- **`on`**
  - EN: ON
  - KO: 켜기
  - [ ] OK    Fix: 

## `eq.buttons`

- **`flattenEQ`**
  - EN: Flatten EQ
  - KO: EQ 초기화
  - [ ] OK    Fix: 

- **`resetBand`**
  - EN: Reset
  - KO: 초기화
  - [ ] OK    Fix: 

## `eq.filterTypes`

- **`allPass`**
  - EN: All-Pass
  - KO: 올패스
  - [ ] OK    Fix: 

- **`bandPass`**
  - EN: Band Pass
  - KO: 밴드패스
  - [ ] OK    Fix: 

- **`highCut`**
  - EN: High Cut
  - KO: 하이 컷
  - [ ] OK    Fix: 

- **`highShelf`**
  - EN: High Shelf
  - KO: 하이 쉘프
  - [ ] OK    Fix: 

- **`lowCut`**
  - EN: Low Cut
  - KO: 로우 컷
  - [ ] OK    Fix: 

- **`lowShelf`**
  - EN: Low Shelf
  - KO: 로우 쉘프
  - [ ] OK    Fix: 

- **`peakNotch`**
  - EN: Peak/Notch
  - KO: 피크/노치
  - [ ] OK    Fix: 

## `eq.labels`

- **`band`**
  - EN: Band
  - KO: 밴드
  - [ ] OK    Fix: 

- **`freq`**
  - EN: Freq:
  - KO: 주파수:
  - [ ] OK    Fix: 

- **`gain`**
  - EN: Gain
  - KO: 게인
  - [ ] OK    Fix: 

- **`q`**
  - EN: Q
  - KO: Q
  - [ ] OK    Fix: 

## `eq.status`

- **`off`**
  - EN: EQ OFF
  - KO: EQ 끄기
  - [ ] OK    Fix: 

- **`on`**
  - EN: EQ ON
  - KO: EQ 켜기
  - [ ] OK    Fix: 

## `fileManager.dialogs`

- **`selectProjectFolder`**
  - EN: Select Project Folder
  - KO: 프로젝트 폴더 선택
  - [ ] OK    Fix: 

## `fileManager.errors`

- **`backupNotFound`**
  - EN: Backup not found
  - KO: 백업을 찾을 수 없습니다
  - [ ] OK    Fix: 

- **`configStateInvalid`**
  - EN: Config state is invalid
  - KO: 구성 상태가 유효하지 않습니다
  - [ ] OK    Fix: 

- **`failedApply`**
  - EN: Failed to apply: {sections}
  - KO: 적용 실패: {sections}
  - [ ] OK    Fix: 

- **`failedCreateFolder`**
  - EN: Failed to create project folder: {path}
  - KO: 프로젝트 폴더 생성 실패: {path}
  - [ ] OK    Fix: 

- **`failedCreateValueTree`**
  - EN: Failed to create ValueTree from XML: {path}
  - KO: XML에서 ValueTree를 생성하지 못했습니다: {path}
  - [ ] OK    Fix: 

- **`failedCreateXML`**
  - EN: Failed to create XML from state
  - KO: 상태에서 XML을 생성하지 못했습니다
  - [ ] OK    Fix: 

- **`failedParseXML`**
  - EN: Failed to parse XML file: {path}
  - KO: XML 파일 분석 실패: {path}
  - [ ] OK    Fix: 

- **`failedWriteFile`**
  - EN: Failed to write file: {path}
  - KO: 파일 쓰기 실패: {path}
  - [ ] OK    Fix: 

- **`fileNotFound`**
  - EN: File not found: {path}
  - KO: 파일을 찾을 수 없습니다: {path}
  - [ ] OK    Fix: 

- **`invalidConfigStructure`**
  - EN: Invalid configuration file structure
  - KO: 구성 파일 구조가 잘못되었습니다
  - [ ] OK    Fix: 

- **`noInputDataInFile`**
  - EN: No input data found in file
  - KO: 파일에서 입력 데이터를 찾을 수 없습니다
  - [ ] OK    Fix: 

- **`noInputDataInSnapshot`**
  - EN: No input data in snapshot
  - KO: 스냅샷에 입력 데이터가 없습니다
  - [ ] OK    Fix: 

- **`noLFOPresetDataInFile`**
  - EN: No LFO preset data found in file
  - KO: 파일에서 LFO 프리셋 데이터를 찾을 수 없습니다
  - [ ] OK    Fix: 

- **`noNetworkDataInFile`**
  - EN: No network data found in file
  - KO: 파일에서 네트워크 데이터를 찾을 수 없습니다
  - [ ] OK    Fix: 

- **`noNetworkSections`**
  - EN: No network sections found in file
  - KO: 파일에서 네트워크 섹션을 찾을 수 없습니다
  - [ ] OK    Fix: 

- **`noOutputDataInFile`**
  - EN: No output data found in file
  - KO: 파일에서 출력 데이터를 찾을 수 없습니다
  - [ ] OK    Fix: 

- **`noProjectFolder`**
  - EN: No project folder specified
  - KO: 프로젝트 폴더가 지정되지 않았습니다
  - [ ] OK    Fix: 

- **`noReverbDataInFile`**
  - EN: No reverb data found in file
  - KO: 파일에서 리버브 데이터를 찾을 수 없습니다
  - [ ] OK    Fix: 

- **`noSystemDataInFile`**
  - EN: No valid system data found in file: {path}
  - KO: 파일에서 유효한 시스템 데이터를 찾을 수 없습니다: {path}
  - [ ] OK    Fix: 

- **`noValidProjectFolder`**
  - EN: No valid project folder
  - KO: 유효한 프로젝트 폴더가 없습니다
  - [ ] OK    Fix: 

- **`prefixInputs`**
  - EN: Inputs: 
  - KO: 입력: 
  - [ ] OK    Fix: 

- **`prefixNetwork`**
  - EN: Network: 
  - KO: 네트워크: 
  - [ ] OK    Fix: 

- **`prefixOutputs`**
  - EN: Outputs: 
  - KO: 출력: 
  - [ ] OK    Fix: 

- **`prefixReverbs`**
  - EN: Reverbs: 
  - KO: 리버브: 
  - [ ] OK    Fix: 

- **`prefixSystem`**
  - EN: System: 
  - KO: 시스템: 
  - [ ] OK    Fix: 

- **`snapshotNotFound`**
  - EN: Snapshot not found
  - KO: 스냅샷을 찾을 수 없습니다
  - [ ] OK    Fix: 

- **`snapshotNotFoundNamed`**
  - EN: Snapshot not found: {name}
  - KO: 스냅샷을 찾을 수 없습니다: {name}
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
  - KO: ADM-OSC는 공간 음향의 상호 운용성을 향상시키기 위한 프로토콜입니다. 콘솔이나 DAW의 오토메이션 커브에서 직교 좌표(X, Y, Z) 또는 극좌표 값(AED: 방위각, 고도, 거리)을 전송합니다.
데이터는 정규화되어 전송됩니다:
- X, Y, Z는 -1.0에서 1.0 사이
- 거리는 0.0에서 1.0 사이
- 방위각은 -180°에서 180° 사이
- 고도는 -90°에서 90° 사이
원점을 이동할 수 있으며 매핑은 스테이지의 안쪽과 바깥쪽에 대해 다른 세그먼트로 조정할 수 있습니다.
그래프의 핸들을 드래그할 때 Shift 키를 누르면 반대쪽에 대칭 조정이 적용됩니다.
  - [ ] OK    Fix: 

- **`title`**
  - EN: ADM-OSC Mappings
  - KO: ADM-OSC 매핑
  - [ ] OK    Fix: 

## `help.automOtion`

- **`body`**
  - EN: One time movement can also be programmed and triggered manually or triggered by sound level.
The coordinates are either relative from the start position or absolute relative to the origin point.
The input can either stay at the end position or revert to the starting position.
Input position cannot be changed while the input is moving this way, but interacting with the input will change the position offset of the input instead.
For audio level triggering select the sound level above which the movement will start. When the sound drops below the reset level the movement will be rearmed, ready to be triggered again. New movement automation will not be possible while the input is moving.
  - KO: 일회성 움직임을 프로그래밍하고 수동 또는 사운드 레벨로 트리거할 수 있습니다.
좌표는 시작 위치에서의 상대값 또는 원점에 대한 절대값입니다.
입력은 끝 위치에 머물거나 시작 위치로 돌아갈 수 있습니다.
이동 중에는 위치를 변경할 수 없지만, 입력과의 상호작용은 위치 오프셋을 변경합니다.
오디오 레벨 트리거의 경우 움직임이 시작될 임계값을 선택합니다. 사운드가 리셋 레벨 아래로 떨어지면 움직임이 재준비됩니다.
  - [ ] OK    Fix: 

- **`title`**
  - EN: AutomOtion
  - KO: AutomOtion
  - [ ] OK    Fix: 

## `help.binaural`

- **`body`**
  - EN: The Binaural renderer is used for:
- listening to a rough spatial mix on headphones,
- creating a mix for stereo output,
- listening to a single soloed track through the spatial processing.
This may take the place of your master mix if it's only feeding headphones and media mix.
The position of the listening position may be adjusted in depth from the origin point and in orientation. Delay and level settings allow you to eventually match the sound at the FOH position.
  - KO: 바이노럴 렌더러 용도:
- 헤드폰으로 대략적인 공간 믹스 청취
- 스테레오 출력용 믹스 생성
- 솔로 트랙을 공간 처리로 청취
헤드폰과 미디어 믹스만 사용하는 경우 마스터 믹스를 대체할 수 있습니다.
청취 위치는 원점에서의 깊이와 방향으로 조정할 수 있습니다. 딜레이와 레벨 설정으로 FOH 위치의 사운드에 맞출 수 있습니다.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Binaural Renderer
  - KO: 바이노럴 렌더러
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
  - KO: 클러스터는 전체적으로 조작하고 애니메이션할 수 있는 입력 그룹입니다.
각 입력은 하나의 클러스터에만 속할 수 있습니다.
각 클러스터에는 트래킹이 완전히 활성화된 입력이 하나만 있을 수 있으며, 이 입력이 기준점이 됩니다.
트래킹 입력이 없으면 기준점에 두 가지 모드가 있습니다: 목록의 첫 번째 입력 또는 할당된 입력의 무게중심입니다.
기준점을 드래그하여 모든 입력을 이동할 수 있습니다. 개별 입력도 별도로 조정 가능합니다. 트래킹이 활성화되고 기준점인 입력을 드래그하면 위치 오프셋과 클러스터의 다른 입력 위치가 정상적으로 영향을 받습니다.
클러스터의 모든 입력을 기준점을 중심으로 회전하거나 크기를 조정할 수 있습니다.
모든 클러스터에 LFO 애니메이션을 할당할 수 있습니다. X, Y, Z 위치, 회전, 스케일을 제어할 수 있습니다. LFO 설정을 패드에 할당하여 빠르게 불러올 수 있습니다. 오른쪽 클릭으로 LFO 매개변수를 패드에 저장합니다. 패드 상단을 더블클릭하면 프리셋 이름을 편집할 수 있습니다. 패드를 클릭하거나 탭하면 LFO 실행 여부와 관계없이 설정을 불러오지만, 중지된 LFO를 시작하지는 않습니다. 더블클릭/탭으로 LFO를 로드하고 시작합니다.
모든 클러스터는 같은 LFO 프리셋 세트를 공유합니다.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Clusters
  - KO: 클러스터
  - [ ] OK    Fix: 

## `help.floorReflections`

- **`body`**
  - EN: Simulating the floor reflections improves the naturalness of the sound. We don't expect sounds to play in a soundproof anechoic chamber. This setting helps recreate the expected floor reflections.
The level of the floor reflections can be adjusted as well as the low cut and the high frequency shelf filters. Diffusion adds a bit of chaos to simulate the unevenness of the floor.
  - KO: 바닥 반사를 시뮬레이션하면 소리의 자연스러움이 향상됩니다. 방음 무향실에서 소리가 재생되는 것을 기대하지 않습니다. 이 설정은 예상되는 바닥 반사를 재현하는 데 도움이 됩니다.
바닥 반사의 레벨은 로우컷 필터 및 고주파 쉘프 필터와 함께 조정할 수 있습니다. 디퓨전은 바닥의 불균일함을 시뮬레이션하기 위해 약간의 카오스를 추가합니다.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Floor Reflections
  - KO: 바닥 반사
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
  - KO: 그래디언트 맵은 X, Y 위치에 따라 감쇠, 높이, 고주파 필터링(1kHz 중심의 부드러운 슬로프 쉘프)을 제어할 수 있습니다. 예를 들어 특정 구역에 진입할 때 소리를 페이드아웃하거나, 무대 전면에서 멀어질 때 고주파 롤오프를 적용하거나, 높은 플랫폼 위의 배우 높이를 자동 조정할 수 있습니다.
감쇠, 높이, HF 쉘프의 3개 레이어가 있습니다. 켜기/끄기 전환과 숨기기가 가능합니다.
각 레이어에는 효과 범위를 조정하는 흰색/검정 매핑 컨트롤이 있습니다. 커브 설정으로 전환을 조정합니다.
각 레이어에 편집 가능한 도형(사각형, 타원, 다각형)을 배치할 수 있으며, 단색 회색, 선형 그래디언트, 방사형 그래디언트를 사용할 수 있습니다.
다각형 생성 시 각 모서리를 클릭하고 더블클릭으로 닫습니다.
기존 점을 더블클릭하면 삭제, 변을 더블클릭하면 새 점이 추가됩니다.
도형과 레이어는 다른 레이어로 복사할 수 있습니다.
설정은 입력 파일에 저장됩니다.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Gradient Maps
  - KO: 그래디언트 맵
  - [ ] OK    Fix: 

## `help.inputAdvanced`

- **`body`**
  - EN: - Sidelines and Fringe enable muting when an input comes close to the limits of a rectangular stage (except downstage).
- Tracking can be enabled and the ID of the tracker can be selected. The amount of smoothing on the position can also be adjusted.
- Maximum Speed can be engaged and the speed limit adjusted. The system will apply a gradual acceleration and deceleration when the movement starts and finishes. When Path is enabled, the system will track the path followed by the input and will not go in a straight line to the final position. This is especially handy if movements have to be operated manually.
- Height Factor lets you work in 2D, when set to 0%, or full 3D, when set at 100%, and anything in between. It's the ratio of the height in the level and delay calculations. If you wish to use floor reflections, set it to 100% and use the parallax correction of the output parameters.
  - KO: - 사이드라인과 프린지는 입력이 직사각형 무대의 경계에 접근할 때 음소거를 가능하게 합니다 (관객 측 제외).
- 트래킹을 활성화하고 트래커 ID를 선택할 수 있습니다. 위치 평활화도 조정할 수 있습니다.
- 최대 속도를 적용하고 속도 제한을 조정할 수 있습니다. 시스템은 움직임 시작과 끝에서 점진적인 가속과 감속을 적용합니다. 경로 모드가 활성화되면 시스템은 입력이 취한 경로를 따르고 최종 위치까지 직선으로 이동하지 않습니다. 움직임을 수동으로 조작해야 하는 경우 특히 유용합니다.
- 높이 계수는 0%로 설정될 때 2D, 100%로 설정될 때 완전한 3D 및 그 사이의 모든 값으로 작업할 수 있게 합니다. 이는 레벨 및 지연 계산에서 높이의 비율입니다. 바닥 반사를 사용하려면 100%로 설정하고 출력 매개변수의 시차 보정을 사용하세요.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Advanced Controls
  - KO: 고급 제어
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
  - KO: 인풋에는 사실적인 음향 보강이나 사운드 디자인을 위한 다양한 설정이 있습니다.
- 인풋 레벨을 조정할 수 있습니다.
- 인풋을 지연시키거나 특정 레이턴시(무선 전송 또는 디지털 이펙트의 디지털 처리)를 고려하여 보상할 수 있습니다.
- 음향 선행 대신 최소 레이턴시를 활성화할 수 있습니다. 시스템이 가능한 빨리 소리를 출력하려고 시도합니다.
- 위치(위치 및 오프셋)는 무대 형태나 다른 채널에 관계없이 직교, 원통 또는 구면 좌표로 지정할 수 있습니다.
- 위치는 직교 좌표에서는 무대 크기로, 극좌표에서는 특정 거리 범위로 제한할 수 있습니다.
- 플립은 원점을 중심으로 대칭 위치를 취합니다.
- 조이스틱과 수직 슬라이더로 위치의 상대적 제어가 가능합니다.
- 인풋을 클러스터에 할당하여 조정된 움직임을 위해 그룹화할 수 있습니다.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Inputs Basic Parameters
  - KO: 인풋 기본 파라미터
  - [ ] OK    Fix: 

## `help.inputHF`

- **`body`**
  - EN: When talking away, the timbre of a voice will sound less bright. Reproducing this was the initial goal here even though we usually want to have support for voices when not addressing the audience or in bi-frontal configurations. This can be put to good use for creative effects such as having a brighter reverb on a dampened direct sound.
The orientation of the input in azimuth and in pitch can be set as well as the angle where the high frequencies will not be filtered.
The HF Shelf will set the maximum attenuation in the back of the input. There is a smooth fade (like a cosine curve) from full brightness in front to damped at the rear.
  - KO: 말하는 사람이 돌아서면 목소리의 음색이 덜 밝게 들립니다. 이를 재현하는 것이 원래의 목표였으며, 관객을 향하지 않는 목소리나 양면 구성에서도 지원이 필요합니다. 감쇠된 직접음에 대해 더 밝은 리버브를 갖는 것과 같은 창의적 효과에 활용할 수 있습니다.
인풋의 방위각과 앙각의 방향, 그리고 고주파가 필터링되지 않는 각도를 설정할 수 있습니다.
HF 셸프는 인풋 뒤쪽의 최대 감쇠를 설정합니다. 앞의 완전한 밝기에서 뒤의 감쇠까지 부드러운 페이드(코사인 커브와 같은)가 있습니다.
  - [ ] OK    Fix: 

- **`title`**
  - EN: High Frequency Directivity
  - KO: 고주파 지향성
  - [ ] OK    Fix: 

## `help.inputLevel`

- **`body`**
  - EN: There are two level attenuation models. One where the level decreases with the distance by a given ratio in dB/m. Otherwise the level is halved each time the distance doubles. This might be more realistic, but it may be too loud close to the source or not give enough focus. The former may be less physically accurate, it offers usually better control to have a more even and stable mix.
  - KO: 두 가지 레벨 감쇠 모델이 있습니다. 하나는 거리에 따라 dB/m의 주어진 비율로 레벨이 감소하는 것입니다. 다른 하나는 거리가 두 배가 될 때마다 레벨이 절반으로 줄어드는 것입니다. 후자가 더 사실적일 수 있지만, 음원 가까이에서 너무 크거나 충분한 포커스를 제공하지 못할 수 있습니다. 전자는 물리적으로 덜 정확할 수 있지만, 일반적으로 더 균일하고 안정적인 믹스를 위한 더 나은 제어를 제공합니다.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Level Adjustments
  - KO: 레벨 조정
  - [ ] OK    Fix: 

## `help.inputMutes`

- **`body`**
  - EN: You can set for each output array a specific attenuation for the selected input.
You can mute each send to any output individually. There are macros to speed up the process.
  - KO: 각 출력 어레이에 대해 선택된 인풋의 특정 감쇠를 설정할 수 있습니다.
각 센드를 모든 출력에 대해 개별적으로 뮤트할 수 있습니다. 프로세스를 빠르게 하는 매크로가 있습니다.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Array Attenuation and Output Mutes
  - KO: 어레이 감쇠 및 출력 뮤트
  - [ ] OK    Fix: 

## `help.lfo`

- **`body`**
  - EN: Input position can be automated. The LFO can control X, Y and Z position coordinates individually as well as the rotation of HF directivity (gyrophone).
Adjust the global period and phase for the LFO.
For X, Y and Z coordinates select a shape, amplitude, rate and phase as desired. A circle in the XY plane would have sine for shape for X and for Y and an offset of ±90° between the two. A square would be the same but with keystone shapes.
Input position can be moved while the LFO is running.
  - KO: 입력 위치를 자동화할 수 있습니다. LFO는 X, Y, Z 좌표를 개별적으로 제어하고 HF 지향성의 회전(자이로폰)도 제어할 수 있습니다.
LFO의 전체 주기와 위상을 조정합니다.
X, Y, Z 좌표에 대해 형태, 진폭, 속도, 위상을 선택합니다. XY 평면의 원은 X와 Y에 사인파를 사용하고 ±90° 오프셋을 설정합니다.
LFO 작동 중에도 입력 위치를 이동할 수 있습니다.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Input LFO
  - KO: 입력 LFO
  - [ ] OK    Fix: 

## `help.liveSource`

- **`body`**
  - EN: A loud source on stage may not need much reinforcement through the nearby speakers. Imagine an opera singer near the edge of the stage. Normally the level distribution would make the level louder near the input position. But if it is already loud enough we should be able to not over-amplify it. This function manages this.
The radius and shape describe how to attenuate the level for a specific source for speakers within the radius of influence of this input. There are various shapes: a V shaped linear effect; a U shape for fast decrease within the radius; a tight V effect or a mix of the previous (sine).
The attenuation can be constant or level dependent like a local compression reacting to transients and to average RMS level.
  - KO: 무대 위의 큰 소리 소스는 근처 스피커를 통한 보강이 필요 없을 수 있습니다. 무대 가장자리의 오페라 가수를 상상해 보세요. 일반적으로 레벨 분배는 입력 위치 근처에서 더 크게 만듭니다. 하지만 이미 충분히 크다면 과도하게 증폭하지 않아야 합니다. 이 기능이 이를 관리합니다.
반경과 형태는 이 입력의 영향 반경 내 스피커에 대한 레벨 감쇠 방법을 설명합니다. 다양한 형태가 있습니다: V자형 선형 효과, 빠른 감소를 위한 U자형, 좁은 V자형 또는 이전 것들의 혼합(사인).
감쇠는 일정하거나 과도 응답과 평균 RMS 레벨에 반응하는 로컬 압축처럼 레벨 의존적일 수 있습니다.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Live Source Tamer
  - KO: 라이브 소스 테이머
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
  - KO: - 입력이나 클러스터를 왼쪽 클릭하여 드래그로 이동합니다.
- Shift+왼쪽 클릭으로 선택에 추가/제거합니다.
- 왼쪽 클릭 드래그로 선택 사각형을 그립니다.
- 더블클릭으로 위치 오프셋을 리셋합니다.
- 움직임 없는 긴 클릭으로 입력 탭으로 전환합니다.
- 입력 외부 클릭으로 선택을 해제합니다.
- 오른쪽 클릭 드래그로 맵을 팬합니다. 두 손가락 드래그도 동일합니다.
- 마우스 휠로 줌합니다. 두 손가락 핀치도 동일합니다.
- 가운데 클릭으로 뷰를 리셋합니다.
- 화살표 키로 X/Y 이동, PageUp/Down으로 높이 조정.
- 두 번째 손가락으로 지향성 회전과 높이 조정이 가능합니다.
- 클러스터에서는 두 번째 손가락으로 회전과 크기 조정이 가능합니다.
- 입력, 출력 배열, 리버브 노드를 숨길 수 있습니다.
- 입력을 잠가 선택과 이동을 방지할 수 있습니다.
- 리버브 노드를 이동 가능. Ctrl/Cmd로 쌍을 대칭 이동합니다.
- 라이브 소스 테이머 반경이 표시됩니다.
- 오디오 레벨을 맵에 표시할 수 있습니다.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Map
  - KO: 맵
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
  - KO: MCP 서버는 AI 어시스턴트(Claude Desktop, Claude Code, 사용자 정의 커넥터가 있는 ChatGPT)가 로컬 네트워크 연결을 통해 이 WFS-DIY 세션의 매개변수를 읽고 쓸 수 있게 해줍니다.

AI가 할 수 있는 일:
• 라이브 상태 읽기: 채널 수, 이름, 위치, 감쇠, EQ, 스냅샷, 클러스터, 전체 매개변수.
• 소스 이동, 채널 이름 변경, 클러스터 할당 설정, 어레이 레이아웃 조정, 출력 및 리버브 배치.
• 준비된 프롬프트 템플릿을 통해 가이드 워크플로 실행 (시스템 튜닝, 위치 문제 해결, 스냅샷 관리).

이 행의 운영자 컨트롤:
• AI: ON / OFF — 메인 스위치. OFF일 때 모든 AI 도구 호출이 거부됩니다. ON일 때 AI는 아래 규칙에 따라 작동합니다.
• 중요 AI 동작: 차단됨 / 허용됨 — 파괴적인 동작(스냅샷 삭제, DSP 재설정, 채널 수 변경)은 기본적으로 차단됩니다. 클릭하여 10분 동안 허용. 빨간색 채우기는 윈도우 만료에 따라 줄어들고 자동으로 다시 차단됩니다.
• AI 기록 열기 — 최근 모든 AI 변경의 스크롤 가능한 타임라인.
• MCP URL 버튼은 URL을 직접 받는 AI 클라이언트용으로 서버 URL을 클립보드에 복사합니다.

운영자 인지:
• 모든 AI 동작은 출처 태그와 함께 기록됩니다. AI 기록 창은 전체 타임라인을 보여줍니다. 행별 ×는 종속성을 포함하여 동작을 되돌립니다.
• AI가 방금 이동한 매개변수를 수동으로 조정하면 AI에 알림이 가고 무작정 재시도하지 않습니다. 항상 마지막 결정은 당신이 합니다.
• Cmd/Ctrl+Alt+Z 및 Cmd/Ctrl+Alt+Y 단축키는 수동 편집(일반 Ctrl+Z 사용)에 영향을 주지 않고 마지막 AI 변경을 실행 취소/다시 실행합니다.

이 서버를 Claude Desktop에 추가하려면:
  1. 설정 → 개발자 → 구성 편집을 엽니다.
  2. 아래 JSON 스니펫을 claude_desktop_config.json에 붙여넣습니다 (이미 있는 mcpServers 블록에 병합).
  3. Claude Desktop을 다시 시작합니다. 서버가 도구 메뉴에 'wfs-diy'로 표시됩니다.

Claude Code에 추가하려면 실행:
  claude mcp add wfs-diy <url> -t http

네트워크 인터페이스를 전환하거나 서버가 다른 포트로 폴백하면 URL이 변경됩니다. 이 행의 URL 버튼은 항상 라이브 URL을 반영합니다.
  - [ ] OK    Fix: 

- **`copyButton`**
  - EN: Copy Config
  - KO: 구성 복사
  - [ ] OK    Fix: 

- **`copyConfirm`**
  - EN: MCP config JSON copied to clipboard
  - KO: MCP 구성 JSON이 클립보드에 복사됨
  - [ ] OK    Fix: 

- **`title`**
  - EN: AI / MCP Server
  - KO: AI / MCP 서버
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
  - KO: 시스템은 OSC를 사용하여 여러 네트워크 프로토콜(UDP 또는 TCP)로 통신할 수 있습니다. OSC Query를 활성화하면 클라이언트가 사용 가능한 OSC 경로를 검색하고 매개변수 업데이트를 구독할 수 있습니다.
선택한 네트워크 인터페이스에 해당하는 로컬 머신 IP가 표시됩니다. 수신 TCP, UDP 포트 및 OSC Query 포트.
전문 OSC 클라이언트:
- Remote: 멀티터치 작동 및 원격 제어용 Android 애플리케이션.
- QLab: 데이터 전송 및 애플리케이션에서 직접 프로그래밍.
- ADM-OSC: 콘솔 및 DAW에서 제어(전용 도움말 참조).
데이터 필터링, 로그 창, 분실된 Android 태블릿 위치 찾기 기능이 있습니다.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Network
  - KO: 네트워크
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
  - KO: 음향 사운드에 맞게 조정하기 위한 몇 가지 파라미터가 있습니다.
이러한 파라미터의 대부분은 전체 어레이에 설정됩니다(전파 모드가 이 출력에서 꺼져 있는 경우 제외). 특정 설정 후 상대적 변경도 선택할 수 있습니다.
- 방향 및 On/Off 각도는 각 스피커가 어떤 인풋을 증폭할지 정의합니다. 기본적으로 스피커는 관객을 향하고 무대에서 벗어납니다. 녹색 영역의 인풋은 증폭되지만, 빨간 영역의 인풋은 증폭되지 않습니다. 서브베이스의 경우 최대로 열면 모든 인풋을 포함할 수 있습니다.
- HF 댐핑은 거리에 따른 고주파 손실을 시뮬레이션합니다.
- 거리 감쇠 퍼센티지는 계산된 감쇠의 적용 정도를 정의합니다. 서브베이스의 경우 50%로 낮추는 것이 현명할 수 있습니다.
- 최소 레이턴시는 최소 계산 딜레이의 스캔을 활성화합니다.
- 라이브 소스 감쇠는 가까운 인풋의 레벨 감소를 활성화합니다.
- 바닥 반사는 서브베이스 및 플라잉 어레이와 같은 이 출력에 반사가 적용되는지 전환합니다...
  - [ ] OK    Fix: 

- **`title`**
  - EN: Advanced Parameters
  - KO: 고급 파라미터
  - [ ] OK    Fix: 

## `help.outputs`

- **`body`**
  - EN: The design of the WFS system has to do with the right choice of equipment and their position. Here is a guide to help you with the design and the tuning of your arrays.
An array is a line (straight or curved) of speakers. This is one of the most important concepts in WFS adapted to sound reinforcement and for creative sound design.
A rule of thumb would be that each listener should hear three speakers of an array to have enough psycho-acoustic cues to feel the direction each sound is coming from. There will be a sweet spot to find between the distance between the speakers and the listeners, their spacing and coverage angle. This is especially true for short range arrays, a.k.a front fills. Speakers with a 120° coverage angle can be spaced by the same distance between the array and the first row of listeners. The number of speakers also depends on the sound pressure level. When placed as a flown array, trapezoidal/asymmetric horns with wide coverage angle (120°) below axis and narrow coverage (60°) on axis will give good coverage for seat rows a bit too far from the ground array and will have enough throw to reach 20m or 30m away while avoiding the walls of the venue where they would create reflections that give away the position of the speakers to the ears of the listeners. Most of the time for larger venues coaxial (elliptical or conical horns) have not enough reach and require one or more delay lines. They are more suited for smaller venues with few rows of seats.
The positioning of the speakers in the system can be done through the 'Wizard of OutZ' and the editable presets it has.
  - KO: WFS 시스템의 설계는 올바른 장비 선택과 배치에 관한 것입니다. 어레이 설계 및 튜닝을 위한 가이드입니다.
어레이는 스피커의 열(직선 또는 곡선)입니다. 이것은 WFS에서 가장 중요한 개념 중 하나입니다.
경험적으로 각 청취자는 방향을 감지하기 위한 충분한 심리음향 단서를 얻기 위해 어레이의 스피커 3개를 들어야 합니다. 스피커와 청취자 사이의 거리, 간격, 커버리지 각도 사이에 최적점이 있습니다. 120° 커버리지 각도의 스피커는 어레이와 첫 번째 줄 사이의 거리와 같은 간격으로 배치할 수 있습니다. 스피커 수는 음압 레벨에도 의존합니다. 플라잉 어레이로 배치할 경우, 축 아래 넓은(120°), 축 위 좁은(60°) 커버리지의 사다리꼴/비대칭 혼은 좋은 커버리지와 20-30m 도달 거리를 제공하면서 벽 반사를 피합니다. 동축 스피커는 대형 장소에서는 보통 도달 거리가 부족하여 딜레이 라인이 필요합니다.
배치는 'Wizard of OutZ'와 편집 가능한 프리셋을 통해 수행할 수 있습니다.
  - [ ] OK    Fix: 

- **`title`**
  - EN: WFS Array Design
  - KO: WFS 어레이 설계
  - [ ] OK    Fix: 

## `help.overview`

- **`body`**
  - EN: This WFS spatial sound processor aims to be a tool for natural sound reinforcement and also a creative tool opening new avenues for writing sound in space.
Some parameters are straightforward: place sound (Map, Tracking, Speed Limiting, Gradient Maps...), work its shape (Attenuation Profile) and its acoustic presence (Directivity, Floor Reflections), give it a one-off motion (AutomOtion) or a repetitive movement (L.F.O). In some cases amplification should be limited around loud sources on stage (Live Source Tamer). All of these functionalities can be stored and recalled internally or with the help of QLab. On the other side the system allows real-time interaction to trigger and move samples, move large clusters of inputs manually or thanks to easily recallable LFO presets.
  - KO: 이 WFS 공간 음향 프로세서는 자연스러운 음향 보강 도구이자 공간에 소리를 쓰는 새로운 길을 여는 창의적 도구입니다.
일부 파라미터는 직관적입니다: 소리 배치(맵, 트래킹, 속도 제한, 그래디언트 맵...), 형태 가공(감쇠 프로파일), 음향적 존재감 조정(지향성, 바닥 반사), 일회성 움직임(AutomOtion) 또는 반복 움직임(L.F.O) 부여. 경우에 따라 무대 위 큰 음원 주변의 증폭을 제한해야 합니다(Live Source Tamer). 이 모든 기능은 내부적으로 또는 QLab의 도움으로 저장 및 호출할 수 있습니다. 또한 시스템은 실시간 상호작용을 통해 샘플을 트리거하고 이동하며, 대규모 인풋 클러스터를 수동으로 또는 LFO 프리셋으로 이동할 수 있게 합니다.
  - [ ] OK    Fix: 

- **`dontShow`**
  - EN: Don't show this again
  - KO: 다시 표시하지 않음
  - [ ] OK    Fix: 

- **`title`**
  - EN: System Overview
  - KO: 시스템 개요
  - [ ] OK    Fix: 

## `help.parallax`

- **`body`**
  - EN: Each speaker points more or less clearly at a listener. To calculate the delay for an input for each speaker, let's consider the distance from the input to this listener, we can also calculate the distance the sound from the speaker to this listener. To match the time of arrival of both we need to apply the difference in the aforementioned distances as a delay for this input and this speaker. This gives greater stability when inputs are moved on stage and especially when they move upstage, away from the edge of the stage. This also can enable the synthesis of floor reflections. This setting can be tuned for finer adjustments, rather than simply measured. Use your ears!
  - KO: 각 스피커는 청취자를 향하고 있습니다. 각 스피커의 인풋에 대한 딜레이를 계산하기 위해, 인풋에서 청취자까지의 거리를 고려하고 스피커에서 청취자까지의 소리 거리도 계산합니다. 두 가지의 도착 시간을 맞추기 위해 거리 차이를 딜레이로 적용해야 합니다. 이것은 무대에서 인풋을 이동할 때, 특히 무대 가장자리에서 멀어질 때 더 큰 안정성을 제공합니다. 바닥 반사 합성도 가능하게 합니다. 이 설정은 단순히 측정하는 것이 아니라 미세 조정할 수 있습니다. 귀를 믿으세요!
  - [ ] OK    Fix: 

- **`title`**
  - EN: Parallax Correction
  - KO: 시차 보정
  - [ ] OK    Fix: 

## `help.reverb`

- **`body`**
  - EN: Reverb helps blur the actual reflections of the speakers in the acoustic space. Without some reverb the listener might perceive the sound missing some depth because the reflections of the speakers in the acoustic of the room gives away their positions and all sounds might feel as coming from the plane of the speakers.
Place the reverb node according to the number of channels and the geometry of the stage and listening area, and the positions of speakers. If necessary the return position can be offset from the feed position.
Other parameters are very similar to Outputs' and Inputs'.
  - KO: 리버브는 스피커의 실제 반사를 흐리게 하는 데 도움이 됩니다.
채널 수와 무대 형상에 따라 리버브 노드를 배치합니다.
기타 매개변수는 출력 및 입력과 유사합니다.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb
  - KO: 리버브
  - [ ] OK    Fix: 

## `help.reverbAlgo`

- **`body`**
  - EN: There are three built-in reverb types inside this spatial sound processor:
- SDN (Scattered Delay Network): The sound bounces between every reverb node which act as reflecting surfaces. This reverb algorithm favours an odd number of nodes that should not have too much symmetry. This helps reduce the artifacts of the reverb or any metallic sounding resonance.
- FDN (Feedback Delay Network): Each reverb node functions as a separate reverb processor with a classical reverb algorithm. Place nodes around the stage and eventually around the audience. Inputs are sent to each node in a similar fashion as with outputs. The return signal from the reverb is then distributed like other inputs to all outputs.
- IR (Impulse Response): This is a classical convolution reverb. You can load audio samples as impulse responses. Each reverb node can share the same IR or they can have different ones. Place nodes around the stage and eventually around the audience.
The node positions can be adjusted directly on the map. The Ctrl/Cmd key will move a reverb node pair in symmetry.
  - KO: 이 공간 음향 프로세서에는 세 가지 리버브 유형이 내장되어 있습니다:
- SDN (산란 지연 네트워크): 소리가 반사면 역할을 하는 각 리버브 노드 사이에서 반사됩니다. 이 알고리즘은 대칭이 적은 홀수 개의 노드가 적합하며 금속성 공명이나 아티팩트를 줄입니다.
- FDN (피드백 지연 네트워크): 각 노드가 클래식 알고리즘을 가진 별도의 리버브 프로세서로 작동합니다. 무대 주변과 관객 주변에 노드를 배치합니다.
- IR (임펄스 응답): 클래식 컨볼루션 리버브입니다. 오디오 샘플을 임펄스 응답으로 로드할 수 있습니다. 각 노드는 같은 IR을 공유하거나 다른 IR을 사용할 수 있습니다.
노드 위치는 맵에서 직접 조정할 수 있습니다. Ctrl/Cmd 키로 리버브 노드 쌍을 대칭으로 이동합니다.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Algorithms
  - KO: 리버브 알고리즘
  - [ ] OK    Fix: 

## `help.reverbFeed`

- **`body`**
  - EN: Pre-processing sending of the input channels to the reverb nodes.
- Orientation and On/Off Angles define what inputs each reverb node will receive. Inputs in the green sector will be picked-up, but not the ones in front in the red sector. There is a fade between both sectors.
- HF Damping simulates the loss of high frequency with distance.
- Distance Attenuation percentage allows you to define if more or less of the attenuation calculated from the distance and parameters of the input is applied.
- Minimal Latency toggles whether the output is scanned for the smallest calculated delay and also if the setting once engaged on an input will reduce the delay for that input through this reverb node.
- Live Source Attenuation toggles the reduction of level of nearby input if they have this setting enabled.
  - KO: 입력 채널에서 리버브 노드로의 전처리 전송.
- 방향과 On/Off 각도가 각 노드가 수신할 입력을 정의합니다.
- HF 감쇠는 거리에 따른 고주파 손실을 시뮬레이션합니다.
- 거리 감쇠 비율은 적용되는 감쇠를 정의합니다.
- 최소 지연은 최소 계산 지연 사용을 전환합니다.
- 라이브 소스 감쇠는 근처 입력의 레벨 감소를 전환합니다.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Feed
  - KO: 리버브 피드
  - [ ] OK    Fix: 

## `help.reverbPostProc`

- **`body`**
  - EN: Includes a 4-band EQ and an Expander that looks at the signal entering the reverb processor to reduce long reverb tails when the inputs are quiet.
  - KO: 4밴드 EQ와 익스팬더를 포함하며, 입력이 조용할 때 리버브 프로세서에 들어가는 신호를 감시하여 긴 리버브 꼬리를 줄입니다.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Post-Processing
  - KO: 리버브 후처리
  - [ ] OK    Fix: 

## `help.reverbPreProc`

- **`body`**
  - EN: Includes a 4-band EQ and a Compressor to remove transients which might excite the reverb processor a bit too much.
  - KO: 4밴드 EQ와 컴프레서를 포함하여 리버브 프로세서를 과도하게 자극할 수 있는 트랜지언트를 제거합니다.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Pre-Processing
  - KO: 리버브 전처리
  - [ ] OK    Fix: 

## `help.reverbReturn`

- **`body`**
  - EN: Post-processing sending to the speakers.
- Distance Attenuation defines the level drop per meter to the speakers.
- Common Attenuation will look for the lowest attenuation and keep only a percentage of this level drop and apply this across all other outputs.
- Mutes and Mute Macros allow to prevent a reverb channel from feeding an output. For instance it may not be necessary to send reverb to sub-bass speakers.
  - KO: 스피커로의 후처리 전송.
- 거리 감쇠는 미터당 레벨 감소를 정의합니다.
- 공통 감쇠는 최저 감쇠의 비율을 유지합니다.
- 뮤트는 리버브 채널이 출력에 공급되는 것을 방지합니다.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Return
  - KO: 리버브 리턴
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
  - KO: 샘플러는 샘플을 트리거하고 실시간으로 상호작용할 수 있습니다.
트랙에서 활성화하면 샘플러는 항상 라이브 입력을 대체합니다.
여러 샘플러를 다른 입력에 할당하고 개별적으로 트리거할 수 있습니다.
샘플러 사용법:
- Roli Lightpad 또는 연결된 Android Remote 앱의 패드를 선택합니다.
- 그리드의 타일에 샘플을 추가합니다. 상대적 시작 위치, 레벨, 인/아웃 포인트를 조정합니다. Shift 키를 누르면서 클릭하면 여러 샘플을 선택할 수 있습니다.
- 샘플 세트 생성: 선택한 샘플이 새 세트에 추가됩니다. Ctrl/Cmd를 누르면서 클릭하면 세트 생성 후 추가/제거할 수 있습니다. 각 세트는 이름 변경이 가능하고 고정 순서 또는 랜덤 순서를 설정할 수 있습니다. 각 세트에는 감쇠 설정과 기본 위치가 있습니다.
- Lightpad 또는 패드를 눌러 샘플을 트리거합니다. 패드 압력은 레벨, 높이, 고주파 필터링에 매핑할 수 있습니다. 손가락 움직임은 조이스틱처럼 소리를 이동시킵니다.
패드를 놓으면 샘플이 중지됩니다.
샘플러 설정은 입력 파일에 저장됩니다.
타일과 세트는 복사, 내보내기, 가져오기가 가능합니다.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Sampler
  - KO: 샘플러
  - [ ] OK    Fix: 

## `help.sessionData`

- **`body`**
  - EN: When starting a session select the working folder where the system will place the files and eventual audio files. For new projects create a new folder. When reloading a previous session navigate to the corresponding folder.
Each section has a separate xml file (System configuration, Network, Outputs, Reverbs, Inputs) and backups. Additionally convolution reverb impulse responses and audio samples will be stored in sub-directories.
Each section can be stored and recalled individually or as a whole.
Each section can also export and import files from other projects.
  - KO: 세션 시작 시 시스템이 파일과 오디오 파일을 저장할 작업 폴더를 선택합니다. 새 프로젝트의 경우 새 폴더를 만듭니다. 이전 세션을 다시 로드하려면 해당 폴더로 이동합니다.
각 섹션에는 별도의 xml 파일(시스템 구성, 네트워크, 출력, 리버브, 입력)과 백업이 있습니다. 컨볼루션 리버브 임펄스 응답과 오디오 샘플은 하위 디렉토리에 저장됩니다.
각 섹션은 개별적으로 또는 전체로 저장하고 불러올 수 있습니다.
각 섹션은 다른 프로젝트에서 파일을 내보내고 가져올 수도 있습니다.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Session Data
  - KO: 세션 데이터
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
  - KO: 스냅샷은 입력 매개변수를 저장하며, 공연 중 불러올 범위를 가질 수 있습니다.
범위는 시스템에 어떤 데이터를 저장하거나 불러올지 지시합니다.
여러 방법이 있습니다:
- 필요한 데이터만 로컬 파일에 기록. 필터는 저장 시 적용됩니다.
- 모든 데이터와 필터를 로컬 파일에 기록. 필터는 불러올 때 적용됩니다.
- 모든 데이터를 QLab 큐에 기록. 대규모 설정에는 권장하지 않습니다.
범위는 수동으로 변경된 매개변수를 표시하고 자동으로 사전 선택할 수 있습니다. 변경된 매개변수는 노란색으로 표시됩니다.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Input Snapshots and Scope
  - KO: 입력 스냅샷 및 범위
  - [ ] OK    Fix: 

## `help.tracking`

- **`body`**
  - EN: Tracking allows to follow the position in 2D or 3D of actors and musicians. There are several tracking solutions based on UWB (UltraWide Band) tags, 3D camera, computer vision systems and Infra-Red LED's followed by IR sensitive cameras.
This application allows you to receive tracking data from several protocols: OSC, MQTT, PosiStageNet/PSN, RTTrP.
You can select which protocol you're using and enter its settings. You may also adjust the mapping (offset, scaling and orientation).
Each input has a toggle to enable tracking, an ID to select which tracking marker it should follow and a smoothing algorithm to reduce jittery movements.
  - KO: 트래킹은 배우와 음악가의 2D 또는 3D 위치를 추적할 수 있습니다. UWB 태그, 3D 카메라, 컴퓨터 비전 시스템, IR 감지 카메라와 적외선 LED 등 다양한 트래킹 솔루션이 있습니다.
이 애플리케이션은 여러 프로토콜에서 트래킹 데이터를 수신할 수 있습니다: OSC, MQTT, PosiStageNet/PSN, RTTrP.
사용할 프로토콜을 선택하고 설정을 입력할 수 있습니다. 매핑(오프셋, 스케일링, 방향)도 조정할 수 있습니다.
각 입력에는 트래킹 활성화 토글, 추적할 마커를 선택하는 ID, 떨림을 줄이는 스무딩 알고리즘이 있습니다.
  - [ ] OK    Fix: 

- **`title`**
  - EN: Tracking
  - KO: 트래킹
  - [ ] OK    Fix: 

## `help.tuning`

- **`body`**
  - EN: System tuning for WFS is different from standard PA tuning. It can proceed as follows:
- Start with the flown array muted. Set the desired levels for the near-field speakers when listening to them in the first row. Adjust the high frequency shelf attenuation so the near-field speakers are not too bright and draw too much attention.
- Mute the near-field array and unmute the flown array, find a suitable level towards the back of the house.
- Unmute both arrays, adjust the delay of the flown array to bring the sound down to the correct height in the lower rows. Adjust the levels, HF shelf/distance ratio and vertical and horizontal parallax for each array to have consistent level wherever your inputs are on stage. This is especially important if your inputs are actors, singers or acoustic music instruments on stage. You may also test adding some Haas effect to delay the whole system if you feel the WFS is too early or adding comb filtering with the acoustic sound.
You may follow a different workflow for tuning or go for different cues.
  - KO: WFS의 시스템 튜닝은 표준 PA 튜닝과 다릅니다. 다음과 같이 진행할 수 있습니다:
- 플라잉 어레이를 뮤트하고 시작합니다. 1열에서 들으면서 니어필드 스피커의 원하는 레벨을 설정합니다. 니어필드 스피커가 너무 밝지 않도록 HF 셸프 감쇠를 조정합니다.
- 니어필드 어레이를 뮤트하고 플라잉 어레이를 언뮤트하여 객석 뒤쪽에 적합한 레벨을 찾습니다.
- 두 어레이를 모두 언뮤트하고, 플라잉 어레이의 딜레이를 조정하여 아래 열에서 올바른 높이로 소리를 내립니다. 각 어레이의 레벨, HF 셸프/거리 비율, 수직 및 수평 시차를 조정하여 무대 어디에 인풋이 있든 일관된 레벨을 달성합니다.
다른 워크플로우로 튜닝하거나 상황에 따라 다른 설정을 목표로 할 수 있습니다.
  - [ ] OK    Fix: 

- **`title`**
  - EN: System Tuning
  - KO: 시스템 튜닝
  - [ ] OK    Fix: 

## `inputs`

- **`arrayPrefix`**
  - EN: Array
  - KO: 어레이
  - [ ] OK    Fix: 

## `inputs.buttons`

- **`deleteSnapshot`**
  - EN: Delete Snapshot
  - KO: 스냅샷 삭제
  - [ ] OK    Fix: 

- **`editScope`**
  - EN: Edit Scope
  - KO: 범위 편집
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - KO: 내보내기
  - [ ] OK    Fix: 

- **`hiddenOnMap`**
  - EN: Input Hidden on Map
  - KO: 맵에서 입력 숨김
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - KO: 가져오기
  - [ ] OK    Fix: 

- **`lockOnMap`**
  - EN: Lock on Map
  - KO: 맵에서 잠금
  - [ ] OK    Fix: 

- **`pauseAll`**
  - EN: Pause All
  - KO: 모두 일시 정지
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Backup
  - KO: 백업 불러오기
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Input Config
  - KO: 입력 설정 다시 불러오기
  - [ ] OK    Fix: 

- **`reloadSnapshot`**
  - EN: Reload Snapshot
  - KO: 스냅샷 불러오기
  - [ ] OK    Fix: 

- **`reloadWithoutScope`**
  - EN: Reload Without Scope
  - KO: 필터 없이 리로드
  - [ ] OK    Fix: 

- **`resumeAll`**
  - EN: Resume All
  - KO: 모두 재개
  - [ ] OK    Fix: 

- **`samplerOff`**
  - EN: Sampler: OFF
  - KO: Sampler: OFF
  - [ ] OK    Fix: 

- **`samplerOn`**
  - EN: Sampler: ON
  - KO: Sampler: ON
  - [ ] OK    Fix: 

- **`setAllInputs`**
  - EN: Set all Inputs...
  - KO: 모든 입력 설정...
  - [ ] OK    Fix: 

- **`solo`**
  - EN: Solo
  - KO: Solo
  - [ ] OK    Fix: 

- **`soloModeMulti`**
  - EN: Multi
  - KO: Multi
  - [ ] OK    Fix: 

- **`soloModeSingle`**
  - EN: Single
  - KO: Single
  - [ ] OK    Fix: 

- **`stopAll`**
  - EN: Stop All
  - KO: 모두 중지
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Input Config
  - KO: 입력 설정 저장
  - [ ] OK    Fix: 

- **`storeSnapshot`**
  - EN: Store Snapshot
  - KO: 스냅샷 저장
  - [ ] OK    Fix: 

- **`updateSnapshot`**
  - EN: Update Snapshot
  - KO: 스냅샷 업데이트
  - [ ] OK    Fix: 

- **`visibleOnMap`**
  - EN: Input Visible on Map
  - KO: 맵에 입력 표시
  - [ ] OK    Fix: 

## `inputs.clusters`

- **`clusterPrefix`**
  - EN: Cluster
  - KO: 클러스터
  - [ ] OK    Fix: 

- **`single`**
  - EN: Single
  - KO: 단일
  - [ ] OK    Fix: 

## `inputs.coordinates`

- **`xyz`**
  - EN: XYZ
  - KO: XYZ
  - [ ] OK    Fix: 

## `inputs.dialogs`

- **`exportConfig`**
  - EN: Export Input Configuration
  - KO: 입력 설정 내보내기
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Input Configuration
  - KO: 입력 설정 가져오기
  - [ ] OK    Fix: 

- **`selectChannel`**
  - EN: Select Channel
  - KO: 채널 선택
  - [ ] OK    Fix: 

- **`snapshotNameLabel`**
  - EN: Name:
  - KO: 이름:
  - [ ] OK    Fix: 

- **`storeSnapshotMessage`**
  - EN: Enter a name for the new snapshot:
  - KO: 새 스냅샷의 이름을 입력하세요:
  - [ ] OK    Fix: 

- **`storeSnapshotTitle`**
  - EN: Store New Snapshot
  - KO: 새 스냅샷 저장
  - [ ] OK    Fix: 

- **`trackingConflictCluster`**
  - EN: Input {current} has tracking enabled, but Input {existing} in Cluster {cluster} is already tracked.

Only one tracked input per cluster is allowed.
  - KO: 입력 {current}은(는) 트래킹이 활성화되어 있지만, 클러스터 {cluster}의 입력 {existing}이(가) 이미 트래킹되고 있습니다.

클러스터당 하나의 트래킹 입력만 허용됩니다.
  - [ ] OK    Fix: 

- **`trackingConflictContinue`**
  - EN: Continue (disable tracking)
  - KO: 계속 (트래킹 비활성화)
  - [ ] OK    Fix: 

- **`trackingConflictSwitch`**
  - EN: Input {existing} in Cluster {cluster} already has tracking enabled.

Only one tracked input per cluster is allowed.

Do you want to disable tracking on Input {existing} and enable it on Input {to}?
  - KO: 클러스터 {cluster}의 입력 {existing}은(는) 이미 트래킹이 활성화되어 있습니다.

클러스터당 하나의 트래킹 입력만 허용됩니다.

입력 {existing}의 트래킹을 비활성화하고 입력 {to}에서 활성화하시겠습니까?
  - [ ] OK    Fix: 

- **`trackingConflictTitle`**
  - EN: Tracking Conflict
  - KO: 트래킹 충돌
  - [ ] OK    Fix: 

- **`trackingConflictYes`**
  - EN: Yes, switch tracking
  - KO: 예, 트래킹 전환
  - [ ] OK    Fix: 

## `inputs.gradientMap`

- **`buttons.copyLayer`**
  - EN: Copy Layer
  - KO: 레이어 복사
  - [ ] OK    Fix: 

- **`buttons.copyShape`**
  - EN: Copy Shape
  - KO: 도형 복사
  - [ ] OK    Fix: 

- **`buttons.delete`**
  - EN: Delete
  - KO: 삭제
  - [ ] OK    Fix: 

- **`buttons.enable`**
  - EN: On
  - KO: 켜기
  - [ ] OK    Fix: 

- **`buttons.lock`**
  - EN: Lock
  - KO: 잠금
  - [ ] OK    Fix: 

- **`buttons.pasteLayer`**
  - EN: Paste Layer
  - KO: 레이어 붙여넣기
  - [ ] OK    Fix: 

- **`buttons.pasteShape`**
  - EN: Paste Shape
  - KO: 도형 붙여넣기
  - [ ] OK    Fix: 

- **`header.attenuationLayer`**
  - EN: Attenuation Layer
  - KO: 감쇠 레이어
  - [ ] OK    Fix: 

- **`header.heightLayer`**
  - EN: Height Layer
  - KO: 높이 레이어
  - [ ] OK    Fix: 

- **`header.hfShelfLayer`**
  - EN: HF Shelf Layer
  - KO: HF 셸프 레이어
  - [ ] OK    Fix: 

- **`help.blackValue`**
  - EN: Parameter value mapped to black (0.00–1.00)
  - KO: 검정에 매핑되는 파라미터 값 (0.00–1.00)
  - [ ] OK    Fix: 

- **`help.blur`**
  - EN: Edge blur in meters
  - KO: 가장자리 블러 (미터 단위)
  - [ ] OK    Fix: 

- **`help.copy`**
  - EN: Copy selected shape or layer
  - KO: 선택한 도형 또는 레이어 복사
  - [ ] OK    Fix: 

- **`help.curve`**
  - EN: Gamma curve (-1 to 1, 0 = linear)
  - KO: 감마 커브 (-1~1, 0 = 선형)
  - [ ] OK    Fix: 

- **`help.ellipseTool`**
  - EN: Draw ellipse
  - KO: 타원 그리기
  - [ ] OK    Fix: 

- **`help.fillTool`**
  - EN: Apply uniform fill to shape
  - KO: 도형에 균일 채우기 적용
  - [ ] OK    Fix: 

- **`help.fillValue`**
  - EN: Fill brightness (0 = black, 1 = white)
  - KO: 채우기 밝기 (0 = 검정, 1 = 흰색)
  - [ ] OK    Fix: 

- **`help.layerEnable`**
  - EN: Enable/disable layer (affects output and OSC)
  - KO: 레이어 활성화/비활성화 (출력 및 OSC에 영향)
  - [ ] OK    Fix: 

- **`help.layerSelect`**
  - EN: Select this layer for editing
  - KO: 이 레이어를 편집용으로 선택
  - [ ] OK    Fix: 

- **`help.layerVisible`**
  - EN: Show/hide layer preview on canvas
  - KO: 캔버스에서 레이어 미리보기 표시/숨기기
  - [ ] OK    Fix: 

- **`help.linGradTool`**
  - EN: Apply linear gradient to shape
  - KO: 도형에 선형 그라데이션 적용
  - [ ] OK    Fix: 

- **`help.paste`**
  - EN: Paste shape or layer from clipboard
  - KO: 클립보드에서 도형 또는 레이어 붙여넣기
  - [ ] OK    Fix: 

- **`help.polygonTool`**
  - EN: Draw polygon (double-click to close)
  - KO: 다각형 그리기 (더블클릭으로 닫기)
  - [ ] OK    Fix: 

- **`help.radGradTool`**
  - EN: Apply radial gradient to shape
  - KO: 도형에 방사형 그라데이션 적용
  - [ ] OK    Fix: 

- **`help.rectTool`**
  - EN: Draw rectangle
  - KO: 사각형 그리기
  - [ ] OK    Fix: 

- **`help.selectTool`**
  - EN: Select and move shapes
  - KO: 도형 선택 및 이동
  - [ ] OK    Fix: 

- **`help.shapeDelete`**
  - EN: Delete selected shape(s)
  - KO: 선택한 도형 삭제
  - [ ] OK    Fix: 

- **`help.shapeEnable`**
  - EN: Enable/disable shape
  - KO: 도형 활성화/비활성화
  - [ ] OK    Fix: 

- **`help.shapeLock`**
  - EN: Lock shape position
  - KO: 도형 위치 잠금
  - [ ] OK    Fix: 

- **`help.whiteValue`**
  - EN: Parameter value mapped to white (0.00–1.00)
  - KO: 흰색에 매핑되는 파라미터 값 (0.00–1.00)
  - [ ] OK    Fix: 

- **`hints.darkMaxAtten`**
  - EN: Dark = max attenuation | Light = none
  - KO: 어둡 = 최대 감쇠 | 밝음 = 없음
  - [ ] OK    Fix: 

- **`hints.darkMaxHeight`**
  - EN: Dark = max height | Light = ground
  - KO: 어둡 = 최대 높이 | 밝음 = 바닥
  - [ ] OK    Fix: 

- **`hints.darkMaxHF`**
  - EN: Dark = max HF shelf | Light = none
  - KO: 어둡 = 최대 HF 셸프 | 밝음 = 없음
  - [ ] OK    Fix: 

- **`hints.polygonClose`**
  - EN: Double-click to close polygon
  - KO: 다각형을 닫으려면 더블클릭
  - [ ] OK    Fix: 

- **`hints.whiteMaxAtten`**
  - EN: White = max attenuation | Black = none
  - KO: 흰색 = 최대 감쇠 | 검정 = 없음
  - [ ] OK    Fix: 

- **`hints.whiteMaxHeight`**
  - EN: White = max height | Black = ground
  - KO: 흰색 = 최대 높이 | 검정 = 바닥
  - [ ] OK    Fix: 

- **`hints.whiteMaxHF`**
  - EN: White = max HF shelf | Black = none
  - KO: 흰색 = 최대 HF 셸프 | 검정 = 없음
  - [ ] OK    Fix: 

- **`labels.black`**
  - EN: Black:
  - KO: 검정:
  - [ ] OK    Fix: 

- **`labels.blur`**
  - EN: Blur:
  - KO: 블러:
  - [ ] OK    Fix: 

- **`labels.center`**
  - EN: Center:
  - KO: 중심:
  - [ ] OK    Fix: 

- **`labels.curve`**
  - EN: Curve:
  - KO: 커브:
  - [ ] OK    Fix: 

- **`labels.edge`**
  - EN: Edge:
  - KO: 가장자리:
  - [ ] OK    Fix: 

- **`labels.end`**
  - EN: End:
  - KO: 끝:
  - [ ] OK    Fix: 

- **`labels.fill`**
  - EN: Fill:
  - KO: 채우기:
  - [ ] OK    Fix: 

- **`labels.name`**
  - EN: Name:
  - KO: 이름:
  - [ ] OK    Fix: 

- **`labels.start`**
  - EN: Start:
  - KO: 시작:
  - [ ] OK    Fix: 

- **`labels.white`**
  - EN: White:
  - KO: 흰색:
  - [ ] OK    Fix: 

- **`layers.attenuation`**
  - EN: Attenuation
  - KO: 감쇠
  - [ ] OK    Fix: 

- **`layers.height`**
  - EN: Height
  - KO: 높이
  - [ ] OK    Fix: 

- **`layers.hfShelf`**
  - EN: HF Shelf
  - KO: HF 셸프
  - [ ] OK    Fix: 

- **`tools.editPoints`**
  - EN: Edit Points
  - KO: 포인트 편집
  - [ ] OK    Fix: 

- **`tools.ellipse`**
  - EN: Ellipse
  - KO: 타원
  - [ ] OK    Fix: 

- **`tools.fill`**
  - EN: Fill
  - KO: 채우기
  - [ ] OK    Fix: 

- **`tools.linGrad`**
  - EN: Linear Grad.
  - KO: 선형 그라데이션
  - [ ] OK    Fix: 

- **`tools.polygon`**
  - EN: Polygon
  - KO: 다각형
  - [ ] OK    Fix: 

- **`tools.radGrad`**
  - EN: Radial Grad.
  - KO: 방사형 그라데이션
  - [ ] OK    Fix: 

- **`tools.rect`**
  - EN: Rectangle
  - KO: 사각형
  - [ ] OK    Fix: 

- **`tools.select`**
  - EN: Select
  - KO: 선택
  - [ ] OK    Fix: 

- **`warnings.heightRatioZero`**
  - EN: Height Ratio is 0% — increase it for height to take effect
  - KO: 높이 비율이 0%입니다 — 높이가 적용되려면 값을 올려주세요
  - [ ] OK    Fix: 

## `inputs.help`

- **`admMapping`**
  - EN: Assign this input to an ADM-OSC mapping for position receive/transmit.
  - KO: 이 입력을 ADM-OSC 매핑에 할당하여 위치 송수신을 수행합니다.
  - [ ] OK    Fix: 

- **`arrayAttenDial`**
  - EN: Attenuation for Array {num} (-60 to 0 dB).
  - KO: 어레이 {num}의 감쇠 (-60~0 dB).
  - [ ] OK    Fix: 

- **`attenuationLawButton`**
  - EN: Attenuation Law Model (Linear Decrease of Volume with Distance Between Object and Speaker or Squared).
  - KO: 감쇠 법칙 모델 (객체와 스피커 사이의 거리에 따른 선형 음량 감소 또는 제곱).
  - [ ] OK    Fix: 

- **`attenuationSlider`**
  - EN: Input Channel Attenuation.
  - KO: 입력 채널 감쇠.
  - [ ] OK    Fix: 

- **`channelSelector`**
  - EN: Input Channel Number and Selection.
  - KO: 입력 채널 번호 및 선택.
  - [ ] OK    Fix: 

- **`clusterSelector`**
  - EN: Object is Part of a Cluster.
  - KO: 객체가 클러스터의 일부입니다.
  - [ ] OK    Fix: 

- **`commonAttenDial`**
  - EN: Percentage of the Common Part of the Attenuation for selected Object Relative to All Outputs.
  - KO: 선택한 객체에 대한 모든 출력 대비 감쇠 공통 부분의 백분율.
  - [ ] OK    Fix: 

- **`constraintDistanceButton`**
  - EN: Limit Position to Distance Range from Origin (for Cylindrical/Spherical modes).
  - KO: 원점으로부터의 거리 범위로 위치 제한 (원통/구형 모드용).
  - [ ] OK    Fix: 

- **`constraintXButton`**
  - EN: Limit Position to the Bounds of the Stage in Width.
  - KO: 위치를 무대 너비 범위로 제한.
  - [ ] OK    Fix: 

- **`constraintYButton`**
  - EN: Limit Position to the Bounds of the Stage in Depth.
  - KO: 위치를 무대 깊이 범위로 제한.
  - [ ] OK    Fix: 

- **`constraintZButton`**
  - EN: Limit Position to the Bounds of the Stage in Height.
  - KO: 위치를 무대 높이 범위로 제한.
  - [ ] OK    Fix: 

- **`coordModeSelector`**
  - EN: Coordinate display mode: Cartesian (X/Y/Z), Cylindrical (radius/azimuth/height), or Spherical (radius/azimuth/elevation).
  - KO: 좌표 표시 모드: 직교 (X/Y/Z), 원통 (반지름/방위각/높이) 또는 구면 (반지름/방위각/고도).
  - [ ] OK    Fix: 

- **`delayLatencySlider`**
  - EN: Input Channel Delay (positive values) or Latency Compensation (negative values).
  - KO: 입력 채널 지연 (양수 값) 또는 지연 보정 (음수 값).
  - [ ] OK    Fix: 

- **`deleteSnapshot`**
  - EN: Delete Selected Input Snapshot With Confirmation.
  - KO: 확인 후 선택한 입력 스냅샷을 삭제합니다.
  - [ ] OK    Fix: 

- **`directivitySlider`**
  - EN: How Wide is the Brightness of The Object.
  - KO: 객체의 밝기 콘 너비.
  - [ ] OK    Fix: 

- **`distanceAttenDial`**
  - EN: Attenuation per Meter Between Object and Speaker.
  - KO: 객체와 스피커 사이의 미터당 감쇠.
  - [ ] OK    Fix: 

- **`distanceMaxEditor`**
  - EN: Maximum Distance from Origin in Meters.
  - KO: 원점으로부터의 최대 거리 (미터).
  - [ ] OK    Fix: 

- **`distanceMinEditor`**
  - EN: Minimum Distance from Origin in Meters.
  - KO: 원점으로부터의 최소 거리 (미터).
  - [ ] OK    Fix: 

- **`distanceRangeSlider`**
  - EN: Set Minimum and Maximum Distance from Origin.
  - KO: 원점으로부터의 최소 및 최대 거리 설정.
  - [ ] OK    Fix: 

- **`distanceRatioDial`**
  - EN: Attenuation Ratio for Squared Model.
  - KO: 제곱 모델의 감쇠 비율.
  - [ ] OK    Fix: 

- **`editScope`**
  - EN: Open Selected Input Snapshot Scope Window.
  - KO: 선택한 입력 스냅샷의 필터 창을 엽니다.
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Input Configuration to file (with file explorer window).
  - KO: 입력 구성을 파일로 내보냅니다 (파일 탐색기 사용).
  - [ ] OK    Fix: 

- **`flipXButton`**
  - EN: X will be Symetrical to the Origin. Keyboard Nudging will be Inverted.
  - KO: X가 원점에 대해 대칭이 됩니다. 키보드 미세 조정이 반전됩니다.
  - [ ] OK    Fix: 

- **`flipYButton`**
  - EN: Y will be Symetrical to the Origin. Keyboard Nudging will be Inverted.
  - KO: Y가 원점에 대해 대칭이 됩니다. 키보드 미세 조정이 반전됩니다.
  - [ ] OK    Fix: 

- **`flipZButton`**
  - EN: Z will be Symetrical to the Origin. Keyboard Nudging will be Inverted.
  - KO: Z가 원점에 대해 대칭이 됩니다. 키보드 미세 조정이 반전됩니다.
  - [ ] OK    Fix: 

- **`frActiveButton`**
  - EN: Enable Simulated Floor Reflections for the Object.
  - KO: 객체에 대한 시뮬레이션된 바닥 반사를 활성화.
  - [ ] OK    Fix: 

- **`frAttenuationSlider`**
  - EN: Attenuation of the Simulated Floor Reflections for the Object.
  - KO: 객체에 대한 시뮬레이션된 바닥 반사의 감쇠.
  - [ ] OK    Fix: 

- **`frDiffusionDial`**
  - EN: Diffusion Effect of the Simulated Floor Reflections for the Object.
  - KO: 객체에 대한 시뮬레이션된 바닥 반사의 확산 효과.
  - [ ] OK    Fix: 

- **`frHighShelfActiveButton`**
  - EN: Enable High Shelf Filter for Floor Reflections.
  - KO: 바닥 반사용 하이쉘프 필터 활성화.
  - [ ] OK    Fix: 

- **`frHighShelfFreqSlider`**
  - EN: High Shelf Frequency for Floor Reflections.
  - KO: 바닥 반사용 하이쉘프 주파수.
  - [ ] OK    Fix: 

- **`frHighShelfGainSlider`**
  - EN: High Shelf Gain for Floor Reflections.
  - KO: 바닥 반사용 하이쉘프 게인.
  - [ ] OK    Fix: 

- **`frHighShelfSlopeSlider`**
  - EN: High Shelf Slope for Floor Reflections.
  - KO: 바닥 반사용 하이쉘프 기울기.
  - [ ] OK    Fix: 

- **`frLowCutActiveButton`**
  - EN: Enable Low Cut Filter for Floor Reflections.
  - KO: 바닥 반사용 로우컷 필터 활성화.
  - [ ] OK    Fix: 

- **`frLowCutFreqSlider`**
  - EN: Low Cut Frequency for Floor Reflections.
  - KO: 바닥 반사용 로우컷 주파수.
  - [ ] OK    Fix: 

- **`heightFactorDial`**
  - EN: Take Elevation of Object into Account Fully, Partially or Not.
  - KO: 객체의 고도를 완전히, 부분적으로 또는 전혀 고려하지 않습니다.
  - [ ] OK    Fix: 

- **`hfShelfSlider`**
  - EN: How Much Brightness is lost in the Back of the Object, Out of its Brightness Cone.
  - KO: 밝기 콘 외부, 객체 뒤쪽에서 얼마나 많은 밝기가 손실되는지.
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Input Configuration from file (with file explorer window).
  - KO: 파일에서 입력 구성을 가져옵니다 (파일 탐색기 사용).
  - [ ] OK    Fix: 

- **`inputDirectivityDial`**
  - EN: Where is the Object pointing to in the Horizontal Plane.
  - KO: 수평면에서 객체의 방향.
  - [ ] OK    Fix: 

- **`jitterSlider`**
  - EN: Sphere of Rapid Movements of the Object.
  - KO: 객체의 빠른 움직임의 구.
  - [ ] OK    Fix: 

- **`lfoActiveButton`**
  - EN: Enable or Disable the Periodic Movement of the Object (LFO).
  - KO: 객체의 주기적 움직임 (LFO)을 활성화 또는 비활성화.
  - [ ] OK    Fix: 

- **`lfoAmplitudeXSlider`**
  - EN: Width of Movement in Relation to Base Position of the Object.
  - KO: 객체 기본 위치에 대한 너비 움직임 크기.
  - [ ] OK    Fix: 

- **`lfoAmplitudeYSlider`**
  - EN: Depth of Movement in Relation to Base Position of the Object.
  - KO: 객체 기본 위치에 대한 깊이 움직임 크기.
  - [ ] OK    Fix: 

- **`lfoAmplitudeZSlider`**
  - EN: Height of Movement in Relation to Base Position of the Object.
  - KO: 객체 기본 위치에 대한 높이 움직임 크기.
  - [ ] OK    Fix: 

- **`lfoGyrophoneSelector`**
  - EN: Rotation of the Brightness Cone of the Object.
  - KO: 객체의 밝기 콘 회전.
  - [ ] OK    Fix: 

- **`lfoPeriodDial`**
  - EN: Base Period of the Movement of the Object.
  - KO: 객체 움직임의 기본 주기.
  - [ ] OK    Fix: 

- **`lfoPhaseDial`**
  - EN: Phase Offset of the Movement of the Object.
  - KO: 객체 움직임의 위상 오프셋.
  - [ ] OK    Fix: 

- **`lfoPhaseXDial`**
  - EN: Phase Offset of the Movement of the Object in Width.
  - KO: 너비에서 객체 움직임의 위상 오프셋.
  - [ ] OK    Fix: 

- **`lfoPhaseYDial`**
  - EN: Phase Offset of the Movement of the Object in Depth.
  - KO: 깊이에서 객체 움직임의 위상 오프셋.
  - [ ] OK    Fix: 

- **`lfoPhaseZDial`**
  - EN: Phase Offset of the Movement of the Object in Height.
  - KO: 높이에서 객체 움직임의 위상 오프셋.
  - [ ] OK    Fix: 

- **`lfoRateXSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Width.
  - KO: 너비에서 기본 주기에 대한 빠르거나 느린 움직임.
  - [ ] OK    Fix: 

- **`lfoRateYSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Depth.
  - KO: 깊이에서 기본 주기에 대한 빠르거나 느린 움직임.
  - [ ] OK    Fix: 

- **`lfoRateZSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Height.
  - KO: 높이에서 기본 주기에 대한 빠르거나 느린 움직임.
  - [ ] OK    Fix: 

- **`lfoShapeXSelector`**
  - EN: Movement Behaviour of the Object in Width.
  - KO: 너비에서 객체 움직임의 동작.
  - [ ] OK    Fix: 

- **`lfoShapeYSelector`**
  - EN: Movement Behaviour of the Object in Depth.
  - KO: 깊이에서 객체 움직임의 동작.
  - [ ] OK    Fix: 

- **`lfoShapeZSelector`**
  - EN: Movement Behaviour of the Object in Height.
  - KO: 높이에서 객체 움직임의 동작.
  - [ ] OK    Fix: 

- **`lsActiveButton`**
  - EN: If You Need to Reduce the Level in Speakers Close to the Object. (eg. Loud Source Present on Stage)
  - KO: 객체에 가까운 스피커의 레벨을 줄여야 하는 경우 (예: 무대 위 큰 음원).
  - [ ] OK    Fix: 

- **`lsAttenuationSlider`**
  - EN: Constant Attenuation of Speakers Around the Object.
  - KO: 객체 주변 스피커의 일정한 감쇠.
  - [ ] OK    Fix: 

- **`lsPeakEnableButton`**
  - EN: Enable or Disable the Fast (Peak) Compressor for the Live Source Tamer.
  - KO: Live Source Tamer의 빠른 (피크) 컴프레서를 활성화 또는 비활성화.
  - [ ] OK    Fix: 

- **`lsPeakRatioDial`**
  - EN: Ratio to Apply the Fast Compression for Speakers Around the Object.
  - KO: 객체 주변 스피커의 빠른 압축에 적용할 비율.
  - [ ] OK    Fix: 

- **`lsPeakThresholdSlider`**
  - EN: Fast Compression Threshold for Speakers Around the Object to Control Transients.
  - KO: 객체 주변 스피커의 트랜지언트 제어를 위한 빠른 압축 임계값.
  - [ ] OK    Fix: 

- **`lsRadiusSlider`**
  - EN: How Far does the Attenuation Affect The Speakers.
  - KO: 감쇠가 스피커에 영향을 미치는 거리.
  - [ ] OK    Fix: 

- **`lsShapeSelector`**
  - EN: Profile of the Attenuation Around the Object.
  - KO: 객체 주변의 감쇠 프로파일.
  - [ ] OK    Fix: 

- **`lsSlowEnableButton`**
  - EN: Enable or Disable the Slow Compressor for the Live Source Tamer.
  - KO: Live Source Tamer의 느린 컴프레서를 활성화 또는 비활성화.
  - [ ] OK    Fix: 

- **`lsSlowRatioDial`**
  - EN: Ratio to Apply the Slow Compression for Speakers Around the Object.
  - KO: 객체 주변 스피커의 느린 압축에 적용할 비율.
  - [ ] OK    Fix: 

- **`lsSlowThresholdSlider`**
  - EN: Slow Compression Threshold for Speakers Around the Object to Control Sustained Level.
  - KO: 객체 주변 스피커의 지속 레벨 제어를 위한 느린 압축 임계값.
  - [ ] OK    Fix: 

- **`mapLockButton`**
  - EN: Prevent Interaction on the Map Tab
  - KO: 맵 탭에서의 상호작용을 방지합니다.
  - [ ] OK    Fix: 

- **`mapVisibilityButton`**
  - EN: Make Visible or Hide The Selected Input on the Map
  - KO: 맵에서 선택한 입력을 표시하거나 숨깁니다.
  - [ ] OK    Fix: 

- **`maxSpeedActiveButton`**
  - EN: Enable or Disable Speed Limiting for Object.
  - KO: 객체의 속도 제한을 활성화 또는 비활성화.
  - [ ] OK    Fix: 

- **`maxSpeedDial`**
  - EN: Maximum Speed Limit for Object.
  - KO: 객체의 최대 속도 제한.
  - [ ] OK    Fix: 

- **`minimalLatencyButton`**
  - EN: Select between Acoustic Precedence and Minimal Latency for Amplification Precedence.
  - KO: 증폭 우선순위에 대해 음향 우선순위와 최소 지연 사이를 선택.
  - [ ] OK    Fix: 

- **`muteButton`**
  - EN: Mute Output {num} for this Object.
  - KO: 이 객체에 대한 출력 {num} 음소거.
  - [ ] OK    Fix: 

- **`muteMacrosSelector`**
  - EN: Mute Macros for Fast Muting and Unmuting of Arrays.
  - KO: 어레이의 빠른 음소거 및 음소거 해제를 위한 음소거 매크로.
  - [ ] OK    Fix: 

- **`muteReverbSendsButton`**
  - EN: Mute sends from this input to all reverb channels.
  - KO: 이 입력에서 모든 리버브 채널로의 전송을 음소거.
  - [ ] OK    Fix: 

- **`nameEditor`**
  - EN: Displayed Input Channel Name (editable).
  - KO: 표시되는 입력 채널 이름 (편집 가능).
  - [ ] OK    Fix: 

- **`nudgeHint1`**
  - EN:  Nudge with Left and Right Arrow Keys.
  - KO:  좌우 화살표 키로 조정.
  - [ ] OK    Fix: 

- **`nudgeHint2`**
  - EN:  Nudge with Up and Down Arrow Keys.
  - KO:  상하 화살표 키로 조정.
  - [ ] OK    Fix: 

- **`nudgeHint3`**
  - EN:  Nudge with Page Up and Page Down Keys.
  - KO:  Page Up/Down으로 조정.
  - [ ] OK    Fix: 

- **`offset1`**
  - EN: Object Offset {name} ({unit}). Adjusted when Tracking is Enabled.
  - KO: 오브젝트 오프셋 {name} ({unit}). 트래킹 활성화 시 조정됨.
  - [ ] OK    Fix: 

- **`offset2`**
  - EN: Object Offset {name} ({unit}). Adjusted when Tracking is Enabled.
  - KO: 오브젝트 오프셋 {name} ({unit}). 트래킹 활성화 시 조정됨.
  - [ ] OK    Fix: 

- **`offset3`**
  - EN: Object Offset {name} ({unit}). Adjusted when Tracking is Enabled.
  - KO: 오브젝트 오프셋 {name} ({unit}). 트래킹 활성화 시 조정됨.
  - [ ] OK    Fix: 

- **`otomoAbsRelButton`**
  - EN: Select Relative or Absolute Coordinates of Displacement.
  - KO: 상대 또는 절대 변위 좌표 선택.
  - [ ] OK    Fix: 

- **`otomoCoordMode`**
  - EN: Coordinate display mode for AutomOtion destinations: Cartesian (X/Y/Z), Cylindrical (r/θ/Z), or Spherical (r/θ/φ).
  - KO: AutomOtion 목적지 좌표 표시 모드: 직교(X/Y/Z), 원통(r/θ/Z), 구면(r/θ/φ).
  - [ ] OK    Fix: 

- **`otomoCurveDial`**
  - EN: Bend the Path to the Left (Negative) or Right (Positive) of the Direction of Travel.
  - KO: 이동 방향의 왼쪽 (음수) 또는 오른쪽 (양수)으로 경로를 구부립니다.
  - [ ] OK    Fix: 

- **`otomoDest1`**
  - EN: Relative or Absolute Destination {name} ({unit}).
  - KO: 상대 또는 절대 목적지 {name} ({unit}).
  - [ ] OK    Fix: 

- **`otomoDest2`**
  - EN: Relative or Absolute Destination {name} ({unit}).
  - KO: 상대 또는 절대 목적지 {name} ({unit}).
  - [ ] OK    Fix: 

- **`otomoDest3`**
  - EN: Relative or Absolute Destination {name} ({unit}).
  - KO: 상대 또는 절대 목적지 {name} ({unit}).
  - [ ] OK    Fix: 

- **`otomoDurationDial`**
  - EN: Duration of the Movement in Seconds (0.1s to 1 hour).
  - KO: 움직임 지속 시간 (초, 0.1초~1시간).
  - [ ] OK    Fix: 

- **`otomoPauseButton`**
  - EN: Pause and Resume the Movement.
  - KO: 움직임 일시 정지 및 재개.
  - [ ] OK    Fix: 

- **`otomoPauseResumeAllButton`**
  - EN: Pause or Resume All Active Movements Globally.
  - KO: 모든 활성 움직임을 전역적으로 일시 정지 또는 재개.
  - [ ] OK    Fix: 

- **`otomoResetDial`**
  - EN: Set the Reset Level for the Automatic Trigger.
  - KO: 자동 트리거 리셋 레벨 설정.
  - [ ] OK    Fix: 

- **`otomoSpeedProfileDial`**
  - EN: Constant Speed or Gradual Acceleration and Slow Down at the Start and the End of the Movement.
  - KO: 움직임 시작과 끝에서 일정한 속도 또는 점진적 가속과 감속.
  - [ ] OK    Fix: 

- **`otomoStartButton`**
  - EN: Start the Movement Manually.
  - KO: 움직임 수동 시작.
  - [ ] OK    Fix: 

- **`otomoStayReturnButton`**
  - EN: At the End of the Movement, should the Source Stay or Return to the Original Position.
  - KO: 움직임 끝에 소스가 머물지 또는 원래 위치로 돌아갈지.
  - [ ] OK    Fix: 

- **`otomoStopAllButton`**
  - EN: Stop All Active Movements Globally.
  - KO: 모든 활성 움직임을 전역적으로 정지.
  - [ ] OK    Fix: 

- **`otomoStopButton`**
  - EN: Stop the Movement.
  - KO: 움직임 정지.
  - [ ] OK    Fix: 

- **`otomoThresholdDial`**
  - EN: Set the Threshold for the Automatic Trigger of the Movement.
  - KO: 움직임의 자동 트리거 임계값 설정.
  - [ ] OK    Fix: 

- **`otomoTriggerButton`**
  - EN: Manual Start of Displacement or Automatic Trigger on the Audio Level.
  - KO: 변위의 수동 시작 또는 오디오 레벨에 따른 자동 트리거.
  - [ ] OK    Fix: 

- **`pathModeButton`**
  - EN: Enable Path Mode to Follow Drawn Movement Paths Instead of Direct Lines.
  - KO: 직선 대신 그려진 이동 경로를 따르는 경로 모드 활성화.
  - [ ] OK    Fix: 

- **`position1`**
  - EN: Object {name} ({unit}).
  - KO: 오브젝트 {name} ({unit}).
  - [ ] OK    Fix: 

- **`position2`**
  - EN: Object {name} ({unit}).
  - KO: 오브젝트 {name} ({unit}).
  - [ ] OK    Fix: 

- **`position3`**
  - EN: Object {name} ({unit}).
  - KO: 오브젝트 {name} ({unit}).
  - [ ] OK    Fix: 

- **`positionJoystick`**
  - EN: Drag to adjust X/Y position in real-time. Returns to center on release.
  - KO: X/Y 위치를 실시간으로 조정하려면 드래그합니다. 놓으면 중앙으로 돌아갑니다.
  - [ ] OK    Fix: 

- **`positionZSlider`**
  - EN: Drag to adjust Z (height) position in real-time. Returns to center on release.
  - KO: Z (높이) 위치를 실시간으로 조정하려면 드래그합니다. 놓으면 중앙으로 돌아갑니다.
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Input Configuration from backup file.
  - KO: 백업 파일에서 입력 구성을 다시 불러옵니다.
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Input Configuration from file.
  - KO: 파일에서 입력 구성을 다시 불러옵니다.
  - [ ] OK    Fix: 

- **`reloadSnapshot`**
  - EN: Reload Selected Input Snapshot for All Objects Taking the Scope into Account.
  - KO: 필터를 고려하여 선택한 입력 스냅샷을 모든 오브젝트에 다시 불러옵니다.
  - [ ] OK    Fix: 

- **`reloadWithoutScope`**
  - EN: Reload Selected Input Snapshot for All Objects Without the Scope.
  - KO: 필터 없이 선택한 입력 스냅샷을 모든 오브젝트에 다시 불러옵니다.
  - [ ] OK    Fix: 

- **`sidelinesActiveButton`**
  - EN: Enable Automatic Muting when Source Approaches Stage Edges. Does Not Apply to Downstage (Front) Edge.
  - KO: 소스가 무대 가장자리에 접근할 때 자동 음소거 활성화. 앞쪽 (관객 측) 가장자리에는 적용되지 않습니다.
  - [ ] OK    Fix: 

- **`sidelinesFringeDial`**
  - EN: Fringe Zone Size in Meters. Outer Half is Full Mute, Inner Half Fades Linearly.
  - KO: 전환 영역 크기 (미터). 외부 절반은 완전히 음소거하고, 내부 절반은 선형으로 페이드합니다.
  - [ ] OK    Fix: 

- **`snapshotSelector`**
  - EN: Select Input Snapshot Without Loading.
  - KO: 입력 스냅샷을 불러오지 않고 선택합니다.
  - [ ] OK    Fix: 

- **`solo`**
  - EN: Listen to Binaural Rendering of this channel.
  - KO: 이 채널의 바이노럴 렌더링을 청취합니다.
  - [ ] OK    Fix: 

- **`soloMode`**
  - EN: Single: one input at a time. Multi: multiple inputs simultaneously.
  - KO: 단일: 한 번에 하나의 입력. 다중: 여러 입력 동시에.
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Input Configuration to file (with backup).
  - KO: 입력 구성을 파일에 저장합니다 (백업 포함).
  - [ ] OK    Fix: 

- **`storeSnapshot`**
  - EN: Store new Input Snapshot for All Objects.
  - KO: 모든 오브젝트에 대한 새 입력 스냅샷을 저장합니다.
  - [ ] OK    Fix: 

- **`tiltSlider`**
  - EN: Where is the Object pointing to in the Vertical Plane.
  - KO: 수직면에서 객체의 방향.
  - [ ] OK    Fix: 

- **`trackingActiveButton`**
  - EN: Enable or Disable Tracking for Object.
  - KO: 객체에 대한 트래킹을 활성화 또는 비활성화.
  - [ ] OK    Fix: 

- **`trackingIdSelector`**
  - EN: Tracker ID for Object.
  - KO: 객체의 트래커 ID.
  - [ ] OK    Fix: 

- **`trackingSmoothDial`**
  - EN: Smoothing of Tracking Data for Object.
  - KO: 객체의 트래킹 데이터 평활화.
  - [ ] OK    Fix: 

- **`updateSnapshot`**
  - EN: Update Selected Input Snapshot (with backup).
  - KO: 선택한 입력 스냅샷을 업데이트합니다 (백업 포함).
  - [ ] OK    Fix: 

## `inputs.labels`

- **`admMapping`**
  - EN: ADM:
  - KO: ADM:
  - [ ] OK    Fix: 

- **`amplitudeX`**
  - EN: Amplitude X:
  - KO: 진폭 X:
  - [ ] OK    Fix: 

- **`amplitudeY`**
  - EN: Amplitude Y:
  - KO: 진폭 Y:
  - [ ] OK    Fix: 

- **`amplitudeZ`**
  - EN: Amplitude Z:
  - KO: 진폭 Z:
  - [ ] OK    Fix: 

- **`arrayAttenuation`**
  - EN: Array Attenuation:
  - KO: 어레이 감쇠:
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Attenuation:
  - KO: 감쇠:
  - [ ] OK    Fix: 

- **`attenuationLaw`**
  - EN: Attenuation Law:
  - KO: 감쇠 법칙:
  - [ ] OK    Fix: 

- **`cluster`**
  - EN: Cluster:
  - KO: 클러스터:
  - [ ] OK    Fix: 

- **`commonAtten`**
  - EN: Common Atten:
  - KO: 공통 감쇠:
  - [ ] OK    Fix: 

- **`coord`**
  - EN: Coord:
  - KO: 좌표:
  - [ ] OK    Fix: 

- **`curve`**
  - EN: Curve:
  - KO: 곡선:
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Delay/Latency:
  - KO: 지연/레이턴시:
  - [ ] OK    Fix: 

- **`destX`**
  - EN: Dest. X:
  - KO: 목적지 X:
  - [ ] OK    Fix: 

- **`destY`**
  - EN: Dest. Y:
  - KO: 목적지 Y:
  - [ ] OK    Fix: 

- **`destZ`**
  - EN: Dest. Z:
  - KO: 목적지 Z:
  - [ ] OK    Fix: 

- **`diffusion`**
  - EN: Diffusion:
  - KO: 확산:
  - [ ] OK    Fix: 

- **`directivity`**
  - EN: Directivity:
  - KO: 지향성:
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Distance Atten:
  - KO: 거리 감쇠:
  - [ ] OK    Fix: 

- **`distanceRatio`**
  - EN: Distance Ratio:
  - KO: 거리 비율:
  - [ ] OK    Fix: 

- **`duration`**
  - EN: Duration:
  - KO: 지속 시간:
  - [ ] OK    Fix: 

- **`frequency`**
  - EN: Frequency:
  - KO: 주파수:
  - [ ] OK    Fix: 

- **`fringe`**
  - EN: Fringe:
  - KO: 프린지:
  - [ ] OK    Fix: 

- **`gain`**
  - EN: Gain:
  - KO: 게인:
  - [ ] OK    Fix: 

- **`gyrophone`**
  - EN: Gyrophone:
  - KO: 자이로폰:
  - [ ] OK    Fix: 

- **`heightFactor`**
  - EN: Height Factor:
  - KO: 높이 계수:
  - [ ] OK    Fix: 

- **`hfShelf`**
  - EN: HF Shelf:
  - KO: 고주파 쉘프:
  - [ ] OK    Fix: 

- **`jitter`**
  - EN: Jitter:
  - KO: 지터:
  - [ ] OK    Fix: 

- **`max`**
  - EN: Max:
  - KO: 최대:
  - [ ] OK    Fix: 

- **`maxSpeed`**
  - EN: Max Speed:
  - KO: 최대 속도:
  - [ ] OK    Fix: 

- **`min`**
  - EN: Min:
  - KO: 최소:
  - [ ] OK    Fix: 

- **`muteMacros`**
  - EN: Mute Macros:
  - KO: 음소거 매크로:
  - [ ] OK    Fix: 

- **`name`**
  - EN: Name:
  - KO: 이름:
  - [ ] OK    Fix: 

- **`offsetX`**
  - EN: Offset X:
  - KO: 오프셋 X:
  - [ ] OK    Fix: 

- **`offsetY`**
  - EN: Offset Y:
  - KO: 오프셋 Y:
  - [ ] OK    Fix: 

- **`offsetZ`**
  - EN: Offset Z:
  - KO: 오프셋 Z:
  - [ ] OK    Fix: 

- **`outX`**
  - EN: Out X:
  - KO: 출력 X:
  - [ ] OK    Fix: 

- **`outY`**
  - EN: Out Y:
  - KO: 출력 Y:
  - [ ] OK    Fix: 

- **`outZ`**
  - EN: Out Z:
  - KO: 출력 Z:
  - [ ] OK    Fix: 

- **`peakRatio`**
  - EN: Peak Ratio:
  - KO: 피크 비율:
  - [ ] OK    Fix: 

- **`peakThreshold`**
  - EN: Peak Threshold:
  - KO: 피크 임계값:
  - [ ] OK    Fix: 

- **`period`**
  - EN: Period:
  - KO: 주기:
  - [ ] OK    Fix: 

- **`phase`**
  - EN: Phase:
  - KO: 위상:
  - [ ] OK    Fix: 

- **`phaseX`**
  - EN: Phase X:
  - KO: 위상 X:
  - [ ] OK    Fix: 

- **`phaseY`**
  - EN: Phase Y:
  - KO: 위상 Y:
  - [ ] OK    Fix: 

- **`phaseZ`**
  - EN: Phase Z:
  - KO: 위상 Z:
  - [ ] OK    Fix: 

- **`positionX`**
  - EN: Position X:
  - KO: 위치 X:
  - [ ] OK    Fix: 

- **`positionY`**
  - EN: Position Y:
  - KO: 위치 Y:
  - [ ] OK    Fix: 

- **`positionZ`**
  - EN: Position Z:
  - KO: 위치 Z:
  - [ ] OK    Fix: 

- **`radius`**
  - EN: Radius:
  - KO: 반지름:
  - [ ] OK    Fix: 

- **`rateX`**
  - EN: Rate X:
  - KO: 레이트 X:
  - [ ] OK    Fix: 

- **`rateY`**
  - EN: Rate Y:
  - KO: 레이트 Y:
  - [ ] OK    Fix: 

- **`rateZ`**
  - EN: Rate Z:
  - KO: 레이트 Z:
  - [ ] OK    Fix: 

- **`reset`**
  - EN: Reset:
  - KO: 초기화:
  - [ ] OK    Fix: 

- **`rotation`**
  - EN: Rotation:
  - KO: 회전:
  - [ ] OK    Fix: 

- **`shape`**
  - EN: Shape:
  - KO: 형태:
  - [ ] OK    Fix: 

- **`shapeX`**
  - EN: Shape X:
  - KO: 형태 X:
  - [ ] OK    Fix: 

- **`shapeY`**
  - EN: Shape Y:
  - KO: 형태 Y:
  - [ ] OK    Fix: 

- **`shapeZ`**
  - EN: Shape Z:
  - KO: 형태 Z:
  - [ ] OK    Fix: 

- **`slope`**
  - EN: Slope:
  - KO: 기울기:
  - [ ] OK    Fix: 

- **`slowRatio`**
  - EN: Slow Ratio:
  - KO: 슬로우 비율:
  - [ ] OK    Fix: 

- **`slowThreshold`**
  - EN: Slow Threshold:
  - KO: 슬로우 임계값:
  - [ ] OK    Fix: 

- **`speedProfile`**
  - EN: Speed Profile:
  - KO: 속도 프로필:
  - [ ] OK    Fix: 

- **`threshold`**
  - EN: Threshold:
  - KO: 임계값:
  - [ ] OK    Fix: 

- **`tilt`**
  - EN: Tilt:
  - KO: 기울기:
  - [ ] OK    Fix: 

- **`trackingId`**
  - EN: Tracking ID:
  - KO: 트래킹 ID:
  - [ ] OK    Fix: 

- **`trackingSmooth`**
  - EN: Tracking Smooth:
  - KO: 트래킹 스무딩:
  - [ ] OK    Fix: 

- **`xyJoystick`**
  - EN: X/Y
  - KO: X/Y
  - [ ] OK    Fix: 

- **`zSlider`**
  - EN: Z
  - KO: Z
  - [ ] OK    Fix: 

## `inputs.lfo`

- **`gyrophone.antiClockwise`**
  - EN: Anti-Clockwise
  - KO: 반시계 방향
  - [ ] OK    Fix: 

- **`gyrophone.clockwise`**
  - EN: Clockwise
  - KO: 시계 방향
  - [ ] OK    Fix: 

- **`gyrophone.off`**
  - EN: OFF
  - KO: 끄기
  - [ ] OK    Fix: 

- **`shapes.exp`**
  - EN: exp
  - KO: 지수
  - [ ] OK    Fix: 

- **`shapes.keystone`**
  - EN: keystone
  - KO: 키스톤
  - [ ] OK    Fix: 

- **`shapes.log`**
  - EN: log
  - KO: 로그
  - [ ] OK    Fix: 

- **`shapes.off`**
  - EN: OFF
  - KO: 끄기
  - [ ] OK    Fix: 

- **`shapes.random`**
  - EN: random
  - KO: 랜덤
  - [ ] OK    Fix: 

- **`shapes.sawtooth`**
  - EN: sawtooth
  - KO: 톱니
  - [ ] OK    Fix: 

- **`shapes.sine`**
  - EN: sine
  - KO: 사인
  - [ ] OK    Fix: 

- **`shapes.square`**
  - EN: square
  - KO: 사각
  - [ ] OK    Fix: 

- **`shapes.triangle`**
  - EN: triangle
  - KO: 삼각
  - [ ] OK    Fix: 

## `inputs.liveSource`

- **`linear`**
  - EN: linear
  - KO: 선형
  - [ ] OK    Fix: 

- **`log`**
  - EN: log
  - KO: 로그
  - [ ] OK    Fix: 

- **`sine`**
  - EN: sine
  - KO: 사인
  - [ ] OK    Fix: 

## `inputs.messages`

- **`assignedCluster`**
  - EN: Input {channel} assigned to Cluster {cluster}
  - KO: 입력 {channel}이(가) 클러스터 {cluster}에 할당됨
  - [ ] OK    Fix: 

- **`backupLoaded`**
  - EN: Input configuration loaded from backup.
  - KO: 백업에서 입력 설정이 로드되었습니다.
  - [ ] OK    Fix: 

- **`configExported`**
  - EN: Input configuration exported.
  - KO: 입력 설정을 내보냈습니다.
  - [ ] OK    Fix: 

- **`configImported`**
  - EN: Input configuration imported.
  - KO: 입력 설정을 가져왔습니다.
  - [ ] OK    Fix: 

- **`configLoaded`**
  - EN: Input configuration loaded.
  - KO: 입력 설정이 로드되었습니다.
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Input configuration saved.
  - KO: 입력 설정이 저장되었습니다.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - KO: 오류: {error}
  - [ ] OK    Fix: 

- **`noSnapshotSelected`**
  - EN: No snapshot selected.
  - KO: 스냅샷이 선택되지 않았습니다.
  - [ ] OK    Fix: 

- **`scopeConfigured`**
  - EN: Scope configured for next snapshot.
  - KO: 다음 스냅샷을 위한 범위가 구성되었습니다.
  - [ ] OK    Fix: 

- **`scopeSaved`**
  - EN: Snapshot scope saved.
  - KO: 스냅샷 범위가 저장되었습니다.
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - KO: 시스템 설정에서 먼저 프로젝트 폴더를 선택하세요.
  - [ ] OK    Fix: 

- **`setSingle`**
  - EN: Input {channel} set to Single
  - KO: 입력 {channel}이(가) 단일로 설정됨
  - [ ] OK    Fix: 

- **`snapshotDeleted`**
  - EN: Snapshot '{name}' deleted.
  - KO: 스냅샷 '{name}'이(가) 삭제되었습니다.
  - [ ] OK    Fix: 

- **`snapshotLoaded`**
  - EN: Snapshot '{name}' loaded.
  - KO: 스냅샷 '{name}'이(가) 로드되었습니다.
  - [ ] OK    Fix: 

- **`snapshotLoadedWithoutScope`**
  - EN: Snapshot '{name}' loaded (without scope).
  - KO: 스냅샷 '{name}'이(가) 로드되었습니다 (범위 없이).
  - [ ] OK    Fix: 

- **`snapshotStored`**
  - EN: Snapshot '{name}' stored.
  - KO: 스냅샷 '{name}'이(가) 저장되었습니다.
  - [ ] OK    Fix: 

- **`snapshotUpdated`**
  - EN: Snapshot '{name}' updated.
  - KO: 스냅샷 '{name}'이(가) 업데이트되었습니다.
  - [ ] OK    Fix: 

- **`trackingDisabled`**
  - EN: Tracking disabled for Input {channel}
  - KO: 입력 {channel}의 트래킹이 비활성화됨
  - [ ] OK    Fix: 

- **`trackingSwitched`**
  - EN: Tracking switched from Input {from} to Input {to}
  - KO: 트래킹이 입력 {from}에서 입력 {to}(으)로 전환됨
  - [ ] OK    Fix: 

## `inputs.muteMacros`

- **`invertMutes`**
  - EN: INVERT MUTES
  - KO: 음소거 반전
  - [ ] OK    Fix: 

- **`muteAll`**
  - EN: MUTE ALL
  - KO: 전체 음소거
  - [ ] OK    Fix: 

- **`muteArrayPrefix`**
  - EN: MUTE ARRAY
  - KO: 어레이 음소거
  - [ ] OK    Fix: 

- **`muteEven`**
  - EN: MUTE EVEN
  - KO: 짝수 음소거
  - [ ] OK    Fix: 

- **`muteOdd`**
  - EN: MUTE ODD
  - KO: 홀수 음소거
  - [ ] OK    Fix: 

- **`selectMacro`**
  - EN: Select Macro...
  - KO: 매크로 선택...
  - [ ] OK    Fix: 

- **`unmuteAll`**
  - EN: UNMUTE ALL
  - KO: 전체 음소거 해제
  - [ ] OK    Fix: 

- **`unmuteArrayPrefix`**
  - EN: UNMUTE ARRAY
  - KO: 어레이 음소거 해제
  - [ ] OK    Fix: 

## `inputs.prefixes`

- **`delay`**
  - EN: Delay:
  - KO: 지연:
  - [ ] OK    Fix: 

## `inputs.sections`

- **`automotion`**
  - EN: AutomOtion
  - KO: AutomOtion (자동이동)
  - [ ] OK    Fix: 

## `inputs.snapshots`

- **`selectSnapshot`**
  - EN: Select Snapshot...
  - KO: 스냅샷 선택...
  - [ ] OK    Fix: 

## `inputs.tabs`

- **`gradientMaps`**
  - EN: Gradient Maps
  - KO: 그라디언트 맵
  - [ ] OK    Fix: 

- **`inputParams`**
  - EN: Input Parameters
  - KO: 입력 매개변수
  - [ ] OK    Fix: 

- **`liveSourceHackoustics`**
  - EN: Live Source & Hackoustics
  - KO: 라이브 소스 & Hackoustics (가상음향)
  - [ ] OK    Fix: 

- **`movements`**
  - EN: Movements
  - KO: 움직임
  - [ ] OK    Fix: 

- **`sampler`**
  - EN: Sampler
  - KO: Sampler
  - [ ] OK    Fix: 

- **`visualisation`**
  - EN: Visualisation
  - KO: 시각화
  - [ ] OK    Fix: 

## `inputs.toggles`

- **`absolute`**
  - EN: Absolute
  - KO: 절대
  - [ ] OK    Fix: 

- **`acousticPrecedence`**
  - EN: Acoustic Precedence
  - KO: 음향 우선
  - [ ] OK    Fix: 

- **`attenuationLawLog`**
  - EN: Log
  - KO: 로그
  - [ ] OK    Fix: 

- **`constraintROff`**
  - EN: Constraint R: OFF
  - KO: 제한 R: 끄기
  - [ ] OK    Fix: 

- **`constraintROn`**
  - EN: Constraint R: ON
  - KO: 제한 R: 켜기
  - [ ] OK    Fix: 

- **`constraintXOff`**
  - EN: Constraint X: OFF
  - KO: 제한 X: 끄기
  - [ ] OK    Fix: 

- **`constraintXOn`**
  - EN: Constraint X: ON
  - KO: 제한 X: 켜기
  - [ ] OK    Fix: 

- **`constraintYOff`**
  - EN: Constraint Y: OFF
  - KO: 제한 Y: 끄기
  - [ ] OK    Fix: 

- **`constraintYOn`**
  - EN: Constraint Y: ON
  - KO: 제한 Y: 켜기
  - [ ] OK    Fix: 

- **`constraintZOff`**
  - EN: Constraint Z: OFF
  - KO: 제한 Z: 끄기
  - [ ] OK    Fix: 

- **`constraintZOn`**
  - EN: Constraint Z: ON
  - KO: 제한 Z: 켜기
  - [ ] OK    Fix: 

- **`flipXOff`**
  - EN: Flip X: OFF
  - KO: X 반전: 끄기
  - [ ] OK    Fix: 

- **`flipXOn`**
  - EN: Flip X: ON
  - KO: X 반전: 켜기
  - [ ] OK    Fix: 

- **`flipYOff`**
  - EN: Flip Y: OFF
  - KO: Y 반전: 끄기
  - [ ] OK    Fix: 

- **`flipYOn`**
  - EN: Flip Y: ON
  - KO: Y 반전: 켜기
  - [ ] OK    Fix: 

- **`flipZOff`**
  - EN: Flip Z: OFF
  - KO: Z 반전: 끄기
  - [ ] OK    Fix: 

- **`flipZOn`**
  - EN: Flip Z: ON
  - KO: Z 반전: 켜기
  - [ ] OK    Fix: 

- **`floorReflectionsOff`**
  - EN: Floor Reflections: OFF
  - KO: 바닥 반사: 끄기
  - [ ] OK    Fix: 

- **`floorReflectionsOn`**
  - EN: Floor Reflections: ON
  - KO: 바닥 반사: 켜기
  - [ ] OK    Fix: 

- **`highShelfOff`**
  - EN: High Shelf: OFF
  - KO: 하이 쉘프: 끄기
  - [ ] OK    Fix: 

- **`highShelfOn`**
  - EN: High Shelf: ON
  - KO: 하이 쉘프: 켜기
  - [ ] OK    Fix: 

- **`lfoOff`**
  - EN: L.F.O: OFF
  - KO: L.F.O: 끄기
  - [ ] OK    Fix: 

- **`lfoOn`**
  - EN: L.F.O: ON
  - KO: L.F.O: 켜기
  - [ ] OK    Fix: 

- **`liveSourceTamerOff`**
  - EN: Live Source Tamer: OFF
  - KO: 테이머: OFF
  - [ ] OK    Fix: 

- **`liveSourceTamerOn`**
  - EN: Live Source Tamer: ON
  - KO: 테이머: ON
  - [ ] OK    Fix: 

- **`lowCutOff`**
  - EN: Low Cut: OFF
  - KO: 로우 컷: 끄기
  - [ ] OK    Fix: 

- **`lowCutOn`**
  - EN: Low Cut: ON
  - KO: 로우 컷: 켜기
  - [ ] OK    Fix: 

- **`lsPeakOff`**
  - EN: Peak: OFF
  - KO: Peak: OFF
  - [ ] OK    Fix: 

- **`lsPeakOn`**
  - EN: Peak: ON
  - KO: Peak: ON
  - [ ] OK    Fix: 

- **`lsSlowOff`**
  - EN: Slow: OFF
  - KO: Slow: OFF
  - [ ] OK    Fix: 

- **`lsSlowOn`**
  - EN: Slow: ON
  - KO: Slow: ON
  - [ ] OK    Fix: 

- **`manual`**
  - EN: Manual
  - KO: 수동
  - [ ] OK    Fix: 

- **`maxSpeedOff`**
  - EN: Max Speed: OFF
  - KO: 최대 속도: 끄기
  - [ ] OK    Fix: 

- **`maxSpeedOn`**
  - EN: Max Speed: ON
  - KO: 최대 속도: 켜기
  - [ ] OK    Fix: 

- **`minimalLatency`**
  - EN: Minimal Latency
  - KO: 최소 레이턴시
  - [ ] OK    Fix: 

- **`pathModeOff`**
  - EN: Path Mode: OFF
  - KO: 경로 모드: 끄기
  - [ ] OK    Fix: 

- **`pathModeOn`**
  - EN: Path Mode: ON
  - KO: 경로 모드: 켜기
  - [ ] OK    Fix: 

- **`relative`**
  - EN: Relative
  - KO: 상대
  - [ ] OK    Fix: 

- **`return`**
  - EN: Return
  - KO: 복귀
  - [ ] OK    Fix: 

- **`reverbSendsMuted`**
  - EN: Sends to Reverbs: Muted
  - KO: 리버브 센드: 음소거
  - [ ] OK    Fix: 

- **`reverbSendsUnmuted`**
  - EN: Sends to Reverbs: Unmuted
  - KO: 리버브 센드: 활성화
  - [ ] OK    Fix: 

- **`sidelinesOff`**
  - EN: Sidelines Off
  - KO: 사이드라인 끄기
  - [ ] OK    Fix: 

- **`sidelinesOn`**
  - EN: Sidelines On
  - KO: 사이드라인 켜기
  - [ ] OK    Fix: 

- **`stay`**
  - EN: Stay
  - KO: 유지
  - [ ] OK    Fix: 

- **`trackingOff`**
  - EN: Tracking: OFF
  - KO: 트래킹: 끄기
  - [ ] OK    Fix: 

- **`trackingOn`**
  - EN: Tracking: ON
  - KO: 트래킹: 켜기
  - [ ] OK    Fix: 

- **`triggered`**
  - EN: Triggered
  - KO: 트리거
  - [ ] OK    Fix: 

## `inputs.visualisation`

- **`delay`**
  - EN: delay
  - KO: 지연
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: HF
damping
  - KO: HF
감쇠
  - [ ] OK    Fix: 

- **`level`**
  - EN: level
  - KO: 레벨
  - [ ] OK    Fix: 

## `levelMeter`

- **`inputs`**
  - EN: Inputs
  - KO: 입력
  - [ ] OK    Fix: 

- **`outputs`**
  - EN: Outputs
  - KO: 출력
  - [ ] OK    Fix: 

- **`windowTitle`**
  - EN: Level Meters
  - KO: 레벨 미터
  - [ ] OK    Fix: 

## `levelMeter.buttons`

- **`clearSolo`**
  - EN: Clear Solo
  - KO: 솔로 해제
  - [ ] OK    Fix: 

- **`soloModeMulti`**
  - EN: Multi
  - KO: Multi
  - [ ] OK    Fix: 

- **`soloModeSingle`**
  - EN: Single
  - KO: Single
  - [ ] OK    Fix: 

## `levelMeter.tooltips`

- **`clearSolo`**
  - EN: Disengage all solo toggles
  - KO: 모든 솔로 비활성화
  - [ ] OK    Fix: 

- **`solo`**
  - EN: Display the contribution of the input to all outputs in the Level Meter display (in Single mode) and play binaural render of soloed inputs
  - KO: 레벨 미터 디스플레이에서 입력의 모든 출력에 대한 기여도 표시(싱글 모드) 및 솔로 입력의 바이노럴 렌더링 재생
  - [ ] OK    Fix: 

- **`soloMode`**
  - EN: Single: one input at a time. Multi: multiple inputs simultaneously.
  - KO: 단일: 한 번에 하나의 입력. 다중: 여러 입력 동시에.
  - [ ] OK    Fix: 

## `map`

- **`detachedMessage`**
  - EN: The map is displayed in a separate window.
  - KO: 맵이 별도의 창에 표시되고 있습니다.
  - [ ] OK    Fix: 

- **`reattach`**
  - EN: Re-attach Map
  - KO: 맵 다시 붙이기
  - [ ] OK    Fix: 

## `map.buttons`

- **`fitInputs`**
  - EN: Fit All Inputs to Screen
  - KO: 모든 입력을 화면에 맞추기
  - [ ] OK    Fix: 

- **`fitStage`**
  - EN: Fit Stage to Screen
  - KO: 무대를 화면에 맞추기
  - [ ] OK    Fix: 

- **`hideLevels`**
  - EN: Hide Levels
  - KO: 레벨 숨기기
  - [ ] OK    Fix: 

- **`showLevels`**
  - EN: Show Levels
  - KO: 레벨 표시
  - [ ] OK    Fix: 

## `map.labels`

- **`reverbPrefix`**
  - EN: R
  - KO: 리
  - [ ] OK    Fix: 

## `map.messages`

- **`assignedCluster`**
  - EN: Input {channel} assigned to Cluster {cluster}
  - KO: 입력 {channel}이(가) 클러스터 {cluster}에 할당되었습니다
  - [ ] OK    Fix: 

- **`assignedClusterMulti`**
  - EN: {count} inputs assigned to Cluster {cluster}
  - KO: {count}개의 입력이 클러스터 {cluster}에 할당되었습니다
  - [ ] OK    Fix: 

- **`clusterBrokenUp`**
  - EN: Cluster {cluster} broken up
  - KO: 클러스터 {cluster}이(가) 해체되었습니다
  - [ ] OK    Fix: 

- **`setSingle`**
  - EN: Input {channel} removed from cluster
  - KO: 입력 {channel}이(가) 클러스터에서 제거되었습니다
  - [ ] OK    Fix: 

- **`setSingleMulti`**
  - EN: {count} inputs removed from clusters
  - KO: {count}개의 입력이 클러스터에서 제거되었습니다
  - [ ] OK    Fix: 

## `map.tooltips`

- **`detach`**
  - EN: Detach the map into a separate window for dual-screen setups
  - KO: 듀얼 스크린 설정을 위해 맵을 별도 창으로 분리
  - [ ] OK    Fix: 

- **`fitInputs`**
  - EN: Adjust the zoom and panning to fit all visible inputs on the display
  - KO: 모든 보이는 입력이 표시되도록 확대/축소 및 이동 조정
  - [ ] OK    Fix: 

- **`fitStage`**
  - EN: Adjust the zoom and panning to fit the stage on the display
  - KO: 무대가 표시되도록 확대/축소 및 이동 조정
  - [ ] OK    Fix: 

- **`levels`**
  - EN: Display the levels for inputs and outputs on the map
  - KO: 맵에 입력 및 출력 레벨 표시
  - [ ] OK    Fix: 

## `network.buttons`

- **`add`**
  - EN: ADD
  - KO: 추가
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - KO: 내보내기
  - [ ] OK    Fix: 

- **`findMyRemote`**
  - EN: Find My Remote
  - KO: 리모컨 찾기
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - KO: 가져오기
  - [ ] OK    Fix: 

- **`openLogWindow`**
  - EN: Open Log Window
  - KO: 로그 창 열기
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Backup
  - KO: 백업 불러오기
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Network Config
  - KO: 네트워크 설정 다시 불러오기
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Network Config
  - KO: 네트워크 설정 저장
  - [ ] OK    Fix: 

## `network.dialogs`

- **`exportConfig`**
  - EN: Export Network Configuration
  - KO: 네트워크 설정 내보내기
  - [ ] OK    Fix: 

- **`findMyRemoteMessage`**
  - EN: Enter the password for your remote device:
  - KO: 리모컨 장치의 비밀번호를 입력하세요:
  - [ ] OK    Fix: 

- **`findMyRemotePassword`**
  - EN: Password:
  - KO: 비밀번호:
  - [ ] OK    Fix: 

- **`findMyRemoteTitle`**
  - EN: Find My Remote
  - KO: 리모컨 찾기
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Network Configuration
  - KO: 네트워크 설정 가져오기
  - [ ] OK    Fix: 

- **`removeTargetMessage`**
  - EN: Delete target '{name}'?
  - KO: 대상 '{name}'을(를) 삭제하시겠습니까?
  - [ ] OK    Fix: 

- **`removeTargetTitle`**
  - EN: Remove Target
  - KO: 대상 제거
  - [ ] OK    Fix: 

- **`trackingConflictsContinue`**
  - EN: Continue
  - KO: 계속
  - [ ] OK    Fix: 

- **`trackingConflictsMessage`**
  - EN: 
Only one tracked input per cluster is allowed. If you continue, tracking will be kept only for the first input in each cluster.
  - KO: 
클러스터당 하나의 트래킹 입력만 허용됩니다. 계속하면 각 클러스터의 첫 번째 입력에 대해서만 트래킹이 유지됩니다.
  - [ ] OK    Fix: 

- **`trackingConflictsTitle`**
  - EN: Tracking Conflicts Detected
  - KO: 트래킹 충돌 감지됨
  - [ ] OK    Fix: 

## `network.help`

- **`addTarget`**
  - EN: Add new network target.
  - KO: 새 네트워크 대상 추가.
  - [ ] OK    Fix: 

- **`admMapping`**
  - EN: Select an ADM-OSC mapping to configure. Cart = Cartesian (xyz), Polar = spherical (aed).
  - KO: 구성할 ADM-OSC 매핑을 선택합니다. Cart = 데카르트 좌표 (xyz), Polar = 구면 좌표 (aed).
  - [ ] OK    Fix: 

- **`admMappingPanel`**
  - EN: Drag dots to edit mapping. Click axis title to swap, click Flip to invert. Hold Shift to edit both sides symmetrically.
  - KO: 점을 드래그하여 매핑 편집. 축 제목을 클릭하여 교체, Flip을 클릭하여 반전. Shift를 누른 채로 양쪽을 대칭으로 편집.
  - [ ] OK    Fix: 

- **`currentIP`**
  - EN: IP address of the Processor.
  - KO: 프로세서의 IP 주소.
  - [ ] OK    Fix: 

- **`dataMode`**
  - EN: Select UDP or TCP data transmission.
  - KO: UDP 또는 TCP 데이터 전송을 선택.
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Network Configuration to file.
  - KO: 네트워크 구성을 파일로 내보내기.
  - [ ] OK    Fix: 

- **`findMyRemote`**
  - EN: Make your Remote Flash and Buzz to Find it.
  - KO: 리모컨을 깜박이고 진동시켜 찾습니다.
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Network Configuration from file.
  - KO: 파일에서 네트워크 구성 가져오기.
  - [ ] OK    Fix: 

- **`networkInterface`**
  - EN: Select the Network Interface.
  - KO: 네트워크 인터페이스를 선택합니다.
  - [ ] OK    Fix: 

- **`openLogWindow`**
  - EN: Open Network Logging window.
  - KO: 네트워크 로그 창 열기.
  - [ ] OK    Fix: 

- **`oscQueryEnable`**
  - EN: Enable/disable OSC Query server for automatic parameter discovery via HTTP/WebSocket.
  - KO: HTTP/WebSocket을 통한 자동 파라미터 검색을 위해 OSC Query 서버를 활성화/비활성화합니다.
  - [ ] OK    Fix: 

- **`oscQueryPort`**
  - EN: HTTP port for OSC Query discovery. Other apps can browse parameters at http://localhost:<port>/
  - KO: OSC Query 검색용 HTTP 포트. 다른 앱은 http://localhost:<port>/에서 파라미터를 탐색할 수 있습니다.
  - [ ] OK    Fix: 

- **`oscSourceFilter`**
  - EN: Filter incoming OSC: Accept All sources or only Registered connections with Rx enabled.
  - KO: 수신 OSC 필터링: 모든 소스 수락 또는 Rx 활성화된 등록된 연결만.
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Select the Protocol: DISABLED, OSC, REMOTE, or ADM-OSC.
  - KO: 프로토콜 선택: DISABLED, OSC, REMOTE 또는 ADM-OSC.
  - [ ] OK    Fix: 

- **`psnInterface`**
  - EN: Network interface for PSN multicast reception
  - KO: PSN 멀티캐스트 수신용 네트워크 인터페이스
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Network Configuration from backup file.
  - KO: 백업 파일에서 네트워크 구성 다시 불러오기.
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Network Configuration from file.
  - KO: 파일에서 네트워크 구성 다시 불러오기.
  - [ ] OK    Fix: 

- **`removeTarget`**
  - EN: Delete this Network Target.
  - KO: 이 네트워크 대상 삭제.
  - [ ] OK    Fix: 

- **`rxEnable`**
  - EN: Enable or Disable Data Reception.
  - KO: 데이터 수신을 활성화 또는 비활성화합니다.
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Network Configuration to file.
  - KO: 네트워크 구성을 파일에 저장.
  - [ ] OK    Fix: 

- **`targetIP`**
  - EN: IP Address of the Target (use 127.0.0.1 for local host).
  - KO: 대상의 IP 주소 (로컬 호스트는 127.0.0.1 사용).
  - [ ] OK    Fix: 

- **`targetName`**
  - EN: Network Target Name.
  - KO: 네트워크 대상 이름.
  - [ ] OK    Fix: 

- **`tcpPort`**
  - EN: TCP Receive Port of the Processor.
  - KO: 프로세서의 TCP 수신 포트.
  - [ ] OK    Fix: 

- **`trackingEnabled`**
  - EN: Enable or Disable Incoming Tracking data processing.
  - KO: 수신 트래킹 데이터 처리를 활성화 또는 비활성화합니다.
  - [ ] OK    Fix: 

- **`trackingFlipX`**
  - EN: Invert Axis of Tracking X Coordinate.
  - KO: 트래킹 X 좌표 축 반전.
  - [ ] OK    Fix: 

- **`trackingFlipY`**
  - EN: Invert Axis of Tracking Y Coordinate.
  - KO: 트래킹 Y 좌표 축 반전.
  - [ ] OK    Fix: 

- **`trackingFlipZ`**
  - EN: Invert Axis of Tracking Z Coordinate.
  - KO: 트래킹 Z 좌표 축 반전.
  - [ ] OK    Fix: 

- **`trackingOffsetX`**
  - EN: Offset Tracking X Coordinate.
  - KO: 트래킹 X 좌표 오프셋.
  - [ ] OK    Fix: 

- **`trackingOffsetY`**
  - EN: Offset Tracking Y Coordinate.
  - KO: 트래킹 Y 좌표 오프셋.
  - [ ] OK    Fix: 

- **`trackingOffsetZ`**
  - EN: Offset Tracking Z Coordinate.
  - KO: 트래킹 Z 좌표 오프셋.
  - [ ] OK    Fix: 

- **`trackingOscPath`**
  - EN: OSC Path for the Tracking in OSC Mode (starts with a /)
  - KO: OSC 모드에서의 트래킹 OSC 경로 (/로 시작)
  - [ ] OK    Fix: 

- **`trackingPort`**
  - EN: Specify the Port to receive Tracking data.
  - KO: 트래킹 데이터를 수신할 포트를 지정.
  - [ ] OK    Fix: 

- **`trackingProtocol`**
  - EN: Select the type of Tracking Protocol.
  - KO: 트래킹 프로토콜 유형 선택.
  - [ ] OK    Fix: 

- **`trackingScaleX`**
  - EN: Scale Tracking X Coordinate.
  - KO: 트래킹 X 좌표 스케일.
  - [ ] OK    Fix: 

- **`trackingScaleY`**
  - EN: Scale Tracking Y Coordinate.
  - KO: 트래킹 Y 좌표 스케일.
  - [ ] OK    Fix: 

- **`trackingScaleZ`**
  - EN: Scale Tracking Z Coordinate.
  - KO: 트래킹 Z 좌표 스케일.
  - [ ] OK    Fix: 

- **`txEnable`**
  - EN: Enable or Disable Data Transmission.
  - KO: 데이터 전송을 활성화 또는 비활성화합니다.
  - [ ] OK    Fix: 

- **`txPort`**
  - EN: Transmit Port for this Target.
  - KO: 이 대상의 전송 포트.
  - [ ] OK    Fix: 

- **`udpPort`**
  - EN: UDP Receive Port of the Processor.
  - KO: 프로세서의 UDP 수신 포트.
  - [ ] OK    Fix: 

## `network.labels`

- **`admMapping`**
  - EN: Mapping:
  - KO: 매핑:
  - [ ] OK    Fix: 

- **`currentIPv4`**
  - EN: Current IPv4:
  - KO: 현재 IPv4:
  - [ ] OK    Fix: 

- **`interface`**
  - EN: Network Interface:
  - KO: 네트워크 인터페이스:
  - [ ] OK    Fix: 

- **`localhost`**
  - EN: Localhost (127.0.0.1)
  - KO: Localhost (127.0.0.1)
  - [ ] OK    Fix: 

- **`mqttHost`**
  - EN: Host:
  - KO: 호스트:
  - [ ] OK    Fix: 

- **`mqttJsonQ`**
  - EN: Q:
  - KO: Q:
  - [ ] OK    Fix: 

- **`mqttJsonX`**
  - EN: X:
  - KO: X:
  - [ ] OK    Fix: 

- **`mqttJsonY`**
  - EN: Y:
  - KO: Y:
  - [ ] OK    Fix: 

- **`mqttJsonZ`**
  - EN: Z:
  - KO: Z:
  - [ ] OK    Fix: 

- **`mqttTagIds`**
  - EN: Tag IDs...
  - KO: 태그 ID...
  - [ ] OK    Fix: 

- **`mqttTopic`**
  - EN: Topic:
  - KO: 토픽:
  - [ ] OK    Fix: 

- **`notAvailable`**
  - EN: Not available
  - KO: 사용할 수 없음
  - [ ] OK    Fix: 

- **`offsetX`**
  - EN: Offset X:
  - KO: 오프셋 X:
  - [ ] OK    Fix: 

- **`offsetY`**
  - EN: Offset Y:
  - KO: 오프셋 Y:
  - [ ] OK    Fix: 

- **`offsetZ`**
  - EN: Offset Z:
  - KO: 오프셋 Z:
  - [ ] OK    Fix: 

- **`oscPath`**
  - EN: OSC Path:
  - KO: OSC Path:
  - [ ] OK    Fix: 

- **`oscQuery`**
  - EN: OSC Query:
  - KO: OSC 쿼리:
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Protocol:
  - KO: 프로토콜:
  - [ ] OK    Fix: 

- **`psnInterface`**
  - EN: PSN Interface:
  - KO: PSN Interface:
  - [ ] OK    Fix: 

- **`rxPort`**
  - EN: Rx Port:
  - KO: 수신 포트:
  - [ ] OK    Fix: 

- **`scaleX`**
  - EN: Scale X:
  - KO: 스케일 X:
  - [ ] OK    Fix: 

- **`scaleY`**
  - EN: Scale Y:
  - KO: 스케일 Y:
  - [ ] OK    Fix: 

- **`scaleZ`**
  - EN: Scale Z:
  - KO: 스케일 Z:
  - [ ] OK    Fix: 

- **`tcpPort`**
  - EN: TCP Port:
  - KO: TCP 포트:
  - [ ] OK    Fix: 

- **`udpPort`**
  - EN: UDP Port:
  - KO: UDP 포트:
  - [ ] OK    Fix: 

## `network.messages`

- **`configExported`**
  - EN: Network configuration exported.
  - KO: 네트워크 설정을 내보냈습니다.
  - [ ] OK    Fix: 

- **`configImported`**
  - EN: Network configuration imported.
  - KO: 네트워크 설정을 가져왔습니다.
  - [ ] OK    Fix: 

- **`configLoadedFromBackup`**
  - EN: Network configuration loaded from backup.
  - KO: 백업에서 네트워크 설정이 로드되었습니다.
  - [ ] OK    Fix: 

- **`configNotFound`**
  - EN: Network config file not found.
  - KO: 네트워크 설정 파일을 찾을 수 없습니다.
  - [ ] OK    Fix: 

- **`configReloaded`**
  - EN: Network configuration reloaded.
  - KO: 네트워크 설정이 다시 로드되었습니다.
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Network configuration saved.
  - KO: 네트워크 설정이 저장되었습니다.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - KO: 오류: {error}
  - [ ] OK    Fix: 

- **`findDeviceSent`**
  - EN: Find Device command sent.
  - KO: 장치 찾기 명령이 전송되었습니다.
  - [ ] OK    Fix: 

- **`maxTargetsReached`**
  - EN: Maximum Number of Targets/Servers Reached.
  - KO: 대상/서버의 최대 수에 도달했습니다.
  - [ ] OK    Fix: 

- **`noBackupFound`**
  - EN: No backup files found.
  - KO: 백업 파일을 찾을 수 없습니다.
  - [ ] OK    Fix: 

- **`onlyOneRemote`**
  - EN: Only one REMOTE connection is allowed.
  - KO: 리모트 연결은 하나만 허용됩니다.
  - [ ] OK    Fix: 

- **`oscManagerError`**
  - EN: Error: OSC Manager not available
  - KO: 오류: OSC 관리자를 사용할 수 없습니다
  - [ ] OK    Fix: 

- **`passwordEmpty`**
  - EN: Password cannot be empty.
  - KO: 비밀번호는 비워둘 수 없습니다.
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - KO: 시스템 설정에서 먼저 프로젝트 폴더를 선택하세요.
  - [ ] OK    Fix: 

## `network.protocols`

- **`admOsc`**
  - EN: ADM-OSC
  - KO: ADM-OSC
  - [ ] OK    Fix: 

- **`disabled`**
  - EN: DISABLED
  - KO: 비활성화
  - [ ] OK    Fix: 

- **`mqtt`**
  - EN: MQTT
  - KO: MQTT
  - [ ] OK    Fix: 

- **`osc`**
  - EN: OSC
  - KO: OSC
  - [ ] OK    Fix: 

- **`psn`**
  - EN: PosiStageNet (PSN)
  - KO: PosiStageNet (PSN)
  - [ ] OK    Fix: 

- **`qlab`**
  - EN: QLab
  - KO: QLab
  - [ ] OK    Fix: 

- **`remote`**
  - EN: Remote
  - KO: 리모트
  - [ ] OK    Fix: 

- **`rttrp`**
  - EN: RTTrP
  - KO: RTTrP
  - [ ] OK    Fix: 

- **`tcp`**
  - EN: TCP
  - KO: TCP
  - [ ] OK    Fix: 

- **`udp`**
  - EN: UDP
  - KO: UDP
  - [ ] OK    Fix: 

## `network.sections`

- **`admOsc`**
  - EN: ADM-OSC Mappings
  - KO: ADM-OSC
  - [ ] OK    Fix: 

- **`connections`**
  - EN: Network Connections
  - KO: 네트워크 연결
  - [ ] OK    Fix: 

- **`network`**
  - EN: Network
  - KO: 네트워크
  - [ ] OK    Fix: 

- **`tracking`**
  - EN: Tracking
  - KO: 트래킹
  - [ ] OK    Fix: 

## `network.table`

- **`defaultTarget`**
  - EN: Target {num}
  - KO: 대상 {num}
  - [ ] OK    Fix: 

- **`ipv4Address`**
  - EN: IPv4 Address
  - KO: IPv4 주소
  - [ ] OK    Fix: 

- **`mode`**
  - EN: Mode
  - KO: 모드
  - [ ] OK    Fix: 

- **`name`**
  - EN: Name
  - KO: 이름
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Protocol
  - KO: 프로토콜
  - [ ] OK    Fix: 

- **`rx`**
  - EN: Rx
  - KO: 수신
  - [ ] OK    Fix: 

- **`tx`**
  - EN: Tx
  - KO: 송신
  - [ ] OK    Fix: 

- **`txPort`**
  - EN: Tx Port
  - KO: 송신 포트
  - [ ] OK    Fix: 

## `network.toggles`

- **`disabled`**
  - EN: Disabled
  - KO: 비활성화됨
  - [ ] OK    Fix: 

- **`enabled`**
  - EN: Enabled
  - KO: 활성화됨
  - [ ] OK    Fix: 

- **`flipXOff`**
  - EN: Flip X: OFF
  - KO: X 반전: 끄기
  - [ ] OK    Fix: 

- **`flipXOn`**
  - EN: Flip X: ON
  - KO: X 반전: 켜기
  - [ ] OK    Fix: 

- **`flipYOff`**
  - EN: Flip Y: OFF
  - KO: Y 반전: 끄기
  - [ ] OK    Fix: 

- **`flipYOn`**
  - EN: Flip Y: ON
  - KO: Y 반전: 켜기
  - [ ] OK    Fix: 

- **`flipZOff`**
  - EN: Flip Z: OFF
  - KO: Z 반전: 끄기
  - [ ] OK    Fix: 

- **`flipZOn`**
  - EN: Flip Z: ON
  - KO: Z 반전: 켜기
  - [ ] OK    Fix: 

- **`off`**
  - EN: OFF
  - KO: 끄기
  - [ ] OK    Fix: 

- **`on`**
  - EN: ON
  - KO: 켜기
  - [ ] OK    Fix: 

- **`oscFilterAcceptAll`**
  - EN: OSC Filter: Accept All
  - KO: OSC 필터: 모두 허용
  - [ ] OK    Fix: 

- **`oscFilterRegisteredOnly`**
  - EN: OSC Filter: Registered Only
  - KO: OSC 필터: 등록된 것만
  - [ ] OK    Fix: 

- **`trackingOff`**
  - EN: Tracking: OFF
  - KO: 트래킹: 끄기
  - [ ] OK    Fix: 

- **`trackingOn`**
  - EN: Tracking: ON
  - KO: 트래킹: 켜기
  - [ ] OK    Fix: 

## `networkLog`

- **`windowTitle`**
  - EN: Network Log
  - KO: 네트워크 로그
  - [ ] OK    Fix: 

## `networkLog.columns`

- **`address`**
  - EN: Address
  - KO: 주소
  - [ ] OK    Fix: 

- **`arguments`**
  - EN: Arguments
  - KO: 인수
  - [ ] OK    Fix: 

- **`direction`**
  - EN: Dir
  - KO: 방향
  - [ ] OK    Fix: 

- **`ip`**
  - EN: IP
  - KO: IP
  - [ ] OK    Fix: 

- **`origin`**
  - EN: Origin
  - KO: 출처
  - [ ] OK    Fix: 

- **`port`**
  - EN: Port
  - KO: 포트
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Protocol
  - KO: 프로토콜
  - [ ] OK    Fix: 

- **`time`**
  - EN: Time
  - KO: 시간
  - [ ] OK    Fix: 

- **`transport`**
  - EN: Trans
  - KO: 전송
  - [ ] OK    Fix: 

## `networkLog.controls`

- **`clear`**
  - EN: CLEAR
  - KO: 지우기
  - [ ] OK    Fix: 

- **`export`**
  - EN: EXPORT
  - KO: 내보내기
  - [ ] OK    Fix: 

- **`hideHeartbeat`**
  - EN: Hide Heartbeat
  - KO: 하트비트 숨기기
  - [ ] OK    Fix: 

- **`logging`**
  - EN: Logging
  - KO: 로깅
  - [ ] OK    Fix: 

## `networkLog.dialogs`

- **`exportCompleteMessage`**
  - EN: Log exported to: {path}
  - KO: 로그가 내보내졌습니다: {path}
  - [ ] OK    Fix: 

- **`exportCompleteTitle`**
  - EN: Export Complete
  - KO: 내보내기 완료
  - [ ] OK    Fix: 

- **`exportFailedMessage`**
  - EN: Could not write to file: {path}
  - KO: 파일에 쓸 수 없습니다: {path}
  - [ ] OK    Fix: 

- **`exportFailedTitle`**
  - EN: Export Failed
  - KO: 내보내기 실패
  - [ ] OK    Fix: 

## `networkLog.exportMenu`

- **`exportAll`**
  - EN: Export All
  - KO: 전체 내보내기
  - [ ] OK    Fix: 

- **`exportFiltered`**
  - EN: Export Filtered
  - KO: 필터된 것 내보내기
  - [ ] OK    Fix: 

## `networkLog.filterModes`

- **`clientIp`**
  - EN: Client IP
  - KO: 클라이언트 IP
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Protocol
  - KO: 프로토콜
  - [ ] OK    Fix: 

- **`rejected`**
  - EN: Rejected
  - KO: 거부됨
  - [ ] OK    Fix: 

- **`tcpUdp`**
  - EN: TCP/UDP
  - KO: TCP/UDP
  - [ ] OK    Fix: 

## `networkLog.filterToggles`

- **`incoming`**
  - EN: Incoming
  - KO: 수신
  - [ ] OK    Fix: 

- **`outgoing`**
  - EN: Outgoing
  - KO: 송신
  - [ ] OK    Fix: 

- **`tcp`**
  - EN: TCP
  - KO: TCP
  - [ ] OK    Fix: 

- **`udp`**
  - EN: UDP
  - KO: UDP
  - [ ] OK    Fix: 

## `networkLog.status`

- **`rejected`**
  - EN: REJECTED
  - KO: 거부됨
  - [ ] OK    Fix: 

## `outputs.arrayModes`

- **`absolute`**
  - EN: ABSOLUTE
  - KO: 절대
  - [ ] OK    Fix: 

- **`array`**
  - EN: Array
  - KO: 어레이
  - [ ] OK    Fix: 

- **`off`**
  - EN: OFF
  - KO: 끄기
  - [ ] OK    Fix: 

- **`relative`**
  - EN: RELATIVE
  - KO: 상대
  - [ ] OK    Fix: 

- **`single`**
  - EN: Single
  - KO: 단일
  - [ ] OK    Fix: 

## `outputs.buttons`

- **`arrayHidden`**
  - EN: Array Hidden on Map
  - KO: 어레이: 맵에서 숨김
  - [ ] OK    Fix: 

- **`arrayVisible`**
  - EN: Array Visible on Map
  - KO: 어레이: 맵에서 표시
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - KO: 내보내기
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - KO: 가져오기
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Backup
  - KO: 백업 불러오기
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Output Config
  - KO: 출력 설정 다시 불러오기
  - [ ] OK    Fix: 

- **`speakerHidden`**
  - EN: Speaker Hidden on Map
  - KO: 스피커: 맵에서 숨김
  - [ ] OK    Fix: 

- **`speakerVisible`**
  - EN: Speaker Visible on Map
  - KO: 맵에 스피커 표시
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Output Config
  - KO: 출력 설정 저장
  - [ ] OK    Fix: 

- **`wizardOfOutZ`**
  - EN: Wizard of OutZ...
  - KO: OutZ 마법사...
  - [ ] OK    Fix: 

## `outputs.coordModes`

- **`xyz`**
  - EN: XYZ
  - KO: XYZ
  - [ ] OK    Fix: 

## `outputs.dialogs`

- **`export`**
  - EN: Export Output Configuration
  - KO: 출력 설정 내보내기
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import Output Configuration
  - KO: 출력 설정 가져오기
  - [ ] OK    Fix: 

## `outputs.help`

- **`angleOff`**
  - EN: Output Channel Will Not Amplify Objects in this Angle in Front of it. (changes may affect the rest of the array)
  - KO: 출력 채널은 앞쪽의 이 각도 내에 있는 오브젝트를 증폭하지 않습니다. (변경 사항이 어레이의 나머지에 영향을 줄 수 있습니다)
  - [ ] OK    Fix: 

- **`angleOn`**
  - EN: Output Channel Will Amplify Objects in this Angle in its Back. (changes may affect the rest of the array)
  - KO: 출력 채널은 뒤쪽의 이 각도 내에 있는 오브젝트를 증폭합니다. (변경 사항이 어레이의 나머지에 영향을 줄 수 있습니다)
  - [ ] OK    Fix: 

- **`applyToArray`**
  - EN: Apply Changes to the rest of the Array (Absolute value or Relative changes).
  - KO: 어레이의 나머지에 변경 사항 적용 (절대값 또는 상대적 변경).
  - [ ] OK    Fix: 

- **`arraySelector`**
  - EN: Selected Output Channel is part of Array.
  - KO: 선택한 출력 채널이 어레이의 일부입니다.
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Output Channel Attenuation. (changes may affect the rest of the array)
  - KO: 출력 채널 감쇠. (변경 사항이 어레이의 나머지에 영향을 줄 수 있습니다)
  - [ ] OK    Fix: 

- **`channelSelector`**
  - EN: Output Channel Number and Selection.
  - KO: 출력 채널 번호 및 선택.
  - [ ] OK    Fix: 

- **`coordMode`**
  - EN: Coordinate display mode: Cartesian (X/Y/Z), Cylindrical (radius/azimuth/height), or Spherical (radius/azimuth/elevation).
  - KO: 좌표 표시 모드: 직교 좌표 (X/Y/Z), 원통 좌표 (반지름/방위각/높이) 또는 구면 좌표 (반지름/방위각/앙각).
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Output Channel Delay (positive values) or Latency Compensation (negative values). (changes may affect the rest of the array)
  - KO: 출력 채널 딜레이 (양수 값) 또는 레이턴시 보정 (음수 값). (변경 사항이 어레이의 나머지에 영향을 줄 수 있습니다)
  - [ ] OK    Fix: 

- **`directional`**
  - EN: Output Channel Directional Control. Drag to change orientation, Shift+drag for Angle Off, Alt+drag for Angle On. (changes may affect the rest of the array)
  - KO: 출력 채널 방향 제어. 드래그하여 방향 변경, Shift+드래그로 Angle Off, Alt+드래그로 Angle On. (변경 사항이 어레이의 나머지에 영향을 줄 수 있습니다)
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Ratio of Distance Attenuation for Selected Output. (changes may affect the rest of the array)
  - KO: 선택한 출력의 거리 감쇠 비율. (변경 사항이 어레이의 나머지에 영향을 줄 수 있습니다)
  - [ ] OK    Fix: 

- **`eqBandToggle`**
  - EN: Toggle Band {band} on/off. When off the band is bypassed.
  - KO: 밴드 {band} 켜기/끄기. 꺼진 밴드는 바이패스됩니다.
  - [ ] OK    Fix: 

- **`eqEnable`**
  - EN: Enable or disable EQ processing for this output.
  - KO: 이 출력의 EQ 처리를 활성화/비활성화합니다.
  - [ ] OK    Fix: 

- **`eqFlatten`**
  - EN: Long press to reset all EQ bands to their default values.
  - KO: 길게 눌러 모든 EQ 밴드를 기본값으로 초기화합니다.
  - [ ] OK    Fix: 

- **`eqFreq`**
  - EN: Output EQ Band {band} frequency (20 Hz - 20 kHz).
  - KO: 출력 EQ 밴드 {band} 주파수 (20 Hz - 20 kHz).
  - [ ] OK    Fix: 

- **`eqGain`**
  - EN: Output EQ Band {band} gain (-24 to +24 dB).
  - KO: 출력 EQ 밴드 {band} 게인 (-24~+24 dB).
  - [ ] OK    Fix: 

- **`eqQ`**
  - EN: Output EQ Band {band} Q factor (0.1 - 10.0).
  - KO: 출력 EQ 밴드 {band} Q 팩터 (0.1 - 10.0).
  - [ ] OK    Fix: 

- **`eqResetBand`**
  - EN: Long press to reset Band {band} to its default values.
  - KO: 길게 눌러 밴드 {band}를 기본값으로 초기화합니다.
  - [ ] OK    Fix: 

- **`eqShape`**
  - EN: Output EQ Band {band} filter shape.
  - KO: 출력 EQ 밴드 {band} 필터 형태.
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Output Configuration to file (with file explorer window).
  - KO: 출력 설정을 파일로 내보냅니다 (파일 탐색기 창 포함).
  - [ ] OK    Fix: 

- **`floorReflections`**
  - EN: Enable or Disable the Floor Reflections for this Speaker.
  - KO: 이 스피커의 바닥 반사를 활성화 또는 비활성화합니다.
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: Loss of High Frequency Depending on Distance from Object to Output. (changes may affect the rest of the array)
  - KO: 오브젝트에서 출력까지의 거리에 따른 고주파 손실. (변경 사항이 어레이의 나머지에 영향을 줄 수 있습니다)
  - [ ] OK    Fix: 

- **`hParallax`**
  - EN: Horizontal Distance from Speaker to 'Targeted' Listener. (changes may affect the rest of the array)
  - KO: 스피커에서 '대상' 청취자까지의 수평 거리. (변경 사항이 어레이의 나머지에 영향을 줄 수 있습니다)
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Output Configuration from file (with file explorer window).
  - KO: 파일에서 출력 설정을 가져옵니다 (파일 탐색기 창 포함).
  - [ ] OK    Fix: 

- **`liveSource`**
  - EN: Disables Live Source Attenuation for Selected Output. (changes may affect the rest of the array)
  - KO: 선택한 출력의 라이브 소스 감쇠를 비활성화합니다. (변경 사항이 어레이의 나머지에 영향을 줄 수 있습니다)
  - [ ] OK    Fix: 

- **`mapVisibility`**
  - EN: Make Visible or Hide The Selected Output on the Map.
  - KO: 선택한 출력을 맵에서 보이게 하거나 숨깁니다.
  - [ ] OK    Fix: 

- **`minLatency`**
  - EN: Disables Minimal Latency Mode for Selected Output. (changes may affect the rest of the array)
  - KO: 선택한 출력의 최소 레이턴시 모드를 비활성화합니다. (변경 사항이 어레이의 나머지에 영향을 줄 수 있습니다)
  - [ ] OK    Fix: 

- **`nameEditor`**
  - EN: Displayed Output Channel Name (editable).
  - KO: 표시되는 출력 채널 이름 (편집 가능).
  - [ ] OK    Fix: 

- **`pitch`**
  - EN: Output Channel Vertical Orientation used to Determine which Objects get amplified. (changes may affect the rest of the array)
  - KO: 어떤 오브젝트가 증폭되는지 결정하는 데 사용되는 출력 채널의 수직 방향. (변경 사항이 어레이의 나머지에 영향을 줄 수 있습니다)
  - [ ] OK    Fix: 

- **`position1`**
  - EN: Output Channel {name} ({unit}).
  - KO: 출력 채널 {name} ({unit}).
  - [ ] OK    Fix: 

- **`position2`**
  - EN: Output Channel {name} ({unit}).
  - KO: 출력 채널 {name} ({unit}).
  - [ ] OK    Fix: 

- **`position3`**
  - EN: Output Channel {name} ({unit}).
  - KO: 출력 채널 {name} ({unit}).
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Output Configuration from backup file.
  - KO: 백업 파일에서 출력 설정을 다시 로드합니다.
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Output Configuration from file.
  - KO: 파일에서 출력 설정을 다시 로드합니다.
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Output Configuration to file (with backup).
  - KO: 출력 설정을 파일로 저장합니다 (백업 포함).
  - [ ] OK    Fix: 

- **`vParallax`**
  - EN: Vertical Distance from Speaker to 'Targeted' Listener. Positive when the Speaker is Below the head of the Listener. (changes may affect the rest of the array)
  - KO: 스피커에서 '대상' 청취자까지의 수직 거리. 스피커가 청취자의 머리 아래에 있을 때 양수. (변경 사항이 어레이의 나머지에 영향을 줄 수 있습니다)
  - [ ] OK    Fix: 

- **`wizardOfOutZ`**
  - EN: Open Wizard of OutZ to Position Speaker Arrays Conveniently.
  - KO: Wizard of OutZ를 열어 스피커 어레이를 편리하게 배치합니다.
  - [ ] OK    Fix: 

## `outputs.labels`

- **`angleOff`**
  - EN: Angle Off:
  - KO: 끄기 각도:
  - [ ] OK    Fix: 

- **`angleOn`**
  - EN: Angle On:
  - KO: 켜기 각도:
  - [ ] OK    Fix: 

- **`applyToArray`**
  - EN: Apply to Array:
  - KO: 어레이에 적용:
  - [ ] OK    Fix: 

- **`array`**
  - EN: Array:
  - KO: 어레이:
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Attenuation:
  - KO: 감쇠:
  - [ ] OK    Fix: 

- **`coordinates`**
  - EN: Coordinates:
  - KO: 좌표:
  - [ ] OK    Fix: 

- **`delay`**
  - EN: Delay:
  - KO: 지연:
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Delay/Latency:
  - KO: 지연/레이턴시:
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Distance Atten:
  - KO: 거리 감쇠:
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: HF Damping:
  - KO: 고주파 감쇠:
  - [ ] OK    Fix: 

- **`hParallax`**
  - EN: Horizontal Parallax:
  - KO: 수평 시차:
  - [ ] OK    Fix: 

- **`latency`**
  - EN: Latency:
  - KO: 레이턴시:
  - [ ] OK    Fix: 

- **`name`**
  - EN: Name:
  - KO: 이름:
  - [ ] OK    Fix: 

- **`orientation`**
  - EN: Orientation:
  - KO: 방향:
  - [ ] OK    Fix: 

- **`pitch`**
  - EN: Pitch:
  - KO: 피치:
  - [ ] OK    Fix: 

- **`positionX`**
  - EN: Position X:
  - KO: 위치 X:
  - [ ] OK    Fix: 

- **`positionY`**
  - EN: Position Y:
  - KO: 위치 Y:
  - [ ] OK    Fix: 

- **`positionZ`**
  - EN: Position Z:
  - KO: 위치 Z:
  - [ ] OK    Fix: 

- **`vParallax`**
  - EN: Vertical Parallax:
  - KO: 수직 시차:
  - [ ] OK    Fix: 

## `outputs.messages`

- **`assignedToArray`**
  - EN: Output {num} assigned to Array {array}
  - KO: 출력 {num}이(가) 어레이 {array}에 할당됨
  - [ ] OK    Fix: 

- **`backupLoaded`**
  - EN: Output configuration loaded from backup.
  - KO: 백업에서 출력 설정이 로드되었습니다.
  - [ ] OK    Fix: 

- **`configExported`**
  - EN: Output configuration exported.
  - KO: 출력 설정을 내보냈습니다.
  - [ ] OK    Fix: 

- **`configImported`**
  - EN: Output configuration imported.
  - KO: 출력 설정을 가져왔습니다.
  - [ ] OK    Fix: 

- **`configLoaded`**
  - EN: Output configuration loaded.
  - KO: 출력 설정이 로드되었습니다.
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Output configuration saved.
  - KO: 출력 설정이 저장되었습니다.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - KO: 오류: {error}
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - KO: 시스템 설정에서 먼저 프로젝트 폴더를 선택하세요.
  - [ ] OK    Fix: 

- **`setToSingle`**
  - EN: Output {num} set to Single
  - KO: 출력 {num}이(가) 단일로 설정됨
  - [ ] OK    Fix: 

## `outputs.tabs`

- **`eq`**
  - EN: Output EQ
  - KO: 출력 EQ
  - [ ] OK    Fix: 

- **`parameters`**
  - EN: Output Parameters
  - KO: 출력 매개변수
  - [ ] OK    Fix: 

## `outputs.toggles`

- **`floorReflectionsOff`**
  - EN: Floor Reflections: OFF
  - KO: 바닥 반사: 끄기
  - [ ] OK    Fix: 

- **`floorReflectionsOn`**
  - EN: Floor Reflections: ON
  - KO: 바닥 반사: 켜기
  - [ ] OK    Fix: 

- **`liveSourceOff`**
  - EN: Live Source Atten: OFF
  - KO: 라이브 소스 감쇠: 끄기
  - [ ] OK    Fix: 

- **`liveSourceOn`**
  - EN: Live Source Atten: ON
  - KO: 라이브 소스 감쇠: 켜기
  - [ ] OK    Fix: 

- **`minLatencyOff`**
  - EN: Minimal Latency: OFF
  - KO: 최소 레이턴시: 끄기
  - [ ] OK    Fix: 

- **`minLatencyOn`**
  - EN: Minimal Latency: ON
  - KO: 최소 레이턴시: 켜기
  - [ ] OK    Fix: 

## `outputs.units`

- **`meters`**
  - EN: m
  - KO: m
  - [ ] OK    Fix: 

- **`ms`**
  - EN: ms
  - KO: ms
  - [ ] OK    Fix: 

## `reverbs`

- **`noChannels`**
  - EN: No reverb channels configured.

Set the number of Reverb Channels in System Config.
  - KO: 리버브 채널이 구성되지 않았습니다.

시스템 설정에서 리버브 채널 수를 설정하세요.
  - [ ] OK    Fix: 

## `reverbs.algorithm`

- **`crossoverHigh`**
  - EN: Crossover High:
  - KO: 크로스오버 고역:
  - [ ] OK    Fix: 

- **`crossoverLow`**
  - EN: Crossover Low:
  - KO: 크로스오버 저역:
  - [ ] OK    Fix: 

- **`decaySection`**
  - EN: Decay
  - KO: 감쇠
  - [ ] OK    Fix: 

- **`diffusion`**
  - EN: Diffusion:
  - KO: 확산:
  - [ ] OK    Fix: 

- **`fdn`**
  - EN: FDN
  - KO: FDN
  - [ ] OK    Fix: 

- **`fdnSection`**
  - EN: FDN
  - KO: FDN
  - [ ] OK    Fix: 

- **`ir`**
  - EN: IR
  - KO: IR
  - [ ] OK    Fix: 

- **`irFile`**
  - EN: IR File:
  - KO: IR 파일:
  - [ ] OK    Fix: 

- **`irImport`**
  - EN: Import IR...
  - KO: IR 가져오기...
  - [ ] OK    Fix: 

- **`irImportSuccess`**
  - EN: IR imported: {file}
  - KO: IR 가져오기 완료: {file}
  - [ ] OK    Fix: 

- **`irLength`**
  - EN: IR Length:
  - KO: IR 길이:
  - [ ] OK    Fix: 

- **`irNoProject`**
  - EN: Set a project folder first
  - KO: 먼저 프로젝트 폴더를 설정하세요
  - [ ] OK    Fix: 

- **`irSection`**
  - EN: IR
  - KO: IR
  - [ ] OK    Fix: 

- **`irTrim`**
  - EN: IR Trim:
  - KO: IR 트림:
  - [ ] OK    Fix: 

- **`noFileLoaded`**
  - EN: No IR loaded
  - KO: IR이 로드되지 않음
  - [ ] OK    Fix: 

- **`perNodeOff`**
  - EN: Per-node IR OFF
  - KO: 노드별 IR 끄기
  - [ ] OK    Fix: 

- **`perNodeOn`**
  - EN: Per-node IR ON
  - KO: 노드별 IR 켜기
  - [ ] OK    Fix: 

- **`rt60`**
  - EN: RT60:
  - KO: RT60:
  - [ ] OK    Fix: 

- **`rt60HighMult`**
  - EN: RT60 High ×:
  - KO: RT60 고역 ×:
  - [ ] OK    Fix: 

- **`rt60LowMult`**
  - EN: RT60 Low ×:
  - KO: RT60 저역 ×:
  - [ ] OK    Fix: 

- **`scale`**
  - EN: Scale:
  - KO: 스케일:
  - [ ] OK    Fix: 

- **`sdn`**
  - EN: SDN
  - KO: SDN
  - [ ] OK    Fix: 

- **`sdnSection`**
  - EN: SDN
  - KO: SDN
  - [ ] OK    Fix: 

- **`size`**
  - EN: Size:
  - KO: 크기:
  - [ ] OK    Fix: 

- **`wetLevel`**
  - EN: Wet Level:
  - KO: 웻 레벨:
  - [ ] OK    Fix: 

## `reverbs.buttons`

- **`editOnMap`**
  - EN: Edit on Map
  - KO: 맵에서 편집
  - [ ] OK    Fix: 

- **`editOnMapOn`**
  - EN: Edit on Map ON
  - KO: 맵에서 편집 ON
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - KO: 내보내기
  - [ ] OK    Fix: 

- **`hiddenOnMap`**
  - EN: Reverbs Hidden on Map
  - KO: 맵에서 리버브 숨김
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - KO: 가져오기
  - [ ] OK    Fix: 

- **`mutePost`**
  - EN: Mute Post
  - KO: Mute Post
  - [ ] OK    Fix: 

- **`mutePostOn`**
  - EN: Mute Post ON
  - KO: 뮤트 포스트 ON
  - [ ] OK    Fix: 

- **`mutePre`**
  - EN: Mute Pre
  - KO: Mute Pre
  - [ ] OK    Fix: 

- **`mutePreOn`**
  - EN: Mute Pre ON
  - KO: 뮤트 프리 ON
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Backup
  - KO: 백업 불러오기
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Reverb Config
  - KO: 리버브 설정 다시 불러오기
  - [ ] OK    Fix: 

- **`soloReverbs`**
  - EN: Solo Reverbs
  - KO: Solo Reverbs
  - [ ] OK    Fix: 

- **`soloReverbsOn`**
  - EN: Solo Reverbs ON
  - KO: 솔로 리버브 ON
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Reverb Config
  - KO: 리버브 설정 저장
  - [ ] OK    Fix: 

- **`visibleOnMap`**
  - EN: Reverbs Visible on Map
  - KO: 맵에 리버브 표시
  - [ ] OK    Fix: 

## `reverbs.coordModes`

- **`xyz`**
  - EN: XYZ
  - KO: XYZ
  - [ ] OK    Fix: 

## `reverbs.dialogs`

- **`export`**
  - EN: Export Reverb Configuration
  - KO: 리버브 설정 내보내기
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import Reverb Configuration
  - KO: 리버브 설정 가져오기
  - [ ] OK    Fix: 

## `reverbs.help`

- **`algoCrossoverHigh`**
  - EN: High crossover frequency for 3-band decay (1 - 10 kHz).
  - KO: 3밴드 감쇠용 고역 크로스오버 주파수 (1 - 10 kHz).
  - [ ] OK    Fix: 

- **`algoCrossoverLow`**
  - EN: Low crossover frequency for 3-band decay (50 - 500 Hz).
  - KO: 3밴드 감쇠용 저역 크로스오버 주파수 (50 - 500 Hz).
  - [ ] OK    Fix: 

- **`algoDiffusion`**
  - EN: Diffusion amount controlling echo density (0 - 100%).
  - KO: 에코 밀도를 제어하는 확산량 (0 - 100%).
  - [ ] OK    Fix: 

- **`algoFDN`**
  - EN: Select FDN (Feedback Delay Network) reverb algorithm.
  - KO: FDN (Feedback Delay Network) 리버브 알고리즘 선택.
  - [ ] OK    Fix: 

- **`algoFDNSize`**
  - EN: FDN delay line size multiplier (0.5 - 2.0x).
  - KO: FDN 딜레이 라인 크기 배수 (0.5 - 2.0x).
  - [ ] OK    Fix: 

- **`algoIR`**
  - EN: Select IR (Impulse Response convolution) reverb algorithm.
  - KO: IR (임펄스 응답 컨볼루션) 리버브 알고리즘 선택.
  - [ ] OK    Fix: 

- **`algoIRFile`**
  - EN: Select or import an impulse response file for convolution.
  - KO: 컨볼루션용 임펄스 응답 파일 선택 또는 가져오기.
  - [ ] OK    Fix: 

- **`algoIRLength`**
  - EN: Maximum impulse response length (0.1 - 6.0 seconds).
  - KO: 최대 임펄스 응답 길이 (0.1 - 6.0초).
  - [ ] OK    Fix: 

- **`algoIRTrim`**
  - EN: Trim the start of the impulse response (0 - 100 ms).
  - KO: 임펄스 응답의 시작 부분 트리밍 (0 - 100 ms).
  - [ ] OK    Fix: 

- **`algoPerNode`**
  - EN: Use a separate IR for each reverb node, or share one IR.
  - KO: 각 리버브 노드에 별도의 IR을 사용하거나 하나의 IR을 공유.
  - [ ] OK    Fix: 

- **`algoRT60`**
  - EN: Reverb decay time RT60 (0.2 - 8.0 seconds).
  - KO: 리버브 감쇠 시간 RT60 (0.2 - 8.0초).
  - [ ] OK    Fix: 

- **`algoRT60HighMult`**
  - EN: High frequency RT60 multiplier (0.1 - 9.0x).
  - KO: 고역 RT60 배수 (0.1 - 9.0x).
  - [ ] OK    Fix: 

- **`algoRT60LowMult`**
  - EN: Low frequency RT60 multiplier (0.1 - 9.0x).
  - KO: 저역 RT60 배수 (0.1 - 9.0x).
  - [ ] OK    Fix: 

- **`algoSDN`**
  - EN: Select SDN (Scattering Delay Network) reverb algorithm.
  - KO: SDN (Scattering Delay Network) 리버브 알고리즘 선택.
  - [ ] OK    Fix: 

- **`algoSDNScale`**
  - EN: SDN inter-node delay scale factor (0.5 - 4.0x).
  - KO: SDN 노드 간 지연 스케일 팩터 (0.5 - 4.0x).
  - [ ] OK    Fix: 

- **`algoWetLevel`**
  - EN: Wet/dry mix level for reverb output (-60 to +12 dB).
  - KO: 리버브 출력의 웻/드라이 믹스 레벨 (-60 ~ +12 dB).
  - [ ] OK    Fix: 

- **`angleOff`**
  - EN: Angle at which no amplification occurs (0-179 degrees).
  - KO: 증폭이 발생하지 않는 각도 (0-179도).
  - [ ] OK    Fix: 

- **`angleOn`**
  - EN: Angle at which amplification starts (1-180 degrees).
  - KO: 증폭이 시작되는 각도 (1-180도).
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Reverb channel attenuation (-92 to 0 dB).
  - KO: 리버브 채널 감쇠 (-92 ~ 0 dB).
  - [ ] OK    Fix: 

- **`channelSelector`**
  - EN: Reverb Channel Number and Selection.
  - KO: 리버브 채널 번호 및 선택.
  - [ ] OK    Fix: 

- **`commonAtten`**
  - EN: Common attenuation percentage (0-100%).
  - KO: 공통 감쇠 백분율 (0-100%).
  - [ ] OK    Fix: 

- **`coordMode`**
  - EN: Coordinate display mode: Cartesian (X/Y/Z), Cylindrical (radius/azimuth/height), or Spherical (radius/azimuth/elevation).
  - KO: 좌표 표시 모드: 직교 좌표 (X/Y/Z), 원통 좌표 (반지름/방위각/높이), 또는 구면 좌표 (반지름/방위각/고도).
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Reverb delay/latency compensation (-100 to +100 ms).
  - KO: 리버브 지연/레이턴시 보정 (-100 ~ +100 ms).
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Distance attenuation for reverb return (-6.0 to 0.0 dB/m).
  - KO: 리버브 리턴에 대한 거리 감쇠 (-6.0 ~ 0.0 dB/m).
  - [ ] OK    Fix: 

- **`distanceAttenEnable`**
  - EN: Distance attenuation percentage (0-200%).
  - KO: 거리 감쇠 백분율 (0-200%).
  - [ ] OK    Fix: 

- **`eqBandToggle`**
  - EN: Toggle pre-EQ Band {band} on/off. When off the band is bypassed.
  - KO: 프리EQ 밴드 {band} 켜기/끄기.
  - [ ] OK    Fix: 

- **`eqEnable`**
  - EN: Enable or disable EQ processing for this reverb.
  - KO: 이 리버브에 대한 EQ 처리를 활성화 또는 비활성화.
  - [ ] OK    Fix: 

- **`eqFlatten`**
  - EN: Long press to reset all pre-EQ bands to their default values.
  - KO: 길게 눌러 모든 프리EQ 밴드를 초기화합니다.
  - [ ] OK    Fix: 

- **`eqFreq`**
  - EN: Pre-EQ Band {band} frequency (20 Hz - 20 kHz).
  - KO: 프리EQ 밴드 {band} 주파수 (20 Hz - 20 kHz).
  - [ ] OK    Fix: 

- **`eqGain`**
  - EN: Pre-EQ Band {band} gain (-24 to +24 dB).
  - KO: 프리EQ 밴드 {band} 게인 (-24~+24 dB).
  - [ ] OK    Fix: 

- **`eqQ`**
  - EN: Pre-EQ Band {band} Q factor (0.1 - 20.0).
  - KO: 프리EQ 밴드 {band} Q 팩터 (0.1 - 20.0).
  - [ ] OK    Fix: 

- **`eqResetBand`**
  - EN: Long press to reset pre-EQ Band {band} to its default values.
  - KO: 길게 눌러 프리EQ 밴드 {band}를 초기화합니다.
  - [ ] OK    Fix: 

- **`eqShape`**
  - EN: Pre-EQ Band {band} filter shape.
  - KO: 프리EQ 밴드 {band} 필터 형태.
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Reverb Configuration to file (with file explorer window).
  - KO: 파일로 리버브 설정 내보내기 (파일 탐색기 사용).
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: High frequency loss per meter (-6.0 to 0.0 dB/m).
  - KO: 미터당 고주파 손실 (-6.0 ~ 0.0 dB/m).
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Reverb Configuration from file (with file explorer window).
  - KO: 파일에서 리버브 설정 가져오기 (파일 탐색기 사용).
  - [ ] OK    Fix: 

- **`liveSourceTooltip`**
  - EN: Enable Live Source attenuation tamer. Reduces level fluctuations from sources close to the array.
  - KO: Live Source 감쇠 테이머를 활성화합니다. 어레이에 가까운 소스의 레벨 변동을 줄입니다.
  - [ ] OK    Fix: 

- **`mapVisibility`**
  - EN: Make Visible or Hide All Reverb Channels on the Map.
  - KO: 맵에서 모든 리버브 채널을 표시하거나 숨기기.
  - [ ] OK    Fix: 

- **`miniLatencyTooltip`**
  - EN: Enable minimal latency mode for this reverb channel. Reduces processing delay at the cost of higher CPU usage.
  - KO: 이 리버브 채널의 최소 지연 모드를 활성화합니다. 더 높은 CPU 사용량을 대가로 처리 지연을 줄입니다.
  - [ ] OK    Fix: 

- **`muteButton`**
  - EN: Toggle mute for this output channel's reverb return.
  - KO: 이 출력 채널의 리버브 리턴 음소거를 전환합니다.
  - [ ] OK    Fix: 

- **`muteMacros`**
  - EN: Quick mute operations for output channels.
  - KO: 출력 채널의 빠른 음소거 작업.
  - [ ] OK    Fix: 

- **`nameEditor`**
  - EN: Displayed Reverb Channel Name (editable).
  - KO: 표시되는 리버브 채널 이름 (편집 가능).
  - [ ] OK    Fix: 

- **`orientation`**
  - EN: Reverb orientation angle (-179 to +180 degrees).
  - KO: 리버브 방향 각도 (-179 ~ +180도).
  - [ ] OK    Fix: 

- **`pitch`**
  - EN: Vertical orientation of reverb (-90 to +90 degrees).
  - KO: 리버브의 수직 방향 (-90 ~ +90도).
  - [ ] OK    Fix: 

- **`position1`**
  - EN: Reverb virtual source {name} ({unit}).
  - KO: 리버브 가상 소스 {name} ({unit}).
  - [ ] OK    Fix: 

- **`position2`**
  - EN: Reverb virtual source {name} ({unit}).
  - KO: 리버브 가상 소스 {name} ({unit}).
  - [ ] OK    Fix: 

- **`position3`**
  - EN: Reverb virtual source {name} ({unit}).
  - KO: 리버브 가상 소스 {name} ({unit}).
  - [ ] OK    Fix: 

- **`postEqBandToggle`**
  - EN: Toggle post-EQ Band {band} on/off. When off the band is bypassed.
  - KO: 포스트EQ 밴드 {band} 켜기/끄기.
  - [ ] OK    Fix: 

- **`postEqEnable`**
  - EN: Enable or disable post-processing EQ.
  - KO: 후처리 EQ를 활성화 또는 비활성화.
  - [ ] OK    Fix: 

- **`postEqFlatten`**
  - EN: Long press to reset all post-EQ bands to their default values.
  - KO: 길게 눌러 모든 포스트EQ 밴드를 초기화합니다.
  - [ ] OK    Fix: 

- **`postEqFreq`**
  - EN: Post-EQ Band {band} frequency (20 Hz - 20 kHz).
  - KO: 포스트 EQ 밴드 {band} 주파수 (20 Hz - 20 kHz).
  - [ ] OK    Fix: 

- **`postEqGain`**
  - EN: Post-EQ Band {band} gain (-24 to +24 dB).
  - KO: 포스트 EQ 밴드 {band} 게인 (-24 ~ +24 dB).
  - [ ] OK    Fix: 

- **`postEqQ`**
  - EN: Post-EQ Band {band} Q factor (0.1 - 20.0).
  - KO: 포스트 EQ 밴드 {band} Q 팩터 (0.1 - 20.0).
  - [ ] OK    Fix: 

- **`postEqResetBand`**
  - EN: Long press to reset post-EQ Band {band} to its default values.
  - KO: 길게 눌러 포스트EQ 밴드 {band}를 초기화합니다.
  - [ ] OK    Fix: 

- **`postEqShape`**
  - EN: Post-EQ Band {band} filter shape.
  - KO: 포스트 EQ 밴드 {band} 필터 형태.
  - [ ] OK    Fix: 

- **`postExpAttack`**
  - EN: Post-expander attack time (0.1 - 50 ms).
  - KO: 포스트 익스팬더 어택 시간 (0.1 - 50 ms).
  - [ ] OK    Fix: 

- **`postExpBypass`**
  - EN: Bypass or enable the post-expander on reverb returns.
  - KO: 리버브 리턴의 포스트 익스팬더를 바이패스 또는 활성화.
  - [ ] OK    Fix: 

- **`postExpRatio`**
  - EN: Post-expander ratio (1:1 to 1:8).
  - KO: 포스트 익스팬더 비율 (1:1 ~ 1:8).
  - [ ] OK    Fix: 

- **`postExpRelease`**
  - EN: Post-expander release time (50 - 2000 ms).
  - KO: 포스트 익스팬더 릴리스 시간 (50 - 2000 ms).
  - [ ] OK    Fix: 

- **`postExpThreshold`**
  - EN: Post-expander threshold (-80 to -10 dB).
  - KO: 포스트 익스팬더 임계값 (-80 ~ -10 dB).
  - [ ] OK    Fix: 

- **`preCompAttack`**
  - EN: Pre-compressor attack time (0.1 - 100 ms).
  - KO: 프리컴프레서 어택 시간 (0.1 - 100 ms).
  - [ ] OK    Fix: 

- **`preCompBypass`**
  - EN: Bypass or enable the pre-compressor on reverb feeds.
  - KO: 리버브 센드의 프리컴프레서를 바이패스 또는 활성화.
  - [ ] OK    Fix: 

- **`preCompRatio`**
  - EN: Pre-compressor ratio (1:1 to 20:1).
  - KO: 프리컴프레서 비율 (1:1 ~ 20:1).
  - [ ] OK    Fix: 

- **`preCompRelease`**
  - EN: Pre-compressor release time (10 - 1000 ms).
  - KO: 프리컴프레서 릴리스 시간 (10 - 1000 ms).
  - [ ] OK    Fix: 

- **`preCompThreshold`**
  - EN: Pre-compressor threshold (-60 to 0 dB).
  - KO: 프리컴프레서 임계값 (-60 ~ 0 dB).
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Reverb Configuration from backup file.
  - KO: 백업 파일에서 리버브 설정 다시 불러오기.
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Reverb Configuration from file.
  - KO: 파일에서 리버브 설정 다시 불러오기.
  - [ ] OK    Fix: 

- **`returnOffset1`**
  - EN: Reverb return offset {name} ({unit}).
  - KO: 리버브 리턴 오프셋 {name} ({unit}).
  - [ ] OK    Fix: 

- **`returnOffset2`**
  - EN: Reverb return offset {name} ({unit}).
  - KO: 리버브 리턴 오프셋 {name} ({unit}).
  - [ ] OK    Fix: 

- **`returnOffset3`**
  - EN: Reverb return offset {name} ({unit}).
  - KO: 리버브 리턴 오프셋 {name} ({unit}).
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Reverb Configuration to file (with backup).
  - KO: 리버브 설정을 파일에 저장 (백업 포함).
  - [ ] OK    Fix: 

## `reverbs.labels`

- **`angleOff`**
  - EN: Angle Off:
  - KO: 끄기 각도:
  - [ ] OK    Fix: 

- **`angleOn`**
  - EN: Angle On:
  - KO: 켜기 각도:
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Attenuation:
  - KO: 감쇠:
  - [ ] OK    Fix: 

- **`commonAtten`**
  - EN: Common Atten:
  - KO: 공통 감쇠:
  - [ ] OK    Fix: 

- **`coordinates`**
  - EN: Coord:
  - KO: 좌표:
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Delay/Latency:
  - KO: 지연/레이턴시:
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Distance Atten:
  - KO: 거리 감쇠:
  - [ ] OK    Fix: 

- **`distanceAttenPercent`**
  - EN: Distance Atten %:
  - KO: 거리 감쇠 %:
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: HF Damping:
  - KO: 고주파 감쇠:
  - [ ] OK    Fix: 

- **`muteMacro`**
  - EN: Mute Macro:
  - KO: 음소거 매크로:
  - [ ] OK    Fix: 

- **`name`**
  - EN: Name:
  - KO: 이름:
  - [ ] OK    Fix: 

- **`orientation`**
  - EN: Orientation:
  - KO: 방향:
  - [ ] OK    Fix: 

- **`outputMutes`**
  - EN: Output Mutes:
  - KO: 출력 음소거:
  - [ ] OK    Fix: 

- **`pitch`**
  - EN: Pitch:
  - KO: 피치:
  - [ ] OK    Fix: 

- **`positionX`**
  - EN: Position X:
  - KO: 위치 X:
  - [ ] OK    Fix: 

- **`positionY`**
  - EN: Position Y:
  - KO: 위치 Y:
  - [ ] OK    Fix: 

- **`positionZ`**
  - EN: Position Z:
  - KO: 위치 Z:
  - [ ] OK    Fix: 

- **`returnOffsetX`**
  - EN: Return Offset X:
  - KO: 리턴 오프셋 X:
  - [ ] OK    Fix: 

- **`returnOffsetY`**
  - EN: Return Offset Y:
  - KO: 리턴 오프셋 Y:
  - [ ] OK    Fix: 

- **`returnOffsetZ`**
  - EN: Return Offset Z:
  - KO: 리턴 오프셋 Z:
  - [ ] OK    Fix: 

## `reverbs.messages`

- **`backupLoaded`**
  - EN: Reverb configuration loaded from backup.
  - KO: 백업에서 리버브 설정이 로드되었습니다.
  - [ ] OK    Fix: 

- **`configExported`**
  - EN: Reverb configuration exported.
  - KO: 리버브 설정을 내보냈습니다.
  - [ ] OK    Fix: 

- **`configImported`**
  - EN: Reverb configuration imported.
  - KO: 리버브 설정을 가져왔습니다.
  - [ ] OK    Fix: 

- **`configLoaded`**
  - EN: Reverb configuration loaded.
  - KO: 리버브 설정이 로드되었습니다.
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Reverb configuration saved.
  - KO: 리버브 설정이 저장되었습니다.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - KO: 오류: {error}
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - KO: 시스템 설정에서 먼저 프로젝트 폴더를 선택하세요.
  - [ ] OK    Fix: 

## `reverbs.muteMacros`

- **`invertMutes`**
  - EN: INVERT MUTES
  - KO: 음소거 반전
  - [ ] OK    Fix: 

- **`muteAll`**
  - EN: MUTE ALL
  - KO: 전체 음소거
  - [ ] OK    Fix: 

- **`muteArray`**
  - EN: MUTE ARRAY
  - KO: 어레이 음소거
  - [ ] OK    Fix: 

- **`muteEven`**
  - EN: MUTE EVEN
  - KO: 짝수 음소거
  - [ ] OK    Fix: 

- **`muteOdd`**
  - EN: MUTE ODD
  - KO: 홀수 음소거
  - [ ] OK    Fix: 

- **`select`**
  - EN: Mute Macro Select
  - KO: 음소거 매크로 선택
  - [ ] OK    Fix: 

- **`unmuteAll`**
  - EN: UNMUTE ALL
  - KO: 전체 음소거 해제
  - [ ] OK    Fix: 

- **`unmuteArray`**
  - EN: UNMUTE ARRAY
  - KO: 어레이 음소거 해제
  - [ ] OK    Fix: 

## `reverbs.postProcessing`

- **`attack`**
  - EN: Attack:
  - KO: 어택:
  - [ ] OK    Fix: 

- **`expander`**
  - EN: Expander
  - KO: 익스팬더
  - [ ] OK    Fix: 

- **`expanderOff`**
  - EN: Expander OFF
  - KO: 익스팬더 끄기
  - [ ] OK    Fix: 

- **`expanderOn`**
  - EN: Expander ON
  - KO: 익스팬더 켜기
  - [ ] OK    Fix: 

- **`ratio`**
  - EN: Ratio:
  - KO: 비율:
  - [ ] OK    Fix: 

- **`release`**
  - EN: Release:
  - KO: 릴리스:
  - [ ] OK    Fix: 

- **`threshold`**
  - EN: Threshold:
  - KO: 임계값:
  - [ ] OK    Fix: 

## `reverbs.preProcessing`

- **`attack`**
  - EN: Attack:
  - KO: 어택:
  - [ ] OK    Fix: 

- **`compressor`**
  - EN: Compressor
  - KO: 컴프레서
  - [ ] OK    Fix: 

- **`compressorOff`**
  - EN: Compressor OFF
  - KO: 컴프레서 끄기
  - [ ] OK    Fix: 

- **`compressorOn`**
  - EN: Compressor ON
  - KO: 컴프레서 켜기
  - [ ] OK    Fix: 

- **`ratio`**
  - EN: Ratio:
  - KO: 비율:
  - [ ] OK    Fix: 

- **`release`**
  - EN: Release:
  - KO: 릴리스:
  - [ ] OK    Fix: 

- **`threshold`**
  - EN: Threshold:
  - KO: 임계값:
  - [ ] OK    Fix: 

## `reverbs.sections`

- **`reverbFeed`**
  - EN: Reverb Feed
  - KO: 리버브 센드
  - [ ] OK    Fix: 

- **`reverbReturn`**
  - EN: Reverb Return
  - KO: 리버브 리턴
  - [ ] OK    Fix: 

## `reverbs.tabs`

- **`algorithm`**
  - EN: Algorithm
  - KO: 알고리즘
  - [ ] OK    Fix: 

- **`channelParams`**
  - EN: Channel Parameters
  - KO: 채널 매개변수
  - [ ] OK    Fix: 

- **`postProcessing`**
  - EN: Post-Processing
  - KO: 후처리
  - [ ] OK    Fix: 

- **`preProcessing`**
  - EN: Pre-Processing
  - KO: 전처리
  - [ ] OK    Fix: 

## `reverbs.toggles`

- **`liveSourceOff`**
  - EN: Live Source Atten OFF
  - KO: 라이브 소스 감쇠 끄기
  - [ ] OK    Fix: 

- **`liveSourceOn`**
  - EN: Live Source Atten ON
  - KO: 라이브 소스 감쇠 켜기
  - [ ] OK    Fix: 

- **`minLatencyOff`**
  - EN: Minimal Latency OFF
  - KO: 최소 레이턴시 끄기
  - [ ] OK    Fix: 

- **`minLatencyOn`**
  - EN: Minimal Latency ON
  - KO: 최소 레이턴시 켜기
  - [ ] OK    Fix: 

## `sampler.buttons`

- **`copy`**
  - EN: Copy
  - KO: 복사
  - [ ] OK    Fix: 

- **`copyCell`**
  - EN: Copy Cell
  - KO: 셀 복사
  - [ ] OK    Fix: 

- **`copySet`**
  - EN: Copy Set
  - KO: 세트 복사
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - KO: 내보내기
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - KO: 가져오기
  - [ ] OK    Fix: 

- **`paste`**
  - EN: Paste
  - KO: 붙여넣기
  - [ ] OK    Fix: 

- **`pasteCell`**
  - EN: Paste Cell
  - KO: 셀 붙여넣기
  - [ ] OK    Fix: 

- **`pasteSet`**
  - EN: Paste Set
  - KO: 세트 붙여넣기
  - [ ] OK    Fix: 

## `sampler.cell`

- **`attenuation`**
  - EN: Attenuation (dB)
  - KO: 감쇠 (dB)
  - [ ] OK    Fix: 

- **`clear`**
  - EN: Clear
  - KO: 지우기
  - [ ] OK    Fix: 

- **`inOut`**
  - EN: In/Out (ms)
  - KO: 인/아웃 (ms)
  - [ ] OK    Fix: 

- **`load`**
  - EN: Load
  - KO: 불러오기
  - [ ] OK    Fix: 

- **`loadTitle`**
  - EN: Load Sample
  - KO: 샘플 불러오기
  - [ ] OK    Fix: 

- **`offset`**
  - EN: Offset (m)
  - KO: 오프셋 (m)
  - [ ] OK    Fix: 

- **`preview`**
  - EN: Preview
  - KO: 미리듣기
  - [ ] OK    Fix: 

- **`previewStop`**
  - EN: Stop
  - KO: 정지
  - [ ] OK    Fix: 

## `sampler.grid`

- **`help`**
  - EN: Click = select
Shift = multi
Ctrl = set toggle
DblClick = load
  - KO: 클릭=선택 | Shift=다중 | Ctrl=세트 전환 | 더블클릭=로드
  - [ ] OK    Fix: 

## `sampler.labels`

- **`lightpadZone`**
  - EN: Lightpad Zone
  - KO: Lightpad 영역
  - [ ] OK    Fix: 

- **`selectZone`**
  - EN: Select Zone
  - KO: 영역 선택
  - [ ] OK    Fix: 

## `sampler.lightpadZone`

- **`none`**
  - EN: None
  - KO: 없음
  - [ ] OK    Fix: 

## `sampler.press`

- **`height`**
  - EN: Height
  - KO: 높이
  - [ ] OK    Fix: 

- **`hf`**
  - EN: HF Shelf
  - KO: HF Shelf
  - [ ] OK    Fix: 

- **`level`**
  - EN: Level
  - KO: 레벨
  - [ ] OK    Fix: 

- **`xy`**
  - EN: Position XY
  - KO: Position XY
  - [ ] OK    Fix: 

## `sampler.remote`

- **`gridLayout`**
  - EN: Grid Layout
  - KO: 그리드 레이아웃
  - [ ] OK    Fix: 

## `sampler.section`

- **`actions`**
  - EN: ACTIONS
  - KO: 동작
  - [ ] OK    Fix: 

- **`cell`**
  - EN: CELL PROPERTIES
  - KO: 셀 속성
  - [ ] OK    Fix: 

- **`pressure`**
  - EN: PRESSURE MAPPINGS
  - KO: 압력 매핑
  - [ ] OK    Fix: 

- **`set`**
  - EN: SET MANAGEMENT
  - KO: 세트 관리
  - [ ] OK    Fix: 

## `sampler.set`

- **`copy`**
  - EN: (copy)
  - KO: (복사)
  - [ ] OK    Fix: 

- **`default`**
  - EN: Set
  - KO: 세트
  - [ ] OK    Fix: 

- **`level`**
  - EN: Level (dB)
  - KO: 레벨 (dB)
  - [ ] OK    Fix: 

- **`pos`**
  - EN: Position (m)
  - KO: 위치 (m)
  - [ ] OK    Fix: 

- **`rename`**
  - EN: Rename
  - KO: 이름 변경
  - [ ] OK    Fix: 

- **`roundRobin`**
  - EN: Round-Robin
  - KO: 라운드 로빈
  - [ ] OK    Fix: 

- **`sequential`**
  - EN: Sequential
  - KO: 순차
  - [ ] OK    Fix: 

## `sampler.tooltips`

- **`addSet`**
  - EN: Create a new set. If cells are selected they will be assigned to it.
  - KO: 새 세트 만들기. 셀이 선택되어 있으면 할당됩니다.
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Cell attenuation in dB (0 = no attenuation, -60 = silent)
  - KO: 셀 감쇠 (dB, 0 = 감쇠 없음, -60 = 무음)
  - [ ] OK    Fix: 

- **`clear`**
  - EN: Remove the sample from the selected cell (long press)
  - KO: 선택한 셀에서 샘플 제거 (길게 누르기)
  - [ ] OK    Fix: 

- **`copy`**
  - EN: Copy selected cell or active set to clipboard
  - KO: 선택한 셀 또는 활성 세트를 클립보드에 복사
  - [ ] OK    Fix: 

- **`deleteSet`**
  - EN: Delete the active set (long press)
  - KO: 활성 세트 삭제 (길게 누르기)
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export sampler configuration to file
  - KO: 샘플러 구성을 파일로 내보내기
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import sampler configuration from file
  - KO: 샘플러 구성을 파일에서 가져오기
  - [ ] OK    Fix: 

- **`inOut`**
  - EN: Set the In/Out time range in milliseconds. Drag between thumbs to shift both.
  - KO: 인/아웃 시간 범위를 밀리초로 설정. 썸 사이를 드래그하여 둘 다 이동.
  - [ ] OK    Fix: 

- **`load`**
  - EN: Load a sample file into the selected cell
  - KO: 선택한 셀에 샘플 파일 로드
  - [ ] OK    Fix: 

- **`offset`**
  - EN: Offset position in meters (X, Y, Z) relative to the set position
  - KO: 세트 위치에 대한 미터 단위의 위치 오프셋 (X, Y, Z)
  - [ ] OK    Fix: 

- **`paste`**
  - EN: Paste clipboard data to selected cell or active set
  - KO: 클립보드 데이터를 선택한 셀 또는 활성 세트에 붙여넣기
  - [ ] OK    Fix: 

- **`playMode`**
  - EN: Toggle between Sequential and Round-Robin playback
  - KO: 순차 및 라운드 로빈 재생 사이 전환
  - [ ] OK    Fix: 

- **`pressCurve`**
  - EN: Pressure response curve (0 = concave, 0.5 = linear, 1 = convex)
  - KO: 압력 응답 곡선 (0 = 오목, 0.5 = 선형, 1 = 볼록)
  - [ ] OK    Fix: 

- **`pressDir`**
  - EN: Toggle pressure direction: + = more pressure increases, - = decreases
  - KO: 압력 방향 전환: + = 더 큰 압력이 증가, - = 감소
  - [ ] OK    Fix: 

- **`pressHeight`**
  - EN: Map finger pressure to vertical position (Z)
  - KO: 손가락 압력을 수직 위치 (Z)에 매핑
  - [ ] OK    Fix: 

- **`pressHF`**
  - EN: Map finger pressure to high-frequency shelf attenuation
  - KO: 손가락 압력을 하이쉘프 감쇠에 매핑
  - [ ] OK    Fix: 

- **`pressLevel`**
  - EN: Map finger pressure to output level
  - KO: 손가락 압력을 출력 레벨에 매핑
  - [ ] OK    Fix: 

- **`pressXY`**
  - EN: Map finger pressure to XY position movement
  - KO: 손가락 압력을 XY 위치 이동에 매핑
  - [ ] OK    Fix: 

- **`pressXYScale`**
  - EN: Sensitivity: how far the source moves per pressure step
  - KO: 감도: 압력 단계당 소스가 이동하는 거리
  - [ ] OK    Fix: 

- **`preview`**
  - EN: Preview the loaded sample
  - KO: 로드된 샘플 미리듣기
  - [ ] OK    Fix: 

- **`renameSet`**
  - EN: Rename the active set
  - KO: 활성 세트 이름 바꾸기
  - [ ] OK    Fix: 

- **`setLevel`**
  - EN: Set output level in dB (0 = unity, -60 = silent)
  - KO: 출력 레벨을 dB로 설정 (0 = 단위, -60 = 무음)
  - [ ] OK    Fix: 

- **`setPos`**
  - EN: Set base position in meters (X, Y, Z)
  - KO: 기본 위치를 미터 단위로 설정 (X, Y, Z)
  - [ ] OK    Fix: 

## `sampler.zone`

- **`remotePad`**
  - EN: Pad {num}
  - KO: Pad {num}
  - [ ] OK    Fix: 

- **`selectRemotePad`**
  - EN: Select Remote Pad
  - KO: 리모컨 패드 선택
  - [ ] OK    Fix: 

## `setAllInputs`

- **`warning`**
  - EN: Changes will apply to ALL inputs
  - KO: 변경 사항이 모든 입력에 적용됩니다
  - [ ] OK    Fix: 

- **`windowTitle`**
  - EN: Set All Inputs
  - KO: 모든 입력 설정
  - [ ] OK    Fix: 

## `setAllInputs.buttons`

- **`all1d`**
  - EN: All 1/d
  - KO: 모두 1/d
  - [ ] OK    Fix: 

- **`allLog`**
  - EN: All Log
  - KO: 모두 로그
  - [ ] OK    Fix: 

- **`closeWindow`**
  - EN: CLOSE WINDOW
  - KO: 창 닫기
  - [ ] OK    Fix: 

- **`flipXyzOff`**
  - EN: Flip XYZ > OFF
  - KO: XYZ 반전 > 끄기
  - [ ] OK    Fix: 

- **`resetDirectivity`**
  - EN: Reset directivity
  - KO: 지향성 초기화
  - [ ] OK    Fix: 

- **`turnOffJitterLfo`**
  - EN: Turn OFF jitter & LFO
  - KO: 지터 & LFO 끄기
  - [ ] OK    Fix: 

- **`turnOffLiveSource`**
  - EN: Turn OFF Live source atten.
  - KO: 라이브 소스 감쇠 끄기
  - [ ] OK    Fix: 

## `setAllInputs.coordinateModes`

- **`rThetaPhi`**
  - EN: r θ φ
  - KO: r θ φ
  - [ ] OK    Fix: 

- **`rThetaZ`**
  - EN: r θ Z
  - KO: r θ Z
  - [ ] OK    Fix: 

- **`xyz`**
  - EN: XYZ
  - KO: XYZ
  - [ ] OK    Fix: 

## `setAllInputs.labels`

- **`common`**
  - EN: common
  - KO: 공통
  - [ ] OK    Fix: 

- **`constraintPositions`**
  - EN: Constraint positions:
  - KO: 위치 제한:
  - [ ] OK    Fix: 

- **`coordinateMode`**
  - EN: Coordinate mode:
  - KO: 좌표 모드:
  - [ ] OK    Fix: 

- **`dbPerMeter`**
  - EN: dB/m
  - KO: dB/m
  - [ ] OK    Fix: 

- **`distanceAttenuation`**
  - EN: Distance attenuation
  - KO: 거리 감쇠
  - [ ] OK    Fix: 

- **`floorReflections`**
  - EN: Floor Reflections:
  - KO: 바닥 반사:
  - [ ] OK    Fix: 

- **`fringe`**
  - EN: Fringe:
  - KO: 프린지:
  - [ ] OK    Fix: 

- **`heightFactor`**
  - EN: Height factor:
  - KO: 높이 계수:
  - [ ] OK    Fix: 

- **`minimalLatency`**
  - EN: Minimal Latency:
  - KO: 최소 레이턴시:
  - [ ] OK    Fix: 

- **`muteMacros`**
  - EN: Mute macros:
  - KO: 음소거 매크로:
  - [ ] OK    Fix: 

- **`ratio`**
  - EN: ratio
  - KO: 비율
  - [ ] OK    Fix: 

- **`sidelines`**
  - EN: Sidelines:
  - KO: 사이드라인:
  - [ ] OK    Fix: 

## `snapshot`

- **`qlabExportDone`**
  - EN: QLab export complete: {count} cues created
  - KO: QLab 내보내기 완료: {count}개 큐 생성
  - [ ] OK    Fix: 

- **`qlabExportStarted`**
  - EN: Writing {count} cues to QLab...
  - KO: QLab에 {count}개 큐 쓰는 중...
  - [ ] OK    Fix: 

- **`qlabGroupName`**
  - EN: Snapshot "{name}"
  - KO: 스냅샷 "{name}"
  - [ ] OK    Fix: 

- **`qlabMemoText`**
  - EN: Run either of the following cues to recall or update this snapshot
  - KO: 다음 큐 중 하나를 실행하여 이 스냅샷을 불러오거나 업데이트하세요
  - [ ] OK    Fix: 

- **`qlabNoTarget`**
  - EN: No QLab target configured
  - KO: QLab 대상이 설정되지 않았습니다
  - [ ] OK    Fix: 

- **`qlabReloadName`**
  - EN: Reload "{name}"
  - KO: 불러오기 "{name}"
  - [ ] OK    Fix: 

- **`qlabUpdateName`**
  - EN: Update "{name}"
  - KO: 업데이트 "{name}"
  - [ ] OK    Fix: 

## `snapshotScope`

- **`all`**
  - EN: ALL
  - KO: 전체
  - [ ] OK    Fix: 

- **`applyScope`**
  - EN: Apply scope:
  - KO: 범위 적용:
  - [ ] OK    Fix: 

- **`autoPreselectModified`**
  - EN: Auto-preselect modified parameters
  - KO: 수정된 파라미터 자동 미리 선택
  - [ ] OK    Fix: 

- **`title`**
  - EN: Snapshot Scope: {name}
  - KO: 스냅샷 범위: {name}
  - [ ] OK    Fix: 

- **`whenRecalling`**
  - EN: When Recalling
  - KO: 호출 시
  - [ ] OK    Fix: 

- **`whenSaving`**
  - EN: When Saving
  - KO: 저장 시
  - [ ] OK    Fix: 

- **`windowTitle`**
  - EN: Input Snapshot Scope
  - KO: 입력 스냅샷 범위
  - [ ] OK    Fix: 

- **`writeSnapshotLoadCue`**
  - EN: Write Snapshot Load Cue to QLab
  - KO: 스냅샷 로드 큐를 QLab에 쓰기
  - [ ] OK    Fix: 

- **`writeSnapshotLoadCueTooltip`**
  - EN: Also create a QLab cue to load this snapshot via OSC
  - KO: OSC로 이 스냅샷을 로드하는 QLab 큐도 생성
  - [ ] OK    Fix: 

- **`writeToQLab`**
  - EN: Write to QLab
  - KO: QLab에 쓰기
  - [ ] OK    Fix: 

- **`writeToQLabTooltip`**
  - EN: Export scope to QLab instead of saving to file
  - KO: 파일 저장 대신 스코프를 QLab으로 내보내기
  - [ ] OK    Fix: 

## `snapshotScope.buttons`

- **`cancel`**
  - EN: Cancel
  - KO: 취소
  - [ ] OK    Fix: 

- **`clearChanges`**
  - EN: Clear Changes
  - KO: 변경 사항 지우기
  - [ ] OK    Fix: 

- **`ok`**
  - EN: OK
  - KO: OK
  - [ ] OK    Fix: 

- **`selectModified`**
  - EN: Select Modified
  - KO: 수정된 항목 선택
  - [ ] OK    Fix: 

## `snapshotScope.sections`

- **`attenuation`**
  - EN: Attenuation
  - KO: 감쇠
  - [ ] OK    Fix: 

- **`automOtion`**
  - EN: AutomOtion
  - KO: AutomOtion (자동이동)
  - [ ] OK    Fix: 

- **`directivity`**
  - EN: Directivity
  - KO: 지향성
  - [ ] OK    Fix: 

- **`hackoustics`**
  - EN: Hackoustics
  - KO: Hackoustics (가상음향)
  - [ ] OK    Fix: 

- **`input`**
  - EN: Input
  - KO: 입력
  - [ ] OK    Fix: 

- **`lfo`**
  - EN: LFO
  - KO: LFO
  - [ ] OK    Fix: 

- **`liveSource`**
  - EN: Live Source
  - KO: 라이브 소스
  - [ ] OK    Fix: 

- **`mutes`**
  - EN: Mutes
  - KO: 음소거
  - [ ] OK    Fix: 

- **`position`**
  - EN: Position
  - KO: 위치
  - [ ] OK    Fix: 

## `statusBar`

- **`displayLabel`**
  - EN: Display:
  - KO: 표시:
  - [ ] OK    Fix: 

- **`helpMode`**
  - EN: Help
  - KO: 도움말
  - [ ] OK    Fix: 

- **`oscMode`**
  - EN: OSC
  - KO: OSC
  - [ ] OK    Fix: 

## `systemConfig.algorithms`

- **`inputBuffer`**
  - EN: InputBuffer (read-time delays)
  - KO: InputBuffer (읽기 시간 지연)
  - [ ] OK    Fix: 

- **`outputBuffer`**
  - EN: OutputBuffer (write-time delays)
  - KO: OutputBuffer (쓰기 시간 지연)
  - [ ] OK    Fix: 

## `systemConfig.binauralOutput`

- **`select`**
  - EN: Select...
  - KO: 선택...
  - [ ] OK    Fix: 

## `systemConfig.buttons`

- **`audioPatch`**
  - EN: Audio Interface and Patching Window
  - KO: 오디오 인터페이스 및 패치 창
  - [ ] OK    Fix: 

- **`binauralOff`**
  - EN: Binaural: OFF
  - KO: 바이노럴: 끄기
  - [ ] OK    Fix: 

- **`binauralOn`**
  - EN: Binaural: ON
  - KO: 바이노럴: 켜기
  - [ ] OK    Fix: 

- **`clearSolo`**
  - EN: Clear Solo
  - KO: Clear Solo
  - [ ] OK    Fix: 

- **`copySystemInfo`**
  - EN: Copy System Info
  - KO: 시스템 정보 복사
  - [ ] OK    Fix: 

- **`diagnosticsCollapsed`**
  - EN: Diagnostics  [+]
  - KO: 진단  [+]
  - [ ] OK    Fix: 

- **`diagnosticsExpanded`**
  - EN: Diagnostics  [-]
  - KO: 진단  [-]
  - [ ] OK    Fix: 

- **`exportLogs`**
  - EN: Export Logs
  - KO: 로그 내보내기
  - [ ] OK    Fix: 

- **`exportSystem`**
  - EN: Export System Configuration
  - KO: 시스템 설정 내보내기
  - [ ] OK    Fix: 

- **`importSystem`**
  - EN: Import System Configuration
  - KO: 시스템 설정 가져오기
  - [ ] OK    Fix: 

- **`levelMeter`**
  - EN: Level Meters
  - KO: Level Meters
  - [ ] OK    Fix: 

- **`openLogFolder`**
  - EN: Open Log Folder
  - KO: 로그 폴더 열기
  - [ ] OK    Fix: 

- **`processingOff`**
  - EN: Processing: OFF
  - KO: 처리: 끄기
  - [ ] OK    Fix: 

- **`processingOn`**
  - EN: Processing: ON
  - KO: 처리: 켜기
  - [ ] OK    Fix: 

- **`quickLongPressOff`**
  - EN: Normal
  - KO: 보통
  - [ ] OK    Fix: 

- **`quickLongPressOn`**
  - EN: Quick
  - KO: 빠르게
  - [ ] OK    Fix: 

- **`reloadComplete`**
  - EN: Reload Complete Configuration
  - KO: 전체 설정 다시 불러오기
  - [ ] OK    Fix: 

- **`reloadCompleteBackup`**
  - EN: Reload Complete Config. Backup
  - KO: 백업에서 전체 설정 불러오기
  - [ ] OK    Fix: 

- **`reloadSystem`**
  - EN: Reload System Configuration
  - KO: 시스템 설정 다시 불러오기
  - [ ] OK    Fix: 

- **`reloadSystemBackup`**
  - EN: Reload System Config. Backup
  - KO: 백업에서 시스템 설정 불러오기
  - [ ] OK    Fix: 

- **`reportIssue`**
  - EN: Report Issue
  - KO: 문제 신고
  - [ ] OK    Fix: 

- **`selectProjectFolder`**
  - EN: Select Project Folder
  - KO: 프로젝트 폴더 선택
  - [ ] OK    Fix: 

- **`setup`**
  - EN: Setup
  - KO: 설정
  - [ ] OK    Fix: 

- **`soloModeMulti`**
  - EN: Solo: Multi
  - KO: Solo: 다중
  - [ ] OK    Fix: 

- **`soloModeSingle`**
  - EN: Solo: Single
  - KO: Solo: 단일
  - [ ] OK    Fix: 

- **`storeComplete`**
  - EN: Store Complete Configuration
  - KO: 전체 설정 저장
  - [ ] OK    Fix: 

- **`storeSystem`**
  - EN: Store System Configuration
  - KO: 시스템 설정 저장
  - [ ] OK    Fix: 

## `systemConfig.colorSchemes`

- **`black`**
  - EN: Black
  - KO: 검정
  - [ ] OK    Fix: 

- **`default`**
  - EN: Default (Dark Gray)
  - KO: 기본 (짙은 회색)
  - [ ] OK    Fix: 

- **`light`**
  - EN: Light
  - KO: 밝은
  - [ ] OK    Fix: 

## `systemConfig.controller`

- **`lightpad`**
  - EN: Lightpad
  - KO: Lightpad
  - [ ] OK    Fix: 

- **`off`**
  - EN: Off
  - KO: 꺼짐
  - [ ] OK    Fix: 

- **`remote`**
  - EN: Remote
  - KO: 리모컨
  - [ ] OK    Fix: 

## `systemConfig.devices`

- **`off`**
  - EN: Off
  - KO: 꺼짐
  - [ ] OK    Fix: 

## `systemConfig.dialogs`

- **`exportSystemConfig`**
  - EN: Export System Configuration
  - KO: 시스템 설정 내보내기
  - [ ] OK    Fix: 

- **`importSystemConfig`**
  - EN: Import System Configuration
  - KO: 시스템 설정 가져오기
  - [ ] OK    Fix: 

- **`reduce`**
  - EN: Reduce
  - KO: 줄이기
  - [ ] OK    Fix: 

- **`reduceInputChannels.message`**
  - EN: Reducing from {current} to {new} input channels will remove settings for channels {start} to {end}.

This cannot be undone.
  - KO: {current}에서 {new}개의 입력 채널로 줄이면 채널 {start}부터 {end}까지의 설정이 제거됩니다.

이 작업은 취소할 수 없습니다.
  - [ ] OK    Fix: 

- **`reduceInputChannels.title`**
  - EN: Reduce Input Channels?
  - KO: 입력 채널을 줄이시겠습니까?
  - [ ] OK    Fix: 

- **`reduceOutputChannels.message`**
  - EN: Reducing from {current} to {new} output channels will remove settings for channels {start} to {end}.

This cannot be undone.
  - KO: {current}에서 {new}개의 출력 채널로 줄이면 채널 {start}부터 {end}까지의 설정이 제거됩니다.

이 작업은 취소할 수 없습니다.
  - [ ] OK    Fix: 

- **`reduceOutputChannels.title`**
  - EN: Reduce Output Channels?
  - KO: 출력 채널을 줄이시겠습니까?
  - [ ] OK    Fix: 

- **`reduceReverbChannels.message`**
  - EN: Reducing from {current} to {new} reverb channels will remove settings for channels {start} to {end}.

This cannot be undone.
  - KO: {current}에서 {new}개의 리버브 채널로 줄이면 채널 {start}부터 {end}까지의 설정이 제거됩니다.

이 작업은 취소할 수 없습니다.
  - [ ] OK    Fix: 

- **`reduceReverbChannels.title`**
  - EN: Reduce Reverb Channels?
  - KO: 리버브 채널을 줄이시겠습니까?
  - [ ] OK    Fix: 

- **`selectProjectFolder`**
  - EN: Select Project Folder
  - KO: 프로젝트 폴더 선택
  - [ ] OK    Fix: 

## `systemConfig.help`

- **`algorithm`**
  - EN: Select the rendering algorithm from the menu.
  - KO: 메뉴에서 렌더링 알고리즘을 선택하세요.
  - [ ] OK    Fix: 

- **`audioPatch`**
  - EN: Opens the Audio Interface and Patching Window.
  - KO: 오디오 인터페이스 및 패치 창을 엽니다.
  - [ ] OK    Fix: 

- **`binauralAngle`**
  - EN: Horizontal rotation of binaural listener view (degrees, 0 = facing stage).
  - KO: 바이노럴 청취자 시점의 수평 회전 (도, 0 = 무대 정면).
  - [ ] OK    Fix: 

- **`binauralAtten`**
  - EN: Overall level offset for binaural output (dB).
  - KO: 바이노럴 출력의 전체 레벨 오프셋 (dB).
  - [ ] OK    Fix: 

- **`binauralDelay`**
  - EN: Additional delay for binaural output (milliseconds).
  - KO: 바이노럴 출력의 추가 지연 (밀리초).
  - [ ] OK    Fix: 

- **`binauralDistance`**
  - EN: Distance of binaural listener from stage origin (meters).
  - KO: 바이노럴 청취자의 무대 원점으로부터의 거리 (미터).
  - [ ] OK    Fix: 

- **`binauralEnable`**
  - EN: Enable or disable the binaural renderer processing.
  - KO: 바이노럴 렌더러 처리를 활성화 또는 비활성화.
  - [ ] OK    Fix: 

- **`binauralOutput`**
  - EN: Select output channel pair for binaural monitoring. Off disables binaural output.
  - KO: 바이노럴 모니터링용 출력 채널 쌍 선택. Off는 바이노럴 출력을 비활성화.
  - [ ] OK    Fix: 

- **`colorScheme`**
  - EN: Select the color scheme: Default (dark gray), Black (pure black for OLED displays), or Light (daytime use).
  - KO: 색상 테마 선택: 기본 (짙은 회색), 검정 (OLED 디스플레이용) 또는 밝은 (주간 사용).
  - [ ] OK    Fix: 

- **`copySystemInfo`**
  - EN: Copy detailed system information to the clipboard for support requests.
  - KO: 지원 요청을 위해 자세한 시스템 정보를 클립보드로 복사합니다.
  - [ ] OK    Fix: 

- **`diagnosticsToggle`**
  - EN: Long-press to show or hide the diagnostic tools (export logs, open log folder, copy system info, report issue).
  - KO: 길게 누르면 진단 도구를 표시하거나 숨깁니다 (로그 내보내기, 로그 폴더 열기, 시스템 정보 복사, 문제 보고).
  - [ ] OK    Fix: 

- **`dialsAndButtons`**
  - EN: Select the hardware controller for dials and buttons: Stream Deck+ or XenceLabs Quick Keys.
  - KO: 다이얼 및 버튼용 하드웨어 컨트롤러 선택: Stream Deck+ 또는 XenceLabs Quick Keys.
  - [ ] OK    Fix: 

- **`domeElevation`**
  - EN: Dome elevation angle: 180 = hemisphere, 360 = full sphere.
  - KO: 돔 고도각: 180 = 반구, 360 = 전구.
  - [ ] OK    Fix: 

- **`exportLogs`**
  - EN: Export diagnostic logs to a zip file for debugging or support.
  - KO: 디버깅 또는 지원을 위해 진단 로그를 zip 파일로 내보냅니다.
  - [ ] OK    Fix: 

- **`exportSystem`**
  - EN: Export System Configuration to file (with file explorer window).
  - KO: 시스템 설정을 파일로 내보내기 (파일 탐색기 사용).
  - [ ] OK    Fix: 

- **`haasEffect`**
  - EN: Hass Effect to apply to the system. Will take into account the Latency Compensations (System, Input and Output).
  - KO: 시스템에 적용할 하스 효과. 지연 보상 (시스템, 입력 및 출력)을 고려합니다.
  - [ ] OK    Fix: 

- **`importSystem`**
  - EN: Import System Configuration from file (with file explorer window).
  - KO: 파일에서 시스템 설정 가져오기 (파일 탐색기 사용).
  - [ ] OK    Fix: 

- **`inputChannels`**
  - EN: Number of Input Channels.
  - KO: 입력 채널 수.
  - [ ] OK    Fix: 

- **`language`**
  - EN: Select the user interface language. Changes take full effect after restarting the application.
  - KO: 사용자 인터페이스 언어를 선택하세요. 애플리케이션을 재시작하면 변경 사항이 완전히 적용됩니다.
  - [ ] OK    Fix: 

- **`lightpadSetup`**
  - EN: Display the connected Roli Lightpads and allow for splitting them in 4 smaller pads.
  - KO: 연결된 Roli Lightpad를 표시하고 4개의 작은 패드로 분할할 수 있습니다.
  - [ ] OK    Fix: 

- **`masterLevel`**
  - EN: Master Level (affects all outputs).
  - KO: 마스터 레벨 (모든 출력에 영향).
  - [ ] OK    Fix: 

- **`openLogFolder`**
  - EN: Open the application's log folder in the system file explorer.
  - KO: 응용 프로그램의 로그 폴더를 시스템 파일 탐색기에서 엽니다.
  - [ ] OK    Fix: 

- **`originCenter`**
  - EN: Set origin to center of stage volume. Typical for Spherical Dome setups.
  - KO: 무대 부피의 중심에 원점 설정. 구형 돔 설정에 적합.
  - [ ] OK    Fix: 

- **`originCenterGround`**
  - EN: Set origin to center of stage at ground level. Typical for Surround or Cylindrical setups.
  - KO: 바닥 수준의 무대 중앙에 원점 설정. 서라운드 또는 실린더 설정에 적합.
  - [ ] OK    Fix: 

- **`originDepth`**
  - EN: Origin Y offset from stage center (0 = centered, negative = front/downstage).
  - KO: 무대 중심에서 원점 Y 오프셋 (0 = 중앙, 음수 = 앞/무대 앞).
  - [ ] OK    Fix: 

- **`originDismissInputs`**
  - EN: Long press to dismiss and keep current input positions.
  - KO: 길게 눌러 현재 입력 위치를 유지합니다.
  - [ ] OK    Fix: 

- **`originDismissOutputs`**
  - EN: Long press to dismiss and keep current output positions.
  - KO: 길게 눌러 현재 출력 위치를 유지합니다.
  - [ ] OK    Fix: 

- **`originDismissReverbs`**
  - EN: Long press to dismiss and keep current reverb positions.
  - KO: 길게 눌러 현재 리버브 위치를 유지합니다.
  - [ ] OK    Fix: 

- **`originFront`**
  - EN: Set origin to front center of stage. Typical for frontal stages.
  - KO: 무대 앞 중앙에 원점 설정. 정면 무대에 적합.
  - [ ] OK    Fix: 

- **`originHeight`**
  - EN: Origin Z offset from floor (0 = floor level, positive = above floor).
  - KO: 바닥에서 원점 Z 오프셋 (0 = 바닥 수준, 양수 = 바닥 위).
  - [ ] OK    Fix: 

- **`originShiftInputs`**
  - EN: Long press to shift all input positions by the origin change.
  - KO: 길게 눌러 원점 변경에 따라 모든 입력 위치를 이동합니다.
  - [ ] OK    Fix: 

- **`originShiftOutputs`**
  - EN: Long press to shift all output positions by the origin change.
  - KO: 길게 눌러 원점 변경에 따라 모든 출력 위치를 이동합니다.
  - [ ] OK    Fix: 

- **`originShiftReverbs`**
  - EN: Long press to shift all reverb positions by the origin change.
  - KO: 길게 눌러 원점 변경에 따라 모든 리버브 위치를 이동합니다.
  - [ ] OK    Fix: 

- **`originWidth`**
  - EN: Origin X offset from stage center (0 = centered, negative = left).
  - KO: 무대 중심에서 원점 X 오프셋 (0 = 중앙, 음수 = 왼쪽).
  - [ ] OK    Fix: 

- **`outputChannels`**
  - EN: Number of Output Channels.
  - KO: 출력 채널 수.
  - [ ] OK    Fix: 

- **`positionControl`**
  - EN: Select the hardware controller for position control: Space Mouse, Joystick, or Game Pad.
  - KO: 위치 제어용 하드웨어 컨트롤러 선택: Space Mouse, 조이스틱 또는 게임패드.
  - [ ] OK    Fix: 

- **`processing`**
  - EN: Lock all I/O parameters and start the DSP. Long press to stop the DSP.
  - KO: 모든 입출력 매개변수를 잠그고 DSP를 시작합니다. 길게 누르면 DSP가 중지됩니다.
  - [ ] OK    Fix: 

- **`quickLongPress`**
  - EN: Long press duration. Instead of confirmation windows this software uses long presses.
  - KO: 길게 누르기 시간. 확인 창 대신 이 소프트웨어는 길게 누르기를 사용합니다.
  - [ ] OK    Fix: 

- **`reloadComplete`**
  - EN: Reload Complete Configuration from files.
  - KO: 파일에서 전체 설정을 다시 불러오기.
  - [ ] OK    Fix: 

- **`reloadCompleteBackup`**
  - EN: Reload Complete Configuration from backup files.
  - KO: 백업 파일에서 전체 설정을 다시 불러오기.
  - [ ] OK    Fix: 

- **`reloadSystem`**
  - EN: Reload System Configuration from file.
  - KO: 파일에서 시스템 설정을 다시 불러오기.
  - [ ] OK    Fix: 

- **`reloadSystemBackup`**
  - EN: Reload System Configuration from backup file.
  - KO: 백업 파일에서 시스템 설정을 다시 불러오기.
  - [ ] OK    Fix: 

- **`remoteSetup`**
  - EN: Select the number of pads in the Remote's XY Pads tab.
  - KO: Remote의 XY Pads 탭에서 패드 수를 선택합니다.
  - [ ] OK    Fix: 

- **`reportIssue`**
  - EN: Open the WFS-DIY GitHub issues page in your default browser.
  - KO: WFS-DIY GitHub 이슈 페이지를 기본 브라우저에서 엽니다.
  - [ ] OK    Fix: 

- **`reverbChannels`**
  - EN: Number of Reverb Channels.
  - KO: 리버브 채널 수.
  - [ ] OK    Fix: 

- **`sampler`**
  - EN: Enable or disable the Sampler feature for input channels.
  - KO: 입력 채널의 Sampler 기능을 활성화 또는 비활성화. 컨트롤러 선택: Lightpad 또는 리모컨.
  - [ ] OK    Fix: 

- **`selectProjectFolder`**
  - EN: Select the Location of the Current Project Folder where to store files.
  - KO: 파일을 저장할 현재 프로젝트 폴더의 위치를 선택하세요.
  - [ ] OK    Fix: 

- **`showLocation`**
  - EN: Location of the current show.
  - KO: 현재 공연의 장소.
  - [ ] OK    Fix: 

- **`showName`**
  - EN: Name of the current show.
  - KO: 현재 공연의 이름.
  - [ ] OK    Fix: 

- **`soloMode`**
  - EN: Single: one input at a time. Multi: multiple inputs simultaneously.
  - KO: 단일: 한 번에 하나의 입력. 다중: 여러 입력 동시에.
  - [ ] OK    Fix: 

- **`speedOfSound`**
  - EN: Speed of Sound (related to the temperature).
  - KO: 음속 (온도와 관련).
  - [ ] OK    Fix: 

- **`stageDepth`**
  - EN: Depth of the stage in meters (Box shape only).
  - KO: 무대 깊이 (미터) (박스 형태만 해당).
  - [ ] OK    Fix: 

- **`stageDiameter`**
  - EN: Diameter of the stage in meters (Cylinder and Dome shapes).
  - KO: 무대 직경 (미터) (실린더 및 돔 형태).
  - [ ] OK    Fix: 

- **`stageDismiss`**
  - EN: Long press to dismiss and keep current input positions.
  - KO: 길게 눌러 현재 입력 위치를 유지합니다.
  - [ ] OK    Fix: 

- **`stageFit`**
  - EN: Long press to move outlier inputs back within the new stage bounds.
  - KO: 길게 눌러 범위를 벗어난 입력을 새 무대 경계 내로 이동합니다.
  - [ ] OK    Fix: 

- **`stageHeight`**
  - EN: Height of the stage in meters (Box and Cylinder shapes).
  - KO: 무대 높이 (미터) (박스 및 실린더 형태).
  - [ ] OK    Fix: 

- **`stageScale`**
  - EN: Long press to scale all input positions proportionally to the new stage dimensions.
  - KO: 길게 눌러 모든 입력 위치를 새 무대 크기에 비례하여 조정합니다.
  - [ ] OK    Fix: 

- **`stageShape`**
  - EN: Select the shape of the stage (Box, Cylinder, or Dome).
  - KO: 무대 형태를 선택하세요 (박스, 실린더 또는 돔).
  - [ ] OK    Fix: 

- **`stageWidth`**
  - EN: Width of the stage in meters (Box shape only).
  - KO: 무대 너비 (미터) (박스 형태만 해당).
  - [ ] OK    Fix: 

- **`storeComplete`**
  - EN: Store Complete Configuration to files (with backup).
  - KO: 전체 설정을 파일에 저장 (백업 포함).
  - [ ] OK    Fix: 

- **`storeSystem`**
  - EN: Store System Configuration to file (with backup).
  - KO: 시스템 설정을 파일에 저장 (백업 포함).
  - [ ] OK    Fix: 

- **`systemLatency`**
  - EN: Total latency of the system (Mixing board & Computer) / Specific Input and Output Latency/Delay can be set in the respective Input and Output settings.
  - KO: 시스템 총 지연 (믹싱 보드 및 컴퓨터) / 특정 입력 및 출력 지연은 각 설정에서 설정할 수 있습니다.
  - [ ] OK    Fix: 

- **`temperature`**
  - EN: Temperature (gives the Speed of Sound).
  - KO: 온도 (음속을 결정).
  - [ ] OK    Fix: 

## `systemConfig.labels`

- **`algorithm`**
  - EN: Algorithm:
  - KO: 알고리즘:
  - [ ] OK    Fix: 

- **`binauralAngle`**
  - EN: Listener Angle:
  - KO: 청취자 각도:
  - [ ] OK    Fix: 

- **`binauralAtten`**
  - EN: Binaural Level:
  - KO: 바이노럴 레벨:
  - [ ] OK    Fix: 

- **`binauralDelay`**
  - EN: Binaural Delay:
  - KO: 바이노럴 지연:
  - [ ] OK    Fix: 

- **`binauralDistance`**
  - EN: Listener Distance:
  - KO: 청취자 거리:
  - [ ] OK    Fix: 

- **`binauralOutput`**
  - EN: Binaural Output:
  - KO: 바이노럴 출력:
  - [ ] OK    Fix: 

- **`clickToSplit`**
  - EN: Click to split
  - KO: 분할하려면 클릭
  - [ ] OK    Fix: 

- **`colorScheme`**
  - EN: Color Scheme:
  - KO: 색상 테마:
  - [ ] OK    Fix: 

- **`dialsAndButtons`**
  - EN: Dials and Buttons:
  - KO: 다이얼 및 버튼:
  - [ ] OK    Fix: 

- **`domeElevation`**
  - EN: Elevation:
  - KO: 돔 각도:
  - [ ] OK    Fix: 

- **`haasEffect`**
  - EN: Haas Effect:
  - KO: 하스 효과:
  - [ ] OK    Fix: 

- **`inputChannels`**
  - EN: Input Channels:
  - KO: 입력 채널:
  - [ ] OK    Fix: 

- **`language`**
  - EN: Language:
  - KO: 언어:
  - [ ] OK    Fix: 

- **`lightpadArrangement`**
  - EN: Lightpad Arrangement
  - KO: Lightpad 배치
  - [ ] OK    Fix: 

- **`masterLevel`**
  - EN: Master Level:
  - KO: 마스터 레벨:
  - [ ] OK    Fix: 

- **`originDepth`**
  - EN: Origin Depth:
  - KO: 원점 깊이:
  - [ ] OK    Fix: 

- **`originHeight`**
  - EN: Origin Height:
  - KO: 원점 높이:
  - [ ] OK    Fix: 

- **`originWidth`**
  - EN: Origin Width:
  - KO: 원점 너비:
  - [ ] OK    Fix: 

- **`outputChannels`**
  - EN: Output Channels:
  - KO: 출력 채널:
  - [ ] OK    Fix: 

- **`positionControl`**
  - EN: Position Control:
  - KO: 위치 제어:
  - [ ] OK    Fix: 

- **`quickLongPress`**
  - EN: Long Press:
  - KO: 길게 누르기:
  - [ ] OK    Fix: 

- **`reverbChannels`**
  - EN: Reverb Channels:
  - KO: 리버브 채널:
  - [ ] OK    Fix: 

- **`sampler`**
  - EN: Sampler:
  - KO: Sampler:
  - [ ] OK    Fix: 

- **`showLocation`**
  - EN: Location:
  - KO: 장소:
  - [ ] OK    Fix: 

- **`showName`**
  - EN: Name:
  - KO: 이름:
  - [ ] OK    Fix: 

- **`speedOfSound`**
  - EN: Speed of Sound:
  - KO: 음속:
  - [ ] OK    Fix: 

- **`split`**
  - EN: Split
  - KO: 분할
  - [ ] OK    Fix: 

- **`stageDepth`**
  - EN: Depth:
  - KO: 깊이:
  - [ ] OK    Fix: 

- **`stageDiameter`**
  - EN: Diameter:
  - KO: 직경:
  - [ ] OK    Fix: 

- **`stageHeight`**
  - EN: Height:
  - KO: 높이:
  - [ ] OK    Fix: 

- **`stageShape`**
  - EN: Stage Shape:
  - KO: 무대 형태:
  - [ ] OK    Fix: 

- **`stageWidth`**
  - EN: Width:
  - KO: 너비:
  - [ ] OK    Fix: 

- **`systemLatency`**
  - EN: System Latency:
  - KO: 시스템 지연:
  - [ ] OK    Fix: 

- **`temperature`**
  - EN: Temperature:
  - KO: 온도:
  - [ ] OK    Fix: 

- **`updateAvailable`**
  - EN: Update {version} available
  - KO: 업데이트 {version} 사용 가능
  - [ ] OK    Fix: 

- **`version`**
  - EN: Version {version}
  - KO: 버전 {version}
  - [ ] OK    Fix: 

## `systemConfig.messages`

- **`configLoaded`**
  - EN: Complete configuration loaded.
  - KO: 전체 설정이 로드되었습니다.
  - [ ] OK    Fix: 

- **`configLoadedFromBackup`**
  - EN: Configuration loaded from backup.
  - KO: 백업에서 설정이 로드되었습니다.
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Complete configuration saved.
  - KO: 전체 설정이 저장되었습니다.
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - KO: 오류: {error}
  - [ ] OK    Fix: 

- **`languageChanged`**
  - EN: Language changed to: {language} (requires restart for full effect)
  - KO: 언어가 {language}(으)로 변경되었습니다 (완전히 적용하려면 재시작 필요)
  - [ ] OK    Fix: 

- **`logDirNotFound`**
  - EN: Log directory not found
  - KO: 로그 디렉터리를 찾을 수 없습니다
  - [ ] OK    Fix: 

- **`logsExported`**
  - EN: Logs exported to {path}
  - KO: 로그가 {path}에 내보내졌습니다
  - [ ] OK    Fix: 

- **`logsExportFailed`**
  - EN: Failed to export logs
  - KO: 로그 내보내기 실패
  - [ ] OK    Fix: 

- **`noBackupFilesFound`**
  - EN: No backup files found.
  - KO: 백업 파일을 찾을 수 없습니다.
  - [ ] OK    Fix: 

- **`partialLoad`**
  - EN: Partial load: {error}
  - KO: 부분 로드: {error}
  - [ ] OK    Fix: 

- **`partialLoadFromBackup`**
  - EN: Partial load from backup: {error}
  - KO: 백업에서 부분 로드: {error}
  - [ ] OK    Fix: 

- **`restartBody`**
  - EN: Please restart the application for the language change to take full effect.
  - KO: 언어 변경을 완전히 적용하려면 애플리케이션을 다시 시작해 주세요.
  - [ ] OK    Fix: 

- **`restartTitle`**
  - EN: Restart Required
  - KO: 재시작 필요
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder first.
  - KO: 먼저 프로젝트 폴더를 선택하세요.
  - [ ] OK    Fix: 

- **`selectLogExportFolder`**
  - EN: Select destination for log export
  - KO: 로그 내보내기 대상 폴더를 선택하세요
  - [ ] OK    Fix: 

- **`systemConfigExported`**
  - EN: System configuration exported.
  - KO: 시스템 설정을 내보냈습니다.
  - [ ] OK    Fix: 

- **`systemConfigFileNotFound`**
  - EN: System configuration file not found.
  - KO: 시스템 설정 파일을 찾을 수 없습니다.
  - [ ] OK    Fix: 

- **`systemConfigImported`**
  - EN: System configuration imported.
  - KO: 시스템 설정을 가져왔습니다.
  - [ ] OK    Fix: 

- **`systemConfigLoaded`**
  - EN: System configuration loaded.
  - KO: 시스템 설정이 로드되었습니다.
  - [ ] OK    Fix: 

- **`systemConfigLoadedFromBackup`**
  - EN: System configuration loaded from backup.
  - KO: 백업에서 시스템 설정이 로드되었습니다.
  - [ ] OK    Fix: 

- **`systemConfigSaved`**
  - EN: System configuration saved.
  - KO: 시스템 설정이 저장되었습니다.
  - [ ] OK    Fix: 

- **`systemInfoCopied`**
  - EN: System info copied to clipboard
  - KO: 시스템 정보가 클립보드에 복사되었습니다
  - [ ] OK    Fix: 

## `systemConfig.sections`

- **`binauralRenderer`**
  - EN: Binaural Renderer
  - KO: 바이노럴 렌더러
  - [ ] OK    Fix: 

- **`controllers`**
  - EN: Controllers
  - KO: Controllers
  - [ ] OK    Fix: 

- **`io`**
  - EN: I/O
  - KO: 입출력
  - [ ] OK    Fix: 

- **`master`**
  - EN: Master Section
  - KO: 마스터 섹션
  - [ ] OK    Fix: 

- **`show`**
  - EN: Show
  - KO: 공연
  - [ ] OK    Fix: 

- **`stage`**
  - EN: Stage
  - KO: 무대
  - [ ] OK    Fix: 

- **`ui`**
  - EN: UI
  - KO: 인터페이스
  - [ ] OK    Fix: 

- **`wfsProcessor`**
  - EN: WFS Processor
  - KO: WFS 프로세서
  - [ ] OK    Fix: 

## `systemConfig.stageShapes`

- **`box`**
  - EN: Box
  - KO: 박스
  - [ ] OK    Fix: 

- **`cylinder`**
  - EN: Cylinder
  - KO: 실린더
  - [ ] OK    Fix: 

- **`dome`**
  - EN: Dome
  - KO: 돔
  - [ ] OK    Fix: 

## `tabs`

- **`clusters`**
  - EN: Clusters
  - KO: 클러스터
  - [ ] OK    Fix: 

- **`inputs`**
  - EN: Inputs
  - KO: 입력
  - [ ] OK    Fix: 

- **`map`**
  - EN: Map
  - KO: 맵
  - [ ] OK    Fix: 

- **`network`**
  - EN: Network
  - KO: 네트워크
  - [ ] OK    Fix: 

- **`outputs`**
  - EN: Outputs
  - KO: 출력
  - [ ] OK    Fix: 

- **`reverb`**
  - EN: Reverb
  - KO: 리버브
  - [ ] OK    Fix: 

- **`systemConfig`**
  - EN: System Config
  - KO: 시스템 설정
  - [ ] OK    Fix: 

## `touchscreens`

- **`button`**
  - EN: Setup
  - KO: 설정
  - [ ] OK    Fix: 

- **`label`**
  - EN: Touchscreen
  - KO: 터치스크린
  - [ ] OK    Fix: 

## `units`

- **`celsius`**
  - EN: °C
  - KO: °C
  - [ ] OK    Fix: 

- **`decibelPerMeter`**
  - EN: dB/m
  - KO: dB/m
  - [ ] OK    Fix: 

- **`decibels`**
  - EN: dB
  - KO: dB
  - [ ] OK    Fix: 

- **`degrees`**
  - EN: °
  - KO: °
  - [ ] OK    Fix: 

- **`meters`**
  - EN: m
  - KO: m
  - [ ] OK    Fix: 

- **`metersPerSecond`**
  - EN: m/s
  - KO: m/s
  - [ ] OK    Fix: 

- **`milliseconds`**
  - EN: ms
  - KO: ms
  - [ ] OK    Fix: 

## `wizard.buttons`

- **`back`**
  - EN: Back
  - KO: 이전
  - [ ] OK    Fix: 

- **`close`**
  - EN: Close
  - KO: 닫기
  - [ ] OK    Fix: 

- **`done`**
  - EN: Done
  - KO: 완료
  - [ ] OK    Fix: 

- **`gettingStarted`**
  - EN: Getting Started
  - KO: 시작하기
  - [ ] OK    Fix: 

- **`gettingStartedHelp`**
  - EN: Help cards guiding you through the first parameters to adjust when starting a new project
  - KO: 새 프로젝트를 시작할 때 먼저 조정해야 할 매개변수를 안내하는 도움말 카드
  - [ ] OK    Fix: 

- **`next`**
  - EN: Next
  - KO: 다음
  - [ ] OK    Fix: 

- **`skip`**
  - EN: Skip
  - KO: 건너뛰기
  - [ ] OK    Fix: 

## `wizard.steps`

- **`audioDevice.description`**
  - EN: Select your audio driver and device, set the sample rate and buffer size. Check the patch routing and test your outputs. Close this window when done.
  - KO: 오디오 드라이버와 장치를 선택하고 샘플레이트와 버퍼 크기를 설정하세요. 패치 라우팅을 확인하고 출력을 테스트하세요. 완료되면 이 창을 닫으세요.
  - [ ] OK    Fix: 

- **`audioDevice.title`**
  - EN: Configure the Audio Interface
  - KO: 오디오 인터페이스 구성
  - [ ] OK    Fix: 

- **`audioInterface.description`**
  - EN: Click the button above or press Next to open the Audio Interface window.
  - KO: 위의 버튼을 클릭하거나 다음을 눌러 오디오 인터페이스 창을 엽니다.
  - [ ] OK    Fix: 

- **`audioInterface.title`**
  - EN: Open the Audio Interface
  - KO: 오디오 인터페이스 열기
  - [ ] OK    Fix: 

- **`configureOutputs.description`**
  - EN: Use the array presets and geometry tools to calculate speaker positions for your arrays. Close this window when done.
  - KO: 어레이 프리셋과 지오메트리 도구를 사용하여 스피커 위치를 계산하세요. 완료되면 이 창을 닫으세요.
  - [ ] OK    Fix: 

- **`configureOutputs.title`**
  - EN: Configure Output Positions
  - KO: 출력 위치 구성
  - [ ] OK    Fix: 

- **`exploreInputs.description`**
  - EN: Click an input on the map to select it, or lasso several to move them together. Drag to position your sources. Zoom with the mouse wheel or pinch gesture, pan with right-drag or two-finger drag. Add inputs, group them into clusters, and shape your sound field. You can also control positions with a keyboard, SpaceMouse, or other external controllers. Have fun!
  - KO: 맵에서 입력을 클릭하여 선택하거나 올가미로 여러 개를 선택하세요. 드래그하여 소스를 배치합니다. 마우스 휠이나 핀치로 줌, 우클릭 드래그나 두 손가락 드래그로 팬할 수 있습니다. 입력을 추가하고 클러스터로 그룹화하여 사운드 필드를 만드세요. 키보드, SpaceMouse 또는 기타 컨트롤러로도 위치를 제어할 수 있습니다. 즐기세요!
  - [ ] OK    Fix: 

- **`exploreInputs.title`**
  - EN: Start Creating!
  - KO: 만들기 시작!
  - [ ] OK    Fix: 

- **`inputChannels.description`**
  - EN: How many audio sources will you be spatializing?
Set the number of input channels to match your source count.
  - KO: 몇 개의 오디오 소스를 공간화하시겠습니까?
소스 수에 맞게 입력 채널 수를 설정하세요.
  - [ ] OK    Fix: 

- **`inputChannels.title`**
  - EN: Set Input Channels
  - KO: 입력 채널 수 설정
  - [ ] OK    Fix: 

- **`originPoint.description`**
  - EN: The origin is the reference point for all coordinates. Use the preset buttons or enter custom values. 'Front' places it at the audience edge.
  - KO: 원점은 모든 좌표의 기준점입니다. 프리셋 버튼을 사용하거나 사용자 정의 값을 입력하세요. 'Front'는 관객 쪽 가장자리에 배치합니다.
  - [ ] OK    Fix: 

- **`originPoint.title`**
  - EN: Set the Origin Point
  - KO: 원점 설정
  - [ ] OK    Fix: 

- **`outputChannels.description`**
  - EN: Set the number of output channels to match your speaker array.
Each output corresponds to one physical speaker.
  - KO: 스피커 어레이에 맞게 출력 채널 수를 설정하세요.
각 출력은 하나의 물리적 스피커에 해당합니다.
  - [ ] OK    Fix: 

- **`outputChannels.title`**
  - EN: Set Output Channels
  - KO: 출력 채널 수 설정
  - [ ] OK    Fix: 

- **`projectFolder.description`**
  - EN: Choose a folder to store your WFS project files. This will hold configurations, snapshots, IR files, and samples. Click the button to open the folder selector.
  - KO: WFS 프로젝트 파일을 저장할 폴더를 선택하세요. 구성, 스냅샷, IR 파일, 샘플이 저장됩니다. 버튼을 클릭하여 폴더 선택기를 엽니다.
  - [ ] OK    Fix: 

- **`projectFolder.title`**
  - EN: Select a Project Folder
  - KO: 프로젝트 폴더 선택
  - [ ] OK    Fix: 

- **`reverbChannels.description`**
  - EN: Reverb channels add room simulation. Set to 0 if you don't need reverb.
  - KO: 리버브 채널은 공간 시뮬레이션을 추가합니다. 리버브가 필요하지 않으면 0으로 설정하세요.
  - [ ] OK    Fix: 

- **`reverbChannels.title`**
  - EN: Set Reverb Channels
  - KO: 리버브 채널 수 설정
  - [ ] OK    Fix: 

- **`stageConfig.description`**
  - EN: Set the shape and dimensions of your performance space. Choose box, cylinder, or dome, then enter the size in meters.
  - KO: 공연 공간의 형태와 크기를 설정합니다. 박스, 실린더, 돔 중 선택하고 미터 단위로 크기를 입력하세요.
  - [ ] OK    Fix: 

- **`stageConfig.title`**
  - EN: Define the Stage
  - KO: 무대 정의
  - [ ] OK    Fix: 

- **`startProcessing.description`**
  - EN: You're all set! Long-press the Processing button to start the WFS engine. You can also start the Binaural Renderer for headphone monitoring.
  - KO: 준비 완료! Processing 버튼을 길게 눌러 WFS 엔진을 시작하세요. 헤드폰 모니터링용 바이노럴 렌더러도 시작할 수 있습니다.
  - [ ] OK    Fix: 

- **`startProcessing.title`**
  - EN: Start the WFS Engine
  - KO: WFS 엔진 시작
  - [ ] OK    Fix: 

- **`wizardOfOutZ.description`**
  - EN: Click the Wizard of OutZ button or press Next to open the output array helper.
  - KO: Wizard of OutZ 버튼을 클릭하거나 다음을 눌러 위치 지정 도우미를 엽니다.
  - [ ] OK    Fix: 

- **`wizardOfOutZ.title`**
  - EN: Position Your Outputs
  - KO: 출력 위치 지정
  - [ ] OK    Fix: 


