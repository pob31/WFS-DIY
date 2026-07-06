# Proofreading checklist — Japanese (日本語)

Locale: `ja`  |  Total keys: 663  |  Source: `Resources/lang/en.json` vs `Resources/lang/ja.json`

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
  - JA: MCP統合全体のメインスイッチ。OFFの場合、すべてのAIツール呼び出しが拒否されます。ONの場合、通常のティア処理が適用されます（クリティカルアクションのトグルは破壊的な呼び出しを別途制御します）。
  - [ ] OK    Fix: 

- **`gate`**
  - EN: Allow destructive AI actions (channel-count changes, port reconfigure, runDSP, etc.) AND skip the per-call confirm handshake for less-destructive tier-2 actions while open. Acts as a superset of the Tier 2 auto-confirm toggle. The red fill drains over 5 minutes, then auto-blocks again.
  - JA: 破壊的なAIアクション（チャンネル数の変更、ポート再設定、runDSPなど）を許可し、ティア2の破壊性が低いアクションでは開いている間呼び出しごとの確認ハンドシェイクをスキップします。ティア2自動確認トグルのスーパーセットとして機能します。赤い塗りつぶしは5分かけて減り、その後自動的に再度ブロックされます。
  - [ ] OK    Fix: 

- **`openHistory`**
  - EN: Open the AI History window: scrollable timeline of every recent AI change with per-row undo / redo and a step-by-step cursor.
  - JA: AI履歴ウィンドウを開く: 最近のすべてのAI変更を行ごとに元に戻す/やり直し可能なステップバイステップカーソル付きのスクロール可能なタイムライン。
  - [ ] OK    Fix: 

- **`urlButton`**
  - EN: Click to copy the MCP server URL. Useful for Claude Code (claude mcp add wfs-diy <URL> -t http) or any MCP client that takes a URL. Claude Desktop instead uses the JSON config snippet — open the (?) help card.
  - JA: クリックしてMCPサーバーURLをコピー。Claude Code（claude mcp add wfs-diy <URL> -t http）またはURLを受け付けるあらゆるMCPクライアントで便利。Claude Desktopは代わりにJSON設定スニペットを使用 — (?)ヘルプカードを開いてください。
  - [ ] OK    Fix: 

## `arrayHelper.status`

- **`applied`**
  - EN: Applied {count} speakers to Array {array}. Ready for next array.
  - JA: {count}個のスピーカーをアレイ{array}に適用しました。次のアレイの準備完了。
  - [ ] OK    Fix: 

- **`calculated`**
  - EN: Calculated {count} positions
  - JA: {count}個の位置を計算しました
  - [ ] OK    Fix: 

- **`ready`**
  - EN: Ready
  - JA: 準備完了
  - [ ] OK    Fix: 

## `audioPatch.dialogs`

- **`unpatchInputsMessage`**
  - EN: Are you sure you want to remove all input patches?
  - JA: すべての入力パッチを削除してもよろしいですか？
  - [ ] OK    Fix: 

- **`unpatchInputsTitle`**
  - EN: Unpatch All Inputs
  - JA: すべての入力を解除
  - [ ] OK    Fix: 

- **`unpatchOutputsMessage`**
  - EN: Are you sure you want to remove all output patches?
  - JA: すべての出力パッチを削除してもよろしいですか？
  - [ ] OK    Fix: 

- **`unpatchOutputsTitle`**
  - EN: Unpatch All Outputs
  - JA: すべての出力を解除
  - [ ] OK    Fix: 

## `audioPatch.messages`

- **`chooseTestSignal`**
  - EN: Choose a Test Signal to Enable Testing
  - JA: テストを有効にするにはテスト信号を選択してください
  - [ ] OK    Fix: 

## `clusters.help`

- **`attenuationSlider`**
  - EN: Adjust attenuation of all cluster inputs (dB).
  - JA: クラスター内のすべての入力の減衰を調整 (dB)。
  - [ ] OK    Fix: 

- **`exportPresets`**
  - EN: Export all 16 LFO presets to an XML file.
  - JA: 16個のLFOプリセットをすべてXMLファイルにエクスポート。
  - [ ] OK    Fix: 

- **`importPresets`**
  - EN: Import LFO presets from an XML file.
  - JA: XMLファイルからLFOプリセットをインポート。
  - [ ] OK    Fix: 

- **`inputsVisibility`**
  - EN: Show or hide this cluster's inputs on the Map tab. Hide propagates to new members; removing an input restores its visibility.
  - JA: このクラスターの入力をマップタブで表示/非表示にします。非表示は新しいメンバーにも適用され、入力を削除すると再び表示されます。
  - [ ] OK    Fix: 

- **`lfoActiveButton`**
  - EN: Enable or Disable the Periodic Movement of the Cluster (LFO).
  - JA: クラスターの周期的動作 (LFO) を有効/無効にします。
  - [ ] OK    Fix: 

- **`lfoAmplitudeRotSlider`**
  - EN: Maximum Rotation Angle (-360 to 360 Degrees).
  - JA: 最大回転角度（-360〜360度）。
  - [ ] OK    Fix: 

- **`lfoAmplitudeScaleSlider`**
  - EN: Maximum Scale Factor (0.1x to 10x, Logarithmic).
  - JA: 最大スケール係数（0.1×〜10×、対数）。
  - [ ] OK    Fix: 

- **`lfoAmplitudeXSlider`**
  - EN: Width of Movement in Relation to Cluster Reference Position.
  - JA: クラスター参照位置に対する幅方向の動きの大きさ。
  - [ ] OK    Fix: 

- **`lfoAmplitudeYSlider`**
  - EN: Depth of Movement in Relation to Cluster Reference Position.
  - JA: クラスター参照位置に対する奥行き方向の動きの大きさ。
  - [ ] OK    Fix: 

- **`lfoAmplitudeZSlider`**
  - EN: Height of Movement in Relation to Cluster Reference Position.
  - JA: クラスター参照位置に対する高さ方向の動きの大きさ。
  - [ ] OK    Fix: 

- **`lfoPeriodDial`**
  - EN: Base Period of the Cluster Movement.
  - JA: クラスター動作の基本周期。
  - [ ] OK    Fix: 

- **`lfoPhaseDial`**
  - EN: Global Phase Offset of the Cluster Movement.
  - JA: クラスター動作のグローバル位相オフセット。
  - [ ] OK    Fix: 

- **`lfoPhaseRotDial`**
  - EN: Phase Offset of the Cluster Rotation.
  - JA: クラスター回転の位相オフセット。
  - [ ] OK    Fix: 

- **`lfoPhaseScaleDial`**
  - EN: Phase Offset of the Cluster Scaling.
  - JA: クラスタースケーリングの位相オフセット。
  - [ ] OK    Fix: 

- **`lfoPhaseXDial`**
  - EN: Phase Offset of the Cluster Movement in Width.
  - JA: 幅方向のクラスターの動きの位相オフセット。
  - [ ] OK    Fix: 

- **`lfoPhaseYDial`**
  - EN: Phase Offset of the Cluster Movement in Depth.
  - JA: 奥行き方向のクラスターの動きの位相オフセット。
  - [ ] OK    Fix: 

- **`lfoPhaseZDial`**
  - EN: Phase Offset of the Cluster Movement in Height.
  - JA: 高さ方向のクラスターの動きの位相オフセット。
  - [ ] OK    Fix: 

- **`lfoRateRotSlider`**
  - EN: Faster or Slower Rotation in Relation to Base Period.
  - JA: ベース周期に対する速い/遅い回転。
  - [ ] OK    Fix: 

- **`lfoRateScaleSlider`**
  - EN: Faster or Slower Scaling in Relation to Base Period.
  - JA: ベース周期に対する速い/遅いスケーリング。
  - [ ] OK    Fix: 

- **`lfoRateXSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Width.
  - JA: 幅方向のベース周期に対する速い/遅い動き。
  - [ ] OK    Fix: 

- **`lfoRateYSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Depth.
  - JA: 奥行き方向のベース周期に対する速い/遅い動き。
  - [ ] OK    Fix: 

