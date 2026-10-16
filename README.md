# Putt Pop — Arcade Mini Golf

The new **Unreal project source and detailed Blender landscape** are in
`unreal/PuttPop`. See its README and `PuttPop-Unreal.code-workspace` for editing.
Unreal is not installed yet, so this edition still needs compilation and an
engine play-test. After installing Unreal and the C++ tools, use
`Prepare Unreal Project.cmd`. Its preview is a Blender render, not Unreal footage.

The **Unity edition** is in `unity/PuttPopHighlands`. Double-click
`Open Unity Game.cmd` to open it, or add that project folder in Unity Hub.
Open `Assets/PuttPop/Scenes/Highlands.unity` and press Play. Its README covers
editing, rebuilding the Blender assets, controls, and Windows builds.
The Unity edition adds an editable mountain basin, pine forest, rocky shores,
a reflective lake, waterfall mist and the close overhead camera.

The browser edition below remains available.

A 3D arcade mini golf game with six elevated Blender-built courses and a close angled overhead camera. Aurora Highlands adds floating islands, waterfalls, a mountain backdrop, a cratered moon, an aurora sky, and animated lake water. Includes PBR turf and brass, modeled golf ball dimples, reflections, bloom, bank shots, bumpers, a rotating gate, sound effects, and a local personal best.

## Open in VS Code

Open this folder, then run in the terminal:

```sh
npm run dev
```

Visit **http://127.0.0.1:4187**. Node.js 20 or newer is recommended. No installation or build step is required. The Three.js runtime is included in `dist/vendor` with its license. Fonts use Google Fonts with local fallbacks.

## Controls

- Mouse/touch: drag backwards to choose your shot direction and power, then release to putt.
- Keyboard: Left/Right to aim; hold Space to build power, release to putt. Escape cancels a shot.
- Full course zooms out for planning. Closer view restores the medium overhead camera. Both views use the same controls.
- The default camera looks down at the course at an angle and pans toward the active ball. Use the + and minus buttons to zoom in or out. It is closer than the full map. Reduced motion settings make repositioning immediate.
- Uphill ramps slow the ball; downhill ramps accelerate it. Weak uphill putts can roll back. All tees and cups sit on flat sections.
- Retry hole resets the current hole and its strokes. Start a fresh round resets all six holes.
- Personal best is stored in this browser only. The game remains playable when browser storage is unavailable.

## Files

- `dist/index.html` — page and interface
- `dist/style.css` — responsive visual design
- `dist/game.js` — Three.js scene, input, audio, scorecard
- `dist/physics.js` — courses and fixed-step golf physics
- `dist/graphics.js` — GLB loading, HDR reflections, bloom, and tone mapping
- `dist/terrain.js` — terrain subdivision for the playable fallback environment
- `dist/backdrop.js` — aurora sky and animated lake
- `dist/assets/courses/` — six Blender-exported GLB environments with embedded textures
- `dist/assets/golf-ball.glb` — golf ball with 330 modeled dimples
- `art/blender/Putt-Pop-Aurora-Highlands-Overhead.blend` — current Blender source with raised courses, backdrop, and the overhead camera
- `art/blender/Putt-Pop-Botanical-Courses.blend` — retained flat source used to regenerate the assets
- `art/blender/highlands-overhead-preview.png` — Blender Cycles preview of the opening course in the new camera
- `art/blender/highlands-finale-preview.png` — Blender Cycles preview of the three-tier finale
- `art/blender/highlands-report.json` — elevation profiles and export summary
- `art/blender/asset-report.json` — verified export counts and texture details
- `tests/` — physics and interface logic checks; run `npm test`
- `.openai/hosting.json` — Sites hosting configuration

The entire `dist` folder can be served by a static web host. Use a local HTTP server rather than opening `index.html` as a file because the game uses JavaScript modules.

The 22 automated tests exercise the actual scene graph and game logic with a simulated DOM and renderer. They parse every real GLB, check exported terrain against the physics heights, and verify uphill rollback, downhill acceleration, collider alignment, gate animation, texture declarations, model installation, camera framing, all six holes, controls, scoring, and storage failure. Tests stub image decoding and do not verify browser WebGL rendering, postprocessing, or real browser input.

## Blender source

Assets were modeled and exported using Blender 5.2.2 LTS. Open the current Overhead `.blend` file to inspect or edit them. The opening course is visible by default; enable another Course collection and hide the current one to edit a different hole.

To regenerate the art, run Blender in background mode with these Python scripts in order:

1. `art/blender/build_garden.py` — models and exports all six courses.
2. `art/blender/polish_assets.py` — builds the dimpled ball and renders the preview.
3. `art/blender/refresh_packed_textures.py` — refreshes embedded texture bytes and export reports.
4. `art/blender/build_highlands.py` — elevates all six courses from the shared profiles and authors the sky and distant scenery.
5. `art/blender/frame_overhead.py` — frames the new overhead view, brings floating islands into view, and merges backdrop meshes.

Keep `art/blender/courses.json` consistent with `dist/physics.js` when changing layouts or elevation profiles. The same smooth height function drives gravity, ball height, camera height, the aiming guide, and Blender terrain. GLBs load after an elevated fallback course initializes. If an asset cannot load, that basic course remains playable. Models are cached in memory and static details are merged by material to limit draw calls.
