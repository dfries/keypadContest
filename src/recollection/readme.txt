This program is my entry in the keypad programming contest.  It is a 
memory game, similar to the 1970's electronic game "Computer Perfection" by 
Lakeside.  

When the game starts up, it will show a "menu" of sorts.  The player must press 
one of buttons 1 through 4 to select the game mode.  The original game had 4 
modes: two single-player modes and two two-player modes.  Currently, I've 
implemented only one of the single-player modes, and it is selected via button 
1.  After the mode is selected, the player must select the difficulty via 
buttons 6 through 8.

The goal of the single-player game is to press the buttons in the correct order.
The order changes from round to round, and is revealed to the player at the 
beginning of each round during a "preview" mode.  When in preview mode, the 
user presses each button once (starting at button 1, then 2, and so on) and the 
game will light up the button that activates the corresponding light.  Once 
the preview is finished, gameplay begins.  The player must then press the 
button that activates light 1, then the button that activates light 2, and so 
forth.  Once all the lights are lit, the gameplay ends and the player is shown 
their score.  The score is determined by how many button presses were required 
to solve the game, and the lowest score wins.

