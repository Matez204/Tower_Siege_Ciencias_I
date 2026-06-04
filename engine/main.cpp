// ============================================================
// Tower Siege — Variant 1
// Engine Developer: [nombre compañero]
// Ciencias de la Computación I — 2026-I
// Universidad Distrital Francisco José de Caldas
//
// Compilar (desde carpeta engine/):
//   g++ -std=c++17 -o tower_siege.exe main.cpp linked_list.cpp tower.cpp
// ============================================================
 
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "linked_list.h"
#include "torre.h"
 
// =============================================================================
// LECTOR JSON MANUAL
// =============================================================================
 
std::string leer_archivo(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}
 
void escribir_archivo(const std::string& path, const std::string& contenido) {
    std::ofstream f(path);
    f << contenido;
}
 
int leer_int(const std::string& json, const std::string& clave, size_t start = 0) {
    std::string buscar = "\"" + clave + "\":";
    size_t pos = json.find(buscar, start);
    if (pos == std::string::npos) return 0;
    pos += buscar.size();
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\n')) pos++;
    size_t fin = pos;
    if (fin < json.size() && json[fin] == '-') fin++;
    while (fin < json.size() && std::isdigit((unsigned char)json[fin])) fin++;
    if (fin == pos) return 0;
    return std::stoi(json.substr(pos, fin - pos));
}
 
std::string leer_string(const std::string& json, const std::string& clave, size_t start = 0) {
    std::string buscar = "\"" + clave + "\":\"";
    size_t pos = json.find(buscar, start);
    if (pos == std::string::npos) return "";
    pos += buscar.size();
    size_t fin = json.find("\"", pos);
    return json.substr(pos, fin - pos);
}
 
std::string leer_array(const std::string& json, const std::string& clave, size_t start = 0) {
    std::string buscar = "\"" + clave + "\":";
    size_t pos = json.find(buscar, start);
    if (pos == std::string::npos) return "";
    pos = json.find("[", pos);
    if (pos == std::string::npos) return "";
    int depth = 0;
    size_t ini = pos;
    for (size_t i = pos; i < json.size(); i++) {
        if (json[i] == '[') depth++;
        else if (json[i] == ']') {
            depth--;
            if (depth == 0) return json.substr(ini + 1, i - ini - 1);
        }
    }
    return "";
}
 
std::vector<std::string> dividir_objetos(const std::string& array_str) {
    std::vector<std::string> objetos;
    int depth = 0;
    size_t ini = std::string::npos;
    for (size_t i = 0; i < array_str.size(); i++) {
        if (array_str[i] == '{') {
            if (depth == 0) ini = i;
            depth++;
        } else if (array_str[i] == '}') {
            depth--;
            if (depth == 0 && ini != std::string::npos) {
                objetos.push_back(array_str.substr(ini, i - ini + 1));
                ini = std::string::npos;
            }
        }
    }
    return objetos;
}
 
