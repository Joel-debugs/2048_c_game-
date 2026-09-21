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

function renderGame(state) {
    boardElement.replaceChildren();

    state.board.forEach((row) => {
        row.forEach((value) => {
            const tile = document.createElement("div");

            tile.className = "tile";
            tile.dataset.value = String(value);
            tile.textContent = value === 0 ? "" : String(value);

            boardElement.appendChild(tile);
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
        console.error(error);
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
            body: JSON.stringify({
                direction
            })
        });

        if (!response.ok) {
            throw new Error(`Server returned ${response.status}`);
        }

        /*
         * The returned JSON is the authoritative game state.
         * JavaScript does not calculate movement, merging, score, or game over.
         */
        const state = await response.json();
        renderGame(state);
    } catch (error) {
        console.error(error);
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

/* Basic swipe support for mobile. */
document.querySelector(".game-area").addEventListener(
    "touchstart",
    (event) => {
        const touch = event.changedTouches[0];
        touchStartX = touch.clientX;
        touchStartY = touch.clientY;
    },
    { passive: true }
);

document.querySelector(".game-area").addEventListener(
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

/* Ask the C server for the initial game state. */
requestNewGame();
