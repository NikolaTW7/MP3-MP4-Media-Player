# Manual test checklist

Run against a Release x64 build at 150% Windows scaling.

## Playback
- [ ] Add files; the first one starts playing
- [ ] Double-click a playlist item plays it
- [ ] Pause, Stop, -10s / +10s, seek slider, volume slider
- [ ] Track button cycles audio tracks
- [ ] Vocals button reduces vocals and restores normal audio

## Playlist
- [ ] Previous / Next, greyed out at the ends
- [ ] Autoplay moves to the next item; stops after the last one
- [ ] Stop near the end of a video does not autoplay
- [ ] Missing or broken files are skipped without looping
- [ ] Repeat Off / All / One
- [ ] Shuffle: each item once per cycle; new cycle with Repeat All

## Full screen
- [ ] F / Full Screen button enter; Esc, F and Exit button leave
- [ ] Aspect ratio kept with black bars
- [ ] Control bar and pointer hide after about 3 s and return on mouse movement
- [ ] Bar stays visible while clicking or dragging its controls
- [ ] Progress and volume sliders, time display, toggle states match the normal window
- [ ] Window size, position and layout restored after leaving

## Keyboard (normal and full screen)
- [ ] Space, Left/Right, N, P, F

## Layout and DPI
- [ ] Text sharp at 150%
- [ ] Maximise and restore resize video, playlist and controls
