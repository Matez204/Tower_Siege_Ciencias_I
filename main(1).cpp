// ============================================================
// Team: [TEAM_NUMBER] | Variant: Tower Siege
// Members: [STUDENT_NAMES]
// engine/main.cpp — C++ engine entry point
// Responsibility: Read input.json produced by Python,
//                 run game logic (linked list + BST),
//                 write state.json for Python to consume.
// Only stdlib is used (no external libraries).
// ============================================================

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>

// ── Minimal JSON parser/builder ───────────────────────────────────────────────
// We implement just enough JSON handling to read input.json and write
// state.json without any external library.
// A proper tokenizer is intentionally kept simple: it covers the subset
// of JSON produced by bridge.py (objects, arrays, strings, numbers,
// booleans, null).  Extend as needed.

// ── Forward declarations ──────────────────────────────────────────────────────
struct JsonValue;
using JsonObject = std::vector<std::pair<std::string, JsonValue*>>;
using JsonArray  = std::vector<JsonValue*>;

// ── JsonValue ─────────────────────────────────────────────────────────────────
struct JsonValue {
    enum class Type { Object, Array, String, Number, Bool, Null };
    Type type;

    JsonObject  obj;
    JsonArray   arr;
    std::string str;
    double      num  = 0.0;
    bool        flag = false;

    ~JsonValue() {
        for (auto& p : obj) delete p.second;
        for (auto* v : arr) delete v;
    }

    // ── Accessors (throw on type mismatch) ────────────────────────────────
    JsonValue* get(const std::string& key) const {
        if (type != Type::Object)
            throw std::runtime_error("JsonValue is not an object");
        for (auto& p : obj)
            if (p.first == key) return p.second;
        throw std::runtime_error("Key not found: " + key);
    }

    // Returns nullptr if key is absent (used for optional fields like speed_override)
    JsonValue* tryGet(const std::string& key) const {
        if (type != Type::Object) return nullptr;
        for (auto& p : obj)
            if (p.first == key) return p.second;
        return nullptr;
    }

    const JsonArray& asArray() const {
        if (type != Type::Array)
            throw std::runtime_error("JsonValue is not an array");
        return arr;
    }

    const std::string& asString() const {
        if (type != Type::String)
            throw std::runtime_error("JsonValue is not a string");
        return str;
    }

    double asDouble() const {
        if (type != Type::Number)
            throw std::runtime_error("JsonValue is not a number");
        return num;
    }

    int asInt() const { return static_cast<int>(asDouble()); }

    bool asBool() const {
        if (type != Type::Bool)
            throw std::runtime_error("JsonValue is not a bool");
        return flag;
    }

    bool isNull() const { return type == Type::Null; }
};

// ── Tokenizer ─────────────────────────────────────────────────────────────────
struct Parser {
    const std::string& src;
    size_t pos = 0;

    explicit Parser(const std::string& s) : src(s) {}

    void skipWS() {
        while (pos < src.size() && std::isspace((unsigned char)src[pos]))
            ++pos;
    }

    char peek() { skipWS(); return pos < src.size() ? src[pos] : '\0'; }
    char consume() { skipWS(); return pos < src.size() ? src[pos++] : '\0'; }

    void expect(char c) {
        char got = consume();
        if (got != c) {
            throw std::runtime_error(
                std::string("Expected '") + c + "' got '" + got + "' at pos " +
                std::to_string(pos));
        }
    }

    std::string parseString() {
        expect('"');
        std::string result;
        while (pos < src.size()) {
            char c = src[pos++];
            if (c == '"') return result;
            if (c == '\\' && pos < src.size()) {
                char esc = src[pos++];
                switch (esc) {
                    case '"':  result += '"';  break;
                    case '\\': result += '\\'; break;
                    case '/':  result += '/';  break;
                    case 'n':  result += '\n'; break;
                    case 'r':  result += '\r'; break;
                    case 't':  result += '\t'; break;
                    default:   result += esc;  break;
                }
            } else {
                result += c;
            }
        }
        throw std::runtime_error("Unterminated string");
    }

    JsonValue* parseValue();

    JsonValue* parseObject() {
        expect('{');
        auto* v = new JsonValue();
        v->type = JsonValue::Type::Object;
        skipWS();
        if (peek() == '}') { consume(); return v; }
        while (true) {
            skipWS();
            std::string key = parseString();
            skipWS(); expect(':');
            JsonValue* val = parseValue();
            v->obj.push_back({key, val});
            skipWS();
            char sep = peek();
            if (sep == '}') { consume(); break; }
            if (sep == ',') { consume(); continue; }
            throw std::runtime_error("Expected ',' or '}' in object");
        }
        return v;
    }

