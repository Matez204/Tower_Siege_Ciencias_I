// =============================================================================
//  Tower Siege — Variant 1
//  Algorithm Developer: Daniel Santiago Pérez Madera
//  Ciencias de la Computación I — 2026-I
//  Universidad Distrital Francisco José de Caldas
// =============================================================================

#pragma once
#include <string>
#include <vector>
#include <sstream>

// ---------------------------------------------------------------------------
// Posición en la cuadrícula
// ---------------------------------------------------------------------------
struct Pos {
    int row;
    int col;
};

// ---------------------------------------------------------------------------
// Enemy — un nodo de la lista enlazada
// ---------------------------------------------------------------------------
struct Enemy {
    std::string id;
    Pos         pos;
    int         hp;
    int         hp_max;
    int         dmg;
    int         atk_range;
    Enemy* next;       // apunta al enemigo que viene detrás (hacia la cola)
    Enemy* prev;       // NUEVO: apunta al enemigo que viene delante (hacia la cabeza)

    // Constructor para inicializar limpiamente
    Enemy(const std::string& id,
          Pos pos,
          int hp,
          int hp_max,
          int dmg,
          int atk_range);

    // Serialización JSON según el formato del estado de prueba.
    // Produce: {"id":"...","hp":N,"pos":{"row":R,"col":C}}
    std::string toJson() const;
};

// ---------------------------------------------------------------------------
// Horde — la lista enlazada completa de una oleada
// ---------------------------------------------------------------------------
struct Horde {
    std::string id;
    Enemy* head;   
    Enemy* tail;    
    int size;      
    int speed;      
    std::vector<Pos>         path;      
    std::vector<std::string> killed_ids;
    int head_path_index;

    // Constructor / destructor
    Horde(const std::string& id, int speed, const std::vector<Pos>& path);
    ~Horde();   // libera todos los nodos


    // Agrega un enemigo al final de la cola (nueva llegada a la horda)
    void push_back(Enemy* enemy);
    
    void updatePath(const std::vector<Pos>& new_path);

    // Elimina la cabeza (murió) y promueve al siguiente como nueva cabeza.
    // Devuelve el puntero al nodo eliminado para que el caller libere memoria o registre la recompensa.
    Enemy* remove_head();

    // Mueve toda la horda un paso a lo largo del path.
    // La cabeza avanza a path[head_path_index], cada enemigo toma la pos del que tenía delante.
    // Devuelve false si la horda llegó al castillo (no hay más pasos en el path).
    bool advance();

    // Utilidades
    bool  is_empty() const;
    void  print_state() const;   // útil para debug

    // Serialización JSON según el formato del estado de prueba.
    // Produce:
    // {
    //   "id": "...",
    //   "head_pos": {"row": R, "col": C},
    //   "enemies_alive": N,
    //   "enemies": [ {toJson de cada Enemy} ],
    //   "killed_ids": ["id0", "id1", ...]
    // }
    std::string toJson() const;
};