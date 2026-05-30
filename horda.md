La orda sera una lista enlaza que funcionara como una cola. por lo que al añadir un nuevo enemigo a la horda este se añadira al final, y cuando muera un enemigo la cabeza pase a ser el siguiente a el.

Horda:
- id
- head
- tail
- size
- velocidad
- ruta a seguir

Eemigo:
- id
- posicion
- vida
- rango
- daño
- next


La ruta a seguir la va a calcular la parte de python a medida que sea necesario.
Despues de la parte de c++ va a mover la horda, o mas precisamente la cabeza de la lista, siguiendo este camino, y todos los enemigos de linked lista se moveran a las posicion en la que estaba el siguiente.