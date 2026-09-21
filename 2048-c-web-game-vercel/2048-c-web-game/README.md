# 2048 — C Game Engine + HTML/CSS/JavaScript Frontend

A complete local 2048 web game where the **actual game engine is written in C**.

The browser is only the presentation/controller layer:

```text
Keyboard / touch
       ↓
JavaScript
       ↓
HTTP POST /api/move
       ↓
C HTTP server
       ↓
C 2048 game engine
       ↓
Board + score + status
       ↓
JSON response
       ↓
JavaScript renderer
       ↓
HTML/CSS
```

## 1. Important architecture rule

The following operations are implemented in `backend/game.c`:

- Board state
- Tile generation
- Movement
- Compression
- Tile merging
- Score calculation
- 2048 detection
- Game-over detection
- Restart/reset

`frontend/script.js` does **not** contain:

- `moveLeft()`
- `moveRight()`
- `moveUp()`
- `moveDown()`
- tile merging
- score calculation
- game-over calculation
- win calculation

It only sends a direction and renders the C server's response.

## 2. Requirements on Windows

This project is designed for Windows + VS Code + GCC.

The simplest setup is **MSYS2 UCRT64 GCC**.

Install MSYS2 from:

https://www.msys2.org/

Open the **MSYS2 UCRT64** terminal and install GCC:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc
```

Verify:

```bash
gcc --version
```

If GCC is already installed at:

```text
C:\msys64\ucrt64\bin
```

make sure that directory is in the Windows `PATH` if you want to use `gcc` from PowerShell.

## 3. Project structure

```text
2048-c-web-game/
│
├── frontend/
│   ├── index.html
│   ├── style.css
│   └── script.js
│
├── backend/
│   ├── game.h
│   ├── game.c
│   ├── server.c
│   └── test_game.c
│
└── README.md
```

## 4. Compile the C server

Open a terminal in the `backend` directory.

### MSYS2 UCRT64 terminal

```bash
cd /d/2048-c-web-game/backend
gcc -Wall -Wextra -std=c11 game.c server.c -o 2048_server.exe -lws2_32
```

The important part is:

```text
-lws2_32
```

### Why is `ws2_32` required?

Browsers communicate over HTTP/TCP. The C server needs socket functions such as:

- `socket()`
- `bind()`
- `listen()`
- `accept()`
- `recv()`
- `send()`

Windows provides these through **Winsock**, whose library is `ws2_32`.

The game rules themselves do not depend on Winsock. They remain in `game.c`.

## 5. Run the game

From the `backend` directory:

```bash
./2048_server.exe
```

You should see:

```text
2048 C server running at http://127.0.0.1:8080
Press Ctrl+C to stop the server.
```

Open:

```text
http://127.0.0.1:8080
```

The C server serves the frontend files and handles the API.

## 6. API

### Start a new game

```http
POST /api/new-game
```

Example response:

```json
{
  "board": [
    [0, 2, 0, 0],
    [0, 0, 0, 0],
    [0, 0, 4, 0],
    [0, 0, 0, 0]
  ],
  "score": 0,
  "status": "PLAYING"
}
```

### Move

```http
POST /api/move
Content-Type: application/json
```

Body:

```json
{
  "direction": "left"
}
```

Possible directions:

```text
left
right
up
down
```

Response:

```json
{
  "board": [
    [4, 8, 0, 0],
    [0, 0, 0, 0],
    [0, 0, 0, 0],
    [0, 0, 0, 0]
  ],
  "score": 12,
  "status": "PLAYING"
}
```

## 7. Standard 2048 behavior

The C engine:

1. Starts with two random tiles.
2. Uses 90% probability for a `2`.
3. Uses 10% probability for a `4`.
4. Compresses tiles toward the requested direction.
5. Merges equal adjacent tiles.
6. Prevents a newly merged tile from merging twice in the same move.
7. Adds exactly one random tile after a valid move.
8. Adds no tile after an invalid move.
9. Adds the merged value to the score.
10. Detects 2048.
11. Detects game over.
12. Resets all state on a new game.

## 8. Example of the no-double-merge rule

Input:

```text
2 2 2 0
```

Move left:

```text
4 2 0 0
```

It does **not** become:

```text
8 0 0 0
```

This is implemented in the C function `mergeRow()`.

## 9. C function responsibilities

### `initializeGame()`

Clears the board, resets score/status, and adds two starting tiles.

### `addRandomTile()`

Finds an empty cell and places either 2 or 4.

### `moveLeft()`

Moves every row left using the C compression/merge algorithm.

### `moveRight()`

Reverses each row, applies the same left algorithm, and reverses it back.

### `moveUp()`

Extracts each column, applies the left algorithm, then writes it back.

### `moveDown()`

Extracts each column in reverse order, applies the left algorithm, then writes it back.

### `checkWin()`

Looks for a tile >= 2048.

### `canMove()`

Checks for:

- an empty cell
- horizontally mergeable neighbors
- vertically mergeable neighbors

### `checkGameOver()`

Declares game over when there is no empty cell and no possible merge.

### `getScore()`

Returns the score maintained by C.

## 10. Run the C unit tests

The project contains deterministic tests so the core logic can be checked without depending on random gameplay.

From `backend`:

```bash
gcc -Wall -Wextra -std=c11 test_game.c game.c -o test_game.exe
./test_game.exe
```

Expected:

```text
All C 2048 engine tests passed.
```

The tests cover:

- preventing double merges
- score calculation
- invalid move behavior
- 2048 win detection
- game-over detection
- right movement

## 11. Why the browser does not need C compiled to WebAssembly

This implementation intentionally uses a **C HTTP server** rather than compiling the C game engine to WebAssembly.

That makes the architecture easy to demonstrate in a viva:

```text
Browser
  |
  | HTTP JSON
  v
