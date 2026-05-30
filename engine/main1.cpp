// ============================================================
// Team: [Team Number] | Variant: Tower Siege
// Members: [Student Names]
// File: engine/main.cpp
// Description: C++ engine entry point.
//              Reads input.json → runs one game tick
//              (linked list + BST logic) → writes state.json.
//
// Build:  g++ -std=c++17 -O2 -o tower_siege main.cpp linked_list.cpp tree.cpp
// Run:    ./tower_siege          (called by Python bridge each tick)
// ============================================================

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <optional>
#include <algorithm>
#include <cmath>
#include <stdexcept>

// #include "linked_list.h"
// #include "tree.h"


// ════════════════════════════════════════════════════════════
// §1  MINIMAL JSON PARSER
// ════════════════════════════════════════════════════════════

struct JValue;
using JObject = std::vector<std::pair<std::string, JValue>>;
using JArray  = std::vector<JValue>;

struct JValue {
    enum class Kind { Null, Bool, Int, Float, String, Object, Array } kind;

    bool        b{};
    long long   i{};
    double      f{};
    std::string s{};
    JObject     obj{};
    JArray      arr{};

    JValue()                       : kind(Kind::Null)   {}
    explicit JValue(bool   v)      : kind(Kind::Bool),   b(v) {}
    explicit JValue(long long v)   : kind(Kind::Int),    i(v) {}
    explicit JValue(double v)      : kind(Kind::Float),  f(v) {}
    explicit JValue(std::string v) : kind(Kind::String), s(std::move(v)) {}
    explicit JValue(JObject v)     : kind(Kind::Object), obj(std::move(v)) {}
    explicit JValue(JArray  v)     : kind(Kind::Array),  arr(std::move(v)) {}

    bool isNull()   const { return kind == Kind::Null;   }
    bool isFloat()  const { return kind == Kind::Float;  }
    bool isInt()    const { return kind == Kind::Int;    }
    bool isString() const { return kind == Kind::String; }
    bool isArray()  const { return kind == Kind::Array;  }

    long long asInt()    const { return isFloat() ? (long long)f : i; }
    double    asDouble() const { return isInt()   ? (double)i    : f; }

    const JValue& operator[](const std::string& key) const {
        static JValue null_val;
        for (const auto& kv : obj)
            if (kv.first == key) return kv.second;
        return null_val;
    }
    const JValue& operator[](size_t idx) const {
        static JValue null_val;
        return (idx < arr.size()) ? arr[idx] : null_val;
    }
    size_t size() const { return isArray() ? arr.size() : obj.size(); }
};

// ── Parser ───────────────────────────────────────────────────
class JsonParser {
    const std::string& src;
    size_t pos = 0;

    void skipWS() {
        while (pos < src.size() && std::isspace((unsigned char)src[pos])) ++pos;
    }
    char peek() { skipWS(); return pos < src.size() ? src[pos] : '\0'; }
    char next() { skipWS(); return pos < src.size() ? src[pos++] : '\0'; }
    void expect(char c) {
        if (next() != c)
            throw std::runtime_error(std::string("JSON: expected '") + c + "'");
    }

    std::string parseString() {
        expect('"');
        std::string out;
        while (pos < src.size()) {
            char c = src[pos++];
            if (c == '"') break;
            if (c == '\\' && pos < src.size()) {
                char esc = src[pos++];
                switch (esc) {
                    case '"':  out += '"';  break;
                    case '\\': out += '\\'; break;
                    case 'n':  out += '\n'; break;
                    case 'r':  out += '\r'; break;
                    case 't':  out += '\t'; break;
                    default:   out += esc;  break;
                }
            } else {
                out += c;
            }
        }
        return out;
    }

    JValue parseNumber() {
        size_t start = pos;
        bool isF = false;
        if (src[pos] == '-') ++pos;
        while (pos < src.size() && std::isdigit((unsigned char)src[pos])) ++pos;
        if (pos < src.size() && src[pos] == '.') { isF = true; ++pos; }
        while (pos < src.size() && std::isdigit((unsigned char)src[pos])) ++pos;
        if (pos < src.size() && (src[pos]=='e'||src[pos]=='E')) {
            isF = true; ++pos;
            if (pos < src.size() && (src[pos]=='+'||src[pos]=='-')) ++pos;
            while (pos < src.size() && std::isdigit((unsigned char)src[pos])) ++pos;
        }
        std::string num = src.substr(start, pos - start);
        if (isF) return JValue(std::stod(num));
        return JValue((long long)std::stoll(num));
    }

