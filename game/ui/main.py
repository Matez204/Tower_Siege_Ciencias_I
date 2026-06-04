# ============================================================
# Tower Siege — Variant 1
# UI Developer: Diego
# Ciencias de la Computación I — 2026-I
# Universidad Distrital Francisco José de Caldas
# ============================================================

import pygame
import sys
import os
import math

sys.setrecursionlimit(5000)

RUTA_UI = os.path.dirname(os.path.abspath(__file__))
RAIZ_PROYECTO = os.path.abspath(os.path.join(RUTA_UI, '../..'))
if RAIZ_PROYECTO not in sys.path:
    sys.path.insert(0, RAIZ_PROYECTO)

from game.bridge import Bridge, Pos, InputState, Tower, Castle, Horde, Enemy
from game.algorithms.greedy import evaluar_amenaza_greedy
from game.algorithms.backtracking import encontrar_ruta_backtracking

pygame.init()
pygame.font.init()

ANCHO, ALTO = 960, 660
pantalla = pygame.display.set_mode((ANCHO, ALTO))
pygame.display.set_caption("Tower Siege — Team 2")
reloj = pygame.time.Clock()

# ── PALETA DE COLORES ──────────────────────────────────────
C_BG        = (10,  12,  20)
C_BG2       = (16,  20,  35)
C_PANEL     = (20,  25,  45)
C_PANEL2    = (28,  34,  58)
C_BORDE     = (50,  60, 100)
C_BORDE2    = (80,  95, 150)

C_TEXTO     = (210, 220, 255)
C_TEXTO2    = (140, 155, 200)
C_GOLD      = (255, 200,  50)
C_GOLD2     = (200, 150,  20)
C_ROJO      = (220,  60,  60)
C_VERDE     = ( 60, 200, 100)
C_VERDE2    = ( 30, 130,  60)
C_AZUL      = ( 70, 130, 220)
C_AZUL2     = ( 40,  80, 160)
C_MORADO    = (160,  80, 220)
C_NARANJA   = (220, 140,  40)

C_SPAWN     = ( 30, 160,  80)
C_SPAWN2    = ( 15, 100,  50)
C_CASTILLO  = (180,  40,  40)
C_CASTILLO2 = (100,  20,  20)
C_TORRE     = ( 60, 110, 200)
C_TORRE2    = ( 30,  60, 130)
C_ENEMIGO   = (180,  50, 180)
C_CELDA_A   = ( 18,  22,  40)
C_CELDA_B   = ( 22,  28,  50)
C_SELEC     = (255, 180,  30)

# ── FUENTES ──────────────────────────────────────────────────
try:
    FUENTE_TITULO  = pygame.font.SysFont("Consolas", 22, bold=True)
    FUENTE_GRANDE  = pygame.font.SysFont("Consolas", 18, bold=True)
    FUENTE_MED     = pygame.font.SysFont("Consolas", 14)
    FUENTE_PEQUE   = pygame.font.SysFont("Consolas", 12)
except:
    FUENTE_TITULO  = pygame.font.SysFont("Arial", 22, bold=True)
    FUENTE_GRANDE  = pygame.font.SysFont("Arial", 18, bold=True)
    FUENTE_MED     = pygame.font.SysFont("Arial", 14)
    FUENTE_PEQUE   = pygame.font.SysFont("Arial", 12)

# ── TABLERO ──────────────────────────────────────────────────
FILAS, COLS  = 9, 9
CELDA        = 52
OX, OY       = 30, 90

POS_SPAWN    = (8, 4)
POS_CASTILLO = (0, 4)

# ── TIPOS DE TORRES ──────────────────────────────────────────
TIPOS_TORRE = {
    "basica":    {"nombre": "Básica",    "costo": 50,  "dmg": 25, "rango": 2, "hordas": 2,  "color": C_TORRE,   "color2": C_TORRE2},
    "reforzada": {"nombre": "Reforzada", "costo": 100, "dmg": 40, "rango": 2, "hordas": 4,  "color": (60,180,120), "color2": (30,100,60)},
    "elite":     {"nombre": "Élite",     "costo": 200, "dmg": 70, "rango": 3, "hordas": 8,  "color": (200,150,30), "color2": (120,80,10)},
}
torre_seleccionada_tipo = "basica"   # tipo activo para construir

ORO_INICIAL  = 200
HP_MAX       = 200

# ── ESTADO DEL JUEGO ─────────────────────────────────────────
bridge_sistema = Bridge()
estado_actual  = None
# No leer state.json al iniciar — siempre arrancar con estado limpio

# torres_construidas: { (fila,col): {"tipo": str, "cargas": int} }
torres_construidas = {}
celda_seleccionada = None
current_tick       = 0
oro_jugador        = ORO_INICIAL
hp_castillo        = HP_MAX
wave_status        = "ongoing"
wave_number        = 0   # número de oleada actual

fase = "construccion"   # "construccion" | "accion" | "tienda" | "derrota"

log_mensaje   = "Selecciona una celda y presiona [T] para construir una torre."
log_color     = C_TEXTO2
log_timer     = 0

# ruta calculada para visualizar
ruta_visual   = []
greedy_celda  = None

# path de la horda — se genera al iniciar combate y se preserva entre ticks
path_horda_actual = []

# animaciones de ataque: lista de {"torre": (f,c), "enemigo": (f,c), "timer": N}
animaciones_ataque = []

# auto-tick
auto_tick          = False
AUTO_TICK_MS       = 800          # milisegundos entre ticks automáticos
ultimo_auto_tick   = 0

