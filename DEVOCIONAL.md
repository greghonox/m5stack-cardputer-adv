# Cardputer-Adv — Devocional bíblico

Leitor offline para M5Stack Cardputer-Adv: mostra a Bíblia completa, um versículo por vez, e sorteia um capítulo longo ao iniciar ou ao pressionar R.

## Requisitos

- Cardputer-Adv.
- MicroSD formatado em FAT32.
- Três arquivos de dados na raiz do cartão: `BIBLIA.TXT`, `CAPITULOS.BIN` e `VERSICULOS.BIN`.

O dispositivo lê os arquivos diretamente do microSD; a Bíblia não é embutida no firmware.

## Preparar os arquivos do microSD

Na raiz do projeto, execute:

```sh
curl -fL https://raw.githubusercontent.com/BibliaJFAAL/JFAAL/main/atualizada/1911-JFAAtualizadaLivre.json -o data/1911-JFAAtualizadaLivre.json
python tools/prepare_bible.py data/1911-JFAAtualizadaLivre.json data/sdcard
```

Copie os três arquivos gerados em `data/sdcard/` para a raiz do cartão FAT32:

- `BIBLIA.TXT`: texto UTF-8, aproximadamente 3,96 MB.
- `CAPITULOS.BIN`: índice com 1.189 capítulos.
- `VERSICULOS.BIN`: índice com 31.102 versículos.

O conjunto ocupa aproximadamente 4,2 MB. O script valida os 66 livros e produz offsets compactos para leitura de um versículo por vez.

## Controles

- Ao iniciar, abre um capítulo aleatório com pelo menos 40 versículos.
- **R**: sorteia outro capítulo longo (159 capítulos nesta edição têm 40 ou mais versículos).
- **→** ou **↓**: próximo versículo; no último, avança para o capítulo seguinte.
- **←** ou **↑**: versículo anterior; no primeiro, volta ao capítulo anterior.
- Se um versículo exceder a área visível, o texto termina com `...`; avance para continuar.

- Os títulos dos livros aparecem sem diacríticos para melhor suporte à fonte compacta do display. O texto dos versículos é convertido para ASCII sem acentos no momento da exibição, para evitar glifos ausentes nessa fonte; `æ`, `œ`, `ſ` também são simplificados.

## Compilar

Use o ambiente Docker do projeto:

```sh
./build-docker.sh cardputer-adv-devotional
```

Binário gerado em `.pio/build/cardputer-adv-devotional/firmware.bin`. O gravador de voz continua separado: `./build-docker.sh cardputer-adv`.

## Instalar

1. Copie o `.bin` para o microSD conforme o procedimento do M5Launcher.
2. Mantenha os três arquivos bíblicos na raiz do mesmo cartão.
3. Instale pelo launcher e reinicie.

O build gera o binário, não grava o aparelho. O app tem limite de partição de cerca de 3,34 MB, menor que o texto bíblico; deixe a Bíblia no microSD.

## Texto e atribuição

Esta variante usa `1911-JFAAtualizadaLivre.json`, mantido pelo projeto JFAAL e baseado em Almeida 1911. O README da JFAAL informa que a edição atualizada foi revisada com auxílio de IA; não é uma transcrição diplomática sem revisão da edição histórica. O projeto solicita atribuição sob CC BY 3.0 Brasil. Consulte `data/ATTRIBUTION.md` e preserve o crédito ao compartilhar os dados. O texto e o firmware têm proveniências/licenças separadas.
