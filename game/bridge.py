# ============================================================
# Team: [Team Number] | Variant: Tower Siege
# Members: [Student Names]
# File: bridge.py
# Description: JSON Bridge — handles all file I/O between
#              Python (Pygame + algorithms) and the C++ engine.
# ============================================================

import json
import os
import subprocess
import time
from dataclasses import dataclass, field
from typing import Optional


# ──────────────────────────────────────────────
# Paths (adjust if your folder layout differs)
# ──────────────────────────────────────────────
_BASE_DIR   = os.path.dirname(os.path.abspath(__file__))
INPUT_PATH  = os.path.join(_BASE_DIR, "data", "input.json")
STATE_PATH  = os.path.join(_BASE_DIR, "data", "state.json")
ENGINE_BIN  = os.path.join(_BASE_DIR, "engine", "tower_siege")   # compiled C++ binary


# ══════════════════════════════════════════════
# 1.  DATA CLASSES  (mirrors the JSON schema)
# ══════════════════════════════════════════════

@dataclass
class Pos:
    row: int
    col: int

    def to_dict(self) -> dict:
        return {"row": self.row, "col": self.col}

    @staticmethod
    def from_dict(d: dict) -> "Pos":
        return Pos(row=d["row"], col=d["col"])


@dataclass
class Enemy:
    id:        str
    hp:        int
    hp_max:    int
    pos:       Pos
    dmg:       int
    atk_range: int

    def to_dict(self) -> dict:
        return {
            "id":        self.id,
            "hp":        self.hp,
            "hp_max":    self.hp_max,
            "pos":       self.pos.to_dict(),
            "dmg":       self.dmg,
            "atk_range": self.atk_range,
        }

    @staticmethod
    def from_dict(d: dict) -> "Enemy":
        return Enemy(
            id=d["id"], hp=d["hp"], hp_max=d["hp_max"],
            pos=Pos.from_dict(d["pos"]),
            dmg=d["dmg"], atk_range=d["atk_range"],
        )


@dataclass
class Horde:
    id:             str
    size:           int
    path:           list[Pos]
    head_pos:       Pos
    enemies:        list[Enemy]
    speed_override: Optional[float] = None   # None → C++ calculates from size

    def to_dict(self) -> dict:
        return {
            "id":             self.id,
            "size":           self.size,
            "speed_override": self.speed_override,
            "path":           [p.to_dict() for p in self.path],
            "head_pos":       self.head_pos.to_dict(),
            "enemies":        [e.to_dict() for e in self.enemies],
        }

    @staticmethod
    def from_dict(d: dict) -> "Horde":
        return Horde(
            id=d["id"], size=d["size"],
            speed_override=d.get("speed_override"),
            path=[Pos.from_dict(p) for p in d["path"]],
            head_pos=Pos.from_dict(d["head_pos"]),
            enemies=[Enemy.from_dict(e) for e in d["enemies"]],
        )


@dataclass
class Tower:
    id:        str
    hp:        int
    pos:       Pos
    atk_range: int
    dmg:       int

    def to_dict(self) -> dict:
        return {
            "id":        self.id,
            "hp":        self.hp,
            "pos":       self.pos.to_dict(),
            "atk_range": self.atk_range,
            "dmg":       self.dmg,
        }

    @staticmethod
    def from_dict(d: dict) -> "Tower":
        return Tower(
            id=d["id"], hp=d["hp"],
            pos=Pos.from_dict(d["pos"]),
            atk_range=d["atk_range"], dmg=d["dmg"],
        )


@dataclass
class Castle:
    id:        str
    hp:        int
    hp_max:    int
    pos:       Pos
    atk_range: int
    dmg:       int

    def to_dict(self) -> dict:
        return {
            "id":        self.id,
            "hp":        self.hp,
            "hp_max":    self.hp_max,
            "pos":       self.pos.to_dict(),
            "atk_range": self.atk_range,
            "dmg":       self.dmg,
        }

    @staticmethod
    def from_dict(d: dict) -> "Castle":
        return Castle(
            id=d["id"], hp=d["hp"], hp_max=d["hp_max"],
            pos=Pos.from_dict(d["pos"]),
            atk_range=d["atk_range"], dmg=d["dmg"],
        )


