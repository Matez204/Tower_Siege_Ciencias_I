# Tower Siege 
Enemies march in waves across a grid toward a castle. The player
places algorithm towers. The greedy advisor recommends the tile with the highest
incoming threat for the next tower. The backtracking engine computes the optimal
enemy path around existing towers. C++ maintains the enemy wave as a linked list
and a BST of towers ordered by attack power.
__Greedy(Python):__ Best tower placement by threat level
__Backtracking(Python):__ Enemy path avoiding active towers
__C++ List:__ Enemy wave queue
__C++ thee:__ Tower registry (BST)


project/
engine/ C++
    linked_list.cpp
    tree.cpp (BST or AVL, depending on variant)
    main.cpp (reads input.json -> writes state.json)
game/ Python
    algorithms/ 
        greedy.py
        backtracking.py
    ui/
        main.py (Pygame grid + HUD)
bridge.py (file I/O: reads state.json, writes input.json)
data/
    input.json
    state.json

• Working Game: A fully playable version of the game demonstrating all required
components. The game must run from a single entry point (game/ui/main.py).
• GitHub Repository: A public or shared repository following the required folder
structure, with a descriptive README.md explaining setup and execution instructions.
• Technical Report (PDF): A document covering:
– Description of each algorithm implemented (greedy and backtracking) with pseu-
docode and complexity analysis.
– Description of each data structure implemented in C++ with diagrams and op-
eration complexity.
– Explanation of the JSON bridge: how Python and C++ communicate.
– Reflections on design decisions, challenges faced, and lessons learned.
• Live Demo: Each team presents their game in class during the final session (Week
16). All three members must be able to explain any part of the project.

Technical Requirements and Constraints
• The C++ engine must be compiled and executed as a standalone binary. No external
C++ libraries beyond the standard library are allowed.
• The Python layer must use Pygame for all UI rendering. The pygame and standard
library packages are the only allowed Python dependencies for the UI.
• The integration between C++ and Python must be done exclusively through
JSON file I/O. No sockets, shared memory, or Python-C++ bindings are allowed.
• All source files must include a header comment with team number, variant name, and
student names.
• The game grid must be at least 9 × 9 cells.
• The C++ linked list and tree must be implemented from scratch. Using std::list,
std::map, std::set, or any equivalent STL container for the required structures is
not allowed.