#include "Arduino.h"
#include "heltec.h"
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_VCNL4010.h>
#include <Servo.h>
#include <math.h>

const char* ssid     = "Laser Head Control";
const char* password = "tahoewoodmaps";

WiFiServer server(80);

String header;

String ipAddress = "";
bool apSetUp = false;

TwoWire wire = TwoWire(1);
Adafruit_VCNL4010 vcnl;
Servo servo;

int minimumMicroseconds = 1700;
int maximumMicroseconds = 2200;
int startMicroseconds = 1900;
float targetDistance = 10;
float microsecondsPerMillimetre = -1;
float acceleration = 2;
bool movementEnabled = true;
float currentMicroseconds = float(startMicroseconds);
float currentDistance = 0;
float millimetreCalculationSlope = -0.5;
float millimetreCalculationCross = 2.45;
float millimetreCalculationPower = 2.4;

void setMessage(String message)
{
  Heltec.display->clear();
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(0, 0, message);
  if (ipAddress.length() > 0) {
    Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
    Heltec.display->setFont(ArialMT_Plain_10);
    Heltec.display->drawString(0, 32, "IP: " + ipAddress);
  }
  if (apSetUp) {
    Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
    Heltec.display->setFont(ArialMT_Plain_10);
    Heltec.display->drawString(0, 12, "AP: " + String(ssid));
    Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
    Heltec.display->setFont(ArialMT_Plain_10);
    Heltec.display->drawString(0, 22, "Password: " + String(password));
  }
  if (currentDistance > 0) {
    Heltec.display->setTextAlignment(TEXT_ALIGN_RIGHT);
    Heltec.display->setFont(ArialMT_Plain_24);
    Heltec.display->drawString(129, 42, String(currentDistance) + "mm");
  }
  Heltec.display->display();
}

void setup() {
  Heltec.begin(true, false, false);
  //Heltec.display->flipScreenVertically();
  setMessage("Setting up WiFi AP...");

  WiFi.softAP(ssid, password);

  ipAddress = WiFi.softAPIP().toString();
  apSetUp = true;

  setMessage("WiFi AP Setup Complete");
  setMessage("Setting up web server...");

  server.begin();

  setMessage("Web server set up");

  setMessage("Initialising servo...");
  servo.attach(13);
  setMessage("Servo setup complete");
  setMessage("Initialising prox sensor...");
  wire.begin(21, 22, 100000);
  if (!vcnl.begin(0x13, &wire))
  {
    while (1)
      ;
  }
  vcnl.setFrequency(VCNL4010_31_25);
  setMessage("Prox sensor setup complete.");
}

