# Cardputer-Adv — “Jesus voltará!”

Firmware Arduino para o **M5Stack Cardputer-Adv**. A tela exibe “Jesus voltara!” e **Fn + `.`** (tecla marcada com seta para baixo) alterna entre seis cores de fundo.

## Compilar sem instalar PlatformIO no sistema

O script constrói uma imagem local `m5stack-cardputer-platformio:local` a partir do `Dockerfile`, baixa PlatformIO para dentro dessa imagem, e então executa `pio run -e cardputer-adv` em um container descartável. A pasta do projeto é montada em `/workspace`; o `.bin` e o cache ficam em `.pio/` dentro do projeto. O Docker precisa estar instalado, iniciado e acessível pelo usuário. O script não grava firmware no dispositivo.

### Acesso ao daemon Docker

Se o Docker estiver ativo, mas o usuário não tiver permissão para acessar o socket, o build pode ser executado explicitamente com `sudo`:

```sh
sudo docker run --rm \\
  --user "$(id -u):$(id -g)" \\
  --env HOME=/tmp \\
  --env PLATFORMIO_CORE_DIR=/workspace/.pio-home \\
  --volume "$PWD:/workspace" \\
  --workdir /workspace \\
  m5stack-cardputer-platformio:local run -e cardputer-adv
```

Para usar o script sem `sudo` em execuções futuras, peça ao administrador para adicionar seu usuário ao grupo `docker` e reabra a sessão. **A associação ao grupo `docker` concede privilégios equivalentes a root**; faça isso apenas se aceitar essa implicação de segurança. O script padrão não usa `sudo` automaticamente e não altera permissões do daemon.


### Linux / Omarchy

Abra um terminal na pasta do projeto e execute:

```sh
./build-docker.sh
```

O script verifica o acesso ao daemon Docker, constrói a imagem local com PlatformIO, compila o ambiente `cardputer-adv` e confirma que o binário foi criado. Na primeira execução, o Docker baixa a imagem base, e o PlatformIO baixa a plataforma ESP32 e as bibliotecas — é necessário acesso à internet.

Se o script informar que não consegue acessar o daemon Docker, use a seção **Acesso ao daemon Docker** abaixo para executar o build explicitamente com `sudo` ou consulte o administrador sobre associação ao grupo `docker` (que concede privilégios equivalentes a root). O script, por padrão, não usa `sudo` nem altera permissões do sistema.

### Local do firmware

Após compilação bem-sucedida:

```text
.pio/build/cardputer-adv/firmware.bin
```

Para localizar e copiar para o microSD, por exemplo:

```sh
cp .pio/build/cardputer-adv/firmware.bin /caminho/do/cartao/
```

Troque `/caminho/do/cartao/` pelo ponto de montagem real do cartão. Não execute o exemplo literalmente sem ajustar o destino.

## Carregar pelo M5Launcher

1. Copie `firmware.bin` para o microSD.
2. Insira o cartão no Cardputer-Adv e abra o M5Launcher.
3. Selecione o arquivo e confirme a ação indicada pelo Launcher.

A compilação **não** grava o dispositivo. O firmware `.bin` não foi validado no hardware; antes de instalar, confira a opção e os avisos exibidos pela versão do seu M5Launcher, pois a instalação altera o firmware ativo na flash.

## Projeto

- `src/main.cpp` — tela e leitura da combinação Fn + seta para baixo.
- `platformio.ini` — ambiente Arduino/ESP32-S3 e dependência oficial M5Cardputer.
- `build-docker.sh` — compilação isolada em Docker, com a pasta do projeto montada.

A configuração do ambiente PlatformIO segue as opções publicadas pela M5Stack para o Cardputer-Adv.
