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
void *frameBuffer=nullptr;
void *drawBuffer=nullptr;
bool detail=false, pressed=false;

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
void textAt(uint16_t x,uint16_t y,const String &s,sFONT *font,uint16_t color,uint16_t background) {
  Paint_DrawString_EN(x,y,s.c_str(),font,color,background);
}
void filledRect(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint16_t color) {
  Paint_DrawRectangle(x,y,x+width,y+height,color,DOT_PIXEL_1X1,DRAW_FILL_FULL);
}
void drawMetricCard(uint16_t x,const String &title,const String &sensor,uint16_t accent) {
  const uint16_t panel=0x10A2;
  const uint16_t muted=0x9E7C;
  filledRect(x,126,452,236,panel);
  filledRect(x,126,6,236,accent);
  textAt(x+24,145,title,&Font24,WHITE,panel);
  filledRect(x+304,143,124,30,0x2128);
  textAt(x+315,151,sensor,&Font12,muted,0x2128);
  filledRect(x+24,186,404,1,0x2946);
  textAt(x+24,200,"N/D",&Font48,WHITE,panel);
  textAt(x+205,225,"LECTURA NO",&Font16,muted,panel);
  textAt(x+205,247,"DISPONIBLE",&Font16,muted,panel);
  filledRect(x+24,290,404,1,0x2946);
  Paint_DrawCircle(x+32,326,5,accent,DOT_PIXEL_1X1,DRAW_FILL_FULL);
  textAt(x+48,315,"SENSOR PENDIENTE DE VALIDACION",&Font12,accent,panel);
}
void draw() {
  Paint_SelectImage((uint8_t*)drawBuffer);
  const uint16_t background=0x0861;
  const uint16_t panel=0x10A2;
  const uint16_t muted=0x9E7C;
  Paint_Clear(background);

  filledRect(0,0,1024,5,0x07FF);
  textAt(40,21,"SOLAR MONITOR",&Font24,WHITE,background);
  textAt(40,54,"PANEL DE ENERGIA  /  ESP32-S3 7B",&Font16,muted,background);
  filledRect(790,24,194,42,panel);
  Paint_DrawCircle(812,45,6,0x07E0,DOT_PIXEL_1X1,DRAW_FILL_FULL);
  textAt(830,33,"EN LINEA",&Font16,0x07E0,panel);

  drawMetricCard(40,"BATERIA","INA228",0x07FF);
  drawMetricCard(532,"SALIDA LED","INA226",0xFD20);

  filledRect(40,385,944,122,panel);
  textAt(62,399,detail ? "DIAGNOSTICO I2C" : "SISTEMA EN MODO SEGURO",&Font20,WHITE,panel);
  if (detail) {
    textAt(62,431,"BUS I2C  /  SDA 8  -  SCL 9",&Font16,muted,panel);
    String addressList=devices.length() ? devices.substring(0,54) : "No se detectaron dispositivos";
    if (devices.length()>54) addressList+="...";
    textAt(62,459,addressList,&Font16,0x07FF,panel);
  } else {
    textAt(62,431,"LECTURAS Y CONTROL DE CARGAS DESACTIVADOS",&Font16,muted,panel);
    textAt(62,459,"Wi-Fi SolarMonitor-7B  /  http://192.168.4.1",&Font16,0x07FF,panel);
  }
  textAt(40,535,"TOCA LA PANTALLA PARA CAMBIAR DE VISTA",&Font12,muted,background);
  textAt(694,535,"DIAGNOSTICO  /  SOLO LECTURA",&Font12,muted,background);
  wavesahre_rgb_lcd_display((uint8_t*)drawBuffer);
}
void setup() {
  Serial.begin(115200); delay(1000);
  Serial.printf("SOLAR_SAFE_7B boot flash=%u psram=%u\n",ESP.getFlashChipSize(),ESP.getPsramSize());
  touch_gt911_init();
  waveshare_esp32_s3_rgb_lcd_init();
  waveshare_get_frame_buffer(&frameBuffer);
  if (!frameBuffer) {Serial.println("FRAMEBUFFER FAILED"); return;}
  drawBuffer=heap_caps_malloc(EXAMPLE_LCD_H_RES*EXAMPLE_LCD_V_RES*sizeof(uint16_t),MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
  if (!drawBuffer) {Serial.println("DRAW BUFFER ALLOCATION FAILED"); return;}
  Paint_NewImage((uint8_t*)drawBuffer,1024,600,0,WHITE); Paint_SetScale(65);
  draw();
  wavesahre_rgb_lcd_bl_on();
  scanBus();
  WiFi.mode(WIFI_AP);
  bool ap=WiFi.softAP("SolarMonitor-7B",SOLAR_AP_PASSWORD);
  server.on("/api/status",[]{server.send(200,"application/json",statusJson());});
  server.on("/",[]{server.send(200,"text/html; charset=utf-8",R"HTML(<!doctype html>
<html lang="es">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta name="theme-color" content="#081321">
  <title>Solar Monitor</title>
  <style>
    :root{color-scheme:dark;--bg:#081321;--panel:#111f30;--line:#24354a;--text:#edf5ff;--muted:#91a3b8;--cyan:#57d6e8;--amber:#ffbd69;--green:#66d6a1}
    *{box-sizing:border-box}
    body{margin:0;background:radial-gradient(ellipse at 12% 0%,#123049 0,transparent 42%),var(--bg);color:var(--text);font:16px/1.5 system-ui,-apple-system,"Segoe UI",sans-serif}
    main{width:min(1040px,100% - 40px);margin:0 auto;padding:40px 0 48px}
    header{display:flex;align-items:center;justify-content:space-between;gap:20px;margin-bottom:32px}
    .eyebrow,.label{font-size:.76rem;font-weight:700;letter-spacing:.12em;text-transform:uppercase;color:var(--muted)}
    h1{font-size:clamp(1.8rem,4vw,2.7rem);letter-spacing:-.04em;line-height:1.1;margin:7px 0}
    .subtitle{color:var(--muted);margin:0}
    .status{display:flex;align-items:center;gap:9px;border:1px solid var(--line);border-radius:999px;padding:9px 14px;color:var(--green);font-size:.85rem;white-space:nowrap}
    .dot{width:8px;height:8px;border-radius:50%;background:currentColor;box-shadow:0 0 12px currentColor}
    .grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:18px}
    article,.diagnostics{background:linear-gradient(145deg,#142438,var(--panel));border:1px solid var(--line);border-radius:20px}
    article{position:relative;overflow:hidden;padding:25px 27px;min-height:250px}
    article:before{content:"";position:absolute;inset:0 auto 0 0;width:4px;background:var(--accent)}
    .card-head{display:flex;justify-content:space-between;gap:10px;align-items:center}
    .chip{border:1px solid var(--line);border-radius:999px;padding:5px 10px;color:var(--muted);font-size:.72rem}
    .measure{font-size:clamp(2.8rem,7vw,4.5rem);font-weight:650;letter-spacing:-.06em;line-height:1.15;margin:24px 0 18px}
    .units{color:var(--muted);font-size:1rem;letter-spacing:0}
    .metrics{display:flex;gap:30px;color:var(--muted)}
    .metrics b{display:block;color:var(--text);font-size:1.15rem;font-weight:600}
    .notice{display:flex;gap:14px;padding:18px 20px;margin:20px 0;border:1px solid #58482e;border-radius:16px;background:#282317;color:#f5d6a2}
    .notice strong{display:block;color:#ffe2ae}
    .notice p{margin:3px 0 0;font-size:.9rem}
    .diagnostics{padding:22px 25px}
    .diag-head{display:flex;justify-content:space-between;align-items:baseline;gap:16px;margin-bottom:12px}
    h2{font-size:1.05rem;margin:0}
    .diag-row{display:flex;justify-content:space-between;gap:18px;padding:10px 0;border-top:1px solid var(--line);color:var(--muted)}
    .diag-row b{color:var(--text);font-weight:500;text-align:right;overflow-wrap:anywhere}
    footer{margin-top:18px;color:var(--muted);font-size:.82rem}
    @media(max-width:650px){main{width:min(100% - 28px,500px);padding-top:26px}header{align-items:flex-start;flex-direction:column;margin-bottom:24px}.grid{grid-template-columns:1fr}article{min-height:220px;padding:22px}.measure{margin:18px 0}.diagnostics{padding:20px}.diag-row{flex-direction:column;gap:2px}.diag-row b{text-align:left}}
  </style>
</head>
<body>
  <main>
    <header>
      <div><div class="eyebrow">Solar Monitor <span aria-hidden="true">/</span> ESP32-S3 7B</div><h1>Panel de energía</h1><p class="subtitle">Estado del sistema y diagnóstico de sensores</p></div>
      <div class="status" id="connection"><span class="dot"></span><span id="connectionText">Conectando...</span></div>
    </header>
    <section class="grid" aria-label="Medidores">
      <article style="--accent:var(--cyan)">
        <div class="card-head"><span class="label">Batería</span><span class="chip">INA228</span></div>
        <div class="measure">--.- <span class="units">V</span></div>
        <div class="metrics"><div><span class="label">Corriente</span><b>--.- A</b></div><div><span class="label">Potencia</span><b>-- W</b></div></div>
      </article>
      <article style="--accent:var(--amber)">
        <div class="card-head"><span class="label">Salida LED</span><span class="chip">INA226</span></div>
        <div class="measure">--.- <span class="units">V</span></div>
        <div class="metrics"><div><span class="label">Corriente</span><b>--.- A</b></div><div><span class="label">Potencia</span><b>-- W</b></div></div>
      </article>
    </section>
    <aside class="notice"><span aria-hidden="true">!</span><div><strong>Lecturas deshabilitadas</strong><p>Los sensores, el hub y los shunts deben verificarse antes de mostrar medidas. No se generan datos simulados ni se controlan cargas.</p></div></aside>
    <section class="diagnostics" aria-labelledby="diagTitle">
      <div class="diag-head"><h2 id="diagTitle">Diagnóstico del dispositivo</h2><span class="label">Solo lectura</span></div>
      <div class="diag-row"><span>Red local</span><b>SolarMonitor-7B</b></div>
      <div class="diag-row"><span>Tiempo activo</span><b id="uptime">--</b></div>
      <div class="diag-row"><span>Bus I2C detectado</span><b id="i2c">Consultando...</b></div>
      <div class="diag-row"><span>Calibración</span><b>No verificada</b></div>
    </section>
    <footer>Conéctate a la red del dispositivo para consultar este panel. Las medidas permanecerán vacías hasta validar el hardware.</footer>
  </main>
  <script>
    const connection=document.getElementById('connection');
    async function updateStatus(){
      try{
        const response=await fetch('/api/status',{cache:'no-store'});
        if(!response.ok)throw new Error('HTTP '+response.status);
        const data=await response.json();
        document.getElementById('uptime').textContent=data.uptime_s+' s';
        document.getElementById('i2c').textContent=data.i2c||'Sin dispositivos detectados';
        document.getElementById('connectionText').textContent='Dispositivo conectado';
        connection.style.color='var(--green)';
      }catch(error){
        document.getElementById('connectionText').textContent='Sin conexión';
        connection.style.color='var(--amber)';
        document.getElementById('i2c').textContent='No disponible';
      }
    }
    updateStatus();
    setInterval(updateStatus,3000);
  </script>
</body>
</html>)HTML");});
  server.begin();
  Serial.printf("SOLAR_READY AP=%d IP=%s\n",ap,WiFi.softAPIP().toString().c_str());
}
void loop() {
  server.handleClient();
  if (drawBuffer) {
    auto p=touch_gt911_read_point(1);
    if(p.cnt && !pressed){ detail=!detail; Serial.printf("TOUCH %u,%u\n",p.x[0],p.y[0]); draw(); }
    pressed=p.cnt>0;
  }
  delay(20);
}
