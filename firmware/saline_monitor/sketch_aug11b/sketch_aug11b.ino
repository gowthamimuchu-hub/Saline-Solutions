#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ESP32Servo.h>
#include <WebServer.h>

// ---------- LCD ----------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------- WiFi & Telegram ----------
const char* ssid = "chiru";
const char* password = "12345678";
#define BOT_TOKEN "8967631862:AAHA-9Zla74HCaIED_ASGlBWkOg2G9_XqCY"
#define CHAT_ID "7290199502"

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

// ---------- Web server ----------
WebServer server(80);

// ---------- Pins ----------
#define SENSOR_PIN 4
#define BUZZER_PIN 25
#define SERVO_PIN 13

// If status is ever backwards after testing, just flip this one line
#define LOW_MEANS_AVAILABLE true

Servo clampServo;

bool alertSent = false;          // ensures Telegram message sends only once per "completed" event
bool salineAvailable = true;     // shared state read by the dashboard's /status endpoint
unsigned long lastCheck = 0;
const unsigned long checkInterval = 1000;

void setup() {
  Serial.begin(115200);

  pinMode(SENSOR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // buzzer OFF by default

  clampServo.attach(SERVO_PIN);
  clampServo.write(0); // tube open by default

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Starting...");

  secured_client.setInsecure();
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("Dashboard IP: ");
  Serial.println(WiFi.localIP());

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP().toString());

  bot.sendMessage(CHAT_ID, "Smart Saline Monitoring System\nStatus: ESP32 online. Monitoring started.", "");

  // ---------- Web server routes ----------
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.begin();
  Serial.println("Web server started");

  delay(1500);
}

void loop() {
  server.handleClient(); // must run every loop for dashboard to stay responsive

  if (millis() - lastCheck > checkInterval) {
    lastCheck = millis();
    checkSalineLevel();
  }
}

