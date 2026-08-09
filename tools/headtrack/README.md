# wfs_headtrack — webcam head-tracking plugin

Webcam-based head tracking for the binaural renderer, so head-tracked
binaural can be used (and tested) before dedicated IMU tracker hardware
exists. Same architecture as the GPU vendor plugins: a standalone shared
library the app loads at runtime through a **versioned C ABI**
(`spatcore/binaural/plugin/HeadTrackPluginApi.h`) — OpenCV and the models
never touch the app build.

## What's inside

| Stage | Implementation |
|---|---|
| Capture | `cv::VideoCapture` (DirectShow→MSMF on Windows, AVFoundation, V4L2), 640×480 MJPG @ 60 fps request, reader thread + latest-frame mailbox (never queues — old frames are dropped, not delayed) |
| Face detection | YuNet (`models/face_detection_yunet_2023mar.onnx`, OpenCV Zoo, MIT, ~230 KB) — box + 5 landmarks |
| Head angles | Geometric, from the three robust landmarks only: roll = eye-line slope, yaw = nose offset between the eyes (de-rolled), pitch = nose drop below the eye line. Axes decoupled by construction. |
| Output | Raw yaw/pitch/roll per frame via callback; smoothing (1-Euro) and Set-Zero calibration live app-side so they tune without rebuilding the plugin |

The mouth corners are deliberately unused (unreliable under beards — on a
bearded face a 5-point solvePnP read nods as roll), and eyes+nose are
untouched by glasses, facial hair, or **headphones**. Absolute gain depends
mildly on the wearer's nose protrusion and the frontal pose carries a constant
bias — both harmless: Set Zero removes the bias and the renderer needs
relative motion, not absolute angles. A dense-mesh model (MediaPipe FaceMesh
via ONNX, or real MediaPipe C++ for whoever wants to fight Bazel) can replace
the internals later **without any app change** — that is what the ABI is for.

## Building

Windows (downloads + caches the official OpenCV prebuilt on first run, ~180 MB):

    tools\headtrack\build-headtrack-plugin.ps1            # Release, stages next to the Release app
    tools\headtrack\build-headtrack-plugin.ps1 -Config Release -StageDir <appDir>

Linux / macOS (system OpenCV):

    sudo apt install libopencv-dev        # or: brew install opencv
    bash tools/headtrack/build-headtrack-plugin.sh Release

Staged files (all must sit next to the app executable):
`wfs_headtrack.dll` (+ `opencv_world4100.dll`, `opencv_videoio_msmf4100_64.dll`
on Windows) and `face_detection_yunet_2023mar.onnx`.
The ffmpeg videoio DLL is for video *files*, not cameras, and is deliberately
not shipped.

## Testing / validating angle signs

    tools\headtrack\build\Release\test-headtrack-plugin.exe [seconds] [cameraIndex]

The smoke host dlopens the plugin exactly like the app (no OpenCV link) and
prints the pose stream. Sit in front of the camera and check:

- shake "no" → **yaw positive when turning to YOUR right**
- nod "yes" → **pitch positive looking up**
- tilt → **roll positive when your right ear drops**

If a camera/driver mirrors, flip the `kSign*` constants in
`src/FaceTracker.h` and rebuild.

## In the app

System Config → Binaural Renderer → render mode Structural HRTF or SOFA →
Head Tracking → **Webcam**. The camera opens on selection (reason logged on
failure; falls back to manual). Look at the stage center and press
**Set Zero**. Camera index is machine-local: `headtrackCameraIndex` in
`WFS-DIY.settings`.

Latency ≈ 30–50 ms motion-to-ear (camera exposure + inference + render block)
— clearly above a dedicated IMU (~5–15 ms) but well inside what feels anchored
for validation and demo use.

## Licenses

- YuNet model: MIT (OpenCV Zoo, `face_detection_yunet_2023mar.onnx`,
  sha256 `8f2383e4…52fa4`)
- OpenCV: Apache-2.0 (runtime DLL redistributed on Windows)