# animaciones de pulso
anim_tick = 0

# ── BOTONES ──────────────────────────────────────────────────
PX = OX + COLS * CELDA + 20   # X del panel derecho
PW = ANCHO - PX - 15           # ancho del panel

btn_greedy      = pygame.Rect(PX, 210, PW, 38)
btn_start       = pygame.Rect(PX, 270, PW, 38)
btn_tick        = pygame.Rect(PX, 320, PW, 38)
btn_auto        = pygame.Rect(PX, 365, PW, 32)
btn_siguiente   = pygame.Rect(PX, 320, PW, 48)   # fase tienda
btn_reset       = pygame.Rect(PX, 410, PW, 32)

# Botones de selección de tipo de torre
btn_tipo_basica    = pygame.Rect(PX,      155, PW//3 - 2, 48)
btn_tipo_reforzada = pygame.Rect(PX + PW//3 + 1, 155, PW//3 - 2, 48)
btn_tipo_elite     = pygame.Rect(PX + (PW//3)*2 + 2, 155, PW//3 - 2, 48)

# ── HELPERS ──────────────────────────────────────────────────
def set_log(msg, color=None):
    global log_mensaje, log_color, log_timer
    log_mensaje = msg
    log_color   = color or C_TEXTO2
    log_timer   = 180

def rect_redondeado(surf, color, rect, radio=8, borde=0, color_borde=None):
    pygame.draw.rect(surf, color, rect, border_radius=radio)
    if borde and color_borde:
        pygame.draw.rect(surf, color_borde, rect, borde, border_radius=radio)

def texto_centrado(surf, texto, fuente, color, rect):
    s = fuente.render(texto, True, color)
    x = rect.x + (rect.w - s.get_width())  // 2
    y = rect.y + (rect.h - s.get_height()) // 2
    surf.blit(s, (x, y))

def barra_vida(surf, rect, valor, maximo, color_lleno, color_vacio=(40,40,60)):
    pygame.draw.rect(surf, color_vacio, rect, border_radius=4)
    if maximo > 0:
        ancho_lleno = int(rect.w * valor / maximo)
        if ancho_lleno > 0:
            r2 = pygame.Rect(rect.x, rect.y, ancho_lleno, rect.h)
            pygame.draw.rect(surf, color_lleno, r2, border_radius=4)
    pygame.draw.rect(surf, C_BORDE, rect, 1, border_radius=4)

def dibujar_boton(surf, rect, texto, fuente, activo=True, pulsado=False):
    alpha = 255 if activo else 100
    if pulsado:
        color_fondo  = C_BORDE2
        color_borde2 = C_TEXTO
    elif activo:
        color_fondo  = C_PANEL2
        color_borde2 = C_BORDE2
    else:
        color_fondo  = C_BG2
        color_borde2 = C_BORDE
    rect_redondeado(surf, color_fondo, rect, 6, 1, color_borde2)
    color_texto = C_TEXTO if activo else C_TEXTO2
    texto_centrado(surf, texto, fuente, color_texto, rect)

def celda_a_pixel(fila, col):
    return OX + col * CELDA, OY + fila * CELDA

def pixel_a_celda(px, py):
    if OX <= px < OX + COLS * CELDA and OY <= py < OY + FILAS * CELDA:
        return (py - OY) // CELDA, (px - OX) // CELDA
    return None

# ── RECALCULAR RUTA ENEMIGA ──────────────────────────────────
def recalcular_ruta():
    global ruta_visual
    ruta = encontrar_ruta_backtracking(
        POS_SPAWN[0], POS_SPAWN[1], POS_CASTILLO,
        torres_construidas, [POS_SPAWN], {POS_SPAWN}
    )
    ruta_visual = ruta if ruta else []

# ── DIBUJAR CUADRÍCULA ───────────────────────────────────────
def dibujar_cuadricula():
    global anim_tick, animaciones_ataque
    anim_tick += 1

    for fila in range(FILAS):
        for col in range(COLS):
            x, y = celda_a_pixel(fila, col)
            rect  = pygame.Rect(x, y, CELDA, CELDA)
            color_base = C_CELDA_A if (fila + col) % 2 == 0 else C_CELDA_B

            # Ruta de backtracking animada
            if (fila, col) in ruta_visual:
                idx = ruta_visual.index((fila, col))
                t = (math.sin(anim_tick * 0.05 - idx * 0.3) + 1) / 2
                r = int(color_base[0] + t * 30)
                g = int(color_base[1] + t * 60)
                b = int(color_base[2] + t * 40)
                pygame.draw.rect(pantalla, (r, g, b), rect)
            else:
                pygame.draw.rect(pantalla, color_base, rect)

            # Celda greedy sugerida
            if greedy_celda == (fila, col):
                pulse = abs(math.sin(anim_tick * 0.06))
                overlay = pygame.Surface((CELDA, CELDA), pygame.SRCALPHA)
                overlay.fill((255, 200, 50, int(60 + 60 * pulse)))
                pantalla.blit(overlay, (x, y))

            # Contenido de la celda
            if (fila, col) == POS_CASTILLO:
                rect_redondeado(pantalla, C_CASTILLO2, rect.inflate(-4,-4), 6)
                pygame.draw.rect(pantalla, C_CASTILLO, rect.inflate(-4,-4), 2, border_radius=6)
                s = FUENTE_GRANDE.render("🏰", True, C_TEXTO)
                pantalla.blit(s, (x + CELDA//2 - s.get_width()//2, y + CELDA//2 - s.get_height()//2))

            elif (fila, col) == POS_SPAWN:
                rect_redondeado(pantalla, C_SPAWN2, rect.inflate(-4,-4), 6)
                pygame.draw.rect(pantalla, C_SPAWN, rect.inflate(-4,-4), 2, border_radius=6)
                s = FUENTE_MED.render("SPAWN", True, C_VERDE)
                pantalla.blit(s, (x + CELDA//2 - s.get_width()//2, y + CELDA//2 - s.get_height()//2))

            elif (fila, col) in torres_construidas:
                info  = torres_construidas[(fila, col)]
                tipo  = TIPOS_TORRE[info["tipo"]]
                col1  = tipo["color"]
                col2  = tipo["color2"]
                rect_redondeado(pantalla, col2, rect.inflate(-4,-4), 6)
                pygame.draw.rect(pantalla, col1, rect.inflate(-4,-4), 2, border_radius=6)
                # Icono según tipo
                icono = {"basica": "▲", "reforzada": "★", "elite": "◆"}.get(info["tipo"], "▲")
                s = FUENTE_GRANDE.render(icono, True, C_TEXTO)
                pantalla.blit(s, (x + CELDA//2 - s.get_width()//2, y + CELDA//2 - s.get_height()//2 - 4))
                # Cargas restantes
                sc = FUENTE_PEQUE.render(f"{info['cargas']}", True, C_GOLD)
                pantalla.blit(sc, (x + CELDA - sc.get_width() - 3, y + CELDA - sc.get_height() - 2))

            # Borde selección
            if celda_seleccionada == (fila, col):
                pulse = abs(math.sin(anim_tick * 0.08))
                c_sel = (
                    int(C_SELEC[0]),
                    int(C_SELEC[1] * (0.6 + 0.4 * pulse)),
                    int(C_SELEC[2] * pulse)
                )
                pygame.draw.rect(pantalla, c_sel, rect, 3, border_radius=4)
            else:
                pygame.draw.rect(pantalla, C_BORDE, rect, 1)

    # Rango de ataque de torre seleccionada
    if celda_seleccionada and celda_seleccionada in torres_construidas:
        tf, tc = celda_seleccionada
        rango = TIPOS_TORRE[torres_construidas[celda_seleccionada]["tipo"]]["rango"]
        for f in range(FILAS):
            for c in range(COLS):
                if abs(f - tf) + abs(c - tc) <= rango and (f, c) != celda_seleccionada:
                    x, y = celda_a_pixel(f, c)
                    overlay = pygame.Surface((CELDA, CELDA), pygame.SRCALPHA)
                    overlay.fill((70, 130, 220, 50))
                    pantalla.blit(overlay, (x, y))
                    pygame.draw.rect(pantalla, C_AZUL, (x, y, CELDA, CELDA), 1)

    # Animaciones de ataque (línea torre → enemigo)
    animaciones_vivas = []
    for anim in animaciones_ataque:
        if anim["timer"] > 0:
            tx, ty = celda_a_pixel(anim["torre"][0], anim["torre"][1])
            ex, ey = celda_a_pixel(anim["enemigo"][0], anim["enemigo"][1])
            tc_x, tc_y = tx + CELDA//2, ty + CELDA//2
            ec_x, ec_y = ex + CELDA//2, ey + CELDA//2
            alpha = int(255 * anim["timer"] / 8)
            surf = pygame.Surface((ANCHO, ALTO), pygame.SRCALPHA)
            pygame.draw.line(surf, (255, 220, 50, alpha), (tc_x, tc_y), (ec_x, ec_y), 2)
            pygame.draw.circle(surf, (255, 180, 0, alpha), (ec_x, ec_y), 6)
            pantalla.blit(surf, (0, 0))
            anim["timer"] -= 1
            animaciones_vivas.append(anim)
    animaciones_ataque = animaciones_vivas

    # Enemigos con barra de HP
    if estado_actual and estado_actual.horde_queue:
        for horda in estado_actual.horde_queue:
            for enemigo in horda.enemies:
                f_e, c_e = enemigo.pos.row, enemigo.pos.col
                if 0 <= f_e < FILAS and 0 <= c_e < COLS:
                    x_e, y_e = celda_a_pixel(f_e, c_e)
                    cx, cy = x_e + CELDA//2, y_e + CELDA//2
                    pulse = abs(math.sin(anim_tick * 0.1))
                    radio = int(16 + 3 * pulse)
                    pygame.draw.circle(pantalla, (120, 30, 120), (cx, cy), radio + 3)
                    pygame.draw.circle(pantalla, C_MORADO, (cx, cy), radio)
                    s = FUENTE_MED.render("☠", True, C_TEXTO)
                    pantalla.blit(s, (cx - s.get_width()//2, cy - s.get_height()//2))
                    # Barra de HP sobre el enemigo
                    barra_w = CELDA - 8
                    barra_rect = pygame.Rect(x_e + 4, y_e + 2, barra_w, 5)
                    hp_max = enemigo.hp_max if enemigo.hp_max > 0 else 80
                    barra_vida(pantalla, barra_rect, enemigo.hp, hp_max, C_VERDE)

# ── DIBUJAR HUD SUPERIOR ─────────────────────────────────────
def dibujar_hud_top():
    # Fondo barra superior
    rect_redondeado(pantalla, C_PANEL, pygame.Rect(10, 8, ANCHO - 20, 72), 10, 1, C_BORDE)

    # TICK
    pantalla.blit(FUENTE_PEQUE.render("TICK", True, C_TEXTO2), (28, 18))
    pantalla.blit(FUENTE_TITULO.render(str(current_tick), True, C_GOLD), (28, 34))

    # FASE
    color_fase = C_VERDE if fase == "construccion" else C_ROJO
    label_fase = "◉ CONSTRUCCION" if fase == "construccion" else "◉ COMBATE"
    pantalla.blit(FUENTE_PEQUE.render("FASE", True, C_TEXTO2), (120, 18))
    pantalla.blit(FUENTE_MED.render(label_fase, True, color_fase), (120, 36))

    # HP CASTILLO
    pantalla.blit(FUENTE_PEQUE.render("HP CASTILLO", True, C_TEXTO2), (290, 14))
    barra_vida(pantalla, pygame.Rect(290, 34, 160, 14), hp_castillo, HP_MAX, C_ROJO)
    s = FUENTE_PEQUE.render(f"{hp_castillo}/{HP_MAX}", True, C_TEXTO)
    pantalla.blit(s, (290 + 80 - s.get_width()//2, 52))

    # ORO
    pantalla.blit(FUENTE_PEQUE.render("ORO", True, C_TEXTO2), (475, 18))
    pantalla.blit(FUENTE_TITULO.render(f"${oro_jugador}", True, C_GOLD), (475, 34))

    # STATUS ENGINE
    pantalla.blit(FUENTE_PEQUE.render("ENGINE", True, C_TEXTO2), (620, 18))
    color_st = C_VERDE if wave_status == "ongoing" else C_ROJO
    pantalla.blit(FUENTE_MED.render(wave_status.upper(), True, color_st), (620, 36))

    # OLEADA
    pantalla.blit(FUENTE_PEQUE.render("OLEADA", True, C_TEXTO2), (780, 18))
    pantalla.blit(FUENTE_TITULO.render(str(wave_number), True, C_MORADO), (780, 34))

# ── DIBUJAR PANEL DERECHO ────────────────────────────────────
def dibujar_panel_derecho():
    rx, rw = PX, PW
    rect_panel = pygame.Rect(rx - 8, OY - 4, rw + 18, FILAS * CELDA + 8)
    rect_redondeado(pantalla, C_PANEL, rect_panel, 10, 1, C_BORDE)

    # Título panel
    titulo = {"construccion": "CONSTRUCCIÓN", "accion": "COMBATE",
              "tienda": "TIENDA", "derrota": "DERROTA"}.get(fase, "ACCIONES")
    s = FUENTE_GRANDE.render(titulo, True, C_TEXTO)
    pantalla.blit(s, (rx, OY + 10))

    # ── FASE CONSTRUCCIÓN / TIENDA ────────────────────────────
    if fase in ("construccion", "tienda"):

        # Selector de tipo de torre
        pantalla.blit(FUENTE_PEQUE.render("TIPO DE TORRE:", True, C_TEXTO2), (rx, OY + 36))
        for btn, tipo_key in [(btn_tipo_basica, "basica"),
                               (btn_tipo_reforzada, "reforzada"),
                               (btn_tipo_elite, "elite")]:
            tipo = TIPOS_TORRE[tipo_key]
            selec = (torre_seleccionada_tipo == tipo_key)
            color_fondo  = tipo["color2"] if selec else C_BG2
            color_borde2 = tipo["color"]  if selec else C_BORDE
            rect_redondeado(pantalla, color_fondo, btn, 5, 2, color_borde2)
            nombre = tipo["nombre"]
            costo  = tipo["costo"]
            s1 = FUENTE_PEQUE.render(nombre, True, C_TEXTO)
            s2 = FUENTE_PEQUE.render(f"${costo}", True, C_GOLD if oro_jugador >= costo else C_ROJO)
            pantalla.blit(s1, (btn.x + btn.w//2 - s1.get_width()//2, btn.y + 6))
            pantalla.blit(s2, (btn.x + btn.w//2 - s2.get_width()//2, btn.y + 24))

        # Info celda seleccionada
        if celda_seleccionada:
            f, c = celda_seleccionada
            if (f, c) == POS_CASTILLO:
                tipo_txt, col_tipo = "Castillo [C]", C_ROJO
            elif (f, c) == POS_SPAWN:
                tipo_txt, col_tipo = "Spawn [S]",    C_VERDE
            elif (f, c) in torres_construidas:
                info = torres_construidas[(f, c)]
                tipo_txt = f"Torre {info['tipo']} ({info['cargas']} hordas)"
                col_tipo = TIPOS_TORRE[info["tipo"]]["color"]
            else:
                tipo_txt, col_tipo = "Celda libre", C_TEXTO2

            rect_info = pygame.Rect(rx, OY + 118, rw, 50)
            rect_redondeado(pantalla, C_BG2, rect_info, 6, 1, C_BORDE)
            pantalla.blit(FUENTE_MED.render(f"({f},{c})", True, C_GOLD),   (rx + 8, OY + 122))
            pantalla.blit(FUENTE_MED.render(tipo_txt, True, col_tipo),      (rx + 8, OY + 138))
        else:
            rect_info = pygame.Rect(rx, OY + 118, rw, 50)
            rect_redondeado(pantalla, C_BG2, rect_info, 6, 1, C_BORDE)
            pantalla.blit(FUENTE_MED.render("Clic en celda para seleccionar", True, C_TEXTO2), (rx + 4, OY + 135))

        # Botón greedy
        dibujar_boton(pantalla, btn_greedy, "Sugerir celda (Greedy)", FUENTE_MED, True)

        # Separador + info ruta
        pygame.draw.line(pantalla, C_BORDE,
            (rx, btn_greedy.bottom + 10), (rx + rw, btn_greedy.bottom + 10))
        if ruta_visual:
            color_ruta = C_ROJO if len(ruta_visual) < 6 else C_NARANJA if len(ruta_visual) < 12 else C_VERDE
            s = FUENTE_PEQUE.render(f"Ruta enemiga: {len(ruta_visual)} pasos", True, color_ruta)
        else:
            s = FUENTE_PEQUE.render("Sin ruta — torres bien puestas", True, C_TEXTO2)
        pantalla.blit(s, (rx, btn_greedy.bottom + 16))

        # Botón iniciar / siguiente oleada
        if fase == "construccion":
            rect_redondeado(pantalla, C_VERDE2, btn_start, 8, 1, C_VERDE)
            texto_centrado(pantalla, "▶  INICIAR OLEADA 1", FUENTE_GRANDE, C_TEXTO, btn_start)
        else:  # tienda
            pulse = abs(math.sin(anim_tick * 0.06))
            color_sig = (int(30 + 20*pulse), int(120 + 30*pulse), int(70 + 20*pulse))
            rect_redondeado(pantalla, color_sig, btn_siguiente, 8, 1, C_VERDE)
            texto_centrado(pantalla, f"▶  OLEADA {wave_number + 1}", FUENTE_GRANDE, C_TEXTO, btn_siguiente)

    # ── FASE COMBATE ──────────────────────────────────────────
    elif fase == "accion":
        # Info enemigos vivos
        n_enemigos = 0
        if estado_actual and estado_actual.horde_queue:
            for h in estado_actual.horde_queue:
                n_enemigos += h.size
        rect_info = pygame.Rect(rx, OY + 38, rw, 50)
        rect_redondeado(pantalla, C_BG2, rect_info, 6, 1, C_BORDE)
        pantalla.blit(FUENTE_MED.render(f"Enemigos: {n_enemigos}", True, C_MORADO), (rx + 8, OY + 44))
        pantalla.blit(FUENTE_MED.render(f"Oleada {wave_number}", True, C_TEXTO2), (rx + 8, OY + 62))

        # Botón tick manual
        pulse = abs(math.sin(anim_tick * 0.06))
        color_tick = (int(30 + 20*pulse), int(120 + 30*pulse), int(70 + 20*pulse))
        rect_redondeado(pantalla, color_tick, btn_tick, 8, 1, C_VERDE)
        texto_centrado(pantalla, f"▶  TICK {current_tick + 1}", FUENTE_GRANDE, C_TEXTO, btn_tick)

        # Botón auto-tick
        color_auto  = (180, 40, 40) if auto_tick else C_PANEL2
        borde_auto  = C_ROJO       if auto_tick else C_BORDE2
        rect_redondeado(pantalla, color_auto, btn_auto, 6, 1, borde_auto)
        label_auto = "⏹ AUTO ON" if auto_tick else "⏵ AUTO OFF"
        texto_centrado(pantalla, label_auto, FUENTE_MED, C_TEXTO, btn_auto)

    # ── FASE DERROTA ──────────────────────────────────────────
    elif fase == "derrota":
        rect_info = pygame.Rect(rx, OY + 38, rw, 80)
        rect_redondeado(pantalla, C_BG2, rect_info, 6, 1, C_ROJO)
        s1 = FUENTE_GRANDE.render("¡CASTILLO", True, C_ROJO)
        s2 = FUENTE_GRANDE.render("DESTRUIDO!", True, C_ROJO)
        pantalla.blit(s1, (rx + rw//2 - s1.get_width()//2, OY + 46))
        pantalla.blit(s2, (rx + rw//2 - s2.get_width()//2, OY + 68))
        s3 = FUENTE_MED.render(f"Oleadas: {wave_number}", True, C_TEXTO2)
        pantalla.blit(s3, (rx + rw//2 - s3.get_width()//2, OY + 96))

    # Botón reset (siempre visible)
    rect_redondeado(pantalla, C_BG2, btn_reset, 6, 1, C_BORDE)
    texto_centrado(pantalla, "↺  Reiniciar", FUENTE_PEQUE, C_TEXTO2, btn_reset)

    # Leyenda tipos de torre
    ly = btn_reset.bottom + 10
    pantalla.blit(FUENTE_PEQUE.render("TORRES:", True, C_TEXTO2), (rx, ly))
    ly += 14
    for tipo_key, tipo in TIPOS_TORRE.items():
        pygame.draw.rect(pantalla, tipo["color"], (rx, ly + 2, 10, 10), border_radius=2)
        s = FUENTE_PEQUE.render(f"{tipo['nombre']} ${tipo['costo']} dmg:{tipo['dmg']} x{tipo['hordas']}", True, C_TEXTO2)
        pantalla.blit(s, (rx + 14, ly))
        ly += 16

# ── DIBUJAR LOG INFERIOR ─────────────────────────────────────
def dibujar_log():
    rect_log = pygame.Rect(10, ALTO - 50, OX + COLS * CELDA - 10, 40)
    rect_redondeado(pantalla, C_PANEL, rect_log, 8, 1, C_BORDE)

    global log_timer
    if log_timer > 0:
        alpha_factor = min(1.0, log_timer / 30)
        color_log = tuple(int(c * alpha_factor) for c in log_color)
        log_timer -= 1
    else:
        color_log = C_TEXTO2

    # truncar si es muy largo
    msg = log_mensaje
    s = FUENTE_MED.render(msg, True, color_log)
    while s.get_width() > rect_log.w - 20 and len(msg) > 10:
        msg = msg[:-4] + "..."
        s = FUENTE_MED.render(msg, True, color_log)
    pantalla.blit(s, (rect_log.x + 12, rect_log.y + 12))

# ── RESET ────────────────────────────────────────────────────
def reset_juego():
    global torres_construidas, celda_seleccionada, current_tick
    global oro_jugador, hp_castillo, wave_status, wave_number
    global fase, ruta_visual, greedy_celda, estado_actual, path_horda_actual
    global animaciones_ataque, torre_seleccionada_tipo
    torres_construidas     = {}
    celda_seleccionada     = None
    current_tick           = 0
    oro_jugador            = ORO_INICIAL
    hp_castillo            = HP_MAX
    wave_status            = "ongoing"
    wave_number            = 0
    fase                   = "construccion"
    ruta_visual            = []
    greedy_celda           = None
    estado_actual          = None
    path_horda_actual      = []
    animaciones_ataque     = []
    torre_seleccionada_tipo = "basica"
    if os.path.exists(bridge_sistema.state_path):
        try:
            os.remove(bridge_sistema.state_path)
        except:
            pass
    recalcular_ruta()
    set_log("Juego reiniciado. Coloca torres y presiona INICIAR OLEADA.", C_VERDE)

# ── PROCESAR TICK ────────────────────────────────────────────
def procesar_tick():
    global current_tick, estado_actual, hp_castillo, oro_jugador
    global fase, wave_number, animaciones_ataque

    current_tick += 1
    lista_torres = [
        Tower(id=f"tower_{i}", pos=Pos(f, c),
              atk_range=TIPOS_TORRE[info["tipo"]]["rango"],
              dmg=TIPOS_TORRE[info["tipo"]]["dmg"])
        for i, ((f, c), info) in enumerate(torres_construidas.items())
    ]
    hp_enemigo    = 80  + (wave_number - 1) * 20
    dmg_enemigo   = 10  + (wave_number - 1) * 5
    cant_enemigos = 3   + (wave_number - 1)

    horde_con_path = []
    if estado_actual and estado_actual.horde_queue and estado_actual.horde_queue[0].size > 0:
        for h in estado_actual.horde_queue:
            h.path = [Pos(r, c) for r, c in path_horda_actual]
            horde_con_path.append(h)
    else:
        spawn_pos = Pos(POS_SPAWN[0], POS_SPAWN[1])
        enemigos_iniciales = [
            Enemy(id=f"enemy_w{wave_number}_{i}",
                  hp=hp_enemigo, hp_max=hp_enemigo,
                  pos=spawn_pos, dmg=dmg_enemigo, atk_range=1)
            for i in range(cant_enemigos)
        ]
        path_pos = [Pos(r, c) for r, c in path_horda_actual]
        horde_con_path = [Horde(
            id="horde_0", size=cant_enemigos,
            path=path_pos, enemies=enemigos_iniciales
        )]

    datos = InputState(
        meta        = {"tick": current_tick, "wave": wave_number},
        castle      = Castle(id="castle_0", hp=hp_castillo, hp_max=HP_MAX),
        towers      = lista_torres,
        horde_queue = horde_con_path,
        gold        = oro_jugador,
        phase       = "action"
    )
    try:
        bridge_sistema.write_input(datos)
        success = bridge_sistema.run_engine_tick()
        if success:
            prev_enemies = {
                e.id: e.pos
                for h in (estado_actual.horde_queue if estado_actual else [])
                for e in h.enemies
            }
            estado_actual = bridge_sistema.read_state(bridge_sistema.state_path)
            hp_castillo   = estado_actual.castle.hp
            oro_ganado    = estado_actual.tick_result.gold_earned
            oro_jugador  += oro_ganado

            # Animaciones de ataque
            if estado_actual.horde_queue and torres_construidas:
                for killed_id in estado_actual.horde_queue[0].killed_ids:
                    if killed_id in prev_enemies:
                        pos_e = prev_enemies[killed_id]
                        torre_cercana = min(
                            torres_construidas.keys(),
                            key=lambda t: abs(t[0]-pos_e.row)+abs(t[1]-pos_e.col)
                        )
                        animaciones_ataque.append({
                            "torre":   torre_cercana,
                            "enemigo": (pos_e.row, pos_e.col),
                            "timer":   10
                        })

            if oro_ganado > 0:
                set_log(f"Tick {current_tick} | HP:{hp_castillo} | +${oro_ganado} oro", C_VERDE)
            else:
                set_log(f"Tick {current_tick} | HP:{hp_castillo}/{HP_MAX}", C_TEXTO2)

            if estado_actual.tick_result.status == "defeat":
                fase = "derrota"
                set_log("¡El castillo fue destruido!", C_ROJO)
            elif estado_actual.tick_result.status == "wave_cleared" or \
                 (estado_actual.horde_queue and estado_actual.horde_queue[0].size == 0):
                for pos_t in list(torres_construidas.keys()):
                    torres_construidas[pos_t]["cargas"] -= 1
                    if torres_construidas[pos_t]["cargas"] <= 0:
                        del torres_construidas[pos_t]
                recalcular_ruta()
                fase = "tienda"
                set_log(f"¡Oleada {wave_number} completada! Compra torres para la siguiente.", C_GOLD)
        else:
            set_log("Error: ejecutable C++ no encontrado en /engine.", C_ROJO)
    except Exception as e:
        set_log(f"Error Tick {current_tick}: {type(e).__name__} — {e}", C_ROJO)

# ── GAME LOOP ────────────────────────────────────────────────
ejecutando = True

set_log("Bienvenido a Tower Siege. Usa [T] para construir torres.", C_VERDE)
recalcular_ruta()

while ejecutando:
    vidas_reales = estado_actual.castle.hp         if estado_actual else hp_castillo
    oro_real     = estado_actual.tick_result.gold_earned if estado_actual else 0
    wave_status  = estado_actual.tick_result.status      if estado_actual else wave_status

    for evento in pygame.event.get():
        if evento.type == pygame.QUIT:
            ejecutando = False

        elif evento.type == pygame.MOUSEBUTTONDOWN:
            pos_mouse = pygame.mouse.get_pos()
            celda = pixel_a_celda(*pos_mouse)

            if celda:
                celda_seleccionada = celda
                f, c = celda
                if (f, c) == POS_CASTILLO:
                    set_log(f"Castillo seleccionado — HP: {hp_castillo}/{HP_MAX}", C_ROJO)
                elif (f, c) == POS_SPAWN:
                    set_log("Punto de Spawn — los enemigos aparecen aquí.", C_VERDE)
                elif (f, c) in torres_construidas:
                    set_log(f"Torre en ({f},{c}) — presiona [T] para eliminarla.", C_AZUL)
                else:
                    costo_actual = TIPOS_TORRE[torre_seleccionada_tipo]["costo"]
                    set_log(f"Celda libre ({f},{c}) — presiona [T] para construir torre (${costo_actual}).")

            # Greedy
            elif btn_greedy.collidepoint(pos_mouse) and fase in ("construccion","tienda"):
                sugerencia = evaluar_amenaza_greedy(torres_construidas, POS_SPAWN, POS_CASTILLO, ruta_visual)
                greedy_celda = sugerencia
                celda_seleccionada = sugerencia
                set_log(f"Greedy sugiere: ({sugerencia[0]},{sugerencia[1]}) — mayor amenaza.", C_GOLD)

            # Selector tipo de torre
            elif btn_tipo_basica.collidepoint(pos_mouse) and fase in ("construccion","tienda"):
                torre_seleccionada_tipo = "basica"
                set_log("Torre Básica — $50, daño 25, 2 hordas.", C_AZUL)
            elif btn_tipo_reforzada.collidepoint(pos_mouse) and fase in ("construccion","tienda"):
                torre_seleccionada_tipo = "reforzada"
                set_log("Torre Reforzada — $100, daño 40, 4 hordas.", C_VERDE)
            elif btn_tipo_elite.collidepoint(pos_mouse) and fase in ("construccion","tienda"):
                torre_seleccionada_tipo = "elite"
                set_log("Torre Élite — $200, daño 70, rango 3, 8 hordas.", C_GOLD)

            # Iniciar oleada 1
            elif btn_start.collidepoint(pos_mouse) and fase == "construccion":
                fase = "accion"
                wave_number = 1
                path_horda_actual = list(ruta_visual) if ruta_visual else [
                    (8,4),(7,4),(6,4),(5,4),(4,4),(3,4),(2,4),(1,4),(0,4)
                ]
                estado_actual = None
                set_log(f"¡Oleada {wave_number} iniciada! Presiona TICK.", C_VERDE)

            # Siguiente oleada desde tienda
            elif btn_siguiente.collidepoint(pos_mouse) and fase == "tienda":
                fase = "accion"
                wave_number += 1
                path_horda_actual = list(ruta_visual) if ruta_visual else path_horda_actual
                estado_actual = None
                set_log(f"¡Oleada {wave_number} iniciada! Enemigos más fuertes.", C_ROJO)

            # Engine tick
            elif btn_tick.collidepoint(pos_mouse) and fase == "accion":
                current_tick += 1
                lista_torres = [
                    Tower(id=f"tower_{i}", pos=Pos(f, c),
                          atk_range=TIPOS_TORRE[info["tipo"]]["rango"],
                          dmg=TIPOS_TORRE[info["tipo"]]["dmg"])
                    for i, ((f, c), info) in enumerate(torres_construidas.items())
                ]
                hp_enemigo    = 80  + (wave_number - 1) * 20
                dmg_enemigo   = 10  + (wave_number - 1) * 5
                cant_enemigos = 3   + (wave_number - 1)

                horde_con_path = []
                if estado_actual and estado_actual.horde_queue and estado_actual.horde_queue[0].size > 0:
                    for h in estado_actual.horde_queue:
                        h.path = [Pos(r, c) for r, c in path_horda_actual]
                        horde_con_path.append(h)
                else:
                    spawn_pos = Pos(POS_SPAWN[0], POS_SPAWN[1])
                    enemigos_iniciales = [
                        Enemy(id=f"enemy_w{wave_number}_{i}",
                              hp=hp_enemigo, hp_max=hp_enemigo,
                              pos=spawn_pos, dmg=dmg_enemigo, atk_range=1)
                        for i in range(cant_enemigos)
                    ]
                    path_pos = [Pos(r, c) for r, c in path_horda_actual]
                    horde_con_path = [Horde(
                        id="horde_0", size=cant_enemigos,
                        path=path_pos, enemies=enemigos_iniciales
                    )]

                datos = InputState(
                    meta        = {"tick": current_tick, "wave": wave_number},
                    castle      = Castle(id="castle_0", hp=vidas_reales, hp_max=HP_MAX),
                    towers      = lista_torres,
                    horde_queue = horde_con_path,
                    gold        = oro_jugador,
                    phase       = "action"
                )
                try:
                    bridge_sistema.write_input(datos)
                    success = bridge_sistema.run_engine_tick()
                    if success:
                        prev_enemies = {
                            e.id: e.pos
                            for h in (estado_actual.horde_queue if estado_actual else [])
                            for e in h.enemies
                        }
                        estado_actual = bridge_sistema.read_state(bridge_sistema.state_path)
                        hp_castillo   = estado_actual.castle.hp
                        oro_ganado    = estado_actual.tick_result.gold_earned
                        oro_jugador  += oro_ganado
                        if estado_actual.horde_queue and torres_construidas:
                            for killed_id in getattr(estado_actual.horde_queue[0], "killed_ids", []):
                                if killed_id in prev_enemies:
                                    pos_e = prev_enemies[killed_id]
                                    torre_cercana = min(
                                        torres_construidas.keys(),
                                        key=lambda t: abs(t[0]-pos_e.row)+abs(t[1]-pos_e.col)
                                    )
                                    animaciones_ataque.append({
                                        "torre":   torre_cercana,
                                        "enemigo": (pos_e.row, pos_e.col),
                                        "timer":   8
                                    })
                        if oro_ganado > 0:
                            set_log(f"Tick {current_tick} | HP:{hp_castillo} | +${oro_ganado} oro", C_VERDE)
                        else:
                            set_log(f"Tick {current_tick} | HP:{hp_castillo}/{HP_MAX}", C_TEXTO2)
                        if estado_actual.tick_result.status == "defeat":
                            fase = "derrota"
                            set_log("¡El castillo fue destruido!", C_ROJO)
                        elif estado_actual.tick_result.status == "wave_cleared" or                              (estado_actual.horde_queue and estado_actual.horde_queue[0].size == 0):
                            for pos_t in list(torres_construidas.keys()):
                                torres_construidas[pos_t]["cargas"] -= 1
                                if torres_construidas[pos_t]["cargas"] <= 0:
                                    del torres_construidas[pos_t]
                            recalcular_ruta()
                            fase = "tienda"
                            set_log(f"¡Oleada {wave_number} completada! Compra torres para la siguiente.", C_GOLD)
                    else:
                        set_log("Error: ejecutable C++ no encontrado en /engine.", C_ROJO)
                except Exception as e:
                    set_log(f"Error Tick {current_tick}: {type(e).__name__} — {e}", C_ROJO)

            # Reset
            elif btn_reset.collidepoint(pos_mouse):
                reset_juego()

        elif evento.type == pygame.KEYDOWN:
            if evento.key == pygame.K_t and celda_seleccionada:
                f, c = celda_seleccionada
                if (f, c) in (POS_SPAWN, POS_CASTILLO):
                    set_log("No puedes construir aquí.", C_ROJO)
                elif fase not in ("construccion","tienda"):
                    set_log("Solo puedes construir en fase de construcción o tienda.", C_ROJO)
                elif (f, c) in torres_construidas:
                    info  = torres_construidas[(f, c)]
                    venta = TIPOS_TORRE[info["tipo"]]["costo"] // 2
                    del torres_construidas[(f, c)]
                    oro_jugador += venta
                    greedy_celda = None
                    recalcular_ruta()
                    set_log(f"Torre eliminada en ({f},{c}). +${venta} de venta.", C_NARANJA)
                else:
                    tipo_info = TIPOS_TORRE[torre_seleccionada_tipo]
                    costo = tipo_info["costo"]
                    if oro_jugador < costo:
                        set_log(f"Oro insuficiente. Necesitas ${costo}, tienes ${oro_jugador}.", C_ROJO)
                    else:
                        torres_construidas[(f, c)] = {"tipo": torre_seleccionada_tipo, "cargas": tipo_info["hordas"]}
                        recalcular_ruta()
                        if not ruta_visual:
                            del torres_construidas[(f, c)]
                            recalcular_ruta()
                            set_log("No puedes bloquear el único camino al castillo.", C_ROJO)
                        else:
                            oro_jugador -= costo
                            greedy_celda = None
                            set_log(f"Torre {tipo_info['nombre']} en ({f},{c}). Ruta: {len(ruta_visual)} pasos.", C_AZUL)

            elif evento.key == pygame.K_ESCAPE:
                celda_seleccionada = None
                greedy_celda = None

    # ── RENDER ──────────────────────────────────────────────
    pantalla.fill(C_BG)

    # Fondo tablero
    tablero_rect = pygame.Rect(OX - 6, OY - 6, COLS * CELDA + 12, FILAS * CELDA + 12)
    rect_redondeado(pantalla, C_BG2, tablero_rect, 8, 1, C_BORDE)

    dibujar_cuadricula()
    dibujar_hud_top()
    dibujar_panel_derecho()
    dibujar_log()

    # Título del juego (esquina inferior derecha del panel)
    s = FUENTE_PEQUE.render("TOWER SIEGE v1.0 — Team 2", True, C_BORDE2)
    pantalla.blit(s, (ANCHO - s.get_width() - 15, ALTO - 18))

    pygame.display.flip()
    reloj.tick(60)

pygame.quit()
sys.exit()