    JValue parseArray() {
        expect('[');
        JArray arr;
        if (peek() == ']') { ++pos; return JValue(arr); }
        do { arr.push_back(parse()); } while (peek() == ',' && (++pos, true));
        expect(']');
        return JValue(arr);
    }

    JValue parseObject() {
        expect('{');
        JObject obj;
        if (peek() == '}') { ++pos; return JValue(obj); }
        do {
            skipWS();
            std::string key = parseString();
            skipWS(); expect(':');
            obj.emplace_back(std::move(key), parse());
        } while (peek() == ',' && (++pos, true));
        expect('}');
        return JValue(obj);
    }

public:
    explicit JsonParser(const std::string& s) : src(s) {}
    JValue parse() {
        skipWS();
        char c = peek();
        if (c == '{') return parseObject();
        if (c == '[') return parseArray();
        if (c == '"') return JValue(parseString());
        if (c == 't') { pos += 4; return JValue(true);  }
        if (c == 'f') { pos += 5; return JValue(false); }
        if (c == 'n') { pos += 4; return JValue();      }
        return parseNumber();
    }
};

JValue loadJson(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Cannot open: " + path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return JsonParser(ss.str()).parse();
}


// ════════════════════════════════════════════════════════════
// §2  JSON SERIALISER
// ════════════════════════════════════════════════════════════

class JsonWriter {
public:  // <-- everything public so buildStateJson can access freely
    std::ostringstream oss;
    int indent = 0;

    void nl()  { oss << '\n' << std::string(indent * 2, ' '); }
    void sep()  { oss << ','; }

    void writeNull()               { oss << "null"; }
    void writeBool(bool b)         { oss << (b ? "true" : "false"); }
    void writeInt(long long v)     { oss << v; }
    void writeDouble(double v)     { char buf[64]; snprintf(buf,64,"%.6g",v); oss<<buf; }
    void writeString(const std::string& s) {
        oss << '"';
        for (char c : s) {
            if      (c == '"')  oss << "\\\"";
            else if (c == '\\') oss << "\\\\";
            else if (c == '\n') oss << "\\n";
            else if (c == '\r') oss << "\\r";
            else if (c == '\t') oss << "\\t";
            else                oss << c;
        }
        oss << '"';
    }
    void writeKey(const std::string& k) { writeString(k); oss << ": "; }

    void beginObject() { oss << '{'; indent++; }
    void endObject()   { indent--; nl(); oss << '}'; }
    void beginArray()  { oss << '['; indent++; }
    void endArray()    { indent--; nl(); oss << ']'; }

    std::string str() const { return oss.str(); }
};


// ════════════════════════════════════════════════════════════
// §3  DOMAIN STRUCTS
// ════════════════════════════════════════════════════════════

struct Pos { int row, col; };

struct Enemy {
    std::string id;
    int hp, hp_max, dmg, atk_range;
    Pos pos;
    bool alive = true;
};

struct Horde {
    std::string id;
    int size;
    std::optional<double> speed_override;
    std::vector<Pos>   path;
    Pos                head_pos;
    std::vector<Enemy> enemies;   // index 0 = head
};

struct Tower {
    std::string id;
    int hp, atk_range, dmg;
    Pos pos;
};

struct Castle {
    std::string id;
    int hp, hp_max, atk_range, dmg;
    Pos pos;
};

struct Meta {
    int    tick;
    std::string phase;
    double elapsed_seconds;
    int    gold;
    double dt;
};

struct GameInput {
    Meta              meta;
    Castle            castle;
    std::vector<Tower> towers;
    std::vector<Horde> horde_queue;
};


// ════════════════════════════════════════════════════════════
// §4  DESERIALISER
// ════════════════════════════════════════════════════════════

