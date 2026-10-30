# Changelog

## 1.0.0 — release candidate for R36S

- Added seven selectable color themes with persistent settings.
- Added an indeterminate media scanning indicator and file count.
- Improved radio catalog refresh and moved stream changes off the UI thread;
  a test on R36S was reported successful, but indefinite blocking remains possible.
- Added user radio playlists and seven supplied station examples.
- Updated documentation, demo screenshots and the ARM64 build artifact name.
- Automatic updates are not implemented. The ARM64 1.0.0 build, themes and
  loading indicator still require device validation.

## Android 0.1.1-beta — GitHub Actions

- Added automatic Android APK compilation on GitHub Actions.
- Added manual builds, builds on `main`/`master` and tagged Release uploads.
- The generated artifact is named `KPlayer-Android-Beta.apk`.
- Updated the workflow to Java 17 and current official GitHub/Gradle actions.

## Android 0.1.0-beta — source

- Added a separate Java Android Studio project using libVLC Android.
- Added responsive portrait and landscape layouts.
- Added first-run languages and Terms of use.
- Added MediaStore scanning and Android folder selection.
- Added local music, video and Radio Browser playback screens.
- Included local Android Studio compilation as an alternative.

## v0.7.1-beta

- Reworked the audio information card with format, size, duration, sample rate,
  channel count and bitrate obtained from the file and libVLC tracks.
- Changed the lower audio card to show the next queued track instead of
  duplicating the current song and cover.
- Redesigned Online Radio with a dedicated player on the left and a selectable
  station list on the right.
- Added refreshed native screenshots for both redesigned players.

## v0.7.0-beta

- Added Online Radio powered by the open Radio Browser directory.
- Added controller-friendly Brazil, Popular, Pop, Rock and News filters.
- Added asynchronous station loading with server failover and bounded parsing.
- Added direct radio stream playback through libVLC using resolved station URLs.
- Added station name, genre, country, codec and bitrate to the native radio UI.
- Expanded the dashboard to include a dedicated Online Radio card.

## v0.6.3-beta

- Combined the duplicated music control rows into one larger bottom bar, with playback shown only once and named shortcuts beside graphical button markers.
- Added asynchronous IP-based city/state and time-zone detection through ip-api.com.
- Added automatic fallback to the console clock whenever the location request is unavailable.
- Added bounded HTTP and JSON handling without API keys or extra runtime libraries.

## v0.6.1-beta

- Replaced the music player's text-heavy bottom shortcuts with native SDL icons.
- Added compact controller-button markers for A, B, X, Y, START, L1/R1 and SELECT.
- Added dedicated lyrics, queue, folder, back, play/pause, track navigation and settings symbols.

## v0.6.0-beta

- Added recursive automatic media discovery in `midias`, `/roms` and `/roms2`, with a 50 KB minimum file size and a 4096-item bound.
- Swapped the console actions: B confirms/opens and A goes back/cancels.
- Changed SELECT to open or close Settings and added an explicit “Exit KPlayer” action there.
- Removed software volume settings, shortcuts, UI labels and libVLC volume calls; volume is left to the console firmware.
- Preserved manual folder browsing, local metadata, lyrics, queue/folder panes and the three-column music transport.

## v0.5.1-beta

- Added the requested three-column bottom music bar: mini cover and metadata, geometric previous/play/next controls, white play/pause circle, progress handle, format badge and actual file size.
- Added file sizes to music browser entries using stat.
- Y opens Lyrics; X opens Queue and switches Queue/Folder. Favorite marking moved to Settings.
- Added front.jpg/front.png sidecar covers and retained synchronized/external/embedded lyrics.
- Adapted the supplied design to the existing native API, without introducing an SDL_ttf dependency.


## v0.5.0-beta — KPlayer (Beta)

- Reorganized the audio screen with an aligned cover/metadata card, switchable right pane and one transport bar.
- Added Lyrics / Queue / Folder switching with X and folder browsing without changing the current queue.
- Added bounded local ID3 metadata reading, embedded JPEG/PNG covers and embedded timed/plain lyrics.
- Preserved valid local tags when libVLC falls back to the filename.
- Updated the music controls: L2/R2 changes volume; START pauses from any pane.
- Prepared the public repository, bilingual documentation, original illustrative previews and install release asset.

**Status:** ARM64 build and local functional/rendering checks; physical R36S validation still pending. This release packages application version 0.5.0 as a beta.

## v0.4.0

- First-run welcome screen and native app identity.
- Persistent PT-BR / English / Spanish choice and translated interface.
- Usage notices accessible before starting and from settings.
- Larger interface fonts and updated layout spacing.

## v0.3.1

- Added storage selection for local media, `/roms`, `/roms2` and `/`.
- Added readable directory symlink support and unavailable-storage messages.

## v0.3.0

- Adapted the supplied simulator design to the native dashboard, audio, video and file-browser screens.
- Added external LRC lyrics, sidecar covers, metadata display and an independent playback queue.

## v0.2.0

- Native C/SDL2 frontend for the target ARM64 system.
- Rebuilt VLC 3.0.23 core and libVLC, application identity changes and Debian t64 module ABI alignment.
