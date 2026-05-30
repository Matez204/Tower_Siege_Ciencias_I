# ============================================================
# Team: [TEAM_NUMBER] | Variant: Tower Siege
# Members: [STUDENT_NAMES]
# bridge.py — JSON Bridge between Python (game logic) and C++ (engine)
# Responsibility: Write input.json for C++ to consume,
#                 and read state.json that C++ produces.
# ============================================================

import json
import os
import time

# ── Paths ────────────────────────────────────────────────────────────────────
_DATA_DIR   = os.path.join(os.path.dirname(__file__), "data")
INPUT_PATH  = os.path.join(_DATA_DIR, "input.json")
STATE_PATH  = os.path.join(_DATA_DIR, "state.json")

# How many seconds to wait for state.json to appear after writing input.json
_STATE_TIMEOUT = 5.0
_POLL_INTERVAL = 0.05          # seconds between file-existence checks


# ── Internal helpers ──────────────────────────────────────────────────────────

def _ensure_data_dir() -> None:
    """Create the data/ directory if it does not exist yet."""
    os.makedirs(_DATA_DIR, exist_ok=True)


def _remove_if_exists(path: str) -> None:
    """Delete a file so we can detect when C++ has written a fresh copy."""
    try:
        os.remove(path)
    except FileNotFoundError:
        pass


# ── Public API ────────────────────────────────────────────────────────────────

def write_input(
    tick:            int,
    phase:           str,
    elapsed_seconds: float,
    gold:            int,
    dt:              float,
    castle:          dict,
    towers:          list[dict],
    horde_queue:     list[dict],
) -> None:
    """
    Serialize the current game state into input.json for the C++ engine.

    Parameters
    ----------
    tick            : Current game tick number.
    phase           : "setup" | "action" | "gameover"
    elapsed_seconds : Total seconds elapsed since the game started.
    gold            : Player's current gold amount.
    dt              : Seconds that have passed since the last tick.
    castle          : Dict with keys: id, hp, hp_max, pos{row,col},
                      atk_range, dmg.
    towers          : List of dicts, each with: id, hp, pos{row,col},
                      atk_range, dmg.
    horde_queue     : List of horde dicts, each with: id, size,
                      speed_override (int | None), path[{row,col}],
                      head_pos{row,col}, enemies[{id,hp,hp_max,
                      pos,dmg,atk_range}].
    """
    _ensure_data_dir()

    payload = {
        "meta": {
            "tick":             tick,
            "phase":            phase,
            "elapsed_seconds":  elapsed_seconds,
            "gold":             gold,
            "dt":               dt,
        },
        "castle":      castle,
        "towers":      towers,
        "horde_queue": horde_queue,
    }

    # Write atomically: dump to a temp file then rename so C++ never reads
    # a half-written file.
    tmp_path = INPUT_PATH + ".tmp"
    with open(tmp_path, "w", encoding="utf-8") as f:
        json.dump(payload, f, indent=2)
    os.replace(tmp_path, INPUT_PATH)


def read_state(timeout: float = _STATE_TIMEOUT) -> dict | None:
    """
    Block until state.json is produced by C++, then parse and return it.

    The function first removes any stale state.json so it can detect the
    fresh file that C++ writes in response to the current input.json.

    Returns the parsed dict on success, or None if the timeout expires.

    Returned dict structure
    -----------------------
    {
      "tick_result": {
          "tick": int,
          "gold_earned": int,
          "status": "ongoing" | "victory" | "defeat"
      },
      "castle_state": {
          "id": str, "hp": int, "hp_max": int,
          "damage_taken_this_tick": int
      },
      "towers_state": [
          {"id": str, "hp": int, "last_target_id": str | None}
      ],
      "horde_state": [
          {
            "id": str,
            "head_pos": {"row": int, "col": int},
            "enemies_alive": int,
            "enemies": [{"id": str, "hp": int, "pos": {"row":int,"col":int}}],
            "killed_ids": [str]
          }
      ],
      "events": [
          {"type": str, "data": dict}
      ]
    }
    """
    # Remove stale state so we know the file we read is from THIS tick.
    _remove_if_exists(STATE_PATH)

    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if os.path.exists(STATE_PATH):
            try:
                with open(STATE_PATH, "r", encoding="utf-8") as f:
                    return json.load(f)
            except (json.JSONDecodeError, OSError):
                # C++ may still be writing; wait and retry.
                pass
        time.sleep(_POLL_INTERVAL)

    # Timeout reached — caller decides how to handle this.
    print(f"[bridge] WARNING: state.json not received within {timeout}s")
    return None


# ── Convenience extractors (used by main.py / game logic) ────────────────────

def get_gold_earned(state: dict) -> int:
    """Return gold earned this tick, or 0 if the key is missing."""
    return state.get("tick_result", {}).get("gold_earned", 0)


def get_game_status(state: dict) -> str:
    """Return 'ongoing', 'victory', or 'defeat'."""
    return state.get("tick_result", {}).get("status", "ongoing")


def get_castle_hp(state: dict) -> int:
    return state.get("castle_state", {}).get("hp", 0)


def get_killed_ids(state: dict) -> list[str]:
    """Return a flat list of every enemy killed this tick across all hordes."""
    killed = []
    for horde in state.get("horde_state", []):
        killed.extend(horde.get("killed_ids", []))
    return killed


def get_events(state: dict) -> list[dict]:
    """Return the events list (ENEMY_KILLED, CASTLE_HIT, etc.)."""
    return state.get("events", [])
