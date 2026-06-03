# ============================================================
# Tower Siege — Variant 1
# Algorithm Developer: Daniel Santiago Pérez Madera
# Ciencias de la Computación I — 2026-I
# Universidad Distrital Francisco José de Caldas
# ============================================================
 
# game/algorithms/backtracking.py
 
def encontrar_ruta_backtracking(fila, col, pos_castillo, torres_construidas, ruta_actual, visitados):
    """
    Algoritmo de Backtracking para encontrar un camino desde (fila, col)
    hasta el castillo evitando las torres.
 
    Los movimientos se ordenan por distancia Manhattan al castillo antes
    de explorar, lo que guía la búsqueda hacia el destino y produce rutas
    más directas sin cambiar la naturaleza recursiva del algoritmo.
    """
    if (fila, col) == pos_castillo:
        return list(ruta_actual)
 
    # Ordenar movimientos por distancia Manhattan al castillo
    # El movimiento que más acerca al destino se intenta primero
    movimientos = [(-1, 0), (1, 0), (0, -1), (0, 1)]
    movimientos.sort(
        key=lambda m: abs((fila + m[0]) - pos_castillo[0])
                    + abs((col  + m[1]) - pos_castillo[1])
    )
 
    for df, dc in movimientos:
        nueva_f = fila + df
        nueva_c = col  + dc
 
        if 0 <= nueva_f < 9 and 0 <= nueva_c < 9:
            if (nueva_f, nueva_c) not in torres_construidas and \
               (nueva_f, nueva_c) not in visitados:
 
                visitados.add((nueva_f, nueva_c))
                ruta_actual.append((nueva_f, nueva_c))
 
                resultado = encontrar_ruta_backtracking(
                    nueva_f, nueva_c, pos_castillo,
                    torres_construidas, ruta_actual, visitados
                )
                if resultado:
                    return resultado
 
                ruta_actual.pop()
                visitados.remove((nueva_f, nueva_c))
 
    return None