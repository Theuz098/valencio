# Guia: montando o seu "astrinho" com ESP32 (versão barata)

Um bichinho digital com tela redonda e olhos animados, que reage a movimento, barulho e frio, no estilo do Starboy, gastando pouco.

> **Preços:** estimativas de AliExpress/Shopee/Mercado Livre em reais, por unidade, no fim de 2026. Variam bastante; confira antes de comprar. Compras internacionais pagam imposto (Remessa Conforme), então conte com uns **+40 a 50%** sobre o preço em dólar.

---

## 1. O que o Starboy tem e o que dá para fazer barato

| Starboy (original) | Versão barata | Observação |
|---|---|---|
| Tela OLED 400×400 redonda | **Tela LCD redonda GC9A01 1,28" 240×240** | Mesma ideia, resolução menor. É a peça que dá a "cara". |
| Câmera (reconhece joinha) | ❌ na v1 | Reconhecer gestos é difícil. Fica para a v2 (seção 9). |
| Microfone | **INMP441** (I2S) | Detecta barulho alto → fica ansioso. |
| Sensor de temperatura | Sensor embutido no **MPU6050** | Mede alguns graus acima do ambiente; serve para "está frio". |
| Acelerômetro | **MPU6050** | Sacudir, toque, inclinar, dormir quando parado. |
| Vibração (háptico) | **Motor de vibração de celular** | |
| Bateria 24h+, USB-C | **LiPo 3,7V + carregador TP4056 USB-C** | Duração mais curta (seção 6). |
| Troca de olhos entre aparelhos | **ESP-NOW** (rádio do próprio ESP32) | Funciona sem internet e sem celular. |
| Corpo usinado em metal | **Impressão 3D** (PLA ou resina) | Metal é caro; resina + pintura fica bem bonito. |

## 2. Lista de compras (Opção A: mais barata, com solda)

| Peça | Para quê | Preço aprox. |
|---|---|---|
| ESP32-C3 SuperMini | cérebro, com USB-C e rádio (Wi-Fi/Bluetooth/ESP-NOW) | R$ 12–25 |
| Tela GC9A01 1,28" redonda (240×240, SPI) | os olhos | R$ 20–35 |
| MPU6050 (módulo GY-521) | movimento + temperatura | R$ 8–15 |
| INMP441 | microfone | R$ 10–18 |
| Motor de vibração 1027 (moeda, 3V) | vibrar | R$ 3–6 |
| Transistor S8050/2N2222 + resistor 1kΩ + diodo 1N4148 | ligar o motor sem queimar o ESP32 | R$ 2 |
| TP4056 USB-C (com proteção, 6 pinos) | carregar a bateria | R$ 3–6 |
| Bateria LiPo 3,7V 500–800mAh (ex.: 503035, 802040) | energia | R$ 15–30 |
| Regulador 3,3V de baixa queda (HT7333 ou ME6211) | estabiliza a tensão da bateria | R$ 2–4 |
| Chave liga/desliga mini + 2 resistores de 100kΩ | ligar/desligar; medir a bateria | R$ 3 |
| Fios finos (30 AWG) e estanho | ligações | R$ 10 |
| **Total eletrônica** | | **≈ R$ 90–150** |
| Carcaça impressa em 3D (serviço) | corpo de estrela | R$ 25–80 |
| Argola/mosquetão de chaveiro | | R$ 3 |
| **Total por unidade** | | **≈ R$ 120–230** |

Comprando para 10 unidades, o preço por peça cai uns 20–30%.

**Ferramentas (se ainda não tiver):** ferro de solda (R$ 60–150), multímetro (R$ 40) e um cabo USB-C de dados.

### Opção B: mais fácil, quase sem solda

**Waveshare ESP32-S3-Touch-LCD-1.28** (ou similar), ≈ R$ 110–170. Já vem com:
- tela redonda 1,28" com toque;
- acelerômetro QMI8658;
- carregador de bateria e conector;
- USB-C.

Falta só o microfone (INMP441) e o motor. É bem mais simples de montar, mas o firmware precisa de ajustes: pinos diferentes e outro sensor de movimento.

