#include <Adafruit_MCP23X17.h>
#include <Wire.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <array>
#include "secrets.h"

Adafruit_MCP23X17 mcp;
static constexpr uint8_t mcp_address = 0;

void write_leds(const int id, const int value)
{
  const int ia = id % 5;
  const int ib = id / 5 + 8;
  mcp.digitalWrite(ia, value);
  mcp.digitalWrite(ib, value == HIGH ? LOW : HIGH);
}

/**
 * qa, qb represent the labels on the PCB, i.e., Q1 and Q7
 */
static constexpr int compute_id(const int qa, const int qb)
{
  return (qa - 1) + (qb - 6) * 5;
}

static constexpr std::array<int, 25> cs {
  compute_id(2, 6),  // connects to Q2, Q6
  compute_id(1, 6),  // connects to Q1, Q6
  compute_id(5, 6),  // connects to Q5, Q6
  compute_id(1, 9),  // connects to Q1, Q9
  compute_id(5, 10), // etc.
  compute_id(4, 6),
  compute_id(4, 10),
  compute_id(2, 10),
  compute_id(3, 9),
  compute_id(1, 10),
  compute_id(2, 9),
  compute_id(3, 6),
  compute_id(3, 7),
  compute_id(4, 8),
  compute_id(3, 10),
  compute_id(2, 7),
  compute_id(3, 8),
  compute_id(5, 8),
  compute_id(4, 7),
  compute_id(4, 9),
  compute_id(1, 7),
  compute_id(1, 8),
  compute_id(5, 9),
  compute_id(5, 7),
  compute_id(2, 8),
};

static constexpr auto ES = cs[0];
static constexpr auto IST = cs[1];
static constexpr auto FUNF = cs[2];
static constexpr auto ZEHN = cs[3];
static constexpr auto UND = cs[4];
static constexpr auto ZWANZIG = cs[5];
static constexpr auto VIERTEL = cs[6];
static constexpr auto MINUTEN = cs[7];
static constexpr auto VOR = cs[8];
static constexpr auto NACH = cs[9];
static constexpr auto HALB = cs[10];
static constexpr auto EINS = cs[11];
static constexpr auto ZWEI = cs[12];
static constexpr auto DREI = cs[13];
static constexpr auto VIER = cs[14];
static constexpr auto FUNF_H = cs[15];
static constexpr auto SECHS = cs[16];
static constexpr auto EIN = cs[17];
static constexpr auto SIEBEN = cs[18];
static constexpr auto ACHT = cs[19];
static constexpr auto NEUN = cs[20];
static constexpr auto ZEHN_H = cs[21];
static constexpr auto ELF = cs[22];
static constexpr auto ZWOLF = cs[23];
static constexpr auto UHR = cs[24];
static constexpr auto NONE = -1;

#define picture_size 6        // the number of LEDs that appear to be ON at the same time
#define Picture std::array<int, picture_size>

void set_picture(const Picture& picture)
{
  static constexpr auto write_leds_duration_us = 0;  // approximation to make all picture equally bright
  static constexpr auto additional_duty_us = 0;
  for (std::size_t i = 0; i < picture.size(); ++i) {
    const auto p = picture[i];
    if (p > 0) {
      write_leds(p, HIGH);
//      delayMicroseconds(additional_duty_us);
      write_leds(p, LOW);
    } else {
//      delayMicroseconds(write_leds_duration_us + additional_duty_us);
    }
  }
}

static constexpr std::array<int, 12> c2     {NONE,  FUNF,    ZEHN,    VIERTEL, ZWANZIG, FUNF, NONE, FUNF, ZEHN, VIERTEL, ZEHN,    FUNF};
static constexpr std::array<int, 12> c3     {NONE,  MINUTEN, MINUTEN, NONE,    MINUTEN, VOR,  NONE, NACH, NACH, NONE,    MINUTEN, MINUTEN};
static constexpr std::array<int, 12> c4     {NONE,  NACH,    NACH,    NACH,    NACH,    HALB, HALB, HALB, HALB, VOR,     VOR,     VOR};
static constexpr std::array<int, 12> offset {0,     0,       0,       0,       0,       1,    1,    1,    1,    1,       1,       1};
static constexpr std::array<int, 12> hours  {ZWOLF, EINS,    ZWEI, DREI, VIER, FUNF_H, SECHS, SIEBEN, ACHT, NEUN, ZEHN_H, ELF};

