# OMNIVY — Ambiente de Desenvolvimento

## Ferramentas Encontradas

| Ferramenta | Status | Versão | Caminho Real | Comando de Verificação |
|------------|--------|--------|--------------|----------------------|
| VS Code | Encontrada e funcional | 1.146.0 | `D:\VSCODE\Microsoft VS Code\bin\code` | `code --version` |
| Git | Encontrada e funcional | 2.53.0.windows.3 | `C:\Program Files\Git\cmd\git.exe` | `git --version` |
| MSVC (cl.exe) | Encontrada e funcional | 19.51.36256.0 | `E:\VS_TOOLS\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\cl.exe` | `cl /Bv` (após vcvars64.bat) |
| Linker MSVC | Encontrada e funcional | 14.51.36256.0 | `E:\VS_TOOLS\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\link.exe` | `where link` (após vcvars64.bat) |
| MSBuild C++ | Encontrada e funcional | v150, v160, v170, v180 | `E:\VS_TOOLS\MSBuild\Microsoft\VC\` | `dir` targets Microsoft.Cpp.* |
| Windows SDK | Encontrada e funcional | 10.0.26100.0 | `C:\Program Files (x86)\Windows Kits\10` | `dir` Include/Lib |
| CMake | Instalado nesta fase | 3.30.5 | `F:\BACKUP\FREELA\PROGRAMS\VST\Dependencies\Tools\cmake\cmake-3.30.5-windows-x86_64\bin\cmake.exe` | `cmake --version` |
| Ninja | Instalado nesta fase | 1.12.1 | `F:\BACKUP\FREELA\PROGRAMS\VST\Dependencies\Tools\ninja\ninja.exe` | `ninja --version` |
| Clang | Ausente | — | — | `where clang.exe` |
| GCC/MinGW | Ausente | — | — | `where g++.exe` |
| JUCE | Instalado nesta fase | 9.0.5 | `F:\BACKUP\FREELA\PROGRAMS\VST\Dependencies\JUCE` | `git log --oneline -1` |

## Caminhos Importantes

| Item | Caminho |
|------|---------|
| Projeto OMNIVY | `F:\BACKUP\FREELA\PROGRAMS\VST\OMNIVY` |
| Dependências compartilhadas | `F:\BACKUP\FREELA\PROGRAMS\VST\Dependencies` |
| JUCE SDK | `F:\BACKUP\FREELA\PROGRAMS\VST\Dependencies\JUCE` |
| CMake | `F:\BACKUP\FREELA\PROGRAMS\VST\Dependencies\Tools\cmake\` |
| Ninja | `F:\BACKUP\FREELA\PROGRAMS\VST\Dependencies\Tools\ninja\` |
| Build Tools MSVC | `E:\VS_TOOLS` |
| vcvars64.bat | `E:\VS_TOOLS\VC\Auxiliary\Build\vcvars64.bat` |
| Windows SDK | `C:\Program Files (x86)\Windows Kits\10` |

## Variáveis de Ambiente (não modificadas globalmente)

Nenhuma variável de ambiente global foi modificada. O ambiente MSVC é carregado por sessão via `vcvars64.bat`.

## Regras

- Não instalar Visual Studio IDE
- Não instalar outra IDE
- Não modificar PATH global sem autorização
- Priorizar F: para dependências e builds
- Não ocupar C: desnecessariamente
