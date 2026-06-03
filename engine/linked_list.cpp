// =============================================================================
//  Tower Siege — Variant 1
//  Algorithm Developer: Daniel Santiago Pérez Madera
//  Ciencias de la Computación I — 2026-I
//  Universidad Distrital Francisco José de Caldas
// =============================================================================

#include "linked_list.h"
#include <iostream>
#include <sstream>


Enemy::Enemy(const std::string& id,
             Pos pos,
             int hp,
             int hp_max,
             int dmg,
             int atk_range)
    : id(id), pos(pos), hp(hp), hp_max(hp_max),
      dmg(dmg), atk_range(atk_range), next(nullptr), prev(nullptr) // NUEVO: Inicializamos prev
{}


Horde::Horde(const std::string& id, int speed, const std::vector<Pos>& path)
    : id(id), head(nullptr), tail(nullptr), size(0), speed(speed),
      path(path), killed_ids(), head_path_index(0)
{}

// 
// Horde — destructor
// Recorre la lista liberando cada nodo para evitar memory leaks.
// 
Horde::~Horde() {
    Enemy* current = head;
    while (current != nullptr) {
        Enemy* next_enemy = current->next;
        delete current;
        current = next_enemy;
    }
    head = nullptr;
    tail = nullptr;
}

//
// push_back — encola un enemigo al final de la horda
//
// Caso 1: lista vacía  →  el enemigo es cabeza Y cola a la vez.
// Caso 2: lista con elementos  →  el tail actual apunta al nuevo, y tail avanza.
//
//   Antes:  head → [e0] → [e1] → tail (e1)
//   Agregar [e2]:
//   Después: head → [e0] → [e1] → [e2] → tail (e2)
// 
void Horde::push_back(Enemy* enemy) {
    enemy->next = nullptr;         
    if (is_empty()) {
        enemy->prev = nullptr;     
        head = enemy;               
        tail = enemy;
    } else {
        enemy->prev = tail;       
        tail->next = enemy;         
        tail = enemy;           
    }
    size++;
}


void Horde::updatePath(const std::vector<Pos>& new_path) {

    if (is_empty() || new_path.empty() || 
       (new_path[0].row != head->pos.row || new_path[0].col != head->pos.col)) {
        return; // O lanza una excepción, o loguea un error
    }


    this->path = new_path;

    head_path_index = 0;
}


// remove_head — elimina la cabeza (enemigo muerto) y promueve al siguiente
//
// Devuelve el puntero al nodo eliminado.
// El CALLER es responsable de: registrar la recompensa y llamar delete.
//
//   Antes:  head → [e0] → [e1] → [e2]
//   Después: head → [e1] → [e2]   (e0 devuelto para liberar)
Enemy* Horde::remove_head() {
    if (is_empty()) return nullptr;

    Enemy* dead = head;             
    head = head->next;              

    if (head == nullptr) {          // la horda quedó vacía
        tail = nullptr;
    } else {
        head->prev = nullptr;       // NUEVO: la nueva cabeza no tiene a nadie delante
    }

    dead->next = nullptr;           
    dead->prev = nullptr;           // NUEVO: limpiamos el puntero prev del nodo retirado
    size--;

    return dead;                    
}

// advance — mueve la horda un paso por el path
//
// head_path_index: índice actual de la cabeza en el vector path[].
//                  Se incrementa aquí para que el caller lleve el seguimiento.
//
// Devuelve false si el head_path_index ya estaba al final (horda llegó al castillo).
bool Horde::advance() {
    if (is_empty()) return false;

    
    int next_index = head_path_index + 1;
    if (next_index >= (int)path.size()) {
        return false;   
    }
   Enemy* cur = tail;
    
    while (cur != head && cur != nullptr) {
        cur->pos = cur->prev->pos;
        
        cur = cur->prev;
    }

    head_path_index++;
    head->pos = path[head_path_index];

    return true;
}


// is_empty
bool Horde::is_empty() const {
    return head == nullptr;
}

// Enemy::toJson
// Formato esperado (según statePrueba.json):
//   {"id":"enemy_1","hp":80,"pos":{"row":6,"col":0}}
std::string Enemy::toJson() const {
    std::ostringstream oss;
    oss << "{"
        << "\"id\":\""  << id  << "\","
        << "\"hp\":"    << hp  << ","
        << "\"pos\":{"
            << "\"row\":" << pos.row << ","
            << "\"col\":" << pos.col
        << "}"
        << "}";
    return oss.str();
}


// Horde::toJson
// Formato esperado (según statePrueba.json):
//   {
//     "id": "horde_0",
//     "head_pos": {"row": 5, "col": 0},
//     "enemies_alive": 1,
//     "enemies": [ ... ],
//     "killed_ids": ["enemy_0"]
//   }
//
// Notas:
//   - "head_pos" se serializa como {"row":-1,"col":-1} si la horda está vacía.
//   - "enemies" recorre la lista de head a tail en orden.
//   - "killed_ids" refleja el vector killed_ids acumulado por el caller
//     (se llena externamente en remove_head() cuando el motor registra la muerte).
std::string Horde::toJson() const {
    std::ostringstream oss;

    int head_row = is_empty() ? -1 : head->pos.row;
    int head_col = is_empty() ? -1 : head->pos.col;

    oss << "{"
        << "\"id\":\""       << id   << "\","
        << "\"head_pos\":{"
            << "\"row\":" << head_row << ","
            << "\"col\":" << head_col
        << "},"
        << "\"enemies_alive\":" << size << ",";


    oss << "\"enemies\":[";
    Enemy* cur = head;
    bool first_enemy = true;
    while (cur != nullptr) {
        if (!first_enemy) oss << ",";
        oss << cur->toJson();
        first_enemy = false;
        cur = cur->next;
    }
    oss << "],";


    oss << "\"killed_ids\":[";
    for (std::size_t i = 0; i < killed_ids.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "\"" << killed_ids[i] << "\"";
    }
    oss << "]";

    oss << "}";
    return oss.str();
}
void Horde::print_state() const {
    std::cout << "=== Horda [" << id << "] | size=" << size
              << " | speed=" << speed << " ===" << std::endl;

    Enemy* cur = head;
    int index = 0;
    while (cur != nullptr) {
        std::cout << "  [" << index << "] " << cur->id
                  << " hp=" << cur->hp << "/" << cur->hp_max
                  << " pos=(" << cur->pos.row << "," << cur->pos.col << ")"
                  << (cur == head ? " <-- CABEZA" : "")
                  << (cur == tail ? " <-- COLA"   : "")
                  << std::endl;
        cur = cur->next;
        index++;
    }

    if (is_empty()) {
        std::cout << "  [vacía]" << std::endl;
    }
}