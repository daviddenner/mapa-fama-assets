# Xadrez sem Rei — prompt de geração de imagem (4:5, perspectiva das brancas)

Peça editorial vertical para a citação:

> JAMAIS JOGUE XADREZ
> COM UM OPONENTE QUE
> NÃO TEM O REI.
> — D. Quinelato

## Ajuste aplicado: perspectiva das brancas

O prompt original descrevia as peças escuras em primeiro plano. Como a imagem deve ser
gerada pela perspectiva das brancas, a câmera fica atrás das peças brancas (primeiro plano)
e o lado das pretas, com a casa do rei vazia, fica do outro lado do tabuleiro, de frente
para quem olha. Para a casa vazia continuar legível, o plano de foco foi movido para a
última fileira das pretas; as brancas em primeiro plano ficam levemente suavizadas pela
profundidade de campo, mas reconhecíveis.

Posição (FEN): `rnbq1bnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1`

Vista das brancas, da esquerda para a direita:

| Fileira      | a | b | c | d | e         | f | g | h |
|--------------|---|---|---|---|-----------|---|---|---|
| 8 (fundo)    | ♜ | ♞ | ♝ | ♛ | **vazia** | ♝ | ♞ | ♜ |
| 7            | ♟ | ♟ | ♟ | ♟ | ♟         | ♟ | ♟ | ♟ |
| 6, 5, 4, 3   | · | · | · | · | ·         | · | · | · |
| 2            | ♙ | ♙ | ♙ | ♙ | ♙         | ♙ | ♙ | ♙ |
| 1 (frente)   | ♖ | ♘ | ♗ | ♕ | ♔         | ♗ | ♘ | ♖ |

Contagem: 16 peças brancas e 15 peças pretas. A casa vazia (e8) é uma casa clara, ao lado
da dama preta (d8, casa escura). As quatro fileiras centrais ficam totalmente vazias.

Referência visual de layout: `mockups/xadrez-sem-rei-layout.png` (render vetorial da
composição, não é a foto final; serve para conferir posição das peças e tipografia, ou
como imagem de referência estrutural em ferramentas que aceitam uma).

## Prompt completo (inglês)

```
Vertical 4:5 editorial photograph in a dimly lit, moody classic library, shot from White's side of the board. The camera sits low behind the white pieces, slightly to their right (kingside), at a three-quarter diagonal angle, looking across the board toward the opponent's side. In the lower half of the frame, an antique weathered wooden chessboard rests on a rustic dark wood table.

Carved vintage wooden Staunton chess pieces stand strictly in the classic starting position. The four middle ranks are completely empty: no moves have been played.

Near side (foreground, closest to the camera): all 16 cream-white wooden pieces in exact standard opening formation. Back rank nearest the camera, left to right: Rook, Knight, Bishop, Queen, King, Bishop, Knight, Rook. Directly in front of them, a neat row of 8 white pawns.

Far side (background, across the board): the dark brown wooden pieces in starting formation. A neat row of 8 dark pawns in front, and on the far back rank ONLY 7 pieces, left to right: Rook, Knight, Bishop, a single Queen, a conspicuously EMPTY light-colored square where the Black King belongs, then Bishop, Knight, Rook. The empty square beside the single dark Queen is clearly visible and unobstructed, highlighting the missing King.

Shallow depth of field with the plane of sharp focus on the far back rank, exactly on the empty square where the Black King should stand; the white pieces nearest the lens are gently softened but remain clearly recognizable.

Soft, warm directional natural light from a window on the right illuminates the carved wood grain and casts gentle shadows across the empty center of the board. The upper half of the frame shows dark, softly blurred vintage bookshelves, providing clean negative space for typography.

Centered in the upper dark space, in an elegant classic serif font (Garamond or Trajan style), warm aged cream-gold color, all uppercase, display the exact text:
"JAMAIS JOGUE XADREZ
COM UM OPONENTE QUE
NÃO TEM O REI."
Directly underneath the last line, right-aligned, in a smaller italic serif font of the same warm cream-gold color, display:
"— D. Quinelato"

Photorealistic, rich atmospheric shadows, subtle 35mm film grain. Exactly 16 white pieces and 15 dark pieces, accurate chess piece count, no duplicate queens, no extra knights, no misplaced pieces, no pieces on the middle ranks, no extra text.
```

## Prompt curto (para ferramentas com limite de caracteres)

```
Vertical 4:5 editorial photo, dim moody classic library, shot from White's side of an antique wooden chessboard on a rustic dark table, low three-quarter angle. Vintage carved wooden Staunton pieces in the exact starting position, four middle ranks empty. Foreground: all 16 cream-white pieces (back rank Rook, Knight, Bishop, Queen, King, Bishop, Knight, Rook; 8 pawns in front). Far side: dark brown pieces, 8 pawns, back rank with ONLY 7 pieces: Rook, Knight, Bishop, single Queen, EMPTY square where the Black King belongs, Bishop, Knight, Rook. Focus on the empty king square; near pieces softly out of focus. Warm window light from the right, blurred dark bookshelves above as negative space. Centered upper text in elegant uppercase serif, aged cream-gold: "JAMAIS JOGUE XADREZ / COM UM OPONENTE QUE / NÃO TEM O REI." and below it, right-aligned italic: "— D. Quinelato". Photorealistic, 35mm film grain, exact piece count, no extra text.
```

## Negative prompt

```
two kings, black king, duplicate queen, extra pieces, missing pawns, pieces on the middle ranks, plastic pieces, modern board, misspelled text, extra text, watermark, signature, logo, blurry text, bright daylight, square or landscape framing, cartoon, illustration, 3D render
```

## Observações para a geração

- Modelos de imagem costumam errar contagem e posição de peças de xadrez. Gere várias
  variações e confira a última fileira das pretas: deve haver exatamente uma casa vazia,
  à direita da dama preta (vista das brancas).
- Texto com acento ("NÃO") é um ponto frágil em alguns modelos; se falhar, gere a foto sem
  texto e aplique a tipografia depois (o mockup mostra a posição e o alinhamento).
- Em ferramentas que aceitam imagem de referência estrutural (ControlNet, image prompt,
  style/structure reference), o mockup pode ser usado para fixar o enquadramento.
