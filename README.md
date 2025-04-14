
# Snake SDL Game

# Description
Classic snake game using C Language with SDL Library.
Video gameplay: https://youtu.be/mPsN94Pp4lU?si=g5q-8jNfGEfCa8SD

## Features

- **Basic Snake Movement:** Use arrow keys in order to control the Snake movement.
- **Auto Border Detection:** When Snake reaches the border, automatically turns right if possible (otherwise, to the left).
- **Snake Collision:** When Snake collides with its own body - the game ends.
- **New Game and Exit:** Use ESC key to quit the game, or start a new one using 'n' key.
- **Total Game Elapsed Time:** The clock is located in the top right corner of screen.
- **Points system:** Whenever Snake eats the dot (blue/red), player earns points.
- **Blue Dot:** Whenever the snake eats the dot, it gets one unit longer and a new random dot appears on the board.
- **Speed Up:** After a fixed time interval, the game speeds up by a certain factor.
- **Red Dot:** At random times, a special red dot appears on the screen for a fixed time interval, which is determined by a Progress Bar in the top right corner of the screen. When Snake eats the red dot, randomly one of two things happen: either the Snake shortens by a unit or slows down its movement.
- **Save and Load Game Mechanism:** You can save the current state of the board(snake position, snake length, timer, points etc.) using the 's' key. Load the game at any time using 'l' key.
- **Best Scores Leaderboard:** Top three scores are being displayed everytime the game ends. If your new score meets that criteria, you can enter your nickname and appear in the appropriate place at the Leaderboard.
- **Easy to customize:** Most parameters of the game (like speed-up interval, speed-up/slow-down rate, red dot lifespan, points per dot etc.)can be modified by changing constants at the beginning of the Code.

## Compilation

In order to compile and run the game, use the start.sh bash script, by inputing following line in Linux Terminal:

```bash
./start.sh
```

## How to play

- Control Snake movement by arrow keys.
- Eat dots to grow longer and gain points.
- To save the game use 's' key and 'l' key to load.
- To exit game use ESC Key, to start new one press 'n'.