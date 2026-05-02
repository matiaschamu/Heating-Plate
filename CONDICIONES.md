# Condiciones y Requisitos del Proyecto

Acá se documentan las condiciones que debe cumplir el firmware. Cada sección agrupa un aspecto del sistema.

---

## Puerto Serie

| # | Condición | Estado |
|---|-----------|--------|
| 1 | El puerto serie siempre termina cada mensaje con `\r\n` (ASCII 13 + ASCII 10) | Implementado |

**Detalles:**
- ASCII 10 = `\n` (LF, Line Feed)
- ASCII 13 = `\r` (CR, Carriage Return)
- Usar `Serial.print("...\r\n")` en lugar de `Serial.println()` para garantizarlo

---

## ADC / Entradas Analógicas

| # | Condición | Estado |
|---|-----------|--------|
| 1 | Los ADC son configurados a máxima resolución de 0 a 3,3 V | Pendiente |
| 2 | Las lecturas del ADC que sean promediadas para eliminar error | Pendiente |

---

## Charlieplexing / Display

### Pines comunes (selección de dígito)

| Dígito | GPIO | Posición |
|--------|------|----------|
| 1      | 33   | Primero (izquierda) |
| 2      | 26   | Segundo |
| 3      | 14   | Tercero |
| 4      | 13   | Cuarto (derecha) |

### Pines de segmento (la dirección define qué segmento)

> La polaridad es siempre relativa al pin común del dígito activo.

| GPIO | Como cátodo `(-)` → segmento | Como ánodo `(+)` → segmento |
|------|------------------------------|------------------------------|
| 32   | A (trazo superior)           | E (trazo inferior izquierdo) |
| 25   | B (trazo superior derecho)   | F (trazo superior izquierdo) |
| 27   | C (trazo inferior derecho)   | G (trazo central)            |
| 12   | D (trazo inferior)           | Colon / DP (ver abajo)       |

```
 AAA
F   B
F   B
 GGG
E   C
E   C
 DDD
```

### Elementos especiales

| Elemento | Ánodo `(+)` | Cátodo `(-)` | Condición |
|----------|-------------|--------------|-----------|
| Colon `:` (puntos centrales hora) | GPIO 12 | GPIO 13 | Ambos activos simultáneamente |
| Punto decimal dígito 2            | GPIO 12 | GPIO 26 | Ambos activos simultáneamente |
| LED Power                         | GPIO 33 | GPIO 26 | — |
| LED Temperatura                   | GPIO 26 | GPIO 33 | — |
| LED Timer                         | GPIO 13 | GPIO 26 | — |

### Condiciones del driver

| # | Condición | Estado |
|---|-----------|--------|
| 1 | Todos los pines arrancan en hi-Z (INPUT); solo el par activo se maneja como OUTPUT | Implementado |
| 2 | El multiplexado cicla por los 4 dígitos dividiendo cada uno en dos fases (COM+/COM-) | Implementado |
| 3 | Refresh rate ≥ 100 Hz para evitar parpadeo visible | Implementado |

---

## General / Timing

| # | Condición | Estado |
|---|-----------|--------|
| 1 | En un main.h dejar una badera llamada Debug que lo que hace sea habilitar la el debug que se imprime por el puerto serie | Pendiente |

---

> **Estados posibles:** `Pendiente` | `Implementado` | `Verificado` | `Descartado`
