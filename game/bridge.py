# ============================================================
# Tower Siege — Variant 1
# Bridge: Python ↔ C++ via JSON file I/O
# Ciencias de la Computación I — 2026-I
# Universidad Distrital Francisco José de Caldas
# ============================================================

# bridge.py
import json
import os
import subprocess
from dataclasses import dataclass, field
from typing import List, Dict, Any

_BASE_DIR  = os.path.dirname(os.path.abspath(__file__))
INPUT_PATH = os.path.join(_BASE_DIR, "data", "input.json")
STATE_PATH = os.path.join(_BASE_DIR, "data", "state.json")
# Windows usa .exe, Linux/Mac no tiene extensión
_EXE = "tower_siege.exe" if os.name == "nt" else "tower_siege"
ENGINE_BIN = os.path.join(_BASE_DIR, "engine", _EXE)

# ── ESTRUCTURAS DE ENTRADA (Python → C++) ────────────────────

@dataclass
class Pos:
    row: int
    col: int
    def to_dict(self):
        return {"row": self.row, "col": self.col}

@dataclass
class Tower:
    id:        str
    pos:       Pos
    atk_range: int
    dmg:       int
    hp:        int = 100
    def to_dict(self):
        return {
            "id":        self.id,
            "pos":       self.pos.to_dict(),
            "atk_range": self.atk_range,
            "dmg":       self.dmg,
            "hp":        self.hp
        }

@dataclass
class Castle:
    id:        str
    hp:        int
    hp_max:    int
    pos:       Pos = None
    atk_range: int = 2
    dmg:       int = 15
    def __post_init__(self):
        if self.pos is None:
            self.pos = Pos(0, 4)
    def to_dict(self):
        return {
            "id":        self.id,
            "hp":        self.hp,
            "hp_max":    self.hp_max,
            "pos":       self.pos.to_dict(),
            "atk_range": self.atk_range,
            "dmg":       self.dmg
        }

@dataclass
class Enemy:
    id:        str
    hp:        int
    hp_max:    int
    pos:       Pos
    dmg:       int = 10
    atk_range: int = 1

@dataclass
class Horde:
    id:      str
    size:    int
    path:    List[Pos]
    enemies: List[Enemy] = field(default_factory=list)

@dataclass
class InputState:
    meta:        Dict[str, Any]
    castle:      Castle
    towers:      List[Tower]
    horde_queue: List[Horde]
    gold:        int = 0
    phase:       str = "action"

    def to_dict(self):
        horde_list = []
        for h in self.horde_queue:
            enemies_list = []
            for e in h.enemies:
                enemies_list.append({
                    "id":        e.id,
                    "hp":        e.hp,
                    "hp_max":    e.hp_max,
                    "pos":       e.pos.to_dict(),
                    "dmg":       e.dmg,
                    "atk_range": e.atk_range
                })
            horde_list.append({
                "id":      h.id,
                "size":    h.size,
                "path":    [p.to_dict() for p in h.path],
                "enemies": enemies_list
            })
        return {
            "meta": {
                **self.meta,
                "phase": self.phase,
                "gold":  self.gold
            },
            "castle":      self.castle.to_dict(),
            "towers":      [t.to_dict() for t in self.towers],
            "horde_queue": horde_list
        }

# ── ESTRUCTURAS DE SALIDA (C++ → Python) ─────────────────────

@dataclass
class TickResult:
    tick:        int
    gold_earned: int
    status:      str

@dataclass
class GameState:
    tick_result:  TickResult
    castle:       Castle
    horde_queue:  List[Horde]

# ── BRIDGE ───────────────────────────────────────────────────

class Bridge:
    def __init__(self):
        self.input_path = INPUT_PATH
        self.state_path = STATE_PATH
        os.makedirs(os.path.dirname(INPUT_PATH), exist_ok=True)

    def write_input(self, input_state: InputState):
        with open(self.input_path, "w") as f:
            json.dump(input_state.to_dict(), f, indent=4)

    def run_engine_tick(self) -> bool:
        if not os.path.exists(ENGINE_BIN):
            return False
        result = subprocess.run(
            [ENGINE_BIN],
            capture_output=True,
            timeout=10
        )
        return result.returncode == 0

    def read_state(self, path: str) -> GameState:
        with open(path, "r") as f:
            data = json.load(f)

        # Leer tick_result
        tr = data.get("tick_result", {})
        tick_result = TickResult(
            tick        = tr.get("tick", 0),
            gold_earned = tr.get("gold_earned", 0),
            status      = tr.get("status", "ongoing")
        )

        # Leer castle — el C++ lo devuelve como "castle_state"
        cs = data.get("castle_state", {})
        castle = Castle(
            id     = cs.get("id", "castle_0"),
            hp     = cs.get("hp", 200),
            hp_max = cs.get("hp_max", 200)
        )

        # Leer horde_queue — el C++ lo devuelve como "horde_state"
        horde_queue = []
        for h in data.get("horde_state", []):
            enemies = []
            for e in h.get("enemies", []):
                p = e.get("pos", {})
                enemies.append(Enemy(
                    id        = e.get("id", ""),
                    hp        = e.get("hp", 0),
                    hp_max    = e.get("hp_max", e.get("hp", 0)),
                    pos       = Pos(p.get("row", 0), p.get("col", 0))
                ))
            horde_queue.append(Horde(
                id      = h.get("id", ""),
                size    = h.get("enemies_alive", 0),
                path    = [],
                enemies = enemies
            ))

        return GameState(
            tick_result = tick_result,
            castle      = castle,
            horde_queue = horde_queue
        )