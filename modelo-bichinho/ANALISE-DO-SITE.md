# Análise completa: lilguy.net (Starboy)

Raio-x do site **lilguy.net** (também servido em hesjustalittleguy.com), feito em 14/11/2026, e guia para montar um modelo igual. O template pronto está em `index.html`, nesta mesma pasta.

---

## 1. O que é o site

- **Produto:** *Starboy*, um "bichinho digital vestível" em forma de estrela, com tela OLED redonda onde aparecem olhos animados. É vendido como chaveiro/pingente.
- **Marca:** "lilguy" / "CREATURE" (creature.company). Criador: Dan Kuntz (@dankuntz).
- **Objetivo da página:** página única de produto (*one-page*) para **pré-venda** em lotes.
- **Tom:** minimalista, tudo em minúsculas, frases curtas e fofas ("he's just a little guy.").

## 2. Estrutura da página (de cima para baixo)

| # | Seção | O que tem |
|---|-------|-----------|
| 1 | **Hero** | Título gigante "when / you / wish / upon a / starboy." alinhado à direita, com 2 renders 3D grandes do produto (transparente e inox) cortados pelas bordas. |
| 2 | **"he's just a little guy."** | Segundo título gigante, à esquerda, com mais 2 renders (alumínio e latão). As letras aparecem quando a seção entra na tela. |
| 3 | **Texto** | 5 parágrafos curtos, coluna estreita (600px) centralizada, muito espaço em branco (200px em cima, 300px embaixo). |
| 4 | **Widget de olhos** | Canvas com os olhos animados do bichinho (o mesmo motor da tela do produto, compilado em WebAssembly). |
| 5 | **Loja** | À esquerda, título "eye looks" com barra "view all ↗ 400". À direita, grade de 13 cartões de produto. |
| 6 | **Detalhe** | Ao clicar num cartão, uma tela cheia desliza por cima: título, lista de especificações, lotes riscados, opção "add brass halo" e botão "pre-order  $249". Itens "coming soon" mostram campo de e-mail. |
| 7 | **Rodapé** | Logo "lilguy" e links: Instagram, TikTok, Twitter, contact, press kit. |

### Textos originais (para referência de tom)

> starboy is an adorable, wearable digital pet that behaves like a real living creature.
>
> STARBOY perceives its environment through a camera, microphone, temperature sensor, and accelerometer. bring him into a loud environment and he gets anxious. Leave him in the cold and he shivers. Shake him and he gets dizzy and mad at you. give him a thumbs up to check the battery level, or flick him off if you feel like making him sad.
>
> Each device ships with a distinct personality and a unique set of eyes. there are over 400 looks, each with associated rarity. STARBOYs communicate when they come into contact in the wild. They swap eyes, spread behaviors, and play with each other.
>
> STARBOY is a completely standalone device. He is too dumb to know how to connect to your phone or the internet. The hardware is built to the standard of a Swiss watch. It feels like a high-end piece of jewelry.
>
> Our first few batches are available for pre-order now. These colors and materials are made to order in small batches.

## 3. Catálogo (dados que estavam na página)

| Produto | Preço | Material | Estoque restante |
|---|---|---|---|
| Sunshine | $139 | alumínio fundido pintado | 622 |
| Snow | $139 | alumínio fundido pintado | 772 |
| Lilac | $139 | alumínio fundido pintado | 525 |
| Magenta | $139 | alumínio fundido pintado | 0 |
| Azure | $139 | alumínio fundido pintado | 321 |
| Aluminum | $249 | alumínio anodizado (+ egg) | 582 |
| Diamond Finish Aluminium | $599 | alumínio diamantado, edição de 100 (+ egg) | 60 |
| Crystal | $299 | acrílico óptico (+ egg) | 0 |
| Stainless Steel | $349 | aço inox 316L (+ egg) | 4 |
| Brass | $399 | latão (+ egg) | 0 |
| Starboy Egg | TBD | estojo de alumínio + silicone | em breve |
| Stand | TBD | base | em breve |
| CC Microbiner | TBD | mosquetão de alumínio, gira 360° | em breve |

**Especificações comuns:** auréola de aço inox 316L · tela OLED 400×400 · câmera · microfone · acelerômetro · vibração (háptico) · bateria de 24h+ · USB-C (cabo incluso) · cor de olhos surpresa.

**Lotes:** "batch 1 – sold out" e "batch 2 – sold out" aparecem riscados com 50% de opacidade; "batch 3 – ships december 2026" em negrito. Isso cria urgência.

**Adicional:** "add brass halo" (auréola de latão), checkbox que soma ao preço.

**Moeda:** o site detecta o país e mostra o preço local (US$, £, €, ¥, R$...).

## 4. Identidade visual

