#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>

namespace {
constexpr int SD_SCK_PIN = 40;
constexpr int SD_MISO_PIN = 39;
constexpr int SD_MOSI_PIN = 14;
constexpr int SD_CS_PIN = 12;
constexpr uint32_t SAMPLE_RATE = 16000;
constexpr size_t SAMPLES_PER_CHUNK = 256;
constexpr uint32_t MAX_RECORDING_SECONDS = 300;
constexpr char RECORDINGS_DIR[] = "/RECORDINGS";

struct __attribute__((packed)) WavHeader {
  char riff[4] = {'R', 'I', 'F', 'F'};
  uint32_t fileSize = 0;
  char wave[4] = {'W', 'A', 'V', 'E'};
  char fmt[4] = {'f', 'm', 't', ' '};
  uint32_t fmtSize = 16;
  uint16_t audioFormat = 1;
  uint16_t channels = 1;
  uint32_t sampleRate = SAMPLE_RATE;
  uint32_t byteRate = SAMPLE_RATE * sizeof(int16_t);
  uint16_t blockAlign = sizeof(int16_t);
  uint16_t bitsPerSample = 16;
  char data[4] = {'d', 'a', 't', 'a'};
  uint32_t dataSize = 0;
};

File recordingFile;
String lastRecording;
bool isRecording = false;
uint32_t recordingStartedAt = 0;
uint32_t recordedBytes = 0;
int16_t audioChunk[SAMPLES_PER_CHUNK];

void drawScreen(const char *status, const char *detail = "") {
  auto &display = M5Cardputer.Display;
  display.fillScreen(TFT_BLACK);
  display.setTextDatum(middle_center);
  display.setTextColor(TFT_CYAN, TFT_BLACK);
  display.setTextSize(2);
  display.drawString("Gravador WAV", display.width() / 2, 22);
  display.setTextColor(isRecording ? TFT_RED : TFT_WHITE, TFT_BLACK);
  display.setTextSize(1);
  display.drawString(status, display.width() / 2, 55);
  display.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  display.drawString(detail, display.width() / 2, 76);
  display.drawString("R: gravar/parar   P: reproduzir", display.width() / 2, 101);
}

String nextRecordingPath() {
  char path[40];
  for (uint32_t number = 1; number < 100000; ++number) {
    snprintf(path, sizeof(path), "%s/VOZ%05lu.WAV", RECORDINGS_DIR,
             static_cast<unsigned long>(number));
    if (!SD.exists(path)) {
      return String(path);
    }
  }
  return String();
}

void startRecording() {
  const String path = nextRecordingPath();
  if (path.isEmpty()) {
    drawScreen("Erro: nao consegui nomear o arquivo");
    return;
  }

  recordingFile = SD.open(path.c_str(), FILE_WRITE);
  if (!recordingFile) {
    drawScreen("Erro ao criar arquivo", path.c_str());
    return;
  }

  const WavHeader header;
  if (recordingFile.write(reinterpret_cast<const uint8_t *>(&header), sizeof(header)) != sizeof(header)) {
    recordingFile.close();
    SD.remove(path.c_str());
    drawScreen("Erro ao escrever WAV");
    return;
  }

  lastRecording = path;
  recordedBytes = 0;
  recordingStartedAt = millis();
  isRecording = true;
  drawScreen("GRAVANDO", path.c_str());
}

void stopRecording() {
  if (!isRecording) return;

  isRecording = false;
  if (recordingFile) {
    WavHeader header;
    header.dataSize = recordedBytes;
    header.fileSize = sizeof(WavHeader) - 8 + recordedBytes;
    recordingFile.seek(0);
    const bool headerWritten = recordingFile.write(
        reinterpret_cast<const uint8_t *>(&header), sizeof(header)) == sizeof(header);
    recordingFile.close();

    if (!headerWritten || recordedBytes == 0) {
      SD.remove(lastRecording.c_str());
      lastRecording = "";
      drawScreen("Gravacao nao foi salva corretamente");
      return;
    }
  }

  drawScreen("Salvo no microSD", lastRecording.c_str());
}

bool readKeyboardCommand(char &command) {
  if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed()) {
    return false;
  }

  const auto keys = M5Cardputer.Keyboard.keysState();
  for (char key : keys.word) {
    if (key == 'r' || key == 'R' || key == 'p' || key == 'P') {
      command = static_cast<char>(tolower(key));
      return true;
    }
  }
  return false;
}

