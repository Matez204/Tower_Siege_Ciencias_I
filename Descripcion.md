Cuadricula de al menos 9x9
objetivo: Defender el castillo
Amenaza: Hordas de enemigos.
Mecanica: Ganar dinero matando enemigos, para poner torres y defenderte.

Elementos:
- Castillo: Elemento a proteger, si es destruido se pierde el juego. Informacion: Vida, rango de ataque, daño.
- Torres: Principales herramientas para defender al castillo. Informacion: Vida, posicion, rango de ataque, daño.
- Enemigos: Enemigos que intentan destruir tu castillo. Informacion: Vida, posicion, rando de ataque, daño.
- Horda: Horda de enemigos que vienen en fila para atacarte. Informacion: Tamaño, velocidad, Punto de incio, camino a recorrer, posicion de la cabeza.

Loop de juego: 
Se empezara con cierta cantidad de oro para construir torres, los enemigos empezaran a aparacer cuando alla posicionado tus torres y le des a empezar. 
cuando mates a un enemigo ganas oro.
Las hordas de enemigos se moveran segun su tamaño, entre mas grande la orda mas lenta, pero entre mas pequeña mas rapida.
Las torres siempre atacar a la cabeza de la orda, cuando esta muera, el siguiente a el pasa a ser la cabeza.
El casitillo siempre se posicionara en la casilla central superior, peuqeda al borde de arriba.
los primeros 10 segundos todas las ordas se moveran a una casillaa cada 2 segundos.

Estructura Arquitectónica del Juego

Python:
    Captura eventos: Clics del usuario para poner torres.

    Renderiza: Dibuja la cuadrícula, las torres y los enemigos usando Pygame.

    Planifica: Corre el Backtracking para saber cuál es la ruta óptima de casillas y el Greedy para recomendar dónde construir.

    Envía órdenes: Le dice a C++: "El usuario puso una torre en (X,Y)" o "Ha pasado 1 segundo, procesa el siguiente paso".

C++:
    Mueve la horda internamente: Python calcula la ruta (por ejemplo: [(0,0), (0,1), (0,2)...]), pero C++ es quien actualiza la posición de la "cabeza" y el cuerpo en su Lista Enlazada basándose en la velocidad actual de la horda.

    Calcula el combate (Crucial): C++ recorre su árbol BST de torres ordenadas por poder de ataque. Para cada torre, verifica si la cabeza de la horda está en su rango de ataque. Si lo está, le resta vida a la cabeza en la lista enlazada.

    Gestiona la muerte y las recompensas: Si la vida de la cabeza llega a 0, C++ elimina ese nodo de la lista enlazada, promueve al siguiente enemigo como la nueva cabeza y añade oro al estado del juego.

    Determina el Game Over: Si la cabeza llega a la posición del castillo, C++ le resta vida al castillo.

Para mantener un código limpio y cumplir con los requisitos, el juego debe dividir sus responsabilidades de forma estricta:  

    El Frontend y la Inteligencia (Python):

        Pygame será el responsable de renderizar la cuadrícula (mínimo de 9x9), el castillo en la parte superior central, las torres, los enemigos y la interfaz del usuario (oro, vida).  

        Greedy: Se ejecuta durante la fase de construcción para recomendar la casilla con mayor amenaza donde el jugador debería colocar su siguiente torre.  

        Backtracking: Calcula la ruta de los enemigos hacia el castillo esquivando las torres activas.  

    El Motor Backend (C++):

        Lista Enlazada: Mantiene la cola de la horda de enemigos. La mecánica que mencionaste, donde las torres siempre atacan a la "cabeza" de la horda y el siguiente asume su lugar al morir, se mapea perfectamente a eliminar (o hacer pop) el primer nodo de esta lista.  

        Árbol (BST/AVL): Registra las torres en el mapa, ordenándolas por su poder de ataque para priorizar qué torre dispara primero en el cálculo del daño.  

    El Puente (JSON Bridge): El flujo de datos serializado que permite a ambos lenguajes intercambiar información del estado del juego en cada ciclo.  

El Loop de Juego (Game Loop)

Dado que la comunicación es mediante lectura y escritura de archivos I/O, el loop debe ser escalonado para no causar bloqueos por rendimiento:  

1. Fase de Preparación (Setup):

    El juego inicia mostrando la cuadrícula vacía con el castillo posicionado en el centro del borde superior.  

    El jugador tiene una cantidad de oro inicial para construir defensas.  

    Al seleccionar una torre, el algoritmo Greedy en Python resalta la mejor casilla disponible.  

    El jugador ubica sus torres y presiona "Empezar".  

2. Cálculo de Ruta Inicial:

    Python ejecuta el algoritmo de Backtracking para trazar el camino que seguirá la horda, evitando las torres recién puestas, y guarda este plano.  

3. Fase de Acción (Oleada/Tick Cycle):
El juego entra en un ciclo continuo que ocurre varias veces por segundo:

    Paso A (Input): Python escribe en input.json la información de la horda actual y las estadísticas de las torres.

    Paso B (Procesamiento C++): El binario de C++ lee el archivo y ejecuta la lógica del turno:  

        Actualiza las posiciones de los enemigos en la Lista Enlazada según su velocidad. Aquí se aplica tu regla: si es el inicio del juego, en los primeros 10 segundos la horda avanza solo una casilla cada 2 segundos. Luego, la velocidad dependerá del tamaño de la horda (más grande = más lenta).  

        Recorre el Árbol BST de torres por orden de ataque. Cada torre en rango aplica daño a la cabeza de la horda.  

        Si la cabeza muere, el nodo se elimina de la lista y se otorga oro.  

        C++ escribe el nuevo estado (vida de enemigos, posiciones, daño al castillo) en state.json.  

    Paso C (Renderizado): Python lee state.json, actualiza las variables de oro y vida, y Pygame dibuja el frame actual en pantalla.  

4. Condición de Fin de Partida:

    Si la cabeza de la horda alcanza el castillo y le reduce la vida a cero, el juego termina en derrota. Si el jugador elimina todas las hordas, gana y avanza de nivel.