#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>

#ifndef DEVOTIONAL_CHAPTER_INDEX
#define DEVOTIONAL_CHAPTER_INDEX "/CAPITULOS.BIN"
#endif
#ifndef DEVOTIONAL_VERSE_INDEX
#define DEVOTIONAL_VERSE_INDEX "/VERSICULOS.BIN"
#endif
#ifndef DEVOTIONAL_TEXT
#define DEVOTIONAL_TEXT "/BIBLIA.TXT"
#endif

namespace {
constexpr int SD_SCK_PIN = 40;
constexpr int SD_MISO_PIN = 39;
constexpr int SD_MOSI_PIN = 14;
constexpr int SD_CS_PIN = 12;
constexpr uint16_t CHAPTER_RECORD_BYTES = 8;
constexpr uint16_t VERSE_RECORD_BYTES = 6;
constexpr uint16_t MAX_VERSES_PER_CHAPTER = 200;
constexpr uint16_t MIN_LONG_CHAPTER_VERSES = 40;
constexpr size_t TEXT_VIEW_BYTES = 4096;
constexpr size_t MAX_CHAPTERS = 1200;
constexpr uint8_t BOOK_COUNT = 66;

struct ChapterRecord {
  uint32_t firstVerse;
  uint16_t verseCount;
  uint8_t bookIndex;
  uint8_t chapterInBook;
};
struct VerseRecord { uint32_t textOffset; uint16_t textLength; };

File textFile;
File chapterIndexFile;
File verseIndexFile;
ChapterRecord currentChapter{};
VerseRecord currentVerse{};
uint16_t chapterCount = 0;
uint16_t verseIndex = 0;
uint16_t chapterNumber = 0;
uint32_t verseCount = 0;
uint16_t qualifyingChapters[MAX_CHAPTERS]{};
uint16_t qualifyingChapterCount = 0;
char textBuffer[TEXT_VIEW_BYTES + 1]{};
char displayBuffer[TEXT_VIEW_BYTES + 1]{};

const char *const BOOK_NAMES[BOOK_COUNT] = {
  "Genesis", "Exodo", "Levitico", "Numeros", "Deuteronomio", "Josue", "Juizes",
  "Rute", "1 Samuel", "2 Samuel", "1 Reis", "2 Reis", "1 Cronicas", "2 Cronicas",
  "Esdras", "Neemias", "Ester", "Jo", "Salmos", "Proverbios", "Eclesiastes",
  "Cantares", "Isaias", "Jeremias", "Lamentacoes", "Ezequiel", "Daniel", "Oseias",
  "Joel", "Amos", "Obadias", "Jonas", "Miqueias", "Naum", "Habacuque", "Sofonias",
  "Ageu", "Zacarias", "Malaquias", "Mateus", "Marcos", "Lucas", "Joao", "Atos",
  "Romanos", "1 Corintios", "2 Corintios", "Galatas", "Efesios", "Filipenses",
  "Colossenses", "1 Tessalonicenses", "2 Tessalonicenses", "1 Timoteo", "2 Timoteo",
  "Tito", "Filemom", "Hebreus", "Tiago", "1 Pedro", "2 Pedro", "1 Joao", "2 Joao",
  "3 Joao", "Judas", "Apocalipse"
};

bool readChapterRecord(uint16_t index, ChapterRecord &record) {
  if (!chapterIndexFile || index >= chapterCount ||
      !chapterIndexFile.seek(static_cast<uint32_t>(index) * CHAPTER_RECORD_BYTES)) return false;
  uint8_t bytes[CHAPTER_RECORD_BYTES];
  if (chapterIndexFile.read(bytes, sizeof(bytes)) != sizeof(bytes)) return false;
  record.firstVerse = static_cast<uint32_t>(bytes[0]) |
                      (static_cast<uint32_t>(bytes[1]) << 8) |
                      (static_cast<uint32_t>(bytes[2]) << 16) |
                      (static_cast<uint32_t>(bytes[3]) << 24);
  record.verseCount = static_cast<uint16_t>(bytes[4] | (bytes[5] << 8));
  record.bookIndex = bytes[6];
  record.chapterInBook = bytes[7];
  return record.verseCount > 0 && record.verseCount <= MAX_VERSES_PER_CHAPTER &&
         record.bookIndex < BOOK_COUNT && record.chapterInBook > 0;
}

bool readVerseRecord(uint32_t index, VerseRecord &record) {
  if (!verseIndexFile || !verseIndexFile.seek(index * VERSE_RECORD_BYTES)) return false;
  uint8_t bytes[VERSE_RECORD_BYTES];
  if (verseIndexFile.read(bytes, sizeof(bytes)) != sizeof(bytes)) return false;
  record.textOffset = static_cast<uint32_t>(bytes[0]) |
                      (static_cast<uint32_t>(bytes[1]) << 8) |
                      (static_cast<uint32_t>(bytes[2]) << 16) |
                      (static_cast<uint32_t>(bytes[3]) << 24);
  record.textLength = static_cast<uint16_t>(bytes[4] | (bytes[5] << 8));
  return record.textLength > 0 && record.textLength <= TEXT_VIEW_BYTES;
}

void drawWrappedText(const char *text, int x, int y, int maxWidth, int lineHeight) {
  String remaining(text);
  String line;
  while (remaining.length() > 0 && y < M5Cardputer.Display.height() - 28) {
    const int split = remaining.indexOf(' ');
    String word = split < 0 ? remaining : remaining.substring(0, split);
    if (word.isEmpty()) {
      remaining = split < 0 ? String() : remaining.substring(split + 1);
      continue;
    }
    String candidate = line.isEmpty() ? word : line + ' ' + word;
    if (M5Cardputer.Display.textWidth(candidate) > maxWidth && !line.isEmpty()) {
      M5Cardputer.Display.drawString(line, x, y);
      y += lineHeight;
      line = word;
    } else {
      line = candidate;
    }
    if (split < 0) {
      if (!line.isEmpty() && y < M5Cardputer.Display.height() - 28)
        M5Cardputer.Display.drawString(line, x, y);
      remaining = String();
      break;
    }
    remaining = remaining.substring(split + 1);
  }
  if (remaining.length() > 0) {
    M5Cardputer.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
    M5Cardputer.Display.drawString("... proximo: >", 8, M5Cardputer.Display.height() - 31);
  }
}

void updateDisplay() {
  auto &display = M5Cardputer.Display;
  display.fillScreen(TFT_BLACK);
  display.setTextDatum(top_left);
  display.setTextSize(1);
  display.setTextColor(TFT_CYAN, TFT_BLACK);
  char heading[64];
  snprintf(heading, sizeof(heading), "%s %u", BOOK_NAMES[currentChapter.bookIndex], currentChapter.chapterInBook);
  display.drawString(heading, 8, 4);
  snprintf(heading, sizeof(heading), "Versiculo %u/%u", verseIndex + 1, currentChapter.verseCount);
  display.setTextColor(TFT_YELLOW, TFT_BLACK);
  display.drawString(heading, 8, 20);

  const uint32_t globalVerse = currentChapter.firstVerse + verseIndex;
  if (!readVerseRecord(globalVerse, currentVerse) ||
      !textFile.seek(currentVerse.textOffset) ||
      textFile.read(reinterpret_cast<uint8_t *>(textBuffer), currentVerse.textLength) != currentVerse.textLength) {
    display.setTextColor(TFT_RED, TFT_BLACK);
    display.drawString("Erro lendo o texto do microSD", 8, 46);
    return;
  }
  textBuffer[currentVerse.textLength] = '\0';

  // M5GFX's compact built-in font is not a full Portuguese Unicode font.
  // Transliterate UTF-8 diacritics to ASCII so every verse remains readable.
  String normalized(textBuffer);
  normalized.replace("á", "a"); normalized.replace("à", "a");
  normalized.replace("ã", "a"); normalized.replace("â", "a"); normalized.replace("ä", "a");
  normalized.replace("Á", "A"); normalized.replace("À", "A");
  normalized.replace("Ã", "A"); normalized.replace("Â", "A"); normalized.replace("Ä", "A");
  normalized.replace("é", "e"); normalized.replace("è", "e");
  normalized.replace("ê", "e"); normalized.replace("ë", "e");
  normalized.replace("É", "E"); normalized.replace("È", "E");
  normalized.replace("Ê", "E"); normalized.replace("Ë", "E");
  normalized.replace("í", "i"); normalized.replace("ì", "i");
  normalized.replace("î", "i"); normalized.replace("ï", "i");
  normalized.replace("Í", "I"); normalized.replace("Ì", "I");
  normalized.replace("Î", "I"); normalized.replace("Ï", "I");
  normalized.replace("ó", "o"); normalized.replace("ò", "o");
  normalized.replace("õ", "o"); normalized.replace("ô", "o"); normalized.replace("ö", "o");
  normalized.replace("Ó", "O"); normalized.replace("Ò", "O");
  normalized.replace("Õ", "O"); normalized.replace("Ô", "O"); normalized.replace("Ö", "O");
  normalized.replace("ú", "u"); normalized.replace("ù", "u");
  normalized.replace("û", "u"); normalized.replace("ü", "u");
  normalized.replace("Ú", "U"); normalized.replace("Ù", "U");
  normalized.replace("Û", "U"); normalized.replace("Ü", "U");
  normalized.replace("ç", "c"); normalized.replace("Ç", "C");
  normalized.replace("æ", "ae"); normalized.replace("œ", "oe");
  normalized.replace("ſ", "s");
  strlcpy(displayBuffer, normalized.c_str(), sizeof(displayBuffer));
  display.setTextColor(TFT_WHITE, TFT_BLACK);
  drawWrappedText(displayBuffer, 8, 43, display.width() - 16, 16);

  display.setTextColor(TFT_GREEN, TFT_BLACK);
  display.drawString("R: cap. longo  >/<: versiculos", 8, display.height() - 17);
}

bool loadChapter(uint16_t index) {
  ChapterRecord next{};
  if (!readChapterRecord(index, next) ||
      static_cast<uint64_t>(next.firstVerse) + next.verseCount > verseCount) return false;
  chapterNumber = index;
  currentChapter = next;
  verseIndex = 0;
  updateDisplay();
  return true;
}

void showError(const char *message);

void randomLongChapter() {
  if (qualifyingChapterCount == 0) return;
  const uint16_t next = qualifyingChapters[random(qualifyingChapterCount)];
  if (!loadChapter(next)) showError("Falha ao abrir capitulo");
}

void showError(const char *message) {
  M5Cardputer.Display.fillScreen(TFT_BLACK);
  M5Cardputer.Display.setTextDatum(top_left);
  M5Cardputer.Display.setTextColor(TFT_RED, TFT_BLACK);
  M5Cardputer.Display.setTextSize(1);
  M5Cardputer.Display.drawString("Devocional Almeida", 8, 8);
  M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5Cardputer.Display.drawString(message, 8, 38);
}
}  // namespace

