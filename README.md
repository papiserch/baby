# BabyAir 👼

Exciter de aire / alta frecuencia en VST3, inspirado en el concepto de "Fresh Air":
genera armónicos nuevos en dos bandas y los mezcla con la señal original.

- **Mid-High**: banda 2.5 kHz – 8 kHz
- **High (Air)**: banda > 8 kHz
- **Mix** y **Output** (±12 dB)
- Interfaz con el angelito animado: flota, respira, se agranda y suelta corazones con el nivel del audio.

## Compilar en GitHub (lo más fácil)

1. Crea un repo nuevo en GitHub y sube todo el contenido de esta carpeta.
2. Ve a la pestaña **Actions** → "Build BabyAir (Windows VST3)" → espera a que termine.
3. Descarga el artifact **BabyAir-VST3-Windows**.

## Instalar en FL Studio

1. Copia la carpeta `BabyAir.vst3` a `C:\Program Files\Common Files\VST3\`
2. En FL Studio: **Options → Manage plugins → Find installed plugins** (con "Verify plugins" activado).
3. Búscalo como **BabyAir** en el Plugin Database y agrégalo a un canal del Mixer.

## Compilar en local (Windows)

Requisitos: Visual Studio 2022 (carga de trabajo "Desarrollo para escritorio con C++") y CMake.

```
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

El resultado queda en `build/BabyAir_artefacts/Release/VST3/`.

## Notas

- Solo VST3 (FL Studio lo soporta). VST2 ya no se puede distribuir por la licencia de Steinberg.
- No es una copia de Slate Digital Fresh Air (su algoritmo es propietario); es un exciter propio con la misma idea.
- La imagen del angelito (`Assets/baby.png`) es arte de terceros: úsala solo si tienes derecho a hacerlo antes de publicar el repo. Puedes reemplazarla por otra con el mismo nombre.