Pos parsePos(const JValue& j) {
    return { (int)j["row"].asInt(), (int)j["col"].asInt() };
}

Enemy parseEnemy(const JValue& j) {
    Enemy e;
    e.id        = j["id"].s;
    e.hp        = (int)j["hp"].asInt();
    e.hp_max    = (int)j["hp_max"].asInt();
    e.pos       = parsePos(j["pos"]);
    e.dmg       = (int)j["dmg"].asInt();
    e.atk_range = (int)j["atk_range"].asInt();
    return e;
}

Tower parseTower(const JValue& j) {
    Tower t;
    t.id        = j["id"].s;
    t.hp        = (int)j["hp"].asInt();
    t.pos       = parsePos(j["pos"]);
    t.atk_range = (int)j["atk_range"].asInt();
    t.dmg       = (int)j["dmg"].asInt();
    return t;
}

Castle parseCastle(const JValue& j) {
    Castle c;
    c.id        = j["id"].s;
    c.hp        = (int)j["hp"].asInt();
    c.hp_max    = (int)j["hp_max"].asInt();
    c.pos       = parsePos(j["pos"]);
    c.atk_range = (int)j["atk_range"].asInt();
    c.dmg       = (int)j["dmg"].asInt();
    return c;
}

Horde parseHorde(const JValue& j) {
    Horde h;
    h.id   = j["id"].s;
    h.size = (int)j["size"].asInt();

    const JValue& sp = j["speed_override"];
    if (!sp.isNull()) h.speed_override = sp.asDouble();

    h.head_pos = parsePos(j["head_pos"]);

    for (size_t i = 0; i < j["path"].size(); ++i)
        h.path.push_back(parsePos(j["path"][i]));

    for (size_t i = 0; i < j["enemies"].size(); ++i)
        h.enemies.push_back(parseEnemy(j["enemies"][i]));

    return h;
}

GameInput parseInput(const JValue& root) {
    GameInput gi;
    const JValue& m    = root["meta"];
    gi.meta.tick             = (int)m["tick"].asInt();
    gi.meta.phase            = m["phase"].s;
    gi.meta.elapsed_seconds  = m["elapsed_seconds"].asDouble();
    gi.meta.gold             = (int)m["gold"].asInt();
    gi.meta.dt               = m["dt"].asDouble();

    gi.castle = parseCastle(root["castle"]);

    for (size_t i = 0; i < root["towers"].size(); ++i)
        gi.towers.push_back(parseTower(root["towers"][i]));

    for (size_t i = 0; i < root["horde_queue"].size(); ++i)
        gi.horde_queue.push_back(parseHorde(root["horde_queue"][i]));

    return gi;
}


// ════════════════════════════════════════════════════════════
// §5  GAME TICK LOGIC
//     Stub: replace inner loops with your LinkedList + BST.
// ════════════════════════════════════════════════════════════

static double hordeSpeed(const Horde& h, double elapsed) {
    if (elapsed < 10.0) return 0.5;                    // 1 step / 2 s
    return (h.size > 0) ? (10.0 / h.size) : 1.0;
}

static bool inRange(const Pos& a, const Pos& b, int range) {
    int dr = a.row - b.row, dc = a.col - b.col;
    return (int)std::sqrt((double)(dr*dr + dc*dc)) <= range;
}

struct TickResult {
    int gold_earned    = 0;
    int castle_damage  = 0;
    std::vector<std::string> killed_ids;
};