void setup() {
  auto config = M5.config();
  M5Cardputer.begin(config, true);
  M5Cardputer.Display.setRotation(1);
  randomSeed(micros());

  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  if (!SD.begin(SD_CS_PIN, SPI, 25000000)) {
    showError("Insira microSD FAT32 com arquivos da Biblia");
    return;
  }

  textFile = SD.open(DEVOTIONAL_TEXT, FILE_READ);
  chapterIndexFile = SD.open(DEVOTIONAL_CHAPTER_INDEX, FILE_READ);
  verseIndexFile = SD.open(DEVOTIONAL_VERSE_INDEX, FILE_READ);
  if (!textFile || !chapterIndexFile || !verseIndexFile) {
    showError("Arquivos ausentes; veja DEVOCIONAL.md");
    return;
  }
  if (chapterIndexFile.size() % CHAPTER_RECORD_BYTES != 0 ||
      verseIndexFile.size() % VERSE_RECORD_BYTES != 0) {
    showError("Indice da Biblia invalido");
    return;
  }
  chapterCount = chapterIndexFile.size() / CHAPTER_RECORD_BYTES;
  verseCount = verseIndexFile.size() / VERSE_RECORD_BYTES;
  if (chapterCount == 0 || chapterCount > MAX_CHAPTERS || verseCount == 0) {
    showError("Indice da Biblia invalido");
    return;
  }
  for (uint16_t i = 0; i < chapterCount; ++i) {
    ChapterRecord record{};
    if (!readChapterRecord(i, record) ||
        static_cast<uint64_t>(record.firstVerse) + record.verseCount > verseCount) {
      showError("Indice da Biblia corrompido");
      return;
    }
    if (record.verseCount >= MIN_LONG_CHAPTER_VERSES)
      qualifyingChapters[qualifyingChapterCount++] = i;
  }
  if (qualifyingChapterCount == 0) {
    showError("Nao ha capitulos longos no indice");
    return;
  }
  if (!loadChapter(0)) {
    showError("Falha ao abrir Biblia no microSD");
    return;
  }
  randomLongChapter();
}

void loop() {
  M5Cardputer.update();
  if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
    const auto keys = M5Cardputer.Keyboard.keysState();
  if (keys.down || keys.right) {
    if (verseIndex + 1 < currentChapter.verseCount) {
      ++verseIndex;
      updateDisplay();
    } else if (chapterNumber + 1 < chapterCount) {
      loadChapter(chapterNumber + 1);
    }
  } else if (keys.up || keys.left) {
    if (verseIndex > 0) {
      --verseIndex;
      updateDisplay();
    } else if (chapterNumber > 0) {
      loadChapter(chapterNumber - 1);
      verseIndex = currentChapter.verseCount - 1;
      updateDisplay();
    }
  } else {
    for (char key : keys.word) {
      if (key == 'r' || key == 'R') randomLongChapter();
    }
  }
  }
  delay(5);
}
