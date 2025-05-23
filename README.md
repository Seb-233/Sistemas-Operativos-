# 📚 Sistema de Préstamo de Libros

Proyecto para el curso **Sistemas Operativos (SIU4085)**  
Pontificia Universidad Javeriana – Mayo 2025  
 

---

## 📝 Descripción General

Este proyecto simula un sistema distribuido para la gestión de préstamos, devoluciones y renovaciones de libros en una biblioteca. Está desarrollado en lenguaje C utilizando **procesos e hilos POSIX**, **pipes con nombre (FIFO)** para comunicación interprocesos, y técnicas de **sincronización** como mutex y semáforos.

---

## 🧱 Estructura del Proyecto

```
prestamo_libros/
│
├── Makefile                    # Compila solicitante y receptor
│
├── receptor.c                  # Proceso RP (lee pipe, atiende solicitudes, crea hilos)
├── solicitante.c               # Proceso PS (archivo o menú interactivo)
│
├── hilos/
│   ├── hilo_aux1.c             # Hilo productor/consumidor: devolución y renovación
│   ├── hilo_aux2.c             # Hilo para comandos del usuario ('s', 'r')
│   └── hilos.h                 # Declaraciones de los hilos
│
├── bd/
│   ├── base_datos.c            # Funciones para leer y escribir libros
│   ├── utilidades.c            # Funciones auxiliares (fechas, impresión, validaciones)
│   └── tipos.h                 # Estructuras: Libro, Ejemplar, Solicitud, enums, defines
│
├── datos/
│   ├── base_datos.txt          # Base de datos inicial de libros
│   ├── archivo_solicitudes.txt # Solicitudes de entrada del PS (-i)
│   └── reporte_salida.txt      # Archivo de salida generado por el RP (-s)
│
└── informe_proyecto.pdf        # Informe explicando el diseño, pruebas y resultados


---

## ⚙️ Compilación

El proyecto se compila desde consola de Linux usando el `makefile`:

```bash
make all
```

Esto genera dos ejecutables:

- `solicitante`
- `receptor`

Para limpiar los archivos objeto y ejecutables:

```bash
make clean
```

---

## ▶️ Ejecución

### 1. Receptor de Peticiones (RP)

```bash
./receptor -p pipeReceptor -f base_datos.txt [-v] [-s salida.txt]
```

- `-p pipeReceptor`: nombre del pipe para comunicación.
- `-f base_datos.txt`: archivo con la base de datos inicial.
- `-v`: (opcional) modo verbose, imprime cada solicitud recibida.
- `-s salida.txt`: (opcional) archivo donde se guarda la BD al finalizar.

---

### 2. Proceso Solicitante (PS)

#### a) Modo Menú Interactivo:

```bash
./solicitante -p pipeReceptor
```

#### b) Modo Automático (archivo):

```bash
./solicitante -i entrada.txt -p pipeReceptor
```

- `entrada.txt` debe contener líneas en el formato:
  ```
  D, Cálculo Diferencial, 1200
  R, Hamlet, 234
  P, Álgebra Lineal, 111
  Q, Salir, 0
  ```

---

## 📂 Formato de la Base de Datos

```
Nombre del Libro, ISBN, Número de Ejemplares
1, D, 1-10-2021
2, P, 1-10-2021
...
```

- `D`: disponible  
- `P`: prestado

---

## 🔄 Funcionalidades

- 📘 Préstamo de libros (si hay ejemplares disponibles).
- 🔁 Renovación de libros (agrega una semana a la fecha actual).
- 📤 Devolución de libros (marca ejemplar como disponible).
- 📊 Generación de reporte por consola con comando `r`.
- ❌ Finalización controlada del sistema con comando `s`.

---

## 🛡️ Sincronización

- Se utiliza un **buffer compartido** tipo productor–consumidor entre el proceso RP y el hilo auxiliar 1.
- Controlado mediante **mutex y variables de condición** para evitar condiciones de carrera.

---

## 👨‍💻 Comandos en Tiempo de Ejecución

Desde la consola del receptor:

- `r`: muestra un reporte de los libros prestados, devueltos o renovados.
- `s`: finaliza la ejecución de todos los procesos e hilos.

---

## 📌 Consideraciones

- Todos los PS se comunican por el mismo pipe.
- El archivo de entrada debe tener el formato exacto exigido.
- El sistema ignora peticiones a libros inexistentes, mostrando un mensaje.

---

## 🧪 Pruebas

Se recomienda probar con:

- Múltiples procesos solicitantes al tiempo.
- Archivos de entrada válidos e inválidos.
- Activación de modo verbose (`-v`) para depuración.
- Pruebas manuales de los comandos `r` y `s`.

---

## 📄 Licencia

Uso académico – Pontificia Universidad Javeriana.  
Prohibida su distribución o reutilización fuera del ámbito del curso.
