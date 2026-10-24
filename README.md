# Cardputer-Adv — Gravador de voz WAV

Firmware Arduino para o **M5Stack Cardputer-Adv**. Grava áudio mono PCM de 16 bits / 16 kHz do microfone interno em arquivos WAV no cartão microSD e reproduz a gravação mais recente pelo alto-falante.

## Controles

- **`R`**: iniciar gravação; pressione `R` novamente para parar e salvar.
- **`P`**: reproduzir a última gravação desta sessão; pressione `P` durante a reprodução para interromper.
- Cada arquivo ganha um nome novo (`/RECORDINGS/VOZ00001.WAV`, etc.), sem sobrescrever gravações anteriores.
- A duração máxima é de **5 minutos por gravação**.

Use um cartão microSD FAT32 inserido no dispositivo. Na primeira inicialização, o firmware cria a pasta `/RECORDINGS`. Se o cartão não for detectado, confirme o formato e reinicie o Cardputer com o cartão inserido.

## Compilar sem instalar PlatformIO no sistema

O script constrói uma imagem local `m5stack-cardputer-platformio:local` a partir do `Dockerfile`, baixa PlatformIO para dentro dessa imagem, e então executa `pio run -e cardputer-adv` em um container descartável. A pasta do projeto é montada em `/workspace`; o `.bin` e o cache ficam em `.pio/` dentro do projeto. O Docker precisa estar instalado, iniciado e acessível pelo usuário. O script não grava firmware no dispositivo.

### Acesso ao daemon Docker

Se o Docker estiver ativo, mas o usuário não tiver permissão para acessar o socket, o build pode ser executado explicitamente com `sudo`:

```sh
sudo docker run --rm \
  --user "$(id -u):$(id -g)" \
  --env HOME=/tmp \
  --env PLATFORMIO_CORE_DIR=/workspace/.pio-home \
  --volume "$PWD:/workspace" \
  --workdir /workspace \
  m5stack-cardputer-platformio:local run -e cardputer-adv
```

Para usar o script sem `sudo` em execuções futuras, peça ao administrador para adicionar seu usuário ao grupo `docker` e reabra a sessão. **A associação ao grupo `docker` concede privilégios equivalentes a root**; faça isso apenas se aceitar essa implicação de segurança. O script padrão não usa `sudo` automaticamente e não altera permissões do daemon.

### Linux / Omarchy

Na pasta do projeto, execute:

```sh
./build-docker.sh
```

O script verifica o acesso ao daemon Docker, constrói a imagem local com PlatformIO, compila o ambiente `cardputer-adv` e confirma que o binário foi criado. Na primeira execução, o Docker baixa a imagem base, e o PlatformIO baixa a plataforma ESP32 e as bibliotecas — é necessário acesso à internet.

### Local do firmware

Após compilação bem-sucedida:

```text
.pio/build/cardputer-adv/firmware.bin
```

O `.bin` ainda não foi testado no aparelho. Para carregá-lo, use o procedimento indicado pelo seu M5Launcher; compilar não grava o Cardputer. A instalação pode substituir o firmware ativo.

## Arquivos do projeto

- `src/main.cpp` — interface, gravação WAV no microSD e reprodução.
- `platformio.ini` — ambiente Arduino/ESP32-S3 e biblioteca M5Cardputer.
- `Dockerfile` e `build-docker.sh` — build isolado em Docker.
