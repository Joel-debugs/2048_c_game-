/*
 * The frontend does NOT implement 2048 rules.
 *
 * It only:
 *   1. Captures input.
 *   2. Sends a direction to the C HTTP backend.
 *   3. Receives board + score + status.
 *   4. Renders the response.
 */

const boardElement = document.getElementById("board");
const scoreElement = document.getElementById("score");
const statusElement = document.getElementById("status");
const newGameButton = document.getElementById("new-game");

const overlay = document.getElementById("overlay");
const overlayTitle = document.getElementById("overlay-title");
const overlayMessage = document.getElementById("overlay-message");
const overlayButton = document.getElementById("overlay-button");

let requestInProgress = false;
let touchStartX = 0;
let touchStartY = 0;

/*
 * Create the 16 tile elements once.
 *
 * The frontend previously destroyed and recreated the entire board after
 * every move. Keeping the same DOM nodes avoids unnecessary layout churn
 * and visual flicker. C remains the authoritative game engine.
 */
const tiles = [];

for (let i = 0; i < 16; i++) {
    const tile = document.createElement("div");
    tile.className = "tile";
    tile.dataset.value = "0";
    boardElement.appendChild(tile);
    tiles.push(tile);
}

function renderGame(state) {
    /* The C server supplies the complete authoritative game state. */
    let index = 0;

    state.board.forEach((row) => {
        row.forEach((value) => {
            const tile = tiles[index];

            tile.dataset.value = String(value);
            tile.textContent = value === 0 ? "" : String(value);

            index++;
        });
    });

    scoreElement.textContent = String(state.score);
    statusElement.textContent = state.status;

    if (state.status === "WON") {
        showOverlay(
            "You Win!",
            "The C game engine detected the 2048 tile."
        );
    } else if (state.status === "GAME_OVER") {
        showOverlay(
            "Game Over",
            "No empty cells and no adjacent equal tiles remain."
        );
    } else {
        hideOverlay();
    }
}

function showOverlay(title, message) {
    overlayTitle.textContent = title;
    overlayMessage.textContent = message;
    overlay.classList.remove("hidden");
}

function hideOverlay() {
    overlay.classList.add("hidden");
}

async function requestNewGame() {
    if (requestInProgress) {
        return;
    }

    requestInProgress = true;

    try {
        const response = await fetch("/api/new-game", {
            method: "POST"
        });

        if (!response.ok) {
            throw new Error(`Server returned ${response.status}`);
        }

        const state = await response.json();
        renderGame(state);
    } catch (error) {
        console.error("New game error:", error);
        statusElement.textContent = "SERVER ERROR";
    } finally {
        requestInProgress = false;
    }
}

async function sendMove(direction) {
    if (requestInProgress) {
        return;
    }

    requestInProgress = true;

    try {
        const response = await fetch("/api/move", {
            method: "POST",
            headers: {
                "Content-Type": "application/json"
            },
            body: JSON.stringify({ direction })
        });

        if (!response.ok) {
            throw new Error(`Server returned ${response.status}`);
        }

        /* C performs all movement, merging, scoring, and status logic. */
        const state = await response.json();
        renderGame(state);
    } catch (error) {
        console.error("Move error:", error);
        statusElement.textContent = "SERVER ERROR";
    } finally {
        requestInProgress = false;
    }
}

function directionFromKey(key) {
    const normalized = key.toLowerCase();

    const directions = {
        arrowleft: "left",
        a: "left",
        arrowright: "right",
        d: "right",
        arrowup: "up",
        w: "up",
        arrowdown: "down",
        s: "down"
    };

    return directions[normalized] || null;
}

document.addEventListener("keydown", (event) => {
    const direction = directionFromKey(event.key);

    if (!direction) {
        return;
    }

    event.preventDefault();
    sendMove(direction);
});

newGameButton.addEventListener("click", requestNewGame);
overlayButton.addEventListener("click", requestNewGame);

const gameArea = document.querySelector(".game-area");

gameArea.addEventListener(
    "touchstart",
    (event) => {
        const touch = event.changedTouches[0];
        touchStartX = touch.clientX;
        touchStartY = touch.clientY;
    },
    { passive: true }
);

gameArea.addEventListener(
    "touchend",
    (event) => {
        const touch = event.changedTouches[0];
        const dx = touch.clientX - touchStartX;
        const dy = touch.clientY - touchStartY;
        const minimumSwipe = 35;

        if (Math.max(Math.abs(dx), Math.abs(dy)) < minimumSwipe) {
            return;
        }

        if (Math.abs(dx) > Math.abs(dy)) {
            sendMove(dx > 0 ? "right" : "left");
        } else {
            sendMove(dy > 0 ? "down" : "up");
        }
    },
    { passive: true }
);

requestNewGame();