int python_like_mod(const int dividend, const int divisor)
{
  return (dividend % divisor + divisor) % divisor;
}

int get_hour(const int hour, const int minute5)
{
  if (minute5 == 0 && hour == 1) {
    return EIN;
  } else {
    return hours[(hour + offset[minute5]) % 12];
  }
}

Picture picture_from_clock(const int hour, const int minute)
{
  const auto minute5 = python_like_mod(minute, 60) / 5;
  if (minute5 == 0) {
    return {ES, IST, get_hour(hour, minute5), UHR, NONE, NONE};
  } else {
    return {ES, IST, c2[minute5], c3[minute5], c4[minute5], get_hour(hour, minute5)};
  }
}

void test_leds_single()
{
  for (int i = 0; i < 25; ++i) {
    write_leds(cs[i], HIGH);
    delay(20);
    write_leds(cs[i], LOW);
  }
}

WiFiUDP ntpUDP;
const long utcOffsetInSeconds = 3600;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

void setup()
{
  Serial.begin(74880);

  // setup connection to the port multiplier
  mcp.begin_I2C();
  for (int i = 0; i < 16; ++i) {
    mcp.pinMode(i, OUTPUT);
  }

  // initialize all leds
  for (int i = 0; i < 25; ++i) {
    write_leds(i, LOW);
  }

  // test all leds in sequence
  for (int i = 0; i < 2; ++i) {
    test_leds_single();
  }

  // connect to WIFI
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    for (int i = 0; i < 5; ++i) {
      write_leds(ES, HIGH);
      delay(50);
      write_leds(ES, LOW);
      delay(50);
    }
  }
  // init ntp client
  timeClient.begin();
  Serial.println("Setup successful");

  // blink 5 times slowly to indicate successful setup
  for (int i = 0; i < 5; ++i) {
    write_leds(ES, HIGH);
    delay(100);
    write_leds(ES, LOW);
    delay(100);
  }
}

const unsigned long ntp_update_interval_ms = 20 * 1000;

class Time
{
public:
  explicit Time(const NTPClient& tc)
    : m_ms_since_start_of_day(1000 * (60 * (60 * tc.getHours() + tc.getMinutes()) + tc.getSeconds()))
  {
  }

  void add_millis(int ms)
  {
    m_ms_since_start_of_day += ms;
  }

  int hours() const
  {
    return (m_ms_since_start_of_day / (1000 * 60 * 60)) % 12;
  }

  int minutes() const
  {
    return (m_ms_since_start_of_day / (1000 * 60)) % 60;
  }

  int seconds() const
  {
    return (m_ms_since_start_of_day / 1000) % 60;
  }

  void print(const char* msg) const
  {
    Serial.printf("Time (%s): %02i:%02i:%02i\n", msg, hours(), minutes(), seconds());
  }

private:
  int m_ms_since_start_of_day;
};

void loop()
{
  auto currentMillis = millis();
  const auto last_ntp_update = currentMillis;
  if (currentMillis + (2 * last_ntp_update) < currentMillis) {
    // peril of overflow, happens after ~49 days
    ESP.reset();
  }

  Serial.println("update ntp time");
  timeClient.update();
  const Time time_at_last_ntp_update(timeClient);

  while (currentMillis - last_ntp_update < ntp_update_interval_ms) {
    time_at_last_ntp_update.print("ntp");
    Time current_time = time_at_last_ntp_update;
    current_time.add_millis(currentMillis - last_ntp_update);
    Serial.printf("millis since last update: %ul\n", currentMillis - last_ntp_update);
    const auto picture = picture_from_clock(current_time.hours(), current_time.minutes());
    current_time.print("current");
    static constexpr auto n = 100; // bigger means less flicker, but watchdog will reset if too large.
    for (int k = 0; k < n; ++k) {
      set_picture(picture);
    }
    yield();
    currentMillis = millis();
  }
}