@dataclass
class GameInput:
    """Full snapshot Python sends to C++ each tick."""
    tick:            int
    phase:           str          # "setup" | "action" | "gameover"
    elapsed_seconds: float
    gold:            int
    dt:              float        # seconds since last tick
    castle:          Castle
    towers:          list[Tower]
    horde_queue:     list[Horde]

    def to_dict(self) -> dict:
        return {
            "meta": {
                "tick":            self.tick,
                "phase":           self.phase,
                "elapsed_seconds": self.elapsed_seconds,
                "gold":            self.gold,
                "dt":              self.dt,
            },
            "castle":       self.castle.to_dict(),
            "towers":       [t.to_dict() for t in self.towers],
            "horde_queue":  [h.to_dict() for h in self.horde_queue],
        }


# ──────────────────────────────────────────────
# State objects returned BY C++
# ──────────────────────────────────────────────

@dataclass
class EnemyState:
    id:  str
    hp:  int
    pos: Pos

    @staticmethod
    def from_dict(d: dict) -> "EnemyState":
        return EnemyState(id=d["id"], hp=d["hp"], pos=Pos.from_dict(d["pos"]))


@dataclass
class HordeState:
    id:           str
    head_pos:     Pos
    enemies_alive: int
    enemies:      list[EnemyState]
    killed_ids:   list[str]

    @staticmethod
    def from_dict(d: dict) -> "HordeState":
        return HordeState(
            id=d["id"],
            head_pos=Pos.from_dict(d["head_pos"]),
            enemies_alive=d["enemies_alive"],
            enemies=[EnemyState.from_dict(e) for e in d["enemies"]],
            killed_ids=d.get("killed_ids", []),
        )


@dataclass
class TowerState:
    id:             str
    hp:             int
    last_target_id: Optional[str]

    @staticmethod
    def from_dict(d: dict) -> "TowerState":
        return TowerState(
            id=d["id"], hp=d["hp"],
            last_target_id=d.get("last_target_id"),
        )


@dataclass
class CastleState:
    id:                    str
    hp:                    int
    hp_max:                int
    damage_taken_this_tick: int

    @staticmethod
    def from_dict(d: dict) -> "CastleState":
        return CastleState(
            id=d["id"], hp=d["hp"], hp_max=d["hp_max"],
            damage_taken_this_tick=d.get("damage_taken_this_tick", 0),
        )


@dataclass
class GameEvent:
    type: str
    data: dict

    @staticmethod
    def from_dict(d: dict) -> "GameEvent":
        return GameEvent(type=d["type"], data=d.get("data", {}))


@dataclass
class GameState:
    """Full snapshot C++ returns to Python each tick."""
    tick:          int
    gold_earned:   int
    status:        str            # "ongoing" | "victory" | "defeat"
    castle:        CastleState
    towers:        list[TowerState]
    hordes:        list[HordeState]
    events:        list[GameEvent]

    @staticmethod
    def from_dict(d: dict) -> "GameState":
        tr = d["tick_result"]
        return GameState(
            tick=tr["tick"],
            gold_earned=tr.get("gold_earned", 0),
            status=tr.get("status", "ongoing"),
            castle=CastleState.from_dict(d["castle_state"]),
            towers=[TowerState.from_dict(t) for t in d.get("towers_state", [])],
            hordes=[HordeState.from_dict(h) for h in d.get("horde_state", [])],
            events=[GameEvent.from_dict(e) for e in d.get("events", [])],
        )


# ══════════════════════════════════════════════
# 2.  BRIDGE CLASS
# ══════════════════════════════════════════════

