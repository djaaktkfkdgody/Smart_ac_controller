#include <WiFi.h>
#include <HTTPClient.h>
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include <set>

// 🔹 네트워크 설정
char ssid[]     = "spreatics_eungam_cctv";   // WiFi ID
char password[] = "spreatics*";              // WiFi PW

// 🔹 서버 엔드포인트
const char* serverUrl = "http://3.34.112.5:5000/ardu_serv/detections";

std::set<String> currentScanDevices;

void btScanCallback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
  if (event == ESP_BT_GAP_DISC_RES_EVT) {
    char addr[18];
    sprintf(addr, "%02x:%02x:%02x:%02x:%02x:%02x",
            param->disc_res.bda[0], param->disc_res.bda[1], param->disc_res.bda[2],
            param->disc_res.bda[3], param->disc_res.bda[4], param->disc_res.bda[5]);

    String mac = String(addr);

    // 이번 스캔 내에서 중복 제거
    if (currentScanDevices.find(mac) != currentScanDevices.end()) {
      return;
    }
    currentScanDevices.insert(mac);

    int8_t rssiValue = 0;
    for (int i = 0; i < param->disc_res.num_prop; i++) {
      if (param->disc_res.prop[i].type == ESP_BT_GAP_DEV_PROP_RSSI) {
        rssiValue = *(int8_t*)param->disc_res.prop[i].val;
      }
    }

    // JSON 데이터 생성 (ble_address + ble_rssi)
    String json = "{";
    json += "\"ble_address\":\"" + mac + "\",";       // ✅ MAC 주소
    json += "\"ble_rssi\":" + String(rssiValue);     // ✅ RSSI
    json += "}";

    // 서버로 POST 요청
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(serverUrl);
      http.addHeader("Content-Type", "application/json");

      int httpResponseCode = http.POST(json);
      if (httpResponseCode > 0) {
        String response = http.getString();  // 🔹 서버 응답 읽기
        Serial.printf("📡 Sent to server: %s (code %d)\n", json.c_str(), httpResponseCode);
        Serial.printf("📩 Response: %s\n", response.c_str()); // 🔹 응답 출력
      } else {
        Serial.printf("❌ Error sending: %s\n", http.errorToString(httpResponseCode).c_str());
      }
      http.end();
    }
  }
  else if (event == ESP_BT_GAP_DISC_STATE_CHANGED_EVT) {
    if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STARTED) {
      Serial.println("Classic BT Scan Started");
      currentScanDevices.clear();
    } else if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
      Serial.println("Classic BT Scan Stopped");
      esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // WiFi 연결
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected");

  // 블루투스 초기화
  if (!btStart()) return;
  if (esp_bluedroid_init() != ESP_OK) return;
  if (esp_bluedroid_enable() != ESP_OK) return;

  esp_bt_gap_register_callback(btScanCallback);
  esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
}

void loop() {
  // 이벤트 기반
}
