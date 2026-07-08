<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Mini Block Game</title>
    <style>
        body {
            margin: 0;
            background: #111;
            color: #fff;
            font-family: sans-serif;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            min-height: 100vh;
            overflow: hidden;
        }
        h1 {
            margin: 5px 0;
            font-size: 1.5rem;
        }
        canvas {
            background: #222;
            border: 4px solid #444;
            border-radius: 4px;
            box-shadow: 0 10px 20px rgba(0,0,0,0.5);
            max-width: 100%;
        }
        .info {
            margin-top: 10px;
            color: #888;
            font-size: 0.9rem;
        }
    </style>
</head>
<body>

    <h1>Mini Block Breaker</h1>
    <canvas id="gameCanvas" width="480" height="320"></canvas>
    <div class="info">Move mouse / swipe to control paddle</div>

<script>
    const canvas = document.getElementById("gameCanvas");
    const ctx = canvas.getContext("2d");

    // Ball configuration
    let ballRadius = 8;
    let x = canvas.width / 2;
    let y = canvas.height - 30;
    let dx = 3;
    let dy = -3;

    // Paddle configuration
    const paddleHeight = 10;
    const paddleWidth = 75;
    let paddleX = (canvas.width - paddleWidth) / 2;

    // Blocks configuration
    const rowCount = 3;
    const columnCount = 5;
    const blockWidth = 75;
    const blockHeight = 20;
    const blockPadding = 10;
    const blockOffsetTop = 30;
    const blockOffsetLeft = 30;

    // Score & Lives
    let score = 0;
    let lives = 3;

    // Initialize blocks array
    const blocks = [];
    for (let c = 0; c < columnCount; c++) {
        blocks[c] = [];
        for (let r = 0; r < rowCount; r++) {
            blocks[c][r] = { x: 0, y: 0, status: 1 };
        }
    }

    // Track mouse movement
    document.addEventListener("mousemove", mouseMoveHandler, false);
    document.addEventListener("touchmove", touchMoveHandler, { passive: false });

    function mouseMoveHandler(e) {
        const relativeX = e.clientX - canvas.offsetLeft;
        if (relativeX > 0 && relativeX < canvas.width) {
            paddleX = relativeX - paddleWidth / 2;
        }
    }

    function touchMoveHandler(e) {
        e.preventDefault();
        const relativeX = e.touches[0].clientX - canvas.offsetLeft;
        if (relativeX > 0 && relativeX < canvas.width) {
            paddleX = relativeX - paddleWidth / 2;
        }
    }

    // Collision detection helper
    function collisionDetection() {
        for (let c = 0; c < columnCount; c++) {
            for (let r = 0; r < rowCount; r++) {
                const b = blocks[c][r];
                if (b.status === 1) {
                    if (x > b.x && x < b.x + blockWidth && y > b.y && y < b.y + blockHeight) {
                        dy = -dy;
                        b.status = 0;
                        score++;
                        if (score === rowCount * columnCount) {
                            alert("YOU WIN, CONGRATULATIONS!");
                            document.location.reload();
                        }
                    }
                }
            }
        }
    }

    // Drawing functions
    function drawBall() {
        ctx.beginPath();
        ctx.arc(x, y, ballRadius, 0, Math.PI * 2);
        ctx.fillStyle = "#0095DD";
        ctx.fill();
        ctx.closePath();
    }

    function drawPaddle() {
        ctx.beginPath();
        ctx.rect(paddleX, canvas.height - paddleHeight - 5, paddleWidth, paddleHeight);
        ctx.fillStyle = "#0095DD";
        ctx.fill();
        ctx.closePath();
    }

    function drawBlocks() {
        for (let c = 0; c < columnCount; c++) {
            for (let r = 0; r < rowCount; r++) {
                if (blocks[c][r].status === 1) {
                    const blockX = (c * (blockWidth + blockPadding)) + blockOffsetLeft;
                    const blockY = (r * (blockHeight + blockPadding)) + blockOffsetTop;
                    blocks[c][r].x = blockX;
                    blocks[c][r].y = blockY;
                    ctx.beginPath();
                    ctx.rect(blockX, blockY, blockWidth, blockHeight);
                    ctx.fillStyle = "#ff5722";
                    ctx.fill();
                    ctx.closePath();
                }
            }
        }
    }

    function drawScore() {
        ctx.font = "16px sans-serif";
        ctx.fillStyle = "#fff";
        ctx.fillText("Score: " + score, 8, 20);
    }

    function drawLives() {
        ctx.font = "16px sans-serif";
        ctx.fillStyle = "#fff";
        ctx.fillText("Lives: " + lives, canvas.width - 65, 20);
    }

    // Main game loop
    function draw() {
        ctx.clearRect(0, 0, canvas.width, canvas.height);
        drawBlocks();
        drawBall();
        drawPaddle();
        drawScore();
        drawLives();
        collisionDetection();

        // Bounce off left/right walls
        if (x + dx > canvas.width - ballRadius || x + dx < ballRadius) {
            dx = -dx;
        }
        // Bounce off top wall
        if (y + dy < ballRadius) {
            dy = -dy;
        } 
        // Bottom wall hit detection
        else if (y + dy > canvas.height - ballRadius - 5) {
            if (x > paddleX && x < paddleX + paddleWidth) {
                dy = -dy;
            } else {
                lives--;
                if (!lives) {
                    alert("GAME OVER");
                    document.location.reload();
                } else {
                    x = canvas.width / 2;
                    y = canvas.height - 30;
                    dx = 3;
                    dy = -3;
                    paddleX = (canvas.width - paddleWidth) / 2;
                }
            }
        }

        x += dx;
        y += dy;
        requestAnimationFrame(draw);
    }

    // Start the loop
    draw();
</script>
</body>
</html>
