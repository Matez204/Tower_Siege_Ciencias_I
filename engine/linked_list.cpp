/*
 * Team: [Número de equipo]
 * Variant: Tower Siege
 * Students: [Nombres del equipo]
 *
 * linked_list.cpp
 * ---------------
 * Implementa la horda de enemigos como una Lista Enlazada Simple.
 * Cada nodo representa un enemigo; el primer nodo es siempre la "cabeza"
 * de la horda (el que recibe daño primero y avanza hacia el castillo).
 *
 * Responsabilidades:
 *   - Cargar enemigos desde el JSON de entrada (horde_queue).
 *   - Mover la cabeza según la velocidad / elapsed_seconds.
 *   - Aplicar daño de torres a la cabeza.
 *   - Eliminar la cabeza al morir y promover al siguiente.
 *   - Detectar si la cabeza alcanza el castillo (Game Over).
 *   - Serializar el estado resultante para state.json.
 */

#include "linked_list.h"

#include <cmath>    // std::abs
#include <stdexcept>

// ─────────────────────────────────────────────────────────────────────────────
// EnemyNode — nodo de la lista
// ─────────────────────────────────────────────────────────────────────────────

EnemyNode::EnemyNode(const std::string& id, int hp, int hp_max,
                     int row, int col, int dmg, int atk_range)
    : id(id), hp(hp), hp_max(hp_max),
      pos{row, col}, dmg(dmg), atk_range(atk_range),
      next(nullptr)
{}

// ─────────────────────────────────────────────────────────────────────────────
// EnemyLinkedList — lista enlazada de una horda
// ─────────────────────────────────────────────────────────────────────────────

EnemyLinkedList::EnemyLinkedList()
    : head(nullptr), size(0)
{}

EnemyLinkedList::~EnemyLinkedList() {
    clear();
}

// Agrega un enemigo al FINAL de la cola (orden de llegada = orden en la fila).
void EnemyLinkedList::push_back(const std::string& id, int hp, int hp_max,
                                int row, int col, int dmg, int atk_range) {
    EnemyNode* nuevo = new EnemyNode(id, hp, hp_max, row, col, dmg, atk_range);
    if (!head) {
        head = nuevo;
    } else {
        EnemyNode* cur = head;
        while (cur->next) cur = cur->next;
        cur->next = nuevo;
    }
    ++size;
}

// Elimina la cabeza (muerto o llegó al castillo) y retorna su id.
// Lanza std::underflow_error si la lista está vacía.
std::string EnemyLinkedList::pop_front() {
    if (!head) throw std::underflow_error("pop_front en lista vacía");
    EnemyNode* viejo = head;
    std::string id   = viejo->id;
    head             = viejo->next;
    delete viejo;
    --size;
    return id;
}

// Acceso a la cabeza sin eliminarla.
EnemyNode* EnemyLinkedList::front() const { return head; }

bool EnemyLinkedList::empty()        const { return head == nullptr; }
int  EnemyLinkedList::get_size()     const { return size; }

