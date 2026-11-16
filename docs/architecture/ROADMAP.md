# OMNIVY — Roadmap do Produto

## Fase 1 — Fundação do Plugin (CONCLUÍDA)

- [x] Estrutura de diretórios criada
- [x] Ambiente de desenvolvimento configurado (MSVC, CMake, Ninja, JUCE)
- [x] CMake configurado (VST3 + Standalone x64)
- [x] PluginProcessor implementado (estrutura base)
- [x] PluginEditor implementado (interface inicial com identidade OMNIVY)
- [x] Standalone application compilável
- [x] Parâmetro de teste exposto
- [ ] Estado salvável/restaurável (testParameter implementado)
- [x] Build VST3 x64 funcional
- [x] Build Standalone x64 funcional
- [x] Documentação técnica criada

## Fase 2 — Motor de Áudio (CONCLUÍDA)

- [x] Sistema de vozes com polifonia
- [x] Seis osciladores independentes
- [x] Motor Virtual Analog (saw, square, pulse, triangle, sine, noise)
- [x] Motor Wavetable básico
- [x] Mixer de seis osciladores
- [x] Filtros iniciais (Low Pass, High Pass)
- [x] Roteamento básico

## Fase 3 — Engines Avançados (Planejado/Em Andamento)

- [x] Arquitetura de engines universais (P3.1)
- [x] Grafo de roteamento tipado com prevenção de ciclos (P3.2)
- [ ] FM / PM Engine (P3.3)
- [ ] Granular Engine (P3.4)
- [ ] Additive Engine (P3.5)
- [ ] Sistema de modulação (LFOs, envelopes, matriz) (P3.6)
- [ ] Mixer e integração completa (P3.7)
- [ ] Testes DSP automatizados

## Fase 4 — Modulação (Planejado)

- [ ] Envelopes (ADSR com curvatura, loop, retrigger)
- [ ] LFOs (formas predefinidas, sync BPM)
- [ ] Matriz de modulação visual
- [ ] Macros
- [ ] MIDI, MPE, Aftertouch
- [ ] Automação de parâmetros

## Fase 5 — Visualização (Planejado)

- [ ] Waveform 2D e Oscilloscope
- [ ] Spectrum Analyzer
- [ ] Wavetable 3D
- [ ] Visualização de modulação

## Fase 6 — Composição Musical (Planejado)

- [ ] Teclado virtual interativo
- [ ] Arpejador
- [ ] Piano Roll
- [ ] Compositor de notas e acordes
- [ ] Escalas musicais
- [ ] Exportação MIDI

## Fase 7 — Efeitos (Planejado)

- [ ] FX Rack (Reverb, Delay, Chorus, Distortion, EQ, Compressor, Limiter, etc.)
- [ ] Cadeias seriais e paralelas
- [ ] Buses

## Fase 8 — Presets e Compatibilidade (Planejado)

- [ ] Formato nativo .omnivy
- [ ] Importador Vital (.vital)
- [ ] Importador Serum (.SerumPreset)
- [ ] Importador Pigments (.pgtx)
- [ ] Relatórios de compatibilidade
- [ ] Biblioteca de presets

## Fase 9 — Interface Premium (Planejado)

- [ ] Redesign visual completo
- [ ] Temas (Obsidian, Aurora)
- [ ] Animações suaves
- [ ] Personalização de layout
- [ ] Atalhos de teclado

## Fase 10 — Testes e Distribuição (Planejado)

- [ ] Testes de áudio (aliasing, clipping, ruído, fase)
- [ ] Testes em DAWs (FL Studio, REAPER)
- [ ] Testes de estabilidade
- [ ] Otimização de CPU e polifonia
- [ ] Build Release
- [ ] Instalador
- [ ] Documentação final

## Critérios de Aceitação (Versão Inicial Funcional)

1. Compilar como VST3 x64
2. Funcionar em uma DAW real
3. Possuir seis osciladores independentes
4. Permitir combinar diferentes engines
5. Possuir mixer e roteamento funcionais
6. Possuir filtros operacionais
7. Possuir matriz de modulação funcional
8. Possuir envelopes e LFOs
9. Possuir visualização de waveform
10. Possuir teclado virtual clicável
11. Possuir arpejador funcional
12. Possuir sequenciador de notas funcional
13. Salvar e restaurar presets nativos
14. Expor parâmetros para automação
15. Não apresentar falhas graves de áudio
