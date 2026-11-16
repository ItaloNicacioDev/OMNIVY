# OMNIVY — Arquitetura do Audio Engine

## Responsabilidade

O Audio Core gerencia:
- Inicialização e encerramento do processamento de áudio
- Gerenciamento de vozes (polifonia, voice stealing)
- Alocação e liberação de recursos
- Controle de parâmetros em tempo real
- Comunicação entre motores de síntese
- Sincronização com o host VST3

## Classes Principais

### OmnivyAudioProcessor

Herda de `juce::AudioProcessor`. Responsável por:
- `prepareToPlay()`: inicialização com sample rate e buffer size
- `processBlock()`: callback principal de processamento de áudio
- `releaseResources()`: limpeza de recursos
- `getStateInformation()` / `setStateInformation()`: serialização de estado
- Exposição de parâmetros para automação do host

## Gerenciamento de Vozes

- Polifonia configurável
- Voice stealing configurável (oldest, none, etc.)
- Alocação eficiente de vozes para notas MIDI
- Liberação de vozes no release de envelopes

## Regras Críticas de Áudio (Thread de Áudio)

NUNCA executar no callback de áudio (`processBlock`):

- [x] Alocações dinâmicas desnecessárias
- [x] Operações de arquivo (disk I/O)
- [x] Acesso à rede
- [x] Bloqueios de mutex
- [x] Operações pesadas de interface
- [x] Operações que possam interromper o processamento

## Sample Rates Suportados (planejado)

44.1 kHz, 48 kHz, 88.2 kHz, 96 kHz, 176.4 kHz, 192 kHz

## Oversampling

Planejado para motores que requerem redução de aliasing (wavetable, analog, etc.)

## Status da Fase 1

- [x] PluginProcessor implementado (estrutura base)
- [x] processBlock funcional (silence com test parameter)
- [x] Estado salvável/restaurável (testParameter)
- [ ] Sistema de vozes completo (Fase 2)
- [ ] Oversampling (Fase 2/3)
