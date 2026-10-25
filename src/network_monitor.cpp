#include <M5Cardputer.h>
#include <WiFi.h>
#include <ESP32Ping.h>
#include <cstring>
#include "network_config.h"

namespace {
constexpr uint32_t REFRESH_INTERVAL_MS = 3000;
constexpr uint32_t PING_INTERVAL_MS = 7000;
constexpr uint32_t WIFI_RETRY_INTERVAL_MS = 10000;
constexpr uint8_t MAX_DISCOVERED_HOSTS = 24;
constexpr uint16_t SCAN_HOST_LIMIT = 254;
constexpr uint16_t BACKGROUND_PALETTE[] = {TFT_BLACK, TFT_NAVY, TFT_DARKGREEN, TFT_MAROON, TFT_PURPLE, TFT_DARKCYAN};
constexpr size_t PALETTE_SIZE = sizeof(BACKGROUND_PALETTE) / sizeof(BACKGROUND_PALETTE[0]);

bool wifiStarted = false;
bool pingInProgress = false;
bool pingReachable = false;
bool pingKnown = false;
bool scanInProgress = false;
bool gameActive = false;
bool scanCancelled = false;
bool colorGameMode = false;
uint16_t currentBackground = TFT_BLACK;
uint8_t foundHostCount = 0;
uint8_t pageNumber = 0;
IPAddress foundHosts[MAX_DISCOVERED_HOSTS];
int pingLatencyMs = -1;
int8_t signalDbm = 0;
uint8_t signalBars = 0;
uint32_t lastWifiRetryAt = 0;
uint32_t lastPingAt = 0;
uint32_t lastScreenUpdateAt = 0;

uint8_t barsFromRssi(int8_t rssi) {
  if (rssi >= -55) return 4;
  if (rssi >= -67) return 3;
  if (rssi >= -75) return 2;
  if (rssi >= -85) return 1;
  return 0;
}

void drawSignal(int x, int y, uint8_t bars) {
  for (int i = 0; i < 4; ++i) {
    const int height = 4 + i * 4;
    M5Cardputer.Display.fillRect(x + i * 6, y + 16 - height, 4, height,
                                  i < bars ? TFT_GREEN : TFT_DARKGREY);
  }
}

void drawScreen() {
  auto &display = M5Cardputer.Display;
  const uint16_t background = colorGameMode ? currentBackground : TFT_BLACK;
  const uint16_t foreground = background == TFT_WHITE ? TFT_BLACK : TFT_WHITE;
  display.fillScreen(background);
  display.setTextDatum(top_left);
  display.setTextSize(1);
  display.setTextColor(TFT_CYAN, background);
  display.drawString("PAINEL DE REDE LOCAL", 8, 5);
  drawSignal(292, 5, signalBars);

  int y = 27;
  display.setTextColor(WiFi.status() == WL_CONNECTED ? TFT_GREEN : TFT_RED, background);
  display.drawString(WiFi.status() == WL_CONNECTED ? "Wi-Fi: CONECTADO" : "Wi-Fi: DESCONECTADO", 8, y);
  y += 16;
  display.setTextColor(foreground, background);

  if (WiFi.status() == WL_CONNECTED) {
    display.drawString("SSID: " + WiFi.SSID(), 8, y); y += 16;
    display.drawString("IP: " + WiFi.localIP().toString(), 8, y); y += 16;
    display.drawString("Gateway: " + WiFi.gatewayIP().toString(), 8, y); y += 16;
    display.drawString("Mascara: " + WiFi.subnetMask().toString(), 8, y); y += 16;
    display.drawString("Sinal: " + String(signalDbm) + " dBm", 8, y); y += 16;
  } else {
    display.drawString("Conectando a rede configurada...", 8, y); y += 20;
  }

  display.setTextColor(TFT_YELLOW, background);
  display.drawString("Gateway (" + WiFi.gatewayIP().toString() + "):", 8, y); y += 16;
  if (WiFi.status() != WL_CONNECTED) {
    display.drawString("Aguardando Wi-Fi", 8, y);
  } else if (!pingKnown || pingInProgress) {
    display.drawString("Verificando gateway...", 8, y);
  } else if (pingReachable) {
    display.setTextColor(TFT_GREEN, background);
    display.drawString("ACESSIVEL - " + String(pingLatencyMs) + " ms", 8, y);
  } else {
    display.setTextColor(TFT_RED, background);
    display.drawString("SEM RESPOSTA (ping bloqueado?)", 8, y);
  }

  display.setTextColor(TFT_LIGHTGREY, background);
  display.drawString(colorGameMode ? "N: sair  ESPACO: iniciar" : "J: jogo IP-cor  OK: atualizar  R: Wi-Fi", 8, display.height() - 25);
  if (scanInProgress) {
    display.setTextColor(TFT_CYAN, background);
    display.drawString("Buscando dispositivos na LAN...", 8, display.height() - 43);
  }
}

void runGatewayPing() {
  if (WiFi.status() != WL_CONNECTED || pingInProgress) return;
  pingInProgress = true;
  const uint32_t start = millis();
  const bool ok = Ping.ping(WiFi.gatewayIP(), 1);
  const uint32_t elapsed = millis() - start;
  pingReachable = ok;
  pingLatencyMs = ok ? static_cast<int>(Ping.averageTime()) : -1;
  if (ok && pingLatencyMs < 0) pingLatencyMs = static_cast<int>(elapsed);
  pingKnown = true;
  pingInProgress = false;
  lastPingAt = millis();
}

void scanWifiNetworks() {
  if (WiFi.status() != WL_CONNECTED || scanInProgress) return;
  scanInProgress = true;
  WiFi.scanDelete();
  const int count = WiFi.scanNetworks(false, true, false, 100);
  auto &display = M5Cardputer.Display;
  display.fillScreen(TFT_BLACK);
  display.setTextDatum(top_left);
  display.setTextSize(1);
  display.setTextColor(TFT_CYAN, TFT_BLACK);
  display.drawString("REDES WI-FI PROXIMAS", 8, 5);
  if (count <= 0) {
    display.setTextColor(TFT_YELLOW, TFT_BLACK);
    display.drawString("Nenhuma rede visivel encontrada", 8, 30);
  } else {
    display.setTextColor(TFT_WHITE, TFT_BLACK);
    const int shown = count < 7 ? count : 7;
    for (int i = 0; i < shown; ++i) {
      String row = WiFi.SSID(i) + "  " + String(WiFi.RSSI(i)) + " dBm";
      display.drawString(row, 8, 25 + i * 18);
    }
    if (count > shown) display.drawString("+" + String(count - shown) + " redes", 8, 25 + shown * 18);
  }
  WiFi.scanDelete();
  delay(1500);
  scanInProgress = false;
  lastScreenUpdateAt = 0;
  drawScreen();
}

void showScanResults() {
  auto &display = M5Cardputer.Display;
  display.fillScreen(TFT_BLACK);
  display.setTextDatum(top_left);
  display.setTextSize(1);
  display.setTextColor(TFT_CYAN, TFT_BLACK);
  display.drawString(scanCancelled ? "VARREDURA CANCELADA" : "FIM DA VARREDURA", 8, 8);
  display.setTextColor(TFT_WHITE, TFT_BLACK);
  display.drawString("Hosts encontrados: " + String(foundHostCount), 8, 30);
  constexpr size_t RESULT_PAGE_SIZE = 7;
  pageNumber = 0;
  const uint8_t pageCount = (foundHostCount + RESULT_PAGE_SIZE - 1) / RESULT_PAGE_SIZE;
  if (foundHostCount > RESULT_PAGE_SIZE) {
    display.setTextColor(TFT_YELLOW, TFT_BLACK);
    display.drawString("Pagina " + String(pageNumber + 1) + "/" + String(pageCount), 8, 50);
  }
  const uint8_t first = pageNumber * RESULT_PAGE_SIZE;
  const uint8_t last = (first + RESULT_PAGE_SIZE < foundHostCount) ? first + RESULT_PAGE_SIZE : foundHostCount;
  for (uint8_t i = first; i < last; ++i) {
    display.setTextColor(TFT_GREEN, TFT_BLACK);
    display.drawString(foundHosts[i].toString(), 8, 72 + (i - first) * 18);
  }
  display.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  display.drawString("< >: pagina  tecla: voltar", 8, display.height() - 20);
}

void playColorGameRound() {
  if (!gameActive || scanInProgress || WiFi.status() != WL_CONNECTED) return;
  scanInProgress = true;
  scanCancelled = false;
  foundHostCount = 0;

  const uint32_t ip = static_cast<uint32_t>(WiFi.localIP());
  const uint32_t mask = static_cast<uint32_t>(WiFi.subnetMask());
  const uint32_t network = ip & mask;
  const uint32_t broadcast = network | ~mask;
  const uint32_t hostCount = broadcast > network + 1 ? broadcast - network - 1 : 0;
  if (hostCount == 0 || hostCount > SCAN_HOST_LIMIT) {
    scanInProgress = false;
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    M5Cardputer.Display.setTextColor(TFT_RED, TFT_BLACK);
    M5Cardputer.Display.drawString("Sub-rede muito grande para scanner", 8, 40);
    delay(1500);
    drawScreen();
    return;
  }

  auto &display = M5Cardputer.Display;
  display.fillScreen(TFT_BLACK);
  display.setTextDatum(top_left);
  display.setTextSize(1);
  display.setTextColor(TFT_CYAN, TFT_BLACK);
  display.drawString("BUSCANDO HOSTS DA SUA LAN", 8, 8);
  display.setTextColor(TFT_YELLOW, TFT_BLACK);
  display.drawString(String(hostCount) + " enderecos; aguarde", 8, 28);
  display.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  display.drawString("Qualquer tecla cancela", 8, 48);

  for (uint32_t host = 1; host <= hostCount; ++host) {
    M5Cardputer.update();
    if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
      scanCancelled = true;
      break;
    }
    const uint32_t candidateRaw = network + host;
    if (candidateRaw == ip || candidateRaw == static_cast<uint32_t>(WiFi.gatewayIP())) continue;
    const IPAddress candidate(candidateRaw);
    if (!Ping.ping(candidate, 1)) continue;

    if (foundHostCount < MAX_DISCOVERED_HOSTS)
      foundHosts[foundHostCount++] = candidate;

    uint16_t nextColor = BACKGROUND_PALETTE[random(PALETTE_SIZE)];
    if (PALETTE_SIZE > 1 && nextColor == currentBackground)
      nextColor = BACKGROUND_PALETTE[(random(PALETTE_SIZE - 1) + 1) % PALETTE_SIZE];
    currentBackground = nextColor;
    const uint16_t ipColor = currentBackground == TFT_DARKGREEN || currentBackground == TFT_NAVY
                                 ? TFT_WHITE : TFT_BLACK;
    display.fillScreen(currentBackground);
    display.setTextDatum(middle_center);
    display.setTextSize(3);
    display.setTextColor(ipColor, currentBackground);
    display.drawString(candidate.toString(), display.width() / 2, display.height() / 2 - 10);
    display.setTextSize(1);
    display.drawString("HOST ENCONTRADO!", 8, display.height() - 24);
    delay(800);
  }