void checkSalineLevel() {
  int sensorState = digitalRead(SENSOR_PIN);

  if (LOW_MEANS_AVAILABLE) {
    salineAvailable = (sensorState == LOW);
  } else {
    salineAvailable = (sensorState == HIGH);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Saline status:");

  if (salineAvailable) {
    // ---------- SALINE AVAILABLE ----------
    alertSent = false; // reset so the next "completed" event can alert again

    digitalWrite(BUZZER_PIN, LOW);  // buzzer OFF
    clampServo.write(0);            // tube stays open

    lcd.setCursor(0, 1);
    lcd.print("Available");

    Serial.println("Sensor state: Available");
    // No Telegram message sent while available

  } else {
    // ---------- SALINE COMPLETED ----------
    digitalWrite(BUZZER_PIN, HIGH); // buzzer ON
    clampServo.write(90);           // tube clamped shut

    lcd.setCursor(0, 1);
    lcd.print("Completed");

    Serial.println("Sensor state: Completed");

    if (!alertSent) {
      bot.sendMessage(CHAT_ID, "Saline status is completed.", "");
      alertSent = true;
      Serial.println("Telegram: Saline status is completed");
    }
    // Dashboard picks this up automatically via /status (see handleStatus below)
  }
}

// ---------- Web handlers ----------
void handleRoot() {
  server.send(200, "text/html", getDashboardHTML());
}

void handleStatus() {
  String json = "{";
  json += "\"salineAvailable\":" + String(salineAvailable ? "true" : "false") + ",";
  json += "\"buzzer\":" + String(digitalRead(BUZZER_PIN) == HIGH ? "true" : "false") + ",";
  json += "\"uptimeSeconds\":" + String(millis() / 1000);
  json += "}";
  server.send(200, "application/json", json);
}

String getDashboardHTML() {
  return R"HTML(<!DOCTYPE html>
<html><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Saline Monitor - Hospital Dashboard</title>
<style>
  :root{
    --bg:#0f172a; --panel:#111827; --panel2:#1f2937; --line:#2a3548;
    --text:#e5e7eb; --muted:#94a3b8;
    --primary:#10b981; --primary-dark:#059669;
    --danger:#ef4444; --warning:#f59e0b; --info:#3b82f6;
    --card:#ffffff; --card-text:#1f2937;
  }
  *{box-sizing:border-box;margin:0;padding:0}
  body{font-family:'Segoe UI',Roboto,Arial,sans-serif;background:var(--bg);color:var(--text);min-height:100vh}
  .screen{display:none;min-height:100vh}
  .screen.active{display:flex}

  /* ---------- AUTH SCREENS ---------- */
  #loginScreen, #signupScreen{align-items:center;justify-content:center;
    background:radial-gradient(circle at 20% 20%,#134e4a 0%,#0f172a 55%);}
  .auth-box{background:var(--panel);width:100%;max-width:380px;border-radius:16px;padding:32px;
    box-shadow:0 12px 30px rgba(0,0,0,.4);border:1px solid var(--line)}
  .auth-box h1{font-size:22px;text-align:center;margin-bottom:4px;color:var(--primary)}
  .auth-box p.sub{text-align:center;color:var(--muted);font-size:13px;margin-bottom:22px}
  .field{margin-bottom:14px}
  .field label{display:block;font-size:12px;color:var(--muted);margin-bottom:6px}
  .field input, .field select{width:100%;padding:10px 12px;border-radius:8px;border:1px solid var(--line);
    background:var(--panel2);color:var(--text);font-size:14px;outline:none}
  .field input:focus, .field select:focus{border-color:var(--primary)}
  .btn{width:100%;padding:11px;border:none;border-radius:8px;background:var(--primary);color:#fff;
    font-weight:700;font-size:14px;cursor:pointer;margin-top:6px;transition:.15s}
  .btn:hover{background:var(--primary-dark)}
  .btn.secondary{background:transparent;border:1px solid var(--line);color:var(--text)}
  .switch-link{text-align:center;margin-top:16px;font-size:13px;color:var(--muted)}
  .switch-link a{color:var(--primary);cursor:pointer;text-decoration:none;font-weight:600}
  .error-msg{background:#3b1220;border:1px solid var(--danger);color:#fca5a5;font-size:12px;
    padding:8px 10px;border-radius:8px;margin-bottom:14px;display:none}
  .demo-hint{margin-top:18px;font-size:11px;color:var(--muted);text-align:center;line-height:1.5}

  /* ---------- DASHBOARD ---------- */
  #dashboardScreen{width:100%}
  .sidebar{width:230px;background:var(--panel);border-right:1px solid var(--line);
    padding:22px 14px;display:flex;flex-direction:column;gap:4px;flex-shrink:0}
  .brand{display:flex;align-items:center;gap:8px;padding:0 8px 20px 8px;border-bottom:1px solid var(--line);margin-bottom:14px}
  .brand .dot{width:10px;height:10px;border-radius:50%;background:var(--primary);box-shadow:0 0 8px var(--primary)}
  .brand h2{font-size:16px}
  .navbtn{display:flex;align-items:center;gap:10px;padding:11px 12px;border-radius:9px;color:var(--muted);
    cursor:pointer;font-size:14px;font-weight:600;transition:.15s}
  .navbtn:hover{background:var(--panel2);color:var(--text)}
  .navbtn.active{background:linear-gradient(90deg,rgba(16,185,129,.18),transparent);color:var(--primary)}
  .navbtn .ic{width:18px;text-align:center}
  .sidebar-foot{margin-top:auto;padding-top:14px;border-top:1px solid var(--line)}
  .user-chip{display:flex;align-items:center;gap:10px;padding:8px;border-radius:9px;background:var(--panel2);margin-bottom:8px}
  .avatar{width:32px;height:32px;border-radius:50%;background:var(--primary);display:flex;align-items:center;
    justify-content:center;font-weight:700;color:#03251c;font-size:13px}
  .user-chip .name{font-size:13px;font-weight:700}
  .user-chip .role{font-size:11px;color:var(--muted);text-transform:capitalize}
  .logout-btn{width:100%;padding:9px;border-radius:8px;border:1px solid var(--line);background:transparent;
    color:var(--danger);font-size:12px;font-weight:700;cursor:pointer}
  .logout-btn:hover{background:rgba(239,68,68,.1)}

  .content{flex:1;padding:26px 30px;overflow-y:auto;max-height:100vh}
  .content-header{display:flex;justify-content:space-between;align-items:center;margin-bottom:22px}
  .content-header h1{font-size:22px}
  .content-header .live-tag{font-size:11px;background:rgba(16,185,129,.15);color:var(--primary);
    padding:5px 10px;border-radius:20px;display:flex;align-items:center;gap:6px;font-weight:700}
  .live-dot{width:7px;height:7px;border-radius:50%;background:var(--primary);animation:pulse 1.4s infinite}
  @keyframes pulse{0%{opacity:1}50%{opacity:.3}100%{opacity:1}}

  .view{display:none}
  .view.active{display:block}

  .stats-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(190px,1fr));gap:16px;margin-bottom:22px}
  .stat-card{background:var(--panel);border:1px solid var(--line);border-radius:14px;padding:18px}
  .stat-card .top{display:flex;justify-content:space-between;align-items:flex-start}
  .stat-card .icon-box{width:38px;height:38px;border-radius:10px;display:flex;align-items:center;justify-content:center;font-size:18px}
  .stat-card .num{font-size:26px;font-weight:800;margin-top:14px}
  .stat-card .lbl{font-size:12px;color:var(--muted);margin-top:2px}
  .bx-emerald{background:rgba(16,185,129,.15)} .bx-red{background:rgba(239,68,68,.15)}
  .bx-blue{background:rgba(59,130,246,.15)} .bx-amber{background:rgba(245,158,11,.15)}

  .panel-row{display:grid;grid-template-columns:1.2fr 1fr;gap:16px;margin-bottom:22px}
  @media(max-width:900px){.panel-row{grid-template-columns:1fr}}
  .panel{background:var(--panel);border:1px solid var(--line);border-radius:14px;padding:20px}
  .panel h3{font-size:14px;margin-bottom:14px;color:var(--muted);text-transform:uppercase;letter-spacing:.5px}

  .saline-card{background:#fff;border-radius:14px;padding:20px;color:var(--card-text);border-left:6px solid var(--primary)}
  .saline-card.completed{border-left-color:var(--danger)}
  .saline-card .status{text-align:center;padding:10px 0}
  .saline-card .icon{font-size:42px}
  .saline-card .label{font-size:20px;font-weight:800;margin-top:4px}
  .saline-card .badge{padding:4px 12px;border-radius:20px;font-size:11px;font-weight:800;float:right}
  .badge.available{background:#d1fae5;color:#065f46}
  .badge.completed-b{background:#fee2e2;color:#991b1b}

  .donut-wrap{display:flex;align-items:center;gap:18px;justify-content:center}
  .legend{font-size:12px}
  .legend .row{display:flex;align-items:center;gap:8px;margin-bottom:8px}
  .legend .sw{width:10px;height:10px;border-radius:3px}

  table{width:100%;border-collapse:collapse;font-size:13px}
  th{text-align:left;color:var(--muted);font-weight:700;padding:10px 12px;border-bottom:1px solid var(--line);font-size:11px;text-transform:uppercase}
  td{padding:11px 12px;border-bottom:1px solid var(--line)}
  tr:hover td{background:var(--panel2)}
  .pill{padding:3px 10px;border-radius:20px;font-size:11px;font-weight:700}
  .pill.on{background:rgba(16,185,129,.15);color:var(--primary)}
  .pill.off{background:rgba(239,68,68,.15);color:var(--danger)}
  .searchbar{margin-bottom:14px}
  .searchbar input{width:100%;max-width:280px;padding:9px 12px;border-radius:8px;border:1px solid var(--line);
    background:var(--panel2);color:var(--text);font-size:13px}
</style></head>
<body>
<div id="app">

  <!-- ================= LOGIN ================= -->
  <div class="screen active" id="loginScreen">
    <div class="auth-box">
      <h1>&#128137; Saline Monitor</h1>
      <p class="sub">Staff Login</p>
      <div class="error-msg" id="loginError"></div>
      <div class="field"><label>Username</label><input id="loginUser" placeholder="e.g. dr.rao"></div>
      <div class="field"><label>Password</label><input id="loginPass" type="password" placeholder="••••••••"></div>
      <button class="btn" onclick="doLogin()">Sign In</button>
      <div class="switch-link">New staff member? <a onclick="showScreen('signupScreen')">Create an account</a></div>
      <div class="demo-hint">Demo accounts &mdash; doctor: <b>dr.rao / doctor123</b><br>nurse: <b>n.priya / nurse123</b></div>
    </div>
  </div>

  <!-- ================= SIGNUP ================= -->
  <div class="screen" id="signupScreen">
    <div class="auth-box">
      <h1>&#128100; Join the Team</h1>
      <p class="sub">Create a staff account</p>
      <div class="error-msg" id="signupError"></div>
      <div class="field"><label>Full Name</label><input id="suName" placeholder="Dr. Meena Kapoor"></div>
      <div class="field"><label>Role</label>
        <select id="suRole">
          <option value="doctor">Doctor</option>
          <option value="nurse">Nurse</option>
        </select>
      </div>
      <div class="field"><label>Username</label><input id="suUser" placeholder="Choose a username"></div>
      <div class="field"><label>Password</label><input id="suPass" type="password" placeholder="Choose a password"></div>
      <button class="btn" onclick="doSignup()">Create Account</button>
      <button class="btn secondary" onclick="showScreen('loginScreen')">Back to Login</button>
    </div>
  </div>

  <!-- ================= DASHBOARD ================= -->
  <div class="screen" id="dashboardScreen">
    <nav class="sidebar">
      <div class="brand"><div class="dot"></div><h2>Saline Monitor</h2></div>
      <div class="navbtn active" data-view="overview" onclick="switchView('overview')"><span class="ic">&#128202;</span> Overview</div>
      <div class="navbtn" data-view="patients" onclick="switchView('patients')"><span class="ic">&#129658;</span> Patients</div>
      <div class="navbtn" data-view="doctors" onclick="switchView('doctors')"><span class="ic">&#129658;&#8205;&#9877;&#65039;</span> Doctors</div>
      <div class="navbtn" data-view="nurses" onclick="switchView('nurses')"><span class="ic">&#128137;</span> Nurses</div>
      <div class="sidebar-foot">
        <div class="user-chip">
          <div class="avatar" id="avatarLetter">?</div>
          <div><div class="name" id="chipName">-</div><div class="role" id="chipRole">-</div></div>
        </div>
        <button class="logout-btn" onclick="doLogout()">Log Out</button>
      </div>
    </nav>

    <main class="content">

      <!-- OVERVIEW -->
      <div class="view active" id="view-overview">
        <div class="content-header">
          <h1>Overview</h1>
          <div class="live-tag"><span class="live-dot"></span> LIVE</div>
        </div>

        <div class="stats-grid">
          <div class="stat-card"><div class="top"><div class="icon-box bx-blue">&#129658;</div></div>
            <div class="num" id="statTotalPatients">-</div><div class="lbl">Total Patients</div></div>
          <div class="stat-card"><div class="top"><div class="icon-box bx-emerald">&#128137;</div></div>
            <div class="num" id="statOnSaline">-</div><div class="lbl">On Saline (Active)</div></div>
          <div class="stat-card"><div class="top"><div class="icon-box bx-red">&#9888;&#65039;</div></div>
            <div class="num" id="statCompletedSaline">-</div><div class="lbl">Saline Completed</div></div>
          <div class="stat-card"><div class="top"><div class="icon-box bx-amber">&#128104;&#8205;&#9877;&#65039;</div></div>
            <div class="num" id="statDoctors">-</div><div class="lbl">Doctors On Duty</div></div>
          <div class="stat-card"><div class="top"><div class="icon-box bx-blue">&#128151;</div></div>
            <div class="num" id="statNurses">-</div><div class="lbl">Nurses On Duty</div></div>
        </div>

        <div class="panel-row">
          <div class="panel">
            <h3>Live Device Reading</h3>
            <div class="saline-card" id="card">
              <h1 style="font-size:15px">Bed Sensor <span class="badge" id="badge">--</span></h1>
              <div class="status"><div class="icon" id="icon">-</div><div class="label" id="label">Loading...</div></div>
            </div>
          </div>
          <div class="panel">
            <h3>Saline Status Split</h3>
            <div class="donut-wrap">
              <canvas id="donut" width="140" height="140"></canvas>
              <div class="legend">
                <div class="row"><div class="sw" style="background:#10b981"></div>Active <b id="legActive">-</b></div>
                <div class="row"><div class="sw" style="background:#ef4444"></div>Completed <b id="legCompleted">-</b></div>
              </div>
            </div>
          </div>
        </div>
      </div>

      <!-- PATIENTS -->
      <div class="view" id="view-patients">
        <div class="content-header"><h1>Patients</h1></div>
        <div class="panel">
          <div class="searchbar"><input id="patientSearch" placeholder="Search by name or bed..." oninput="renderPatients()"></div>
          <table>
            <thead><tr><th>Name</th><th>Age</th><th>Bed</th><th>Doctor</th><th>Nurse</th><th>Saline</th><th>Admitted</th></tr></thead>
            <tbody id="patientsBody"></tbody>
          </table>
        </div>
      </div>

      <!-- DOCTORS -->
      <div class="view" id="view-doctors">
        <div class="content-header"><h1>Doctors</h1></div>
        <div class="panel">
          <table>
            <thead><tr><th>Name</th><th>Specialization</th><th>Patients</th><th>Contact</th></tr></thead>
            <tbody id="doctorsBody"></tbody>
          </table>
        </div>
      </div>

      <!-- NURSES -->
      <div class="view" id="view-nurses">
        <div class="content-header"><h1>Nurses</h1></div>
        <div class="panel">
          <table>
            <thead><tr><th>Name</th><th>Shift</th><th>Patients</th><th>Contact</th></tr></thead>
            <tbody id="nursesBody"></tbody>
          </table>
        </div>
      </div>

    </main>
  </div>
</div>

<script>
/* ============================================================
   DEMO DATA STORE
   Replace the fetch() calls below with real ESP32 endpoints
   (/login, /signup, /patients, /doctors, /nurses) as you build
   them. Everything here falls back to this in-memory data so
   the dashboard works standalone right now.
   ============================================================ */
let USERS = [
  {username:'dr.rao', password:'doctor123', role:'doctor', name:'Dr. Ramesh Rao'},
  {username:'n.priya', password:'nurse123', role:'nurse', name:'Priya Nair'}
];

let DOCTORS = [
  {id:1, name:'Dr. Ramesh Rao', spec:'General Medicine', contact:'9876500011'},
  {id:2, name:'Dr. Anitha Verma', spec:'Cardiology', contact:'9876500022'},
  {id:3, name:'Dr. Suresh Iyer', spec:'Orthopedics', contact:'9876500033'}
];

let NURSES = [
  {id:1, name:'Priya Nair', shift:'Morning (7AM-3PM)', contact:'9876511111'},
  {id:2, name:'Kavya Menon', shift:'Evening (3PM-11PM)', contact:'9876522222'},
  {id:3, name:'Lakshmi Bai', shift:'Night (11PM-7AM)', contact:'9876533333'}
];

let PATIENTS = [
  {id:1, name:'Ravi Kumar', age:54, bed:'A-101', doctor:'Dr. Ramesh Rao', nurse:'Priya Nair', saline:true, admitted:'2026-08-05'},
  {id:2, name:'Sita Devi', age:67, bed:'A-102', doctor:'Dr. Anitha Verma', nurse:'Priya Nair', saline:true, admitted:'2026-08-06'},
  {id:3, name:'Manoj Reddy', age:38, bed:'B-201', doctor:'Dr. Suresh Iyer', nurse:'Kavya Menon', saline:false, admitted:'2026-08-04'},
  {id:4, name:'Lakshmi Prasad', age:72, bed:'B-202', doctor:'Dr. Ramesh Rao', nurse:'Kavya Menon', saline:true, admitted:'2026-08-08'},
  {id:5, name:'Ahmed Khan', age:45, bed:'C-301', doctor:'Dr. Anitha Verma', nurse:'Lakshmi Bai', saline:false, admitted:'2026-08-03'},
  {id:6, name:'Divya Sharma', age:29, bed:'C-302', doctor:'Dr. Suresh Iyer', nurse:'Lakshmi Bai', saline:true, admitted:'2026-08-09'}
];

let currentUser = null;

/* ---------------- SCREEN SWITCHING ---------------- */
function showScreen(id){
  document.querySelectorAll('.screen').forEach(s=>s.classList.remove('active'));
  document.getElementById(id).classList.add('active');
  document.getElementById('loginError').style.display='none';
  document.getElementById('signupError').style.display='none';
}

/* ---------------- AUTH ---------------- */
async function doLogin(){
  const u = document.getElementById('loginUser').value.trim();
  const p = document.getElementById('loginPass').value;
  const errBox = document.getElementById('loginError');
  if(!u || !p){ errBox.textContent='Please enter username and password.'; errBox.style.display='block'; return; }

  // Try a real backend first, fall back to local demo data
  let user = null;
  try{
    const r = await fetch('/login', {method:'POST', headers:{'Content-Type':'application/json'},
      body: JSON.stringify({username:u, password:p})});
    if(r.ok){ user = await r.json(); }
  }catch(e){ /* no backend yet - fall back below */ }

  if(!user){
    user = USERS.find(x=>x.username===u && x.password===p) || null;
  }

  if(!user){ errBox.textContent='Invalid username or password.'; errBox.style.display='block'; return; }
  currentUser = user;
  enterDashboard();
}

async function doSignup(){
  const name = document.getElementById('suName').value.trim();
  const role = document.getElementById('suRole').value;
  const u = document.getElementById('suUser').value.trim();
  const p = document.getElementById('suPass').value;
  const errBox = document.getElementById('signupError');

  if(!name || !u || !p){ errBox.textContent='Please fill all fields.'; errBox.style.display='block'; return; }
  if(USERS.some(x=>x.username===u)){ errBox.textContent='Username already taken.'; errBox.style.display='block'; return; }

  const newUser = {username:u, password:p, role, name};
  try{
    await fetch('/signup', {method:'POST', headers:{'Content-Type':'application/json'}, body:JSON.stringify(newUser)});
  }catch(e){ /* backend not ready - keep it local for this session */ }

  USERS.push(newUser);
  if(role==='doctor') DOCTORS.push({id:DOCTORS.length+1, name, spec:'General Medicine', contact:'-'});
  if(role==='nurse') NURSES.push({id:NURSES.length+1, name, shift:'Unassigned', contact:'-'});

  currentUser = newUser;
  enterDashboard();
}

function doLogout(){
  currentUser = null;
  document.getElementById('loginUser').value='';
  document.getElementById('loginPass').value='';
  showScreen('loginScreen');
}

/* ---------------- DASHBOARD ENTRY ---------------- */
function enterDashboard(){
  document.querySelectorAll('.screen').forEach(s=>s.classList.remove('active'));
  document.getElementById('dashboardScreen').classList.add('active');

  document.getElementById('chipName').textContent = currentUser.name;
  document.getElementById('chipRole').textContent = currentUser.role;
  document.getElementById('avatarLetter').textContent = currentUser.name.charAt(0).toUpperCase();

  switchView('overview');
  renderAll();
  pollStatus();
  setInterval(pollStatus, 2000);
}

function switchView(view){
  document.querySelectorAll('.navbtn').forEach(b=>b.classList.toggle('active', b.dataset.view===view));
  document.querySelectorAll('.view').forEach(v=>v.classList.remove('active'));
  document.getElementById('view-'+view).classList.add('active');
}

/* ---------------- RENDER: STATS + TABLES ---------------- */
function renderAll(){
  renderStats();
  renderDonut();
  renderPatients();
  renderDoctors();
  renderNurses();
}

function renderStats(){
  const onSaline = PATIENTS.filter(p=>p.saline).length;
  const completed = PATIENTS.length - onSaline;
  document.getElementById('statTotalPatients').textContent = PATIENTS.length;
  document.getElementById('statOnSaline').textContent = onSaline;
  document.getElementById('statCompletedSaline').textContent = completed;
  document.getElementById('statDoctors').textContent = DOCTORS.length;
  document.getElementById('statNurses').textContent = NURSES.length;
}

function renderDonut(){
  const onSaline = PATIENTS.filter(p=>p.saline).length;
  const completed = PATIENTS.length - onSaline;
  const total = onSaline + completed || 1;
  const ctx = document.getElementById('donut').getContext('2d');
  ctx.clearRect(0,0,140,140);
  const cx=70, cy=70, r=55, lw=18;
  let start = -Math.PI/2;
  function seg(value, color){
    const angle = (value/total) * Math.PI*2;
    ctx.beginPath();
    ctx.arc(cx,cy,r,start,start+angle);
    ctx.strokeStyle=color; ctx.lineWidth=lw; ctx.lineCap='butt';
    ctx.stroke();
    start += angle;
  }
  ctx.lineWidth=lw;
  seg(onSaline, '#10b981');
  seg(completed, '#ef4444');
  ctx.fillStyle='#e5e7eb'; ctx.font='bold 16px sans-serif'; ctx.textAlign='center';
  ctx.fillText(total, cx, cy+6);
  document.getElementById('legActive').textContent = onSaline;
  document.getElementById('legCompleted').textContent = completed;
}

function renderPatients(){
  const q = (document.getElementById('patientSearch')?.value || '').toLowerCase();
  const rows = PATIENTS.filter(p => p.name.toLowerCase().includes(q) || p.bed.toLowerCase().includes(q))
    .map(p => `<tr>
      <td>${p.name}</td><td>${p.age}</td><td>${p.bed}</td><td>${p.doctor}</td><td>${p.nurse}</td>
      <td><span class="pill ${p.saline?'on':'off'}">${p.saline?'Active':'Completed'}</span></td>
      <td>${p.admitted}</td></tr>`).join('');
  document.getElementById('patientsBody').innerHTML = rows || '<tr><td colspan="7">No patients found.</td></tr>';
}

function renderDoctors(){
  document.getElementById('doctorsBody').innerHTML = DOCTORS.map(d=>{
    const count = PATIENTS.filter(p=>p.doctor===d.name).length;
    return `<tr><td>${d.name}</td><td>${d.spec}</td><td>${count}</td><td>${d.contact}</td></tr>`;
  }).join('');
}

function renderNurses(){
  document.getElementById('nursesBody').innerHTML = NURSES.map(n=>{
    const count = PATIENTS.filter(p=>p.nurse===n.name).length;
    return `<tr><td>${n.name}</td><td>${n.shift}</td><td>${count}</td><td>${n.contact}</td></tr>`;
  }).join('');
}

/* ---------------- LIVE SALINE DEVICE POLL (kept from original) ---------------- */
async function pollStatus(){
  try{
    const r = await fetch('/status', {cache:'no-store'});
    const d = await r.json();
    const card = document.getElementById('card');
    const badge = document.getElementById('badge');
    const icon = document.getElementById('icon');
    const label = document.getElementById('label');
    card.className = 'saline-card ' + (d.salineAvailable ? '' : 'completed');
    badge.className = 'badge ' + (d.salineAvailable ? 'available' : 'completed-b');
    badge.textContent = d.salineAvailable ? 'AVAILABLE' : 'COMPLETED';
    icon.textContent = d.salineAvailable ? '\u{1F4A7}' : '\u{26A0}\u{FE0F}';
    label.textContent = d.salineAvailable ? 'Saline Available' : 'Saline Completed';
  }catch(e){ /* device endpoint not reachable yet */ }
}
</script>
</body></html>)HTML";
}
