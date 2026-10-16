# Putt Pop Highlands - Unreal edition

**Status: project source and Blender scenery prepared. Unreal Engine is not
installed on this computer, so the Unreal module, asset builder, map, and gameplay
have not yet been compiled or tested in Unreal.** The Blender preview is labeled
as such. The earlier Unity Windows game remains available.

## Open it

1. Install [Unreal Engine 5 through Epic Games Launcher](https://www.unrealengine.com/download).
   This project targets current UE5 APIs; an engine association is deliberately
   omitted so your installed UE5 version can register it.
2. Install Visual Studio 2022 (or its Build Tools) with the C++ game development
   tools, MSVC and a Windows SDK. Epic's
   [Visual Studio setup guide](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-visual-studio-for-unreal-engine)
   lists the compiler requirements for your engine version.
3. Double-click `Prepare Unreal Project.cmd` in the main `arcade-mini-golf`
   folder. It compiles the editor module, opens Unreal and runs the landscape
   builder. The first shader/mesh build can take a while.
4. When the Output Log says **native assets and Highlands map saved**, open
   `/Game/Highlands/Maps/Highlands` and press Play. Compilation or Python errors
   must be resolved before this step; see `Saved/Logs` and `QA`.

For an engine installed somewhere else:

```powershell
.\unreal\Prepare-Unreal.ps1 -EngineRoot "D:\Epic Games\UE_5.x"
```

Open `PuttPop-Unreal.code-workspace` from the main folder in VS Code to edit
the C++ game and Python asset builder. The `.uproject` file is here in `PuttPop`.

## Landscape

- Original 320 x 320 metre mountain basin, divided into 64 mesh tiles.
- Layered rock and meadow surfaces, snowy summits and a rippled reflective lake.
- 955 varied pine instances, with modeled branches, twigs and individual needles.
- Fern leaflets, grass blades and four displaced mossy boulder variations.
- A cedar clubhouse with standing-seam roof and warm windows.
- A 78-plank timber boardwalk and a tiered cliff waterfall with foam rings.
- Six existing detailed Blender courses, elevated putting surfaces, moving
  obstacle pivots and the original dimpled ball.

The exports contain 101 unique mesh assets and about 1.4 million unique triangles.
The full scene uses linked/instanced meshes. In Unreal, foliage uses HISM groups,
three generated mesh detail levels, and groundcover distance culling. Terrain,
boulders and pines request Nanite, with area preservation on the pine needles.
Lumen software lighting, sky capture, virtual shadows,
subtle fog and grading are configured. Performance and final appearance still
need measurement in the installed engine, particularly on the integrated GPU.

`SourceArt/Putt-Pop-Unreal-Highlands.blend` is the editable scene.
`QA/blender-landscape-preview.png` is a Blender render of this scene.
Every original mesh also has an FBX for manual reuse.

`SourceArt/Highlands_1009.r16` is an optional native Landscape heightmap. The
generated scene uses the tiled meshes, so the heightmap is not imported by default.
For a replacement sculptable Landscape use 1009 x 1009, 63 quads per section,
2 x 2 sections per component and 8 x 8 components. XY scale is approximately
31.746 cm, Z scale 19.53155 and location (-16000,-16000,3800.076) cm. The raw file
is unsigned little endian 16 bit and its rows match Unreal's XY coordinates.
Hide the mesh tiles when using this alternative to avoid overlapping surfaces.

## Play controls implemented in the source

Drag backward on the green and release to putt. Alternatively use Left/Right
arrows to turn and hold/release Space to charge a putt. Right-click cancels.
The aiming line shows direction; it is not a predicted trajectory.

- Wheel: zoom the close angled overhead camera.
- C: landscape overview / golf camera.
- R: retry the current hole; T: restart the round.
- Enter: advance after sinking; otherwise the next hole loads automatically.
- F1: lower global illumination quality; F2: high quality. Escape: quit.

The simulation uses a fixed 120 Hz step, slope gravity, rail and bumper rebounds,
rotating gates, a single-use boost strip, cup capture, stroke totals and a local
best score in `Saved/best-score.txt`. The six courses retain their earlier rules.

## Rebuilding and editing

`Content/Python/build_highlands.py` creates the native materials, meshes and map.
It calls `HighlandsAssetBuilder` to turn Blender's exact `PPM3` centimetre geometry
into real Unreal StaticMesh assets. This preserves UVs, split normals, material
slots, handedness and pivots without depending on FBX importer axis conventions.
Standard FBX copies are included alongside these buffers.

The map's generated actors are tagged `PuttPopGenerated`. Rerunning the builder
replaces those tagged actors and the generated mesh/material assets. User-added
actors without that tag remain. Keep custom material/mesh edits in separate assets
because their generated originals will be rebuilt.

The `Editable landscape preview` actor is only an editor presentation. It is
hidden and destroyed in play and excluded from cooking. The native pawn builds
the same instanced scenery from `Content/Data/highlands.json` during gameplay.

To regenerate the original artwork, run the installed Blender with:

```powershell
& "$env:LOCALAPPDATA\Microsoft\WindowsApps\blender-launcher.exe" --background --factory-startup --python "FULL_PATH\arcade-mini-golf\art\blender\build_unreal_highlands.py"
```

It regenerates `SourceArt` and its `.blend` file; save personal changes separately.
The generator uses the existing authored course scene. All scenery is procedural
Blender artwork; no Fab, Marketplace or downloaded commercial assets are required.

## Verification scope

`QA/geometry-checks.json` records the independent export checks: buffer integrity,
finite vertices, normalized normals, triangle winding, material slots, ball scale,
each course's actual ramp heights, obstacle pivots and heightmap dimensions.
These checks do not establish Unreal compilation, import success or playability.

The standalone check in `../../tests/unreal-physics.cpp` compiles the very same
`Source/PuttPop/GolfSimulation.h` used by the Unreal pawn. Its fixtures are generated
from the shipped JSON by `../../tests/generate-unreal-course-checks.py`.
An Unreal build and interactive test remain necessary once the engine is installed.
