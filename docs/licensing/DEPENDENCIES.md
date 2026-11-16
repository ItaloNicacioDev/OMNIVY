# OMNIVY — Dependências e Licenças

## Dependências do Projeto

| Dependência | Versão | Licença | Caminho | Uso |
|-------------|--------|---------|---------|-----|
| JUCE | 9.0.5 | AGPLv3 / Comercial | `F:\BACKUP\FREELA\PROGRAMS\VST\Dependencies\JUCE` | Framework principal (áudio, GUI, plugin) |
| VST3 SDK (Steinberg) | Embutido no JUCE | MIT / Comercial | `.../JUCE/modules/juce_audio_processors_headless/format_types/VST3_SDK` | Formato de plugin VST3 |
| CMake | 3.30.5 | BSD-like | `F:\BACKUP\FREELA\PROGRAMS\VST\Dependencies\Tools\cmake\` | Sistema de build |
| Ninja | 1.12.1 | Apache 2.0 | `F:\BACKUP\FREELA\PROGRAMS\VST\Dependencies\Tools\ninja\` | Gerador de build |
| MSVC | 14.51.36256 | Proprietária (Microsoft) | `E:\VS_TOOLS` | Compilador C++ |
| Windows SDK | 10.0.26100.0 | Proprietária (Microsoft) | `C:\Program Files (x86)\Windows Kits\10` | Headers e bibliotecas Windows |

## Licença do Projeto OMNIVY

**GNU Affero General Public License v3.0 (AGPLv3)**

Veja o arquivo `LICENSE` na raiz do projeto.

## Licença JUCE

O JUCE é dual-licenciado:
- **AGPLv3**: open-source, obrigatório abrir código-fonte de obras derivadas
- **Comercial**: permite distribuição fechada/proprietária

Este projeto utiliza o JUCE sob os termos da **AGPLv3**.

## Obrigações de Atribuição

- O projeto OMNIVY é distribuído sob AGPLv3
- O código-fonte deve estar disponível para usuários que interagem com o software em rede
- O aviso de uso do JUCE deve ser mantido (conforme termos da licença)

## Verificação

Todas as licenças foram confirmadas em fontes oficiais:
- https://www.gnu.org/licenses/agpl-3.0.txt
- https://juce.com/get-juce/
- https://github.com/juce-framework/JUCE