    JsonValue* parseArray() {
        expect('[');
        auto* v = new JsonValue();
        v->type = JsonValue::Type::Array;
        skipWS();
        if (peek() == ']') { consume(); return v; }
        while (true) {
            v->arr.push_back(parseValue());
            skipWS();
            char sep = peek();
            if (sep == ']') { consume(); break; }
            if (sep == ',') { consume(); continue; }
            throw std::runtime_error("Expected ',' or ']' in array");
        }
        return v;
    }

    JsonValue* parseNumber() {
        size_t start = pos;
        if (pos < src.size() && src[pos] == '-') ++pos;
        while (pos < src.size() && (std::isdigit((unsigned char)src[pos]) ||
               src[pos] == '.' || src[pos] == 'e' || src[pos] == 'E' ||
               src[pos] == '+' || src[pos] == '-'))
            ++pos;
        auto* v = new JsonValue();
        v->type = JsonValue::Type::Number;
        v->num  = std::stod(src.substr(start, pos - start));
        return v;
    }
};

JsonValue* Parser::parseValue() {
    char c = peek();
    if (c == '{') return parseObject();
    if (c == '[') return parseArray();
    if (c == '"') {
        auto* v = new JsonValue();
        v->type = JsonValue::Type::String;
        v->str  = parseString();
        return v;
    }
    if (c == 't') {
        pos += 4;   // "true"
        auto* v = new JsonValue(); v->type = JsonValue::Type::Bool; v->flag = true; return v;
    }
    if (c == 'f') {
        pos += 5;   // "false"
        auto* v = new JsonValue(); v->type = JsonValue::Type::Bool; v->flag = false; return v;
    }
    if (c == 'n') {
        pos += 4;   // "null"
        auto* v = new JsonValue(); v->type = JsonValue::Type::Null; return v;
    }
    if (c == '-' || std::isdigit((unsigned char)c)) return parseNumber();
    throw std::runtime_error(std::string("Unexpected char '") + c + "' at pos " + std::to_string(pos));
}