TickResult runTick(GameInput& gi) {
    TickResult result;
    double elapsed = gi.meta.elapsed_seconds;
    double dt      = gi.meta.dt;

    for (auto& horde : gi.horde_queue) {
        if (horde.enemies.empty()) continue;

        // ── 1. Move head ─────────────────────────────────
        double speed = horde.speed_override.value_or(hordeSpeed(horde, elapsed));
        bool should_move = (speed * dt >= 0.5);

        if (should_move && !horde.path.empty()) {
            Pos next_pos = horde.path.front();
            horde.path.erase(horde.path.begin());   // TODO: LinkedList::popFront()

            if (next_pos.row == gi.castle.pos.row &&
                next_pos.col == gi.castle.pos.col) {
                // Head reached castle
                result.castle_damage += horde.enemies[0].dmg;
                gi.castle.hp -= horde.enemies[0].dmg;
            } else {
                horde.head_pos       = next_pos;
                horde.enemies[0].pos = next_pos;
            }
        }

        // ── 2. Towers attack head ────────────────────────
        // TODO: replace with BST in-order traversal (highest dmg first)
        auto towers_sorted = gi.towers;
        std::sort(towers_sorted.begin(), towers_sorted.end(),
            [](const Tower& a, const Tower& b){ return a.dmg > b.dmg; });

        for (const auto& tower : towers_sorted) {
            if (horde.enemies.empty()) break;
            Enemy& head = horde.enemies[0];
            if (inRange(tower.pos, head.pos, tower.atk_range))
                head.hp -= tower.dmg;
        }

        // Castle also attacks if head is in range
        if (!horde.enemies.empty()) {
            Enemy& head = horde.enemies[0];
            if (inRange(gi.castle.pos, head.pos, gi.castle.atk_range))
                head.hp -= gi.castle.dmg;
        }

        // ── 3. Remove dead heads ─────────────────────────
        // TODO: replace with LinkedList::popFront() until head is alive
        while (!horde.enemies.empty() && horde.enemies[0].hp <= 0) {
            result.killed_ids.push_back(horde.enemies[0].id);
            result.gold_earned += 30;
            horde.enemies.erase(horde.enemies.begin());
            if (!horde.enemies.empty())
                horde.head_pos = horde.enemies[0].pos;
        }
    }

    return result;
}


// ════════════════════════════════════════════════════════════
// §6  SERIALISER → state.json
// ════════════════════════════════════════════════════════════

