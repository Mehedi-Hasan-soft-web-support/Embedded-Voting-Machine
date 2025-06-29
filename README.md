# Embedded-Voting-Machine with Coupon Authentication and ThingSpeak Cloud Integration

This project implements a microcontroller-based Electronic Voting Machine (EVM) using an ESP32, I2C LCD, 4x4 keypad, and ThingSpeak for cloud-based vote tracking. The voting system supports unique coupon-based voter authentication to prevent duplicate voting.
https://mehedi-hasan-soft-web-support.github.io/Embedded-Voting-Machine-with-Coupon-Authentication-and-ThingSpeak-Cloud-Integration/ 

## 🔧 Features

- Coupon code-based secure voting
- 3-tier voting (President, General Secretary, Vice President)
- Admin mode to view results and sync data
- Automatic reset after timeout
- Real-time cloud sync to ThingSpeak
- Voter input via 4x4 Matrix Keypad
- User feedback via 20x4 I2C LCD display

---

## 📟 Hardware Components

| Component                  | Quantity | Description                                          |
|---------------------------|----------|------------------------------------------------------|
| ESP32 Development Board   | 1        | Main controller with WiFi capability                 |
| 20x4 I2C LCD Display       | 1        | Displays instructions, candidates, and results       |
| 4x4 Matrix Keypad         | 1        | For user input and coupon code entry                |
| Jumper Wires              | Several  | For connections                                      |
| Breadboard (optional)     | 1        | For prototyping                                      |
| USB Cable                 | 1        | For ESP32 programming and power                      |

---

## 🔌 Circuit Diagram (Table View)

| ESP32 GPIO Pin | Connected To     | Component          |
|----------------|------------------|--------------------|
| GPIO 13        | Row 1            | Keypad             |
| GPIO 12        | Row 2            | Keypad             |
| GPIO 14        | Row 3            | Keypad             |
| GPIO 27        | Row 4            | Keypad             |
| GPIO 26        | Col 1            | Keypad             |
| GPIO 25        | Col 2            | Keypad             |
| GPIO 33        | Col 3            | Keypad             |
| GPIO 32        | Col 4            | Keypad             |
| GPIO 21 (SDA)  | SDA              | I2C LCD            |
| GPIO 22 (SCL)  | SCL              | I2C LCD            |
| 3.3V           | VCC              | I2C LCD & Keypad   |
| GND            | GND              | I2C LCD & Keypad   |

---

## 🚦 Voting Flow

1. **Welcome Screen:** Voter is prompted to press `1` to vote or `*` to enter admin mode.
2. **Coupon Entry:** Voter inputs a unique code and confirms with `#`.
3. **Voting Stages:** Voter votes for:
   - President
   - General Secretary
   - Vice President
4. **Confirmation:** Voter reviews and confirms vote.
5. **Cloud Sync:** Votes are sent to ThingSpeak channel.

---

## 🔑 Admin Mode

Accessed by pressing `*` on the welcome screen.

- Press `1` to display local results
- Press `2` to manually sync data with ThingSpeak
- Press `#` to exit admin mode

---

## ☁️ ThingSpeak Configuration

- Create a channel with at least 6 fields (one per candidate).
- Replace your **API Key** and **Channel ID** in the code.

```cpp
const char* server = "api.thingspeak.com";
String writeAPIKey = "YOUR_API_KEY_HERE";
unsigned long channelID = YOUR_CHANNEL_ID;
