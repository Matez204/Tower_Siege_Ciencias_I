// =============================================================================
// Tower Siege — engine/linked_list.h
// Equipo: [Número de equipo] | Variante: [Nombre]
// Integrantes: [Nombres]
//
// Define las estructuras Enemy (nodo) y Horde (lista enlazada tipo cola).
// La horda funciona como una QUEUE:
//   - Encolar  → push_back()  : nuevo enemigo llega por la cola (tail)
//   - Desencolar → remove_head(): la cabeza muere y la sigue el siguiente
// =============================================================================

#pragma once
#include <string>
#include <vector>

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
};

// ---------------------------------------------------------------------------
// Horde — la lista enlazada completa de una oleada
// ---------------------------------------------------------------------------
struct Horde {
    std::string        id;
    Enemy*             head;       // cabeza: el enemigo más adelantado (más cerca del castillo)
    Enemy*             tail;       // cola:  el último en llegar
    int                size;       // cantidad actual de enemigos vivos
    int                speed;      // casillas que avanza por tick (calculado según tamaño)
    std::vector<Pos>   path;       // ruta calculada por Python

    // Constructor / destructor
    Horde(const std::string& id, int speed, const std::vector<Pos>& path);
    ~Horde();   // libera todos los nodos

    // --- Operaciones principales ---

    // Agrega un enemigo al final de la cola (nueva llegada a la horda)
    void push_back(Enemy* enemy);

    // Elimina la cabeza (murió) y promueve al siguiente como nueva cabeza.
    // Devuelve el puntero al nodo eliminado para que el caller libere memoria o registre la recompensa.
    Enemy* remove_head();

    // Mueve toda la horda un paso a lo largo del path.
    // La cabeza avanza a path[head_path_index], cada enemigo toma la pos del que tenía delante.
    // Devuelve false si la horda llegó al castillo (no hay más pasos en el path).
    bool advance(int& head_path_index);

    // Utilidades
    bool  is_empty() const;
    void  print_state() const;   // útil para debug
};
