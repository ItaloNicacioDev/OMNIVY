# OMNIVY — Arquitetura Geral do Sistema

## Visão Geral

OMNIVY é um sintetizador profissional multi-engine VST3 x64 para Windows 11,
desenvolvido em C++20 utilizando o framework JUCE 9.0.5.

O sistema segue uma arquitetura modular com separação clara entre:
- **DSP (Processamento de Áudio)**: motores de síntese, filtros, efeitos, mixer
- **UI (Interface Gráfica)**: editor, componentes, painéis, teclado virtual
- **Infraestrutura**: gerenciamento de estado, presets, MIDI, comunicação host-plugin

## Módulos do Sistema

| Módulo | Responsabilidade |
|--------|-----------------|
| `src/Core` | PluginProcessor, PluginEditor, ponto de entrada standalone |
| `src/Audio` | Engine de áudio, gerenciamento de vozes, mixer |
| `src/DSP` | Motores de síntese (wavetable, granular, analog, FM, additive, sample) |
| `src/Modulation` | Envelopes, LFOs, matriz de modulação, macros |
| `src/Effects` | FX rack, efeitos individuais |
| `src/Presets` | Formato nativo .omnivy, importadores (Vital, Serum, Pigments) |
| `src/Sequencer` | Arpejador, sequenciador, piano roll, compositor |
| `src/Visualization` | Waveform 2D/3D, spectrum, modulação visual |
| `src/UI` | Componentes de interface, temas, controles |

## Fluxo de Sinal

```
MIDI Input → Vozes → Motores de Síntese (OSC 1-6) → Mixer → Filtros → FX Rack → Master → Áudio Saída
                              ↳ Modulação (Envelopes, LFOs, Macros) → Parâmetros
```

## Threading

- **Thread de Áudio**: processamento DSP, sem alocações, sem bloqueios
- **Thread da Interface**: renderização UI, interação usuário
- **Comunicação**: estruturas thread-safe (atomics, lock-free fifos) entre threads

## Estado e Presets

- `getStateInformation()` / `setStateInformation()` para salvamento/restauração
- Formato nativo `.omnivy` (planejado Fase 8)
- Parâmetros expostos para automação do host VST3

## Licença

AGPLv3 — veja `LICENSE` e `docs/licensing/DEPENDENCIES.md`.