**Recomendação:** monte o primeiro protótipo numa protoboard com a Opção A. É barato, você aprende como tudo funciona e só depois solda.

## 3. Ligações (Opção A)

```
                ESP32-C3 SuperMini
              ┌──────────────────────┐
  Tela GC9A01 │ GPIO4  ── SCL        │
              │ GPIO6  ── SDA        │
              │ GPIO7  ── CS         │
              │ GPIO5  ── DC         │
              │ GPIO10 ── RST        │
              │ GPIO2  ── BLK (brilho)
              │ 3V3    ── VCC        │
   MPU6050    │ GPIO8  ── SDA        │
              │ GPIO9  ── SCL        │
   INMP441    │ GPIO0  ── SCK        │
              │ GPIO1  ── WS         │
              │ GPIO21 ── SD         │  (L/R do INMP441 no GND)
   Motor      │ GPIO20 ── 1kΩ ── base do transistor
   Bateria    │ GPIO3  ── meio do divisor 100k/100k
              └──────────────────────┘
  Todos os módulos: VCC → 3V3, GND → GND
```

**Motor de vibração:**
```
3V3 ──┬── motor (+)
      │   motor (−) ── coletor do S8050
      └── diodo 1N4148 (listra no 3V3), em paralelo com o motor
GPIO20 ── 1kΩ ── base       emissor ── GND
```

**Energia:**
```
USB-C ─► TP4056 ─► B+/B− : bateria LiPo
                 OUT+ ─► chave ─► regulador 3,3V (HT7333) ─► 3V3 do ESP32 (e módulos)
                 OUT− ─► GND
BAT+ ── 100kΩ ──┬── 100kΩ ── GND
                └── GPIO3   (para medir a bateria)
```

> ⚠️ **Não ligue a bateria (até 4,2V) direto no pino 3V3.** Passe pelo regulador. E, se quiser gravar o firmware com a bateria ligada, desligue a chave primeiro.

## 4. Gravando o firmware

1. Instale a **Arduino IDE 2**.
2. Em *Preferências → URLs adicionais*, cole:
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
3. No *Gerenciador de Placas*, instale **esp32 by Espressif** (versão 3.x).
4. No *Gerenciador de Bibliotecas*, instale **LovyanGFX**.
5. Abra `firmware/astrinho/astrinho.ino`.
6. Em *Ferramentas*, escolha a placa **ESP32C3 Dev Module** e ative **USB CDC On Boot: Enabled**.
7. Conecte e clique em *Carregar*. Se não gravar, segure o botão **BOOT**, aperte **RESET** e solte o BOOT.

> O código ainda **não foi compilado nem testado em placa de verdade**: o ambiente onde ele foi escrito não tem acesso ao toolchain do ESP32. A sintaxe foi verificada, mas erros pequenos podem aparecer na primeira compilação. Me mande a mensagem de erro que eu corrijo.

## 5. Calibrando (5 minutos)

Abra o **Monitor Serial** (115200). A cada segundo aparece algo como:

```
g=1.01 temp=27.3 ruido=840 bat=86% humor=0
```

Ajuste no topo do código:
- **`LOUD_LEVEL`:** fale alto ou bata palmas e veja quanto o `ruido` sobe. Use um valor entre o silêncio e o barulho.
- **`COLD_C`:** o MPU6050 mede uns 3–5°C acima do ambiente. Ponha o aparelho na geladeira por 1 minuto para testar.
- **`SHAKE_G` / `TAP_G`:** sensibilidade da sacudida e do toque.
- **Tela com cores invertidas?** Troque `p.invert = true` por `false`.

### O que ele faz