void recordChunk() {
  if (!isRecording) return;

  if (millis() - recordingStartedAt >= MAX_RECORDING_SECONDS * 1000UL) {
    stopRecording();
    drawScreen("Limite de 5 min; arquivo salvo", lastRecording.c_str());
    return;
  }

  if (!M5Cardputer.Mic.record(audioChunk, SAMPLES_PER_CHUNK, SAMPLE_RATE)) {
    return;
  }

  const size_t bytesToWrite = sizeof(audioChunk);
  if (recordingFile.write(reinterpret_cast<const uint8_t *>(audioChunk), bytesToWrite) != bytesToWrite) {
    stopRecording();
    drawScreen("Erro no microSD; gravacao encerrada");
    return;
  }
  recordedBytes += bytesToWrite;

  static uint32_t lastScreenUpdate = 0;
  if (millis() - lastScreenUpdate >= 1000) {
    lastScreenUpdate = millis();
    char detail[64];
    snprintf(detail, sizeof(detail), "%s | %lus",
             lastRecording.c_str(),
             static_cast<unsigned long>((millis() - recordingStartedAt) / 1000));
    drawScreen("GRAVANDO", detail);
  }
}

void playLastRecording() {
  if (lastRecording.isEmpty()) {
    drawScreen("Ainda nao ha gravacao nesta sessao");
    return;
  }

  File file = SD.open(lastRecording.c_str(), FILE_READ);
  if (!file) {
    drawScreen("Nao consegui abrir o WAV", lastRecording.c_str());
    return;
  }

  WavHeader header;
  if (file.read(reinterpret_cast<uint8_t *>(&header), sizeof(header)) != sizeof(header) ||
      memcmp(header.riff, "RIFF", 4) != 0 || memcmp(header.wave, "WAVE", 4) != 0 ||
      header.channels != 1 || header.bitsPerSample != 16 || header.sampleRate == 0) {
    file.close();
    drawScreen("Arquivo WAV invalido", lastRecording.c_str());
    return;
  }

  isRecording = false;
  M5Cardputer.Mic.end();
  M5Cardputer.Speaker.begin();
  M5Cardputer.Speaker.setVolume(160);
  drawScreen("REPRODUZINDO", lastRecording.c_str());

  uint32_t remainingBytes = header.dataSize;
  bool interrupted = false;
  while (remainingBytes >= sizeof(audioChunk)) {
    M5Cardputer.update();
    char command;
    if (readKeyboardCommand(command) && command == 'p') {
      interrupted = true;
      break;
    }

    if (file.read(reinterpret_cast<uint8_t *>(audioChunk), sizeof(audioChunk)) != sizeof(audioChunk)) {
      break;
    }
    M5Cardputer.Speaker.playRaw(audioChunk, SAMPLES_PER_CHUNK, header.sampleRate);
    remainingBytes -= sizeof(audioChunk);
    while (M5Cardputer.Speaker.isPlaying()) {
      delay(1);
      M5Cardputer.update();
      if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
        const auto keys = M5Cardputer.Keyboard.keysState();
        for (char key : keys.word) {
          if (key == 'p' || key == 'P') interrupted = true;
        }
      }
      if (interrupted) {
        M5Cardputer.Speaker.stop();
        break;
      }
    }
    if (interrupted) break;
  }

  file.close();
  M5Cardputer.Speaker.end();
  M5Cardputer.Mic.begin();
  drawScreen(interrupted ? "Reproducao interrompida" : "Reproducao finalizada",
             lastRecording.c_str());
}
}  // namespace

void setup() {
  auto config = M5.config();
  M5Cardputer.begin(config, true);
  M5Cardputer.Display.setRotation(1);

  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  if (!SD.begin(SD_CS_PIN, SPI, 25000000)) {
    drawScreen("microSD nao detectado", "Insira um cartao FAT32 e reinicie");
    return;
  }

  if (!SD.exists(RECORDINGS_DIR) && !SD.mkdir(RECORDINGS_DIR)) {
    drawScreen("Erro ao criar pasta no microSD");
    return;
  }

  M5Cardputer.Speaker.end();
  if (!M5Cardputer.Mic.begin()) {
    drawScreen("Microfone indisponivel");
    return;
  }
  drawScreen("Pronto", "Arquivos WAV em /RECORDINGS");
}

void loop() {
  M5Cardputer.update();

  char command;
  if (readKeyboardCommand(command)) {
    if (command == 'r') {
      if (isRecording) stopRecording();
      else startRecording();
    } else if (command == 'p' && !isRecording) {
      playLastRecording();
    }
  }

  recordChunk();
  delay(1);
}
