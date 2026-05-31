// =============================================================================
// Tower Siege — engine/test_linked_list.cpp
// Prueba rápida de la lista enlazada de horda.
// Compilar: g++ -std=c++17 -o test_horde linked_list.cpp test_linked_list.cpp
// =============================================================================

#include "linked_list.h"
#include <iostream>
#include <cassert>

int main() {

    // -------------------------------------------------------------------------
    // 1. Construir una horda con un path de ejemplo (columna izquierda hacia arriba)
    // -------------------------------------------------------------------------
    std::vector<Pos> path = {
        {8, 0}, {7, 0}, {6, 0}, {5, 0}, {4, 0},
        {3, 0}, {2, 0}, {1, 0}, {0, 4}   // último paso = castillo
    };

    Horde horde("horde_0", 1, path);

    // -------------------------------------------------------------------------
    // 2. push_back — encolar 3 enemigos
    // -------------------------------------------------------------------------
    std::cout << "\n--- Encolando 3 enemigos ---\n";

    horde.push_back(new Enemy("enemy_0", {8, 0}, 80, 80, 10, 1));
    horde.push_back(new Enemy("enemy_1", {8, 0}, 80, 80, 10, 1));  // aún no se han movido
    horde.push_back(new Enemy("enemy_2", {8, 0}, 80, 80, 10, 1));

    horde.print_state();

    assert(horde.size == 3);
    assert(horde.head->id == "enemy_0");
    assert(horde.tail->id == "enemy_2");

    // -------------------------------------------------------------------------
    // 3. advance — mover la horda 2 pasos por el path
    // -------------------------------------------------------------------------
    std::cout << "\n--- Avanzando 2 pasos ---\n";

    horde.advance();   // head_idx -> 1
    horde.print_state();

    horde.advance();   // head_idx -> 2
    horde.print_state();

    // Después de 2 avances:
    //   head  (enemy_0) debería estar en path[2] = (6,0)
    //   enemy_1 en path[1] = (7,0)
    //   enemy_2 en path[0] = (8,0)
    assert(horde.head->pos.row == 6 && horde.head->pos.col == 0);
    assert(horde.head->next->pos.row == 7);

    // -------------------------------------------------------------------------
    // 4. remove_head — simular que la cabeza muere
    // -------------------------------------------------------------------------
    std::cout << "\n--- Murió la cabeza (enemy_0) ---\n";
    Enemy* dead = horde.remove_head();
    std::cout << "Eliminado: " << dead->id << " (recompensa: +30 oro)\n";
    delete dead;   // liberar memoria

    horde.print_state();

    assert(horde.size == 2);
    assert(horde.head->id == "enemy_1");

    // -------------------------------------------------------------------------
    // 5. Vaciar la horda por completo
    // -------------------------------------------------------------------------
    std::cout << "\n--- Vaciando horda ---\n";
    while (!horde.is_empty()) {
        Enemy* e = horde.remove_head();
        std::cout << "Eliminado: " << e->id << "\n";
        delete e;
    }
    horde.print_state();
    assert(horde.is_empty());
    assert(horde.tail == nullptr);

    // -------------------------------------------------------------------------
    // 6. advance sobre lista vacía — no debe crashear
    // -------------------------------------------------------------------------
    std::cout << "\n--- advance sobre horda vacía (no debe crashear) ---\n";
    bool moved = horde.advance();
    assert(moved == false);
    std::cout << "OK, devolvió false correctamente.\n";

    std::cout << "\n=== Todos los tests pasaron ===\n";
    return 0;
}