class Bridge:
    """
    Handles all I/O between Python and the C++ engine.

    Typical usage inside the game loop:
        bridge = Bridge()
        bridge.write_input(game_input)
        bridge.run_engine()                 # blocks until engine finishes
        state = bridge.read_state()
    """

    def __init__(
        self,
        input_path:  str = INPUT_PATH,
        state_path:  str = STATE_PATH,
        engine_bin:  str = ENGINE_BIN,
        timeout_sec: float = 2.0,
    ):
        self.input_path  = input_path
        self.state_path  = state_path
        self.engine_bin  = engine_bin
        self.timeout_sec = timeout_sec

        # Make sure the data/ directory exists
        os.makedirs(os.path.dirname(input_path), exist_ok=True)

    # ── Write ────────────────────────────────
    def write_input(self, game_input: GameInput) -> None:
        """Serialize GameInput → input.json (the file C++ reads)."""
        try:
            with open(self.input_path, "w", encoding="utf-8") as f:
                json.dump(game_input.to_dict(), f, indent=2)
        except OSError as exc:
            raise BridgeError(f"Cannot write input.json: {exc}") from exc

    # ── Execute ──────────────────────────────
    def run_engine(self) -> None:
        """
        Launch the C++ binary synchronously.
        The binary reads input.json, processes one tick, writes state.json.
        Raises BridgeError on timeout or non-zero exit code.
        """
        if not os.path.isfile(self.engine_bin):
            raise BridgeError(
                f"Engine binary not found: {self.engine_bin}\n"
                "Run  make  inside the engine/ directory first."
            )
        try:
            result = subprocess.run(
                [self.engine_bin],
                timeout=self.timeout_sec,
                capture_output=True,
                text=True,
            )
        except subprocess.TimeoutExpired as exc:
            raise BridgeError(
                f"C++ engine timed out after {self.timeout_sec}s"
            ) from exc

        if result.returncode != 0:
            raise BridgeError(
                f"C++ engine exited with code {result.returncode}.\n"
                f"stderr: {result.stderr.strip()}"
            )

    # ── Read ─────────────────────────────────
    def read_state(self) -> GameState:
        """Deserialize state.json → GameState (written by C++)."""
        # Retry once: the engine might still be flushing the file
        for attempt in range(2):
            try:
                with open(self.state_path, "r", encoding="utf-8") as f:
                    raw = json.load(f)
                return GameState.from_dict(raw)
            except (OSError, json.JSONDecodeError) as exc:
                if attempt == 0:
                    time.sleep(0.05)
                    continue
                raise BridgeError(f"Cannot read state.json: {exc}") from exc

    # ── Convenience ──────────────────────────
    def tick(self, game_input: GameInput) -> GameState:
        """
        Full round-trip in one call:
            write input → run engine → read state → return state
        """
        self.write_input(game_input)
        self.run_engine()
        return self.read_state()


# ══════════════════════════════════════════════
# 3.  CUSTOM EXCEPTION
# ══════════════════════════════════════════════

class BridgeError(RuntimeError):
    """Raised when any bridge I/O or subprocess step fails."""


# ══════════════════════════════════════════════
# 4.  QUICK SMOKE-TEST  (python bridge.py)
# ══════════════════════════════════════════════

if __name__ == "__main__":
    import sys

    print("=== Bridge smoke-test ===")

    # Build a minimal GameInput that mirrors inputPrueba.json
    sample_input = GameInput(
        tick=42,
        phase="action",
        elapsed_seconds=8.4,
        gold=150,
        dt=0.5,
        castle=Castle(
            id="castle_0", hp=200, hp_max=200,
            pos=Pos(row=0, col=4),
            atk_range=2, dmg=15,
        ),
        towers=[
            Tower(id="tower_1", hp=100,
                  pos=Pos(row=3, col=2),
                  atk_range=2, dmg=20),
        ],
        horde_queue=[
            Horde(
                id="horde_0", size=5,
                path=[Pos(8,0), Pos(7,0), Pos(0,4)],
                head_pos=Pos(6,0),
                enemies=[
                    Enemy("enemy_0", 60, 80, Pos(6,0), 10, 1),
                    Enemy("enemy_1", 80, 80, Pos(7,0), 10, 1),
                ],
            )
        ],
    )

    bridge = Bridge()

    # 1. Write
    bridge.write_input(sample_input)
    print(f"✔  Wrote input.json → {bridge.input_path}")

    # 2. Verify round-trip serialization
    with open(bridge.input_path) as f:
        reloaded = json.load(f)
    assert reloaded["meta"]["tick"] == 42
    assert reloaded["castle"]["id"] == "castle_0"
    assert len(reloaded["horde_queue"]) == 1
    print("✔  Serialization round-trip OK")

    # 3. Try reading statePrueba.json if it exists in data/
    sample_state_path = os.path.join(
        os.path.dirname(bridge.state_path), "statePrueba.json"
    )
    if os.path.exists(sample_state_path):
        with open(sample_state_path) as f:
            raw = json.load(f)
        state = GameState.from_dict(raw)
        print(f"✔  Parsed state.json — status: {state.status}, "
              f"castle hp: {state.castle.hp}, "
              f"events: {[e.type for e in state.events]}")
    else:
        print("ℹ  No statePrueba.json found — skipping state parse test")

    print("\nAll checks passed. Bridge is ready.")
    sys.exit(0)