// =============================================================================
// MAIN
// =============================================================================
int main() {
    std::string input_path = "data/input.json";
    std::string state_path = "data/state.json";
 
    // ── 1. LEER input.json ────────────────────────────────────
    std::string json = leer_archivo(input_path);
    if (json.empty()) {
        std::cerr << "[Engine] Error: no se pudo leer " << input_path << std::endl;
        return 1;
    }
 
    int tick = leer_int(json, "tick");
    int gold = leer_int(json, "gold");
 
    // Leer castle (buscar el objeto "castle": { ... })
    int castle_hp  = 200;
    int castle_max = 200;
    size_t castle_pos = json.find("\"castle\":");
    if (castle_pos != std::string::npos) {
        size_t ini = json.find("{", castle_pos);
        // encontrar cierre del objeto castle
        size_t fin = ini + 1;
        int d = 1;
        while (fin < json.size() && d > 0) {
            if (json[fin] == '{') d++;
            else if (json[fin] == '}') d--;
            fin++;
        }
        std::string castle_str = json.substr(ini, fin - ini);
        int hp_val  = leer_int(castle_str, "hp");
        int max_val = leer_int(castle_str, "hp_max");
        if (hp_val  > 0) castle_hp  = hp_val;
        if (max_val > 0) castle_max = max_val;
    }
 
    // ── 2. CONSTRUIR TORRES ───────────────────────────────────
    std::vector<Torre*> torres;
    std::string towers_array = leer_array(json, "towers");
    std::vector<std::string> tower_objs = dividir_objetos(towers_array);
 
    for (size_t i = 0; i < tower_objs.size(); i++) {
        const std::string& t = tower_objs[i];
        std::string t_id = leer_string(t, "id");
        int t_hp         = leer_int(t, "hp");
        int t_range      = leer_int(t, "atk_range");
        int t_dmg        = leer_int(t, "dmg");
        size_t pos_ini   = t.find("\"pos\":");
        int t_row = 0, t_col = 0;
        if (pos_ini != std::string::npos) {
            t_row = leer_int(t, "row", pos_ini);
            t_col = leer_int(t, "col", pos_ini);
        }
        if (t_hp    <= 0) t_hp    = 100;
        if (t_range <= 0) t_range = 2;
        if (t_dmg   <= 0) t_dmg   = 20;
        if (t_id.empty()) t_id    = "tower_" + std::to_string(i);
        torres.push_back(new Torre(t_id, t_hp, {t_row, t_col}, t_range, t_dmg));
    }
 
    // ── 3. CONSTRUIR HORDA ────────────────────────────────────
    std::vector<Pos> path;
    std::string horde_array = leer_array(json, "horde_queue");
    std::vector<std::string> horde_objs = dividir_objetos(horde_array);
 
    if (!horde_objs.empty()) {
        std::string path_array = leer_array(horde_objs[0], "path");
        std::vector<std::string> path_objs = dividir_objetos(path_array);
        for (const std::string& p : path_objs) {
            int r = leer_int(p, "row");
            int c = leer_int(p, "col");
            path.push_back({r, c});
        }
    }
 
    // Path por defecto si no viene en el JSON
    if (path.empty()) {
        path = {{8,4},{7,4},{6,4},{5,4},{4,4},{3,4},{2,4},{1,4},{0,4}};
    }
 
    Horde horda("horde_0", 1, path);
 
    // Cargar enemigos desde JSON
    if (!horde_objs.empty()) {
        std::string enemies_array = leer_array(horde_objs[0], "enemies");
        std::vector<std::string> enemy_objs = dividir_objetos(enemies_array);
 
        for (const std::string& e : enemy_objs) {
            std::string e_id = leer_string(e, "id");
            int e_hp         = leer_int(e, "hp");
            int e_hp_max     = leer_int(e, "hp_max");
            int e_dmg        = leer_int(e, "dmg");
            int e_range      = leer_int(e, "atk_range");
            size_t pos_ini   = e.find("\"pos\":");
            int e_row = 0, e_col = 0;
            if (pos_ini != std::string::npos) {
                e_row = leer_int(e, "row", pos_ini);
                e_col = leer_int(e, "col", pos_ini);
            }
            if (e_hp_max <= 0) e_hp_max = e_hp;
            if (e_dmg   <= 0) e_dmg    = 10;
            if (e_range <= 0) e_range  = 1;
 
            horda.push_back(new Enemy(e_id, {e_row, e_col}, e_hp, e_hp_max, e_dmg, e_range));
        }
 
        // Sincronizar head_path_index con la posición de la CABEZA (primer enemigo)
        // La cabeza es el enemigo más adelantado — el primero en el JSON
        if (!horda.is_empty()) {
            int head_row = horda.head->pos.row;
            int head_col = horda.head->pos.col;
            for (size_t pi = 0; pi < path.size(); pi++) {
                if (path[pi].row == head_row && path[pi].col == head_col) {
                    horda.head_path_index = (int)pi;
                    break;
                }
            }
        }
    }
 
    // Horda de prueba si no vienen enemigos
    if (horda.is_empty()) {
        horda.push_back(new Enemy("enemy_0", path[0], 80, 80, 10, 1));
        horda.push_back(new Enemy("enemy_1", path[0], 80, 80, 10, 1));
        horda.push_back(new Enemy("enemy_2", path[0], 80, 80, 10, 1));
        horda.head_path_index = 0;
    }
 
    // ── 4. AVANZAR LA HORDA ───────────────────────────────────
    horda.advance();   // sin parámetro — usa head_path_index interno
 
    // ── 5. TORRES ATACAN LA CABEZA ────────────────────────────
    int gold_earned = 0;
 
    for (Torre* torre : torres) {
        if (horda.is_empty()) break;
        Enemy* muerto = torre->attack(horda);
        if (muerto != nullptr) {
            gold_earned += 30;
            gold        += 30;
            horda.killed_ids.push_back(muerto->id);
            delete muerto;
        }
    }
 
    // ── 6. ¿LA CABEZA LLEGÓ AL CASTILLO? ─────────────────────
    int damage_castillo = 0;
    std::string status  = "ongoing";
 
    if (!horda.is_empty()) {
        Enemy* cabeza = horda.head;
        if (cabeza->pos.row == 0 && cabeza->pos.col == 4) {
            damage_castillo = cabeza->dmg;
            castle_hp -= damage_castillo;
            if (castle_hp <= 0) {
                castle_hp = 0;
                status    = "defeat";
            }
        }
    } else {
        status = "wave_cleared";
    }
 
    // ── 7. ESCRIBIR state.json usando toJson() de la horda ────
    // Serializar killed_ids como array JSON
    std::string killed_json = "";
    for (size_t i = 0; i < horda.killed_ids.size(); i++) {
        if (i > 0) killed_json += ",";
        killed_json += "\"" + horda.killed_ids[i] + "\"";
    }
 
    // Serializar enemigos vivos
    std::ostringstream enemies_json;
    Enemy* cur   = horda.head;
    bool primero = true;
    while (cur != nullptr) {
        if (!primero) enemies_json << ",";
        enemies_json << cur->toJson();
        primero = false;
        cur = cur->next;
    }
 
    std::ostringstream state;
    state << "{\n"
          << "  \"tick_result\": {\n"
          << "    \"tick\": "        << tick        << ",\n"
          << "    \"gold_earned\": " << gold_earned << ",\n"
          << "    \"gold_total\": "  << gold        << ",\n"
          << "    \"status\": \""    << status      << "\"\n"
          << "  },\n"
          << "  \"castle_state\": {\n"
          << "    \"id\": \"castle_0\",\n"
          << "    \"hp\": "          << castle_hp   << ",\n"
          << "    \"hp_max\": "      << castle_max  << ",\n"
          << "    \"damage_taken_this_tick\": " << damage_castillo << "\n"
          << "  },\n"
          << "  \"horde_state\": [{\n"
          << "    \"id\": \"horde_0\",\n"
          << "    \"head_pos\": {"
          <<      "\"row\": " << (horda.is_empty() ? -1 : horda.head->pos.row) << ","
          <<      "\"col\": " << (horda.is_empty() ? -1 : horda.head->pos.col)
          << "},\n"
          << "    \"enemies_alive\": " << horda.size << ",\n"
          << "    \"enemies\": ["      << enemies_json.str() << "],\n"
          << "    \"killed_ids\": ["   << killed_json << "]\n"
          << "  }]\n"
          << "}\n";
 
    escribir_archivo(state_path, state.str());
 
    std::cout << "[Engine] Tick "         << tick
              << " | Enemigos vivos: "    << horda.size
              << " | HP Castillo: "       << castle_hp
              << " | Oro ganado: "        << gold_earned
              << " | Status: "            << status
              << std::endl;
 
    for (Torre* t : torres) delete t;
    return 0;
}