| Item | Valor |
|---|---|
| Fundo | `#f9f9f9` (quase branco) |
| Texto / botões | `#2a2b2c` (grafite) |
| Secundário | `#9b9b9b`, `#999` |
| Bordas da grade | `#eaeaea`, 1px |
| Fundo do widget | `#ececec` |
| Fonte | **205TF Milling Triplex** (paga; espessuras 1mm e 1,5mm). Geométrica, de traço único, cantos arredondados. |
| Títulos | `font-size: 13cqw`, `line-height: 0.83`, `letter-spacing: -0.08em`, tudo minúsculo |
| Texto | `letter-spacing: -0.04em`, ~15px |
| Curva de animação | `cubic-bezier(0.38, 0.05, 0, 1)` (arranca rápido e assenta devagar) |
| Breakpoints | 1200px (grade de 2 colunas), 768px, 600px (1 coluna) |

**Fontes gratuitas parecidas:** Figtree (usada no modelo), Outfit, Manrope, Urbanist.

## 5. Interações e efeitos (o que dá a sensação "premium")

1. **Letras que sobem uma a uma.** O JS divide o título em `<span class="char">`, cada uma com `animation-delay: 200ms + 20ms × índice`. No segundo título, um `IntersectionObserver` dispara a animação quando ele aparece na tela.
2. **"Pulinho" no hover.** Ao passar o mouse sobre uma letra, ela sobe por 100ms.
3. **Parallax.** Os 4 renders do topo andam a −0,2× a velocidade da rolagem (`translate: 0 scrollY*-0.2px`).
4. **Giro 360° nos cartões.** Cada produto tem **24 quadros WebP** de um giro completo (15° por quadro). No hover dá uma volta a 10ms por quadro e para no quadro 19. Os 24 quadros vêm num único arquivo `.bin` por produto, para carregar rápido. No celular, o cartão no centro da tela é que gira.
5. **Cursor personalizado.** Círculo de 40px, cinza translúcido com `backdrop-filter: blur(6px)`. Sobre botões ele encolhe e "abraça" o elemento. Some em telas de toque.
6. **Barra de rolagem própria.** A nativa é escondida; uma barra fina aparece ao rolar e dá para arrastá-la.
7. **Cartões.** No hover, o rodapé do cartão fica grafite com texto branco e o nome troca por "pre-order" / "sold out".
8. **Detalhe deslizante.** Ocupa a tela toda, com o produto girando à direita. A URL ganha `#produto`, então o link é compartilhável.
9. **Efeito glitch** (`glitch.js`). Caracteres como ░▒▓⚇★ piscam no texto, usado em estados como "sold out".
10. **Olhos em WebAssembly.** `animation_renderer.wasm` + `anim_data.bin`: o mesmo código da tela do produto rodando no navegador. Formatos de olho: dot, circle, cat, acorn. Mais de 100 cores com nomes (marmalade, matcha, whale, bubblegum...). Piscam a cada 3–6s e olham ao redor a cada 5–10s.

## 6. Tecnologia

- HTML, CSS e JavaScript puros (sem React nem framework), tudo em uma página.
- Hospedagem atrás da **Cloudflare** (proteção de e-mail, cache).
- **WebAssembly** (Emscripten) para os olhos.
- Imagens em **WebP**, com pré-carregamento (`<link rel="preload">`) dos renders do topo.
- Checkout próprio em `/checkout`, que recebe a origem do visitante (UTM, referência, `fbclid`).
- **Meta Pixel** para anúncios: o `fbclid` fica guardado por 90 dias.
- Open Graph com **vídeo** (`og:video` preview.mp4) e 4 imagens, para o link ficar bonito ao ser compartilhado.

## 7. Como montar o seu (passo a passo)

1. **Produto e fotos (o mais importante).** O site depende de renders 3D perfeitos.
   - Modele no Blender (ou peça a um designer 3D).
   - Gere 4 renders grandes, com fundo transparente, para o topo.
   - Para cada variante, faça um giro (*turntable*) de **24 quadros** (15° cada), 1024×1024, exportados em WebP.
2. **Abra o template** `index.html` e edite:
   - `:root` no CSS: cores e fontes.
   - `PRODUCTS` no JS: nomes, preços, estoque, estado e descrição.
   - `ADDON` e `CURRENCY`: adicional e moeda.
   - Os textos das seções.
3. **Troque os desenhos pelos seus renders.** Hoje a função `drawProduct()` desenha uma estrela de exemplo no canvas. Substitua por `<img>`: no hover, troque o `src` entre os 24 quadros (a lógica de quadros já está pronta: `FRAMES`, `REST`, `MS_PER_FRAME`).
4. **Checkout:** preencha `checkout` de cada produto com um link de pagamento (Stripe Payment Links, Mercado Pago, Shopify Buy Button, Hotmart...).
5. **Lista de e-mail:** ligue o formulário "em breve" a Mailchimp, Brevo, ConvertKit ou um Google Forms.
6. **Hospedagem grátis:** Netlify, Vercel, Cloudflare Pages ou GitHub Pages (basta subir a pasta).
7. **Opcional:** Meta Pixel / Google Analytics, imagem e vídeo de Open Graph, favicon.

## 8. Cuidados legais

Copie a **estrutura e as mecânicas** (são padrões de design comuns). **Não** use as imagens, a marca "Starboy"/"lilguy", os textos ou o formato de personagem deles. A fonte Milling Triplex é paga: compre a licença na 205TF ou use uma gratuita.
