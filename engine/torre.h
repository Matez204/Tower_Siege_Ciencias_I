#pragma once
#include <string>
#include "linked_list.h"   // Pos, Enemy, Horde

class Torre {
public:
    // Constructor
    Torre(const std::string& id, int hp, Pos pos, int atk_range, int dmg);

    
    // Combate
    // Retorna true si la posición objetivo está dentro del rango de ataque
    bool inRange(Pos target) const;

    // Ataca a la cabeza de la horda si está en rango y la torre está viva.
    //
    // Flujo:
    //   1. Verifica que la torre esté viva y que la horda no esté vacía.
    //   2. Comprueba si la cabeza (horde.head) está dentro del rango de ataque.
    //   3. Resta dmg_ a la vida de la cabeza.
    //   4. Si la cabeza llega a 0 HP, llama a horde.remove_head() y devuelve
    //      el puntero al Enemy muerto (el caller debe liberar memoria y
    //      registrar la recompensa de oro).
    //   5. Si la cabeza sobrevive, devuelve nullptr.
    //
    // Devuelve:
    //   Enemy* → la cabeza murió; puntero al nodo eliminado (pendiente de free/delete)
    //   nullptr → la cabeza sobrevivió, estaba fuera de rango, o la torre está muerta
    Enemy* attack(Horde& horde);

    // Recibe daño (preparado para mecánicas futuras donde enemigos atacan torres)
    void takeDamage(int amount);

    // Serialización liviana para el JSON bridge con Python
    std::string toJson() const;

public:
    std::string id_;
    int         hp_;
    Pos         pos_;
    int         atk_range_;
    int         dmg_;
};