C HTTP server
  |
  v
game.c
```

The browser never executes the game rules.

## 12. Viva explanation

### HTML

Creates:

- title
- score card
- game board
- restart button
- status area
- instructions
- win/game-over overlay

### CSS

Controls:

- responsive layout
- board grid
- tile appearance
- colors
- spacing
- rounded corners
- overlays
- mobile layout

### JavaScript

JavaScript is the controller/renderer.

It:

1. Detects keyboard/touch input.
2. Converts it into `left`, `right`, `up`, or `down`.
3. Sends the direction to the C API.
4. Receives JSON.
5. Displays the returned board.
6. Displays the returned score.
7. Displays the returned status.

### C

C is the actual game engine.

It owns:

```text
board[4][4]
score
status
```

and performs:

```text
movement
compression
merging
random tile generation
score calculation
win detection
game-over detection
reset
```

## 13. Example data flow

```text
User presses Arrow Left
        ↓
JavaScript keydown event
        ↓
direction = "left"
        ↓
POST /api/move
        ↓
C server receives JSON
        ↓
handle_move()
        ↓
moveLeft()
        ↓
C compresses tiles
        ↓
C merges equal tiles
        ↓
C updates score
        ↓
C adds one random tile
        ↓
C checks 2048
        ↓
C checks game over
        ↓
C builds JSON
        ↓
Browser receives response
        ↓
renderGame()
        ↓
HTML board + score + status updated
```

## 14. Why invalid moves do not create tiles

Each C movement function copies the board before the move.

After movement:

```text
before == after
```

means the board did not change.

Therefore:

```text
return 0
```

and no random tile is generated.

If the board changed:

```text
addRandomTile();
```

is called exactly once.

## 15. Why this satisfies the "logic must be in C" requirement

The frontend has no representation of the game's authoritative state.

It does not decide:

```text
2 + 2 = 4
```

It does not decide:

```text
GAME_OVER
```

It does not decide:

```text
WON
```

It does not calculate:

```text
score += mergedValue
```

All of these decisions are made by `game.c`.

The frontend displays the state returned by C.


## 21. Deploying this C version to Vercel

Vercel now supports deploying HTTP servers from a `Dockerfile.vercel` as Vercel Functions using Fluid Compute. This project includes `Dockerfile.vercel` specifically for that deployment model.

The C server has also been adjusted for hosting:

- It reads the `PORT` environment variable.
- It binds to `0.0.0.0` instead of only `127.0.0.1`.
- The game engine remains in `game.c`.
- The browser still communicates with the C server through `/api/move` and `/api/new-game`.

### Deploy through GitHub

1. Push the complete project to GitHub.
2. Import the repository into Vercel.
3. Keep the project root as the repository root.
4. Vercel detects `Dockerfile.vercel`.
5. Deploy.

The resulting architecture remains:

```text
Browser
   ↓
Vercel
   ↓
C HTTP server
   ↓
game.c
   ↓
JSON
   ↓
Browser
```

### CLI deployment

Install/login to Vercel if needed:

```powershell
npm install -g vercel
vercel login
```

From the project root:

```powershell
vercel
```

For production:

```powershell
vercel --prod
```

After deployment, open the generated Vercel URL.

### Important

Do not deploy only the `frontend` folder. The C backend and `Dockerfile.vercel` must be included in the Vercel project root.
