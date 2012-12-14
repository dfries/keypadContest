This is the example firmware that Nathan demonstrated running on his keypad
 at the conclusion of the geek speak.  It takes care of nearly all of the
 low-level stuff so that you could probably make a game by merely re-writing
 task_1_game_action() and task_3_play_song() (or leave it out altogether if
 you're not going to have music and a way to play it).  See the source file's
 comments for its documentation.

The Makefile in this directory handles the EEPROM stuff, unlike the original
 release in the top-level directory.  I don't have a USBtinyISP, so I can't be
 sure that I set up the Makefile correctly.  Please fix it if there's an issue.
