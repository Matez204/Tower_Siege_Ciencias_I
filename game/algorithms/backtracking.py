# game/algorithms/backtracking.py

def encontrar_ruta_backtracking(fila, col, pos_castillo, torres_construidas, ruta_actual, visitados):
    """
    Algoritmo de Backtracking para encontrar un camino desde (fila, col) hasta el Castillo (0,4)
    evitando las celdas que contienen torres ('T').
    """
    if (fila, col) == pos_castillo:
        return list(ruta_actual)
        
    movimientos = [(-1, 0), (1, 0), (0, -1), (0, 1)]
    
    for df, dc in movimientos:
        nueva_f = fila + df
        nueva_c = col + dc
        
        if 0 <= nueva_f < 9 and 0 <= nueva_c < 9:
            if (nueva_f, nueva_c) not in torres_construidas and (nueva_f, nueva_c) not in visitados:
                
                visitados.add((nueva_f, nueva_c))
                ruta_actual.append((nueva_f, nueva_c))
                
                resultado = encontrar_ruta_backtracking(nueva_f, nueva_c, pos_castillo, torres_construidas, ruta_actual, visitados)
                if resultado:
                    return resultado
                    
                ruta_actual.pop()
                visitados.remove((nueva_f, nueva_c))
                
    return None