- **`lfoRateZSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Height.
  - JA: 高さ方向のベース周期に対する速い/遅い動き。
  - [ ] OK    Fix: 

- **`lfoShapeRotSelector`**
  - EN: Rotation Behaviour of the Cluster.
  - JA: クラスターの回転挙動。
  - [ ] OK    Fix: 

- **`lfoShapeScaleSelector`**
  - EN: Scale Behaviour of the Cluster.
  - JA: クラスターのスケール挙動。
  - [ ] OK    Fix: 

- **`lfoShapeXSelector`**
  - EN: Movement Behaviour of the Cluster in Width.
  - JA: 幅方向のクラスターの動きの挙動。
  - [ ] OK    Fix: 

- **`lfoShapeYSelector`**
  - EN: Movement Behaviour of the Cluster in Depth.
  - JA: 奥行き方向のクラスターの動きの挙動。
  - [ ] OK    Fix: 

- **`lfoShapeZSelector`**
  - EN: Movement Behaviour of the Cluster in Height.
  - JA: 高さ方向のクラスターの動きの挙動。
  - [ ] OK    Fix: 

- **`planeSelector`**
  - EN: Select the plane for rotation and scale operations (XY, XZ, YZ).
  - JA: 回転とスケールの操作用の平面を選択 (XY、XZ、YZ)。
  - [ ] OK    Fix: 

- **`positionJoystick`**
  - EN: Move all cluster inputs in X/Y. Hold and drag to translate.
  - JA: クラスター内のすべての入力をX/Yで移動。ホールドしてドラッグで平行移動。
  - [ ] OK    Fix: 

- **`presetTile`**
  - EN: Click: recall preset. Double-click: recall + start. Middle/right-click: store current LFO.
  - JA: クリック: プリセット呼び出し。ダブルクリック: 呼び出し+開始。中/右クリック: 現在のLFOを保存。
  - [ ] OK    Fix: 

- **`qlabPreset`**
  - EN: Create a QLab network cue to recall the last selected LFO preset for the current cluster.
  - JA: 現在のクラスターで最後に選択されたLFOプリセットを呼び出すQLabネットワークキューを作成。
  - [ ] OK    Fix: 

- **`referenceMode`**
  - EN: Select the reference point for cluster transforms: First Input, Barycenter, or Shared Position (all members coincide; scale and rotation apply to per-input offsets).
  - JA: クラスター変換の基準点を選択: First Input、重心、またはShared Position（全メンバーが同一位置に集まり、スケールと回転は各入力のオフセットに適用されます）。
  - [ ] OK    Fix: 

- **`rotationDial`**
  - EN: Rotate all cluster inputs around the reference point in the selected plane.
  - JA: 選択した平面上で、基準点を中心にクラスターのすべての入力を回転。
  - [ ] OK    Fix: 

- **`scaleJoystick`**
  - EN: Scale cluster inputs relative to the reference point in the selected plane.
  - JA: 選択した平面上で、基準点を中心にクラスター入力をスケーリング。
  - [ ] OK    Fix: 

- **`stopAllLFO`**
  - EN: Stop LFO on all 10 clusters.
  - JA: 10個すべてのクラスターでLFOを停止。
  - [ ] OK    Fix: 

- **`zSlider`**
  - EN: Move all cluster inputs along the Z axis (height).
  - JA: クラスター内のすべての入力をZ軸（高さ）に沿って移動。
  - [ ] OK    Fix: 

## `clusters.status`

- **`noInputs`**
  - EN: No inputs assigned
  - JA: 入力が割り当てられていません
  - [ ] OK    Fix: 

- **`trackedMarker`**
  - EN: [T]
  - JA: [T]
  - [ ] OK    Fix: 

- **`tracking`**
  - EN: Tracking: Input {num} (overrides reference)
  - JA: トラッキング: 入力 {num} (リファレンスを上書き)
  - [ ] OK    Fix: 

## `common`

- **`add`**
  - EN: Add
  - JA: 追加
  - [ ] OK    Fix: 

- **`all`**
  - EN: All
  - JA: すべて
  - [ ] OK    Fix: 

- **`apply`**
  - EN: Apply
  - JA: 適用
  - [ ] OK    Fix: 

- **`cancel`**
  - EN: Cancel
  - JA: キャンセル
  - [ ] OK    Fix: 

- **`close`**
  - EN: Close
  - JA: 閉じる
  - [ ] OK    Fix: 

- **`delete`**
  - EN: Delete
  - JA: 削除
  - [ ] OK    Fix: 

- **`disable`**
  - EN: Disable
  - JA: 無効
  - [ ] OK    Fix: 

- **`edit`**
  - EN: Edit
  - JA: 編集
  - [ ] OK    Fix: 

- **`enable`**
  - EN: Enable
  - JA: 有効
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export
  - JA: エクスポート
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import
  - JA: インポート
  - [ ] OK    Fix: 

- **`load`**
  - EN: Load
  - JA: 読込
  - [ ] OK    Fix: 

- **`no`**
  - EN: No
  - JA: いいえ
  - [ ] OK    Fix: 

- **`none`**
  - EN: None
  - JA: なし
  - [ ] OK    Fix: 

- **`off`**
  - EN: OFF
  - JA: OFF
  - [ ] OK    Fix: 

- **`ok`**
  - EN: OK
  - JA: OK
  - [ ] OK    Fix: 

- **`on`**
  - EN: ON
  - JA: ON
  - [ ] OK    Fix: 

- **`reload`**
  - EN: Reload
  - JA: 再読込
  - [ ] OK    Fix: 

- **`remove`**
  - EN: Remove
  - JA: 削除
  - [ ] OK    Fix: 

- **`reset`**
  - EN: Reset
  - JA: リセット
  - [ ] OK    Fix: 

- **`save`**
  - EN: Save
  - JA: 保存
  - [ ] OK    Fix: 

- **`select`**
  - EN: Select
  - JA: 選択
  - [ ] OK    Fix: 

- **`store`**
  - EN: Store
  - JA: 保存
  - [ ] OK    Fix: 

- **`yes`**
  - EN: Yes
  - JA: はい
  - [ ] OK    Fix: 

## `eq.status`

- **`off`**
  - EN: EQ OFF
  - JA: EQ オフ
  - [ ] OK    Fix: 

- **`on`**
  - EN: EQ ON
  - JA: EQ オン
  - [ ] OK    Fix: 

## `fileManager.dialogs`

- **`selectProjectFolder`**
  - EN: Select Project Folder
  - JA: プロジェクトフォルダーを選択
  - [ ] OK    Fix: 

## `help.admOsc`

- **`body`**
  - EN: ADM-OSC is a protocol aiming to improve interoperability for spatial sound. It sends Cartesian positions (X, Y, Z) or polar values (AED for Azimuth, Elevation, Distance) from the console or from a DAW's automation curves.\nData is sent normalised:\n- between -1.0 and 1.0 for X, Y and Z;\n- between 0.0 to 1.0 for distance,\n- between -180° to 180° for Azimuth\n- between -90° to 90° for elevation.\nThe origin point can be moved and the mapping can also be adjusted in different segments for the inner and outer parts of the stage.\nWhen dragging the handles on the graphs, holding the shift key will apply symmetrical adjustments on the opposite side.
  - JA: ADM-OSCは空間音響の相互運用性を向上させるプロトコルです。コンソールやDAWのオートメーションカーブからデカルト座標（X, Y, Z）または極座標値（AED：方位角、仰角、距離）を送信します。\nデータは正規化されて送信されます：\n- X、Y、Zは-1.0から1.0の間\n- 距離は0.0から1.0の間\n- 方位角は-180°から180°の間\n- 仰角は-90°から90°の間\n原点を移動でき、マッピングはステージの内側と外側で異なるセグメントに調整できます。\nグラフのハンドルをドラッグする際、Shiftキーを押すと反対側に対称的な調整が適用されます。
  - [ ] OK    Fix: 

- **`title`**
  - EN: ADM-OSC Mappings
  - JA: ADM-OSC Mappings
  - [ ] OK    Fix: 

## `help.automOtion`

- **`body`**
  - EN: One time movement can also be programmed and triggered manually or triggered by sound level.\nThe coordinates are either relative from the start position or absolute relative to the origin point.\nThe input can either stay at the end position or revert to the starting position.\nInput position cannot be changed while the input is moving this way, but interacting with the input will change the position offset of the input instead.\nFor audio level triggering select the sound level above which the movement will start. When the sound drops below the reset level the movement will be rearmed, ready to be triggered again. New movement automation will not be possible while the input is moving.
  - JA: 一回限りの動きをプログラムし、手動またはサウンドレベルでトリガーできます。\n座標は開始位置からの相対値または原点に対する絶対値です。\n入力は終了位置に留まるか開始位置に戻ることができます。\n移動中は位置を変更できませんが、入力との対話により位置オフセットが変更されます。\nオーディオレベルトリガーでは、動きが開始するしきい値を選択します。サウンドがリセットレベル以下になると動きが再準備されます。
  - [ ] OK    Fix: 

- **`title`**
  - EN: AutomOtion
  - JA: AutomOtion
  - [ ] OK    Fix: 

## `help.binaural`

- **`body`**
  - EN: The Binaural renderer is used for:\n- listening to a rough spatial mix on headphones,\n- creating a mix for stereo output,\n- listening to a single soloed track through the spatial processing.\nThis may take the place of your master mix if it's only feeding headphones and media mix.\nThe position of the listening position may be adjusted in depth from the origin point and in orientation. Delay and level settings allow you to eventually match the sound at the FOH position.
  - JA: Binaural Rendererの用途：\n- ヘッドフォンで大まかな空間ミックスを聴く\n- ステレオ出力用のミックスを作成する\n- ソロトラックを空間処理で聴く\nヘッドフォンとメディアミックスのみに使用する場合、マスターミックスの代わりになります。\nリスニング位置は原点からの奥行きと向きで調整できます。ディレイとレベルの設定でFOH位置の音に合わせることができます。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Binaural Renderer
  - JA: Binaural Renderer
  - [ ] OK    Fix: 

## `help.clusters`

- **`body`**
  - EN: Clusters are groups of inputs that can be manipulated and animated as a whole.\nEach input can only be part of one cluster.\nEach cluster can only have one input with tracking fully enabled. Then this input will become the reference points for the cluster.\nIf no input with tracking is part of the cluster then there are two modes for the reference point of the cluster. Either the first input assigned in the list becomes the reference or the barycentre, in other words the center of gravity or the middle of the shape formed by the assigned inputs.\nAll inputs of the clusters can be moved by dragging the reference point. The individual inputs (other than a first input that would be a reference point) can still be adjusted individually. Dragging an input with tracking activated that is also a reference point for a cluster will affect its position offset and the position of the other inputs of the cluster normally.\nAll inputs in a cluster can be rotated or scaled around the reference point.\nAll clusters can be assigned an animation via an LFO. The positions X, Y and Z, the rotation and scale of the cluster can be controlled. The LFO has a period and a phase setting. Each individual parameter has shape, amplitude, rate and phase. The LFO settings can be assigned to pads for a quick recall. A right click will store the LFO parameters to a pad. Double clicking the top of the pad will allow to edit the name of the preset. Clicking or tapping a pad will recall the settings whether the LFO is running or not, but it will not start it if is isn't. A double click/tap will load and start the LFO.\nAll input clusters share the same set of LFO presets.
  - JA: クラスターは全体として操作・アニメーションできる入力のグループです。\n各入力は1つのクラスターにのみ所属できます。\n各クラスターにはトラッキングが完全に有効な入力が1つだけ設定でき、その入力がクラスターの基準点になります。\nトラッキング入力がない場合、基準点には2つのモードがあります：リスト最初の入力か、割り当てられた入力の重心です。\n基準点をドラッグしてすべての入力を移動できます。個々の入力も個別に調整可能です。トラッキングが有効で基準点でもある入力をドラッグすると、その位置オフセットとクラスターの他の入力の位置が通常通り影響を受けます。\nクラスター内のすべての入力を基準点を中心に回転・スケーリングできます。\nすべてのクラスターにLFOアニメーションを割り当てられます。X、Y、Z位置、回転、スケールを制御できます。LFO設定をパッドに割り当てて素早く呼び出せます。右クリックでLFOパラメータをパッドに保存します。パッド上部をダブルクリックするとプリセット名を編集できます。パッドをクリックまたはタップするとLFOの実行状態に関係なく設定を呼び出しますが、停止中のLFOは開始しません。ダブルクリック/タップでLFOを読み込んで開始します。\nすべてのクラスターは同じLFOプリセットセットを共有します。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Clusters
  - JA: クラスター
  - [ ] OK    Fix: 

## `help.floorReflections`

- **`body`**
  - EN: Simulating the floor reflections improves the naturalness of the sound. We don't expect sounds to play in a soundproof anechoic chamber. This setting helps recreate the expected floor reflections.\nThe level of the floor reflections can be adjusted as well as the low cut and the high frequency shelf filters. Diffusion adds a bit of chaos to simulate the unevenness of the floor.
  - JA: 床面反射のシミュレーションは音の自然さを向上させます。無響室で音が再生されることは期待していません。この設定は期待される床面反射を再現するのに役立ちます。\n床面反射のレベルは、ローカットフィルターや高域シェルフフィルターと共に調整できます。ディフュージョンは床の凹凸をシミュレートするために少しのカオスを加えます。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Floor Reflections
  - JA: Floor Reflections
  - [ ] OK    Fix: 

## `help.gradientMap`

- **`body`**
  - EN: Gradient maps allow to control attenuation, height and high frequency filtering (shelf with a smooth slope centered at 1kHz) depending on the X, Y position. For example, you can fade out a sound when entering a certain zone, you can have high frequency roll-off when moving away from the front of the stage, you can automatically adjust the height of an actor even when they are standing on elevated platforms without having to control height manually.\nThere are three layers for attenuation, height and HF shelf. They can be toggled on and off and they also can be hidden. The focused layer will look dimmed if disabled. Unfocused layers will look dimmed if active and only the shape outlines will be visible if they are deactivated.\nEach layer has a mapping control for white and black to adjust the range of the effect. The curve setting adjusts the transition.\nEach layer can have editable shapes (rectangle, ellipse or polygon) with either a single shade of grey, a linear gradient or a radial gradient. End points of the gradients can be adjusted.\nWhen creating a polygon click for each corner. Double-clicking will create a last corner and close the shape.\nDouble-clicking an existing point on a rectangle or a polygon will remove this corner. Double-clicking on a side will add a new point.\nThe scale and rotation of each shape can be edited for its center or from the origin point.\nWhen enabled the corner points of the rectangles and polygons can also be edited individually.\nShapes and layers can be copied to another layer on the same input or any other input.\nGradient map settings are stored in the input files.
  - JA: グラデーションマップはX、Y位置に応じて減衰、高さ、高周波フィルタリング（1kHz中心のなめらかなスロープのシェルフ）を制御できます。例えば、特定のゾーンに入った時に音をフェードアウトしたり、ステージ前方から離れた時に高周波ロールオフを適用したり、高台に立つ俳優の高さを自動調整できます。\n減衰、高さ、HFシェルフの3つのレイヤーがあります。オン/オフの切り替えと非表示が可能です。\n各レイヤーには白と黒のマッピングコントロールがあり、エフェクトの範囲を調整します。カーブ設定で遷移を調整します。\n各レイヤーには編集可能な形状（矩形、楕円、ポリゴン）を配置でき、単色グレー、線形グラデーション、放射状グラデーションが使えます。\nポリゴン作成時は各角をクリックし、ダブルクリックで閉じます。\n既存の点をダブルクリックで削除、辺をダブルクリックで新しい点を追加します。\n形状やレイヤーは他のレイヤーにコピーできます。\n設定は入力ファイルに保存されます。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Gradient Maps
  - JA: Gradient Maps
  - [ ] OK    Fix: 

## `help.inputAdvanced`

- **`body`**
  - EN: - Sidelines and Fringe enable muting when an input comes close to the limits of a rectangular stage (except downstage).\n- Tracking can be enabled and the ID of the tracker can be selected. The amount of smoothing on the position can also be adjusted.\n- Maximum Speed can be engaged and the speed limit adjusted. The system will apply a gradual acceleration and deceleration when the movement starts and finishes. When Path is enabled, the system will track the path followed by the input and will not go in a straight line to the final position. This is especially handy if movements have to be operated manually.\n- Height Factor lets you work in 2D, when set to 0%, or full 3D, when set at 100%, and anything in between. It's the ratio of the height in the level and delay calculations. If you wish to use floor reflections, set it to 100% and use the parallax correction of the output parameters.
  - JA: - サイドラインとフリンジは、入力が長方形ステージの境界に近づいたとき（観客側を除く）にミュートを可能にします。\n- トラッキングを有効化し、トラッカーIDを選択できます。位置の平滑化も調整できます。\n- 最大速度を有効化し、速度制限を調整できます。システムは動きの開始と終了で段階的な加速と減速を適用します。パスモードが有効な場合、システムは入力の通った経路をたどり、最終位置への直線移動を行いません。これは動きを手動で操作する場合に特に便利です。\n- 高さ係数は、0%設定時の2D、100%設定時の完全な3D、その間のすべての値で動作することを可能にします。これはレベルとディレイの計算における高さの比率です。Floor Reflectionsを使用したい場合は、100%に設定し、出力パラメーターのパララックス補正を使用してください。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Advanced Controls
  - JA: アドバンスト制御
  - [ ] OK    Fix: 

## `help.inputBasic`

- **`body`**
  - EN: Inputs have a wide variety of settings to account for different situations necessitating realistic sound reinforcement or creative tools for sound design.\n- Input level can be adjusted.\n- Inputs can be delayed or they can try to take into account specific latency (digital processing of wireless transmission or digital effects) and compensate for it to better align the amplification and the acoustic sound.\n- Minimal Latency can be toggled instead of Acoustic Precedence. On the other hand this tries to let the sound out through the system as soon as possible. The system scans this input's feeds to the outputs for lowest delay and subtracts it from all delays and bypasses additional Haas effect. Here the idea would be to beat the acoustic sound on stage to try and place a sound in a slightly different position by altering the location first perceived.\n- The location (position and offset) for any input can be given in Cartesian, Cylindrical or Spherical coordinates independently from the stage shape or other channels.\n- The position can be constrained to the dimensions of the stage in Cartesian coordinates or to a specific distance range in polar coordinates.\n- Flip will take symmetrical position for the given coordinate around the origin point.\n- The joystick and vertical slider allow relative control of the position.\n- Inputs can be assigned to a cluster to group them for coordinated movements.
  - JA: インプットには、リアリスティックな音響補強やサウンドデザインのための創造的なツールなど、さまざまな状況に対応するための多くの設定があります。\n- インプットレベルを調整できます。\n- インプットを遅延させたり、特定のレイテンシー（ワイヤレス伝送やデジタルエフェクトのデジタル処理）を考慮して補償し、増幅と音響音をより適切に整合させることができます。\n- 音響先行の代わりにMinimal Latencyを有効にできます。これはシステムを通じてできるだけ早く音を出そうとします。システムはこのインプットの出力への送りを最小遅延でスキャンし、すべての遅延から差し引きます。\n- 位置（位置とオフセット）は、ステージの形状や他のチャンネルに関係なく、直交座標、円筒座標、球面座標で指定できます。\n- 位置は直交座標ではステージの寸法に、極座標では特定の距離範囲に制限できます。\n- フリップは原点を中心に対称位置を取ります。\n- ジョイスティックと垂直スライダーで位置の相対制御ができます。\n- インプットをクラスターに割り当てて、協調した動きのためにグループ化できます。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Inputs Basic Parameters
  - JA: インプット基本パラメータ
  - [ ] OK    Fix: 

## `help.inputHF`

- **`body`**
  - EN: When talking away, the timbre of a voice will sound less bright. Reproducing this was the initial goal here even though we usually want to have support for voices when not addressing the audience or in bi-frontal configurations. This can be put to good use for creative effects such as having a brighter reverb on a dampened direct sound.\nThe orientation of the input in azimuth and in pitch can be set as well as the angle where the high frequencies will not be filtered.\nThe HF Shelf will set the maximum attenuation in the back of the input. There is a smooth fade (like a cosine curve) from full brightness in front to damped at the rear.
  - JA: 話者が向きを変えると、声の音色は明るさが減ります。これを再現することが本来の目的でしたが、観客に向かって話していない場合やバイフロンタル構成でもサポートが必要です。減衰した直接音に対してより明るいリバーブを持たせるなど、クリエイティブな効果にも活用できます。\nインプットの方位角と仰角の向きを設定でき、高周波がフィルタリングされない角度も設定できます。\nHFシェルフはインプットの背面での最大減衰を設定します。前面の完全な明るさから背面の減衰まで、滑らかなフェード（コサインカーブのような）があります。
  - [ ] OK    Fix: 

- **`title`**
  - EN: High Frequency Directivity
  - JA: 高周波指向性
  - [ ] OK    Fix: 

## `help.inputLevel`

- **`body`**
  - EN: There are two level attenuation models. One where the level decreases with the distance by a given ratio in dB/m. Otherwise the level is halved each time the distance doubles. This might be more realistic, but it may be too loud close to the source or not give enough focus. The former may be less physically accurate, it offers usually better control to have a more even and stable mix.
  - JA: レベル減衰モデルは2つあります。距離に応じてdB/mの比率でレベルが減少するもの。もう一つは距離が2倍になるごとにレベルが半分になるものです。後者はよりリアルかもしれませんが、音源の近くで音が大きすぎたり、十分なフォーカスが得られない場合があります。前者は物理的に正確ではないかもしれませんが、通常はより均一で安定したミックスのためのより良いコントロールを提供します。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Level Adjustments
  - JA: レベル調整
  - [ ] OK    Fix: 

## `help.inputMutes`

- **`body`**
  - EN: You can set for each output array a specific attenuation for the selected input.\nYou can mute each send to any output individually. There are macros to speed up the process.
  - JA: 各出力アレイに対して、選択したインプットの特定の減衰を設定できます。\n各送りを任意の出力に対して個別にミュートできます。プロセスを高速化するためのマクロがあります。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Array Attenuation and Output Mutes
  - JA: Array Attenuation and Output Mutes
  - [ ] OK    Fix: 

## `help.lfo`

- **`body`**
  - EN: Input position can be automated. The LFO can control X, Y and Z position coordinates individually as well as the rotation of HF directivity (gyrophone).\nAdjust the global period and phase for the LFO.\nFor X, Y and Z coordinates select a shape, amplitude, rate and phase as desired. A circle in the XY plane would have sine for shape for X and for Y and an offset of ±90° between the two. A square would be the same but with keystone shapes.\nInput position can be moved while the LFO is running.
  - JA: 入力位置を自動化できます。LFOはX、Y、Z座標を個別に制御でき、HF指向性の回転（ジャイロフォン）も制御できます。\nLFOのグローバルな周期と位相を調整します。\nX、Y、Zそれぞれに形状、振幅、レート、位相を選択します。XY平面の円にはX、Yにサイン波形を使い±90°のオフセットを設定します。正方形は同じですがキーストーン形状を使います。\nLFO動作中も入力位置を移動できます。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Input LFO
  - JA: 入力LFO
  - [ ] OK    Fix: 

## `help.liveSource`

- **`body`**
  - EN: A loud source on stage may not need much reinforcement through the nearby speakers. Imagine an opera singer near the edge of the stage. Normally the level distribution would make the level louder near the input position. But if it is already loud enough we should be able to not over-amplify it. This function manages this.\nThe radius and shape describe how to attenuate the level for a specific source for speakers within the radius of influence of this input. There are various shapes: a V shaped linear effect; a U shape for fast decrease within the radius; a tight V effect or a mix of the previous (sine).\nThe attenuation can be constant or level dependent like a local compression reacting to transients and to average RMS level.
  - JA: ステージ上の大音量ソースは近くのスピーカーからの増幅が不要な場合があります。ステージ端のオペラ歌手を想像してください。通常、レベル配分は入力位置付近のレベルを上げます。しかし既に十分な音量であれば、過剰増幅すべきではありません。この機能がそれを管理します。\n半径と形状は、この入力の影響半径内のスピーカーに対するレベル減衰方法を記述します。様々な形状があります：V字型の線形効果、U字型の急速減衰、狭いV字型、またはそれらの混合（サイン）。\n減衰は一定にするか、トランジェントと平均RMSレベルに反応するローカルコンプレッションのようにレベル依存にできます。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Live Source Tamer
  - JA: Live Source Tamer
  - [ ] OK    Fix: 

## `help.map`

- **`body`**
  - EN: - A left click on an input or a cluster will allow to move it by dragging it. A single finger touch will do the same.\n- A left click with the shift key pressed will add or remove inputs to the selection. A double tap and drag will act the same way.\n- A left click drag will draw a selection rectangle to select multiple inputs and clusters at the same time.\n- A left double-click or tap will reset the position offset of the input.\n- A long left click or press with no movement will switch to the input tab with the focus on the selected input on release.\n- A left click away from any input will clear the selection.\n- A right click and drag will pan the view of the map. A two finger drag with no selected input or cluster will do the same if your operating system supports multitouch.\n- The mouse wheel will zoom in and out. A two finger pinch with no selected input or cluster will also zoom in and out.\n- A middle click will reset the view to fit the stage on the map display. There is also a dedicated set of buttons to reset the view to fit all inputs and to fit the stage respectively.\n- Selected inputs and clusters can also be moved with the arrow keys for X and Y and with the PageUp and PageDown keys for height. Hardware controllers can be used too.\n- When an input is touched, a second finger nearby can rotate the input directivity and adjust the height by pinching if your operating system allows multitouch interaction.\n- When a cluster is touched, a second finger nearby can rotate the cluster and scale it by pinching.\n- Inputs, output arrays and the reverb nodes can be hidden on the map.\n- Inputs can also be locked to prevent selecting and moving them on the map. They will still be moved by clusters, network commands, tracking and hardware controllers.\n- All reverb nodes can be moved on the map if this is enabled on the reverb tab. Holding the Ctrl/Cmd key will move each pair of reverb nodes in symmetry.\n- Inputs with offsets, LFO or with speed regulation will have a temporary position marker. But the point of interaction will remain the normal marker.\n- The Live Source Tamer radius will be displayed around input when activated.\n- There is a toggle to display the audio level for the inputs and outputs on the map tab, that's active when the audio processing is running.
  - JA: - 入力やクラスターを左クリックしてドラッグで移動できます。\n- Shiftキー+左クリックで選択に追加/削除できます。\n- 左クリックドラッグで選択矩形を描きます。\n- ダブルクリックで位置オフセットをリセットします。\n- 長押しで入力タブに切り替えます。\n- 入力以外をクリックで選択をクリアします。\n- 右クリックドラッグでマップをパンします。2本指ドラッグも同様です。\n- マウスホイールでズーム。2本指ピンチも同様です。\n- 中クリックでビューをリセットします。\n- 矢印キーでX/Y移動、PageUp/Downで高さ調整。\n- 2本目の指で指向性の回転と高さ調整ができます。\n- クラスターでは2本目の指で回転とスケーリングができます。\n- 入力、出力配列、リバーブノードを非表示にできます。\n- 入力をロックして選択・移動を防止できます。\n- リバーブノードを移動可能。Ctrl/Cmdでペアを対称移動します。\n- Live Source Tamerの半径が表示されます。\n- オーディオレベルをマップに表示できます。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Map
  - JA: マップ
  - [ ] OK    Fix: 

## `help.mcp`

- **`body`**
  - EN: The MCP server lets an AI assistant (Claude Desktop, Claude Code, ChatGPT with custom connectors) read and write the parameters of this WFS-DIY session over a local network connection.\n\nWhat the AI can do:\n• Read live state: channel counts, names, positions, attenuations, EQs, snapshots, clusters, the full parameter surface.\n• Move sources, rename channels, set cluster assignments, adjust the array layout, place outputs and reverbs.\n• Run guided workflows (system tuning walkthroughs, troubleshooting localization, snapshot management) via prepared prompt templates.\n\nOperator controls on this row:\n• AI: ON / OFF — master switch. When OFF every AI tool call is refused; when ON the AI works under the rules below.\n• AI critical actions: blocked / ALLOWED — the destructive actions (deleting snapshots, resetting DSP, changing channel counts) are blocked by default. Click to allow them for 10 minutes; the red fill drains as the window expires, then they auto-block again.\n• Open AI History — scrollable timeline of every recent AI change with per-row undo / redo and a step-by-step cursor.\n• The MCP URL button copies the server URL to the clipboard for AI clients that take a URL directly.\n\nOperator awareness:\n• Every AI action is recorded with origin tags. The AI History window shows the full timeline; per-row × reverses an action with its dependents.\n• If you manually adjust a parameter the AI just moved, the AI is notified and will not blindly retry. You always have the last word.\n• The Cmd/Ctrl+Alt+Z and Cmd/Ctrl+Alt+Y shortcuts undo and redo the last AI change without affecting your manual edits (which use plain Ctrl+Z as usual).\n\nTo add this server to Claude Desktop:\n  1. Open Settings → Developer → Edit Config.\n  2. Paste the JSON snippet below into claude_desktop_config.json (merge into the existing mcpServers block if you already have one).\n  3. Restart Claude Desktop. The server appears as 'wfs-diy' in the tools menu.\n\nTo add to Claude Code, run:\n  claude mcp add wfs-diy <url> -t http\n\nThe URL changes if you switch network interface or if the server falls back to a different port. The URL button on this row always reflects the live URL.
  - JA: MCP Serverは、AIアシスタント（Claude Desktop、Claude Code、カスタムコネクタ付きのChatGPT）が、ローカルネットワーク接続を介してこのWFS-DIYセッションのパラメーターを読み書きできるようにします。\n\nAIができること:\n• ライブ状態の読み取り: チャンネル数、名前、位置、減衰、EQ、スナップショット、クラスター、すべてのパラメーター。\n• ソースの移動、チャンネル名の変更、クラスター割り当ての設定、アレイレイアウトの調整、出力およびリバーブの配置。\n• 用意されたプロンプトテンプレートを介してガイド付きワークフロー（システムチューニングウォークスルー、定位のトラブルシューティング、スナップショット管理）を実行。\n\nこの行のオペレーター制御:\n• AI: ON / OFF — メインスイッチ。OFFの場合、すべてのAIツール呼び出しが拒否されます。ONの場合、AIは以下のルールに従って動作します。\n• クリティカルAIアクション: ブロック / 許可 — 破壊的なアクション（スナップショットの削除、DSPのリセット、チャンネル数の変更）はデフォルトでブロックされます。クリックして10分間許可。赤い塗りつぶしはウィンドウの期限切れとともに減り、その後自動的に再度ブロックされます。\n• AI履歴を開く — 最近のすべてのAI変更のスクロール可能なタイムライン。\n• MCP URLボタンは、URLを直接受け取るAIクライアント用にサーバーURLをクリップボードにコピーします。\n\nオペレーターの注意:\n• 各AIアクションは起源タグ付きで記録されます。AI履歴ウィンドウは完全なタイムラインを表示。行ごとの×は、依存関係を含めてアクションを取り消します。\n• AIが先ほど移動したパラメーターを手動で調整すると、AIは通知され、盲目的に再試行しません。常にあなたが最後の判断を下します。\n• Cmd/Ctrl+Alt+ZおよびCmd/Ctrl+Alt+Yショートカットは、手動編集（通常のCtrl+Z）に影響を与えずに最後のAI変更を取り消す/やり直します。\n\nこのサーバーをClaude Desktopに追加するには:\n  1. 設定 → 開発者 → 設定を編集を開きます。\n  2. 下のJSONスニペットをclaude_desktop_config.jsonに貼り付けます（既にmcpServersブロックがある場合はマージ）。\n  3. Claude Desktopを再起動。サーバーはツールメニューに'wfs-diy'として表示されます。\n\nClaude Codeに追加するには、実行:\n  claude mcp add wfs-diy <url> -t http\n\nURLは、Network Interfaceを切り替えたり、サーバーが別のポートにフォールバックすると変更されます。この行のURLボタンは常にライブのURLを反映します。
  - [ ] OK    Fix: 

- **`copyButton`**
  - EN: Copy Config
  - JA: 設定をコピー
  - [ ] OK    Fix: 

- **`copyConfirm`**
  - EN: MCP config JSON copied to clipboard
  - JA: MCP設定JSONをクリップボードにコピーしました
  - [ ] OK    Fix: 

- **`title`**
  - EN: AI / MCP Server
  - JA: AI / MCP Server
  - [ ] OK    Fix: 

## `help.network`

- **`body`**
  - EN: The system can communicate through several network protocols, (UDP or TCP) using OSC. OSC query can be enabled to allow the clients to discover the possible OSC paths and subscribe to some parameter updates.\nThe IP of the local machine corresponding to the selected network interface is shown. The incoming TCP and UDP ports as well as the OSC Query port.\nThere are a few specialised OSC clients such as:\n- Remote for the Android application for multitouch operation and for remote control.\n- QLab that can send data and that can also be programmed directly from the application.\n- ADM-OSC for control from consoles and DAW (see specific help).\nThe data can be filtered to only allow the data from the recorded IP addresses or to allow any client sending on the correct ports.\nThere is a Log window to see what data comes in or out, filter by the type of protocol, client and so on.\nThere is also a locator function to find a lost remote Android tablet. It will flash and sound the alarm on the missing device.
  - JA: システムはOSCを使用して複数のネットワークプロトコル（UDPまたはTCP）で通信できます。OSC Queryを有効にすると、クライアントが利用可能なOSCパスを発見しパラメータ更新を購読できます。\n選択したネットワークインターフェースに対応するローカルマシンのIPが表示されます。受信TCP・UDPポートおよびOSC Queryポート。\n専用OSCクライアント：\n- Remote：マルチタッチ操作とリモートコントロール用のAndroidアプリケーション。\n- QLab：データ送信とアプリケーションからの直接プログラミング。\n- ADM-OSC：コンソールやDAWからの制御（専用ヘルプ参照）。\nデータフィルタリング、ログウィンドウ、紛失したAndroidタブレットの位置特定機能があります。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Network
  - JA: ネットワーク
  - [ ] OK    Fix: 

## `help.outputAdvanced`

- **`body`**
  - EN: There are a few parameters to help you adjust to the acoustic sound.\nMost of these parameters are set for whole arrays unless the propagation mode is switched to off for this output in the array. Relative change can also be selected after a specific setting.\n- Orientation and On/Off Angles define what inputs each speaker will amplify. By default the speakers are pointing to the audience, away from the stage. Inputs in the green sector will be amplified, but not the ones in front of the speaker, in the red sector. There is a fade between both sectors. For sub-bass speakers which usually come in limited numbers and locations, opening all the way to the maximum will allow you to have all inputs possibly picked up by the subwoofers.\n- HF Damping simulates the loss of high frequency with distance. Speakers close to the listeners can have more than speakers away from the stage and the listeners.\n- Distance Attenuation percentage allows you to define if more or less of the attenuation calculated from the distance and parameters of the input is applied. Again for Sub-bass in case you only have two and don't want to lose too much level or have hot-spots it may be wise to lower this setting to 50%.\n- Minimal Latency toggles whether the output is scanned for the smallest calculated delay and also if the setting once engaged on an input will reduce the delay for that input through this speaker.\n- Live Source Attenuation toggles the reduction of level of nearby input if they have this setting enabled. This may not be necessary for speakers away from the audience or for sub-bass.\n- Floor Reflections toggles if the reflections are applied to the signal for this output such as sub-bass and flown arrays...
  - JA: 音響サウンドに合わせて調整するためのいくつかのパラメータがあります。\nこれらのパラメータのほとんどは、アレイ全体に設定されます（伝搬モードがこの出力でオフの場合を除く）。特定の設定後に相対変更も選択できます。\n- 方向とOn/Off角度は、各スピーカーがどのインプットを増幅するかを定義します。デフォルトではスピーカーは観客に向かい、ステージから離れています。緑のセクターのインプットは増幅されますが、スピーカーの前方の赤いセクターのインプットは増幅されません。サブバスの場合、最大まで開くことですべてのインプットをカバーできます。\n- HFダンピングは距離による高周波の損失をシミュレートします。\n- Distance attenuationのパーセンテージは、計算された減衰のどれだけが適用されるかを定義します。サブバスの場合、50%に下げるのが賢明かもしれません。\n- Minimal Latencyは最小計算遅延のスキャンを有効にします。\n- Live Source減衰は近くのインプットのレベル低減を有効にします。\n- Floor Reflectionsは、サブバスやフライングアレイなど、この出力に反射が適用されるかどうかを切り替えます...
  - [ ] OK    Fix: 

- **`title`**
  - EN: Advanced Parameters
  - JA: アドバンストパラメータ
  - [ ] OK    Fix: 

## `help.outputs`

- **`body`**
  - EN: The design of the WFS system has to do with the right choice of equipment and their position. Here is a guide to help you with the design and the tuning of your arrays.\nAn array is a line (straight or curved) of speakers. This is one of the most important concepts in WFS adapted to sound reinforcement and for creative sound design.\nA rule of thumb would be that each listener should hear three speakers of an array to have enough psycho-acoustic cues to feel the direction each sound is coming from. There will be a sweet spot to find between the distance between the speakers and the listeners, their spacing and coverage angle. This is especially true for short range arrays, a.k.a front fills. Speakers with a 120° coverage angle can be spaced by the same distance between the array and the first row of listeners. The number of speakers also depends on the sound pressure level. When placed as a flown array, trapezoidal/asymmetric horns with wide coverage angle (120°) below axis and narrow coverage (60°) on axis will give good coverage for seat rows a bit too far from the ground array and will have enough throw to reach 20m or 30m away while avoiding the walls of the venue where they would create reflections that give away the position of the speakers to the ears of the listeners. Most of the time for larger venues coaxial (elliptical or conical horns) have not enough reach and require one or more delay lines. They are more suited for smaller venues with few rows of seats.\nThe positioning of the speakers in the system can be done through the 'Wizard of OutZ' and the editable presets it has.
  - JA: WFSシステムの設計は、適切な機器の選択とその配置に関わります。アレイの設計とチューニングのガイドです。\nアレイはスピーカーの列（直線または曲線）です。これはWFSにおける最も重要な概念の一つです。\n経験則として、各リスナーは方向を感じるための十分な心理音響的手がかりを得るために、アレイの3つのスピーカーを聞くべきです。スピーカーとリスナーの距離、間隔、カバレージ角度の間にスイートスポットがあります。120°のカバレージ角を持つスピーカーは、アレイと最前列の間の距離と同じ間隔で配置できます。スピーカーの数は音圧レベルにも依存します。フライングアレイとして配置する場合、軸下に広い(120°)、軸上に狭い(60°)カバレージを持つ台形/非対称ホーンが良いカバレージと20-30mの到達距離を提供し、壁での反射を避けます。同軸スピーカーは大規模会場では通常十分な到達距離がなく、ディレイラインが必要です。\n配置は'Wizard of OutZ'とその編集可能なプリセットを通じて行えます。
  - [ ] OK    Fix: 

- **`title`**
  - EN: WFS Array Design
  - JA: WFSアレイ設計
  - [ ] OK    Fix: 

## `help.overview`

- **`body`**
  - EN: This WFS spatial sound processor aims to be a tool for natural sound reinforcement and also a creative tool opening new avenues for writing sound in space.\nSome parameters are straightforward: place sound (Map, Tracking, Speed Limiting, Gradient Maps...), work its shape (Attenuation Profile) and its acoustic presence (Directivity, Floor Reflections), give it a one-off motion (AutomOtion) or a repetitive movement (L.F.O). In some cases amplification should be limited around loud sources on stage (Live Source Tamer). All of these functionalities can be stored and recalled internally or with the help of QLab. On the other side the system allows real-time interaction to trigger and move samples, move large clusters of inputs manually or thanks to easily recallable LFO presets.
  - JA: このWFS空間音響プロセッサーは、自然な音響補強のためのツールであると同時に、空間に音を書く新しい可能性を開く創造的なツールです。\nいくつかのパラメータは直感的です：音を配置する（マップ、トラッキング、速度制限、グラジエントマップ...）、形を加工する（減衰プロファイル）、音響的存在感を調整する（指向性、Floor Reflections）、単発の動き（AutomOtion）や繰り返しの動き（L.F.O）を与える。場合によってはステージ上の大音量の音源周辺の増幅を制限する必要があります（Live Source Tamer）。これらの機能はすべて内部で、またはQLAbの助けを借りて保存・呼び出しが可能です。さらにシステムはリアルタイムインタラクションによるサンプルのトリガーと移動、大きなインプットクラスターの手動移動やLFOプリセットによる移動を可能にします。
  - [ ] OK    Fix: 

- **`dontShow`**
  - EN: Don't show this again
  - JA: 今後表示しない
  - [ ] OK    Fix: 

- **`title`**
  - EN: System Overview
  - JA: システム概要
  - [ ] OK    Fix: 

## `help.parallax`

- **`body`**
  - EN: Each speaker points more or less clearly at a listener. To calculate the delay for an input for each speaker, let's consider the distance from the input to this listener, we can also calculate the distance the sound from the speaker to this listener. To match the time of arrival of both we need to apply the difference in the aforementioned distances as a delay for this input and this speaker. This gives greater stability when inputs are moved on stage and especially when they move upstage, away from the edge of the stage. This also can enable the synthesis of floor reflections. This setting can be tuned for finer adjustments, rather than simply measured. Use your ears!
  - JA: 各スピーカーは多かれ少なかれリスナーに向いています。各スピーカーのインプットの遅延を計算するために、インプットからリスナーまでの距離を考慮し、スピーカーからリスナーまでの音の距離も計算できます。両方の到達時間を一致させるために、前述の距離の差を遅延として適用する必要があります。これにより、ステージ上でインプットを移動する際、特にステージの端から離れる際に、より大きな安定性が得られます。これは床面反射の合成も可能にします。この設定は単に測定するのではなく、微調整できます。耳を信じてください！
  - [ ] OK    Fix: 

- **`title`**
  - EN: Parallax Correction
  - JA: パララックス補正
  - [ ] OK    Fix: 

## `help.reverb`

- **`body`**
  - EN: Reverb helps blur the actual reflections of the speakers in the acoustic space. Without some reverb the listener might perceive the sound missing some depth because the reflections of the speakers in the acoustic of the room gives away their positions and all sounds might feel as coming from the plane of the speakers.\nPlace the reverb node according to the number of channels and the geometry of the stage and listening area, and the positions of speakers. If necessary the return position can be offset from the feed position.\nOther parameters are very similar to Outputs' and Inputs'.
  - JA: リバーブはスピーカーの実際の反射を曖昧にするのに役立ちます。\nチャンネル数とステージの形状に応じてリバーブノードを配置します。\nその他のパラメータは出力と入力に似ています。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb
  - JA: リバーブ
  - [ ] OK    Fix: 

## `help.reverbAlgo`

- **`body`**
  - EN: There are three built-in reverb types inside this spatial sound processor:\n- SDN (Scattered Delay Network): The sound bounces between every reverb node which act as reflecting surfaces. This reverb algorithm favours an odd number of nodes that should not have too much symmetry. This helps reduce the artifacts of the reverb or any metallic sounding resonance.\n- FDN (Feedback Delay Network): Each reverb node functions as a separate reverb processor with a classical reverb algorithm. Place nodes around the stage and eventually around the audience. Inputs are sent to each node in a similar fashion as with outputs. The return signal from the reverb is then distributed like other inputs to all outputs.\n- IR (Impulse Response): This is a classical convolution reverb. You can load audio samples as impulse responses. Each reverb node can share the same IR or they can have different ones. Place nodes around the stage and eventually around the audience.\nThe node positions can be adjusted directly on the map. The Ctrl/Cmd key will move a reverb node pair in symmetry.
  - JA: この空間音響プロセッサには3つのリバーブタイプが内蔵されています：\n- SDN（散乱遅延ネットワーク）：音が反射面として機能する各リバーブノード間で反射します。このアルゴリズムは対称性の少ない奇数のノードが適しており、金属的な共鳴やアーティファクトを軽減します。\n- FDN（フィードバック遅延ネットワーク）：各ノードが古典的なアルゴリズムを持つ独立したリバーブプロセッサとして機能します。ステージ周辺や観客周辺にノードを配置します。\n- IR（インパルス応答）：古典的な畳み込みリバーブです。オーディオサンプルをインパルス応答として読み込めます。各ノードは同じIRを共有するか、異なるIRを使用できます。\nノードの位置はマップ上で直接調整できます。Ctrl/Cmdキーでリバーブノードのペアを対称的に移動します。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Algorithms
  - JA: リバーブアルゴリズム
  - [ ] OK    Fix: 

## `help.reverbFeed`

- **`body`**
  - EN: Pre-processing sending of the input channels to the reverb nodes.\n- Orientation and On/Off Angles define what inputs each reverb node will receive. Inputs in the green sector will be picked-up, but not the ones in front in the red sector. There is a fade between both sectors.\n- HF Damping simulates the loss of high frequency with distance.\n- Distance Attenuation percentage allows you to define if more or less of the attenuation calculated from the distance and parameters of the input is applied.\n- Minimal Latency toggles whether the output is scanned for the smallest calculated delay and also if the setting once engaged on an input will reduce the delay for that input through this reverb node.\n- Live Source Attenuation toggles the reduction of level of nearby input if they have this setting enabled.
  - JA: Input Channelsからリバーブノードへの前処理送信。\n- 方向とOn/Off角度が各ノードが受信する入力を定義します。\n- HF減衰は距離による高周波損失をシミュレートします。\n- Distance attenuation率は適用される減衰を定義します。\n- 最小レイテンシは最小計算遅延の使用を切り替えます。\n- Live Source減衰は近くの入力のレベル低減を切り替えます。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Feed
  - JA: Reverb Feed
  - [ ] OK    Fix: 

## `help.reverbPostProc`

- **`body`**
  - EN: Includes a 4-band EQ and an Expander that looks at the signal entering the reverb processor to reduce long reverb tails when the inputs are quiet.
  - JA: 4バンドEQとエキスパンダーを含み、入力が静かな時にリバーブプロセッサに入る信号を監視して長いリバーブテールを軽減します。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Post-Processing
  - JA: リバーブ後処理
  - [ ] OK    Fix: 

## `help.reverbPreProc`

- **`body`**
  - EN: Includes a 4-band EQ and a Compressor to remove transients which might excite the reverb processor a bit too much.
  - JA: 4バンドEQとコンプレッサーを含み、リバーブプロセッサを過度に励起する可能性のあるトランジェントを除去します。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Pre-Processing
  - JA: リバーブ前処理
  - [ ] OK    Fix: 

## `help.reverbReturn`

- **`body`**
  - EN: Post-processing sending to the speakers.\n- Distance Attenuation defines the level drop per meter to the speakers.\n- Common Attenuation will look for the lowest attenuation and keep only a percentage of this level drop and apply this across all other outputs.\n- Mutes and Mute Macros allow to prevent a reverb channel from feeding an output. For instance it may not be necessary to send reverb to sub-bass speakers.
  - JA: スピーカーへの後処理送信。\n- Distance attenuationはメートルあたりのレベル低下を定義します。\n- 共通減衰は最低減衰のパーセンテージを維持します。\n- ミュートはReverb Channelsが出力に送られるのを防ぎます。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Reverb Return
  - JA: Reverb Return
  - [ ] OK    Fix: 

## `help.sampler`

- **`body`**
  - EN: The sampler allows to trigger samples and interact with them in real time.\nThe sampler when enabled on a track will replace the live input at all times.\nSeveral samplers can be assigned to different inputs and triggered individually.\nTo use the sampler:\n- Select a Roli Lightpad or a pad on the connected Android Remote app.\n- Add samples to the different tiles in the grid to the left. Adjust their relative starting position and their level and eventually their in and out points. Several samples can be selected using the shift key while clicking.\n- Create sets of samples: selected samples will be added to new sets. Samples can be added or removed after the creation of a set by holding Ctrl/Cmd while clicking on the tiles. Each set can be renamed. Each set can either have a fixed sequence or a random order (round robin, each sample is played once before a new random order is drawn). Each set has an attenuation setting. Each set has a base position applied to the input when selecting the set. It can be moved on the map or using external control. The sample position offset is added to the set position each time a sample is triggered.\n- Press a Roli Lightpad or a pad on the Android app to trigger a sample. The pressure applied to the pad can be mapped to any of the following controls: level, height and high frequency filtering. The sensitivity can be adjusted for each. The movement of the finger on the pad will cause the sound to move. This acts by measuring the deflection from the initial contact point like a joystick. This can be disabled. All sets have their respective settings for the interaction.\nReleasing the pad will stop the triggered sample.\nSampler settings are stored in the input files.\nFor convenience sample tiles and sets can be copied, exported and imported.
  - JA: サンプラーはサンプルをトリガーしてリアルタイムで操作できます。\nトラックで有効にするとサンプラーは常にライブ入力を置き換えます。\n複数のサンプラーを異なる入力に割り当てて個別にトリガーできます。\nサンプラーの使い方：\n- Roli Lightpadまたは接続されたAndroid Remoteアプリのパッドを選択します。\n- グリッドの各タイルにサンプルを追加します。相対的な開始位置、レベル、イン/アウトポイントを調整します。Shiftキーを押しながらクリックで複数選択できます。\n- サンプルセットを作成：選択したサンプルが新しいセットに追加されます。Ctrl/Cmdを押しながらクリックでセット作成後に追加・削除できます。各セットは名前変更可能で、固定順序またはランダム順序を設定できます。各セットには減衰設定と基本位置があります。\n- Lightpadまたはパッドを押してサンプルをトリガーします。パッドの圧力はレベル、高さ、高周波フィルタリングにマッピングできます。指の動きはジョイスティックのように音を移動させます。\nパッドを離すとサンプルが停止します。\nサンプラー設定は入力ファイルに保存されます。\nタイルとセットはコピー、エクスポート、インポートできます。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Sampler
  - JA: サンプラー
  - [ ] OK    Fix: 

## `help.sessionData`

- **`body`**
  - EN: When starting a session select the working folder where the system will place the files and eventual audio files. For new projects create a new folder. When reloading a previous session navigate to the corresponding folder.\nEach section has a separate xml file (System configuration, Network, Outputs, Reverbs, Inputs) and backups. Additionally convolution reverb impulse responses and audio samples will be stored in sub-directories.\nEach section can be stored and recalled individually or as a whole.\nEach section can also export and import files from other projects.
  - JA: セッション開始時に、システムがファイルとオーディオファイルを配置する作業フォルダを選択します。新規プロジェクトの場合は新しいフォルダを作成します。以前のセッションを再読み込みする場合は対応するフォルダに移動します。\n各セクションには個別のxmlファイル（システム設定、ネットワーク、出力、リバーブ、入力）とバックアップがあります。畳み込みリバーブのインパルス応答とオーディオサンプルはサブディレクトリに保存されます。\n各セクションは個別または一括で保存・呼び出しできます。\n各セクションは他のプロジェクトからファイルをエクスポート・インポートすることもできます。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Session Data
  - JA: セッションデータ
  - [ ] OK    Fix: 

## `help.snapshotScope`

- **`body`**
  - EN: Snapshots store input parameters, but can have a scope to be recalled during a performance. They can have between all parameters for all inputs and only one parameter for a single channel. They can be updated and renamed for convenience.\nThe Scope tells the system what data to store or recall. It's the opposite of 'safe' parameters.\nThere are several ways to do this in this application:\n- Record only the needed data in local files. The scope filter is applied when storing the data. A recall cue can be created in QLab to trigger the reading of the local file.\n- Record all data and a filter in local files. The scope filter is applied when recalling the data. This allows to eventually recall all data not taking into account the scope filter. This may come in handy when a complete configuration should be recalled during rehearsal for example. A recall cue can be created in QLab to trigger the reading of the local file.\n- Record all data in scope in QLab cues. This should not be used to recall all parameters for large configurations since QLab may stall when recalling so much data.\nThe scope can show and automatically pre-select the parameters that have been manually changed (local UI, hardware controllers, remote Android application). Changed parameters are marked with a yellow mark.
  - JA: スナップショットは入力パラメータを保存しますが、パフォーマンス中に呼び出すスコープを持つことができます。\nスコープはシステムにどのデータを保存または呼び出すかを指示します。\n複数の方法があります：\n- 必要なデータのみをローカルファイルに記録。フィルターは保存時に適用されます。\n- すべてのデータとフィルターをローカルファイルに記録。フィルターは呼び出し時に適用されます。\n- すべてのデータをQLab cueに記録。大規模な設定には推奨されません。\nスコープは手動で変更されたパラメータを表示し自動的に事前選択できます。変更されたパラメータは黄色でマークされます。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Input Snapshots and Scope
  - JA: 入力スナップショットとスコープ
  - [ ] OK    Fix: 

## `help.tracking`

- **`body`**
  - EN: Tracking allows to follow the position in 2D or 3D of actors and musicians. There are several tracking solutions based on UWB (UltraWide Band) tags, 3D camera, computer vision systems and Infra-Red LED's followed by IR sensitive cameras.\nThis application allows you to receive tracking data from several protocols: OSC, MQTT, PosiStageNet/PSN, RTTrP.\nYou can select which protocol you're using and enter its settings. You may also adjust the mapping (offset, scaling and orientation).\nEach input has a toggle to enable tracking, an ID to select which tracking marker it should follow and a smoothing algorithm to reduce jittery movements.
  - JA: トラッキングは俳優やミュージシャンの2Dまたは3D位置を追跡できます。UWBタグ、3Dカメラ、コンピュータビジョンシステム、赤外線LEDとIR感度カメラなど、さまざまなトラッキングソリューションがあります。\nこのアプリケーションは複数のプロトコルからトラッキングデータを受信できます：OSC、MQTT、PosiStageNet/PSN、RTTrP。\n使用するプロトコルを選択して設定を入力できます。マッピング（オフセット、スケーリング、方向）も調整できます。\n各入力にはトラッキングを有効にするトグル、追跡するマーカーを選択するID、ジッタを軽減するスムージングアルゴリズムがあります。
  - [ ] OK    Fix: 

- **`title`**
  - EN: Tracking
  - JA: トラッキング
  - [ ] OK    Fix: 

## `help.tuning`

- **`body`**
  - EN: System tuning for WFS is different from standard PA tuning. It can proceed as follows:\n- Start with the flown array muted. Set the desired levels for the near-field speakers when listening to them in the first row. Adjust the high frequency shelf attenuation so the near-field speakers are not too bright and draw too much attention.\n- Mute the near-field array and unmute the flown array, find a suitable level towards the back of the house.\n- Unmute both arrays, adjust the delay of the flown array to bring the sound down to the correct height in the lower rows. Adjust the levels, HF shelf/distance ratio and vertical and horizontal parallax for each array to have consistent level wherever your inputs are on stage. This is especially important if your inputs are actors, singers or acoustic music instruments on stage. You may also test adding some Haas effect to delay the whole system if you feel the WFS is too early or adding comb filtering with the acoustic sound.\nYou may follow a different workflow for tuning or go for different cues.
  - JA: WFSのシステムチューニングは標準的なPAチューニングとは異なります。以下のように進めることができます：\n- フライングアレイをミュートして開始します。ニアフィールドスピーカーを最前列で聴きながら、希望のレベルを設定します。ニアフィールドスピーカーが明るすぎないようにHFシェルフの減衰を調整します。\n- ニアフィールドアレイをミュートしてフライングアレイをアンミュートし、客席後方に適切なレベルを見つけます。\n- 両方のアレイをアンミュートし、フライングアレイの遅延を調整して下の列で正しい高さに音を下ろします。各アレイのレベル、HFシェルフ/距離比、垂直・水平パララックスを調整して、ステージ上のどこにインプットがあっても一貫したレベルを実現します。\n異なるワークフローで調整したり、状況に応じて異なる設定を目指すこともできます。
  - [ ] OK    Fix: 

- **`title`**
  - EN: System Tuning
  - JA: システムチューニング
  - [ ] OK    Fix: 

## `inputs.dialogs`

- **`exportConfig`**
  - EN: Export Input Configuration
  - JA: 入力設定をエクスポート
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Input Configuration
  - JA: 入力設定をインポート
  - [ ] OK    Fix: 

- **`selectChannel`**
  - EN: Select Channel
  - JA: チャンネルを選択
  - [ ] OK    Fix: 

- **`snapshotNameLabel`**
  - EN: Name:
  - JA: 名前：
  - [ ] OK    Fix: 

- **`storeSnapshotMessage`**
  - EN: Enter a name for the new snapshot:
  - JA: 新しいスナップショットの名前を入力してください：
  - [ ] OK    Fix: 

- **`storeSnapshotTitle`**
  - EN: Store New Snapshot
  - JA: 新しいスナップショットを保存
  - [ ] OK    Fix: 

- **`trackingConflictCluster`**
  - EN: Input {current} has tracking enabled, but Input {existing} in Cluster {cluster} is already tracked.\n\nOnly one tracked input per cluster is allowed.
  - JA: 入力 {current} はトラッキングが有効ですが、クラスター {cluster} の入力 {existing} は既にトラッキングされています。\n\nクラスターごとに1つのトラッキング入力のみが許可されています。
  - [ ] OK    Fix: 

- **`trackingConflictContinue`**
  - EN: Continue (disable tracking)
  - JA: 続行 (トラッキングを無効化)
  - [ ] OK    Fix: 

- **`trackingConflictSwitch`**
  - EN: Input {existing} in Cluster {cluster} already has tracking enabled.\n\nOnly one tracked input per cluster is allowed.\n\nDo you want to disable tracking on Input {existing} and enable it on Input {to}?
  - JA: クラスター {cluster} の入力 {existing} は既にトラッキングが有効です。\n\nクラスターごとに1つのトラッキング入力のみが許可されています。\n\n入力 {existing} のトラッキングを無効化して入力 {to} で有効化しますか？
  - [ ] OK    Fix: 

- **`trackingConflictTitle`**
  - EN: Tracking Conflict
  - JA: トラッキングの競合
  - [ ] OK    Fix: 

- **`trackingConflictYes`**
  - EN: Yes, switch tracking
  - JA: はい、トラッキングを切り替え
  - [ ] OK    Fix: 

## `inputs.gradientMap`

- **`help.blackValue`**
  - EN: Parameter value mapped to black (0.00–1.00)
  - JA: 黒にマッピングされるパラメータ値（0.00–1.00）
  - [ ] OK    Fix: 

- **`help.blur`**
  - EN: Edge blur in meters
  - JA: エッジぼかし（メートル単位）
  - [ ] OK    Fix: 

- **`help.copy`**
  - EN: Copy selected shape or layer
  - JA: 選択したシェイプまたはレイヤーをコピー
  - [ ] OK    Fix: 

- **`help.curve`**
  - EN: Gamma curve (-1 to 1, 0 = linear)
  - JA: ガンマカーブ（-1～1、0 = リニア）
  - [ ] OK    Fix: 

- **`help.ellipseTool`**
  - EN: Draw ellipse
  - JA: 楕円を描画
  - [ ] OK    Fix: 

- **`help.fillTool`**
  - EN: Apply uniform fill to shape
  - JA: シェイプに均一塗りつぶしを適用
  - [ ] OK    Fix: 

- **`help.fillValue`**
  - EN: Fill brightness (0 = black, 1 = white)
  - JA: 塗りつぶし明度（0 = 黒、1 = 白）
  - [ ] OK    Fix: 

- **`help.layerEnable`**
  - EN: Enable/disable layer (affects output and OSC)
  - JA: レイヤーの有効/無効（出力とOSCに影響）
  - [ ] OK    Fix: 

- **`help.layerSelect`**
  - EN: Select this layer for editing
  - JA: このレイヤーを編集用に選択
  - [ ] OK    Fix: 

- **`help.layerVisible`**
  - EN: Show/hide layer preview on canvas
  - JA: キャンバス上のレイヤープレビューを表示/非表示
  - [ ] OK    Fix: 

- **`help.linGradTool`**
  - EN: Apply linear gradient to shape
  - JA: シェイプに線形グラデーションを適用
  - [ ] OK    Fix: 

- **`help.paste`**
  - EN: Paste shape or layer from clipboard
  - JA: クリップボードからシェイプまたはレイヤーを貼り付け
  - [ ] OK    Fix: 

- **`help.polygonTool`**
  - EN: Draw polygon (double-click to close)
  - JA: ポリゴンを描画（ダブルクリックで閉じる）
  - [ ] OK    Fix: 

- **`help.radGradTool`**
  - EN: Apply radial gradient to shape
  - JA: シェイプに放射グラデーションを適用
  - [ ] OK    Fix: 

- **`help.rectTool`**
  - EN: Draw rectangle
  - JA: 矩形を描画
  - [ ] OK    Fix: 

- **`help.selectTool`**
  - EN: Select and move shapes
  - JA: シェイプの選択と移動
  - [ ] OK    Fix: 

- **`help.shapeDelete`**
  - EN: Delete selected shape(s)
  - JA: 選択したシェイプを削除
  - [ ] OK    Fix: 

- **`help.shapeEnable`**
  - EN: Enable/disable shape
  - JA: シェイプの有効/無効
  - [ ] OK    Fix: 

- **`help.shapeLock`**
  - EN: Lock shape position
  - JA: シェイプの位置をロック
  - [ ] OK    Fix: 

- **`help.whiteValue`**
  - EN: Parameter value mapped to white (0.00–1.00)
  - JA: 白にマッピングされるパラメータ値（0.00–1.00）
  - [ ] OK    Fix: 

## `inputs.help`

- **`admMapping`**
  - EN: Assign this input to an ADM-OSC mapping for position receive/transmit.
  - JA: この入力をADM-OSCマッピングに割り当て、位置の送受信を行います。
  - [ ] OK    Fix: 

- **`arrayAttenDial`**
  - EN: Attenuation for Array {num} (-60 to 0 dB).
  - JA: アレイ{num}の減衰（-60〜0 dB）。
  - [ ] OK    Fix: 

- **`attenuationLawButton`**
  - EN: Attenuation Law Model (Linear Decrease of Volume with Distance Between Object and Speaker or Squared).
  - JA: Attenuation Lawモデル（オブジェクトとスピーカー間の距離による線形音量減少、または二次）。
  - [ ] OK    Fix: 

- **`attenuationSlider`**
  - EN: Input Channel Attenuation.
  - JA: 入力チャンネルの減衰。
  - [ ] OK    Fix: 

- **`channelSelector`**
  - EN: Input Channel Number and Selection.
  - JA: 入力チャンネル番号と選択。
  - [ ] OK    Fix: 

- **`clusterSelector`**
  - EN: Object is Part of a Cluster.
  - JA: オブジェクトはクラスターの一部です。
  - [ ] OK    Fix: 

- **`commonAttenDial`**
  - EN: Percentage of the Common Part of the Attenuation for selected Object Relative to All Outputs.
  - JA: 選択したオブジェクトのすべての出力に対する減衰の共通部分のパーセンテージ。
  - [ ] OK    Fix: 

- **`constraintDistanceButton`**
  - EN: Limit Position to Distance Range from Origin (for Cylindrical/Spherical modes).
  - JA: 位置を原点からの距離範囲に制限（円筒/球面モード用）。
  - [ ] OK    Fix: 

- **`constraintXButton`**
  - EN: Limit Position to the Bounds of the Stage in Width.
  - JA: 位置をステージの幅に制限。
  - [ ] OK    Fix: 

- **`constraintYButton`**
  - EN: Limit Position to the Bounds of the Stage in Depth.
  - JA: 位置をステージの奥行きに制限。
  - [ ] OK    Fix: 

- **`constraintZButton`**
  - EN: Limit Position to the Bounds of the Stage in Height.
  - JA: 位置をステージの高さに制限。
  - [ ] OK    Fix: 

- **`coordModeSelector`**
  - EN: Coordinate display mode: Cartesian (X/Y/Z), Cylindrical (radius/azimuth/height), or Spherical (radius/azimuth/elevation).
  - JA: 座標表示モード: 直交 (X/Y/Z)、円筒 (半径/方位角/高さ)、または球面 (半径/方位角/仰角)。
  - [ ] OK    Fix: 

- **`delayLatencySlider`**
  - EN: Input Channel Delay (positive values) or Latency Compensation (negative values).
  - JA: 入力チャンネルのディレイ（正の値）またはレイテンシー補正（負の値）。
  - [ ] OK    Fix: 

- **`deleteSnapshot`**
  - EN: Delete Selected Input Snapshot With Confirmation.
  - JA: 確認付きで選択した入力スナップショットを削除します。
  - [ ] OK    Fix: 

- **`directivitySlider`**
  - EN: How Wide is the Brightness of The Object.
  - JA: オブジェクトの輝度コーンの幅。
  - [ ] OK    Fix: 

- **`distanceAttenDial`**
  - EN: Attenuation per Meter Between Object and Speaker.
  - JA: オブジェクトとスピーカー間の1メートルあたりの減衰。
  - [ ] OK    Fix: 

- **`distanceMaxEditor`**
  - EN: Maximum Distance from Origin in Meters.
  - JA: 原点からの最大距離（メートル）。
  - [ ] OK    Fix: 

- **`distanceMinEditor`**
  - EN: Minimum Distance from Origin in Meters.
  - JA: 原点からの最小距離（メートル）。
  - [ ] OK    Fix: 

- **`distanceRangeSlider`**
  - EN: Set Minimum and Maximum Distance from Origin.
  - JA: 原点からの最小および最大距離を設定。
  - [ ] OK    Fix: 

- **`distanceRatioDial`**
  - EN: Attenuation Ratio for Squared Model.
  - JA: 二次モデルの減衰比率。
  - [ ] OK    Fix: 

- **`editScope`**
  - EN: Open Selected Input Snapshot Scope Window.
  - JA: 選択した入力スナップショットのフィルターウィンドウを開きます。
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Input Configuration to file (with file explorer window).
  - JA: 入力設定をファイルにエクスポートします（ファイルエクスプローラー使用）。
  - [ ] OK    Fix: 

- **`flipXButton`**
  - EN: X will be Symetrical to the Origin. Keyboard Nudging will be Inverted.
  - JA: Xが原点に対して対称になります。キーボードの微調整は反転します。
  - [ ] OK    Fix: 

- **`flipYButton`**
  - EN: Y will be Symetrical to the Origin. Keyboard Nudging will be Inverted.
  - JA: Yが原点に対して対称になります。キーボードの微調整は反転します。
  - [ ] OK    Fix: 

- **`flipZButton`**
  - EN: Z will be Symetrical to the Origin. Keyboard Nudging will be Inverted.
  - JA: Zが原点に対して対称になります。キーボードの微調整は反転します。
  - [ ] OK    Fix: 

- **`frActiveButton`**
  - EN: Enable Simulated Floor Reflections for the Object.
  - JA: オブジェクトのシミュレートされたFloor Reflectionsを有効化。
  - [ ] OK    Fix: 

- **`frAttenuationSlider`**
  - EN: Attenuation of the Simulated Floor Reflections for the Object.
  - JA: オブジェクトのシミュレートされたFloor Reflectionsの減衰。
  - [ ] OK    Fix: 

- **`frDiffusionDial`**
  - EN: Diffusion Effect of the Simulated Floor Reflections for the Object.
  - JA: オブジェクトのシミュレートされたFloor Reflectionsの拡散効果。
  - [ ] OK    Fix: 

- **`frHighShelfActiveButton`**
  - EN: Enable High Shelf Filter for Floor Reflections.
  - JA: Floor ReflectionsのHigh Shelfフィルターを有効化。
  - [ ] OK    Fix: 

- **`frHighShelfFreqSlider`**
  - EN: High Shelf Frequency for Floor Reflections.
  - JA: Floor ReflectionsのHigh Shelf周波数。
  - [ ] OK    Fix: 

- **`frHighShelfGainSlider`**
  - EN: High Shelf Gain for Floor Reflections.
  - JA: Floor ReflectionsのHigh Shelfゲイン。
  - [ ] OK    Fix: 

- **`frHighShelfSlopeSlider`**
  - EN: High Shelf Slope for Floor Reflections.
  - JA: Floor ReflectionsのHigh Shelfスロープ。
  - [ ] OK    Fix: 

- **`frLowCutActiveButton`**
  - EN: Enable Low Cut Filter for Floor Reflections.
  - JA: Floor ReflectionsのLow Cutフィルターを有効化。
  - [ ] OK    Fix: 

- **`frLowCutFreqSlider`**
  - EN: Low Cut Frequency for Floor Reflections.
  - JA: Floor ReflectionsのLow Cut周波数。
  - [ ] OK    Fix: 

- **`heightFactorDial`**
  - EN: Take Elevation of Object into Account Fully, Partially or Not.
  - JA: オブジェクトの標高を完全に、部分的に、またはまったく考慮しません。
  - [ ] OK    Fix: 

- **`hfShelfSlider`**
  - EN: How Much Brightness is lost in the Back of the Object, Out of its Brightness Cone.
  - JA: オブジェクトの背面、輝度コーン外でどれだけ輝度が失われるか。
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Input Configuration from file (with file explorer window).
  - JA: ファイルから入力設定をインポートします（ファイルエクスプローラー使用）。
  - [ ] OK    Fix: 

- **`inputDirectivityDial`**
  - EN: Where is the Object pointing to in the Horizontal Plane.
  - JA: 水平面でのオブジェクトの方向。
  - [ ] OK    Fix: 

- **`jitterSlider`**
  - EN: Sphere of Rapid Movements of the Object.
  - JA: オブジェクトの急速な動きの球。
  - [ ] OK    Fix: 

- **`lfoActiveButton`**
  - EN: Enable or Disable the Periodic Movement of the Object (LFO).
  - JA: オブジェクトの周期的な動き（LFO）を有効または無効にします。
  - [ ] OK    Fix: 

- **`lfoAmplitudeXSlider`**
  - EN: Width of Movement in Relation to Base Position of the Object.
  - JA: オブジェクトのベース位置に対する幅方向の動きの大きさ。
  - [ ] OK    Fix: 

- **`lfoAmplitudeYSlider`**
  - EN: Depth of Movement in Relation to Base Position of the Object.
  - JA: オブジェクトのベース位置に対する奥行き方向の動きの大きさ。
  - [ ] OK    Fix: 

- **`lfoAmplitudeZSlider`**
  - EN: Height of Movement in Relation to Base Position of the Object.
  - JA: オブジェクトのベース位置に対する高さ方向の動きの大きさ。
  - [ ] OK    Fix: 

- **`lfoGyrophoneSelector`**
  - EN: Rotation of the Brightness Cone of the Object.
  - JA: オブジェクトの輝度コーンの回転。
  - [ ] OK    Fix: 

- **`lfoPeriodDial`**
  - EN: Base Period of the Movement of the Object.
  - JA: オブジェクトの動きのベース周期。
  - [ ] OK    Fix: 

- **`lfoPhaseDial`**
  - EN: Phase Offset of the Movement of the Object.
  - JA: オブジェクトの動きの位相オフセット。
  - [ ] OK    Fix: 

- **`lfoPhaseXDial`**
  - EN: Phase Offset of the Movement of the Object in Width.
  - JA: 幅方向のオブジェクトの動きの位相オフセット。
  - [ ] OK    Fix: 

- **`lfoPhaseYDial`**
  - EN: Phase Offset of the Movement of the Object in Depth.
  - JA: 奥行き方向のオブジェクトの動きの位相オフセット。
  - [ ] OK    Fix: 

- **`lfoPhaseZDial`**
  - EN: Phase Offset of the Movement of the Object in Height.
  - JA: 高さ方向のオブジェクトの動きの位相オフセット。
  - [ ] OK    Fix: 

- **`lfoRateXSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Width.
  - JA: 幅方向のベース周期に対する速い/遅い動き。
  - [ ] OK    Fix: 