  scanInProgress = false;
  gameActive = false;
  colorGameMode = false;
  showScanResults();
}

void connectWifi() {
  if (strcmp(NETWORK_WIFI_SSID, "PREENCHA_O_NOME_DA_REDE") == 0 ||
      strcmp(NETWORK_WIFI_PASSWORD, "PREENCHA_A_SENHA") == 0 ||
      strlen(NETWORK_WIFI_SSID) == 0) {
    M5Cardputer.Display.fillScreen(TFT_BLACK);
    M5Cardputer.Display.setTextColor(TFT_RED, TFT_BLACK);
    M5Cardputer.Display.drawString("Configure include/network_config.h", 8, 35);
    return;
  }
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(NETWORK_WIFI_SSID, NETWORK_WIFI_PASSWORD);
  wifiStarted = true;
}
}  // namespace

void setup() {
  auto config = M5.config();
  M5Cardputer.begin(config, true);
  M5Cardputer.Display.setRotation(1);
  WiFi.setSleep(false);
  connectWifi();
  drawScreen();
}

void loop() {
  M5Cardputer.update();
  const uint32_t now = millis();
  const wl_status_t wifiStatus = WiFi.status();
  if (wifiStatus == WL_CONNECTED) {
    signalDbm = static_cast<int8_t>(WiFi.RSSI());
    signalBars = barsFromRssi(signalDbm);
    if (!pingInProgress && now - lastPingAt >= PING_INTERVAL_MS) runGatewayPing();
  } else if (!wifiStarted || (wifiStatus != WL_IDLE_STATUS && now - lastWifiRetryAt >= WIFI_RETRY_INTERVAL_MS)) {
    WiFi.disconnect(false, false);
    WiFi.begin(NETWORK_WIFI_SSID, NETWORK_WIFI_PASSWORD);
    wifiStarted = true;
    lastWifiRetryAt = now;
  }

  if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
    const auto keys = M5Cardputer.Keyboard.keysState();
    if (gameActive) {
      if (!scanInProgress && (keys.left || keys.up || keys.right || keys.down)) {
        constexpr uint8_t RESULT_PAGE_SIZE = 7;
        const uint8_t pageCount = (foundHostCount + RESULT_PAGE_SIZE - 1) / RESULT_PAGE_SIZE;
        if (keys.right || keys.down) pageNumber = (pageNumber + 1) % (pageCount ? pageCount : 1);
        else pageNumber = (pageNumber == 0) ? (pageCount ? pageCount - 1 : 0) : pageNumber - 1;
        showScanResults();
      } else if (keys.space) {
        playColorGameRound();
      } else {
        for (char key : keys.word) {
          if ((key == 'n' || key == 'N') && !scanInProgress) {
            gameActive = false;
            colorGameMode = false;
            drawScreen();
            break;
          }
        }
      }
    } else {
      bool handled = false;
      for (char key : keys.word) {
        if ((key == 'j' || key == 'J') && WiFi.status() == WL_CONNECTED) {
          gameActive = true;
          colorGameMode = true;
          currentBackground = TFT_BLACK;
          drawScreen();
          M5Cardputer.Display.setTextDatum(middle_center);
          M5Cardputer.Display.setTextSize(2);
          M5Cardputer.Display.setTextColor(TFT_WHITE, currentBackground);
          M5Cardputer.Display.drawString("SPACE: iniciar jogo", M5Cardputer.Display.width() / 2,
                                          M5Cardputer.Display.height() / 2);
          handled = true;
          break;
        }
      }
      if (!handled) {
        if (keys.enter || keys.space) {
          lastPingAt = 0;
          runGatewayPing();
        } else {
          for (char key : keys.word) if (key == 'r' || key == 'R') scanWifiNetworks();
        }
      }
    }
  }

  if (now - lastScreenUpdateAt >= REFRESH_INTERVAL_MS && !gameActive && !scanInProgress) {
    lastScreenUpdateAt = now;
    drawScreen();
  }
  delay(10);
}
