from flask import Flask, render_template_string, request
import subprocess, os

app = Flask(__name__)

# Paths
GAMES_DIR = "/home/sebas/gameconsole/games"
THUMB_DIR = "/home/sebas/gameconsole/static/thumbnails"

HTML = """
<!DOCTYPE html>
<html>
<head>
    <title>Arduino Game Loader</title>
    <script src="https://unpkg.com/htmx.org@1.9.10"></script>

    <style>
        body {
            margin: 0;
            background: #060606;
            color: #d3d3d3;
            font-family: Consolas, 'Courier New', monospace;
            text-align: center;
            padding-bottom: 50px;
        }

        h1 {
            margin-top: 40px;
            font-size: 42px;
            font-weight: 700;
            color: #e0e0e0;
            letter-spacing: 2px;
        }

        h3 {
            opacity: 0.55;
            margin-bottom: 35px;
            font-weight: 300;
            color: #bbbbbb;
        }

        .game-grid {
            display: grid;
            grid-template-columns: repeat(auto-fill, minmax(230px, 1fr));
            gap: 26px;
            width: 85%;
            margin: 0 auto;
        }

        .game-card {
            background: #0d0d0d;
            border: 1px solid #1b1b1b;
            padding: 14px;
            border-radius: 6px;
            transition: 0.2s ease;
        }

        .game-card:hover {
            border-color: #585858;
            transform: scale(1.04);
        }

        .thumbnail {
            width: 100%;
            height: 130px;
            background: #141414;
            border: 1px solid #222;
            border-radius: 6px;
            color: #777;
            display: flex;
            align-items: center;
            justify-content: center;
            font-size: 18px;
            overflow: hidden;
        }

        .thumbnail img {
            width: 100%;
            height: 100%;
            object-fit: cover;
            border-radius: 6px;
        }

        .button {
            width: 100%;
            padding: 12px;
            border-radius: 4px;
            font-size: 16px;
            border: 1px solid #2a2a2a;
            background: #111;
            color: #d6d6d6;
            cursor: pointer;
            transition: 0.2s;
            letter-spacing: 0.5px;
        }

        .button:hover {
            background: #171717;
            border-color: #555;
            color: #fff;
        }

        #log-box {
            margin: 45px auto;
            width: 85%;
            background: #0c0c0c;
            border: 1px solid #222;
            border-radius: 6px;
            padding: 18px;
            white-space: pre-wrap;
            text-align: left;
            color: #b0ffa0;
            font-size: 13.5px;
            min-height: 150px;
        }

        .loading {
            color: #ffff80;
            animation: blink 1s infinite;
        }

        @keyframes blink {
            50% { opacity: 0.3; }
        }
    </style>
</head>

<body>

<h1>Arduino GamePi4</h1>
<h3>Choose a game to flash 🎮</h3>

<div class="game-grid">
    {% for game in games %}
    <div class="game-card">

        <div class="thumbnail">
            {% if game.thumb %}
                <img src="{{ game.thumb }}">
            {% else %}
                IMG
            {% endif %}
        </div>

        <form 
            hx-post="/flash"
            hx-target="#log-box"
            hx-swap="innerHTML"
            hx-on:click="document.getElementById('log-box').innerHTML='Flashing...';"
        >
            <input type="hidden" name="game" value="{{ game.file }}">
            <button class="button">{{ game.pretty }}</button>
        </form>

    </div>
    {% endfor %}
</div>

<div id="log-box">Waiting for flash request...</div>

</body>
</html>
"""

@app.route("/", methods=["GET"])
def index():
    games = []

    for filename in os.listdir(GAMES_DIR):
        if filename.endswith(".hex"):
            base = filename.replace(".ino.hex", "")

            # Match thumbnail
            thumb_path = os.path.join(THUMB_DIR, base + ".png")
            thumb_url = f"/static/thumbnails/{base}.png" if os.path.exists(thumb_path) else None

            pretty = base.replace("_", " ").title()

            games.append({
                "file": filename,
                "pretty": pretty,
                "thumb": thumb_url
            })

    return render_template_string(HTML, games=games)


@app.route("/flash", methods=["POST"])
def flash_game():
    game = request.form["game"]
    fullpath = os.path.join(GAMES_DIR, game)

    result = subprocess.run(
        [
            "avrdude", "-v",
            "-patmega328p",
            "-carduino",
            "-P/dev/ttyACM0",
            "-b115200",
            "-D",
            f"-Uflash:w:{fullpath}:i"
        ],
        capture_output=True,
        text=True
    )

    return (
        "<span class='loading'>Flashing... done!</span>\n\n"
        + result.stdout
        + "\n"
        + result.stderr
    )


app.run(host="0.0.0.0", port=5000)