- **`lfoRateYSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Depth.
  - JA: 奥行き方向のベース周期に対する速い/遅い動き。
  - [ ] OK    Fix: 

- **`lfoRateZSlider`**
  - EN: Faster or Slower Movement in Relation to Base Period in Height.
  - JA: 高さ方向のベース周期に対する速い/遅い動き。
  - [ ] OK    Fix: 

- **`lfoShapeXSelector`**
  - EN: Movement Behaviour of the Object in Width.
  - JA: 幅方向のオブジェクトの動きの挙動。
  - [ ] OK    Fix: 

- **`lfoShapeYSelector`**
  - EN: Movement Behaviour of the Object in Depth.
  - JA: 奥行き方向のオブジェクトの動きの挙動。
  - [ ] OK    Fix: 

- **`lfoShapeZSelector`**
  - EN: Movement Behaviour of the Object in Height.
  - JA: 高さ方向のオブジェクトの動きの挙動。
  - [ ] OK    Fix: 

- **`lsActiveButton`**
  - EN: If You Need to Reduce the Level in Speakers Close to the Object. (eg. Loud Source Present on Stage)
  - JA: オブジェクトに近いスピーカーのレベルを下げる必要がある場合（例：ステージ上の大きな音源）。
  - [ ] OK    Fix: 

- **`lsAttenuationSlider`**
  - EN: Constant Attenuation of Speakers Around the Object.
  - JA: オブジェクトの周りのスピーカーの一定減衰。
  - [ ] OK    Fix: 

- **`lsPeakEnableButton`**
  - EN: Enable or Disable the Fast (Peak) Compressor for the Live Source Tamer.
  - JA: Live Source Tamerの高速（ピーク）コンプレッサーを有効または無効にします。
  - [ ] OK    Fix: 

- **`lsPeakRatioDial`**
  - EN: Ratio to Apply the Fast Compression for Speakers Around the Object.
  - JA: オブジェクトの周りのスピーカーに高速圧縮を適用する比率。
  - [ ] OK    Fix: 

- **`lsPeakThresholdSlider`**
  - EN: Fast Compression Threshold for Speakers Around the Object to Control Transients.
  - JA: オブジェクトの周りのスピーカーのトランジエント制御のための高速圧縮スレッショルド。
  - [ ] OK    Fix: 

- **`lsRadiusSlider`**
  - EN: How Far does the Attenuation Affect The Speakers.
  - JA: 減衰がスピーカーにどこまで影響するか。
  - [ ] OK    Fix: 

- **`lsShapeSelector`**
  - EN: Profile of the Attenuation Around the Object.
  - JA: オブジェクトの周りの減衰のプロファイル。
  - [ ] OK    Fix: 

- **`lsSlowEnableButton`**
  - EN: Enable or Disable the Slow Compressor for the Live Source Tamer.
  - JA: Live Source Tamerの低速コンプレッサーを有効または無効にします。
  - [ ] OK    Fix: 

- **`lsSlowRatioDial`**
  - EN: Ratio to Apply the Slow Compression for Speakers Around the Object.
  - JA: オブジェクトの周りのスピーカーに低速圧縮を適用する比率。
  - [ ] OK    Fix: 

- **`lsSlowThresholdSlider`**
  - EN: Slow Compression Threshold for Speakers Around the Object to Control Sustained Level.
  - JA: オブジェクトの周りのスピーカーの持続レベル制御のための低速圧縮スレッショルド。
  - [ ] OK    Fix: 

- **`mapLockButton`**
  - EN: Prevent Interaction on the Map Tab
  - JA: マップタブでの操作を防止します。
  - [ ] OK    Fix: 

- **`mapVisibilityButton`**
  - EN: Make Visible or Hide The Selected Input on the Map
  - JA: マップ上で選択した入力を表示または非表示にします。
  - [ ] OK    Fix: 

- **`maxSpeedActiveButton`**
  - EN: Enable or Disable Speed Limiting for Object.
  - JA: オブジェクトの速度制限を有効または無効にします。
  - [ ] OK    Fix: 

- **`maxSpeedDial`**
  - EN: Maximum Speed Limit for Object.
  - JA: オブジェクトの最大速度制限。
  - [ ] OK    Fix: 

- **`minimalLatencyButton`**
  - EN: Select between Acoustic Precedence and Minimal Latency for Amplification Precedence.
  - JA: 増幅プレシデンスとしてAcoustic PrecedenceとMinimal Latencyのいずれかを選択。
  - [ ] OK    Fix: 

- **`muteButton`**
  - EN: Mute Output {num} for this Object.
  - JA: このオブジェクトの出力{num}をミュート。
  - [ ] OK    Fix: 

- **`muteMacrosSelector`**
  - EN: Mute Macros for Fast Muting and Unmuting of Arrays.
  - JA: アレイの素早いミュート/解除のためのMute Macros。
  - [ ] OK    Fix: 

- **`muteReverbSendsButton`**
  - EN: Mute sends from this input to all reverb channels.
  - JA: この入力からすべてのリバーブチャンネルへの送信をミュート。
  - [ ] OK    Fix: 

- **`nameEditor`**
  - EN: Displayed Input Channel Name (editable).
  - JA: 表示される入力チャンネル名（編集可能）。
  - [ ] OK    Fix: 

- **`nudgeHint1`**
  - EN:  Nudge with Left and Right Arrow Keys.
  - JA:  左右キーで調整。
  - [ ] OK    Fix: 

- **`nudgeHint2`**
  - EN:  Nudge with Up and Down Arrow Keys.
  - JA:  上下キーで調整。
  - [ ] OK    Fix: 

- **`nudgeHint3`**
  - EN:  Nudge with Page Up and Page Down Keys.
  - JA:  Page Up/Downで調整。
  - [ ] OK    Fix: 

- **`offset1`**
  - EN: Object Offset {name} ({unit}). Adjusted when Tracking is Enabled.
  - JA: オブジェクトのオフセット{name}（{unit}）。トラッキング有効時に調整。
  - [ ] OK    Fix: 

- **`offset2`**
  - EN: Object Offset {name} ({unit}). Adjusted when Tracking is Enabled.
  - JA: オブジェクトのオフセット{name}（{unit}）。トラッキング有効時に調整。
  - [ ] OK    Fix: 

- **`offset3`**
  - EN: Object Offset {name} ({unit}). Adjusted when Tracking is Enabled.
  - JA: オブジェクトのオフセット{name}（{unit}）。トラッキング有効時に調整。
  - [ ] OK    Fix: 

- **`otomoAbsRelButton`**
  - EN: Select Relative or Absolute Coordinates of Displacement.
  - JA: 相対または絶対の変位座標を選択。
  - [ ] OK    Fix: 

- **`otomoCoordMode`**
  - EN: Coordinate display mode for AutomOtion destinations: Cartesian (X/Y/Z), Cylindrical (r/θ/Z), or Spherical (r/θ/φ).
  - JA: AutomOtion目的地の座標表示モード：デカルト（X/Y/Z）、円筒（r/θ/Z）、球面（r/θ/φ）。
  - [ ] OK    Fix: 

- **`otomoCurveDial`**
  - EN: Bend the Path to the Left (Negative) or Right (Positive) of the Direction of Travel.
  - JA: 進行方向の左（負）または右（正）にパスを曲げます。
  - [ ] OK    Fix: 

- **`otomoDest1`**
  - EN: Relative or Absolute Destination {name} ({unit}).
  - JA: 相対または絶対の目的地 {name}（{unit}）。
  - [ ] OK    Fix: 

- **`otomoDest2`**
  - EN: Relative or Absolute Destination {name} ({unit}).
  - JA: 相対または絶対の目的地 {name}（{unit}）。
  - [ ] OK    Fix: 

- **`otomoDest3`**
  - EN: Relative or Absolute Destination {name} ({unit}).
  - JA: 相対または絶対の目的地 {name}（{unit}）。
  - [ ] OK    Fix: 

- **`otomoDurationDial`**
  - EN: Duration of the Movement in Seconds (0.1s to 1 hour).
  - JA: 動きの継続時間（秒、0.1秒〜1時間）。
  - [ ] OK    Fix: 

- **`otomoPauseButton`**
  - EN: Pause and Resume the Movement.
  - JA: 動きを一時停止して再開。
  - [ ] OK    Fix: 

- **`otomoPauseResumeAllButton`**
  - EN: Pause or Resume All Active Movements Globally.
  - JA: すべてのアクティブな動きをグローバルに一時停止または再開。
  - [ ] OK    Fix: 

- **`otomoResetDial`**
  - EN: Set the Reset Level for the Automatic Trigger.
  - JA: 自動トリガーのリセットレベルを設定。
  - [ ] OK    Fix: 

- **`otomoSpeedProfileDial`**
  - EN: Constant Speed or Gradual Acceleration and Slow Down at the Start and the End of the Movement.
  - JA: 動きの開始と終了で一定速度または段階的な加速と減速。
  - [ ] OK    Fix: 

- **`otomoStartButton`**
  - EN: Start the Movement Manually.
  - JA: 動きを手動で開始。
  - [ ] OK    Fix: 

- **`otomoStayReturnButton`**
  - EN: At the End of the Movement, should the Source Stay or Return to the Original Position.
  - JA: 動きの終わりに、ソースが留まるか元の位置に戻るか。
  - [ ] OK    Fix: 

- **`otomoStopAllButton`**
  - EN: Stop All Active Movements Globally.
  - JA: すべてのアクティブな動きをグローバルに停止。
  - [ ] OK    Fix: 

- **`otomoStopButton`**
  - EN: Stop the Movement.
  - JA: 動きを停止。
  - [ ] OK    Fix: 

- **`otomoThresholdDial`**
  - EN: Set the Threshold for the Automatic Trigger of the Movement.
  - JA: 動きの自動トリガーのスレッショルドを設定。
  - [ ] OK    Fix: 

- **`otomoTriggerButton`**
  - EN: Manual Start of Displacement or Automatic Trigger on the Audio Level.
  - JA: 変位の手動開始またはオーディオレベルでの自動トリガー。
  - [ ] OK    Fix: 

- **`pathModeButton`**
  - EN: Enable Path Mode to Follow Drawn Movement Paths Instead of Direct Lines.
  - JA: 直線ではなく描画された移動パスに従うパスモードを有効化。
  - [ ] OK    Fix: 

- **`position1`**
  - EN: Object {name} ({unit}).
  - JA: オブジェクトの{name}（{unit}）。
  - [ ] OK    Fix: 

- **`position2`**
  - EN: Object {name} ({unit}).
  - JA: オブジェクトの{name}（{unit}）。
  - [ ] OK    Fix: 

- **`position3`**
  - EN: Object {name} ({unit}).
  - JA: オブジェクトの{name}（{unit}）。
  - [ ] OK    Fix: 

- **`positionJoystick`**
  - EN: Drag to adjust X/Y position in real-time. Returns to center on release.
  - JA: ドラッグしてX/Y位置をリアルタイムに調整。離すと中央に戻ります。
  - [ ] OK    Fix: 

- **`positionZSlider`**
  - EN: Drag to adjust Z (height) position in real-time. Returns to center on release.
  - JA: ドラッグしてZ位置（高さ）をリアルタイムに調整。離すと中央に戻ります。
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Input Configuration from backup file.
  - JA: バックアップファイルから入力設定を再読み込みします。
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Input Configuration from file.
  - JA: ファイルから入力設定を再読み込みします。
  - [ ] OK    Fix: 

- **`reloadSnapshot`**
  - EN: Reload Selected Input Snapshot for All Objects Taking the Scope into Account.
  - JA: フィルターを考慮して、選択した入力スナップショットを全オブジェクトに再読み込みします。
  - [ ] OK    Fix: 

- **`reloadWithoutScope`**
  - EN: Reload Selected Input Snapshot for All Objects Without the Scope.
  - JA: フィルターなしで、選択した入力スナップショットを全オブジェクトに再読み込みします。
  - [ ] OK    Fix: 

- **`sidelinesActiveButton`**
  - EN: Enable Automatic Muting when Source Approaches Stage Edges. Does Not Apply to Downstage (Front) Edge.
  - JA: ソースがステージの端に近づいたときに自動ミュートを有効化。前方（観客側）の端には適用されません。
  - [ ] OK    Fix: 

- **`sidelinesFringeDial`**
  - EN: Fringe Zone Size in Meters. Outer Half is Full Mute, Inner Half Fades Linearly.
  - JA: フリンジゾーンのサイズ（メートル）。外側の半分は完全ミュート、内側の半分は線形フェード。
  - [ ] OK    Fix: 

- **`snapshotSelector`**
  - EN: Select Input Snapshot Without Loading.
  - JA: 入力スナップショットを読み込まずに選択します。
  - [ ] OK    Fix: 

- **`solo`**
  - EN: Listen to Binaural Rendering of this channel.
  - JA: このチャンネルのBinauralレンダリングを聴く。
  - [ ] OK    Fix: 

- **`soloMode`**
  - EN: Single: one input at a time. Multi: multiple inputs simultaneously.
  - JA: 単一: 一度に1つの入力。複数: 複数の入力を同時に。
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Input Configuration to file (with backup).
  - JA: 入力設定をファイルに保存します（バックアップ付き）。
  - [ ] OK    Fix: 

- **`storeSnapshot`**
  - EN: Store new Input Snapshot for All Objects.
  - JA: 全オブジェクトの新しい入力スナップショットを保存します。
  - [ ] OK    Fix: 

- **`tiltSlider`**
  - EN: Where is the Object pointing to in the Vertical Plane.
  - JA: 垂直面でのオブジェクトの方向。
  - [ ] OK    Fix: 

- **`trackingActiveButton`**
  - EN: Enable or Disable Tracking for Object.
  - JA: オブジェクトのトラッキングを有効または無効にします。
  - [ ] OK    Fix: 

- **`trackingIdSelector`**
  - EN: Tracker ID for Object.
  - JA: オブジェクトのトラッカーID。
  - [ ] OK    Fix: 

- **`trackingSmoothDial`**
  - EN: Smoothing of Tracking Data for Object.
  - JA: オブジェクトのトラッキングデータの平滑化。
  - [ ] OK    Fix: 

- **`updateSnapshot`**
  - EN: Update Selected Input Snapshot (with backup).
  - JA: 選択した入力スナップショットを更新します（バックアップ付き）。
  - [ ] OK    Fix: 

## `inputs.messages`

- **`assignedCluster`**
  - EN: Input {channel} assigned to Cluster {cluster}
  - JA: 入力 {channel} をクラスター {cluster} に割り当てました
  - [ ] OK    Fix: 

- **`backupLoaded`**
  - EN: Input configuration loaded from backup.
  - JA: バックアップから入力設定を読み込みました。
  - [ ] OK    Fix: 

- **`configExported`**
  - EN: Input configuration exported.
  - JA: 入力設定をエクスポートしました。
  - [ ] OK    Fix: 

- **`configImported`**
  - EN: Input configuration imported.
  - JA: 入力設定をインポートしました。
  - [ ] OK    Fix: 

- **`configLoaded`**
  - EN: Input configuration loaded.
  - JA: 入力設定を読み込みました。
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Input configuration saved.
  - JA: 入力設定を保存しました。
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - JA: エラー: {error}
  - [ ] OK    Fix: 

- **`noSnapshotSelected`**
  - EN: No snapshot selected.
  - JA: スナップショットが選択されていません。
  - [ ] OK    Fix: 

- **`scopeConfigured`**
  - EN: Scope configured for next snapshot.
  - JA: 次のスナップショットのスコープを設定しました。
  - [ ] OK    Fix: 

- **`scopeSaved`**
  - EN: Snapshot scope saved.
  - JA: スナップショットのスコープを保存しました。
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - JA: まずSystem Configでプロジェクトフォルダを選択してください。
  - [ ] OK    Fix: 

- **`setSingle`**
  - EN: Input {channel} set to Single
  - JA: 入力 {channel} をシングルに設定しました
  - [ ] OK    Fix: 

- **`snapshotDeleted`**
  - EN: Snapshot '{name}' deleted.
  - JA: スナップショット '{name}' を削除しました。
  - [ ] OK    Fix: 

- **`snapshotLoaded`**
  - EN: Snapshot '{name}' loaded.
  - JA: スナップショット '{name}' を読み込みました。
  - [ ] OK    Fix: 

- **`snapshotLoadedWithoutScope`**
  - EN: Snapshot '{name}' loaded (without scope).
  - JA: スナップショット '{name}' を読み込みました（スコープなし）。
  - [ ] OK    Fix: 

- **`snapshotStored`**
  - EN: Snapshot '{name}' stored.
  - JA: スナップショット '{name}' を保存しました。
  - [ ] OK    Fix: 

- **`snapshotUpdated`**
  - EN: Snapshot '{name}' updated.
  - JA: スナップショット '{name}' を更新しました。
  - [ ] OK    Fix: 

- **`trackingDisabled`**
  - EN: Tracking disabled for Input {channel}
  - JA: 入力 {channel} のトラッキングを無効にしました
  - [ ] OK    Fix: 

- **`trackingSwitched`**
  - EN: Tracking switched from Input {from} to Input {to}
  - JA: トラッキングを入力 {from} から入力 {to} に切り替えました
  - [ ] OK    Fix: 

## `levelMeter.tooltips`

- **`clearSolo`**
  - EN: Disengage all solo toggles
  - JA: すべてのソロを解除
  - [ ] OK    Fix: 

- **`solo`**
  - EN: Display the contribution of the input to all outputs in the Level Meter display (in Single mode) and play binaural render of soloed inputs
  - JA: レベルメーター表示で入力の全出力への貢献度を表示（シングルモード時）し、ソロ入力のバイノーラルレンダリングを再生
  - [ ] OK    Fix: 

- **`soloMode`**
  - EN: Single: one input at a time. Multi: multiple inputs simultaneously.
  - JA: 単一: 一度に1つの入力。複数: 複数の入力を同時に。
  - [ ] OK    Fix: 

## `map.messages`

- **`assignedCluster`**
  - EN: Input {channel} assigned to Cluster {cluster}
  - JA: 入力 {channel} をクラスター {cluster} に割り当てました
  - [ ] OK    Fix: 

- **`assignedClusterMulti`**
  - EN: {count} inputs assigned to Cluster {cluster}
  - JA: {count} 個の入力をクラスター {cluster} に割り当てました
  - [ ] OK    Fix: 

- **`clusterBrokenUp`**
  - EN: Cluster {cluster} broken up
  - JA: クラスター {cluster} を解散しました
  - [ ] OK    Fix: 

- **`setSingle`**
  - EN: Input {channel} removed from cluster
  - JA: 入力 {channel} をクラスターから削除しました
  - [ ] OK    Fix: 

- **`setSingleMulti`**
  - EN: {count} inputs removed from clusters
  - JA: {count} 個の入力をクラスターから削除しました
  - [ ] OK    Fix: 

## `map.tooltips`

- **`detach`**
  - EN: Detach the map into a separate window for dual-screen setups
  - JA: デュアルスクリーン設定用にマップを別ウィンドウに切り離す
  - [ ] OK    Fix: 

- **`fitInputs`**
  - EN: Adjust the zoom and panning to fit all visible inputs on the display
  - JA: すべての可視入力が表示されるようにズームとパンを調整
  - [ ] OK    Fix: 

- **`fitStage`**
  - EN: Adjust the zoom and panning to fit the stage on the display
  - JA: ステージが表示されるようにズームとパンを調整
  - [ ] OK    Fix: 

- **`levels`**
  - EN: Display the levels for inputs and outputs on the map
  - JA: マップ上に入出力のレベルを表示
  - [ ] OK    Fix: 

## `meta`

- **`author`**
  - EN: WFS-DIY Team
  - JA: WFS-DIY Team
  - [ ] OK    Fix: 

- **`language`**
  - EN: English
  - JA: 日本語
  - [ ] OK    Fix: 

- **`locale`**
  - EN: en
  - JA: ja
  - [ ] OK    Fix: 

- **`version`**
  - EN: 1.0.0
  - JA: 1.0.0
  - [ ] OK    Fix: 

## `network.dialogs`

- **`exportConfig`**
  - EN: Export Network Configuration
  - JA: ネットワーク設定をエクスポート
  - [ ] OK    Fix: 

- **`findMyRemoteMessage`**
  - EN: Enter the password for your remote device:
  - JA: リモートデバイスのパスワードを入力してください:
  - [ ] OK    Fix: 

- **`findMyRemotePassword`**
  - EN: Password:
  - JA: パスワード:
  - [ ] OK    Fix: 

- **`findMyRemoteTitle`**
  - EN: Find My Remote
  - JA: リモートを探す
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Network Configuration
  - JA: ネットワーク設定をインポート
  - [ ] OK    Fix: 

- **`removeTargetMessage`**
  - EN: Delete target '{name}'?
  - JA: ターゲット「{name}」を削除しますか？
  - [ ] OK    Fix: 

- **`removeTargetTitle`**
  - EN: Remove Target
  - JA: ターゲットを削除
  - [ ] OK    Fix: 

- **`trackingConflictsContinue`**
  - EN: Continue
  - JA: 続行
  - [ ] OK    Fix: 

- **`trackingConflictsMessage`**
  - EN: \nOnly one tracked input per cluster is allowed. If you continue, tracking will be kept only for the first input in each cluster.
  - JA: \nクラスターごとに1つのトラッキング入力のみが許可されています。続行すると、各クラスターの最初の入力のみトラッキングが保持されます。
  - [ ] OK    Fix: 

- **`trackingConflictsTitle`**
  - EN: Tracking Conflicts Detected
  - JA: トラッキングの競合が検出されました
  - [ ] OK    Fix: 

## `network.help`

- **`addTarget`**
  - EN: Add new network target.
  - JA: 新しいネットワークターゲットを追加。
  - [ ] OK    Fix: 

- **`admAxisSwap`**
  - EN: Which incoming ADM-OSC axis maps to this internal axis.
  - JA: (missing — falls back to English)
  - [ ] OK    Fix: 

- **`admAzFlip`**
  - EN: Invert the direction of incoming azimuth.
  - JA: (missing — falls back to English)
  - [ ] OK    Fix: 

- **`admAzOffset`**
  - EN: Azimuth offset (deg) applied to incoming ADM-OSC azimuth.
  - JA: (missing — falls back to English)
  - [ ] OK    Fix: 

- **`admBreakpoint`**
  - EN: Normalized breakpoint (0-1) for piecewise linear stretch.
  - JA: (missing — falls back to English)
  - [ ] OK    Fix: 

- **`admCenterOffset`**
  - EN: Physical position (m) where normalized 0.0 maps to.
  - JA: (missing — falls back to English)
  - [ ] OK    Fix: 

- **`admDistMax`**
  - EN: Maximum physical distance (m) at ADM-OSC distance=1.
  - JA: (missing — falls back to English)
  - [ ] OK    Fix: 

- **`admDistMin`**
  - EN: Minimum physical distance (m) at ADM-OSC distance=0.
  - JA: (missing — falls back to English)
  - [ ] OK    Fix: 

- **`admElFlip`**
  - EN: Invert the sign of incoming elevation.
  - JA: (missing — falls back to English)
  - [ ] OK    Fix: 

- **`admInnerWidth`**
  - EN: Physical extent (m) from center to breakpoint.
  - JA: (missing — falls back to English)
  - [ ] OK    Fix: 

- **`admInputAssign`**
  - EN: Assign this input to an ADM-OSC mapping for receive/transmit.
  - JA: (missing — falls back to English)
  - [ ] OK    Fix: 

- **`admLinkAll`**
  - EN: Select all 6 sides at once for uniform editing.
  - JA: (missing — falls back to English)
  - [ ] OK    Fix: 

- **`admMapping`**
  - EN: Select an ADM-OSC mapping to configure. Cart = Cartesian (xyz), Polar = spherical (aed).
  - JA: 設定するADM-OSCマッピングを選択します。Cart = カーテシアン (xyz)、Polar = 球面 (aed)。
  - [ ] OK    Fix: 

- **`admMappingPanel`**
  - EN: Drag dots to edit mapping. Click axis title to swap, click Flip to invert. Hold Shift to edit both sides symmetrically.
  - JA: ドットをドラッグしてマッピングを編集。軸タイトルをクリックで入れ替え、Flipをクリックで反転。Shiftを押しながらで両側を対称に編集。
  - [ ] OK    Fix: 

- **`admOuterWidth`**
  - EN: Physical extent (m) from breakpoint to ±1.
  - JA: (missing — falls back to English)
  - [ ] OK    Fix: 

- **`admSideSelect`**
  - EN: Select sides to edit. Changes apply to all selected sides at once.
  - JA: (missing — falls back to English)
  - [ ] OK    Fix: 

- **`admSignFlip`**
  - EN: Invert the sign of the incoming axis value.
  - JA: (missing — falls back to English)
  - [ ] OK    Fix: 

- **`currentIP`**
  - EN: IP address of the Processor.
  - JA: プロセッサーのIPアドレス。
  - [ ] OK    Fix: 

- **`dataMode`**
  - EN: Select UDP or TCP data transmission.
  - JA: UDPまたはTCPデータ送信を選択。
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Network Configuration to file.
  - JA: ネットワーク設定をファイルにエクスポート。
  - [ ] OK    Fix: 

- **`findMyRemote`**
  - EN: Make your Remote Flash and Buzz to Find it.
  - JA: リモートを点滅させてバイブレーションで見つけます。
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Network Configuration from file.
  - JA: ファイルからネットワーク設定をインポート。
  - [ ] OK    Fix: 

- **`networkInterface`**
  - EN: Select the Network Interface.
  - JA: Network Interfaceを選択。
  - [ ] OK    Fix: 

- **`openLogWindow`**
  - EN: Open Network Logging window.
  - JA: ネットワークログウィンドウを開く。
  - [ ] OK    Fix: 

- **`oscQueryEnable`**
  - EN: Enable/disable OSC Query server for automatic parameter discovery via HTTP/WebSocket.
  - JA: HTTP/WebSocketによる自動パラメーター検出のため、OSC Queryサーバーを有効/無効にします。
  - [ ] OK    Fix: 

- **`oscQueryPort`**
  - EN: HTTP port for OSC Query discovery. Other apps can browse parameters at http://localhost:<port>/
  - JA: OSC Queryディスカバリー用のHTTPポート。他のアプリは http://localhost:<port>/ でパラメーターを参照できます。
  - [ ] OK    Fix: 

- **`oscSourceFilter`**
  - EN: Filter incoming OSC: Accept All sources or only Registered connections with Rx enabled.
  - JA: 受信OSCをフィルタリング: 全ソースを受け入れるか、Rx有効の登録済み接続のみ。
  - [ ] OK    Fix: 

- **`protocol`**
  - EN: Select the Protocol: DISABLED, OSC, REMOTE, or ADM-OSC.
  - JA: プロトコルを選択: DISABLED、OSC、REMOTE、またはADM-OSC。
  - [ ] OK    Fix: 

- **`psnInterface`**
  - EN: Network interface for PSN multicast reception
  - JA: PSNマルチキャスト受信用のネットワークインターフェース
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Network Configuration from backup file.
  - JA: バックアップファイルからネットワーク設定を再読み込み。
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Network Configuration from file.
  - JA: ファイルからネットワーク設定を再読み込み。
  - [ ] OK    Fix: 

- **`removeTarget`**
  - EN: Delete this Network Target.
  - JA: このネットワークターゲットを削除。
  - [ ] OK    Fix: 

- **`rxEnable`**
  - EN: Enable or Disable Data Reception.
  - JA: データ受信を有効または無効にします。
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Network Configuration to file.
  - JA: ネットワーク設定をファイルに保存。
  - [ ] OK    Fix: 

- **`targetIP`**
  - EN: IP Address of the Target (use 127.0.0.1 for local host).
  - JA: ターゲットのIPアドレス（ローカルホストには127.0.0.1を使用）。
  - [ ] OK    Fix: 

- **`targetName`**
  - EN: Network Target Name.
  - JA: ネットワークターゲット名。
  - [ ] OK    Fix: 

- **`tcpPort`**
  - EN: TCP Receive Port of the Processor.
  - JA: プロセッサーのTCP受信ポート。
  - [ ] OK    Fix: 

- **`trackingEnabled`**
  - EN: Enable or Disable Incoming Tracking data processing.
  - JA: 受信トラッキングデータの処理を有効または無効にします。
  - [ ] OK    Fix: 

- **`trackingFlipX`**
  - EN: Invert Axis of Tracking X Coordinate.
  - JA: トラッキングX座標の軸を反転。
  - [ ] OK    Fix: 

- **`trackingFlipY`**
  - EN: Invert Axis of Tracking Y Coordinate.
  - JA: トラッキングY座標の軸を反転。
  - [ ] OK    Fix: 

- **`trackingFlipZ`**
  - EN: Invert Axis of Tracking Z Coordinate.
  - JA: トラッキングZ座標の軸を反転。
  - [ ] OK    Fix: 

- **`trackingOffsetX`**
  - EN: Offset Tracking X Coordinate.
  - JA: トラッキングX座標のオフセット。
  - [ ] OK    Fix: 

- **`trackingOffsetY`**
  - EN: Offset Tracking Y Coordinate.
  - JA: トラッキングY座標のオフセット。
  - [ ] OK    Fix: 

- **`trackingOffsetZ`**
  - EN: Offset Tracking Z Coordinate.
  - JA: トラッキングZ座標のオフセット。
  - [ ] OK    Fix: 

- **`trackingOscPath`**
  - EN: OSC Path for the Tracking in OSC Mode (starts with a /)
  - JA: OSCモードでのトラッキング用OSC Path（/で始まる）
  - [ ] OK    Fix: 

- **`trackingPort`**
  - EN: Specify the Port to receive Tracking data.
  - JA: トラッキングデータを受信するポートを指定。
  - [ ] OK    Fix: 

- **`trackingProtocol`**
  - EN: Select the type of Tracking Protocol.
  - JA: トラッキングプロトコルの種類を選択。
  - [ ] OK    Fix: 

- **`trackingScaleX`**
  - EN: Scale Tracking X Coordinate.
  - JA: トラッキングX座標のスケール。
  - [ ] OK    Fix: 

- **`trackingScaleY`**
  - EN: Scale Tracking Y Coordinate.
  - JA: トラッキングY座標のスケール。
  - [ ] OK    Fix: 

- **`trackingScaleZ`**
  - EN: Scale Tracking Z Coordinate.
  - JA: トラッキングZ座標のスケール。
  - [ ] OK    Fix: 

- **`txEnable`**
  - EN: Enable or Disable Data Transmission.
  - JA: データ送信を有効または無効にします。
  - [ ] OK    Fix: 

- **`txPort`**
  - EN: Transmit Port for this Target.
  - JA: このターゲットの送信ポート。
  - [ ] OK    Fix: 

- **`udpPort`**
  - EN: UDP Receive Port of the Processor.
  - JA: プロセッサーのUDP受信ポート。
  - [ ] OK    Fix: 

## `network.messages`

- **`configExported`**
  - EN: Network configuration exported.
  - JA: ネットワーク設定をエクスポートしました。
  - [ ] OK    Fix: 

- **`configImported`**
  - EN: Network configuration imported.
  - JA: ネットワーク設定をインポートしました。
  - [ ] OK    Fix: 

- **`configLoadedFromBackup`**
  - EN: Network configuration loaded from backup.
  - JA: バックアップからネットワーク設定を読み込みました。
  - [ ] OK    Fix: 

- **`configNotFound`**
  - EN: Network config file not found.
  - JA: ネットワーク設定ファイルが見つかりません。
  - [ ] OK    Fix: 

- **`configReloaded`**
  - EN: Network configuration reloaded.
  - JA: ネットワーク設定を再読込しました。
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Network configuration saved.
  - JA: ネットワーク設定を保存しました。
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - JA: エラー: {error}
  - [ ] OK    Fix: 

- **`findDeviceSent`**
  - EN: Find Device command sent.
  - JA: デバイス検索コマンドを送信しました。
  - [ ] OK    Fix: 

- **`maxTargetsReached`**
  - EN: Maximum Number of Targets/Servers Reached.
  - JA: ターゲット/サーバーの最大数に達しました。
  - [ ] OK    Fix: 

- **`noBackupFound`**
  - EN: No backup files found.
  - JA: バックアップファイルが見つかりません。
  - [ ] OK    Fix: 

- **`onlyOneRemote`**
  - EN: Only one REMOTE connection is allowed.
  - JA: リモート接続は1つのみ許可されています。
  - [ ] OK    Fix: 

- **`oscManagerError`**
  - EN: Error: OSC Manager not available
  - JA: エラー: OSCマネージャーが利用できません
  - [ ] OK    Fix: 

- **`passwordEmpty`**
  - EN: Password cannot be empty.
  - JA: パスワードを空にすることはできません。
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - JA: まずSystem Configでプロジェクトフォルダを選択してください。
  - [ ] OK    Fix: 

## `networkLog.dialogs`

- **`exportCompleteMessage`**
  - EN: Log exported to: {path}
  - JA: ログをエクスポートしました: {path}
  - [ ] OK    Fix: 

- **`exportCompleteTitle`**
  - EN: Export Complete
  - JA: エクスポート完了
  - [ ] OK    Fix: 

- **`exportFailedMessage`**
  - EN: Could not write to file: {path}
  - JA: ファイルに書き込めませんでした: {path}
  - [ ] OK    Fix: 

- **`exportFailedTitle`**
  - EN: Export Failed
  - JA: エクスポート失敗
  - [ ] OK    Fix: 

## `networkLog.status`

- **`rejected`**
  - EN: REJECTED
  - JA: 拒否
  - [ ] OK    Fix: 

## `outputs.dialogs`

- **`export`**
  - EN: Export Output Configuration
  - JA: 出力設定をエクスポート
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import Output Configuration
  - JA: 出力設定をインポート
  - [ ] OK    Fix: 

## `outputs.help`

- **`angleOff`**
  - EN: Output Channel Will Not Amplify Objects in this Angle in Front of it. (changes may affect the rest of the array)
  - JA: 出力チャンネルはこの角度内の前方にあるオブジェクトを増幅しません。（変更はアレイの残りに影響する場合があります）
  - [ ] OK    Fix: 

- **`angleOn`**
  - EN: Output Channel Will Amplify Objects in this Angle in its Back. (changes may affect the rest of the array)
  - JA: 出力チャンネルはこの角度内の背後にあるオブジェクトを増幅します。（変更はアレイの残りに影響する場合があります）
  - [ ] OK    Fix: 

- **`applyToArray`**
  - EN: Apply Changes to the rest of the Array (Absolute value or Relative changes).
  - JA: アレイの残りに変更を適用（絶対値または相対変更）。
  - [ ] OK    Fix: 

- **`arraySelector`**
  - EN: Selected Output Channel is part of Array.
  - JA: 選択した出力チャンネルはアレイの一部です。
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Output Channel Attenuation. (changes may affect the rest of the array)
  - JA: 出力チャンネルの減衰。（変更はアレイの残りに影響する場合があります）
  - [ ] OK    Fix: 

- **`channelSelector`**
  - EN: Output Channel Number and Selection.
  - JA: 出力チャンネル番号と選択。
  - [ ] OK    Fix: 

- **`coordMode`**
  - EN: Coordinate display mode: Cartesian (X/Y/Z), Cylindrical (radius/azimuth/height), or Spherical (radius/azimuth/elevation).
  - JA: 座標表示モード：直交座標（X/Y/Z）、円筒座標（半径/方位角/高さ）、または球面座標（半径/方位角/仰角）。
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Output Channel Delay (positive values) or Latency Compensation (negative values). (changes may affect the rest of the array)
  - JA: 出力チャンネルのディレイ（正の値）またはレイテンシー補正（負の値）。（変更はアレイの残りに影響する場合があります）
  - [ ] OK    Fix: 

- **`directional`**
  - EN: Output Channel Directional Control. Drag to change orientation, Shift+drag for Angle Off, Alt+drag for Angle On. (changes may affect the rest of the array)
  - JA: 出力チャンネルの指向性制御。ドラッグで向きを変更、Shift+ドラッグでAngle Off、Alt+ドラッグでAngle On。（変更はアレイの残りに影響する場合があります）
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Ratio of Distance Attenuation for Selected Output. (changes may affect the rest of the array)
  - JA: 選択した出力の距離減衰比率。（変更はアレイの残りに影響する場合があります）
  - [ ] OK    Fix: 

- **`eqBandToggle`**
  - EN: Toggle Band {band} on/off. When off the band is bypassed.
  - JA: バンド{band}のオン/オフ切り替え。オフの場合バイパスされます。
  - [ ] OK    Fix: 

- **`eqEnable`**
  - EN: Enable or disable EQ processing for this output.
  - JA: この出力のEQ処理を有効/無効にします。
  - [ ] OK    Fix: 

- **`eqFlatten`**
  - EN: Long press to reset all EQ bands to their default values.
  - JA: 長押しで全EQバンドをデフォルトにリセットします。
  - [ ] OK    Fix: 

- **`eqFreq`**
  - EN: Output EQ Band {band} frequency (20 Hz - 20 kHz).
  - JA: Output EQバンド{band}の周波数 (20 Hz - 20 kHz)。
  - [ ] OK    Fix: 

- **`eqGain`**
  - EN: Output EQ Band {band} gain (-24 to +24 dB).
  - JA: Output EQバンド{band}のゲイン (-24〜+24 dB)。
  - [ ] OK    Fix: 

- **`eqQ`**
  - EN: Output EQ Band {band} Q factor (0.1 - 10.0).
  - JA: Output EQバンド{band}のQ値 (0.1 - 10.0)。
  - [ ] OK    Fix: 

- **`eqResetBand`**
  - EN: Long press to reset Band {band} to its default values.
  - JA: 長押しでバンド{band}をデフォルトにリセットします。
  - [ ] OK    Fix: 

- **`eqShape`**
  - EN: Output EQ Band {band} filter shape.
  - JA: Output EQバンド{band}のフィルター形状。
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Output Configuration to file (with file explorer window).
  - JA: 出力設定をファイルにエクスポートします（ファイルエクスプローラーウィンドウ付き）。
  - [ ] OK    Fix: 

- **`floorReflections`**
  - EN: Enable or Disable the Floor Reflections for this Speaker.
  - JA: このスピーカーのFloor Reflectionsを有効または無効にします。
  - [ ] OK    Fix: 

- **`hParallax`**
  - EN: Horizontal Distance from Speaker to 'Targeted' Listener. (changes may affect the rest of the array)
  - JA: スピーカーから「ターゲット」リスナーまでの水平距離。（変更はアレイの残りに影響する場合があります）
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: Loss of High Frequency Depending on Distance from Object to Output. (changes may affect the rest of the array)
  - JA: オブジェクトから出力までの距離に応じた高周波の損失。（変更はアレイの残りに影響する場合があります）
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Output Configuration from file (with file explorer window).
  - JA: ファイルから出力設定をインポートします（ファイルエクスプローラーウィンドウ付き）。
  - [ ] OK    Fix: 

- **`liveSource`**
  - EN: Disables Live Source Attenuation for Selected Output. (changes may affect the rest of the array)
  - JA: 選択した出力のLive Source減衰を無効にします。（変更はアレイの残りに影響する場合があります）
  - [ ] OK    Fix: 

- **`mapVisibility`**
  - EN: Make Visible or Hide The Selected Output on the Map.
  - JA: 選択した出力をマップ上で表示または非表示にします。
  - [ ] OK    Fix: 

- **`minLatency`**
  - EN: Disables Minimal Latency Mode for Selected Output. (changes may affect the rest of the array)
  - JA: 選択した出力のMinimal Latencyモードを無効にします。（変更はアレイの残りに影響する場合があります）
  - [ ] OK    Fix: 

- **`nameEditor`**
  - EN: Displayed Output Channel Name (editable).
  - JA: 表示される出力チャンネル名（編集可能）。
  - [ ] OK    Fix: 

- **`pitch`**
  - EN: Output Channel Vertical Orientation used to Determine which Objects get amplified. (changes may affect the rest of the array)
  - JA: どのオブジェクトを増幅するかを決定するための出力チャンネルの垂直方向。（変更はアレイの残りに影響する場合があります）
  - [ ] OK    Fix: 

- **`position1`**
  - EN: Output Channel {name} ({unit}).
  - JA: 出力チャンネル{name}（{unit}）。
  - [ ] OK    Fix: 

- **`position2`**
  - EN: Output Channel {name} ({unit}).
  - JA: 出力チャンネル{name}（{unit}）。
  - [ ] OK    Fix: 

- **`position3`**
  - EN: Output Channel {name} ({unit}).
  - JA: 出力チャンネル{name}（{unit}）。
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Output Configuration from backup file.
  - JA: バックアップファイルから出力設定を再読み込みします。
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Output Configuration from file.
  - JA: ファイルから出力設定を再読み込みします。
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Output Configuration to file (with backup).
  - JA: 出力設定をファイルに保存します（バックアップ付き）。
  - [ ] OK    Fix: 

- **`vParallax`**
  - EN: Vertical Distance from Speaker to 'Targeted' Listener. Positive when the Speaker is Below the head of the Listener. (changes may affect the rest of the array)
  - JA: スピーカーから「ターゲット」リスナーまでの垂直距離。スピーカーがリスナーの頭より下にある場合は正の値。（変更はアレイの残りに影響する場合があります）
  - [ ] OK    Fix: 

- **`wizardOfOutZ`**
  - EN: Open Wizard of OutZ to Position Speaker Arrays Conveniently.
  - JA: Wizard of OutZを開いてスピーカーアレイを簡単に配置します。
  - [ ] OK    Fix: 

## `outputs.messages`

- **`assignedToArray`**
  - EN: Output {num} assigned to Array {array}
  - JA: 出力 {num} をアレイ {array} に割り当て
  - [ ] OK    Fix: 

- **`backupLoaded`**
  - EN: Output configuration loaded from backup.
  - JA: バックアップから出力設定を読み込みました。
  - [ ] OK    Fix: 

- **`configExported`**
  - EN: Output configuration exported.
  - JA: 出力設定をエクスポートしました。
  - [ ] OK    Fix: 

- **`configImported`**
  - EN: Output configuration imported.
  - JA: 出力設定をインポートしました。
  - [ ] OK    Fix: 

- **`configLoaded`**
  - EN: Output configuration loaded.
  - JA: 出力設定を読み込みました。
  - [ ] OK    Fix: 

- **`configNotFound`**
  - EN: Output config file not found.
  - JA: 出力設定ファイルが見つかりません。
  - [ ] OK    Fix: 

- **`configReloaded`**
  - EN: Output configuration reloaded.
  - JA: 出力設定を再読込しました。
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Output configuration saved.
  - JA: 出力設定を保存しました。
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - JA: エラー: {error}
  - [ ] OK    Fix: 

- **`noBackup`**
  - EN: No backup output configuration found.
  - JA: 出力設定のバックアップが見つかりません。
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - JA: まずSystem Configでプロジェクトフォルダを選択してください。
  - [ ] OK    Fix: 

- **`setToSingle`**
  - EN: Output {num} set to Single
  - JA: 出力 {num} をシングルに設定
  - [ ] OK    Fix: 

## `reverbs.dialogs`

- **`export`**
  - EN: Export Reverb Configuration
  - JA: リバーブ設定をエクスポート
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import Reverb Configuration
  - JA: リバーブ設定をインポート
  - [ ] OK    Fix: 

## `reverbs.help`

- **`algoCrossoverHigh`**
  - EN: High crossover frequency for 3-band decay (1 - 10 kHz).
  - JA: 3バンド減衰用の高域クロスオーバー周波数（1 - 10 kHz）。
  - [ ] OK    Fix: 

- **`algoCrossoverLow`**
  - EN: Low crossover frequency for 3-band decay (50 - 500 Hz).
  - JA: 3バンド減衰用の低域クロスオーバー周波数（50 - 500 Hz）。
  - [ ] OK    Fix: 

- **`algoDiffusion`**
  - EN: Diffusion amount controlling echo density (0 - 100%).
  - JA: エコー密度を制御する拡散量（0 - 100%）。
  - [ ] OK    Fix: 

- **`algoFDN`**
  - EN: Select FDN (Feedback Delay Network) reverb algorithm.
  - JA: FDN（Feedback Delay Network）リバーブアルゴリズムを選択。
  - [ ] OK    Fix: 

- **`algoFDNSize`**
  - EN: FDN delay line size multiplier (0.5 - 2.0x).
  - JA: FDN ディレイライン長の乗数（0.5 - 2.0x）。
  - [ ] OK    Fix: 

- **`algoIR`**
  - EN: Select IR (Impulse Response convolution) reverb algorithm.
  - JA: IR（インパルス応答畳み込み）リバーブアルゴリズムを選択。
  - [ ] OK    Fix: 

- **`algoIRFile`**
  - EN: Select or import an impulse response file for convolution.
  - JA: 畳み込み用のインパルス応答ファイルを選択またはインポート。
  - [ ] OK    Fix: 

- **`algoIRLength`**
  - EN: Maximum impulse response length (0.1 - 6.0 seconds).
  - JA: インパルス応答の最大長（0.1 - 6.0秒）。
  - [ ] OK    Fix: 

- **`algoIRTrim`**
  - EN: Trim the start of the impulse response (0 - 100 ms).
  - JA: インパルス応答の先頭をトリム（0 - 100 ms）。
  - [ ] OK    Fix: 

- **`algoPerNode`**
  - EN: Use a separate IR for each reverb node, or share one IR.
  - JA: リバーブノードごとに別々のIRを使用、または1つのIRを共有。
  - [ ] OK    Fix: 

- **`algoRT60`**
  - EN: Reverb decay time RT60 (0.2 - 8.0 seconds).
  - JA: リバーブ減衰時間 RT60（0.2 - 8.0秒）。
  - [ ] OK    Fix: 

- **`algoRT60HighMult`**
  - EN: High frequency RT60 multiplier (0.1 - 9.0x).
  - JA: 高域 RT60 乗数（0.1 - 9.0x）。
  - [ ] OK    Fix: 

- **`algoRT60LowMult`**
  - EN: Low frequency RT60 multiplier (0.1 - 9.0x).
  - JA: 低域 RT60 乗数（0.1 - 9.0x）。
  - [ ] OK    Fix: 

- **`algoSDN`**
  - EN: Select SDN (Scattering Delay Network) reverb algorithm.
  - JA: SDN（Scattering Delay Network）リバーブアルゴリズムを選択。
  - [ ] OK    Fix: 

- **`algoSDNScale`**
  - EN: SDN inter-node delay scale factor (0.5 - 4.0x).
  - JA: SDN ノード間遅延スケールファクター（0.5 - 4.0x）。
  - [ ] OK    Fix: 

- **`algoWetLevel`**
  - EN: Wet/dry mix level for reverb output (-60 to +12 dB).
  - JA: リバーブ出力のウェット/ドライミックスレベル（-60～+12 dB）。
  - [ ] OK    Fix: 

- **`angleOff`**
  - EN: Angle at which no amplification occurs (0-179 degrees).
  - JA: 増幅が行われない角度（0-179度）。
  - [ ] OK    Fix: 

- **`angleOn`**
  - EN: Angle at which amplification starts (1-180 degrees).
  - JA: 増幅が開始する角度（1-180度）。
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Reverb channel attenuation (-92 to 0 dB).
  - JA: リバーブチャンネルの減衰（-92～0 dB）。
  - [ ] OK    Fix: 

- **`channelSelector`**
  - EN: Reverb Channel Number and Selection.
  - JA: リバーブチャンネル番号と選択。
  - [ ] OK    Fix: 

- **`commonAtten`**
  - EN: Common attenuation percentage (0-100%).
  - JA: 共通減衰のパーセンテージ（0-100%）。
  - [ ] OK    Fix: 

- **`coordMode`**
  - EN: Coordinate display mode: Cartesian (X/Y/Z), Cylindrical (radius/azimuth/height), or Spherical (radius/azimuth/elevation).
  - JA: 座標表示モード：デカルト座標（X/Y/Z）、円筒座標（半径/方位角/高さ）、または球座標（半径/方位角/仰角）。
  - [ ] OK    Fix: 

- **`delayLatency`**
  - EN: Reverb delay/latency compensation (-100 to +100 ms).
  - JA: リバーブの遅延/レイテンシー補正（-100～+100 ms）。
  - [ ] OK    Fix: 

- **`distanceAtten`**
  - EN: Distance attenuation for reverb return (-6.0 to 0.0 dB/m).
  - JA: Reverb ReturnのDistance attenuation（-6.0～0.0 dB/m）。
  - [ ] OK    Fix: 

- **`distanceAttenEnable`**
  - EN: Distance attenuation percentage (0-200%).
  - JA: Distance attenuationのパーセンテージ（0-200%）。
  - [ ] OK    Fix: 

- **`eqBandToggle`**
  - EN: Toggle pre-EQ Band {band} on/off. When off the band is bypassed.
  - JA: プリEQバンド{band}のオン/オフ切り替え。
  - [ ] OK    Fix: 

- **`eqEnable`**
  - EN: Enable or disable EQ processing for this reverb.
  - JA: このリバーブのEQ処理を有効または無効にする。
  - [ ] OK    Fix: 

- **`eqFlatten`**
  - EN: Long press to reset all pre-EQ bands to their default values.
  - JA: 長押しで全プリEQバンドをデフォルトにリセットします。
  - [ ] OK    Fix: 

- **`eqFreq`**
  - EN: Pre-EQ Band {band} frequency (20 Hz - 20 kHz).
  - JA: プリEQバンド{band}の周波数 (20 Hz - 20 kHz)。
  - [ ] OK    Fix: 

- **`eqGain`**
  - EN: Pre-EQ Band {band} gain (-24 to +24 dB).
  - JA: プリEQバンド{band}のゲイン (-24〜+24 dB)。
  - [ ] OK    Fix: 

- **`eqQ`**
  - EN: Pre-EQ Band {band} Q factor (0.1 - 20.0).
  - JA: プリEQバンド{band}のQ値 (0.1 - 20.0)。
  - [ ] OK    Fix: 

- **`eqResetBand`**
  - EN: Long press to reset pre-EQ Band {band} to its default values.
  - JA: 長押しでプリEQバンド{band}をデフォルトにリセットします。
  - [ ] OK    Fix: 

- **`eqShape`**
  - EN: Pre-EQ Band {band} filter shape.
  - JA: プリEQバンド{band}のフィルター形状。
  - [ ] OK    Fix: 

- **`exportConfig`**
  - EN: Export Reverb Configuration to file (with file explorer window).
  - JA: ファイルにリバーブ設定をエクスポート（ファイルエクスプローラー使用）。
  - [ ] OK    Fix: 

- **`hfDamping`**
  - EN: High frequency loss per meter (-6.0 to 0.0 dB/m).
  - JA: メートルあたりの高周波損失（-6.0～0.0 dB/m）。
  - [ ] OK    Fix: 

- **`importConfig`**
  - EN: Import Reverb Configuration from file (with file explorer window).
  - JA: ファイルからリバーブ設定をインポート（ファイルエクスプローラー使用）。
  - [ ] OK    Fix: 

- **`liveSourceTooltip`**
  - EN: Enable Live Source attenuation tamer. Reduces level fluctuations from sources close to the array.
  - JA: Live Source Tamerを有効化。アレイに近いソースのレベル変動を抑えます。
  - [ ] OK    Fix: 

- **`mapVisibility`**
  - EN: Make Visible or Hide All Reverb Channels on the Map.
  - JA: マップ上のすべてのReverb Channelsを表示または非表示にする。
  - [ ] OK    Fix: 

- **`miniLatencyTooltip`**
  - EN: Enable minimal latency mode for this reverb channel. Reduces processing delay at the cost of higher CPU usage.
  - JA: このリバーブチャンネルの最小レイテンシーモードを有効化。CPU使用率の上昇と引き換えに処理遅延を低減します。
  - [ ] OK    Fix: 

- **`muteButton`**
  - EN: Toggle mute for this output channel's reverb return.
  - JA: この出力チャンネルのリバーブリターンのミュートを切り替えます。
  - [ ] OK    Fix: 

- **`muteMacros`**
  - EN: Quick mute operations for output channels.
  - JA: 出力チャンネルのクイックミュート操作。
  - [ ] OK    Fix: 

- **`nameEditor`**
  - EN: Displayed Reverb Channel Name (editable).
  - JA: 表示されるリバーブチャンネル名（編集可能）。
  - [ ] OK    Fix: 

- **`orientation`**
  - EN: Reverb orientation angle (-179 to +180 degrees).
  - JA: リバーブの向き角度（-179～+180度）。
  - [ ] OK    Fix: 

- **`pitch`**
  - EN: Vertical orientation of reverb (-90 to +90 degrees).
  - JA: リバーブの垂直方向（-90～+90度）。
  - [ ] OK    Fix: 

- **`position1`**
  - EN: Reverb virtual source {name} ({unit}).
  - JA: リバーブ仮想音源の{name}（{unit}）。
  - [ ] OK    Fix: 

- **`position2`**
  - EN: Reverb virtual source {name} ({unit}).
  - JA: リバーブ仮想音源の{name}（{unit}）。
  - [ ] OK    Fix: 

- **`position3`**
  - EN: Reverb virtual source {name} ({unit}).
  - JA: リバーブ仮想音源の{name}（{unit}）。
  - [ ] OK    Fix: 

- **`postEqBandToggle`**
  - EN: Toggle post-EQ Band {band} on/off. When off the band is bypassed.
  - JA: ポストEQバンド{band}のオン/オフ切り替え。
  - [ ] OK    Fix: 

- **`postEqEnable`**
  - EN: Enable or disable post-processing EQ.
  - JA: ポストプロセッシングEQを有効または無効にする。
  - [ ] OK    Fix: 

- **`postEqFlatten`**
  - EN: Long press to reset all post-EQ bands to their default values.
  - JA: 長押しで全ポストEQバンドをデフォルトにリセットします。
  - [ ] OK    Fix: 

- **`postEqFreq`**
  - EN: Post-EQ Band {band} frequency (20 Hz - 20 kHz).
  - JA: ポストEQ バンド {band} の周波数（20 Hz - 20 kHz）。
  - [ ] OK    Fix: 

- **`postEqGain`**
  - EN: Post-EQ Band {band} gain (-24 to +24 dB).
  - JA: ポストEQ バンド {band} のゲイン（-24～+24 dB）。
  - [ ] OK    Fix: 

- **`postEqQ`**
  - EN: Post-EQ Band {band} Q factor (0.1 - 20.0).
  - JA: ポストEQ バンド {band} のQ値（0.1 - 20.0）。
  - [ ] OK    Fix: 

- **`postEqResetBand`**
  - EN: Long press to reset post-EQ Band {band} to its default values.
  - JA: 長押しでポストEQバンド{band}をデフォルトにリセットします。
  - [ ] OK    Fix: 

- **`postEqShape`**
  - EN: Post-EQ Band {band} filter shape.
  - JA: ポストEQ バンド {band} のフィルター形状。
  - [ ] OK    Fix: 

- **`postExpAttack`**
  - EN: Post-expander attack time (0.1 - 50 ms).
  - JA: ポストエキスパンダーのアタック時間（0.1 - 50 ms）。
  - [ ] OK    Fix: 

- **`postExpBypass`**
  - EN: Bypass or enable the post-expander on reverb returns.
  - JA: リバーブリターンのポストエキスパンダーをバイパスまたは有効にする。
  - [ ] OK    Fix: 

- **`postExpRatio`**
  - EN: Post-expander ratio (1:1 to 1:8).
  - JA: ポストエキスパンダーのレシオ（1:1～1:8）。
  - [ ] OK    Fix: 

- **`postExpRelease`**
  - EN: Post-expander release time (50 - 2000 ms).
  - JA: ポストエキスパンダーのリリース時間（50 - 2000 ms）。
  - [ ] OK    Fix: 

- **`postExpThreshold`**
  - EN: Post-expander threshold (-80 to -10 dB).
  - JA: ポストエキスパンダーのスレッショルド（-80～-10 dB）。
  - [ ] OK    Fix: 

- **`preCompAttack`**
  - EN: Pre-compressor attack time (0.1 - 100 ms).
  - JA: プリコンプレッサーのアタック時間（0.1 - 100 ms）。
  - [ ] OK    Fix: 

- **`preCompBypass`**
  - EN: Bypass or enable the pre-compressor on reverb feeds.
  - JA: リバーブフィードのプリコンプレッサーをバイパスまたは有効にする。
  - [ ] OK    Fix: 

- **`preCompRatio`**
  - EN: Pre-compressor ratio (1:1 to 20:1).
  - JA: プリコンプレッサーのレシオ（1:1～20:1）。
  - [ ] OK    Fix: 

- **`preCompRelease`**
  - EN: Pre-compressor release time (10 - 1000 ms).
  - JA: プリコンプレッサーのリリース時間（10 - 1000 ms）。
  - [ ] OK    Fix: 

- **`preCompThreshold`**
  - EN: Pre-compressor threshold (-60 to 0 dB).
  - JA: プリコンプレッサーのスレッショルド（-60～0 dB）。
  - [ ] OK    Fix: 

- **`reloadBackup`**
  - EN: Reload Reverb Configuration from backup file.
  - JA: バックアップファイルからリバーブ設定を再読み込み。
  - [ ] OK    Fix: 

- **`reloadConfig`**
  - EN: Reload Reverb Configuration from file.
  - JA: ファイルからリバーブ設定を再読み込み。
  - [ ] OK    Fix: 

- **`returnOffset1`**
  - EN: Reverb return offset {name} ({unit}).
  - JA: リバーブリターンオフセット{name}（{unit}）。
  - [ ] OK    Fix: 

- **`returnOffset2`**
  - EN: Reverb return offset {name} ({unit}).
  - JA: リバーブリターンオフセット{name}（{unit}）。
  - [ ] OK    Fix: 

- **`returnOffset3`**
  - EN: Reverb return offset {name} ({unit}).
  - JA: リバーブリターンオフセット{name}（{unit}）。
  - [ ] OK    Fix: 

- **`storeConfig`**
  - EN: Store Reverb Configuration to file (with backup).
  - JA: リバーブ設定をファイルに保存（バックアップ付き）。
  - [ ] OK    Fix: 

## `reverbs.messages`

- **`backupLoaded`**
  - EN: Reverb configuration loaded from backup.
  - JA: バックアップからリバーブ設定を読み込みました。
  - [ ] OK    Fix: 

- **`configExported`**
  - EN: Reverb configuration exported.
  - JA: リバーブ設定をエクスポートしました。
  - [ ] OK    Fix: 

- **`configImported`**
  - EN: Reverb configuration imported.
  - JA: リバーブ設定をインポートしました。
  - [ ] OK    Fix: 

- **`configLoaded`**
  - EN: Reverb configuration loaded.
  - JA: リバーブ設定を読み込みました。
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Reverb configuration saved.
  - JA: リバーブ設定を保存しました。
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - JA: エラー: {error}
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder in System Config first.
  - JA: まずSystem Configでプロジェクトフォルダを選択してください。
  - [ ] OK    Fix: 

## `sampler`

- **`guide`**
  - EN: Select a cell on the grid to edit its properties.\nDouble-click to load a sample.\nUse Ctrl+Click to assign cells to the active set.
  - JA: グリッド上のセルを選択してプロパティを編集します。\nダブルクリックでサンプルをロード。\nCtrl+クリックでセルをアクティブセットに割り当て。
  - [ ] OK    Fix: 

## `sampler.grid`

- **`help`**
  - EN: Click = select\nShift = multi\nCtrl = set toggle\nDblClick = load
  - JA: クリック=選択 | Shift=複数 | Ctrl=セット切替 | ダブルクリック=読込
  - [ ] OK    Fix: 

## `sampler.tooltips`

- **`addSet`**
  - EN: Create a new set. If cells are selected they will be assigned to it.
  - JA: 新しいセットを作成。セルが選択されている場合、それらが割り当てられます。
  - [ ] OK    Fix: 

- **`attenuation`**
  - EN: Cell attenuation in dB (0 = no attenuation, -60 = silent)
  - JA: セルの減衰（dB、0=減衰なし、-60=無音）
  - [ ] OK    Fix: 

- **`clear`**
  - EN: Remove the sample from the selected cell (long press)
  - JA: 選択したセルからサンプルを削除（長押し）
  - [ ] OK    Fix: 

- **`copy`**
  - EN: Copy selected cell or active set to clipboard
  - JA: 選択したセルまたはアクティブなセットをクリップボードにコピー
  - [ ] OK    Fix: 

- **`deleteSet`**
  - EN: Delete the active set (long press)
  - JA: アクティブなセットを削除（長押し）
  - [ ] OK    Fix: 

- **`export`**
  - EN: Export sampler configuration to file
  - JA: サンプラー設定をファイルにエクスポート
  - [ ] OK    Fix: 

- **`import`**
  - EN: Import sampler configuration from file
  - JA: サンプラー設定をファイルからインポート
  - [ ] OK    Fix: 

- **`inOut`**
  - EN: Set the In/Out time range in milliseconds. Drag between thumbs to shift both.
  - JA: イン/アウト時間範囲をミリ秒で設定。サムの間をドラッグして両方を移動。
  - [ ] OK    Fix: 

- **`load`**
  - EN: Load a sample file into the selected cell
  - JA: 選択したセルにサンプルファイルをロード
  - [ ] OK    Fix: 

- **`offset`**
  - EN: Offset position in meters (X, Y, Z) relative to the set position
  - JA: セット位置に対するメートル単位の位置オフセット（X、Y、Z）
  - [ ] OK    Fix: 

- **`paste`**
  - EN: Paste clipboard data to selected cell or active set
  - JA: クリップボードのデータを選択したセルまたはアクティブなセットに貼り付け
  - [ ] OK    Fix: 

- **`playMode`**
  - EN: Toggle between Sequential and Round-Robin playback
  - JA: シーケンシャルとラウンドロビン再生を切り替え
  - [ ] OK    Fix: 

- **`pressCurve`**
  - EN: Pressure response curve (0 = concave, 0.5 = linear, 1 = convex)
  - JA: 圧力応答カーブ（0=凹、0.5=線形、1=凸）
  - [ ] OK    Fix: 

- **`pressDir`**
  - EN: Toggle pressure direction: + = more pressure increases, - = decreases
  - JA: 圧力方向を切り替え: + = 圧力増加で増加、- = 減少
  - [ ] OK    Fix: 

- **`pressHF`**
  - EN: Map finger pressure to high-frequency shelf attenuation
  - JA: 指の圧力をハイシェルフ減衰にマップ
  - [ ] OK    Fix: 

- **`pressHeight`**
  - EN: Map finger pressure to vertical position (Z)
  - JA: 指の圧力を垂直位置（Z）にマップ
  - [ ] OK    Fix: 

- **`pressLevel`**
  - EN: Map finger pressure to output level
  - JA: 指の圧力を出力レベルにマップ
  - [ ] OK    Fix: 

- **`pressXY`**
  - EN: Map finger pressure to XY position movement
  - JA: 指の圧力をXY位置移動にマップ
  - [ ] OK    Fix: 

- **`pressXYScale`**
  - EN: Sensitivity: how far the source moves per pressure step
  - JA: 感度: 圧力ステップごとにソースが移動する距離
  - [ ] OK    Fix: 

- **`preview`**
  - EN: Preview the loaded sample
  - JA: ロードされたサンプルをプレビュー
  - [ ] OK    Fix: 

- **`renameSet`**
  - EN: Rename the active set
  - JA: アクティブなセットの名前を変更
  - [ ] OK    Fix: 

- **`setLevel`**
  - EN: Set output level in dB (0 = unity, -60 = silent)
  - JA: 出力レベルをdBで設定（0=ユニティ、-60=無音）
  - [ ] OK    Fix: 

- **`setPos`**
  - EN: Set base position in meters (X, Y, Z)
  - JA: ベース位置をメートル単位で設定（X、Y、Z）
  - [ ] OK    Fix: 

## `systemConfig.dialogs`

- **`exportSystemConfig`**
  - EN: Export System Configuration
  - JA: システム設定をエクスポート
  - [ ] OK    Fix: 

- **`importSystemConfig`**
  - EN: Import System Configuration
  - JA: システム設定をインポート
  - [ ] OK    Fix: 

- **`reduce`**
  - EN: Reduce
  - JA: 減らす
  - [ ] OK    Fix: 

- **`reduceInputChannels.message`**
  - EN: Reducing from {current} to {new} input channels will remove settings for channels {start} to {end}.\n\nThis cannot be undone.
  - JA: {current}から{new}入力チャンネルに減らすと、チャンネル{start}から{end}の設定が削除されます。\n\nこの操作は元に戻せません。
  - [ ] OK    Fix: 

- **`reduceInputChannels.title`**
  - EN: Reduce Input Channels?
  - JA: Input Channelsを減らしますか？
  - [ ] OK    Fix: 

- **`reduceOutputChannels.message`**
  - EN: Reducing from {current} to {new} output channels will remove settings for channels {start} to {end}.\n\nThis cannot be undone.
  - JA: {current}から{new}出力チャンネルに減らすと、チャンネル{start}から{end}の設定が削除されます。\n\nこの操作は元に戻せません。
  - [ ] OK    Fix: 

- **`reduceOutputChannels.title`**
  - EN: Reduce Output Channels?
  - JA: Output Channelsを減らしますか？
  - [ ] OK    Fix: 

- **`reduceReverbChannels.message`**
  - EN: Reducing from {current} to {new} reverb channels will remove settings for channels {start} to {end}.\n\nThis cannot be undone.
  - JA: {current}から{new}リバーブチャンネルに減らすと、チャンネル{start}から{end}の設定が削除されます。\n\nこの操作は元に戻せません。
  - [ ] OK    Fix: 

- **`reduceReverbChannels.title`**
  - EN: Reduce Reverb Channels?
  - JA: Reverb Channelsを減らしますか？
  - [ ] OK    Fix: 

- **`selectProjectFolder`**
  - EN: Select Project Folder
  - JA: プロジェクトフォルダを選択
  - [ ] OK    Fix: 

## `systemConfig.help`

- **`algorithm`**
  - EN: Select the rendering algorithm from the menu.
  - JA: メニューからレンダリングアルゴリズムを選択します。
  - [ ] OK    Fix: 

- **`audioPatch`**
  - EN: Opens the Audio Interface and Patching Window.
  - JA: オーディオインターフェースとパッチングウィンドウを開きます。
  - [ ] OK    Fix: 

- **`binauralAngle`**
  - EN: Horizontal rotation of binaural listener view (degrees, 0 = facing stage).
  - JA: バイノーラルリスナーの視点の水平回転（度、0=ステージ正面）。
  - [ ] OK    Fix: 

- **`binauralAtten`**
  - EN: Overall level offset for binaural output (dB).
  - JA: バイノーラル出力の全体レベルオフセット（dB）。
  - [ ] OK    Fix: 

- **`binauralDelay`**
  - EN: Additional delay for binaural output (milliseconds).
  - JA: バイノーラル出力の追加ディレイ（ミリ秒）。
  - [ ] OK    Fix: 

- **`binauralDistance`**
  - EN: Distance of binaural listener from stage origin (meters).
  - JA: バイノーラルリスナーのステージ原点からの距離（メートル）。
  - [ ] OK    Fix: 

- **`binauralEnable`**
  - EN: Enable or disable the binaural renderer processing.
  - JA: バイノーラルレンダラー処理を有効または無効にします。
  - [ ] OK    Fix: 

- **`binauralOutput`**
  - EN: Select output channel pair for binaural monitoring. Off disables binaural output.
  - JA: バイノーラルモニタリング用の出力チャンネルペアを選択。Offはバイノーラル出力を無効にします。
  - [ ] OK    Fix: 

- **`clearSolo`**
  - EN: Clear all input solo states.
  - JA: Clear all input solo states.
  - [ ] OK    Fix: 

- **`colorScheme`**
  - EN: Select the color scheme: Default (dark gray), Black (pure black for OLED displays), or Light (daytime use).
  - JA: カラースキームを選択：デフォルト（ダークグレー）、ブラック（OLEDディスプレイ用の純粋な黒）、またはライト（日中使用）。
  - [ ] OK    Fix: 

- **`copySystemInfo`**
  - EN: Copy detailed system information to the clipboard for support requests.
  - JA: サポートリクエスト用に詳細なシステム情報をクリップボードにコピー。
  - [ ] OK    Fix: 

- **`diagnosticsToggle`**
  - EN: Long-press to show or hide the diagnostic tools (export logs, open log folder, copy system info, report issue).
  - JA: 長押しで診断ツールの表示/非表示を切り替えます（ログのエクスポート、ログフォルダーを開く、システム情報のコピー、問題の報告）。
  - [ ] OK    Fix: 

- **`dialsAndButtons`**
  - EN: Select the hardware controller for dials and buttons: Stream Deck+.
  - JA: ダイヤルとボタン用のハードウェアコントローラーを選択: Stream Deck+。
  - [ ] OK    Fix: 

- **`domeElevation`**
  - EN: Dome elevation angle: 180 = hemisphere, 360 = full sphere.
  - JA: ドームの仰角：180 = 半球、360 = 完全な球。
  - [ ] OK    Fix: 

- **`exportLogs`**
  - EN: Export diagnostic logs to a zip file for debugging or support.
  - JA: デバッグまたはサポート用に診断ログをzipファイルにエクスポート。
  - [ ] OK    Fix: 

- **`exportSystem`**
  - EN: Export System Configuration to file (with file explorer window).
  - JA: システム設定をファイルにエクスポートします（ファイルエクスプローラー使用）。
  - [ ] OK    Fix: 

- **`haasEffect`**
  - EN: Hass Effect to apply to the system. Will take into account the Latency Compensations (System, Input and Output).
  - JA: システムに適用するハース効果。遅延補正（システム、入力、出力）を考慮します。
  - [ ] OK    Fix: 

- **`importSystem`**
  - EN: Import System Configuration from file (with file explorer window).
  - JA: ファイルからシステム設定をインポートします（ファイルエクスプローラー使用）。
  - [ ] OK    Fix: 

- **`inputChannels`**
  - EN: Number of Input Channels.
  - JA: Input Channels数。
  - [ ] OK    Fix: 

- **`language`**
  - EN: Select the user interface language. Changes take full effect after restarting the application.
  - JA: ユーザーインターフェースの言語を選択します。変更はアプリケーションの再起動後に完全に有効になります。
  - [ ] OK    Fix: 

- **`levelMeter`**
  - EN: Opens the Level Meter Window.
  - JA: Opens the Level Meter Window.
  - [ ] OK    Fix: 

- **`lightpadSetup`**
  - EN: Display the connected Roli Lightpads and allow for splitting them in 4 smaller pads.
  - JA: 接続されたRoli Lightpadを表示し、4つの小さなパッドに分割できます。
  - [ ] OK    Fix: 

- **`masterLevel`**
  - EN: Master Level (affects all outputs).
  - JA: Master Level（すべての出力に影響）。
  - [ ] OK    Fix: 

- **`openLogFolder`**
  - EN: Open the application's log folder in the system file explorer.
  - JA: アプリケーションのログフォルダーをシステムファイルエクスプローラーで開きます。
  - [ ] OK    Fix: 

- **`originCenter`**
  - EN: Set origin to center of stage volume. Typical for Spherical Dome setups.
  - JA: 原点をステージボリュームの中央に設定します。球形ドーム設定に一般的です。
  - [ ] OK    Fix: 

- **`originCenterGround`**
  - EN: Set origin to center of stage at ground level. Typical for Surround or Cylindrical setups.
  - JA: 原点をステージ中央の地面レベルに設定します。サラウンドまたはシリンドリカル設定に一般的です。
  - [ ] OK    Fix: 

- **`originDepth`**
  - EN: Origin Y offset from stage center (0 = centered, negative = front/downstage).
  - JA: ステージ中心からの原点Yオフセット（0 = 中央、負 = 前方/ダウンステージ）。
  - [ ] OK    Fix: 

- **`originDismissInputs`**
  - EN: Long press to dismiss and keep current input positions.
  - JA: 長押しで現在の入力位置を維持します。
  - [ ] OK    Fix: 

- **`originDismissOutputs`**
  - EN: Long press to dismiss and keep current output positions.
  - JA: 長押しで現在の出力位置を維持します。
  - [ ] OK    Fix: 

- **`originDismissReverbs`**
  - EN: Long press to dismiss and keep current reverb positions.
  - JA: 長押しで現在のリバーブ位置を維持します。
  - [ ] OK    Fix: 

- **`originFront`**
  - EN: Set origin to front center of stage. Typical for frontal stages.
  - JA: 原点をステージ前方中央に設定します。フロンタルステージに一般的です。
  - [ ] OK    Fix: 

- **`originHeight`**
  - EN: Origin Z offset from floor (0 = floor level, positive = above floor).
  - JA: 床からの原点Zオフセット（0 = 床レベル、正 = 床上）。
  - [ ] OK    Fix: 

- **`originShiftInputs`**
  - EN: Long press to shift all input positions by the origin change.
  - JA: 長押しで原点の変更に応じてすべての入力位置をシフトします。
  - [ ] OK    Fix: 

- **`originShiftOutputs`**
  - EN: Long press to shift all output positions by the origin change.
  - JA: 長押しで原点の変更に応じてすべての出力位置をシフトします。
  - [ ] OK    Fix: 

- **`originShiftReverbs`**
  - EN: Long press to shift all reverb positions by the origin change.
  - JA: 長押しで原点の変更に応じてすべてのリバーブ位置をシフトします。
  - [ ] OK    Fix: 

- **`originWidth`**
  - EN: Origin X offset from stage center (0 = centered, negative = left).
  - JA: ステージ中心からの原点Xオフセット（0 = 中央、負 = 左）。
  - [ ] OK    Fix: 

- **`outputChannels`**
  - EN: Number of Output Channels.
  - JA: Output Channels数。
  - [ ] OK    Fix: 

- **`positionControl`**
  - EN: Select the hardware controller for position control: Space Mouse, Joystick, or Game Pad.
  - JA: 位置コントロール用のハードウェアコントローラーを選択: Space Mouse、ジョイスティック、またはゲームパッド。
  - [ ] OK    Fix: 

- **`processing`**
  - EN: Lock all I/O parameters and start the DSP. Long press to stop the DSP.
  - JA: すべてのI/Oパラメータをロックし、DSPを開始します。長押しでDSPを停止します。
  - [ ] OK    Fix: 

- **`quickLongPress`**
  - EN: Long press duration. Instead of confirmation windows this software uses long presses.
  - JA: 長押しの持続時間。確認ウィンドウの代わりにこのソフトウェアは長押しを使用します。
  - [ ] OK    Fix: 

- **`reloadComplete`**
  - EN: Reload Complete Configuration from files.
  - JA: ファイルから完全な設定を再読込します。
  - [ ] OK    Fix: 

- **`reloadCompleteBackup`**
  - EN: Reload Complete Configuration from backup files.
  - JA: バックアップファイルから完全な設定を再読込します。
  - [ ] OK    Fix: 

- **`reloadSystem`**
  - EN: Reload System Configuration from file.
  - JA: ファイルからシステム設定を再読込します。
  - [ ] OK    Fix: 

- **`reloadSystemBackup`**
  - EN: Reload System Configuration from backup file.
  - JA: バックアップファイルからシステム設定を再読込します。
  - [ ] OK    Fix: 

- **`remoteSetup`**
  - EN: Select the number of pads in the Remote's XY Pads tab.
  - JA: RemoteのXY Padsタブのパッド数を選択します。
  - [ ] OK    Fix: 

- **`reportIssue`**
  - EN: Open the WFS-DIY GitHub issues page in your default browser.
  - JA: WFS-DIYのGitHub Issuesページをデフォルトブラウザーで開きます。
  - [ ] OK    Fix: 

- **`reverbChannels`**
  - EN: Number of Reverb Channels.
  - JA: Reverb Channels数。
  - [ ] OK    Fix: 

- **`sampler`**
  - EN: Enable or disable the Sampler feature for input channels.
  - JA: 入力チャンネルのSampler機能を有効または無効にします。コントローラーを選択: LightpadまたはRemote。
  - [ ] OK    Fix: 

- **`screenReader`**
  - EN: Enable or disable screen reader announcements. When enabled, parameter names and values are announced on hover, and help text is read after a few seconds.
  - JA: スクリーンリーダーのアナウンスを有効または無効にします。有効にすると、パラメータ名と値がホバー時にアナウンスされ、数秒後にヘルプテキストが読み上げられます。
  - [ ] OK    Fix: 

- **`selectProjectFolder`**
  - EN: Select the Location of the Current Project Folder where to store files.
  - JA: ファイルを保存する現在のプロジェクトフォルダの場所を選択します。
  - [ ] OK    Fix: 

- **`showLocation`**
  - EN: Location of the current show.
  - JA: 現在のショーの場所。
  - [ ] OK    Fix: 

- **`showName`**
  - EN: Name of the current show.
  - JA: 現在のショーの名前。
  - [ ] OK    Fix: 

- **`soloMode`**
  - EN: Single: one input at a time. Multi: multiple inputs simultaneously.
  - JA: 単一: 一度に1つの入力。複数: 複数の入力を同時に。
  - [ ] OK    Fix: 

- **`speedOfSound`**
  - EN: Speed of Sound (related to the temperature).
  - JA: Speed of Sound（温度に関連）。
  - [ ] OK    Fix: 

- **`stageDepth`**
  - EN: Depth of the stage in meters (Box shape only).
  - JA: ステージの奥行（メートル）（ボックス形状のみ）。
  - [ ] OK    Fix: 

- **`stageDiameter`**
  - EN: Diameter of the stage in meters (Cylinder and Dome shapes).
  - JA: ステージの直径（メートル）（シリンダーおよびドーム形状）。
  - [ ] OK    Fix: 

- **`stageDismiss`**
  - EN: Long press to dismiss and keep current input positions.
  - JA: 長押しで現在の入力位置を維持します。
  - [ ] OK    Fix: 

- **`stageFit`**
  - EN: Long press to move outlier inputs back within the new stage bounds.
  - JA: 長押しで範囲外の入力を新しいステージ境界内に移動します。
  - [ ] OK    Fix: 

- **`stageHeight`**
  - EN: Height of the stage in meters (Box and Cylinder shapes).
  - JA: ステージの高さ（メートル）（ボックスおよびシリンダー形状）。
  - [ ] OK    Fix: 

- **`stageScale`**
  - EN: Long press to scale all input positions proportionally to the new stage dimensions.
  - JA: 長押しで全入力位置を新しいステージ寸法に比例してスケーリングします。
  - [ ] OK    Fix: 

- **`stageShape`**
  - EN: Select the shape of the stage (Box, Cylinder, or Dome).
  - JA: ステージの形状を選択します（ボックス、シリンダー、またはドーム）。
  - [ ] OK    Fix: 

- **`stageWidth`**
  - EN: Width of the stage in meters (Box shape only).
  - JA: ステージの幅（メートル）（ボックス形状のみ）。
  - [ ] OK    Fix: 

- **`storeComplete`**
  - EN: Store Complete Configuration to files (with backup).
  - JA: 完全な設定をファイルに保存します（バックアップ付き）。
  - [ ] OK    Fix: 

- **`storeSystem`**
  - EN: Store System Configuration to file (with backup).
  - JA: システム設定をファイルに保存します（バックアップ付き）。
  - [ ] OK    Fix: 

- **`systemLatency`**
  - EN: Total latency of the system (Mixing board & Computer) / Specific Input and Output Latency/Delay can be set in the respective Input and Output settings.
  - JA: システムの総遅延（ミキシングボードとコンピュータ）/ 特定の入出力遅延はそれぞれの設定で設定できます。
  - [ ] OK    Fix: 

- **`temperature`**
  - EN: Temperature (gives the Speed of Sound).
  - JA: 温度（Speed of Soundを決定）。
  - [ ] OK    Fix: 

## `systemConfig.messages`

- **`configLoaded`**
  - EN: Complete configuration loaded.
  - JA: 完全な設定を読み込みました。
  - [ ] OK    Fix: 

- **`configLoadedFromBackup`**
  - EN: Configuration loaded from backup.
  - JA: バックアップから設定を読み込みました。
  - [ ] OK    Fix: 

- **`configSaved`**
  - EN: Complete configuration saved.
  - JA: 完全な設定を保存しました。
  - [ ] OK    Fix: 

- **`error`**
  - EN: Error: {error}
  - JA: エラー: {error}
  - [ ] OK    Fix: 

- **`languageChanged`**
  - EN: Language changed to: {language} (requires restart for full effect)
  - JA: 言語を変更しました: {language}（完全に有効にするには再起動が必要です）
  - [ ] OK    Fix: 

- **`logDirNotFound`**
  - EN: Log directory not found
  - JA: ログディレクトリが見つかりません
  - [ ] OK    Fix: 

- **`logsExportFailed`**
  - EN: Failed to export logs
  - JA: ログのエクスポートに失敗しました
  - [ ] OK    Fix: 

- **`logsExported`**
  - EN: Logs exported to {path}
  - JA: ログを {path} にエクスポートしました
  - [ ] OK    Fix: 

- **`noBackupFilesFound`**
  - EN: No backup files found.
  - JA: バックアップファイルが見つかりません。
  - [ ] OK    Fix: 

- **`partialLoad`**
  - EN: Partial load: {error}
  - JA: 部分的な読み込み: {error}
  - [ ] OK    Fix: 

- **`partialLoadFromBackup`**
  - EN: Partial load from backup: {error}
  - JA: バックアップからの部分的な読み込み: {error}
  - [ ] OK    Fix: 

- **`restartBody`**
  - EN: Please restart the application for the language change to take full effect.
  - JA: 言語の変更を完全に反映するには、アプリケーションを再起動してください。
  - [ ] OK    Fix: 

- **`restartTitle`**
  - EN: Restart Required
  - JA: 再起動が必要です
  - [ ] OK    Fix: 

- **`selectFolderFirst`**
  - EN: Please select a project folder first.
  - JA: 最初にプロジェクトフォルダを選択してください。
  - [ ] OK    Fix: 

- **`selectLogExportFolder`**
  - EN: Select destination for log export
  - JA: ログのエクスポート先を選択してください
  - [ ] OK    Fix: 

- **`systemConfigExported`**
  - EN: System configuration exported.
  - JA: システム設定をエクスポートしました。
  - [ ] OK    Fix: 

- **`systemConfigFileNotFound`**
  - EN: System configuration file not found.
  - JA: システム設定ファイルが見つかりません。
  - [ ] OK    Fix: 

- **`systemConfigImported`**
  - EN: System configuration imported.
  - JA: システム設定をインポートしました。
  - [ ] OK    Fix: 

- **`systemConfigLoaded`**
  - EN: System configuration loaded.
  - JA: システム設定を読み込みました。
  - [ ] OK    Fix: 

- **`systemConfigLoadedFromBackup`**
  - EN: System configuration loaded from backup.
  - JA: バックアップからシステム設定を読み込みました。
  - [ ] OK    Fix: 

- **`systemConfigSaved`**
  - EN: System configuration saved.
  - JA: システム設定を保存しました。
  - [ ] OK    Fix: 

- **`systemInfoCopied`**
  - EN: System info copied to clipboard
  - JA: システム情報をクリップボードにコピーしました
  - [ ] OK    Fix:
