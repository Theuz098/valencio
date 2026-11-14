/*
  ASTRINHO — bichinho digital com ESP32-C3 + tela redonda GC9A01
  ---------------------------------------------------------------
  Placa:      ESP32-C3 SuperMini (Arduino IDE: "ESP32C3 Dev Module",
              USB CDC On Boot = Enabled)
  Core:       esp32 by Espressif 3.x  (Gerenciador de Placas)
  Biblioteca: LovyanGFX (Gerenciador de Bibliotecas)

  O que ele faz:
   - Olhos com personalidade única (formato + cor sorteados no 1º boot, salvos na flash)
   - Pisca e olha ao redor sozinho; olha para o lado em que você inclina
   - Sacudiu  -> fica TONTO (olhos em espiral) e depois BRAVO
   - Toquinho -> fica FELIZ (^ ^) e vibra
   - Barulho alto por >1s -> fica ANSIOSO (pupilas tremendo, pisca rápido)
   - Frio -> TREME e os olhos ficam azulados
   - Parado por 2 min -> DORME (tela escurece). Mexeu, acorda.
   - Dois toques -> mostra a bateria
   - Encontrou outro astrinho por perto (ESP-NOW) -> fica feliz e pode "pegar" a cor dele

  Ligações (veja GUIA-ESP32.md):
   GC9A01:  SCL->4  SDA->6  CS->7  DC->5  RST->10  BLK->2  VCC->3V3  GND
   MPU6050: SDA->8  SCL->9  VCC->3V3  GND
   INMP441: SCK->0  WS->1   SD->21  L/R->GND  VDD->3V3  GND
   Motor:   GPIO20 -> 1k -> base do transistor (2N2222/S8050); motor entre 3V3 e coletor; diodo 1N4148 em paralelo
   Bateria: divisor 100k/100k entre BAT+ e GND, meio -> GPIO3
*/

#include <LovyanGFX.hpp>
#include <Wire.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ESP_I2S.h>

// ------------------------- PINOS -------------------------
#define PIN_SCLK 4
#define PIN_MOSI 6
#define PIN_CS   7
#define PIN_DC   5
#define PIN_RST  10
#define PIN_BL   2   // brilho (PWM). Se ligar BLK direto no 3V3, só não escurece ao dormir
#define PIN_SDA  8
#define PIN_SCL  9
#define PIN_MIC_SCK 0
#define PIN_MIC_WS  1
#define PIN_MIC_SD  21
#define PIN_MOTOR 20
#define PIN_BAT   3

// ------------------------- AJUSTES -------------------------
const float SHAKE_G        = 2.2f;   // aceleração (g) que conta como sacudida
const float TAP_G          = 1.6f;   // pico curto que conta como toque
const float LOUD_LEVEL     = 2500;   // nível de barulho (ajuste olhando o Serial)
const float COLD_C         = 18.0f;  // abaixo disso sente frio (o MPU6050 mede uns 3-5°C acima do ambiente)
const uint32_t SLEEP_MS    = 120000; // tempo parado até dormir
const int  FRIEND_RSSI     = -50;    // quão perto outro astrinho precisa estar

// ------------------------- TELA (LovyanGFX) -------------------------
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_GC9A01 panel;
  lgfx::Bus_SPI bus;
  lgfx::Light_PWM light;
public:
  LGFX() {
    auto b = bus.config();
    b.spi_host = SPI2_HOST; b.spi_mode = 0; b.freq_write = 40000000;
    b.pin_sclk = PIN_SCLK; b.pin_mosi = PIN_MOSI; b.pin_miso = -1; b.pin_dc = PIN_DC;
    b.dma_channel = SPI_DMA_CH_AUTO;
    bus.config(b); panel.setBus(&bus);
    auto p = panel.config();
    p.pin_cs = PIN_CS; p.pin_rst = PIN_RST; p.pin_busy = -1;
    p.panel_width = 240; p.panel_height = 240; p.invert = true;
    panel.config(p);
    auto l = light.config();
    l.pin_bl = PIN_BL; l.invert = false; l.freq = 12000; l.pwm_channel = 1;
    light.config(l); panel.setLight(&light);
    setPanel(&panel);
  }
};
LGFX tft;
LGFX_Sprite fb(&tft);   // desenha tudo aqui e envia de uma vez (sem piscar)

// ------------------------- PERSONALIDADE -------------------------
enum Shape : uint8_t { DOT, CIRCLE, CAT, ACORN, SHAPE_COUNT };
struct Look { uint8_t shape; uint16_t color; };
const uint16_t PALETTE[] = {
  0xFCE0 /*marmelada*/, 0x8E09 /*matcha*/, 0x3B5F /*baleia*/, 0xFB76 /*chiclete*/,
  0xF7B9 /*lua*/, 0xFAC9 /*lagosta*/, 0xB4FF /*lilás*/, 0x7F19 /*vidro do mar*/,
  0x3FF1 /*neon*/, 0xFBEA /*pôr do sol*/
};
const int PALETTE_N = sizeof(PALETTE) / sizeof(PALETTE[0]);
Look me;
Preferences prefs;

// ------------------------- ESTADO -------------------------
enum Mood { NORMAL, HAPPY, DIZZY, ANGRY, ANXIOUS, COLD, SLEEPY, BATTERY };
Mood mood = NORMAL;
uint32_t moodUntil = 0, lastMotion = 0, blinkAt = 0, saccadeAt = 0, lastTap = 0, motorOff = 0;
float lookX = 0, lookY = 0, tgtX = 0, tgtY = 0;
float ax = 0, ay = 0, az = 1, tempC = 25, noise = 0;
int loudFrames = 0;
I2SClass mic;

void setMood(Mood m, uint32_t ms) { mood = m; moodUntil = millis() + ms; }
void buzz(uint32_t ms) { digitalWrite(PIN_MOTOR, HIGH); motorOff = millis() + ms; }

// ------------------------- SENSORES -------------------------
void mpuWrite(uint8_t reg, uint8_t v) { Wire.beginTransmission(0x68); Wire.write(reg); Wire.write(v); Wire.endTransmission(); }

void readMotion() {
  Wire.beginTransmission(0x68); Wire.write(0x3B); Wire.endTransmission(false);
  if (Wire.requestFrom(0x68, 8) != 8) return;
  int16_t r[4];
  for (int i = 0; i < 4; i++) r[i] = (Wire.read() << 8) | Wire.read();
  ax = r[0] / 16384.0f; ay = r[1] / 16384.0f; az = r[2] / 16384.0f;
  tempC = tempC * 0.98f + (r[3] / 340.0f + 36.53f) * 0.02f;   // média lenta
}

void readNoise() {
  int32_t buf[128];
  size_t n = mic.readBytes((char*)buf, sizeof(buf)) / 4;
  if (!n) return;
  double sum = 0;
  for (size_t i = 0; i < n; i++) { float s = (buf[i] >> 14); sum += s * s; }
  noise = noise * 0.8f + sqrt(sum / n) * 0.2f;
}

int batteryPercent() {
  float v = analogReadMilliVolts(PIN_BAT) * 2 / 1000.0f;   // divisor 1:2
  return constrain((int)((v - 3.3f) / (4.2f - 3.3f) * 100), 0, 100);
}

// ------------------------- ENCONTROS (ESP-NOW) -------------------------
struct Beacon { char tag[4]; uint8_t shape; uint16_t color; };
uint32_t beaconAt = 0;

void onRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len != sizeof(Beacon) || memcmp(data, "AST", 3) != 0) return;
  if (info->rx_ctrl->rssi < FRIEND_RSSI || mood == SLEEPY) return;
  const Beacon *b = (const Beacon*)data;
  if (random(100) < 10) {             // 10% de chance de "pegar" a cor do amigo
    me.color = b->color;
    prefs.putUShort("color", me.color);
  }
  setMood(HAPPY, 2500);
  buzz(80);
}

void sendBeacon() {
  Beacon b = {{'A','S','T',0}, me.shape, me.color};
  uint8_t all[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
  esp_now_send(all, (uint8_t*)&b, sizeof(b));
}

// ------------------------- DESENHO DOS OLHOS -------------------------
uint16_t tint(uint16_t c) {               // deixa a cor azulada (frio)
  uint8_t r = (c >> 11) & 0x1F, g = (c >> 5) & 0x3F, b = c & 0x1F;
  uint8_t nb = (b + 12 > 31) ? 31 : b + 12;
  return ((r / 3) << 11) | ((g * 2 / 3) << 5) | nb;
}

void drawEye(int x, int y, int w, int h, float open, uint16_t col) {
  int hh = h * open;
  if (hh < 4) { fb.fillRoundRect(x - w / 2, y - 3, w, 6, 3, col); return; }
  switch (me.shape) {
    case DOT:    fb.fillEllipse(x, y, w * 0.32, hh * 0.32, col); break;
    case CIRCLE: fb.fillEllipse(x, y, w / 2, hh / 2, col); fb.fillEllipse(x, y, w / 4, hh / 4, TFT_BLACK); break;
    case CAT:    fb.fillEllipse(x, y, w / 2, hh / 2, col); fb.fillEllipse(x, y, w / 11, hh * 0.42, TFT_BLACK); break;
    case ACORN:  fb.fillEllipse(x, y + hh / 8, w / 2, hh * 0.38, col); fb.fillTriangle(x - w / 2, y, x + w / 2, y, x, y - hh / 2, col); break;
  }
}

void drawFace() {
  uint32_t t = millis();
  fb.fillScreen(TFT_BLACK);
  uint16_t col = (mood == COLD) ? tint(me.color) : me.color;

  // piscar
  float open = 1;
  int32_t sinceBlink = t - blinkAt;
  if (sinceBlink >= 0 && sinceBlink < 180) open = 1 - sin(sinceBlink / 180.0f * PI) * 0.97f;

  float jx = 0, jy = 0;
  if (mood == COLD)    { jx = random(-3, 4); }                       // tremendo de frio
  if (mood == ANXIOUS) { jx = random(-2, 3); jy = random(-2, 3); }   // nervoso

  int ox = 120 + lookX * 22 + jx, oy = 120 + lookY * 16 + jy;
  int w = 46, h = 70;

  switch (mood) {
    case HAPPY:      // ^ ^
      for (int s : {-1, 1}) fb.fillArc(ox + s * 36, oy + 16, 20, 28, 200, 340, col);
      break;
    case DIZZY: {    // espirais girando
      float a0 = t / 120.0f;
      for (int s : {-1, 1})
        for (float a = 0; a < 4 * PI; a += 0.15f) {
          float r = a * 2.6f;
          fb.fillCircle(ox + s * 36 + cos(a + a0 * s) * r, oy + sin(a + a0 * s) * r, 3, col);
        }
      break;
    }
    case SLEEPY:     // olhos fechados + z
      drawEye(ox - 36, oy + 10, w, h, 0, col);
      drawEye(ox + 36, oy + 10, w, h, 0, col);
      fb.setTextColor(0x7BEF); fb.setTextSize(2);
      fb.drawString("z", 160 + (t / 300) % 8, 60 - (t / 200) % 20);
      break;
    case BATTERY: {
      int p = batteryPercent();
      if (p > 0) fb.fillArc(120, 120, 92, 100, -90, -90 + p * 360 / 100, p > 20 ? 0x07E0 : TFT_RED);
      fb.setTextColor(TFT_WHITE); fb.setTextSize(3); fb.setTextDatum(MC_DATUM);
      fb.drawString(String(p) + "%", 120, 120);
      fb.setTextDatum(TL_DATUM);
      break;
    }
    default:
      if (mood == ANXIOUS) { w = 30; h = 40; }
      drawEye(ox - 36, oy, w, h, open, col);
      drawEye(ox + 36, oy, w, h, open, col);
      if (mood == ANGRY) {   // sobrancelhas: "corta" o topo dos olhos em diagonal
        fb.fillTriangle(ox - 70, oy - 50, ox - 6, oy - 10, ox - 6, oy - 50, TFT_BLACK);
        fb.fillTriangle(ox + 70, oy - 50, ox + 6, oy - 10, ox + 6, oy - 50, TFT_BLACK);
      }
  }
  fb.pushSprite(0, 0);
}

// ------------------------- SETUP -------------------------
void setup() {
  Serial.begin(115200);
  pinMode(PIN_MOTOR, OUTPUT);
  analogReadResolution(12);

  tft.init(); tft.setBrightness(200);
  fb.setColorDepth(16);
  if (!fb.createSprite(240, 240)) { fb.setColorDepth(8); fb.createSprite(240, 240); }

  Wire.begin(PIN_SDA, PIN_SCL, 400000);
  mpuWrite(0x6B, 0x00);   // acorda o MPU6050
  mpuWrite(0x1C, 0x08);   // acelerômetro em ±4g

  mic.setPins(PIN_MIC_SCK, PIN_MIC_WS, -1, PIN_MIC_SD);
  mic.begin(I2S_MODE_STD, 16000, I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO, I2S_STD_SLOT_LEFT);

  // personalidade única: sorteada uma vez e guardada para sempre
  prefs.begin("astrinho", false);
  if (!prefs.isKey("shape")) {
    randomSeed(esp_random());
    prefs.putUChar("shape", random(SHAPE_COUNT));
    prefs.putUShort("color", PALETTE[random(PALETTE_N)]);
  }
  me.shape = prefs.getUChar("shape"); me.color = prefs.getUShort("color");

  WiFi.mode(WIFI_STA);
  if (esp_now_init() == ESP_OK) {
    esp_now_register_recv_cb(onRecv);
    esp_now_peer_info_t peer = {};
    memset(peer.peer_addr, 0xFF, 6); peer.channel = 0; peer.encrypt = false;
    esp_now_add_peer(&peer);
  }

  lastMotion = millis();
  blinkAt = millis() + 1500;
  setMood(HAPPY, 1500);   // acorda feliz
}

// ------------------------- LOOP -------------------------
float lastMag = 1;
void loop() {
  uint32_t t = millis();
  readMotion();
  readNoise();

  float mag = sqrt(ax * ax + ay * ay + az * az);
  float jerk = fabs(mag - lastMag); lastMag = mag;
  if (jerk > 0.08f) lastMotion = t;

  // ---- reações (da mais forte para a mais fraca) ----
  if (mag > SHAKE_G && mood != DIZZY) { setMood(DIZZY, 3000); buzz(300); }
  else if (mood == DIZZY && t > moodUntil) { setMood(ANGRY, 3000); }
  else if (jerk > TAP_G - 1 && mag < SHAKE_G && mood != DIZZY && mood != SLEEPY) {
    if (t - lastTap < 400) setMood(BATTERY, 2500);   // dois toques
    else { setMood(HAPPY, 1500); buzz(60); }
    lastTap = t;
  }

  loudFrames = noise > LOUD_LEVEL ? loudFrames + 1 : max(0, loudFrames - 2);
  if (loudFrames > 30 && mood == NORMAL) setMood(ANXIOUS, 4000);

  if (mood == SLEEPY && t - lastMotion < 500) { tft.setBrightness(200); setMood(HAPPY, 1200); }
  else if (t - lastMotion > SLEEP_MS && mood != SLEEPY) { setMood(SLEEPY, UINT32_MAX / 2); tft.setBrightness(25); }

  if (mood != NORMAL && mood != SLEEPY && t > moodUntil) mood = NORMAL;
  if (mood == NORMAL && tempC < COLD_C) mood = COLD;
  if (mood == COLD && tempC >= COLD_C + 1) mood = NORMAL;

  // ---- olhar: segue a inclinação; parado, olha ao redor sozinho ----
  if (fabs(ax) > 0.25f || fabs(ay) > 0.25f) { tgtX = constrain(-ay * 1.5f, -1, 1); tgtY = constrain(ax * 1.5f, -1, 1); saccadeAt = t + 2000; }
  else if (t > saccadeAt) { tgtX = random(-100, 101) / 100.0f; tgtY = random(-70, 71) / 100.0f; saccadeAt = t + random(1500, 4000); }
  lookX += (tgtX - lookX) * 0.15f; lookY += (tgtY - lookY) * 0.15f;

  if (t > blinkAt + 180) blinkAt = t + (mood == ANXIOUS ? random(400, 1200) : random(3000, 6000));
  if (motorOff && t > motorOff) { digitalWrite(PIN_MOTOR, LOW); motorOff = 0; }
  if (t > beaconAt) { sendBeacon(); beaconAt = t + 2000; }

  drawFace();

  static uint32_t dbg = 0;   // abra o Monitor Serial para calibrar
  if (t > dbg) { Serial.printf("g=%.2f temp=%.1f ruido=%.0f bat=%d%% humor=%d\n", mag, tempC, noise, batteryPercent(), mood); dbg = t + 1000; }
}