// Libera todos los nodos.
void EnemyLinkedList::clear() {
    while (head) {
        EnemyNode* tmp = head;
        head = head->next;
        delete tmp;
    }
    size = 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// HordeManager — gestiona todas las hordas de la partida
// ─────────────────────────────────────────────────────────────────────────────

HordeManager::HordeManager() {}

HordeManager::~HordeManager() {
    for (auto& kv : hordes) delete kv.second;
    hordes.clear();
}

// Crea una nueva horda vacía con la ruta y metadatos dados.
void HordeManager::add_horde(const std::string& horde_id,
                             int original_size,
                             int speed_override,    // -1 = sin override
                             const std::vector<Position>& path) {
    if (hordes.count(horde_id)) return;   // ya existe

    HordeMeta* meta     = new HordeMeta();
    meta->horde_id      = horde_id;
    meta->list          = new EnemyLinkedList();
    meta->original_size = original_size;
    meta->speed_override = speed_override;
    meta->path          = path;
    meta->path_index    = 0;   // índice actual en el camino

    hordes[horde_id] = meta;
}

// Agrega un enemigo a una horda existente.
void HordeManager::add_enemy_to_horde(const std::string& horde_id,
                                      const std::string& enemy_id,
                                      int hp, int hp_max,
                                      int row, int col,
                                      int dmg, int atk_range) {
    auto it = hordes.find(horde_id);
    if (it == hordes.end()) return;
    it->second->list->push_back(enemy_id, hp, hp_max, row, col, dmg, atk_range);
}

// ─────────────────────────────────────────────────────────────────────────────
// Movimiento de la cabeza
//
// Regla de velocidad (Descripcion.md):
//   - Los primeros 10 s: la horda avanza 1 casilla cada 2 s  (0.5 casillas/s).
//   - Después: velocidad ∝ 1 / tamaño_original  (horda grande = más lenta).
//     Aquí usamos: casillas_por_segundo = BASE_SPEED / original_size
//     donde BASE_SPEED = 2.0 da un ritmo razonable para el juego.
//
// Si hay speed_override (≥ 0) se usa directamente como casillas/s.
// ─────────────────────────────────────────────────────────────────────────────

static constexpr double BASE_SPEED      = 2.0;   // casillas/s para horda de tamaño 1
static constexpr double EARLY_SPEED     = 0.5;   // casillas/s durante los primeros 10 s
static constexpr double EARLY_THRESHOLD = 10.0;  // segundos

// Retorna la velocidad efectiva en casillas/segundo.
static double compute_speed(const HordeMeta* meta, double elapsed) {
    if (meta->speed_override >= 0)
        return static_cast<double>(meta->speed_override);
    if (elapsed < EARLY_THRESHOLD)
        return EARLY_SPEED;
    int sz = meta->original_size > 0 ? meta->original_size : 1;
    return BASE_SPEED / static_cast<double>(sz);
}

// Avanza la cabeza de la horda según dt y elapsed_seconds.
// Retorna el número de pasos realizados (puede ser 0).
int HordeManager::advance_horde(const std::string& horde_id,
                                double dt,
                                double elapsed_seconds) {
    auto it = hordes.find(horde_id);
    if (it == hordes.end()) return 0;

    HordeMeta* meta = it->second;
    if (meta->list->empty()) return 0;

    double speed       = compute_speed(meta, elapsed_seconds);
    double steps_float = speed * dt;
    int steps          = static_cast<int>(std::floor(steps_float));

    // Acumulamos el residuo para el siguiente tick.
    meta->step_accum += steps_float;
    steps             = static_cast<int>(std::floor(meta->step_accum));
    meta->step_accum -= static_cast<double>(steps);

    EnemyNode* cab = meta->list->front();

    for (int s = 0; s < steps; ++s) {
        // Buscar el siguiente índice en la ruta a partir de la posición actual.
        int next_idx = -1;
        for (int i = meta->path_index + 1;
             i < static_cast<int>(meta->path.size()); ++i) {
            const Position& wp = meta->path[i];
            if (wp.row != cab->pos.row || wp.col != cab->pos.col) {
                next_idx = i;
                break;
            }
        }
        if (next_idx == -1) break;   // ya llegó al destino final

        cab->pos       = meta->path[next_idx];
        meta->path_index = next_idx;
    }

    return steps;
}

// ─────────────────────────────────────────────────────────────────────────────
// Combate: aplica daño de una torre a la cabeza de la horda.
// Retorna true si la cabeza murió.
// ─────────────────────────────────────────────────────────────────────────────

bool HordeManager::apply_damage_to_head(const std::string& horde_id,
                                        int damage,
                                        KillResult& out_kill) {
    auto it = hordes.find(horde_id);
    if (it == hordes.end()) return false;

    HordeMeta* meta = it->second;
    if (meta->list->empty()) return false;

    EnemyNode* cab = meta->list->front();
    cab->hp -= damage;

    if (cab->hp <= 0) {
        out_kill.killed_id   = cab->id;
        out_kill.gold_reward = compute_gold_reward(cab);
        meta->list->pop_front();
        return true;
    }
    return false;
}

// Recompensa base: 10 + 5 × (hp_max / 20).
int HordeManager::compute_gold_reward(const EnemyNode* enemy) const {
    return 10 + 5 * (enemy->hp_max / 20);
}

// ─────────────────────────────────────────────────────────────────────────────
// Detección de alcance entre torre y cabeza de horda.
// Usa distancia de Chebyshev (casillas en cuadrícula 8-direccional).
// ─────────────────────────────────────────────────────────────────────────────

bool HordeManager::is_head_in_range(const std::string& horde_id,
                                    int tower_row, int tower_col,
                                    int atk_range) const {
    auto it = hordes.find(horde_id);
    if (it == hordes.end()) return false;

    const EnemyLinkedList* lst = it->second->list;
    if (lst->empty()) return false;

    const EnemyNode* cab = lst->front();
    int dr = std::abs(cab->pos.row - tower_row);
    int dc = std::abs(cab->pos.col - tower_col);
    return (dr <= atk_range && dc <= atk_range);
}

// ─────────────────────────────────────────────────────────────────────────────
// Detección Game Over: la cabeza llegó al castillo.
// ─────────────────────────────────────────────────────────────────────────────

bool HordeManager::head_reached_castle(const std::string& horde_id,
                                       int castle_row, int castle_col) const {
    auto it = hordes.find(horde_id);
    if (it == hordes.end()) return false;

    const EnemyLinkedList* lst = it->second->list;
    if (lst->empty()) return false;

    const EnemyNode* cab = lst->front();
    return (cab->pos.row == castle_row && cab->pos.col == castle_col);
}

// ─────────────────────────────────────────────────────────────────────────────
// Serialización a JSON (sin librerías externas, construcción manual).
// ─────────────────────────────────────────────────────────────────────────────

// Convierte la lista de una horda al bloque "horde_state[i]" del state.json.
std::string HordeManager::serialize_horde_state(
        const std::string& horde_id,
        const std::vector<std::string>& killed_ids) const {

    auto it = hordes.find(horde_id);
    if (it == hordes.end()) return "{}";

    const HordeMeta* meta = it->second;
    const EnemyLinkedList* lst = meta->list;

    // head_pos
    std::string head_pos = "{\"row\":0,\"col\":0}";
    if (!lst->empty()) {
        const EnemyNode* cab = lst->front();
        head_pos = "{\"row\":" + std::to_string(cab->pos.row) +
                   ",\"col\":" + std::to_string(cab->pos.col) + "}";
    }

    // enemies array
    std::string enemies_arr = "[";
    bool first = true;
    EnemyNode* cur = lst->front();
    while (cur) {
        if (!first) enemies_arr += ",";
        enemies_arr += "{\"id\":\"" + cur->id + "\","
                       "\"hp\":"    + std::to_string(cur->hp) + ","
                       "\"pos\":{\"row\":" + std::to_string(cur->pos.row) +
                       ",\"col\":"        + std::to_string(cur->pos.col) + "}}";
        first = false;
        cur = cur->next;
    }
    enemies_arr += "]";

    // killed_ids array
    std::string killed_arr = "[";
    for (std::size_t i = 0; i < killed_ids.size(); ++i) {
        if (i > 0) killed_arr += ",";
        killed_arr += "\"" + killed_ids[i] + "\"";
    }
    killed_arr += "]";

    return "{"
           "\"id\":\"" + horde_id + "\","
           "\"head_pos\":" + head_pos + ","
           "\"enemies_alive\":" + std::to_string(lst->get_size()) + ","
           "\"enemies\":" + enemies_arr + ","
           "\"killed_ids\":" + killed_arr +
           "}";
}

// Retorna la lista de metadatos de todas las hordas activas.
const std::unordered_map<std::string, HordeMeta*>&
HordeManager::get_all_hordes() const {
    return hordes;
}
