#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "local_config.h"
#include "rgb_lcd_port.h"
#include "gui_paint.h"
#include "gt911.h"

extern DEV_I2C_Port handle;
WebServer server(80);
String devices;
void *frontBuffer=nullptr, *backBuffer=nullptr;
bool detail=false, pressed=false;
uint32_t lastDraw=0;

// This diagnostic only probes addresses. It does not configure INA registers,
// assume shunt values, select unknown mux channels, or switch external loads.
void scanBus() {
  devices="";
  for (uint8_t a=8;a<120;a++) {
    if (i2c_master_probe(handle.bus,a,15)==ESP_OK) {
      char s[8]; snprintf(s,sizeof(s),"0x%02X ",a); devices+=s;
    }
  }
  Serial.println("I2C: "+devices);
}
String statusJson() {
  return "{\"mode\":\"safe-diagnostic\",\"uptime_s\":"+String(millis()/1000)+
    ",\"i2c\":\""+devices+"\",\"battery_v\":null,\"led_v\":null,\"current_a\":null,\"power_w\":null,\"calibrated\":false}";
}
void textAt(int x,int y,const String &s,uint16_t color=WHITE) {
  Paint_DrawString_EN(x,y,s.c_str(),&Font24,color,0x0841);
}
void draw() {
  Paint_SelectImage((uint8_t*)backBuffer);
  Paint_Clear(0x0841);
  textAt(40,35,"SOLAR / ESP32-S3 7B",0x07EF);
  textAt(40,90,"PRUEBA SEGURA - SIN CONTROL DE CARGAS");
  textAt(40,160,"BATERIA INA228: -- V / -- A / -- W");
  textAt(40,220,"LEDs    INA226: -- V / -- A / -- W");
  textAt(40,285,"Medidas pendientes: verificar sensores y shunts",0xFFE0);
  textAt(40,350,detail ? "I2C SDA=8 SCL=9: "+devices : "Wi-Fi: SolarMonitor-7B  /  clave configurada");
  textAt(40,410,"Web: http://192.168.4.1");
  textAt(40,475,"TOCA PARA CAMBIAR VISTA / Uptime "+String(millis()/1000)+" s");
  textAt(40,535,"Sin datos simulados. Hub/multiplexor pendiente.",0x07EF);
  wavesahre_rgb_lcd_display((uint8_t*)backBuffer);
  std::swap(frontBuffer,backBuffer);
}
void setup() {
  Serial.begin(115200); delay(1000);
  Serial.printf("SOLAR_SAFE_7B boot flash=%u psram=%u\n",ESP.getFlashChipSize(),ESP.getPsramSize());
  touch_gt911_init();
  waveshare_esp32_s3_rgb_lcd_init();
  wavesahre_rgb_lcd_bl_on();
  waveshare_get_frame_buffer(&frontBuffer,&backBuffer);
  if (!frontBuffer || !backBuffer) {Serial.println("FRAMEBUFFER FAILED"); return;}
  Paint_NewImage((uint8_t*)backBuffer,1024,600,0,WHITE); Paint_SetScale(65);
  scanBus();
  WiFi.mode(WIFI_AP);
  bool ap=WiFi.softAP("SolarMonitor-7B",SOLAR_AP_PASSWORD);
  server.on("/api/status",[]{server.send(200,"application/json",statusJson());});
  server.on("/",[]{server.send(200,"text/html; charset=utf-8",R"HTML(<!doctype html><meta name="viewport" content="width=device-width"><title>Monitor solar</title><style>body{background:#101e29;color:#eaf5ff;font:22px system-ui;max-width:850px;margin:40px auto;padding:20px}article{background:#203447;padding:22px;margin:20px 0;border-radius:16px}small{color:#77ddd1}</style><h1>Monitor solar · Prueba segura</h1><article>Batería / INA228<br><b>-- V · -- A · -- W</b></article><article>LEDs / INA226<br><b>-- V · -- A · -- W</b></article><p>Medidas pendientes: verificar módulos, direcciones, hub y shunts.</p><small id="status"></small><script>async function update(){try{let d=await(await fetch('/api/status')).json();document.getElementById('status').textContent='I²C: '+d.i2c+' | activo '+d.uptime_s+' s';}catch(e){document.getElementById('status').textContent='Sin conexión';}}update();setInterval(update,2000);</script>)HTML");});
  server.begin(); draw();
  Serial.printf("SOLAR_READY AP=%d IP=%s\n",ap,WiFi.softAPIP().toString().c_str());
}
void loop() {
  server.handleClient();
  if (backBuffer) {
    auto p=touch_gt911_read_point(1);
    if(p.cnt && !pressed){ detail=!detail; Serial.printf("TOUCH %u,%u\n",p.x[0],p.y[0]); draw(); }
    pressed=p.cnt>0;
    if(millis()-lastDraw>2000){lastDraw=millis();draw(); Serial.println(statusJson());}
  }
  delay(20);
}