// ── File helpers ──────────────────────────────────────────────────────────────
static std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open())
        throw std::runtime_error("Cannot open file: " + path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static void writeFile(const std::string& path, const std::string& content) {
    // Write to a .tmp then rename for atomicity (mirrors Python bridge).
    std::string tmp = path + ".tmp";
    std::ofstream f(tmp);
    if (!f.is_open())
        throw std::runtime_error("Cannot write file: " + tmp);
    f << content;
    f.close();
    // std::rename is C-stdlib, available everywhere.
    if (std::rename(tmp.c_str(), path.c_str()) != 0)
        throw std::runtime_error("Cannot rename " + tmp + " to " + path);
}

// ── Plain data structs (filled from JSON) ────────────────────────────────────
struct Pos { int row, col; };

struct Enemy {
    std::string id;
    int hp, hp_max, dmg, atk_range;
    Pos pos;
};

struct Horde {
    std::string id;
    int size;
    bool has_speed_override;
    int  speed_override;
    std::vector<Pos>    path;
    Pos                 head_pos;
    std::vector<Enemy>  enemies;
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
    std::vector<Tower>  towers;
    std::vector<Horde>  horde_queue;
};

// ── JSON → structs ────────────────────────────────────────────────────────────
static Pos parsePos(JsonValue* v) {
    return { v->get("row")->asInt(), v->get("col")->asInt() };
}

static Enemy parseEnemy(JsonValue* v) {
    Enemy e;
    e.id        = v->get("id")->asString();
    e.hp        = v->get("hp")->asInt();
    e.hp_max    = v->get("hp_max")->asInt();
    e.dmg       = v->get("dmg")->asInt();
    e.atk_range = v->get("atk_range")->asInt();
    e.pos       = parsePos(v->get("pos"));
    return e;
}

static Horde parseHorde(JsonValue* v) {
    Horde h;
    h.id   = v->get("id")->asString();
    h.size = v->get("size")->asInt();

    JsonValue* sp = v->tryGet("speed_override");
    if (sp && !sp->isNull()) {
        h.has_speed_override = true;
        h.speed_override     = sp->asInt();
    } else {
        h.has_speed_override = false;
        h.speed_override     = 0;
    }

    for (JsonValue* p : v->get("path")->asArray())
        h.path.push_back(parsePos(p));

    h.head_pos = parsePos(v->get("head_pos"));

    for (JsonValue* e : v->get("enemies")->asArray())
        h.enemies.push_back(parseEnemy(e));

    return h;
}

static Tower parseTower(JsonValue* v) {
    Tower t;
    t.id        = v->get("id")->asString();
    t.hp        = v->get("hp")->asInt();
    t.atk_range = v->get("atk_range")->asInt();
    t.dmg       = v->get("dmg")->asInt();
    t.pos       = parsePos(v->get("pos"));
    return t;
}

static Castle parseCastle(JsonValue* v) {
    Castle c;
    c.id        = v->get("id")->asString();
    c.hp        = v->get("hp")->asInt();
    c.hp_max    = v->get("hp_max")->asInt();
    c.atk_range = v->get("atk_range")->asInt();
    c.dmg       = v->get("dmg")->asInt();
    c.pos       = parsePos(v->get("pos"));
    return c;
}

static GameInput parseInput(const std::string& json) {
    Parser p(json);
    JsonValue* root = p.parseObject();

    GameInput gi;

    // meta
    JsonValue* meta     = root->get("meta");
    gi.meta.tick            = meta->get("tick")->asInt();
    gi.meta.phase           = meta->get("phase")->asString();
    gi.meta.elapsed_seconds = meta->get("elapsed_seconds")->asDouble();
    gi.meta.gold            = meta->get("gold")->asInt();
    gi.meta.dt              = meta->get("dt")->asDouble();

    // castle
    gi.castle = parseCastle(root->get("castle"));

    // towers
    for (JsonValue* t : root->get("towers")->asArray())
        gi.towers.push_back(parseTower(t));

    // horde_queue
    for (JsonValue* h : root->get("horde_queue")->asArray())
        gi.horde_queue.push_back(parseHorde(h));

    delete root;
    return gi;
}

// ── Structs → JSON (state.json builder) ──────────────────────────────────────
// We build the JSON string manually; no external library needed.

static std::string escStr(const std::string& s) {
    // Simple escape: only handle the characters bridge.py might produce.
    std::string out = "\"";
    for (char c : s) {
        if (c == '"')       out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else                out += c;
    }
    out += "\"";
    return out;
}

// ── State structs (filled by game logic, serialized to state.json) ────────────

struct EnemyState {
    std::string id;
    int         hp;
    Pos         pos;
};

struct HordeState {
    std::string              id;
    Pos                      head_pos;
    int                      enemies_alive;
    std::vector<EnemyState>  enemies;
    std::vector<std::string> killed_ids;
};

struct TowerState {
    std::string id;
    int         hp;
    std::string last_target_id;   // empty string = no target
};

struct CastleState {
    std::string id;
    int         hp, hp_max;
    int         damage_taken_this_tick;
};

struct Event {
    std::string type;
    std::string enemy_id;      // for ENEMY_KILLED
    int         gold_reward;   // for ENEMY_KILLED
    // Extend with a union / variant if you add more event types later.
};

struct GameState {
    int                      tick;
    int                      gold_earned;
    std::string              status;      // "ongoing" | "victory" | "defeat"
    CastleState              castle_state;
    std::vector<TowerState>  towers_state;
    std::vector<HordeState>  horde_state;
    std::vector<Event>       events;
};

// ── Serializer ────────────────────────────────────────────────────────────────
static std::string buildStateJson(const GameState& gs) {
    std::ostringstream o;
    o << "{\n";

    // tick_result
    o << "  \"tick_result\": {\n"
      << "    \"tick\": "        << gs.tick        << ",\n"
      << "    \"gold_earned\": " << gs.gold_earned << ",\n"
      << "    \"status\": "      << escStr(gs.status) << "\n"
      << "  },\n";

    // castle_state
    o << "  \"castle_state\": {\n"
      << "    \"id\": "                    << escStr(gs.castle_state.id)  << ",\n"
      << "    \"hp\": "                    << gs.castle_state.hp          << ",\n"
      << "    \"hp_max\": "               << gs.castle_state.hp_max      << ",\n"
      << "    \"damage_taken_this_tick\": " << gs.castle_state.damage_taken_this_tick << "\n"
      << "  },\n";

    // towers_state
    o << "  \"towers_state\": [\n";
    for (size_t i = 0; i < gs.towers_state.size(); ++i) {
        const auto& ts = gs.towers_state[i];
        o << "    {\n"
          << "      \"id\": " << escStr(ts.id) << ",\n"
          << "      \"hp\": " << ts.hp          << ",\n"
          << "      \"last_target_id\": ";
        if (ts.last_target_id.empty()) o << "null";
        else                           o << escStr(ts.last_target_id);
        o << "\n    }";
        if (i + 1 < gs.towers_state.size()) o << ",";
        o << "\n";
    }
    o << "  ],\n";

    // horde_state
    o << "  \"horde_state\": [\n";
    for (size_t hi = 0; hi < gs.horde_state.size(); ++hi) {
        const auto& hs = gs.horde_state[hi];
        o << "    {\n"
          << "      \"id\": "            << escStr(hs.id) << ",\n"
          << "      \"head_pos\": { \"row\": " << hs.head_pos.row
          <<                           ", \"col\": " << hs.head_pos.col << " },\n"
          << "      \"enemies_alive\": " << hs.enemies_alive << ",\n"
          << "      \"enemies\": [\n";
        for (size_t ei = 0; ei < hs.enemies.size(); ++ei) {
            const auto& e = hs.enemies[ei];
            o << "        { \"id\": " << escStr(e.id)
              << ", \"hp\": " << e.hp
              << ", \"pos\": { \"row\": " << e.pos.row
              << ", \"col\": " << e.pos.col << " } }";
            if (ei + 1 < hs.enemies.size()) o << ",";
            o << "\n";
        }
        o << "      ],\n";

        // killed_ids
        o << "      \"killed_ids\": [";
        for (size_t ki = 0; ki < hs.killed_ids.size(); ++ki) {
            o << escStr(hs.killed_ids[ki]);
            if (ki + 1 < hs.killed_ids.size()) o << ", ";
        }
        o << "]\n";

        o << "    }";
        if (hi + 1 < gs.horde_state.size()) o << ",";
        o << "\n";
    }
    o << "  ],\n";

    // events
    o << "  \"events\": [\n";
    for (size_t i = 0; i < gs.events.size(); ++i) {
        const auto& ev = gs.events[i];
        o << "    {\n"
          << "      \"type\": " << escStr(ev.type) << ",\n"
          << "      \"data\": { \"enemy_id\": " << escStr(ev.enemy_id)
          << ", \"gold_reward\": " << ev.gold_reward << " }\n"
          << "    }";
        if (i + 1 < gs.events.size()) o << ",";
        o << "\n";
    }
    o << "  ]\n";

    o << "}\n";
    return o.str();
}

// ── Stub: game logic placeholder ─────────────────────────────────────────────
// Replace the body of this function with your linked list + BST logic.
GameState processTick(const GameInput& gi) {
    GameState gs;
    gs.tick        = gi.meta.tick;
    gs.gold_earned = 0;
    gs.status      = "ongoing";

    // Mirror castle state (no damage applied yet — add combat logic here)
    gs.castle_state.id                    = gi.castle.id;
    gs.castle_state.hp                    = gi.castle.hp;
    gs.castle_state.hp_max                = gi.castle.hp_max;
    gs.castle_state.damage_taken_this_tick = 0;

    // Mirror tower states
    for (const auto& t : gi.towers) {
        TowerState ts;
        ts.id             = t.id;
        ts.hp             = t.hp;
        ts.last_target_id = "";
        gs.towers_state.push_back(ts);
    }

    // Mirror horde states (movement + combat go here)
    for (const auto& h : gi.horde_queue) {
        HordeState hs;
        hs.id            = h.id;
        hs.head_pos      = h.head_pos;
        hs.enemies_alive = static_cast<int>(h.enemies.size());
        for (const auto& e : h.enemies) {
            EnemyState es;
            es.id  = e.id;
            es.hp  = e.hp;
            es.pos = e.pos;
            hs.enemies.push_back(es);
        }
        gs.horde_state.push_back(hs);
    }

    // No events yet — add ENEMY_KILLED / CASTLE_HIT here as you implement combat
    return gs;
}

// ── main ──────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    // Paths can be overridden via CLI args for easier testing:
    //   ./engine ../data/input.json ../data/state.json
    std::string inputPath = (argc > 1) ? argv[1] : "../data/input.json";
    std::string statePath = (argc > 2) ? argv[2] : "../data/state.json";

    try {
        // 1. Read input.json written by Python
        std::string raw = readFile(inputPath);

        // 2. Parse JSON → GameInput struct
        GameInput gi = parseInput(raw);

        std::cout << "[engine] tick=" << gi.meta.tick
                  << " phase=" << gi.meta.phase
                  << " gold="  << gi.meta.gold
                  << " towers=" << gi.towers.size()
                  << " hordes=" << gi.horde_queue.size()
                  << "\n";

        // 3. Run game logic (stub — fill in your linked list / BST here)
        GameState gs = processTick(gi);

        // 4. Serialize → state.json for Python to read
        std::string stateJson = buildStateJson(gs);
        writeFile(statePath, stateJson);

        std::cout << "[engine] state.json written OK\n";

    } catch (const std::exception& ex) {
        std::cerr << "[engine] ERROR: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
