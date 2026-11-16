# OMNIVY — Sistema de Modulação (Planejado)

## Visão Geral

O sistema de modulação é um dos pilares do OMNIVY. Permite:
- Arrastar uma fonte de modulação até um parâmetro
- Visualizar visualmente a conexão
- Ajustar intensidade de modulação (positiva e negativa)
- Combinar várias fontes em um mesmo parâmetro
- Utilizar macros como fontes de modulação

## Fontes de Modulação (Planejado)

| Fonte | Descrição |
|-------|-----------|
| Envelopes | ADSR com curvatura, loop, retrigger |
| LFOs | Formas predefinidas, desenho livre, step LFO, sync BPM |
| Arpeggiator | Padrões de notas como fonte de modulação |
| Sequencers | Padrões rítmicos e de controle |
| Macros | Controles globais mapeáveis |
| Velocity | Velocidade da nota MIDI |
| Aftertouch | Pressão após o toque |
| Poly Aftertouch | Pressão polifônica por nota |
| MPE | Expressão MIDI polifônica |
| Mod Wheel | Roda de modulação |
| Expression | Expressão MIDI |
| Key Tracking | Rastreamento de tecla |

## Estrutura da Matriz de Modulação

```
SOURCE → DESTINATION → AMOUNT → CURVE → MODE
```

Cada conexão permite:
- Escolher origem
- Escolher destino
- Ajustar intensidade
- Polaridade (normal/inverso)
- Curva de modulação
- Ativação/desativação
- Visualização da conexão

## Status da Fase 1

- [ ] Matriz de modulação (Fase 4)
- [ ] Envelopes (Fase 4)
- [ ] LFOs (Fase 4)
- [ ] Macros (Fase 4)