| Ação | Reação |
|---|---|
| Ligou pela 1ª vez | Sorteia **formato e cor de olhos únicos** e guarda para sempre |
| Parado | Pisca a cada 3–6s e olha ao redor |
| Inclinou | Olha para o lado da inclinação |
| Toquinho | Fica feliz (^ ^) e vibra |
| Dois toques | Mostra a bateria em % |
| Sacudiu | Fica tonto (espirais), depois bravo |
| Barulho alto | Fica ansioso: pupilas pequenas tremendo, pisca rápido |
| Frio | Treme e os olhos ficam azulados |
| Parado 2 min | Dorme (tela escurece); mexeu, acorda |
| Outro astrinho a ~1m | Fica feliz e tem 10% de chance de pegar a cor dele |

## 6. Bateria: a verdade

| Consumo | Corrente aprox. |
|---|---|
| ESP32-C3 com rádio ligado | 60–90 mA |
| Luz da tela | 15–25 mA |
| **Total** | **≈ 80–110 mA** |

- Com **500mAh**, dura **≈ 4–6 horas** de uso contínuo.
- Para chegar perto das 24h do Starboy:
  1. Bateria maior (800–1000mAh, se couber).
  2. Ligar o rádio ESP-NOW só a cada X segundos.
  3. Usar *light sleep* quando estiver dormindo.
  4. Escurecer a tela.

Esses são os próximos passos de otimização.

## 7. Carcaça

- **Modelagem grátis:** Tinkercad (mais fácil) ou Fusion 360 / Onshape.
- **Medidas:**
  - Tela GC9A01: placa redonda de ≈ 39mm, área visível ≈ 32mm.
  - Faça a estrela com ≈ 60–70mm de ponta a ponta e ≈ 14–16mm de espessura.
  - Cabem dentro: bateria 503035 + ESP32-C3 SuperMini (22×18mm) + sensores.
- **Vidro:** um cristal de relógio abaulado de 32–35mm (R$ 10–20) colado sobre a tela dá o efeito de "lente" do original.
- **Impressão:** resina (melhor acabamento) ou PLA. Lixe, use primer e pinte com tinta spray metálica ou cores pastel.
- **Detalhes:**
  - furo para o USB-C;
  - furinho para o microfone;
  - argola no topo (a "auréola").

## 8. Ordem de montagem sugerida

1. **Protoboard:** só ESP32 + tela. Grave o firmware e veja os olhos piscando.
2. Adicione o MPU6050 e teste inclinar e sacudir.
3. Adicione o microfone e o motor.
4. Adicione energia (TP4056 + bateria + regulador). Meça com o multímetro antes de ligar no ESP32.
5. Imprima a carcaça, solde tudo com fios curtos e feche.

## 9. Próximas versões (quando quiser evoluir)

- **Câmera e gestos:** trocar o ESP32-C3 por um **Seeed XIAO ESP32S3 Sense** (≈ R$ 90–130), que já tem câmera e microfone. Dá para treinar um modelo de "joinha / dedo do meio" no **Edge Impulse** (grátis) e rodar no próprio chip.
- **Mais visuais:** acrescentar formatos e cores em `PALETTE` e em `drawEye()`, com raridade (sorteio com pesos).
- **Tela melhor:** AMOLED redonda 1,43" (466×466, Waveshare/LilyGO, ≈ R$ 150–220). Cores bem mais vivas, perto do original.
- **Placa própria (PCB):** quando o protótipo estiver bom, desenhe no **EasyEDA** e encomende na **JLCPCB**. 5 placas saem por ≈ US$ 5–20 + frete; com as peças já soldadas, o custo por unidade cai bastante em lote.

## 10. Como o projeto foi feito

1. **Análise do site:** baixei o HTML do lilguy.net e li o CSS e o JavaScript (catálogo, especificações, animações) e tirei capturas de tela. O resultado está em `../ANALISE-DO-SITE.md`.
2. **Página de vendas:** montei o template em `../index.html`.
3. **Hardware:** a lista do Starboy (tela OLED redonda, câmera, microfone, temperatura, acelerômetro, vibração, bateria, USB-C) foi trocada peça por peça pelo equivalente mais barato e fácil de achar no Brasil.
4. **Firmware:** as reações do texto do site (barulho → ansioso, frio → treme, sacudir → tonto e bravo, olhos únicos, troca de olhos entre aparelhos) viraram estados no código.
