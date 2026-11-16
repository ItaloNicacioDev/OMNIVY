# OMNIVY — Instruções de Build

## Pré-requisitos

- Windows 11 x64
- Visual Studio Build Tools 2026 (MSVC x64) — `E:\VS_TOOLS`
- Windows SDK 10.0.26100.0
- CMake 3.30.5 — `F:\BACKUP\FREELA\PROGRAMS\VST\Dependencies\Tools\cmake\`
- Ninja — `F:\BACKUP\FREELA\PROGRAMS\VST\Dependencies\Tools\ninja\`
- JUCE 9.0.5 — `F:\BACKUP\FREELA\PROGRAMS\VST\Dependencies\JUCE\`
- Git 2.53.0

## Comandos de Build

### 1. Carregar ambiente MSVC

```cmd
call E:\VS_TOOLS\VC\Auxiliary\Build\vcvars64.bat
```

### 2. Configurar CMake

```cmd
F:\BACKUP\FREELA\PROGRAMS\VST\Dependencies\Tools\cmake\cmake-3.30.5-windows-x86_64\bin\cmake.exe -S . -B Builds -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_MAKE_PROGRAM="F:\BACKUP\FREELA\PROGRAMS\VST\Dependencies\Tools\ninja\ninja.exe" -DCMAKE_C_COMPILER=cl.exe -DCMAKE_CXX_COMPILER=cl.exe -DJUCE_PATH=F:\BACKUP\FREELA\PROGRAMS\VST\Dependencies\JUCE
```

### 3. Compilar

```cmd
F:\BACKUP\FREELA\PROGRAMS\VST\Dependencies\Tools\ninja\ninja.exe -C Builds
```

## Saída do Build

| Artefato | Caminho |
|----------|---------|
| Plugin VST3 | `Builds/Release/VST3/OmnivyPlugin.vst3/Contents/x86_64-win/OmnivyPlugin.vst3` |
| Standalone | `Builds/Release/OmnivyStandalone.exe` |

## Scripts Auxiliares

- `C:/Users/eufra/AppData/Local/hermes/cache/scratch/configure_omnivy_ninja2.bat` — configuração CMake
- `C:/Users/eufra/AppData/Local/hermes/cache/scratch/build_omnivy.bat` — compilação

## Versões Confirmadas

| Ferramenta | Versão |
|------------|--------|
| MSVC (cl.exe) | 19.51.36256.0 |
| Windows SDK | 10.0.26100.0 |
| CMake | 3.30.5 |
| Ninja | 1.12.1 |
| JUCE | 9.0.5 |
| Git | 2.53.0 |
| VS Code | 1.146.0 |
