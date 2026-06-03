#include "torre.h"
#include <cmath>
#include <sstream>
#include <algorithm>

// ─────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────
Torre::Torre(const std::string& id, int hp, Pos pos, int atk_range, int dmg)
    : id_(id), hp_(hp), pos_(pos), atk_range_(atk_range), dmg_(dmg) {}

// ─────────────────────────────────────────────
// Getters
// ─────────────────────────────────────────────
// const std::string& Torre::getId()       const { return id_; }
// int                Torre::getHp()       const { return hp_; }
// Pos                Torre::getPos()      const { return pos_; }
// int                Torre::getAtkRange() const { return atk_range_; }
// int                Torre::getDmg()      const { return dmg_; }
// bool               Torre::isAlive()     const { return hp_ > 0; }

// ─────────────────────────────────────────────
// Combate
// ─────────────────────────────────────────────

// Distancia Manhattan entre la torre y la posición objetivo.
// Se usa Manhattan en vez de Euclidiana porque el movimiento
// de las hordas ocurre en una cuadrícula (casilla a casilla).
bool Torre::inRange(Pos target) const {
    int dist = std::abs(pos_.row - target.row)
             + std::abs(pos_.col - target.col);
    return dist <= atk_range_;
}

// Ataca a la cabeza de la horda si está en rango.
//
// Flujo:
//   1. Verifica que la torre esté viva y la horda no esté vacía.
//   2. Comprueba si horde.head está dentro del rango de ataque.
//   3. Resta dmg_ al HP de la cabeza.
//   4. Si la cabeza llega a 0 HP:
//        - Fija su HP en 0 (evita valores negativos en el log de eventos).
//        - Llama a horde.remove_head(), que desvincula el nodo y promueve
//          al siguiente enemigo como nueva cabeza.
//        - Devuelve el puntero al nodo eliminado → el caller debe:
//            a) registrar el evento ENEMY_KILLED y sumar el oro, y
//            b) liberar la memoria con delete.
//   5. Si la cabeza sobrevive o está fuera de rango → devuelve nullptr.
Enemy* Torre::attack(Horde& horde) {
    // Precondiciones: torre viva y horda con al menos un enemigo
    if (hp_<=0 || horde.is_empty()) return nullptr;

    Enemy* head = horde.head;

    // Solo atacar si la cabeza está dentro del rango de ataque
    if (!inRange(head->pos)) return nullptr;

    // Aplicar daño
    head->hp -= dmg_;

    // Si la cabeza murió, sacarla de la lista y devolver el nodo al caller
    if (head->hp <= 0) {
        head->hp = 0;
        return horde.remove_head();
    }

    return nullptr;
}

// Permite que la torre reciba daño (preparado para mecánicas futuras).
void Torre::takeDamage(int amount) {
    if (amount < 0) return;
    hp_ = std::max(0, hp_ - amount);
}

// ─────────────────────────────────────────────
// Serialización (JSON bridge con Python)
// ─────────────────────────────────────────────
std::string Torre::toJson() const {
    std::ostringstream oss;
    oss << "{"
        << "\"id\":"        << "\"" << id_ << "\","
        << "\"hp\":"        << hp_  << ","
        << "\"pos\":{"
            << "\"row\":"   << pos_.row << ","
            << "\"col\":"   << pos_.col
        << "},"
        << "\"atk_range\":" << atk_range_ << ","
        << "\"dmg\":"       << dmg_
        << "}";
    return oss.str();
}