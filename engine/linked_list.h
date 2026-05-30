/*
 * Team: [Número de equipo]
 * Variant: Tower Siege
 * Students: [Nombres del equipo]
 *
 * linked_list.h
 * -------------
 * Declaraciones públicas de la Lista Enlazada de la Horda y su gestor.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>

// ─────────────────────────────────────────────────────────────────────────────
// Tipos auxiliares
// ─────────────────────────────────────────────────────────────────────────────

struct Position {
    int row;
    int col;
};

struct KillResult {
    std::string killed_id;
    int         gold_reward = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// EnemyNode — nodo de la lista (un enemigo)
// ─────────────────────────────────────────────────────────────────────────────

struct EnemyNode {
    std::string id;
    int         hp;
    int         hp_max;
    Position    pos;
    int         dmg;
    int         atk_range;
    EnemyNode*  next;

    EnemyNode(const std::string& id, int hp, int hp_max,
              int row, int col, int dmg, int atk_range);
};

// ─────────────────────────────────────────────────────────────────────────────
// EnemyLinkedList — cola de enemigos de una horda
// ─────────────────────────────────────────────────────────────────────────────

class EnemyLinkedList {
public:
    EnemyLinkedList();
    ~EnemyLinkedList();

    void        push_back(const std::string& id, int hp, int hp_max,
                          int row, int col, int dmg, int atk_range);
    std::string pop_front();     // elimina y retorna id de la cabeza
    EnemyNode*  front() const;

    bool        empty()     const;
    int         get_size()  const;
    void        clear();

private:
    EnemyNode* head;
    int        size;
};

// ─────────────────────────────────────────────────────────────────────────────
// HordeMeta — metadatos de una horda (lista + ruta + acumulador de pasos)
// ─────────────────────────────────────────────────────────────────────────────

struct HordeMeta {
    std::string              horde_id;
    EnemyLinkedList*         list          = nullptr;
    int                      original_size = 0;
    int                      speed_override = -1;   // -1 = usar fórmula
    std::vector<Position>    path;
    int                      path_index    = 0;
    double                   step_accum    = 0.0;   // fracción de paso acumulada
};

// ─────────────────────────────────────────────────────────────────────────────
// HordeManager — gestiona el conjunto de hordas activas
// ─────────────────────────────────────────────────────────────────────────────

class HordeManager {
public:
    HordeManager();
    ~HordeManager();

    // Construcción
    void add_horde(const std::string& horde_id,
                   int original_size,
                   int speed_override,
                   const std::vector<Position>& path);

    void add_enemy_to_horde(const std::string& horde_id,
                            const std::string& enemy_id,
                            int hp, int hp_max,
                            int row, int col,
                            int dmg, int atk_range);

    // Lógica de juego
    int  advance_horde(const std::string& horde_id,
                       double dt,
                       double elapsed_seconds);

    bool apply_damage_to_head(const std::string& horde_id,
                              int damage,
                              KillResult& out_kill);

    bool is_head_in_range(const std::string& horde_id,
                          int tower_row, int tower_col,
                          int atk_range) const;

    bool head_reached_castle(const std::string& horde_id,
                             int castle_row, int castle_col) const;

    // Serialización
    std::string serialize_horde_state(
            const std::string& horde_id,
            const std::vector<std::string>& killed_ids) const;

    const std::unordered_map<std::string, HordeMeta*>& get_all_hordes() const;

private:
    std::unordered_map<std::string, HordeMeta*> hordes;

    int compute_gold_reward(const EnemyNode* enemy) const;
};
