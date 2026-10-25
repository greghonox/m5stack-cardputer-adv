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
./build-docker.sh cardputer-adv
```

Para compilar o painel de rede local com o jogo IP-cor, use `./build-docker.sh cardputer-adv-network`; o binário ficará em `.pio/build/cardputer-adv-network/firmware.bin`. Esse build precisa de `include/network_config.h`, arquivo local ignorado pelo Git. O script verifica o acesso ao daemon Docker, constrói a imagem local com PlatformIO, compila o ambiente solicitado e confirma que o binário foi criado. Na primeira execução, são baixadas dependências e é necessário acesso à internet.

### Local do firmware

- Gravador de voz: `.pio/build/cardputer-adv/firmware.bin`.
- Devocional: `.pio/build/cardputer-adv-devotional/firmware.bin`.
- Monitor de rede: `.pio/build/cardputer-adv-network/firmware.bin` (contém credenciais Wi-Fi; não compartilhe).

O `.bin` ainda não foi testado no aparelho. Para carregá-lo, use o procedimento indicado pelo seu M5Launcher; compilar não grava o Cardputer. A instalação pode substituir o firmware ativo.

- Gravador: `./build-docker.sh cardputer-adv`
- Devocional: `./build-docker.sh cardputer-adv-devotional`
- Painel de rede / jogo IP-cor: `./build-docker.sh cardputer-adv-network`

Consulte [`REDE_LOCAL.md`](REDE_LOCAL.md) para configurar o Wi-Fi e usar o painel/jogo. O firmware de rede incorpora as credenciais locais; não compartilhe esse `.bin`. `include/network_config.h` fica ignorado pelo Git.

Consulte [`DEVOCIONAL.md`](DEVOCIONAL.md) para o devocional offline. O texto bíblico fica em arquivos separados para microSD. A edição JFAAL é baseada na Almeida 1911, foi revisada com auxílio de IA e exige a atribuição CC BY 3.0 Brasil descrita em [`data/ATTRIBUTION.md`](data/ATTRIBUTION.md).

Os binários `.bin` não são publicados junto ao código-fonte. Compilar não instala nem testa o firmware no Cardputer; siga o procedimento do M5Launcher e valide no aparelho.
