# ============================================================
# Tower Siege — Variant 1
# Algorithm Developer: [nombre compañero]
# Ciencias de la Computación I — 2026-I
# Universidad Distrital Francisco José de Caldas
# ============================================================

# game/algorithms/greedy.py

def evaluar_amenaza_greedy(torres_construidas, pos_spawn, pos_castillo, ruta_actual=None):
    """
    Sugiere la celda vacía con mayor amenaza para colocar la siguiente torre.
    
    Estrategia: recorre todas las celdas libres del tablero y calcula
    la distancia Manhattan mínima a cualquier celda de la ruta enemiga
    actual. La celda más CERCANA a la ruta tiene mayor amenaza, porque
    una torre ahí interrumpe el camino enemigo más eficientemente.
    
    Si no hay ruta disponible, usa distancia al segmento spawn→castillo
    como fallback.
    
    Complejidad: O(n²·r) donde n=9 (tablero) y r=longitud de ruta.
    """
    FILAS, COLS = 9, 9
    celdas_bloqueadas = set(torres_construidas.keys()) | {pos_spawn, pos_castillo}

    # Referencia de amenaza: la ruta enemiga actual, o línea recta spawn→castillo
    if ruta_actual and len(ruta_actual) > 0:
        referencia = ruta_actual
    else:
        # Fallback: interpolación Manhattan entre spawn y castillo
        referencia = _linea_manhattan(pos_spawn, pos_castillo)

    mejor_celda   = None
    menor_distancia = float('inf')

    for f in range(FILAS):
        for c in range(COLS):
            if (f, c) in celdas_bloqueadas:
                continue

            # Distancia mínima de esta celda a cualquier punto de la ruta
            dist_min = min(
                abs(f - rf) + abs(c - rc)
                for rf, rc in referencia
            )

            # Desempate: preferir celdas más centradas (más opciones de cobertura)
            if dist_min < menor_distancia:
                menor_distancia = dist_min
                mejor_celda = (f, c)

    return mejor_celda


def _linea_manhattan(origen, destino):
    """Genera una lista de celdas en línea Manhattan entre dos puntos."""
    f, c = origen
    df = destino[0] - f
    dc = destino[1] - c
    pasos = []
    # Avanzar primero en filas, luego en columnas
    paso_f = 1 if df > 0 else -1
    paso_c = 1 if dc > 0 else -1
    while f != destino[0]:
        pasos.append((f, c))
        f += paso_f
    while c != destino[1]:
        pasos.append((f, c))
        c += paso_c
    pasos.append(destino)
    return pasos