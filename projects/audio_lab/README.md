# Judas audio listening lab

Ordinary project for M60 streaming, Doppler, resident geometry obstruction, environmental reverb and routing. Open `audio_lab.judasproj` in the editor and Play; export through the normal project action. Safe initial gains are deliberately modest. Original sounds are CC0, synthesized by `tools/CreateAudioAssets.py`; music is an actual 180-second FLAC file.

## Controls

- WASD / left stick moves; mouse looks.
- Esc opens controls and releases pointer capture. Resume recaptures it.
- P toggles moving orange tone Doppler; T stops/restarts that tone for isolated comparisons.
- E opens/closes the real hinged door in the left wall. Green source behind it is streamed pink noise with obstruction gain/filter.
- B toggles actual obstruction/environment bypass for the noise and impulse source.
- Space plays an independent short impulse through the shared environment processor.
- R reloads a clean world.

The blue floor at x=-8,z=-5 is a long hall setting. Orange floor at x=8,z=-5 is a smaller damped booth. Outside both zones is dry. Walk close to the wall/door and compare spectra at the same distance. Stop the moving tone to hear the noise/reverb separately. Menu buttons pause/resume, seek and toggle music looping, pause the effects group, compare effects and control music gain. Effects-group pause is independent of music and UI sounds. Opening the menu alone does not prescribe a global audio pause.

The on-screen diagnostics show media time, readiness, PCM allocation, underruns, rays, retirements and processor count. They do not prove auditory quality. [Public API](../../docs/judasjs/audio.md).

## Streamed Spring Range

`../streamed_range/streamed_range.judasproj` retains one root music stream, one ambience stream per loaded gallery and alternating authored environments. Q carries/adopts an ordinary sounding prop; its voice survives birthplace unload. Walk outward and all the way back. F9/F10 retain/release manual gallery demands; existing locale/pause/shot controls remain.
