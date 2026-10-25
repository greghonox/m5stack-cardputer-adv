# Painel de rede local e jogo de cores — Cardputer-Adv

Firmware para o M5Stack Cardputer-Adv com conexão Wi-Fi e indicadores da rede local. Não usa Telegram, nuvem nem acessa mensagens pessoais.

## Funcionalidades nesta versão

- Status do Wi-Fi, SSID, IP, gateway, máscara, intensidade do sinal.
- Ping automático ao gateway e atualização manual por Enter.
- `R`: busca redes Wi-Fi próximas.
- **J**: inicia o modo jogo; pressione espaço para procurar hosts na sub-rede local.
- Cada host que responde ao ping aparece com uma cor de fundo aleatória e seu IP.
- A busca pode ser cancelada; o resultado suporta até 24 IPs, em páginas de sete. Setas navegam pelas páginas; qualquer tecla volta ao painel.

O scanner usa ping ICMP, não varre portas e não tenta login; dispositivos que bloqueiam ping não serão detectados. A varredura fica limitada a até 254 endereços da sub-rede local detectada.

## Configuração de Wi-Fi


O arquivo `include/network_config.h` é local e está excluído do Git. Crie-o a partir do exemplo:

```sh
cp include/network_config.example.h include/network_config.h
```

Edite o arquivo no VSCodium e preencha o SSID e a senha da sua rede 2.4 GHz. Não publique esse arquivo nem o binário contendo as credenciais. Se o arquivo permanecer com o valor de exemplo, o firmware vai exibir que precisa configurá-lo.

## Compilar

Na raiz do projeto:

```sh
./build-docker.sh cardputer-adv-network
```

O binário sai em `.pio/build/cardputer-adv-network/firmware.bin`. O script usa Docker e não grava o dispositivo.

## IP do roteador

O firmware testa automaticamente o gateway recebido via DHCP; não precisa configurar seu endereço manualmente.

## Carregar

Copie o `.bin` ao microSD e instale pela opção apropriada do M5Launcher. Confirme a tela do launcher antes de instalar, pois isso troca o firmware ativo. Compilação não equivale a instalação ou teste físico.
