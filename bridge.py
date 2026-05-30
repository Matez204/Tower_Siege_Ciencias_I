# bridge.py
import json
import os
import subprocess
from dataclasses import dataclass, field
from typing import List, Dict, Any

_BASE_DIR = os.path.dirname(os.path.abspath(__file__))
INPUT_PATH = os.path.join(_BASE_DIR, "data", "input.json")
STATE_PATH = os.path.join(_BASE_DIR, "data", "state.json")
ENGINE_BIN = os.path.join(_BASE_DIR, "engine", "tower_siege")

@dataclass
class Pos:
    row: int
    col: int
    def to_dict(self): return {"row": self.row, "col": self.col}

@dataclass
class Tower:
    id: str
    pos: Pos
    atk_range: int
    dmg: int
    def to_dict(self):
        return {"id": self.id, "pos": self.pos.to_dict(), "range": self.atk_range, "dmg": self.dmg}

@dataclass
class Castle:
    id: str
    hp: int
    hp_max: int
    def to_dict(self): return {"id": self.id, "hp": self.hp, "hp_max": self.hp_max}

@dataclass
class Horde:
    id: str
    size: int
    path: List[Pos]
    enemies: List[Any]

@dataclass
class InputState:
    meta: Dict[str, Any]
    castle: Castle
    towers: List[Tower]
    horde_queue: List[Horde]

    def to_dict(self):
        return {
            "meta": self.meta,
            "castle": self.castle.to_dict(),
            "towers": [t.to_dict() for t in self.towers],
            "horde_queue": []
        }

class Bridge:
    def __init__(self):
        self.input_path = INPUT_PATH
        self.state_path = STATE_PATH

    def write_input(self, input_state: InputState):
        with open(self.input_path, "w") as f:
            json.dump(input_state.to_dict(), f, indent=4)

    def run_engine_tick(self) -> bool:
        if os.path.exists(ENGINE_BIN):
            subprocess.run([ENGINE_BIN])
            return True
        return False

    def read_state(self, path: str):
        # Esta función cargará el estado que el C++ escriba
        with open(path, "r") as f:
            return json.load(f) # Simplificado para pruebas rápidas