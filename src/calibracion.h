#pragma once

// Convierte la lectura cruda del MAX6675 a temperatura real corregida usando
// una tabla de calibración con interpolación lineal por tramos.
//
// La tabla se obtiene midiendo con un termómetro de referencia en estado
// estacionario para varias temperaturas, y editando CALIB_TABLE en el .cpp.
//
// Para valores fuera de los extremos de la tabla, se extrapola usando la
// pendiente del segmento más cercano.
float calibrarTemperatura(float rawMax6675);
