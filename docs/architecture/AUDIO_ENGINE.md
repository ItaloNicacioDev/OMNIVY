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

## Roteamento (P3.2 — Implementado)

O sistema de roteamento é baseado em um grafo tipado com seis nós de osciladores (0..5, mapeando OSC 1..6) e nós reservados para mixer/buses (6+).

### Conexões tipadas
Cada conexão possui tipo próprio com semântica de processamento distinta:

| Tipo | Semântica |
|------|-----------|
| Audio | Soma a saída da fonte na entrada de áudio externa do destino |
| FrequencyModulation | Adiciona à entrada de modulação de frequência do destino |
| PhaseModulation | Adiciona à entrada de modulação de fase do destino |
| RingModulation | Multiplica a saída do destino pela saída da fonte |
| HardSync | Reseta a fase do destino no zero-crossing da fonte |

### Segurança em tempo real
- Modificações de grafo ocorrem apenas na thread da mensagem
- Validação de ciclos via DFS executa fora do callback de áudio
- A thread de áudio lê um snapshot imutável (`shared_ptr<const RoutingConfig>`) publicado atomicamente — zero alocações, zero mutexes, zero buscas em grafo dentro de `processBlock`

### Prevenção de ciclos
Conexões que criariam ciclos no grafo de áudio são rejeitadas. Auto-loops (fonte == destino) também são rejeitados. Feedback explícito é tratado como recurso controlado, nunca como consequência acidental de um roteamento cíclico.

## FM / PM Engine (P3.3 — Implementado)

O FMOscillator implementa síntese por modulação de frequência (FM) e modulação de fase (PM) em taxa de áudio:

### FM (Frequency Modulation)
A frequência instantânea do portador é modulada pelo sinal de entrada:
```
phase_increment = base_freq * ratio * fine_ratio / sample_rate
phase_increment += freqModInput * fmAmount
phase += phase_increment
```

### PM (Phase Modulation)
A fase do portador é modulada diretamente:
```
phase += phaseModInput * pmAmount
```

### Recursos do FMOscillator
- **Frequency ratio**: razão portador:modulador
- **Fine ratio**: ajuste fino da razão
- **FM amount**: índice de modulação (0..100)
- **PM amount**: quantidade de modulação de fase (0..10)
- **Feedback**: auto-feedback (-1..1) com clamp para evitar runaway
- **Output level**: ganho de saída (0..2)
- **Hard sync**: reset de fase no zero-crossing da fonte
- **Ring mod**: multiplicação da saída pelo sinal de entrada
- **External audio input**: soma de áudio externo na saída

### Processamento por voz
Cada voz possui seus próprios 6 `OscillatorEngine` instances com estado independente (phase, feedback, mod inputs). O roteamento é compartilhado (snapshot imutável) mas o estado de sinal é por-voz, garantindo que notas simultâneas nunca interfirem umas nas outras.

### Segurança
- Feedback é limitado a [-1, 1] para evitar runaway
- Saída é limitada a [-2, 2] via outputLevel clamp
- Todas as operações são real-time safe (zero alocações, zero mutexes)
- Output é verificado para finitude (não NaN/Inf)

## Status da Fase 1

- [x] PluginProcessor implementado (estrutura base)
- [x] processBlock funcional (silence com test parameter)
- [x] Estado salvável/restaurável (testParameter)
- [ ] Sistema de vozes completo (Fase 2)
- [ ] Oversampling (Fase 2/3)