void loop(){
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // turns the GPIOs on and off
            if (header.indexOf("GET /minimumMicroseconds") >= 0) {
              if (header.indexOf("GET /minimumMicroseconds/") >= 0) {
                String valueToSave = header.substring(header.indexOf("GET /minimumMicroseconds/") + String("GET /minimumMicroseconds/").length());
                valueToSave = valueToSave.substring(0, valueToSave.indexOf(" "));
                int valueToSaveInt = valueToSave.toInt();
                if (valueToSaveInt != 0 || valueToSave.charAt(0) == '0') {
                  minimumMicroseconds = valueToSaveInt;
                  setMessage("Set minimumMicroseconds = " + String(valueToSaveInt));
                }
              }
              client.print(String(minimumMicroseconds));
            } else if (header.indexOf("GET /maximumMicroseconds") >= 0) {
              if (header.indexOf("GET /maximumMicroseconds/") >= 0) {
                String valueToSave = header.substring(header.indexOf("GET /maximumMicroseconds/") + String("GET /maximumMicroseconds/").length());
                valueToSave = valueToSave.substring(0, valueToSave.indexOf(" "));
                int valueToSaveInt = valueToSave.toInt();
                if (valueToSaveInt != 0 || valueToSave.charAt(0) == '0') {
                  maximumMicroseconds = valueToSaveInt;
                  setMessage("Set maximumMicroseconds = " + String(valueToSaveInt));
                }
              }
              client.print(String(maximumMicroseconds));
            } else if (header.indexOf("GET /targetDistance") >= 0) {
              if (header.indexOf("GET /targetDistance/") >= 0) {
                String valueToSave = header.substring(header.indexOf("GET /targetDistance/") + String("GET /targetDistance/").length());
                valueToSave = valueToSave.substring(0, valueToSave.indexOf(" "));
                float valueToSaveFloat = valueToSave.toFloat();
                if (valueToSaveFloat != 0 || valueToSave.charAt(0) == '0') {
                  targetDistance = valueToSaveFloat;
                  setMessage("Set targetDistance = " + String(valueToSaveFloat));
                }

              }
              client.print(String(targetDistance));
            } else if (header.indexOf("GET /microsecondsPerMillimetre") >= 0) {
              if (header.indexOf("GET /microsecondsPerMillimetre/") >= 0) {
                String valueToSave = header.substring(header.indexOf("GET /microsecondsPerMillimetre/") + String("GET /microsecondsPerMillimetre/").length());
                valueToSave = valueToSave.substring(0, valueToSave.indexOf(" "));
                float valueToSaveFloat = valueToSave.toFloat();
                if (valueToSaveFloat != 0 || valueToSave.charAt(0) == '0') {
                  microsecondsPerMillimetre = valueToSaveFloat;
                  setMessage("Set microsecondsPerMillimetre = " + String(valueToSaveFloat));
                }
              }
              client.print(String(microsecondsPerMillimetre));
            } else if (header.indexOf("GET /acceleration") >= 0) {
              if (header.indexOf("GET /acceleration/") >= 0) {
                String valueToSave = header.substring(header.indexOf("GET /acceleration/") + String("GET /acceleration/").length());
                valueToSave = valueToSave.substring(0, valueToSave.indexOf(" "));
                float valueToSaveFloat = valueToSave.toFloat();
                if (valueToSaveFloat != 0 || valueToSave.charAt(0) == '0') {
                  acceleration = valueToSaveFloat;
                  setMessage("Set acceleration = " + String(valueToSaveFloat));
                }
              }
              client.print(String(acceleration));
            } else if (header.indexOf("GET /movementEnabled") >= 0) {
              if (header.indexOf("GET /movementEnabled/true") >= 0) {
                movementEnabled = true;
                setMessage("Set movementEnabled = true");
              }
              if (header.indexOf("GET /movementEnabled/false") >= 0) {
                movementEnabled = false;
                setMessage("Set movementEnabled = false");
              }
              if (movementEnabled) {
              client.print("true");
              } else {
                client.print("false");
              }
            } else if (header.indexOf("GET /millimetreCalculationSlope") >= 0) {
              if (header.indexOf("GET /millimetreCalculationSlope/") >= 0) {
                String valueToSave = header.substring(header.indexOf("GET /millimetreCalculationSlope/") + String("GET /millimetreCalculationSlope/").length());
                valueToSave = valueToSave.substring(0, valueToSave.indexOf(" "));
                float valueToSaveFloat = valueToSave.toFloat();
                if (valueToSaveFloat != 0 || valueToSave.charAt(0) == '0') {
                  millimetreCalculationSlope = valueToSaveFloat;
                  setMessage("Set millimetreCalculationSlope = " + String(valueToSaveFloat));
                }
              }
              client.print(String(millimetreCalculationSlope));
            } else if (header.indexOf("GET /millimetreCalculationCross") >= 0) {
              if (header.indexOf("GET /millimetreCalculationCross/") >= 0) {
                String valueToSave = header.substring(header.indexOf("GET /millimetreCalculationCross/") + String("GET /millimetreCalculationCross/").length());
                valueToSave = valueToSave.substring(0, valueToSave.indexOf(" "));
                float valueToSaveFloat = valueToSave.toFloat();
                if (valueToSaveFloat != 0 || valueToSave.charAt(0) == '0') {
                  millimetreCalculationCross = valueToSaveFloat;
                  setMessage("Set millimetreCalculationCross = " + String(valueToSaveFloat));
                }
              }
              client.print(String(millimetreCalculationCross));
            } else if (header.indexOf("GET /millimetreCalculationPower") >= 0) {
              if (header.indexOf("GET /millimetreCalculationPower/") >= 0) {
                String valueToSave = header.substring(header.indexOf("GET /millimetreCalculationPower/") + String("GET /millimetreCalculationPower/").length());
                valueToSave = valueToSave.substring(0, valueToSave.indexOf(" "));
                float valueToSaveFloat = valueToSave.toFloat();
                if (valueToSaveFloat != 0 || valueToSave.charAt(0) == '0') {
                  millimetreCalculationPower = valueToSaveFloat;
                  setMessage("Set millimetreCalculationPower = " + String(valueToSaveFloat));
                }
              }
              client.print(String(millimetreCalculationPower));
            } else if (header.indexOf("GET / ") >= 0) {
              client.println("<!DOCTYPE html><html class=\"no-js\" lang=\"\"> <head> <meta charset=\"utf-8\"/> <title></title> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"/> <style>html{font-family:sans-serif;-ms-text-size-adjust:100%;-webkit-text-size-adjust:100%}body{margin:0}article,aside,details,figcaption,figure,footer,header,hgroup,main,menu,nav,section,summary{display:block}audio,canvas,progress,video{display:inline-block;vertical-align:baseline}audio:not([controls]){display:none;height:0}[hidden],template{display:none}a{background-color:transparent}a:active,a:hover{outline:0}abbr[title]{border-bottom:1px dotted}b,strong{font-weight:700}dfn{font-style:italic}h1{font-size:2em;margin:.67em 0}mark{background:#ff0;color:#000}small{font-size:80%}sub,sup{font-size:75%;line-height:0;position:relative;vertical-align:baseline}sup{top:-.5em}sub{bottom:-.25em}img{border:0}svg:not(:root){overflow:hidden}figure{margin:1em 40px}hr{-webkit-box-sizing:content-box;box-sizing:content-box;height:0}pre{overflow:auto}code,kbd,pre,samp{font-family:monospace,monospace;font-size:1em}button,input,optgroup,select,textarea{color:inherit;font:inherit;margin:0}button{overflow:visible}button,select{text-transform:none}button,html input[type=button],input[type=reset],input[type=submit]{-webkit-appearance:button;cursor:pointer}button[disabled],html input[disabled]{cursor:default}button::-moz-focus-inner,input::-moz-focus-inner{border:0;padding:0}input{line-height:normal}input[type=checkbox],input[type=radio]{-webkit-box-sizing:border-box;box-sizing:border-box;padding:0}input[type=number]::-webkit-inner-spin-button,input[type=number]::-webkit-outer-spin-button{height:auto}input[type=search]{-webkit-appearance:textfield;-webkit-box-sizing:content-box;box-sizing:content-box}input[type=search]::-webkit-search-cancel-button,input[type=search]::-webkit-search-decoration{-webkit-appearance:none}fieldset{border:1px solid silver;margin:0 2px;padding:.35em .625em .75em}legend{border:0;padding:0}textarea{overflow:auto}optgroup{font-weight:700}table{border-collapse:collapse;border-spacing:0}td,th{padding:0}.hidden,[hidden]{display:none!important}.pure-img{max-width:100%;height:auto;display:block}.pure-button{display:inline-block;zoom:1;line-height:normal;white-space:nowrap;vertical-align:middle;text-align:center;cursor:pointer;-webkit-user-drag:none;-webkit-user-select:none;-moz-user-select:none;-ms-user-select:none;user-select:none;-webkit-box-sizing:border-box;box-sizing:border-box}.pure-button::-moz-focus-inner{padding:0;border:0}.pure-button-group{letter-spacing:-.31em;text-rendering:optimizespeed}.opera-only :-o-prefocus,.pure-button-group{word-spacing:-.43em}.pure-button-group .pure-button{letter-spacing:normal;word-spacing:normal;vertical-align:top;text-rendering:auto}.pure-button{font-family:inherit;font-size:100%;padding:.5em 1em;color:#444;color:rgba(0,0,0,.8);border:1px solid #999;border:none transparent;background-color:#e6e6e6;text-decoration:none;border-radius:2px}.pure-button-hover,.pure-button:focus,.pure-button:hover{background-image:-webkit-gradient(linear,left top,left bottom,from(transparent),color-stop(40%,rgba(0,0,0,.05)),to(rgba(0,0,0,.1)));background-image:-webkit-linear-gradient(transparent,rgba(0,0,0,.05) 40%,rgba(0,0,0,.1));background-image:linear-gradient(transparent,rgba(0,0,0,.05) 40%,rgba(0,0,0,.1))}.pure-button:focus{outline:0}.pure-button-active,.pure-button:active{-webkit-box-shadow:0 0 0 1px rgba(0,0,0,.15) inset,0 0 6px rgba(0,0,0,.2) inset;box-shadow:0 0 0 1px rgba(0,0,0,.15) inset,0 0 6px rgba(0,0,0,.2) inset;border-color:#000}.pure-button-disabled,.pure-button-disabled:active,.pure-button-disabled:focus,.pure-button-disabled:hover,.pure-button[disabled]{border:none;background-image:none;opacity:.4;cursor:not-allowed;-webkit-box-shadow:none;box-shadow:none;pointer-events:none}.pure-button-hidden{display:none}.pure-button-primary,.pure-button-selected,a.pure-button-primary,a.pure-button-selected{background-color:#0078e7;color:#fff}.pure-button-group .pure-button{margin:0;border-radius:0;border-right:1px solid #111;border-right:1px solid rgba(0,0,0,.2)}.pure-button-group .pure-button:first-child{border-top-left-radius:2px;border-bottom-left-radius:2px}.pure-button-group .pure-button:last-child{border-top-right-radius:2px;border-bottom-right-radius:2px;border-right:none}.pure-form input[type=color],.pure-form input[type=date],.pure-form input[type=datetime-local],.pure-form input[type=datetime],.pure-form input[type=email],.pure-form input[type=month],.pure-form input[type=number],.pure-form input[type=password],.pure-form input[type=search],.pure-form input[type=tel],.pure-form input[type=text],.pure-form input[type=time],.pure-form input[type=url],.pure-form input[type=week],.pure-form select,.pure-form textarea{padding:.5em .6em;display:inline-block;border:1px solid #ccc;-webkit-box-shadow:inset 0 1px 3px #ddd;box-shadow:inset 0 1px 3px #ddd;border-radius:4px;vertical-align:middle;-webkit-box-sizing:border-box;box-sizing:border-box}.pure-form input:not([type]){padding:.5em .6em;display:inline-block;border:1px solid #ccc;-webkit-box-shadow:inset 0 1px 3px #ddd;box-shadow:inset 0 1px 3px #ddd;border-radius:4px;-webkit-box-sizing:border-box;box-sizing:border-box}.pure-form input[type=color]{padding:.2em .5em}.pure-form input[type=color]:focus,.pure-form input[type=date]:focus,.pure-form input[type=datetime-local]:focus,.pure-form input[type=datetime]:focus,.pure-form input[type=email]:focus,.pure-form input[type=month]:focus,.pure-form input[type=number]:focus,.pure-form input[type=password]:focus,.pure-form input[type=search]:focus,.pure-form input[type=tel]:focus,.pure-form input[type=text]:focus,.pure-form input[type=time]:focus,.pure-form input[type=url]:focus,.pure-form input[type=week]:focus,.pure-form select:focus,.pure-form textarea:focus{outline:0;border-color:#129fea}.pure-form input:not([type]):focus{outline:0;border-color:#129fea}.pure-form input[type=checkbox]:focus,.pure-form input[type=file]:focus,.pure-form input[type=radio]:focus{outline:thin solid #129fea;outline:1px auto #129fea}.pure-form .pure-checkbox,.pure-form .pure-radio{margin:.5em 0;display:block}.pure-form input[type=color][disabled],.pure-form input[type=date][disabled],.pure-form input[type=datetime-local][disabled],.pure-form input[type=datetime][disabled],.pure-form input[type=email][disabled],.pure-form input[type=month][disabled],.pure-form input[type=number][disabled],.pure-form input[type=password][disabled],.pure-form input[type=search][disabled],.pure-form input[type=tel][disabled],.pure-form input[type=text][disabled],.pure-form input[type=time][disabled],.pure-form input[type=url][disabled],.pure-form input[type=week][disabled],.pure-form select[disabled],.pure-form textarea[disabled]{cursor:not-allowed;background-color:#eaeded;color:#cad2d3}.pure-form input:not([type])[disabled]{cursor:not-allowed;background-color:#eaeded;color:#cad2d3}.pure-form input[readonly],.pure-form select[readonly],.pure-form textarea[readonly]{background-color:#eee;color:#777;border-color:#ccc}.pure-form input:focus:invalid,.pure-form select:focus:invalid,.pure-form textarea:focus:invalid{color:#b94a48;border-color:#e9322d}.pure-form input[type=checkbox]:focus:invalid:focus,.pure-form input[type=file]:focus:invalid:focus,.pure-form input[type=radio]:focus:invalid:focus{outline-color:#e9322d}.pure-form select{height:2.25em;border:1px solid #ccc;background-color:#fff}.pure-form select[multiple]{height:auto}.pure-form label{margin:.5em 0 .2em}.pure-form fieldset{margin:0;padding:.35em 0 .75em;border:0}.pure-form legend{display:block;width:100%;padding:.3em 0;margin-bottom:.3em;color:#333;border-bottom:1px solid #e5e5e5}.pure-form-stacked input[type=color],.pure-form-stacked input[type=date],.pure-form-stacked input[type=datetime-local],.pure-form-stacked input[type=datetime],.pure-form-stacked input[type=email],.pure-form-stacked input[type=file],.pure-form-stacked input[type=month],.pure-form-stacked input[type=number],.pure-form-stacked input[type=password],.pure-form-stacked input[type=search],.pure-form-stacked input[type=tel],.pure-form-stacked input[type=text],.pure-form-stacked input[type=time],.pure-form-stacked input[type=url],.pure-form-stacked input[type=week],.pure-form-stacked label,.pure-form-stacked select,.pure-form-stacked textarea{display:block;margin:.25em 0}.pure-form-stacked input:not([type]){display:block;margin:.25em 0}.pure-form-aligned .pure-help-inline,.pure-form-aligned input,.pure-form-aligned select,.pure-form-aligned textarea,.pure-form-message-inline{display:inline-block;vertical-align:middle}.pure-form-aligned textarea{vertical-align:top}.pure-form-aligned .pure-control-group{margin-bottom:.5em}.pure-form-aligned .pure-control-group label{text-align:right;display:inline-block;vertical-align:middle;width:10em;margin:0 1em 0 0}.pure-form-aligned .pure-controls{margin:1.5em 0 0 11em}.pure-form .pure-input-rounded,.pure-form input.pure-input-rounded{border-radius:2em;padding:.5em 1em}.pure-form .pure-group fieldset{margin-bottom:10px}.pure-form .pure-group input,.pure-form .pure-group textarea{display:block;padding:10px;margin:0 0 -1px;border-radius:0;position:relative;top:-1px}.pure-form .pure-group input:focus,.pure-form .pure-group textarea:focus{z-index:3}.pure-form .pure-group input:first-child,.pure-form .pure-group textarea:first-child{top:1px;border-radius:4px 4px 0 0;margin:0}.pure-form .pure-group input:first-child:last-child,.pure-form .pure-group textarea:first-child:last-child{top:1px;border-radius:4px;margin:0}.pure-form .pure-group input:last-child,.pure-form .pure-group textarea:last-child{top:-2px;border-radius:0 0 4px 4px;margin:0}.pure-form .pure-group button{margin:.35em 0}.pure-form .pure-input-1{width:100%}.pure-form .pure-input-3-4{width:75%}.pure-form .pure-input-2-3{width:66%}.pure-form .pure-input-1-2{width:50%}.pure-form .pure-input-1-3{width:33%}.pure-form .pure-input-1-4{width:25%}.pure-form .pure-help-inline,.pure-form-message-inline{display:inline-block;padding-left:.3em;color:#666;vertical-align:middle;font-size:.875em}.pure-form-message{display:block;color:#666;font-size:.875em}@media only screen and (max-width :480px){.pure-form button[type=submit]{margin:.7em 0 0}.pure-form input:not([type]),.pure-form input[type=color],.pure-form input[type=date],.pure-form input[type=datetime-local],.pure-form input[type=datetime],.pure-form input[type=email],.pure-form input[type=month],.pure-form input[type=number],.pure-form input[type=password],.pure-form input[type=search],.pure-form input[type=tel],.pure-form input[type=text],.pure-form input[type=time],.pure-form input[type=url],.pure-form input[type=week],.pure-form label{margin-bottom:.3em;display:block}.pure-group input:not([type]),.pure-group input[type=color],.pure-group input[type=date],.pure-group input[type=datetime-local],.pure-group input[type=datetime],.pure-group input[type=email],.pure-group input[type=month],.pure-group input[type=number],.pure-group input[type=password],.pure-group input[type=search],.pure-group input[type=tel],.pure-group input[type=text],.pure-group input[type=time],.pure-group input[type=url],.pure-group input[type=week]{margin-bottom:0}.pure-form-aligned .pure-control-group label{margin-bottom:.3em;text-align:left;display:block;width:100%}.pure-form-aligned .pure-controls{margin:1.5em 0 0 0}.pure-form .pure-help-inline,.pure-form-message,.pure-form-message-inline{display:block;font-size:.75em;padding:.2em 0 .8em}}body{background-color: #00171F; color: #EEEEEE;}*{-webkit-box-sizing: border-box; box-sizing: border-box; text-rendering: optimizeLegibility; -webkit-font-smoothing: antialiased; -moz-osx-font-smoothing: grayscale; font-kerning: auto;}html{font-size: 12pt; line-height: 1.4; font-weight: 400; font-family: 'Helvetica Neue', 'Myriad Pro', 'Segoe UI', Myriad, Helvetica, 'Lucida Grande', 'DejaVu Sans Condensed', 'Liberation Sans', 'Nimbus Sans L', Tahoma, Geneva, Arial, sans-serif;}body{padding: 1em; margin: 0 auto; max-width: 800px;}code, pre, blockquote{padding: .2em; background: rgba(0,0,0,.1);}code, pre{font-family: Consolas, \"Andale Mono WT\", \"Andale Mono\", \"Lucida Console\", \"Lucida Sans Typewriter\", \"DejaVu Sans Mono\", \"Bitstream Vera Sans Mono\", \"Liberation Mono\", \"Nimbus Mono L\", Monaco, \"Courier New\", Courier, monospace;}h1, h2, h3, h4, h5, h6{margin: 0 0 .5em 0; line-height: 1.2; letter-spacing: -.02em;}[class*=float-]{margin: 0 auto 1em auto; display: block; width: auto; max-width: 100%; clear: both;}@media (min-width: 600px){h1{font-size: 300%;}h2{font-size: 200%;}h3{font-size: 180%;}h4{font-size: 160%;}h5{font-size: 140%;}h6{font-size: 120%;}[class*=float-]{max-width: 40%;}.float-left{float: left; margin: 0 1em .5em 0;}.float-right{float: right; margin: 0 0 .5em 1em;}}.pure-form-message{font-size: 80%; color: #BBBBBB;}.pure-form legend{color: #EEEEEE; font-size: 150%; font-weight: bold;}.pure-form fieldset.banner input{font-size: 300%; font-weight: bold;}.pure-form input{background-color: #003459;}</style> <meta name=\"theme-color\" content=\"#fafafa\"/> </head> <body><!--[if IE]> <p class=\"browserupgrade\"> You are using an <strong>outdated</strong> browser. Please <a href=\"https://browsehappy.com/\">upgrade your browser</a> to improve your experience and security. </p><![endif]--> <h1>Laser Head Control</h1> <form class=\"pure-form pure-form-stacked\"> <fieldset class=\"banner\"> <legend>Editable Parameters</legend> </fieldset> <fieldset class=\"main\"> </fieldset> </form> <script>!function(e,t,n){function r(e,t){return typeof e===t}function o(e){var t=b.className,n=Modernizr._config.classPrefix||\"\";if(S&&(t=t.baseVal),Modernizr._config.enableJSClass){var r=new RegExp(\"(^|\\\\s)\"+n+\"no-js(\\\\s|$)\");t=t.replace(r,\"$1\"+n+\"js$2\")}Modernizr._config.enableClasses&&(e.length>0&&(t+=\" \"+n+e.join(\" \"+n)),S?b.className.baseVal=t:b.className=t)}function i(e,t){if(\"object\"==typeof e)for(var n in e)E(e,n)&&i(n,e[n]);else{e=e.toLowerCase();var r=e.split(\".\"),s=Modernizr[r[0]];if(2===r.length&&(s=s[r[1]]),void 0!==s)return Modernizr;t=\"function\"==typeof t?t():t,1===r.length?Modernizr[r[0]]=t:(!Modernizr[r[0]]||Modernizr[r[0]]instanceof Boolean||(Modernizr[r[0]]=new Boolean(Modernizr[r[0]])),Modernizr[r[0]][r[1]]=t),o([(t&&!1!==t?\"\":\"no-\")+r.join(\"-\")]),Modernizr._trigger(e,t)}return Modernizr}function s(){return\"function\"!=typeof t.createElement?t.createElement(arguments[0]):S?t.createElementNS.call(t,\"http://www.w3.org/2000/svg\",arguments[0]):t.createElement.apply(t,arguments)}function a(){var e=t.body;return e||(e=s(S?\"svg\":\"body\"),e.fake=!0),e}function l(e,n,r,o){var i,l,u,f,c=\"modernizr\",d=s(\"div\"),p=a();if(parseInt(r,10))for(;r--;)u=s(\"div\"),u.id=o?o[r]:c+(r+1),d.appendChild(u);return i=s(\"style\"),i.type=\"text/css\",i.id=\"s\"+c,(p.fake?p:d).appendChild(i),p.appendChild(d),i.styleSheet?i.styleSheet.cssText=e:i.appendChild(t.createTextNode(e)),d.id=c,p.fake&&(p.style.background=\"\",p.style.overflow=\"hidden\",f=b.style.overflow,b.style.overflow=\"hidden\",b.appendChild(p)),l=n(d,e),p.fake?(p.parentNode.removeChild(p),b.style.overflow=f,b.offsetHeight):d.parentNode.removeChild(d),!!l}function u(e,t){return!!~(\"\"+e).indexOf(t)}function f(e){return e.replace(/([A-Z])/g,function(e,t){return\"-\"+t.toLowerCase()}).replace(/^ms-/,\"-ms-\")}function c(t,n,r){var o;if(\"getComputedStyle\"in e){o=getComputedStyle.call(e,t,n);var i=e.console;if(null!==o)r&&(o=o.getPropertyValue(r));else if(i){var s=i.error?\"error\":\"log\";i[s].call(i,\"getComputedStyle returning null, its possible modernizr test results are inaccurate\")}}else o=!n&&t.currentStyle&&t.currentStyle[r];return o}function d(t,r){var o=t.length;if(\"CSS\"in e&&\"supports\"in e.CSS){for(;o--;)if(e.CSS.supports(f(t[o]),r))return!0;return!1}if(\"CSSSupportsRule\"in e){for(var i=[];o--;)i.push(\"(\"+f(t[o])+\":\"+r+\")\");return i=i.join(\" or \"),l(\"@supports (\"+i+\"){#modernizr{position: absolute;}}\",function(e){return\"absolute\"===c(e,null,\"position\")})}return n}function p(e){return e.replace(/([a-z])-([a-z])/g,function(e,t,n){return t+n.toUpperCase()}).replace(/^-/,\"\")}function m(e,t,o,i){function a(){f&&(delete L.style,delete L.modElem)}if(i=!r(i,\"undefined\")&&i,!r(o,\"undefined\")){var l=d(e,o);if(!r(l,\"undefined\"))return l}for(var f,c,m,h,v,A=[\"modernizr\",\"tspan\",\"samp\"];!L.style&&A.length;)f=!0,L.modElem=s(A.shift()),L.style=L.modElem.style;for(m=e.length,c=0;c<m;c++)if(h=e[c],v=L.style[h],u(h,\"-\")&&(h=p(h)),L.style[h]!==n){if(i||r(o,\"undefined\"))return a(),\"pfx\"!==t||h;try{L.style[h]=o}catch(e){}if(L.style[h]!==v)return a(),\"pfx\"!==t||h}return a(),!1}function h(e,t){return function(){return e.apply(t,arguments)}}function v(e,t,n){var o;for(var i in e)if(e[i]in t)return!1===n?e[i]:(o=t[e[i]],r(o,\"function\")?h(o,n||t):o);return!1}function A(e,t,n,o,i){var s=e.charAt(0).toUpperCase()+e.slice(1),a=(e+\" \"+z.join(s+\" \")+s).split(\" \");return r(t,\"string\")||r(t,\"undefined\")?m(a,t,o,i):(a=(e+\" \"+x.join(s+\" \")+s).split(\" \"),v(a,t,n))}function g(e,t,r){return A(e,n,n,t,r)}var y=[],w={_version:\"3.7.1\",_config:{classPrefix:\"\",enableClasses:!0,enableJSClass:!0,usePrefixes:!0},_q:[],on:function(e,t){var n=this;setTimeout(function(){t(n[e])},0)},addTest:function(e,t,n){y.push({name:e,fn:t,options:n})},addAsyncTest:function(e){y.push({name:null,fn:e})}},Modernizr=function(){};Modernizr.prototype=w,Modernizr=new Modernizr;var C=[],b=t.documentElement,S=\"svg\"===b.nodeName.toLowerCase(),_=\"Moz O ms Webkit\",x=w._config.usePrefixes?_.toLowerCase().split(\" \"):[];w._domPrefixes=x;var T=w._config.usePrefixes?\" -webkit- -moz- -o- -ms- \".split(\" \"):[\"\",\"\"];w._prefixes=T;var E;!function(){var e={}.hasOwnProperty;E=r(e,\"undefined\")||r(e.call,\"undefined\")?function(e,t){return t in e&&r(e.constructor.prototype[t],\"undefined\")}:function(t,n){return e.call(t,n)}}(),w._l={},w.on=function(e,t){this._l[e]||(this._l[e]=[]),this._l[e].push(t),Modernizr.hasOwnProperty(e)&&setTimeout(function(){Modernizr._trigger(e,Modernizr[e])},0)},w._trigger=function(e,t){if(this._l[e]){var n=this._l[e];setTimeout(function(){var e;for(e=0;e<n.length;e++)(0,n[e])(t)},0),delete this._l[e]}},Modernizr._q.push(function(){w.addTest=i});var P=function(){function e(e,r){var o;return!!e&&(r&&\"string\"!=typeof r||(r=s(r||\"div\")),e=\"on\"+e,o=e in r,!o&&t&&(r.setAttribute||(r=s(\"div\")),r.setAttribute(e,\"\"),o=\"function\"==typeof r[e],r[e]!==n&&(r[e]=n),r.removeAttribute(e)),o)}var t=!(\"onblur\"in b);return e}();w.hasEvent=P;var k=function(){var t=e.matchMedia||e.msMatchMedia;return t?function(e){var n=t(e);return n&&n.matches||!1}:function(t){var n=!1;return l(\"@media \"+t+\"{#modernizr{position: absolute;}}\",function(t){n=\"absolute\"===(e.getComputedStyle?e.getComputedStyle(t,null):t.currentStyle).position}),n}}();w.mq=k;var B=function(e,t){var n=!1,r=s(\"div\"),o=r.style;if(e in o){var i=x.length;for(o[e]=t,n=o[e];i--&&!n;)o[e]=\"-\"+x[i]+\"-\"+t,n=o[e]}return\"\"===n&&(n=!1),n};w.prefixedCSSValue=B;var z=w._config.usePrefixes?_.split(\" \"):[];w._cssomPrefixes=z;var O={elem:s(\"modernizr\")};Modernizr._q.push(function(){delete O.elem});var L={style:O.elem.style};Modernizr._q.unshift(function(){delete L.style}),w.testAllProps=A,w.testAllProps=g;w.testProp=function(e,t,r){return m([e],n,t,r)},w.testStyles=l;Modernizr.addTest(\"customelements\",\"customElements\"in e),Modernizr.addTest(\"history\",function(){var t=navigator.userAgent;return(-1===t.indexOf(\"Android 2.\")&&-1===t.indexOf(\"Android 4.0\")||-1===t.indexOf(\"Mobile Safari\")||-1!==t.indexOf(\"Chrome\")||-1!==t.indexOf(\"Windows Phone\")||\"file:\"===location.protocol)&&(e.history&&\"pushState\"in e.history)}),Modernizr.addTest(\"pointerevents\",function(){var e=!1,t=x.length;for(e=Modernizr.hasEvent(\"pointerdown\");t--&&!e;)P(x[t]+\"pointerdown\")&&(e=!0);return e});var N=new Boolean(\"postMessage\"in e);N.structuredclones=!0;try{e.postMessage({toString:function(){N.structuredclones=!1}},\"*\")}catch(e){}Modernizr.addTest(\"postmessage\",N),Modernizr.addTest(\"webgl\",function(){return\"WebGLRenderingContext\"in e});var R=!1;try{R=\"WebSocket\"in e&&2===e.WebSocket.CLOSING}catch(e){}Modernizr.addTest(\"websockets\",R),Modernizr.addTest(\"cssanimations\",g(\"animationName\",\"a\",!0)),function(){Modernizr.addTest(\"csscolumns\",function(){var e=!1,t=g(\"columnCount\");try{e=!!t,e&&(e=new Boolean(e))}catch(e){}return e});for(var e,t,n=[\"Width\",\"Span\",\"Fill\",\"Gap\",\"Rule\",\"RuleColor\",\"RuleStyle\",\"RuleWidth\",\"BreakBefore\",\"BreakAfter\",\"BreakInside\"],r=0;r<n.length;r++)e=n[r].toLowerCase(),t=g(\"column\"+n[r]),\"breakbefore\"!==e&&\"breakafter\"!==e&&\"breakinside\"!==e||(t=t||g(n[r])),Modernizr.addTest(\"csscolumns.\"+e,t)}(),Modernizr.addTest(\"flexbox\",g(\"flexBasis\",\"1px\",!0)),Modernizr.addTest(\"picture\",\"HTMLPictureElement\"in e),Modernizr.addAsyncTest(function(){var e,t,n,r=s(\"img\"),o=\"sizes\"in r;!o&&\"srcset\"in r?(t=\"data:image/gif;base64,R0lGODlhAgABAPAAAP///wAAACH5BAAAAAAALAAAAAACAAEAAAICBAoAOw==\",e=\"data:image/gif;base64,R0lGODlhAQABAAAAACH5BAEKAAEALAAAAAABAAEAAAICTAEAOw==\",n=function(){i(\"sizes\",2===r.width)},r.onload=n,r.onerror=n,r.setAttribute(\"sizes\",\"9px\"),r.srcset=e+\" 1w,\"+t+\" 8w\",r.src=e):i(\"sizes\",o)}),Modernizr.addTest(\"srcset\",\"srcset\"in s(\"img\")),Modernizr.addTest(\"webworkers\",\"Worker\"in e),function(){var e,t,n,o,i,s,a;for(var l in y)if(y.hasOwnProperty(l)){if(e=[],t=y[l],t.name&&(e.push(t.name.toLowerCase()),t.options&&t.options.aliases&&t.options.aliases.length))for(n=0;n<t.options.aliases.length;n++)e.push(t.options.aliases[n].toLowerCase());for(o=r(t.fn,\"function\")?t.fn():t.fn,i=0;i<e.length;i++)s=e[i],a=s.split(\".\"),1===a.length?Modernizr[a[0]]=o:(!Modernizr[a[0]]||Modernizr[a[0]]instanceof Boolean||(Modernizr[a[0]]=new Boolean(Modernizr[a[0]])),Modernizr[a[0]][a[1]]=o),C.push((o?\"\":\"no-\")+a.join(\"-\"))}}(),o(C),delete w.addTest,delete w.addAsyncTest;for(var j=0;j<Modernizr._q.length;j++)Modernizr._q[j]();e.Modernizr=Modernizr}(window,document); (function(){var method; var noop=function (){}; var methods=[ 'assert', 'clear', 'count', 'debug', 'dir', 'dirxml', 'error', 'exception', 'group', 'groupCollapsed', 'groupEnd', 'info', 'log', 'markTimeline', 'profile', 'profileEnd', 'table', 'time', 'timeEnd', 'timeline', 'timelineEnd', 'timeStamp', 'trace', 'warn']; var length=methods.length; var console=(window.console=window.console ||{}); while (length--){method=methods[length]; if (!console[method]){console[method]=noop;}}}()); (function(a){function b(){this._callbacks=[];}b.prototype.then=function(a,c){var d;if(this._isdone)d=a.apply(c,this.result);else{d=new b();this._callbacks.push(function(){var b=a.apply(c,arguments);if(b&&typeof b.then==='function')b.then(d.done,d);});}return d;};b.prototype.done=function(){this.result=arguments;this._isdone=true;for(var a=0;a<this._callbacks.length;a++)this._callbacks[a].apply(null,arguments);this._callbacks=[];};function c(a){var c=new b();var d=[];if(!a||!a.length){c.done(d);return c;}var e=0;var f=a.length;function g(a){return function(){e+=1;d[a]=Array.prototype.slice.call(arguments);if(e===f)c.done(d);};}for(var h=0;h<f;h++)a[h].then(g(h));return c;}function d(a,c){var e=new b();if(a.length===0)e.done.apply(e,c);else a[0].apply(null,c).then(function(){a.splice(0,1);d(a,arguments).then(function(){e.done.apply(e,arguments);});});return e;}function e(a){var b=\"\";if(typeof a===\"string\")b=a;else{var c=encodeURIComponent;var d=[];for(var e in a)if(a.hasOwnProperty(e))d.push(c(e)+'='+c(a[e]));b=d.join('&');}return b;}function f(){var a;if(window.XMLHttpRequest)a=new XMLHttpRequest();else if(window.ActiveXObject)try{a=new ActiveXObject(\"Msxml2.XMLHTTP\");}catch(b){a=new ActiveXObject(\"Microsoft.XMLHTTP\");}return a;}function g(a,c,d,g){var h=new b();var j,k;d=d||{};g=g||{};try{j=f();}catch(l){h.done(i.ENOXHR,\"\");return h;}k=e(d);if(a==='GET'&&k){c+='?'+k;k=null;}j.open(a,c);var m='application/x-www-form-urlencoded';for(var n in g)if(g.hasOwnProperty(n))if(n.toLowerCase()==='content-type')m=g[n];else j.setRequestHeader(n,g[n]);j.setRequestHeader('Content-type',m);function o(){j.abort();h.done(i.ETIMEOUT,\"\",j);}var p=i.ajaxTimeout;if(p)var q=setTimeout(o,p);j.onreadystatechange=function(){if(p)clearTimeout(q);if(j.readyState===4){var a=(!j.status||(j.status<200||j.status>=300)&&j.status!==304);h.done(a,j.responseText,j);}};j.send(k);return h;}function h(a){return function(b,c,d){return g(a,b,c,d);};}var i={Promise:b,join:c,chain:d,ajax:g,get:h('GET'),post:h('POST'),put:h('PUT'),del:h('DELETE'),ENOXHR:1,ETIMEOUT:2,ajaxTimeout:0};if(typeof define==='function'&&define.amd)define(function(){return i;});else a.promise=i;})(this); !function(a){function b(b){var c=b.length,d=typeof b;return!o(d)&&b!==a&&(!(1!==b.nodeType||!c)||(p(d)||0===c||\"number\"==typeof c&&c>0&&c-1 in b))}function c(a,b){var c,d;this.originalEvent=a,d=function(a,b){\"preventDefault\"===a?this[a]=function(){return this.defaultPrevented=!0,b[a]()}:\"stopImmediatePropagation\"===a?this[a]=function(){return this.immediatePropagationStopped=!0,b[a]()}:o(b[a])?this[a]=function(){return b[a]()}:this[a]=b[a]};for(c in a)(a[c]||\"function\"==typeof a[c])&&d.call(this,c,a);q.extend(this,b,{isImmediatePropagationStopped:function(){return!!this.immediatePropagationStopped}})}var d,e=a.$,f=a.jBone,g=/^<(\\w+)\\s*\\/?>$/,h=/^(?:[^#<]*(<[\\w\\W]+>)[^>]*$|#([\\w\\-]*)$)/,i=[].slice,j=[].splice,k=Object.keys,l=a.document,m=function(a){return\"string\"==typeof a},n=function(a){return a instanceof Object},o=function(a){return\"[object Function]\"==={}.toString.call(a)},p=function(a){return Array.isArray(a)},q=function(a,b){return new d.init(a,b)};q.noConflict=function(){return a.$=e,a.jBone=f,q},d=q.fn=q.prototype={init:function(a,b){var c,d,e,f;if(!a)return this;if(m(a)){if(d=g.exec(a))return this[0]=l.createElement(d[1]),this.length=1,n(b)&&this.attr(b),this;if((d=h.exec(a))&&d[1]){for(f=l.createDocumentFragment(),e=l.createElement(\"div\"),e.innerHTML=a;e.lastChild;)f.appendChild(e.firstChild);return c=i.call(f.childNodes),q.merge(this,c)}if(q.isElement(b))return q(b).find(a);try{return c=l.querySelectorAll(a),q.merge(this,c)}catch(a){return this}}return a.nodeType?(this[0]=a,this.length=1,this):o(a)?a():a instanceof q?a:q.makeArray(a,this)},pop:[].pop,push:[].push,reverse:[].reverse,shift:[].shift,sort:[].sort,splice:[].splice,slice:[].slice,indexOf:[].indexOf,forEach:[].forEach,unshift:[].unshift,concat:[].concat,join:[].join,every:[].every,some:[].some,filter:[].filter,map:[].map,reduce:[].reduce,reduceRight:[].reduceRight,length:0},d.constructor=q,d.init.prototype=d,q.setId=function(b){var c=b.jid;b===a?c=\"window\":void 0===b.jid&&(b.jid=c=++q._cache.jid),q._cache.events[c]||(q._cache.events[c]={})},q.getData=function(b){b=b instanceof q?b[0]:b;var c=b===a?\"window\":b.jid;return{jid:c,events:q._cache.events[c]}},q.isElement=function(a){return a&&a instanceof q||a instanceof HTMLElement||m(a)},q._cache={events:{},jid:0},d.pushStack=function(a){return q.merge(this.constructor(),a)},q.merge=function(a,b){for(var c=b.length,d=a.length,e=0;e<c;)a[d++]=b[e++];return a.length=d,a},q.contains=function(a,b){return a.contains(b)},q.extend=function(a){var b;return j.call(arguments,1).forEach(function(c){if(b=a,c)for(var d in c)b[d]=c[d]}),a},q.makeArray=function(a,c){var d=c||[];return null!==a&&(b(a)?q.merge(d,m(a)?[a]:a):d.push(a)),d},q.unique=function(a){if(null==a)return[];for(var b=[],c=0,d=a.length;c<d;c++){var e=a[c];b.indexOf(e)<0&&b.push(e)}return b},q.Event=function(a,b){var c,d;return a.type&&!b&&(b=a,a=a.type),c=a.split(\".\").splice(1).join(\".\"),d=a.split(\".\")[0],a=l.createEvent(\"Event\"),a.initEvent(d,!0,!0),q.extend(a,{namespace:c,isDefaultPrevented:function(){return a.defaultPrevented}},b)},q.event={add:function(a,b,c,d,e){q.setId(a);var f,g,h,i=function(b){q.event.dispatch.call(a,b)},j=q.getData(a).events;for(b=b.split(\" \"),g=b.length;g--;)h=b[g],f=h.split(\".\")[0],j[f]=j[f]||[],j[f].length?i=j[f][0].fn:a.addEventListener&&a.addEventListener(f,i,!1),j[f].push({namespace:h.split(\".\").splice(1).join(\".\"),fn:i,selector:e,data:d,originfn:c})},remove:function(a,b,c,d){var e,f,g=function(a,b,d,e,f){var g;(c&&f.originfn===c||!c)&&(g=f.fn),a[b][d].fn===g&&(a[b].splice(d,1),a[b].length||e.removeEventListener(b,g))},h=q.getData(a).events;if(h)return!b&&h?k(h).forEach(function(b){for(f=h[b],e=f.length;e--;)g(h,b,e,a,f[e])}):void b.split(\" \").forEach(function(b){var c,i=b.split(\".\")[0],j=b.split(\".\").splice(1).join(\".\");if(h[i])for(f=h[i],e=f.length;e--;)c=f[e],(!j||j&&c.namespace===j)&&(!d||d&&c.selector===d)&&g(h,i,e,a,c);else j&&k(h).forEach(function(b){for(f=h[b],e=f.length;e--;)c=f[e],c.namespace.split(\".\")[0]===j.split(\".\")[0]&&g(h,b,e,a,c)})})},trigger:function(a,b){var c=[];m(b)?c=b.split(\" \").map(function(a){return q.Event(a)}):(b=b instanceof Event?b:q.Event(b),c=[b]),c.forEach(function(b){b.type&&a.dispatchEvent&&a.dispatchEvent(b)})},dispatch:function(a){for(var b,d,e,f,g,h=0,i=0,j=this,k=q.getData(j).events[a.type],l=k.length,m=[],n=[];h<l;h++)m.push(k[h]);for(h=0,l=m.length;h<l&&~k.indexOf(m[h])&&(!f||!f.isImmediatePropagationStopped());h++)if(d=null,g={},e=m[h],e.data&&(g.data=e.data),e.selector){if(~(n=q(j).find(e.selector)).indexOf(a.target)&&(d=a.target)||j!==a.target&&j.contains(a.target)){if(!d)for(b=n.length,i=0;i<b;i++)n[i]&&n[i].contains(a.target)&&(d=n[i]);if(!d)continue;g.currentTarget=d,f=new c(a,g),a.namespace&&a.namespace!==e.namespace||e.originfn.call(d,f)}}else f=new c(a,g),a.namespace&&a.namespace!==e.namespace||e.originfn.call(j,f)}},d.on=function(a,b,c,d){var e=this.length,f=0;if(null==c&&null==d?(d=b,c=b=void 0):null==d&&(\"string\"==typeof b?(d=c,c=void 0):(d=c,c=b,b=void 0)),!d)return this;for(;f<e;f++)q.event.add(this[f],a,d,c,b);return this},d.one=function(a){var b,c=arguments,d=0,e=this.length,f=i.call(c,1,c.length-1),g=i.call(c,-1)[0];for(b=function(b){var c=q(b);a.split(\" \").forEach(function(a){var d=function(e){c.off(a,d),g.call(b,e)};c.on.apply(c,[a].concat(f,d))})};d<e;d++)b(this[d]);return this},d.trigger=function(a){var b=0,c=this.length;if(!a)return this;for(;b<c;b++)q.event.trigger(this[b],a);return this},d.off=function(a,b,c){var d=0,e=this.length;for(o(b)&&(c=b,b=void 0);d<e;d++)q.event.remove(this[d],a,c,b);return this},d.find=function(a){for(var b=[],c=0,d=this.length,e=function(c){o(c.querySelectorAll)&&[].forEach.call(c.querySelectorAll(a),function(a){b.push(a)})};c<d;c++)e(this[c]);return q(b)},d.get=function(a){return null!=a?a<0?this[a+this.length]:this[a]:i.call(this)},d.eq=function(a){return q(this[a])},d.parent=function(){for(var a,b=[],c=0,d=this.length;c<d;c++)!~b.indexOf(a=this[c].parentElement)&&a&&b.push(a);return q(b)},d.toArray=function(){return i.call(this)},d.is=function(){var a=arguments;return this.some(function(b){return b.tagName.toLowerCase()===a[0]})},d.has=function(){var a=arguments;return this.some(function(b){return b.querySelectorAll(a[0]).length})},d.add=function(a,b){return this.pushStack(q.unique(q.merge(this.get(),q(a,b))))},d.attr=function(a,b){var c,d=arguments,e=0,f=this.length;if(m(a)&&1===d.length)return this[0]&&this[0].getAttribute(a);for(2===d.length?c=function(c){c.setAttribute(a,b)}:n(a)&&(c=function(b){k(a).forEach(function(c){b.setAttribute(c,a[c])})});e<f;e++)c(this[e]);return this},d.removeAttr=function(a){for(var b=0,c=this.length;b<c;b++)this[b].removeAttribute(a);return this},d.val=function(a){var b=0,c=this.length;if(0===arguments.length)return this[0]&&this[0].value;for(;b<c;b++)this[b].value=a;return this},d.css=function(b,c){var d,e=arguments,f=0,g=this.length;if(m(b)&&1===e.length)return this[0]&&a.getComputedStyle(this[0])[b];for(2===e.length?d=function(a){a.style[b]=c}:n(b)&&(d=function(a){k(b).forEach(function(c){a.style[c]=b[c]})});f<g;f++)d(this[f]);return this},d.data=function(a,b){var c,d=arguments,e={},f=0,g=this.length,h=function(a,b,c){n(c)?(a.jdata=a.jdata||{},a.jdata[b]=c):a.dataset[b]=c},i=function(a){return\"true\"===a||\"false\"!==a&&a};if(0===d.length)return this[0].jdata&&(e=this[0].jdata),k(this[0].dataset).forEach(function(a){e[a]=i(this[0].dataset[a])},this),e;if(1===d.length&&m(a))return this[0]&&i(this[0].dataset[a]||this[0].jdata&&this[0].jdata[a]);for(1===d.length&&n(a)?c=function(b){k(a).forEach(function(c){h(b,c,a[c])})}:2===d.length&&(c=function(c){h(c,a,b)});f<g;f++)c(this[f]);return this},d.removeData=function(a){for(var b,c,d=0,e=this.length;d<e;d++)if(b=this[d].jdata,c=this[d].dataset,a)b&&b[a]&&delete b[a],delete c[a];else{for(a in b)delete b[a];for(a in c)delete c[a]}return this},d.addClass=function(a){for(var b=0,c=0,d=this.length,e=a?a.trim().split(/\\s+/):[];b<d;b++)for(c=0,c=0;c<e.length;c++)this[b].classList.add(e[c]);return this},d.removeClass=function(a){for(var b=0,c=0,d=this.length,e=a?a.trim().split(/\\s+/):[];b<d;b++)for(c=0,c=0;c<e.length;c++)this[b].classList.remove(e[c]);return this},d.toggleClass=function(a,b){var c=0,d=this.length,e=\"toggle\";if(!0===b&&(e=\"add\")||!1===b&&(e=\"remove\"),a)for(;c<d;c++)this[c].classList[e](a);return this},d.hasClass=function(a){var b=0,c=this.length;if(a)for(;b<c;b++)if(this[b].classList.contains(a))return!0;return!1},d.html=function(a){var b,c=arguments;return 1===c.length&&void 0!==a?this.empty().append(a):0===c.length&&(b=this[0])?b.innerHTML:this},d.append=function(a){var b,c=0,d=this.length;for(m(a)&&h.exec(a)?a=q(a):n(a)||(a=document.createTextNode(a)),a=a instanceof q?a:q(a),b=function(b,c){a.forEach(function(a){c?b.appendChild(a.cloneNode(!0)):b.appendChild(a)})};c<d;c++)b(this[c],c);return this},d.appendTo=function(a){return q(a).append(this),this},d.empty=function(){for(var a,b=0,c=this.length;b<c;b++)for(a=this[b];a.lastChild;)a.removeChild(a.lastChild);return this},d.remove=function(){var a,b=0,c=this.length;for(this.off();b<c;b++)a=this[b],delete a.jdata,a.parentNode&&a.parentNode.removeChild(a);return this},\"object\"==typeof module&&module&&\"object\"==typeof module.exports?module.exports=q:\"function\"==typeof define&&define.amd?(define(function(){return q}),a.jBone=a.$=q):\"object\"==typeof a&&\"object\"==typeof a.document&&(a.jBone=a.$=q)}(\"undefined\"!=typeof window?window:this); var params=[{name: 'minimumMicroseconds', type: 'int', default: 1700, displayName: 'Minimum Microseconds', description: 'The minimum rotation of the servo motor in microseconds (A standard servo is at 0 degrees at 1000&mu;s, 90 degrees at 1500&mu;s, 180 degrees at 2000&mu;s)', rangeMin: 500, rangeMax: 2500, increment: 5},{name: 'maximumMicroseconds', type: 'int', default: 2200, displayName: 'Maximum Microseconds', description: 'The maximum rotation of the servo motor in microseconds (A standard servo is at 0 degrees at 1000&mu;s, 90 degrees at 1500&mu;s, 180 degrees at 2000&mu;s)', rangeMin: 500, rangeMax: 2500, increment: 5},{name: 'targetDistance', type: 'float', default: 10, displayName: 'Target Distance', description: 'The target distance from the surface', rangeMin: 0, rangeMax: 100, increment: 0.2, banner: true},{name: 'microsecondsPerMillimetre', type: 'float', default: -0.5, displayName: 'Movement Multiplier', description: 'The speed and direction of the movement of the head', rangeMin: -100, rangeMax: 100, increment: 0.5},{name: 'acceleration', type: 'float', default: 2, displayName: 'Movement Acceleration', description: 'The acceleration of the movement of the head (1=no acceleration, greater for higher acceleration when further away from the target distance)', rangeMin: 0.5, rangeMax: 5, increment: 0.1},{name: 'movementEnabled', type: 'bool', default: true, displayName: 'Movement Enabled', description: 'Is movement of the head enabled', banner: true},{name: 'millimetreCalculationSlope', type: 'float', default: -0.5, displayName: 'Millimetre Calculation: Slope', description: 'The slope used when calculating millimetres distance', rangeMin: -5, rangeMax: -0.05, increment: 0.05},{name: 'millimetreCalculationCross', type: 'float', default: 2.45, displayName: 'Millimetre Calculation: Cross', description: 'The cross point used when calculating millimetres distance', rangeMin: 0, rangeMax: 10, increment: 0.05},{name: 'millimetreCalculationPower', type: 'float', default: 2.4, displayName: 'Millimetre Calculation: Power', description: 'The power used when calculating millimetres distance', rangeMin: 0.25, rangeMax: 5, increment: 0.05}]; function getValue(name){return promise.get('/' + name);}function setValue(name, value){return promise.get('/' + name + '/' + value)}params.forEach(function(param){var htmlString=''; if (param.type=='float' || param.type=='int'){htmlString='<label for=\"' + param.name + '\">' + param.displayName + '</label><input id=\"' + param.name + '\" type=\"number\" value=\"' + param.default + '\" min=\"' + param.rangeMin + '\" max=\"' + param.rangeMax + '\" step=\"' + param.increment + '\"><span class=\"pure-form-message\">' + param.description + '</span>';}else if (param.type=='bool'){htmlString='<label for=\"' + param.name + '\"><input id=\"' + param.name + '\" type=\"checkbox\"'; if (param.default){htmlString +=' checked=\"checked\"';}htmlString +='> ' + param.displayName + '</label><span class=\"pure-form-message\">' + param.description + '</span>';}if (param.banner){$('form fieldset.banner').append(htmlString);}else{$('form fieldset.main').append(htmlString);}getValue(param.name).then(function(error, text){if (!error){if (param.type=='float' || param.type=='int'){$('#' + param.name).val(text);}else if (param.type=='bool'){if (text=='true'){document.getElementById(param.name).checked=true;}else{document.getElementById(param.name).checked=false;}}}}); $('#' + param.name).on('change', function(){if (param.updateTimeout){clearTimeout(param.updateTimeout);}var inputRef=$('#' + param.name); var valueToSave=''; if (param.type=='float'){var floatValue=parseFloat(inputRef.val()); if (Number.isNaN(floatValue) || floatValue < param.rangeMin){floatValue=param.rangeMin;}else if (floatValue > param.rangeMax){floatValue=param.rangeMax;}inputRef.val(floatValue); valueToSave=floatValue.toString();}if (param.type=='int'){var intValue=parseInt(inputRef.val()); if (Number.isNaN(intValue) || intValue < param.rangeMin){intValue=param.rangeMin;}else if (intValue > param.rangeMax){intValue=param.rangeMax;}inputRef.val(intValue); valueToSave=intValue.toString();}if (param.type=='bool'){var boolValue=document.getElementById(param.name).checked; if (boolValue){valueToSave='true';}else{valueToSave='false';}}param.updateTimeout=setTimeout(function(){setValue(param.name, valueToSave).then(function(error, text){if (!error){if (param.type=='float' || param.type=='int'){$('#' + param.name).val(text);}else if (param.type=='bool'){if (text=='true'){document.getElementById(param.name).checked=true;}else{document.getElementById(param.name).checked=false;}}}});}, 500);});}); </script> </body></html>");
              client.println();
              setMessage("Returned control web page");
            }

            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
  }

  uint16_t proximity = vcnl.readProximity();
  currentDistance = pow(pow(10, log10(float(proximity)) * millimetreCalculationSlope + millimetreCalculationCross), millimetreCalculationPower);
  float outBy = (currentDistance - targetDistance);
  float moveMicroseconds = pow(abs(outBy), acceleration) * microsecondsPerMillimetre;
  if (outBy < 0)
    moveMicroseconds = moveMicroseconds * -1;
  currentMicroseconds = currentMicroseconds + moveMicroseconds;

  if (currentMicroseconds < float(minimumMicroseconds))
    currentMicroseconds = float(minimumMicroseconds);
  if (currentMicroseconds > float(maximumMicroseconds))
    currentMicroseconds = float(maximumMicroseconds);
  servo.writeMicroseconds((int)round(currentMicroseconds));

  setMessage("Working normally");
}