std::string buildStateJson(const GameInput& gi, const TickResult& tr) {
    JsonWriter w;

    std::string status = "ongoing";
    if (gi.castle.hp <= 0) {
        status = "defeat";
    } else {
        bool all_dead = true;
        for (const auto& h : gi.horde_queue)
            if (!h.enemies.empty()) { all_dead = false; break; }
        if (all_dead) status = "victory";
    }

    // ── root open ────────────────────────────────────────
    w.beginObject();

    // tick_result
    w.nl(); w.writeKey("tick_result");
    w.beginObject();
      w.nl(); w.writeKey("tick");        w.writeInt(gi.meta.tick);
      w.sep(); w.nl(); w.writeKey("gold_earned");  w.writeInt(tr.gold_earned);
      w.sep(); w.nl(); w.writeKey("status");       w.writeString(status);
    w.endObject();

    // castle_state
    w.sep(); w.nl(); w.writeKey("castle_state");
    w.beginObject();
      w.nl(); w.writeKey("id");                      w.writeString(gi.castle.id);
      w.sep(); w.nl(); w.writeKey("hp");             w.writeInt(gi.castle.hp);
      w.sep(); w.nl(); w.writeKey("hp_max");         w.writeInt(gi.castle.hp_max);
      w.sep(); w.nl(); w.writeKey("damage_taken_this_tick"); w.writeInt(tr.castle_damage);
    w.endObject();

    // towers_state
    w.sep(); w.nl(); w.writeKey("towers_state");
    w.beginArray();
    for (size_t i = 0; i < gi.towers.size(); ++i) {
        const auto& t = gi.towers[i];
        if (i > 0) w.sep();
        w.nl();
        w.beginObject();
          w.nl(); w.writeKey("id");             w.writeString(t.id);
          w.sep(); w.nl(); w.writeKey("hp");    w.writeInt(t.hp);
          w.sep(); w.nl(); w.writeKey("last_target_id"); w.writeNull();
        w.endObject();
    }
    w.endArray();

    // horde_state
    w.sep(); w.nl(); w.writeKey("horde_state");
    w.beginArray();
    for (size_t hi = 0; hi < gi.horde_queue.size(); ++hi) {
        const auto& h = gi.horde_queue[hi];
        if (hi > 0) w.sep();
        w.nl();
        w.beginObject();
          w.nl(); w.writeKey("id");           w.writeString(h.id);

          w.sep(); w.nl(); w.writeKey("head_pos");
          w.beginObject();
            w.nl(); w.writeKey("row"); w.writeInt(h.head_pos.row);
            w.sep(); w.nl(); w.writeKey("col"); w.writeInt(h.head_pos.col);
          w.endObject();

          w.sep(); w.nl(); w.writeKey("enemies_alive"); w.writeInt((int)h.enemies.size());

          w.sep(); w.nl(); w.writeKey("enemies");
          w.beginArray();
          for (size_t ei = 0; ei < h.enemies.size(); ++ei) {
              const auto& e = h.enemies[ei];
              if (ei > 0) w.sep();
              w.nl();
              w.beginObject();
                w.nl(); w.writeKey("id");  w.writeString(e.id);
                w.sep(); w.nl(); w.writeKey("hp");  w.writeInt(e.hp);
                w.sep(); w.nl(); w.writeKey("pos");
                w.beginObject();
                  w.nl(); w.writeKey("row"); w.writeInt(e.pos.row);
                  w.sep(); w.nl(); w.writeKey("col"); w.writeInt(e.pos.col);
                w.endObject();
              w.endObject();
          }
          w.endArray();

          w.sep(); w.nl(); w.writeKey("killed_ids");
          w.beginArray();
          for (size_t ki = 0; ki < tr.killed_ids.size(); ++ki) {
              if (ki > 0) w.sep();
              w.nl(); w.writeString(tr.killed_ids[ki]);
          }
          w.endArray();

        w.endObject();
    }
    w.endArray();

    // events
    w.sep(); w.nl(); w.writeKey("events");
    w.beginArray();
    size_t evIdx = 0;
    for (const auto& kid : tr.killed_ids) {
        if (evIdx++ > 0) w.sep();
        w.nl();
        w.beginObject();
          w.nl(); w.writeKey("type"); w.writeString("ENEMY_KILLED");
          w.sep(); w.nl(); w.writeKey("data");
          w.beginObject();
            w.nl(); w.writeKey("enemy_id");    w.writeString(kid);
            w.sep(); w.nl(); w.writeKey("gold_reward"); w.writeInt(30);
          w.endObject();
        w.endObject();
    }
    if (tr.castle_damage > 0) {
        if (evIdx++ > 0) w.sep();
        w.nl();
        w.beginObject();
          w.nl(); w.writeKey("type"); w.writeString("CASTLE_DAMAGED");
          w.sep(); w.nl(); w.writeKey("data");
          w.beginObject();
            w.nl(); w.writeKey("damage"); w.writeInt(tr.castle_damage);
          w.endObject();
        w.endObject();
    }
    w.endArray();

    // ── root close ───────────────────────────────────────
    w.endObject();
    return w.str();
}

void saveJson(const std::string& path, const std::string& content) {
    std::ofstream f(path);
    if (!f) throw std::runtime_error("Cannot write: " + path);
    f << content << '\n';
}


// ════════════════════════════════════════════════════════════
// §7  MAIN
// ════════════════════════════════════════════════════════════

int main(int argc, char* argv[]) {
    std::string input_path = (argc > 1) ? argv[1] : "../data/input.json";
    std::string state_path = (argc > 2) ? argv[2] : "../data/state.json";

    try {
        // A. Read
        JValue    root = loadJson(input_path);
        GameInput gi   = parseInput(root);

        std::cerr << "[engine] tick=" << gi.meta.tick
                  << "  phase=" << gi.meta.phase
                  << "  elapsed=" << gi.meta.elapsed_seconds << "s\n";

        // B. Process
        TickResult tr = runTick(gi);

        std::cerr << "[engine] gold_earned=" << tr.gold_earned
                  << "  killed="      << tr.killed_ids.size()
                  << "  castle_hp="   << gi.castle.hp << "\n";

        // C. Write
        saveJson(state_path, buildStateJson(gi, tr));
        std::cerr << "[engine] state.json written OK\n";
        return 0;

    } catch (const std::exception& ex) {
        std::cerr << "[engine] FATAL: " << ex.what() << "\n";
        return 1;
    }
}
