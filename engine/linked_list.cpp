// =============================================================================
// Tower Siege — engine/linked_list.cpp
// Equipo: [Número de equipo] | Variante: [Nombre]
// Integrantes: [Nombres]
//
// Implementación de la horda como lista enlazada (cola / queue).
//
// Mecánica central:
//   - La CABEZA (head) es el enemigo más cercano al castillo.
//   - La COLA (tail) es el último enemigo en entrar a la horda.
//   - Los punteros `next` van de head hacia tail  →  head → e1 → e2 → tail
//   - Cuando la cabeza muere: se hace remove_head() y el e1 pasa a ser head.
//   - Cuando avanza la horda: head toma la nueva posición en el path, y cada
//     enemigo toma la posición que tenía el que estaba delante de él.
// =============================================================================

#include "linked_list.h"
#include <iostream>
#include <sstream>

// =============================================================================
// Enemy — constructor
// =============================================================================
Enemy::Enemy(const std::string& id,
             Pos pos,
             int hp,
             int hp_max,
             int dmg,
             int atk_range)
    : id(id), pos(pos), hp(hp), hp_max(hp_max),
      dmg(dmg), atk_range(atk_range), next(nullptr), prev(nullptr) // NUEVO: Inicializamos prev
{}

// =============================================================================
// Horde — constructor
// =============================================================================
Horde::Horde(const std::string& id, int speed, const std::vector<Pos>& path)
    : id(id), head(nullptr), tail(nullptr), size(0), speed(speed),
      path(path), killed_ids(), head_path_index(0)
{}

// =============================================================================
// Horde — destructor
// Recorre la lista liberando cada nodo para evitar memory leaks.
// =============================================================================
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

// =============================================================================
// push_back — encola un enemigo al final de la horda
//
// Caso 1: lista vacía  →  el enemigo es cabeza Y cola a la vez.
// Caso 2: lista con elementos  →  el tail actual apunta al nuevo, y tail avanza.
//
//   Antes:  head → [e0] → [e1] → tail (e1)
//   Agregar [e2]:
//   Después: head → [e0] → [e1] → [e2] → tail (e2)
// =============================================================================
void Horde::push_back(Enemy* enemy) {
    enemy->next = nullptr;          // el nuevo siempre es el último

    if (is_empty()) {
        enemy->prev = nullptr;      // NUEVO: no tiene a nadie delante
        head = enemy;               
        tail = enemy;
    } else {
        enemy->prev = tail;         // NUEVO: el nuevo enemigo apunta hacia adelante al antiguo tail
        tail->next = enemy;         // el tail actual apunta al nuevo
        tail = enemy;               // el nuevo pasa a ser el tail
    }
    size++;
}


void Horde::updatePath(const std::vector<Pos>& new_path) {
    // 1. Validación de seguridad: 
    // Aseguramos que el nuevo camino sea coherente (comienza donde está la cabeza)
    if (is_empty() || new_path.empty() || 
       (new_path[0].row != head->pos.row || new_path[0].col != head->pos.col)) {
        return; // O lanza una excepción, o loguea un error
    }

    // 2. Reemplazo directo (O(1) si usamos move, O(N) si copiamos)
    // Esto descarta el camino antiguo y pone el nuevo.
    this->path = new_path;

    // 3. ¡La parte crucial! 
    // Resetear el índice. Como el nuevo camino empieza en new_path[0],
    // que es donde está la cabeza (head->pos), el índice ahora debe ser 0.
    head_path_index = 0;
}
// =============================================================================
// remove_head — elimina la cabeza (enemigo muerto) y promueve al siguiente
//
// Devuelve el puntero al nodo eliminado.
// El CALLER es responsable de: registrar la recompensa y llamar delete.
//
//   Antes:  head → [e0] → [e1] → [e2]
//   Después: head → [e1] → [e2]   (e0 devuelto para liberar)
// =============================================================================

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
    head_path_index--;
    return dead;                    
}
// =============================================================================
// advance — mueve la horda un paso por el path
//
// head_path_index: índice actual de la cabeza en el vector path[].
//                  Se incrementa aquí para que el caller lleve el seguimiento.
//
// Lógica de movimiento (cola de serpiente):
//   Cada enemigo hereda la posición del que estaba delante de él.
//   Se recorre la lista de atrás hacia adelante lógicamente, pero como solo
//   tenemos punteros hacia adelante, usamos un arreglo auxiliar de punteros.
//
//   Ejemplo con path = [(8,0),(7,0),(6,0),(5,0)] y head_path_index = 1:
//     Antes:  head(7,0) → e1(8,0)
//     Avanzar: head_path_index pasa a 2
//     e1 toma la pos de head → e1 queda en (7,0)
//     head toma path[2]      → head queda en (6,0)
//     Después: head(6,0) → e1(7,0)
//
// Devuelve false si el head_path_index ya estaba al final (horda llegó al castillo).
// =============================================================================
bool Horde::advance() {
    if (is_empty()) return false;

    // ¿Queda algún paso en el path?
    int next_index = head_path_index + 1;
    if (next_index >= (int)path.size()) {
        return false;   // la cabeza ya está en el castillo, no puede avanzar más
    }

    // NUEVA LÓGICA OPTIMIZADA: Lista doblemente enlazada
    // Iteramos desde la cola (tail) hacia la cabeza usando el puntero 'prev'.
    // Esto elimina la necesidad del std::vector auxiliar.
    Enemy* cur = tail;
    
    // Mientras no lleguemos a la cabeza (la cabeza se actualiza al final)
    while (cur != head && cur != nullptr) {
        // Hereda la posición del enemigo que tiene delante
        cur->pos = cur->prev->pos;
        
        // Retrocedemos hacia la cabeza
        cur = cur->prev;
    }

    // La cabeza avanza al siguiente paso del path de forma independiente.
    head_path_index++;
    head->pos = path[head_path_index];


    return true;
}

// =============================================================================
// is_empty
// =============================================================================
bool Horde::is_empty() const {
    return head == nullptr;
}

// =============================================================================
// Enemy::toJson
// Formato esperado (según statePrueba.json):
//   {"id":"enemy_1","hp":80,"pos":{"row":6,"col":0}}
// =============================================================================
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

// =============================================================================
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
// =============================================================================
std::string Horde::toJson() const {
    std::ostringstream oss;

    // --- head_pos ---
    // Si la horda está vacía usamos centinela (-1,-1) para no dejar el campo en null.
    int head_row = is_empty() ? -1 : head->pos.row;
    int head_col = is_empty() ? -1 : head->pos.col;

    oss << "{"
        << "\"id\":\""       << id   << "\","
        << "\"head_pos\":{"
            << "\"row\":" << head_row << ","
            << "\"col\":" << head_col
        << "},"
        << "\"enemies_alive\":" << size << ",";

    // --- enemies: recorre head → tail ---
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

    // --- killed_ids ---
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