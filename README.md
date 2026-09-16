## Xerces Chat (Aplicación de chat)
Este repositorio contiene una aplicación de chat programada en C y Kotlin.
Forma parte del proyecto 1 de la materia "Modelado y programación" de la facultad de ciencias de la UNAM.

### Requisitos

Servidor:
- Meson
- cJSON
- gcc
- pkg-config


Cliente:
- Kotlin
- Gradle
- OpenJDK 21

Usando Void Linux:
```
# xbps-install -S meson pkg-config cJSON-devel openjdk21 gradle kotlin-bin
```
### Instalación

Clona el repositorio:
```
git clone https://github.com/bsoto-fc/proyecto-1-myp.git
cd proyecto-1-myp
git submodule init
git submodule update
```

Para el servidor:
```
cd server/
meson setup builddir/
meson compile -C builddir/
./builddir/server
```

Para el cliente:
```
cd gui/
./gradlew run # Utilizando el wrapper de gradle.
gradle run # Utilizando la instalación del sistema de gradle.
```
