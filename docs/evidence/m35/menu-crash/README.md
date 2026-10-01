# Options crash follow-up

Operator crash: `Renderer::MeasureUIText` reached through `UIMenuScreen::Draw` after clicking Options.
Root cause: `InteractivePlay::Begin` assigned `PauseMenu()` into its member. The temporary's callbacks captured its `this`, leaving dangling callbacks after assignment. Options pushed a destroyed member screen.

Repair: `PauseMenu::Reset` clears state in place; copy/move are prohibited because callbacks and navigation pointers bind to their owner. No input mapping algorithm changes.

Incremental Release build: judas, judas_ui_tests, judas_project_application_tests, judas_input_tests. Build log in build.log.
Executed: UI tests PASS (including repeated reset/Options/toggle/Quit ownership); actual application/GL test 29 checks / 0 failures (mouse-click Options, draw nested screen, toggle, Back, Resume); input tests 40 checks / 0 failures. SDL offscreen, dummy audio, software GL for application/input checks.

No repeated full production run. The earlier clean build and 64-suite run describe the pre-fix candidate, whose fingerprints are preserved in PRE_FIX_SOURCE_SHA256.json and final/. Current source fingerprints are SOURCE_SHA256.json. Protected evidence and prototypes unchanged.
