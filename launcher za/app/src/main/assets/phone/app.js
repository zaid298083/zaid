const bridge = window.SampPhone || null;

const state = {
  view: "home",
  previous: "home",
  runningApp: false,
  appView: "whatsapp",
  tab: "chats",
  filter: "all",
  query: "",
  currentChat: -1,
  lastSeen: 1,
  lockedContactPhone: false,
  voice: { recording: false, startedAt: 0, timer: 0, timeout: 0, pointerId: null },
  camera: { mode: "world", url: "", previewUrl: "", returnView: "chat", dragging: false, lastX: 0, lastY: 0, lastMove: 0, pendingDx: 0, pendingDy: 0, moveTimer: 0, syncTimers: [], closing: false },
  call: { status: "idle", phone: "", name: "", direction: "", startedAt: 0, timer: 0, returnView: "whatsapp" },
};

const profile = { accountId: "", name: "Jogador", bio: "Disponivel", phone: "" };
const contacts = [{ self: true, name: "Jogador (Voce)", phone: "", note: "Seu celular", avatar: "photo" }];
const chats = [];
const statuses = [];
const groups = [];
const calls = [];
const bank = {
  agency: "0001",
  account: "",
  balance: 1000000,
  moneyIn: 0,
  moneyOut: 0,
  creditLimit: 151000,
  creditUsed: 0,
  creditBlocked: false,
  dirty: false,
  movementScore: 0,
  invoiceDue: "7 dias",
  invoiceAmount: 0,
  financingPaid: 0,
  financingTotal: 0,
  financingSlots: 0,
  financingId: 0,
  financingTotalCents: 0,
  financingInstallment: 0,
  financingPaidMask: 0,
  financeItems: [],
  selectedFinanceId: 0,
  selectedInvoiceMask: 0,
  amountDigits: "",
  target: null,
  pending: false,
  receipt: null,
  transactionIds: new Set(),
  extractTab: "in",
  recipients: [
    { id: "302", name: "Leticia Isabel Vieira", bank: "Nu Pagamentos S.A.", agency: "0001", account: "1234567-8" },
    { id: "305", name: "Bernardo Claudio Cesar Santos", bank: "Itau Unibanco S.A.", agency: "5961", account: "20123-4" }
  ],
  transactions: []
};
const sportfy = {
  tab: "home",
  query: "",
  results: [],
  favorites: [],
  history: [],
  playlists: [],
  libraryPlaylist: "",
  current: null,
  playing: false,
  volume: 86,
  filter: "todos",
  queue: [],
  queueIndex: 0,
  ytPlayer: null,
  playerReady: false,
  apiLoading: false,
  progressTimer: 0,
  seeking: false,
  startedAt: 0,
  pausedAt: 0,
  playbackTimer: 0,
  playbackVideo: "",
  repeat: false,
  shuffle: false,
  nowOpen: false,
  bluetooth: { connected: false, vehicle: -1 },
  searchTimer: 0,
  searchSerial: 0,
  nativeSearchResolvers: {},
  localFrame: null,
  unavailable: new Set(),
  prefetched: new Set(),
  ambientFrame: null,
  ambient: { vehicle: 0, video: "", volume: 0, pos: 0, mode: "", updatedAt: 0 },
  ambientSources: {},
  ambientVolumeCurrent: 0,
  ambientVolumeTarget: 0,
  ambientVolumeTimer: 0,
  nativeAudio: { active: false, video: "" },
  btStateTimer: 0,
  btStateLastSent: 0,
  btSpatialTimer: 0,
  btSpatialLost: 0,
  suppressPlayerEventsUntil: 0,
  suppressBluetoothStateUntil: 0
};
const instagram = {
  tab: "feed",
  query: "",
  searchTimer: 0,
  ready: bridge === null,
  onboarding: bridge === null,
  demoSeeded: false,
  viewingProfile: "",
  profile: { id: "", username: "", name: "Jogador", bio: "", avatar: "", posts: 0, followers: 0, following: 0 },
  posts: [],
  feedIds: [],
  feedMode: "initial",
  feedLoading: false,
  feedHasMore: true,
  stories: [],
  users: [],
  suggestions: [],
  activity: [],
  conversations: [],
  messages: {},
  comments: {},
  storyViewers: {},
  currentPeer: "",
  currentPost: "",
  detailPost: "",
  focusCommentActor: "",
  unread: 0,
  compose: { open: false, mode: "post", image: "", preview: "", caption: "" },
  edit: { open: false, avatar: "" },
  story: null,
  storyGroup: [],
  storyIndex: 0,
  storyTimer: 0,
  storySeen: new Set(),
  storySyncing: false,
  nextStories: [],
};
const radarbot = {
  active: false,
  markers: [],
  ready: false,
  count: 0,
  map: { x: 0, y: 0, dragging: false, startX: 0, startY: 0, baseX: 0, baseY: 0 }
};
const lastSeenOptions = ["Todos", "Meus contatos", "Ninguem"];
const SETTINGS_STORAGE_KEY = "capitalPhoneSettings";
const HUD_NOTIFY_STORAGE_KEY = "capitalPhoneNotificationPrefs";
const SPORTFY_STORAGE_KEY = "capitalSportfyCache";
let phoneScopedAccount = "";
let sportfyLoadedAccount = "";
const SPORTFY_SEARCH_ENDPOINTS = [
  "https://inv.thepixora.com",
  "https://iv.melmac.space",
  "https://yewtu.be"
];
const SPORTFY_SEED_TRACKS = [
  { id: "SsqF4xiQbLU", title: "POSSO ATE NAO TE DAR FLORES", author: "MC Ryan SP, MC Jacare e DJ Japa NK", duration: 245, tag: "funk" },
  { id: "dQw4w9WgXcQ", title: "Never Gonna Give You Up", author: "Rick Astley", duration: 213, tag: "pop" },
  { id: "kJQP7kiw5Fk", title: "Despacito", author: "Luis Fonsi e Daddy Yankee", duration: 282, tag: "latino" },
  { id: "fJ9rUzIMcZQ", title: "Bohemian Rhapsody", author: "Queen", duration: 359, tag: "rock" },
  { id: "60ItHLz5WEA", title: "Faded", author: "Alan Walker", duration: 213, tag: "eletronica" },
  { id: "OPf0YbXqDm0", title: "Uptown Funk", author: "Mark Ronson e Bruno Mars", duration: 270, tag: "funk" },
  { id: "8LJsMUkxIWY", title: "Oruam - Paranoid", author: "Oruam", duration: 196, tag: "trap" }
];
const BANK_EXTRACT_LIMIT = 5;
const wallpaperNames = { 1: "Principal", 2: "Imagem 2", 3: "Imagem 3" };
const lightWallpapers = new Set(["2", "3"]);
const phoneSettings = { wallpaper: "1", notifications: { whatsapp: true, bank: true, game: true } };
const blockedContacts = [];
const viewportLock = { width: 0, height: 0 };
let currentAudio = null;
let currentAudioButton = null;
let currentAudioFrame = 0;
let currentAudioNative = false;
let currentAudioStartedAt = 0;
let currentAudioDuration = 0;
const AUTOLOAD_IMAGE_LIMIT = 3;
const renderQueue = { chats: 0, messages: 0, all: 0 };
let transitionTimer = 0;
const mediaResolver = { cache: new Map(), pending: new Set() };
const EMPTY_IMAGE_DATA = "data:image/gif;base64,R0lGODlhAQABAIAAAAAAAP///ywAAAAAAQABAAACAUwAOw==";
const INSTAGRAM_STORY_SECONDS = 15;

function $(selector, root = document) { return root.querySelector(selector); }
function $all(selector, root = document) { return Array.from(root.querySelectorAll(selector)); }
function icon(id) { return `<svg><use href="#${id}"/></svg>`; }
function esc(value) {
  return String(value ?? "").replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/"/g, "&quot;");
}
function cssEscape(value) {
  if (window.CSS?.escape) return CSS.escape(String(value || ""));
  return String(value || "").replace(/["\\]/g, "\\$&");
}
function encodePayload(value) {
  return encodeURIComponent(value || "").replace(/%/g, "#");
}
function decode(value) {
  const raw = String(value || "");
  const normalized = raw.replace(/#([0-9A-Fa-f]{2})/g, "%$1");
  try { return decodeURIComponent(normalized); } catch {}
  try { return decodeURIComponent(raw); } catch {}
  return raw;
}
function needsNativeMediaResolve(url) {
  const value = String(url || "");
  return /phone_upload[#/](files|uploads)[#/]/i.test(value) || /phone_upload(#2F|%2F)(files|uploads)/i.test(value);
}
function resolveMediaUrl(url) {
  const value = decode(url || "");
  if (/^https:\/\/appassets\.androidplatform\.net\/phone_upload\/files\//i.test(value)) return value;
  if (!value || !needsNativeMediaResolve(value) || !bridge?.resolveMedia) return value;
  const cached = mediaResolver.cache.get(value);
  if (cached) return cached;
  if (!mediaResolver.pending.has(value)) {
    mediaResolver.pending.add(value);
    bridge.resolveMedia(value);
  }
  return "";
}
function mediaNativeStatus(url, dataUrl, error) {
  const key = decode(url || "");
  mediaResolver.pending.delete(key);
  if (dataUrl) {
    mediaResolver.cache.set(key, dataUrl);
    renderAll();
    if (instagram.story) $("#igStoryImage").src = resolveMediaUrl(instagram.story.media) || EMPTY_IMAGE_DATA;
  } else if (error) {
    console.warn("midia indisponivel", key, error);
  }
}
function payloadToObject(payload) {
  const data = {};
  if (typeof payload !== "string") return data;
  payload.split(/[;&]/).forEach((part) => {
    const index = part.indexOf("=");
    if (index > 0) data[part.slice(0, index)] = part.slice(index + 1);
  });
  return data;
}
function initials(value) {
  const parts = String(value || "?").trim().split(/\s+/).filter(Boolean);
  return (parts.slice(0, 2).map((part) => part[0]).join("") || "?").toUpperCase();
}
function nowText() { return $("#clock")?.textContent || ""; }
function eventTime(data) { return decode(data.time) || nowText(); }
function formatVoiceDuration(value) {
  const seconds = Math.max(0, Math.min(60, Number(value) || 0));
  return `${String(Math.floor(seconds / 60)).padStart(2, "0")}:${String(seconds % 60).padStart(2, "0")}`;
}
function formatCallDuration(value) {
  const seconds = Math.max(0, Number(value) || 0);
  return `${String(Math.floor(seconds / 60)).padStart(2, "0")}:${String(seconds % 60).padStart(2, "0")}`;
}
function formatMoney(value, signed = false) {
  const cents = Math.round(Number(value) || 0);
  const prefix = signed && cents > 0 ? "+ " : signed && cents < 0 ? "- " : "";
  const absolute = Math.abs(cents);
  const reais = Math.floor(absolute / 100).toLocaleString("pt-BR");
  const centavos = String(absolute % 100).padStart(2, "0");
  return `${prefix}R$ ${reais},${centavos}`;
}
function firstName(value) {
  return String(value || "Jogador").trim().split(/\s+/)[0] || "Jogador";
}
function normalizePhone(value) {
  const clean = String(value || "").replace(/[^\d]/g, "").slice(0, 6);
  if (clean.length <= 3) return clean;
  return `${clean.slice(0, 3)}-${clean.slice(3)}`;
}

function readStorageJson(key, fallback) {
  try {
    const raw = window.localStorage?.getItem(key);
    return raw ? JSON.parse(raw) : fallback;
  } catch {}
  return fallback;
}

function accountStorageSuffix() {
  const account = String(profile.accountId || "guest").replace(/[^\dA-Za-z_-]/g, "") || "guest";
  return account;
}

function scopedStorageKey(base) {
  return `${base}:${accountStorageSuffix()}`;
}

function loadPhoneSettings() {
  const saved = readStorageJson(SETTINGS_STORAGE_KEY, {});
  if (wallpaperNames[saved.wallpaper]) phoneSettings.wallpaper = String(saved.wallpaper);
  if (saved.notifications && typeof saved.notifications === "object") {
    phoneSettings.notifications.whatsapp = saved.notifications.whatsapp !== false;
    phoneSettings.notifications.bank = saved.notifications.bank !== false;
    phoneSettings.notifications.game = saved.notifications.game !== false;
  }
  const savedBlocked = readStorageJson(scopedStorageKey(`${SETTINGS_STORAGE_KEY}:blocked`), []);
  if (Array.isArray(savedBlocked)) {
    blockedContacts.splice(0, blockedContacts.length, ...savedBlocked
      .map((contact) => ({
        phone: normalizePhone(contact.phone),
        name: String(contact.name || "").trim(),
        avatar: String(contact.avatar || "").trim()
      }))
      .filter((contact) => contact.phone));
  }
}

function savePhoneSettings() {
  try {
    window.localStorage?.setItem(SETTINGS_STORAGE_KEY, JSON.stringify(phoneSettings));
    window.localStorage?.setItem(HUD_NOTIFY_STORAGE_KEY, JSON.stringify(phoneSettings.notifications));
  } catch {}
}

function saveBlockedContacts() {
  try { window.localStorage?.setItem(scopedStorageKey(`${SETTINGS_STORAGE_KEY}:blocked`), JSON.stringify(blockedContacts)); } catch {}
}

function applyPhoneSettings(syncServer = true) {
  document.documentElement.style.setProperty("--home-wallpaper", `url("assets/wallpaper-${phoneSettings.wallpaper}.webp")`);
  document.body.classList.toggle("light-wallpaper", lightWallpapers.has(phoneSettings.wallpaper));
  document.body.classList.toggle("home-ui", state.view === "home");
  savePhoneSettings();
  if (syncServer) {
    bridge?.action?.("notify_pref", `app=whatsapp;enabled=${phoneSettings.notifications.whatsapp ? 1 : 0}`);
    bridge?.action?.("notify_pref", `app=bank;enabled=${phoneSettings.notifications.bank ? 1 : 0}`);
  }
  renderConfig();
}

function fitPhone() {
  const width = Math.max(window.innerWidth || 0, document.documentElement.clientWidth || 0);
  let height = Math.max(window.innerHeight || 0, document.documentElement.clientHeight || 0);
  if (!width || !height) return;
  const typing = document.activeElement && document.activeElement.matches("input, textarea");
  if (!typing || width !== viewportLock.width || height > viewportLock.height) {
    viewportLock.width = width;
    viewportLock.height = height;
  } else if (viewportLock.height && height < viewportLock.height * 0.82) {
    height = viewportLock.height;
  }
  const landscape = width > height;
  const lowLandscape = landscape && height < 520;
  const margin = Math.max(lowLandscape ? 6 : 8, Math.round(Math.min(width, height) * (lowLandscape ? 0.012 : 0.018)));
  const usableH = Math.max(360, height - margin * 2);
  const phoneRatio = 2.08;
  const maxW = lowLandscape ? width * 0.21 : (landscape ? width * 0.22 : width * 0.88);
  let phoneH = usableH;
  let phoneW = phoneH / phoneRatio;
  if (phoneW > maxW) {
    phoneW = maxW;
    phoneH = phoneW * phoneRatio;
  }
  phoneW = Math.max(lowLandscape ? 174 : 226, Math.min(lowLandscape ? 286 : 320, phoneW));
  phoneH = Math.min(usableH, phoneW * phoneRatio);
  document.documentElement.style.setProperty("--phone-w", `${Math.round(phoneW)}px`);
  document.documentElement.style.setProperty("--phone-h", `${Math.round(phoneH)}px`);
  document.documentElement.style.setProperty("--phone-top", `${Math.max(margin, Math.floor((height - phoneH) / 2))}px`);
  document.documentElement.style.setProperty("--phone-right", `${margin}px`);
  document.body.classList.toggle("phone-tiny", lowLandscape || phoneW < 286 || phoneH < 600);
  document.body.classList.toggle("phone-compact", phoneW < 340 || phoneH < 760);
  if (state.view === "camera") scheduleCameraSync();
  if (typing) window.scrollTo(0, 0);
}

function beep() {
  try {
    const Ctx = window.AudioContext || window.webkitAudioContext;
    if (!Ctx) return;
    const ctx = beep.ctx || (beep.ctx = new Ctx());
    const osc = ctx.createOscillator();
    const gain = ctx.createGain();
    osc.frequency.setValueAtTime(620, ctx.currentTime);
    osc.frequency.exponentialRampToValueAtTime(420, ctx.currentTime + 0.05);
    gain.gain.setValueAtTime(0.001, ctx.currentTime);
    gain.gain.exponentialRampToValueAtTime(0.07, ctx.currentTime + 0.008);
    gain.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + 0.06);
    osc.connect(gain);
    gain.connect(ctx.destination);
    osc.start();
    osc.stop(ctx.currentTime + 0.065);
  } catch {}
}

function showToast(text) {
  let toast = $("#phoneToast");
  if (!toast) {
    toast = document.createElement("div");
    toast.id = "phoneToast";
    toast.className = "phone-toast";
    $(".screen")?.appendChild(toast);
  }
  toast.textContent = text;
  toast.classList.add("show");
  window.clearTimeout(showToast.timer);
  showToast.timer = window.setTimeout(() => toast.classList.remove("show"), 1900);
}

function viewGroup(view) {
  if (view === "home") return "home";
  if (String(view).startsWith("bank")) return "bank";
  if (String(view).startsWith("config")) return "config";
  if (String(view).startsWith("sportfy")) return "sportfy";
  if (String(view).startsWith("instagram")) return "instagram";
  if (view === "camera") return "camera";
  if (view === "call") return "call";
  return "whatsapp";
}

function setViewTransition(previous, next) {
  document.body.classList.remove("transition-home", "transition-app", "transition-inside");
  window.clearTimeout(transitionTimer);
  const mode = next === "home"
    ? "transition-home"
    : previous === "home"
      ? "transition-app"
      : viewGroup(previous) === viewGroup(next)
        ? "transition-inside"
        : "transition-app";
  document.body.classList.add(mode);
  transitionTimer = window.setTimeout(() => {
    document.body.classList.remove("transition-home", "transition-app", "transition-inside");
  }, 180);
}

function resetToHomeInstant() {
  window.clearTimeout(transitionTimer);
  document.body.classList.remove("transition-home", "transition-app", "transition-inside", "bank-ui", "settings-ui", "camera-active");
  closeMenus();
  closeRecents();
  $all(".view").forEach((view) => view.classList.toggle("active", view.dataset.view === "home"));
  state.previous = state.view;
  state.view = "home";
  document.body.classList.add("home-ui");
  window.scrollTo(0, 0);
}

function showView(view) {
  const next = $(`[data-view="${view}"]`);
  if (!next || state.view === view) return;
  const current = $(".view.active");
  const previousView = state.view;
  if (state.view === "camera" && view !== "camera" && !state.camera.closing) {
    stopCameraSession();
  } else if (view !== "camera") {
    document.body.classList.remove("camera-active");
  }
  closeMenus();
  setViewTransition(previousView, view);
  current?.classList.remove("active");
  next.classList.add("active");
  state.previous = previousView;
  state.view = view;
  document.body.classList.toggle("bank-ui", view.startsWith("bank"));
  document.body.classList.toggle("settings-ui", view.startsWith("config"));
  document.body.classList.toggle("home-ui", view === "home");
  if (view !== "home") {
    state.runningApp = true;
    state.appView = view;
  }
  if (view === "instagram") {
    bridge?.action?.("insta_open", "");
    renderInstagram();
  }
  if (view === "radarbot") {
    bridge?.action?.("radarbot_open", "");
    renderRadarBot();
  }
}
function replayPhoneOpenAnimation() {
  document.body.classList.remove("phone-show");
  void document.body.offsetWidth;
  document.body.classList.add("phone-show");
}
function goHome() {
  closeRecents();
  if (state.view === "camera") {
    state.camera.closing = true;
    stopCameraSession();
    showView("home");
    state.camera.closing = false;
    return;
  }
  if (state.view !== "home") showView("home");
}
function goBack() {
  if ($("#recents")?.classList.contains("open")) return closeRecents();
  if (state.view === "camera") return closeCamera();
  if (state.view === "instagram") return goInstagramBack();
  const active = $(".view.active");
  const back = active?.querySelector("[data-back]");
  if (back) return showView(back.dataset.back);
  if (state.view !== "home") return showView("home");
}
function openRecents() {
  closeMenus();
  const isBank = String(state.appView || "").startsWith("bank");
  const isConfig = String(state.appView || "").startsWith("config");
  const isSportfy = String(state.appView || "").startsWith("sportfy");
  const isInstagram = String(state.appView || "").startsWith("instagram");
  const isRadarBot = String(state.appView || "").startsWith("radarbot");
  $("#recentTitle").textContent = state.runningApp ? (isBank ? "Mercantil" : isConfig ? "Config" : isSportfy ? "Sportfy" : isInstagram ? "Instagram" : isRadarBot ? "RadarBot" : "WhatsApp") : "Nenhum app";
  const recentIcon = $("#resumeApp span");
  if (recentIcon) {
    recentIcon.classList.toggle("bank-recent-icon", isBank);
    recentIcon.classList.toggle("config-recent-icon", isConfig);
    recentIcon.classList.toggle("sportfy-recent-icon", isSportfy);
    recentIcon.classList.toggle("instagram-recent-icon", isInstagram);
    recentIcon.classList.toggle("radarbot-recent-icon", isRadarBot);
    recentIcon.innerHTML = isBank
      ? `<img src="assets/bancologo.png" alt="" />`
        : isConfig
          ? `<img src="assets/config.png" alt="" />`
          : isSportfy
          ? `<img src="assets/sportfy-logo.png" alt="" />`
          : isInstagram
          ? `<img src="assets/instagram-logo.png" alt="" />`
          : isRadarBot
          ? `<img src="assets/radarbot-logo.png" alt="" />`
          : `<img src="assets/whatsapp.png" alt="" />`;
  }
  $("#resumeApp").disabled = !state.runningApp;
  $("#closeRecentApp").hidden = !state.runningApp;
  $("#recents").classList.add("open");
}
function closeRecents() { $("#recents")?.classList.remove("open"); }
function closePhone() {
  if (state.call.status !== "idle") {
    showView(state.call.status === "incoming" ? state.call.returnView || "whatsapp" : "call");
    return;
  }
  bridge?.close?.();
}
function closeMenus() {
  $("#chatMenu")?.classList.remove("open");
  $("#attachmentMenu")?.classList.remove("open");
  $("#modalLayer")?.classList.remove("open");
  $all(".modal").forEach((modal) => modal.classList.remove("active"));
  $("#contactPhone") && ($("#contactPhone").readOnly = false);
  $("#contactPhoneLabel") && ($("#contactPhoneLabel").hidden = false);
  if (document.activeElement && document.activeElement.blur) document.activeElement.blur();
  window.scrollTo(0, 0);
}
function openModal(name) {
  $("#modalLayer").classList.add("open");
  $all(".modal").forEach((modal) => modal.classList.toggle("active", modal.dataset.modal === name));
  if (document.activeElement && document.activeElement.blur) document.activeElement.blur();
}

function findContact(phone) { return contacts.find((contact) => !contact.self && contact.phone === phone); }
function findChat(phone) { return chats.findIndex((chat) => chat.phone === phone); }
function makeAvatar(name, photo) {
  if (photo) return `<span class="avatar photo"></span>`;
  return `<span class="avatar">${esc(initials(name))}</span>`;
}
function updateProfile() {
  contacts[0].name = `${profile.name} (Voce)`;
  contacts[0].phone = profile.phone;
  contacts[0].note = profile.phone || "Seu celular";
  $("#settingsName").textContent = profile.name;
  $("#settingsBio").textContent = profile.bio;
  $("#profileName").textContent = profile.name;
  $("#profileBio").textContent = profile.bio;
  $("#profilePhone").textContent = profile.phone;
  $("#myStatusAvatar").textContent = initials(profile.name);
}

function bankAccountFromId(accountId = profile.accountId) {
  const raw = String(accountId || "301").replace(/[^\d]/g, "") || "301";
  const number = String((Number(raw) * 7919) % 9000000 + 1000000);
  return `${number.slice(0, 7)}-${Number(raw) % 10}`;
}

function activeFinance() {
  if (!bank.financeItems.length) return null;
  return bank.financeItems.find((item) => item.id === bank.selectedFinanceId) || bank.financeItems[0];
}

function financeValue(key, fallback = 0) {
  const item = activeFinance();
  return item ? Number(item[key] || 0) : Number(fallback || 0);
}

function financeFullMask() {
  const slots = financeValue("installments", bank.financingSlots || bank.financingTotal);
  return slots > 0 ? (1 << slots) - 1 : 0;
}

function financePaidCount(mask = financeValue("paidMask", bank.financingPaidMask)) {
  let value = Number(mask || 0);
  let count = 0;
  while (value > 0) {
    count += value & 1;
    value >>= 1;
  }
  return count;
}

function financeInstallmentAmount(index) {
  const slots = financeValue("installments", bank.financingSlots || bank.financingTotal);
  const total = financeValue("totalCents", bank.financingTotalCents);
  const installment = financeValue("installmentCents", bank.financingInstallment);
  if (slots < 1) return 0;
  if (index === slots - 1) {
    return Math.max(0, total - (installment * (slots - 1)));
  }
  return installment;
}

function financeItemInstallmentAmount(item, index) {
  if (!item || item.installments < 1) return 0;
  if (index === item.installments - 1) {
    return Math.max(0, item.totalCents - (item.installmentCents * (item.installments - 1)));
  }
  return item.installmentCents;
}

function nextInvoiceAmount() {
  const slots = financeValue("installments", bank.financingSlots || bank.financingTotal);
  const paidMask = financeValue("paidMask", bank.financingPaidMask);
  for (let index = 0; index < slots; index += 1) {
    if (!(paidMask & (1 << index))) return financeInstallmentAmount(index);
  }
  return activeFinance() ? 0 : bank.creditUsed;
}

function selectedInvoiceAmount(mask = bank.selectedInvoiceMask) {
  let total = 0;
  const slots = financeValue("installments", bank.financingSlots || bank.financingTotal);
  const paidMask = financeValue("paidMask", bank.financingPaidMask);
  for (let index = 0; index < slots; index += 1) {
    const bit = 1 << index;
    if ((mask & bit) && !(paidMask & bit)) total += financeInstallmentAmount(index);
  }
  return total;
}

function financePendingAmount(item = activeFinance()) {
  if (!item) return bank.creditUsed;
  let total = 0;
  for (let index = 0; index < item.installments; index += 1) {
    if (!(item.paidMask & (1 << index))) total += financeItemInstallmentAmount(item, index);
  }
  return total;
}

function financeAggregateNextAmount() {
  if (!bank.financeItems.length) return bank.creditUsed;
  return bank.financeItems.reduce((total, item) => {
    for (let index = 0; index < item.installments; index += 1) {
      if (!(item.paidMask & (1 << index))) return total + financeItemInstallmentAmount(item, index);
    }
    return total;
  }, 0);
}

function renderBank() {
  if (!$("#bankFirstName")) return;
  const available = Math.max(0, bank.creditLimit - bank.creditUsed);
  const selectedFinance = activeFinance();
  const aggregateInvoice = financeAggregateNextAmount();
  const selectedInvoice = selectedFinance ? nextInvoiceAmount() : aggregateInvoice;
  const usedPercent = bank.creditLimit > 0 ? Math.min(100, Math.round((bank.creditUsed / bank.creditLimit) * 100)) : 0;
  bank.account = bank.account || bankAccountFromId();
  $("#bankFirstName").textContent = firstName(profile.name);
  $("#bankAgency").textContent = bank.agency;
  $("#bankAccountNumber").textContent = bank.account;
  $("#bankBalanceHome").textContent = formatMoney(bank.balance);
  $("#bankBalanceAccount").textContent = formatMoney(bank.balance);
  $("#bankAmountBalance").textContent = formatMoney(bank.balance);
  $("#bankInvoiceHome").textContent = formatMoney(aggregateInvoice);
  $("#bankInvoiceCard").textContent = formatMoney(aggregateInvoice);
  $("#bankInvoiceAmount").textContent = formatMoney(selectedInvoice);
  $("#bankInvoicePending") && ($("#bankInvoicePending").textContent = formatMoney(selectedFinance ? financePendingAmount(selectedFinance) : bank.creditUsed));
  $("#bankCreditAvailable").textContent = formatMoney(available);
  $("#bankCreditLimit").textContent = `Limite ${formatMoney(bank.creditLimit)}`;
  $("#bankCreditBar").style.width = `${usedPercent}%`;
  $("#bankMoneyIn").textContent = formatMoney(bank.moneyIn, true);
  $("#bankMoneyOut").textContent = formatMoney(-bank.moneyOut, true);
  $("#bankInvoiceDue").textContent = bank.invoiceDue || "em aberto";
  $("#bankInvoiceDate").textContent = bank.invoiceDue || "em aberto";
  $("#bankCreditName").textContent = firstName(profile.name).toUpperCase();
  $("#bankCreditStatus").innerHTML = bank.financingTotal > 0
    ? `<b>Financiamento ativo</b><small>${bank.financeItems.length} veiculo(s) financiado(s).</small>`
    : bank.creditBlocked
    ? `<b>Credito bloqueado</b><small>Pague a fatura para limpar o nome e voltar a comprar parcelado.</small>`
    : bank.creditLimit > 0
      ? `<b>Credito ativo</b><small>O limite cresce conforme suas movimentacoes dos ultimos 5 dias.</small>`
      : `<b>Em analise automatica</b><small>Movimente a conta por alguns dias para liberar cartao de credito.</small>`;
  $("#bankInvoiceWarning").innerHTML = bank.dirty
    ? `<b>Nome negativado</b><small>Existe fatura vencida. O credito fica travado ate o pagamento.</small>`
    : bank.financingTotal > 0
      ? `<b>Financiamento ativo</b><small>Pague a fatura atual, adiante uma parcela ou quite tudo de uma vez.</small>`
      : `<b>Pagamento seguro</b><small>Ao pagar, o limite usado volta a ficar disponivel automaticamente.</small>`;
  $("#bankFinanceActions")?.classList.toggle("show", Boolean(selectedFinance) && bank.creditUsed > 0);
  renderBankExtract();
  renderBankPeople();
  renderBankAmount();
  renderBankReview();
  renderBankShareContacts();
  renderBankFinances();
  renderBankInstallments();
  renderBankInvoiceHistory();
}

function radarbotBounds() {
  if (!radarbot.markers.length) return { minX: -3000, maxX: 3000, minY: -3000, maxY: 3000 };
  return radarbot.markers.reduce((bounds, marker) => ({
    minX: Math.min(bounds.minX, marker.x),
    maxX: Math.max(bounds.maxX, marker.x),
    minY: Math.min(bounds.minY, marker.y),
    maxY: Math.max(bounds.maxY, marker.y)
  }), { minX: Infinity, maxX: -Infinity, minY: Infinity, maxY: -Infinity });
}

function renderRadarBotMap() {
  const grid = $("#radarbotMapGrid");
  if (!grid) return;
  const bounds = radarbotBounds();
  const spanX = Math.max(1, bounds.maxX - bounds.minX);
  const spanY = Math.max(1, bounds.maxY - bounds.minY);
  grid.style.transform = `translate(${radarbot.map.x}px, ${radarbot.map.y}px)`;
  grid.innerHTML = radarbot.markers.map((marker) => {
    const left = 8 + ((marker.x - bounds.minX) / spanX) * 84;
    const top = 8 + ((bounds.maxY - marker.y) / spanY) * 84;
    return `<span class="radarbot-marker" style="left:${left.toFixed(2)}%;top:${top.toFixed(2)}%" title="Radar ${marker.id} - ${marker.limit} km/h"><i></i><b>${esc(marker.limit)}</b></span>`;
  }).join("");
}

function renderRadarBot() {
  $("#radarbotStatus").textContent = radarbot.active ? "ATIVO" : "INATIVO";
  $("#radarbotStatusCard")?.classList.toggle("active", radarbot.active);
  $("#radarbotButtonVerb").textContent = radarbot.active ? "PARAR" : "INICIAR";
  $("#radarbotToggle")?.classList.toggle("active", radarbot.active);
  $("#radarbotHint").textContent = radarbot.active
    ? `${radarbot.count || radarbot.markers.length} radares monitorados em tempo real`
    : "Sera mostrado radares proximos ao iniciar";
  renderRadarBotMap();
}

function updateRadarBotFromEvent(data) {
  const type = String(data.type || "");
  if (type === "radarbot_clear") {
    radarbot.markers = [];
    radarbot.ready = false;
    radarbot.count = 0;
    return renderRadarBot();
  }
  if (type === "radarbot_marker") {
    const marker = {
      id: Number(data.i || data.id) || radarbot.markers.length + 1,
      x: Number(data.x) || 0,
      y: Number(data.y) || 0,
      z: Number(data.z) || 0,
      a: Number(data.a) || 0,
      limit: Number(data.limit) || 80
    };
    const index = radarbot.markers.findIndex((item) => item.id === marker.id);
    if (index >= 0) radarbot.markers[index] = marker;
    else radarbot.markers.push(marker);
    radarbot.count = radarbot.markers.length;
    return renderRadarBot();
  }
  if (type === "radarbot_ready") {
    radarbot.ready = true;
    radarbot.active = String(data.active || "0") === "1";
    radarbot.count = Number(data.count) || radarbot.markers.length;
    return renderRadarBot();
  }
  if (type === "radarbot_status") {
    radarbot.active = String(data.active || "0") === "1";
    const message = decode(data.message || "").replace(/_/g, " ");
    renderRadarBot();
    if (message) showToast(message);
  }
}

function renderBankExtract() {
  const box = $("#bankExtract");
  if (!box) return;
  const direction = bank.extractTab === "out" ? "out" : "in";
  $all("[data-bank-extract]").forEach((button) => {
    button.classList.toggle("active", button.dataset.bankExtract === direction);
  });
  const rows = bank.transactions.filter((txn) => txn.direction === direction).slice(0, BANK_EXTRACT_LIMIT);
  if (!rows.length) {
    box.innerHTML = `<p class="empty">Nenhum Pix ${direction === "out" ? "enviado" : "recebido"} ainda.</p>`;
    return;
  }
  box.innerHTML = rows.map((txn) => `
    <button class="bank-txn ${txn.direction === "out" ? "out" : "in"}" type="button">
      <i></i>
      <span><small>${esc(txn.date || "HOJE")} - ${esc(txn.note || "Movimentacao")}</small><strong>${esc(txn.label || "Mercantil")}</strong><em>${formatMoney(txn.direction === "out" ? -txn.amount : txn.amount, true)}</em></span>
    </button>
  `).join("");
}

function bankPeople() {
  const base = bank.recipients.slice();
  contacts.filter((contact) => !contact.self).forEach((contact) => {
    if (base.some((item) => item.name === contact.name)) return;
    const id = String(contact.phone || "").replace(/[^\d]/g, "").slice(0, 6);
    if (id) base.push({ id, name: contact.name, bank: "Mercantil", agency: "0001", account: bankAccountFromId(id) });
  });
  return base;
}

function renderBankPeople() {
  const recent = $("#bankPixRecent");
  const all = $("#bankPixContacts");
  if (!recent || !all) return;
  const people = bankPeople();
  const row = (person) => `
    <button class="bank-contact-row" type="button" data-bank-pix-id="${esc(person.id)}" data-bank-pix-name="${esc(person.name)}" data-bank-pix-bank="${esc(person.bank || "Mercantil")}" data-bank-pix-agency="${esc(person.agency || "0001")}" data-bank-pix-account="${esc(person.account || bankAccountFromId(person.id))}">
      <span>${esc(initials(person.name))}</span><span><b>${esc(person.name)}</b><small>${esc(person.bank || "Mercantil")} - ${esc(person.agency || "0001")} - ${esc(person.account || bankAccountFromId(person.id))}</small></span><svg><use href="#i-back"/></svg>
    </button>`;
  recent.innerHTML = people.slice(0, 2).map(row).join("") || `<p class="empty">Busque pelo ID do jogador.</p>`;
  all.innerHTML = people.map(row).join("") || `<p class="empty">Nenhum contato bancario.</p>`;
}

function setBankTarget(person) {
  if (!person || !person.id) return;
  bank.target = {
    id: String(person.id),
    name: person.name || `ID ${person.id}`,
    bank: person.bank || "Mercantil",
    agency: person.agency || "0001",
    account: person.account || bankAccountFromId(person.id)
  };
  if (!bank.recipients.some((item) => String(item.id) === String(bank.target.id))) bank.recipients.unshift(bank.target);
  bank.amountDigits = "";
  renderBank();
  showView("bankAmount");
}

function lookupBankTarget(rawId) {
  const id = String(rawId || $("#bankPixKey")?.value || "").replace(/[^\d]/g, "");
  if (!id) return showToast("Digite o ID do jogador.");
  if (String(profile.accountId) === id) return showToast("Voce nao pode transferir para voce mesmo.");
  bank.pending = true;
  renderBank();
  bridge?.action?.("bank_lookup", `target=${encodePayload(id)}`);
  if (!bridge) {
    window.setTimeout(() => setBankTarget({ id, name: `Jogador ${id}`, bank: "Mercantil", agency: "0001", account: bankAccountFromId(id) }), 180);
  }
}

function renderBankAmount() {
  const input = $("#bankAmountInput");
  if (!input) return;
  const cents = Number(bank.amountDigits || 0);
  input.value = cents > 0 ? formatMoney(cents).replace("R$ ", "") : "";
  const target = bank.target;
  $("#bankTargetName").textContent = target?.name || "Jogador";
  $("#bankTargetId").textContent = target?.id || "0";
  $("#bankTargetAvatar").textContent = initials(target?.name || "?");
  $("#bankContinueAmount").disabled = cents < 1 || cents > bank.balance;
}

function pushBankKey(key) {
  if (key === "clear") bank.amountDigits = "";
  else if (key === "back") bank.amountDigits = bank.amountDigits.slice(0, -1);
  else if (/^\d$/.test(key) && bank.amountDigits.length < 12) {
    bank.amountDigits = (bank.amountDigits + key).replace(/^0+(?=\d)/, "");
  }
  renderBankAmount();
}

function bankAmountCents() {
  return Math.max(0, Number(bank.amountDigits || 0));
}

function renderBankReview() {
  const target = bank.target;
  const amount = bankAmountCents();
  const html = !target ? "" : `
    <p><b>Valor</b><span>${formatMoney(amount)}</span></p>
    <p><b>Para</b><span>${esc(target.name)}</span></p>
    <p><b>Chave Pix</b><span>ID ${esc(target.id)}</span></p>
    <p><b>Banco</b><span>${esc(target.bank)}</span></p>
    <p><b>Agencia</b><span>${esc(target.agency)}</span></p>
    <p><b>Conta</b><span>${esc(target.account)}</span></p>
    <p><b>Tipo de conta</b><span>Corrente</span></p>
    <p><b>Data</b><span>Hoje ${esc(nowText())}</span></p>`;
  const review = $("#bankReviewCard");
  const receipt = $("#bankReceiptCard");
  if (review) review.innerHTML = html;
  if (receipt) {
    const done = bank.receipt || { target, amount, time: nowText(), id: "local" };
    receipt.innerHTML = `
      <p><b>Valor</b><span>${formatMoney(done.amount)}</span></p>
      <p><b>Para</b><span>${esc(done.target?.name || target?.name || "")}</span></p>
      <p><b>Chave Pix</b><span>ID ${esc(done.target?.id || target?.id || "")}</span></p>
      <p><b>Codigo</b><span>${esc(done.id || "local")}</span></p>
      <p><b>Horario</b><span>${esc(done.time || nowText())}</span></p>`;
  }
}

function confirmBankTransfer() {
  const amount = bankAmountCents();
  if (!bank.target || amount < 1) return showToast("Informe o destino e o valor.");
  if (amount > bank.balance) return showToast("Saldo insuficiente.");
  bank.pending = true;
  $("#bankConfirmTransfer").disabled = true;
  bridge?.action?.("bank_transfer", `target=${encodePayload(bank.target.id)};amount=${amount}`);
  if (!bridge) {
    window.setTimeout(() => receiveFromNative(`type=bank_transfer;status=ok;target=${bank.target.id};name=${encodePayload(bank.target.name)};amount=${amount};balance=${bank.balance - amount};tx=PREVIEW${Date.now()}`), 260);
  }
}

function applyBankTransfer(data) {
  bank.pending = false;
  $("#bankConfirmTransfer") && ($("#bankConfirmTransfer").disabled = false);
  if (decode(data.status) !== "ok") {
    showToast(decode(data.message) || "Transferencia recusada.");
    return;
  }
  const amount = Math.max(0, Number(decode(data.amount || "")) || bankAmountCents());
  const target = {
    id: decode(data.target || bank.target?.id || ""),
    name: decode(data.name || bank.target?.name || "Jogador"),
    bank: decode(data.bank || bank.target?.bank || "Mercantil"),
    agency: decode(data.agency || bank.target?.agency || "0001"),
    account: decode(data.account || bank.target?.account || bankAccountFromId(data.target))
  };
  if (data.balance !== undefined) bank.balance = Number(decode(data.balance)) || 0;
  else bank.balance = Math.max(0, bank.balance - amount);
  bank.moneyOut += amount;
  bank.target = target;
  bank.receipt = { id: decode(data.tx || `PIX${Date.now()}`), target, amount, time: eventTime(data) };
  addBankTransaction({ id: bank.receipt.id, direction: "out", label: target.name, note: "Pix enviado", amount, date: "HOJE" }, true);
  bank.amountDigits = "";
  renderBank();
  showView("bankReceipt");
}

function updateBankFromEvent(data) {
  if (data.balance !== undefined) bank.balance = Number(decode(data.balance)) || 0;
  if (data.moneyIn !== undefined) bank.moneyIn = Number(decode(data.moneyIn)) || 0;
  if (data.moneyOut !== undefined) bank.moneyOut = Number(decode(data.moneyOut)) || 0;
  if (data.creditLimit !== undefined) bank.creditLimit = Number(decode(data.creditLimit)) || 0;
  if (data.creditUsed !== undefined) bank.creditUsed = Number(decode(data.creditUsed)) || 0;
  if (data.creditBlocked !== undefined) bank.creditBlocked = data.creditBlocked === "1";
  if (data.dirty !== undefined) bank.dirty = data.dirty === "1";
  if (data.movementScore !== undefined) bank.movementScore = Number(decode(data.movementScore)) || 0;
  if (data.invoiceAmount !== undefined) bank.invoiceAmount = Number(decode(data.invoiceAmount)) || 0;
  if (data.financingPaid !== undefined) bank.financingPaid = Number(decode(data.financingPaid)) || 0;
  if (data.financingTotal !== undefined) bank.financingTotal = Number(decode(data.financingTotal)) || 0;
  if (data.financeCount !== undefined) {
    bank.financeItems = [];
    bank.selectedInvoiceMask = 0;
  }
  if (data.financeItem === "1") {
    const item = {
      id: Number(decode(data.financeId)) || 0,
      name: decode(data.financeName || "Veiculo"),
      totalCents: Number(decode(data.financeTotalCents)) || 0,
      installmentCents: Number(decode(data.financeInstallment)) || 0,
      installments: Number(decode(data.financeInstallments)) || 0,
      paidMask: Number(decode(data.financePaidMask)) || 0
    };
    const existing = bank.financeItems.findIndex((entry) => entry.id === item.id);
    if (existing >= 0) bank.financeItems[existing] = item;
    else bank.financeItems.push(item);
    if (!bank.selectedFinanceId || !bank.financeItems.some((entry) => entry.id === bank.selectedFinanceId)) bank.selectedFinanceId = item.id;
  }
  if (data.financeInstallments !== undefined) bank.financingSlots = Number(decode(data.financeInstallments)) || 0;
  if (data.financeId !== undefined) bank.financingId = Number(decode(data.financeId)) || 0;
  if (data.financeTotalCents !== undefined) bank.financingTotalCents = Number(decode(data.financeTotalCents)) || 0;
  if (data.financeInstallment !== undefined) bank.financingInstallment = Number(decode(data.financeInstallment)) || 0;
  if (data.financePaidMask !== undefined) bank.financingPaidMask = Number(decode(data.financePaidMask)) || 0;
  if (bank.financeItems.length && !bank.financeItems.some((entry) => entry.id === bank.selectedFinanceId)) bank.selectedFinanceId = bank.financeItems[0].id;
  bank.selectedInvoiceMask &= financeFullMask() & ~financeValue("paidMask", bank.financingPaidMask);
  if (data.agency) bank.agency = decode(data.agency);
  if (data.account) bank.account = decode(data.account);
  if (data.invoiceDue) bank.invoiceDue = decode(data.invoiceDue);
  renderBank();
}

function addBankTransaction(txn, front = true) {
  const id = txn.id || `${txn.direction}:${txn.amount}:${txn.label}:${txn.note}:${txn.date}`;
  if (bank.transactionIds.has(id)) return false;
  bank.transactionIds.add(id);
  const normalized = { ...txn, id };
  if (front) bank.transactions.unshift(normalized);
  else bank.transactions.push(normalized);
  trimBankTransactions();
  return true;
}

function trimBankTransactions() {
  const counts = { in: 0, out: 0 };
  for (let index = 0; index < bank.transactions.length; index += 1) {
    const txn = bank.transactions[index];
    const direction = txn.direction === "out" ? "out" : "in";
    counts[direction] += 1;
    if (counts[direction] > BANK_EXTRACT_LIMIT) {
      const removed = bank.transactions.splice(index, 1)[0];
      if (removed?.id) bank.transactionIds.delete(removed.id);
      index -= 1;
    }
  }
}

function addBankTransactionFromEvent(data) {
  const amount = Math.max(0, Number(decode(data.amount || "")) || 0);
  if (!amount) return;
  addBankTransaction({
    id: decode(data.tx || ""),
    direction: decode(data.direction || "out"),
    label: decode(data.label || data.name || "Mercantil"),
    note: decode(data.note || "Movimentacao"),
    amount,
    date: decode(data.date || "HOJE")
  }, data.history !== "1");
  renderBank();
}

function isInvoicePayment(txn) {
  const text = `${txn.note || ""} ${txn.label || ""}`.toLowerCase();
  return text.includes("fatura");
}

function renderBankInvoiceHistory() {
  const box = $("#bankInvoiceHistory");
  if (!box) return;
  const paid = bank.transactions.filter(isInvoicePayment).slice(0, 8);
  if (!paid.length) {
    box.innerHTML = `<p class="bank-history-empty">Nenhuma fatura paga ainda.</p>`;
    return;
  }
  box.innerHTML = paid.map((txn) => `
    <div class="bank-history-row">
      <span><b>${esc(txn.label || "Fatura Mercantil")}</b><small>${esc(txn.date || "HOJE")} - ${esc(txn.note || "Pagamento de fatura")}</small></span>
      <strong>${formatMoney(txn.amount)}</strong>
    </div>
  `).join("");
}

function renderBankFinances() {
  const box = $("#bankFinances");
  if (!box) return;
  if (!bank.financeItems.length) {
    box.innerHTML = `<p class="bank-history-empty">Nenhum veiculo financiado.</p>`;
    return;
  }
  box.innerHTML = bank.financeItems.map((item) => `
    <button class="bank-finance-row${item.id === bank.selectedFinanceId ? " selected" : ""}" type="button" data-bank-finance="${item.id}">
      <span><b>${esc(item.name)}</b><small>${financePaidCount(item.paidMask)}/${item.installments} parcelas pagas</small></span>
      <strong>${formatMoney(financePendingAmount(item))}</strong>
    </button>
  `).join("");
}

function renderBankInstallments() {
  const box = $("#bankInstallments");
  if (!box) return;
  const item = activeFinance();
  const slots = financeValue("installments", bank.financingSlots || bank.financingTotal);
  if (!item || slots < 1 || bank.creditUsed < 1) {
    box.innerHTML = `<p class="bank-history-empty">Escolha um veiculo financiado.</p>`;
    bank.selectedInvoiceMask = 0;
    return;
  }
  box.innerHTML = Array.from({ length: slots }, (_, index) => {
    const bit = 1 << index;
    const paid = Boolean(item.paidMask & bit);
    const selected = Boolean(bank.selectedInvoiceMask & bit);
    return `
      <button class="bank-installment${paid ? " paid" : ""}${selected ? " selected" : ""}" type="button" data-bank-installment="${index}" ${paid ? "disabled" : ""}>
        <b>${paid ? "Pago" : selected ? "Selecionada" : "Em aberto"} ${index + 1}/${slots}</b>
        <small>${formatMoney(financeInstallmentAmount(index))}</small>
      </button>`;
  }).join("");
}

function currentInvoiceAmount(mode = "selected") {
  if (mode === "all") return bank.creditUsed;
  if (mode === "selected") return bank.financingTotal > 0 ? selectedInvoiceAmount() : bank.creditUsed;
  return bank.financingTotal > 0 ? nextInvoiceAmount() : bank.creditUsed;
}

function toggleFinanceInstallment(index) {
  const bit = 1 << Number(index || 0);
  const item = activeFinance();
  if (!bit || !item || (item.paidMask & bit)) return;
  bank.selectedInvoiceMask ^= bit;
  renderBank();
}

function payBankInvoice(mode = "selected") {
  const item = activeFinance();
  if (mode === "selected" && item && !bank.selectedInvoiceMask) return showToast("Selecione uma ou mais parcelas.");
  const amount = currentInvoiceAmount(mode);
  if (amount < 1) return showToast("Nao existe fatura em aberto.");
  if (bank.balance < amount) return showToast("Saldo insuficiente para pagar a fatura.");
  const mask = mode === "all" ? financeFullMask() : bank.selectedInvoiceMask;
  bridge?.action?.("bank_invoice_pay", `mode=${encodePayload(mode)};financeId=${item?.id || 0};mask=${mask}`);
  if (!bridge) {
    bank.balance -= amount;
    bank.moneyOut += amount;
    addBankTransaction({ id: `INVOICE${Date.now()}`, direction: "out", label: "Fatura Mercantil", note: mode === "all" ? "Quitacao de financiamento" : "Pagamento de parcelas", amount, date: "HOJE" }, true);
    bank.creditUsed = Math.max(0, bank.creditUsed - amount);
    bank.financingPaidMask |= mask;
    bank.financingPaid = financePaidCount();
    if (mode === "all" || bank.creditUsed < 1) bank.financingPaidMask = financeFullMask();
    bank.selectedInvoiceMask = 0;
    bank.dirty = false;
    bank.creditBlocked = false;
    renderBank();
    showToast(mode === "all" ? "Financiamento quitado." : "Parcelas pagas.");
  }
}

function renderBankShareContacts() {
  const box = $("#bankShareContacts");
  if (!box) return;
  const rows = contacts.filter((contact) => !contact.self && contact.phone);
  if (!rows.length) {
    box.innerHTML = `<p class="empty">Salve um contato no WhatsApp para compartilhar.</p>`;
    return;
  }
  box.innerHTML = rows.map((contact) => `
    <button class="bank-contact-row" type="button" data-bank-share-phone="${esc(contact.phone)}" data-bank-share-name="${esc(contact.name)}">
      <span>${esc(initials(contact.name))}</span><span><b>${esc(contact.name)}</b><small>${esc(contact.phone)}</small></span><svg><use href="#i-back"/></svg>
    </button>`).join("");
}

function shareBankReceipt(phone, name) {
  if (!bank.receipt || !phone) return;
  const text = `Comprovante Pix Mercantil: ${formatMoney(bank.receipt.amount)} para ${bank.receipt.target.name} as ${bank.receipt.time}.`;
  const index = ensureChat(phone, name || phone, true);
  chats[index].messages.push({ from: "me", kind: "text", text, time: nowText() });
  chats[index].preview = text;
  chats[index].time = nowText();
  bridge?.action?.("message", `to=${encodePayload(phone)};text=${encodePayload(text.slice(0, 118))}`);
  setCurrentChat(index);
  showView("chat");
}
function saveContactLocal(phone, name) {
  if (!phone || !name) return;
  const existing = findContact(phone);
  if (existing) {
    existing.name = name;
    existing.note = phone;
    existing.avatar = initials(name);
  } else {
    contacts.push({ name, phone, note: phone, avatar: initials(name) });
  }
  const chatIndex = findChat(phone);
  if (chatIndex >= 0) {
    chats[chatIndex].name = name;
    chats[chatIndex].avatar = initials(name);
  }
}
function isBlockedPhone(phone) {
  const normalized = normalizePhone(phone) || phone;
  return blockedContacts.some((contact) => contact.phone === normalized);
}
function blockContact(chat) {
  if (!chat?.phone) return;
  const phone = normalizePhone(chat.phone) || chat.phone;
  const name = chat.name || findContact(phone)?.name || phone;
  const existing = blockedContacts.find((contact) => contact.phone === phone);
  if (existing) {
    existing.name = name;
    existing.avatar = initials(name);
  } else {
    blockedContacts.unshift({ phone, name, avatar: initials(name) });
  }
  saveBlockedContacts();
  renderConfig();
}
function unblockContact(phone) {
  const normalized = normalizePhone(phone) || phone;
  const index = blockedContacts.findIndex((contact) => contact.phone === normalized);
  if (index >= 0) {
    blockedContacts.splice(index, 1);
    saveBlockedContacts();
    renderConfig();
    showToast("Contato desbloqueado.");
  }
}
function instagramBlockedProfiles() {
  const blocked = [...instagram.conversations, ...instagram.users].filter((person) => person.blocked);
  return [...new Map(blocked.map((person) => [String(person.id), person])).values()];
}
function unblockInstagramProfile(account) {
  const person = [...instagram.conversations, ...instagram.users].find((item) => String(item.id) === String(account));
  if (!person) return;
  instagram.conversations.forEach((item) => {
    if (String(item.id) === String(account)) item.blocked = false;
  });
  instagram.users.forEach((item) => {
    if (String(item.id) === String(account)) item.blocked = false;
  });
  sendInstagramAction("insta_block", `target=${encodePayload(account)};on=0`);
  renderInstagramDirect();
  renderConfig();
  showToast("Perfil desbloqueado.");
}
function ensureChat(phone, name = "", online = false) {
  const normalized = normalizePhone(phone) || phone;
  let index = findChat(normalized);
  if (index >= 0) {
    chats[index].online = !!online;
    return index;
  }
  const contact = findContact(normalized);
  const title = contact?.name || name || normalized;
  chats.unshift({ phone: normalized, name: title, avatar: initials(title), online, preview: "", time: nowText(), unread: 0, messages: [] });
  return 0;
}
function ensureChatForContact(index) {
  const contact = contacts[Number(index)];
  if (!contact || contact.self) return -1;
  const chatIndex = ensureChat(contact.phone, contact.name);
  return chatIndex;
}
function setCurrentChat(index) {
  const next = Number(index);
  state.currentChat = chats[next] ? next : -1;
  const chat = chats[state.currentChat];
  if (!chat) return;
  $("#chatName").textContent = chat.name;
  $("#chatStatus").textContent = chat.online ? "online agora" : "offline";
  $("#chatAvatar").textContent = initials(chat.name);
  $("#startCallButton").dataset.phone = chat.phone;
  $("#startCallButton").dataset.name = chat.name;
  $("#saveCurrentContact").hidden = !!findContact(chat.phone);
  $("#blockCurrentContact").textContent = isBlockedPhone(chat.phone) ? "Bloqueado" : "Bloquear";
  chat.unread = 0;
  bridge?.action?.("read", `phone=${encodePayload(chat.phone)}`);
  renderChats();
  renderMessages();
}

function unreadTotal() { return chats.reduce((sum, chat) => sum + (Number(chat.unread) || 0), 0); }
function setBadge(selector, value) {
  const badge = $(selector);
  if (!badge) return;
  badge.textContent = value;
  badge.hidden = value < 1;
}
function syncBadges() {
  const total = unreadTotal();
  setBadge("#homeWhatsBadge", total);
  setBadge("#unreadFilterCount", total);
  setBadge("#chatTabBadge", total);
  renderInstagramBadges();
}
function renderChats() {
  const list = $("#chatList");
  const query = state.query.trim().toLowerCase();
  const rows = chats
    .map((chat, index) => ({ chat, index }))
    .filter(({ chat }) => state.filter !== "unread" || chat.unread > 0)
    .filter(({ chat }) => !query || `${chat.name} ${chat.phone} ${chat.preview}`.toLowerCase().includes(query));
  if (!rows.length) {
    list.innerHTML = `<p class="empty">${query ? "Nenhuma conversa encontrada." : "Nenhuma conversa ainda."}</p>`;
    syncBadges();
    return;
  }
  list.innerHTML = rows.map(({ chat, index }) => `
    <button class="chat-row" type="button" data-chat="${index}">
      ${makeAvatar(chat.name)}
      <span class="main">
        <span class="topline"><strong>${esc(chat.name)}</strong><small class="time">${esc(chat.time)}</small></span>
        <span class="preview-line"><small class="preview">${esc(chat.preview || chat.phone)}</small>${chat.unread ? `<em class="unread">${chat.unread}</em>` : ""}</span>
      </span>
    </button>
  `).join("");
  syncBadges();
}
function scheduleRenderChats() {
  if (renderQueue.chats || renderQueue.all) return;
  renderQueue.chats = window.requestAnimationFrame(() => {
    renderQueue.chats = 0;
    renderChats();
  });
}
function scheduleRenderMessages() {
  if (renderQueue.messages || renderQueue.all) return;
  renderQueue.messages = window.requestAnimationFrame(() => {
    renderQueue.messages = 0;
    renderMessages();
  });
}
function scheduleRenderAll() {
  if (renderQueue.all) return;
  if (renderQueue.chats) {
    window.cancelAnimationFrame(renderQueue.chats);
    renderQueue.chats = 0;
  }
  if (renderQueue.messages) {
    window.cancelAnimationFrame(renderQueue.messages);
    renderQueue.messages = 0;
  }
  renderQueue.all = window.requestAnimationFrame(() => {
    renderQueue.all = 0;
    renderAll();
  });
}
function renderContacts() {
  $("#contactCount").textContent = `${Math.max(contacts.length - 1, 0)} contatos`;
  $("#contactList").innerHTML = contacts.map((contact, index) => `
    <button class="contact-row" type="button" data-contact="${index}">
      ${makeAvatar(contact.name, contact.self)}
      <span><strong>${esc(contact.name)}</strong><small>${esc(contact.note || contact.phone)}</small></span>
    </button>
  `).join("");
}
function renderMessages() {
  const chat = chats[state.currentChat];
  const box = $("#messages");
  if (!chat || !chat.messages.length) {
    box.innerHTML = `<p class="empty">Nenhuma mensagem ainda.</p>`;
    return;
  }
  const visibleMessages = chat.messages.slice(-80);
  const autoloadImages = getAutoloadImageIndexes(visibleMessages);
  box.innerHTML = visibleMessages.map((message, index) => renderMessage(message, autoloadImages.has(index))).join("");
  scrollMessagesToEnd(box);
}
function getAutoloadImageIndexes(messages) {
  const indexes = new Set();
  let remaining = AUTOLOAD_IMAGE_LIMIT;
  for (let index = messages.length - 1; index >= 0 && remaining > 0; index -= 1) {
    if (messages[index]?.kind === "image" && messages[index]?.url) {
      indexes.add(index);
      remaining -= 1;
    }
  }
  return indexes;
}
function scrollMessagesToEnd(box = $("#messages")) {
  if (!box) return;
  const move = () => { box.scrollTop = Math.max(0, box.scrollHeight - box.clientHeight + 2); };
  window.requestAnimationFrame(() => {
    move();
    window.setTimeout(move, 60);
  });
}
function renderMessage(message, autoloadImage = true) {
  const side = message.from === "me" ? "me" : "them";
  if (message.kind === "image") {
    const rawUrl = message.url || "";
    const resolvedUrl = resolveMediaUrl(rawUrl);
    const url = esc(resolvedUrl);
    const content = autoloadImage && resolvedUrl
      ? `<img src="${url}" alt="" loading="lazy" decoding="async" />`
      : imagePlaceholderHtml(rawUrl ? "Carregando foto" : "Imagem indisponivel");
    return `
      <div class="bubble ${side} image">
        <button class="image-card${autoloadImage && resolvedUrl ? "" : " pending"}" type="button" data-image-url="${esc(rawUrl)}">
          ${content}
        </button>
        <time>${esc(message.time)}</time>
      </div>
    `;
  }
  if (message.kind === "audio") {
    const bars = Array.from({ length: 15 }, () => "<span></span>").join("");
    const duration = Math.max(1, Math.min(60, Number(message.duration) || 1));
    return `
      <div class="bubble ${side} audio">
        <button class="audio-play" type="button" data-audio-url="${esc(message.url)}" data-audio-duration="${duration}" aria-label="Reproduzir audio">${icon("i-play")}</button>
        <span class="audio-wave" aria-hidden="true">${bars}</span>
        <strong class="audio-duration" data-original-duration="${duration}">${formatVoiceDuration(duration)}</strong>
        <time>${esc(message.time)}</time>
      </div>
    `;
  }
  if (message.kind === "location") {
    return `
      <div class="bubble ${side} location">
        <button class="location-card" type="button" data-location-x="${esc(message.x)}" data-location-y="${esc(message.y)}" data-location-z="${esc(message.z)}">
          <span class="location-map"></span>
          <strong>Localizacao atual</strong>
          <small>Toque para marcar no mapa</small>
        </button>
        <time>${esc(message.time)}</time>
      </div>
    `;
  }
  return `<div class="bubble ${side}">${esc(message.text)}<time>${esc(message.time)}</time></div>`;
}
function renderStatus() {
  $("#myStatusNote").textContent = statuses[0]?.note || "Nenhum status postado";
  $("#statusList").innerHTML = statuses.length ? statuses.map((status) => `<button class="row" type="button"><span class="avatar story">${esc(initials(status.name))}</span><span><strong>${esc(status.name)}</strong><small>${esc(status.note)}</small></span></button>`).join("") : `<p class="empty">Nenhum status recente.</p>`;
}
function renderGroups() {
  $("#groupList").innerHTML = groups.length ? groups.map((group) => `<button class="group-row" type="button"><span class="avatar">${esc(initials(group.name))}</span><span><strong>${esc(group.name)}</strong><small>${esc(group.note || "Grupo criado")}</small></span></button>`).join("") : `<p class="empty">Nenhum grupo criado.</p>`;
}
function renderCalls() {
  $("#callList").innerHTML = calls.length ? calls.map((call) => `<button class="call-row" type="button" data-call="${esc(call.phone)}" data-call-name="${esc(call.name)}">${makeAvatar(call.name)}<span><strong>${esc(call.name)}</strong><small>${esc(call.note)}</small></span>${icon("i-phone")}</button>`).join("") : `<p class="empty">Nenhuma ligacao registrada.</p>`;
}
function renderConfig() {
  const wallpaperLabel = $("#configWallpaperLabel");
  if (wallpaperLabel) wallpaperLabel.textContent = wallpaperNames[phoneSettings.wallpaper] || "Principal";
  const instagramBlocked = instagramBlockedProfiles();
  const blockedTotal = blockedContacts.length + instagramBlocked.length;
  const blockedCount = $("#configBlockedCount");
  if (blockedCount) {
    blockedCount.textContent = blockedTotal
      ? `${blockedTotal} bloqueado${blockedTotal > 1 ? "s" : ""}`
      : "Nenhum bloqueado";
  }
  $all("[data-notify-toggle]").forEach((button) => {
    const app = button.dataset.notifyToggle;
    button.classList.toggle("enabled", phoneSettings.notifications[app] !== false);
  });
  $all("[data-wallpaper]").forEach((button) => {
    button.classList.toggle("active", button.dataset.wallpaper === phoneSettings.wallpaper);
  });
  const blockedList = $("#configBlockedList");
  if (blockedList) {
    if (!blockedTotal) {
      blockedList.innerHTML = `<p class="config-empty">Nenhum perfil ou contato bloqueado.</p>`;
    } else {
      const whatsappRows = blockedContacts.map((contact) => `
        <div class="blocked-row">
          <span class="avatar">${esc(contact.avatar || initials(contact.name || contact.phone))}</span>
          <span><strong>${esc(contact.name || contact.phone)}</strong><small>${esc(contact.phone)}</small></span>
          <button type="button" data-unblock-phone="${esc(contact.phone)}">Desbloquear</button>
        </div>
      `).join("");
      const instagramRows = instagramBlocked.map((person) => `
        <div class="blocked-row">
          ${instagramAvatar(person)}
          <span><strong>${esc(person.name || person.username || person.id)}</strong><small>Instagram - @${esc(person.username || person.id)}</small></span>
          <button type="button" data-unblock-instagram="${esc(person.id)}">Desbloquear</button>
        </div>
      `).join("");
      blockedList.innerHTML = `${whatsappRows}${instagramRows}`;
    }
  }
}
function sportfyTrackById(video) {
  const playlistTracks = [];
  sportfy.playlists.forEach((playlist) => playlistTracks.push(...(playlist.tracks || [])));
  return [sportfy.current, ...sportfy.queue, ...sportfy.results, ...sportfy.history, ...sportfy.favorites, ...playlistTracks]
    .filter(Boolean)
    .find((track) => track.id === video) || null;
}
function sportfyNormalizeTrack(item) {
  const id = item?.videoId || item?.id || item?.video || "";
  if (!id) return null;
  const duration = Number(item?.lengthSeconds || item?.duration || 0) || 0;
  return {
    id,
    title: String(item?.title || item?.name || id).slice(0, 90),
    author: String(item?.author || item?.artist || "YouTube").slice(0, 64),
    duration,
    thumb: item?.videoThumbnails?.[1]?.url || item?.videoThumbnails?.[0]?.url || item?.thumb || `https://i.ytimg.com/vi/${id}/hqdefault.jpg`
  };
}
function sportfyFormatDuration(value) {
  const seconds = Math.max(0, Number(value) || 0);
  return seconds ? `${Math.floor(seconds / 60)}:${String(seconds % 60).padStart(2, "0")}` : "";
}
function sportfyDurationSeconds(track) {
  return Math.max(0, Number(track?.duration) || 0);
}
function sportfyFilteredSeeds() {
  if (sportfy.filter === "todos") return SPORTFY_SEED_TRACKS.map(sportfyNormalizeTrack).filter(Boolean);
  return SPORTFY_SEED_TRACKS.filter((track) => track.tag === sportfy.filter).map(sportfyNormalizeTrack).filter(Boolean);
}
function sportfyFallbackSearch(query) {
  const term = query.toLowerCase().trim();
  if (!term) return [];
  const rows = SPORTFY_SEED_TRACKS.filter((track) => {
    const haystack = `${track.title} ${track.author} ${track.tag}`.toLowerCase();
    return haystack.includes(term) || term.includes(track.tag);
  });
  return (rows.length ? rows : SPORTFY_SEED_TRACKS).map(sportfyNormalizeTrack).filter((track) => track && !sportfy.unavailable.has(track.id)).slice(0, 12);
}
function sportfyApplySearchRows(rows, serial) {
  if (Number(serial) !== sportfy.searchSerial) return false;
  const normalized = (Array.isArray(rows) ? rows : [])
    .map(sportfyNormalizeTrack)
    .filter((track) => track && !sportfy.unavailable.has(track.id))
    .slice(0, 18);
  if (!normalized.length) return false;
  sportfy.results = normalized;
  sportfyPrefetchTracks(sportfy.results);
  const empty = $("#sportfySearchEmpty");
  if (empty) empty.textContent = "";
  renderSportfy();
  return true;
}
function sportfyNativeSearch(term, serial) {
  return new Promise((resolve) => {
    if (!bridge?.searchSportfy) return resolve([]);
    const timer = window.setTimeout(() => {
      if (sportfy.nativeSearchResolvers[serial]) delete sportfy.nativeSearchResolvers[serial];
      resolve([]);
    }, 6500);
    sportfy.nativeSearchResolvers[serial] = (rows) => {
      window.clearTimeout(timer);
      delete sportfy.nativeSearchResolvers[serial];
      resolve(Array.isArray(rows) ? rows : []);
    };
    try {
      bridge.searchSportfy(term, serial);
    } catch {
      window.clearTimeout(timer);
      delete sportfy.nativeSearchResolvers[serial];
      resolve([]);
    }
  });
}
window.__sportfyNativeSearchResult = function(serial, term, rows) {
  const resolver = sportfy.nativeSearchResolvers[serial];
  if (resolver) resolver(rows);
  if (String(term || "").trim() === String(sportfy.query || "").trim()) {
    sportfyApplySearchRows(rows, serial);
  }
};
function sportfySaveCache() {
  try {
    localStorage.setItem(scopedStorageKey(SPORTFY_STORAGE_KEY), JSON.stringify({
      favorites: sportfy.favorites.slice(0, 80),
      history: sportfy.history.slice(0, 80),
      playlists: sportfy.playlists.map((playlist) => ({ ...playlist, tracks: (playlist.tracks || []).slice(0, 80) }))
    }));
  } catch {}
}

function sportfyResetAccountCache() {
  sportfy.favorites = [];
  sportfy.history = [];
  sportfy.playlists = [];
  sportfy.libraryPlaylist = "";
  sportfy.current = null;
  sportfy.queue = [];
  sportfy.queueIndex = 0;
  sportfy.results = [];
  sportfy.query = "";
  sportfy.playing = false;
  sportfy.pausedAt = 0;
  sportfy.startedAt = 0;
  window.clearTimeout(sportfy.playbackTimer);
  sportfy.playbackTimer = 0;
  sportfy.playbackVideo = "";
  sportfy.localFrame?.remove();
  sportfy.localFrame = null;
  sportfy.nativeAudio = { active: false, video: "" };
}

function sportfyLoadCache() {
  const scopedAccount = accountStorageSuffix();
  sportfyResetAccountCache();
  sportfyLoadedAccount = scopedAccount;
  try {
    const key = scopedStorageKey(SPORTFY_STORAGE_KEY);
    let raw = localStorage.getItem(key);
    if (!raw && scopedAccount !== "guest") {
      const legacyRaw = localStorage.getItem(SPORTFY_STORAGE_KEY);
      if (legacyRaw) {
        raw = legacyRaw;
        localStorage.setItem(key, legacyRaw);
        localStorage.removeItem(SPORTFY_STORAGE_KEY);
      }
    }
    const data = JSON.parse(raw || "{}");
    if (Array.isArray(data.favorites)) sportfy.favorites = data.favorites.map(sportfyNormalizeTrack).filter(Boolean);
    if (Array.isArray(data.history)) sportfy.history = data.history.map(sportfyNormalizeTrack).filter(Boolean);
    if (Array.isArray(data.playlists)) {
      const loaded = data.playlists.map((playlist, index) => ({
        id: String(playlist.id || `playlist_${index}`),
        name: String(playlist.name || "Minha playlist").slice(0, 32),
        tracks: Array.isArray(playlist.tracks) ? playlist.tracks.map(sportfyNormalizeTrack).filter(Boolean) : []
      })).filter((playlist) => playlist.id !== "trevin");
      sportfy.playlists = loaded;
    }
  } catch {}
}
function sportfyPayloadForTrack(track, extra = "") {
  return `v=${encodePayload(track?.id || "")};t=${encodePayload(String(track?.title || "").slice(0, 32))};a=${encodePayload(String(track?.author || "").slice(0, 22))};d=${Number(track?.duration) || 0}${extra}`;
}
function sportfySendTrackAction(action, track, extra = "") {
  if (track?.id) bridge?.action?.(action, sportfyPayloadForTrack(track, extra));
}
function sportfyPrefetchTracks(tracks) {
  try {
    if (!bridge?.prefetchSportfyAudio) return;
    const ids = (tracks || [])
      .map((track) => sportfyNormalizeTrack(track)?.id)
      .filter((id) => id && !sportfy.unavailable.has(id) && !sportfy.prefetched.has(id))
      .slice(0, 8);
    ids.forEach((id) => sportfy.prefetched.add(id));
    if (ids.length) bridge.prefetchSportfyAudio(ids.join(","));
  } catch {}
}
function sportfyBluetoothStateExtra(playing = sportfy.playing) {
  return `;p=${playing ? 1 : 0};vol=${sportfy.volume};pos=${Math.max(0, Math.floor(sportfyStartedSeconds()))}`;
}
function sportfyReadVehicleAudioState(vehicle = sportfy.bluetooth.vehicle) {
  try {
    const requestedVehicle = Number.isFinite(Number(vehicle)) ? Math.trunc(Number(vehicle)) : -1;
    const raw = bridge?.getSportfyVehicleAudioState?.(requestedVehicle);
    if (!raw) return null;
    const data = typeof raw === "string" ? JSON.parse(raw) : raw;
    return data && data.ok ? data : null;
  } catch {
    return null;
  }
}
function sportfyVehicleSpatialVolume(nativeState, forcePlaying = false) {
  if (!nativeState || !sportfy.current || (!sportfy.playing && !forcePlaying)) return { volume: 0, mode: "" };
  const base = Math.max(0, Math.min(100, Number(sportfy.volume) || 0));
  if (nativeState.inVehicle) return { volume: base, mode: "inside" };
  const distance = Math.max(0, Number(nativeState.distance) || 9999);
  const fullRange = 3.0;
  const maxRange = 42.0;
  if (distance >= maxRange) return { volume: 0, mode: "out" };
  const fade = distance <= fullRange ? 1 : Math.pow(1 - ((distance - fullRange) / (maxRange - fullRange)), 1.35);
  return { volume: Math.max(0, Math.min(42, Math.round(base * 0.52 * fade))), mode: "muffled" };
}
function sportfyStopBluetoothSpatialMonitor() {
  window.clearInterval(sportfy.btSpatialTimer);
  sportfy.btSpatialTimer = 0;
  sportfy.btSpatialLost = 0;
}
function sportfyUpdateConnectedVehicleSpatialAudio(forcePlaying = false) {
  if (!sportfy.bluetooth.connected || sportfy.bluetooth.vehicle < 0 || !sportfy.current) {
    sportfyStopBluetoothSpatialMonitor();
    return;
  }
  const nativeState = sportfyReadVehicleAudioState();
  if (!nativeState) {
    sportfy.btSpatialLost += 1;
    if (sportfy.btSpatialLost >= 10) {
      sportfyStopNativeAmbientSource(sportfy.bluetooth.vehicle);
    }
    return;
  }
  if (Number(nativeState.vehicle) >= 0 && Number(nativeState.vehicle) !== sportfy.bluetooth.vehicle) {
    if (sportfy.bluetooth.vehicle >= 0) sportfyStopNativeAmbientSource(sportfy.bluetooth.vehicle);
    sportfy.bluetooth.vehicle = Number(nativeState.vehicle);
  }
  sportfy.btSpatialLost = 0;
  const spatial = sportfyVehicleSpatialVolume(nativeState, forcePlaying);
  const pos = Math.max(0, Math.floor(sportfyStartedSeconds()));
  const shouldPlay = sportfy.playing || forcePlaying;
  if (!shouldPlay) {
    sportfySetNativeVolume(0);
    sportfyPostPlayerCommand({ func: "setVolume", args: [0] }, sportfy.localFrame);
  } else if (spatial.volume <= 0) {
    sportfySetNativeVolume(0);
    sportfyPostPlayerCommand({ func: "setVolume", args: [0] }, sportfy.localFrame);
  } else if (!sportfy.nativeAudio.active || sportfy.nativeAudio.video !== sportfy.current.id) {
    sportfyPlayNativeAudio(sportfy.current.id, spatial.volume, pos);
  } else {
    sportfySetNativeVolume(spatial.volume);
    sportfyPostPlayerCommand({ func: "setVolume", args: [spatial.volume] }, sportfy.localFrame);
    try {
      sportfy.ytPlayer?.playVideo?.();
      sportfyPostPlayerCommand({ func: "playVideo" }, sportfy.localFrame);
    } catch {}
  }
}
function sportfyStartBluetoothSpatialMonitor() {
  if (!sportfy.bluetooth.connected || sportfy.bluetooth.vehicle < 0) return;
  if (!sportfy.btSpatialTimer) {
    sportfy.btSpatialTimer = window.setInterval(() => sportfyUpdateConnectedVehicleSpatialAudio(false), 180);
  }
  sportfyUpdateConnectedVehicleSpatialAudio(true);
}
function sportfyEnterBluetoothAudioMode() {
  sportfySuppressPlayerEvents(700);
  if (sportfy.bluetooth.vehicle >= 0) sportfyStopNativeAmbientSource(sportfy.bluetooth.vehicle);
  sportfySyncNativeAudioKeepAlive();
}
function sportfyStartConnectedVehicleAudio(immediate = false) {
  if (!sportfy.bluetooth.connected || sportfy.bluetooth.vehicle < 0 || !sportfy.current) return;
  sportfyEnterBluetoothAudioMode();
  sportfyStartBluetoothSpatialMonitor();
  if (immediate) sportfySendBluetoothStateThrottled(true);
}
function sportfySendBluetoothStateThrottled(immediate = false) {
  if (!sportfy.bluetooth.connected || !sportfy.current) return;
  const send = () => {
    window.clearTimeout(sportfy.btStateTimer);
    sportfy.btStateTimer = 0;
    sportfy.btStateLastSent = Date.now();
    sportfySendTrackAction("sportfy_bt_state", sportfy.current, sportfyBluetoothStateExtra());
  };
  if (immediate) return send();
  const elapsed = Date.now() - sportfy.btStateLastSent;
  if (elapsed >= 280) return send();
  window.clearTimeout(sportfy.btStateTimer);
  sportfy.btStateTimer = window.setTimeout(send, 280 - elapsed);
}
function sportfyIsFavorite(video) {
  return sportfy.favorites.some((track) => track.id === video);
}
function sportfyUpsert(list, track, limit = 60) {
  const clean = sportfyNormalizeTrack(track);
  if (!clean) return;
  const index = list.findIndex((item) => item.id === clean.id);
  if (index >= 0) list.splice(index, 1);
  list.unshift(clean);
  list.splice(limit);
}
function sportfyToggleFavorite(track) {
  const clean = sportfyNormalizeTrack(track || sportfy.current);
  if (!clean) return;
  const index = sportfy.favorites.findIndex((item) => item.id === clean.id);
  if (index >= 0) {
    sportfy.favorites.splice(index, 1);
    sportfySendTrackAction("sportfy_fav", clean, ";on=0");
    showToast("Removida das favoritas.");
  } else {
    sportfy.favorites.unshift(clean);
    sportfySendTrackAction("sportfy_fav", clean, ";on=1");
    showToast("Salva nas favoritas.");
  }
  sportfySaveCache();
  renderSportfy();
}
function sportfyAddToPlaylist(track) {
  const clean = sportfyNormalizeTrack(track || sportfy.current);
  if (!clean) return showToast("Escolha uma musica primeiro.");
  const playlist = sportfy.playlists.find((item) => item.id === sportfy.libraryPlaylist) || sportfy.playlists[0];
  if (!playlist) {
    showToast("Crie uma playlist na biblioteca.");
    sportfy.tab = "library";
    renderSportfy();
    return;
  }
  sportfyUpsert(playlist.tracks, clean, 80);
  sportfySaveCache();
  showToast("Adicionada na playlist.");
  renderSportfy();
}
function sportfyCreatePlaylist(name) {
  const title = String(name || "").trim().slice(0, 32);
  if (!title) return showToast("Digite o nome da playlist.");
  const playlist = { id: `playlist_${Date.now()}`, name: title, tracks: [] };
  sportfy.playlists.unshift(playlist);
  sportfy.libraryPlaylist = playlist.id;
  sportfySaveCache();
  renderSportfy();
}
function sportfyDeletePlaylist(id) {
  sportfy.playlists = sportfy.playlists.filter((playlist) => playlist.id !== id);
  if (sportfy.libraryPlaylist === id) sportfy.libraryPlaylist = "";
  sportfySaveCache();
  renderSportfy();
}
function sportfyConfirmPlayback(video = sportfy.playbackVideo) {
  if (!sportfy.current || video !== sportfy.current.id || sportfy.unavailable.has(video)) return;
  sportfyUpsert(sportfy.history, sportfy.current, 80);
  sportfy.history = sportfy.history.filter((track) => !sportfy.unavailable.has(track.id)).slice(0, 80);
  sportfySaveCache();
  sportfySendTrackAction("sportfy_hist", sportfy.current);
  renderSportfy();
}
function sportfyHandleUnavailable(video = sportfy.current?.id) {
  if (!video) return;
  sportfy.unavailable.add(video);
  window.clearTimeout(sportfy.playbackTimer);
  sportfy.results = sportfy.results.filter((track) => track.id !== video);
  sportfy.history = sportfy.history.filter((track) => track.id !== video);
  sportfy.favorites = sportfy.favorites.filter((track) => track.id !== video);
  sportfy.playlists.forEach((playlist) => { playlist.tracks = (playlist.tracks || []).filter((track) => track.id !== video); });
  if (sportfy.current?.id === video) {
    sportfyApplyPlaying(false);
    sportfy.current = null;
    sportfy.localFrame?.remove();
    sportfy.localFrame = null;
    if (!sportfy.ambient?.video) sportfyStopNativeAudio();
  }
  sportfySaveCache();
  showToast("Essa musica nao esta disponivel agora.");
  renderSportfy();
}
function sportfyPostPlayerCommand(command, frame = sportfy.localFrame) {
  try {
    frame?.contentWindow?.postMessage(JSON.stringify({ event: "command", func: command.func, args: command.args || [] }), "*");
  } catch {}
}
function sportfySyncNativeAudioKeepAlive() {
  try {
    const hasAmbient = Object.keys(sportfy.ambientSources || {}).length > 0 || !!sportfy.ambient?.video;
    bridge?.keepAudioAlive?.(!!(sportfy.playing || hasAmbient));
  } catch {}
}
function sportfyAmbientVolume(volume, mode = sportfy.ambient?.mode) {
  const level = Math.max(0, Math.min(100, Number(volume) || 0));
  if (mode === "muffled") return Math.max(1, Math.min(38, level));
  if (mode === "open") return Math.max(4, level);
  return level;
}
function sportfyPlayNativeAudio(video, volume = sportfy.volume, startSeconds = 0) {
  try {
    if (!video) return false;
    if (sportfy.bluetooth.connected && sportfy.ambient?.video === video) sportfySuppressBluetoothState();
    const safeVolume = Math.max(0, Math.min(100, Number(volume) || 0));
    const safeStart = Math.max(0, Math.floor(Number(startSeconds) || 0));
    if (sportfy.ytPlayer?.loadVideoById) {
      sportfy.ytPlayer.setVolume?.(safeVolume);
      if (safeStart > 0) sportfy.ytPlayer.loadVideoById({ videoId: video, startSeconds: safeStart });
      else sportfy.ytPlayer.loadVideoById(video);
      window.setTimeout(() => {
        try {
          sportfy.ytPlayer.setVolume?.(safeVolume);
          sportfy.ytPlayer.unMute?.();
          sportfy.ytPlayer.playVideo?.();
        } catch {}
      }, 80);
    } else {
      sportfyLoadYoutubeApi();
      sportfy.localFrame = sportfyEnsureFrame("#sportfyFrameFallback", video, false, safeStart);
      sportfyPostPlayerCommand({ func: "setVolume", args: [safeVolume] }, sportfy.localFrame);
      if (safeStart > 0) sportfyPostPlayerCommand({ func: "seekTo", args: [safeStart, true] }, sportfy.localFrame);
      sportfyPostPlayerCommand({ func: "playVideo" }, sportfy.localFrame);
    }
    sportfySyncNativeAudioKeepAlive();
    sportfy.nativeAudio = { active: true, video };
    return true;
  } catch {
    return false;
  }
}
function sportfySuppressPlayerEvents(ms = 900) {
  sportfy.suppressPlayerEventsUntil = Math.max(sportfy.suppressPlayerEventsUntil, Date.now() + ms);
}
function sportfyPlayerEventsSuppressed() {
  return Date.now() < sportfy.suppressPlayerEventsUntil;
}
function sportfySuppressBluetoothState(ms = 900) {
  sportfy.suppressBluetoothStateUntil = Math.max(sportfy.suppressBluetoothStateUntil, Date.now() + ms);
}
function sportfyBluetoothStateSuppressed() {
  return Date.now() < sportfy.suppressBluetoothStateUntil;
}
function sportfyStopNativeAudio(silent = false) {
  try {
    if (silent) sportfySuppressPlayerEvents();
    sportfy.ytPlayer?.stopVideo?.();
    sportfy.localFrame?.remove();
    sportfy.localFrame = null;
    bridge?.stopSportfyAudio?.();
  } catch {}
  sportfy.nativeAudio = { active: false, video: "" };
}
function sportfyPauseCurrentAudio() {
  try {
    sportfySuppressPlayerEvents(300);
    sportfy.ytPlayer?.pauseVideo?.();
    sportfyPostPlayerCommand({ func: "pauseVideo" }, sportfy.localFrame);
    bridge?.setSportfyAudioVolume?.(0);
  } catch {}
}
function sportfyResumeCurrentAudio() {
  try {
    if (sportfy.ytPlayer?.playVideo && sportfy.nativeAudio.video === sportfy.current?.id) {
      sportfy.ytPlayer.setVolume?.(sportfy.volume);
      sportfy.ytPlayer.unMute?.();
      sportfy.ytPlayer.playVideo();
      sportfySyncNativeAudioKeepAlive();
      return true;
    }
    if (sportfy.localFrame?.dataset.video === sportfy.current?.id) {
      sportfyPostPlayerCommand({ func: "setVolume", args: [sportfy.volume] }, sportfy.localFrame);
      sportfyPostPlayerCommand({ func: "playVideo" }, sportfy.localFrame);
      sportfySyncNativeAudioKeepAlive();
      return true;
    }
  } catch {}
  return false;
}
function sportfySetNativeVolume(volume = sportfy.volume) {
  try {
    sportfy.ytPlayer?.setVolume?.(Math.max(0, Math.min(100, Number(volume) || 0)));
    bridge?.setSportfyAudioVolume?.(Math.max(0, Math.min(100, Number(volume) || 0)));
  } catch {}
}
function sportfySetAmbientVolumeSmooth(volume) {
  const target = Math.max(0, Math.min(100, Number(volume) || 0));
  sportfy.ambientVolumeTarget = target;
  window.clearInterval(sportfy.ambientVolumeTimer);
  sportfy.ambientVolumeTimer = window.setInterval(() => {
    const current = Number(sportfy.ambientVolumeCurrent) || 0;
    const diff = sportfy.ambientVolumeTarget - current;
    if (Math.abs(diff) <= 1) {
      sportfy.ambientVolumeCurrent = sportfy.ambientVolumeTarget;
      sportfySetNativeVolume(sportfy.ambientVolumeCurrent);
      window.clearInterval(sportfy.ambientVolumeTimer);
      sportfy.ambientVolumeTimer = 0;
      return;
    }
    sportfy.ambientVolumeCurrent = current + (diff * (diff > 0 ? 0.78 : 0.42));
    sportfySetNativeVolume(sportfy.ambientVolumeCurrent);
  }, 45);
}
function sportfySetNativeAmbientSource(vehicle, video, volume, startSeconds = 0, mode = "") {
  const vehicleId = Number.isFinite(Number(vehicle)) ? Math.trunc(Number(vehicle)) : -1;
  const cleanVideo = String(video || "").trim();
  const safeVolume = Math.max(0, Math.min(100, Number(volume) || 0));
  const safeStart = Math.max(0, Math.floor(Number(startSeconds) || 0));
  if (vehicleId < 0 || !cleanVideo || safeVolume <= 0) {
    sportfyStopNativeAmbientSource(vehicleId);
    return;
  }
  sportfy.ambientSources[vehicleId] = {
    vehicle: vehicleId,
    video: cleanVideo,
    volume: safeVolume,
    pos: safeStart,
    mode: String(mode || ""),
    updatedAt: Date.now()
  };
  try {
    bridge?.updateSportfyAmbientAudio?.(vehicleId, cleanVideo, safeVolume, safeStart, String(mode || ""));
  } catch {}
  sportfySyncNativeAudioKeepAlive();
}
function sportfyStopNativeAmbientSource(vehicle = -1) {
  const vehicleId = Number.isFinite(Number(vehicle)) ? Math.trunc(Number(vehicle)) : -1;
  if (vehicleId >= 0) delete sportfy.ambientSources[vehicleId];
  else sportfy.ambientSources = {};
  try {
    bridge?.stopSportfyAmbientAudio?.(vehicleId);
  } catch {}
  sportfySyncNativeAudioKeepAlive();
}
function sportfyLoadYoutubeApi() {
  if (window.YT?.Player) {
    sportfyEnsureYoutubePlayer();
    return;
  }
  if (sportfy.apiLoading) return;
  sportfy.apiLoading = true;
  const previous = window.onYouTubeIframeAPIReady;
  window.onYouTubeIframeAPIReady = () => {
    if (typeof previous === "function") previous();
    sportfyEnsureYoutubePlayer();
  };
  const script = document.createElement("script");
  script.src = "https://www.youtube.com/iframe_api";
  document.head.appendChild(script);
}
function sportfyEnsureYoutubePlayer() {
  if (sportfy.ytPlayer || !window.YT?.Player || !$("#sportfyYoutubePlayer")) return;
  sportfy.ytPlayer = new YT.Player("sportfyYoutubePlayer", {
    height: "1",
    width: "1",
    playerVars: { autoplay: 1, controls: 0, disablekb: 1, enablejsapi: 1, playsinline: 1, rel: 0 },
    events: {
      onReady: (event) => {
        sportfy.playerReady = true;
        event.target.setVolume(sportfy.volume);
        try { event.target.unMute(); } catch {}
        if (sportfy.current) {
          sportfy.localFrame?.remove();
          sportfy.localFrame = null;
          event.target.loadVideoById(sportfy.current.id);
          if (sportfy.playing) {
            window.setTimeout(() => {
              try { event.target.unMute(); event.target.playVideo(); } catch {}
            }, 120);
          }
        }
      },
      onStateChange: (event) => {
        if (!window.YT?.PlayerState) return;
        if (event.data === YT.PlayerState.PLAYING) {
          try { event.target.unMute(); event.target.setVolume(sportfy.bluetooth.connected && sportfy.ambient?.video ? sportfyAmbientVolume(sportfy.ambient.volume) : sportfy.volume); } catch {}
          sportfyApplyPlaying(true);
          sportfyConfirmPlayback(sportfy.current?.id);
        }
        if (event.data === YT.PlayerState.PAUSED) {
          if (sportfyPlayerEventsSuppressed()) return;
          sportfyApplyPlaying(false);
        }
        if (event.data === YT.PlayerState.ENDED) {
          if (sportfyPlayerEventsSuppressed()) return;
          if (sportfy.repeat) sportfyPlay(sportfy.current, sportfy.queue, sportfy.queueIndex);
          else sportfyNextTrack();
        }
      },
      onError: (event) => {
        sportfyApplyPlaying(false);
        console.warn("Sportfy YouTube player error", event?.data);
        showToast("Nao foi possivel iniciar o player agora.");
      }
    }
  });
}
function sportfyEnsureFrame(hostSelector, video, ambient = false, startSeconds = 0) {
  const host = $(hostSelector);
  if (!host || !video) return null;
  const safeStart = Math.max(0, Math.floor(Number(startSeconds) || 0));
  const current = host.querySelector("iframe");
  if (current?.dataset.video === video && Number(current.dataset.start || 0) === safeStart) return current;
  host.innerHTML = "";
  const frame = document.createElement("iframe");
  frame.dataset.video = video;
  frame.dataset.start = String(safeStart);
  frame.allow = "autoplay; encrypted-media; fullscreen";
  frame.setAttribute("allowfullscreen", "true");
  // O telefone roda a partir de file:// no WebView; enviar origin=file:// faz
  // o player do YouTube rejeitar o embed. Sem esse parâmetro o iframe mantém
  // a reprodução iniciada pelo toque do usuário.
  frame.src = `https://www.youtube.com/embed/${encodeURIComponent(video)}?enablejsapi=1&playsinline=1&autoplay=1&controls=0&rel=0&start=${safeStart}`;
  host.appendChild(frame);
  frame.addEventListener("load", () => {
    sportfyPostPlayerCommand({ func: "addEventListener", args: ["onStateChange"] }, frame);
    sportfyPostPlayerCommand({ func: "addEventListener", args: ["onError"] }, frame);
    sportfyPostPlayerCommand({ func: "setVolume", args: [ambient ? sportfyAmbientVolume(sportfy.ambient.volume) : sportfy.volume] }, frame);
    if (safeStart > 0) sportfyPostPlayerCommand({ func: "seekTo", args: [safeStart, true] }, frame);
    sportfyPostPlayerCommand({ func: "playVideo" }, frame);
  });
  return frame;
}
function sportfyHandlePlayerMessage(event) {
  if (!String(event.origin || "").includes("youtube.com")) return;
  let data = event.data;
  try {
    if (typeof data === "string") data = JSON.parse(data);
  } catch {
    return;
  }
  if (!data || typeof data !== "object") return;
  const video = sportfy.localFrame?.dataset.video || sportfy.current?.id || "";
  if (data.event === "onError") {
    console.warn("Sportfy iframe error", video, data);
    showToast("Nao foi possivel iniciar o player agora.");
    return;
  }
  if (data.event === "onStateChange") {
    if (data.info === 1) {
      sportfyApplyPlaying(true);
      sportfyConfirmPlayback(video);
    }
    if (data.info === 2) {
      if (sportfyPlayerEventsSuppressed()) return;
      sportfyApplyPlaying(false);
    }
    if (data.info === 0) {
      if (sportfyPlayerEventsSuppressed()) return;
      sportfyApplyPlaying(false);
      sportfy.repeat ? sportfyPlay(sportfy.current, sportfy.queue, sportfy.queueIndex) : sportfyNextTrack();
    }
  }
}
function sportfyApplyPlaying(playing) {
  const changed = sportfy.playing !== !!playing;
  sportfy.playing = !!playing;
  sportfySyncNativeAudioKeepAlive();
  if (sportfy.bluetooth.connected && sportfy.current && sportfy.playing && !sportfyBluetoothStateSuppressed()) {
    sportfySendBluetoothStateThrottled(changed);
  }
  renderSportfyPlayer();
  renderSportfyNowPlaying();
  sportfyUpdateProgress();
}
function sportfySetLocalPlaying(playing) {
  if (!sportfy.current) return;
  if (playing) {
    sportfy.startedAt = Date.now() - (sportfy.pausedAt * 1000);
    sportfyLoadYoutubeApi();
    if (sportfy.bluetooth.connected) {
      sportfyStartConnectedVehicleAudio(false);
    } else if (sportfyResumeCurrentAudio()) {
      sportfy.nativeAudio = { active: true, video: sportfy.current.id };
    } else if (!sportfyPlayNativeAudio(sportfy.current.id, sportfy.volume)) {
      sportfy.localFrame = sportfyEnsureFrame("#sportfyFrameFallback", sportfy.current.id, false);
      sportfyPostPlayerCommand({ func: "playVideo" }, sportfy.localFrame);
    }
  } else {
    const current = sportfyStartedSeconds();
    sportfy.pausedAt = current;
    if (sportfy.bluetooth.connected) {
      sportfyPauseCurrentAudio();
    } else {
      sportfyPauseCurrentAudio();
    }
  }
  sportfyApplyPlaying(playing);
}
function sportfyPlay(track, nextQueue = sportfy.queue, index = 0) {
  const clean = sportfyNormalizeTrack(track);
  if (!clean) return;
  if (sportfy.unavailable.has(clean.id)) return showToast("Essa musica nao esta disponivel agora.");
  sportfyLoadYoutubeApi();
  sportfy.current = clean;
  sportfy.queue = (nextQueue?.length ? nextQueue : [clean]).map(sportfyNormalizeTrack).filter((track) => track && !sportfy.unavailable.has(track.id));
  sportfy.queueIndex = Math.max(0, index);
  window.clearTimeout(sportfy.playbackTimer);
  sportfy.playbackVideo = clean.id;
  sportfy.pausedAt = 0;
  sportfy.startedAt = Date.now();
  if (sportfy.bluetooth.connected) {
    sportfyStartConnectedVehicleAudio(false);
  } else if (sportfyPlayNativeAudio(clean.id, sportfy.volume)) {
    sportfy.localFrame?.remove();
    sportfy.localFrame = null;
  } else {
    sportfy.localFrame = sportfyEnsureFrame("#sportfyFrameFallback", clean.id, false);
    sportfyPostPlayerCommand({ func: "setVolume", args: [sportfy.volume] }, sportfy.localFrame);
    sportfyPostPlayerCommand({ func: "playVideo" }, sportfy.localFrame);
  }
  sportfy.playbackTimer = window.setTimeout(() => sportfyConfirmPlayback(clean.id), 2600);
  sportfyApplyPlaying(true);
  sportfySaveCache();
  if (sportfy.bluetooth.connected) sportfySendBluetoothStateThrottled(true);
  renderSportfy();
}
function sportfyNextTrack() {
  if (!sportfy.queue.length) return;
  sportfySuppressPlayerEvents(700);
  const nextIndex = sportfy.shuffle ? Math.floor(Math.random() * sportfy.queue.length) : (sportfy.queueIndex + 1) % sportfy.queue.length;
  sportfyPlay(sportfy.queue[nextIndex], sportfy.queue, nextIndex);
}
function sportfyPreviousTrack() {
  if (!sportfy.queue.length) return;
  sportfySuppressPlayerEvents(700);
  const prevIndex = (sportfy.queueIndex - 1 + sportfy.queue.length) % sportfy.queue.length;
  sportfyPlay(sportfy.queue[prevIndex], sportfy.queue, prevIndex);
}
function sportfyFormatClock(seconds) {
  const value = Math.max(0, Math.floor(Number(seconds) || 0));
  return `${Math.floor(value / 60)}:${String(value % 60).padStart(2, "0")}`;
}
function sportfyStartedSeconds() {
  if (!sportfy.current) return 0;
  if (!sportfy.playing) return Math.max(0, Number(sportfy.pausedAt) || 0);
  return Math.max(0, (Date.now() - sportfy.startedAt) / 1000);
}
function sportfyResumePersonalAfterBluetooth() {
  if (!sportfy.playing) return;
  const vehicleVideo = sportfy.ambient?.video || "";
  const currentVideo = vehicleVideo || sportfy.current?.id || "";
  if (!currentVideo) return;
  const track = sportfyTrackById(currentVideo);
  if (track && sportfy.current?.id !== track.id) {
    sportfy.current = sportfyNormalizeTrack(track);
    const queueIndex = sportfy.queue.findIndex((item) => item.id === sportfy.current.id);
    if (queueIndex >= 0) sportfy.queueIndex = queueIndex;
  }
  const currentPos = sportfyStartedSeconds();
  const vehiclePos = sportfy.ambient?.pos ? sportfy.ambient.pos + Math.max(0, (Date.now() - (sportfy.ambient.updatedAt || Date.now())) / 1000) : 0;
  const resumeAt = Math.max(0, Math.floor(Math.max(currentPos, vehiclePos)));
  sportfy.pausedAt = 0;
  sportfy.startedAt = Date.now() - (resumeAt * 1000);
  sportfy.playbackVideo = currentVideo;
  sportfyPlayNativeAudio(currentVideo, sportfy.volume, resumeAt);
}
function sportfyUpdateProgress() {
  const progress = $("#sportfyProgress");
  if (!progress || sportfy.seeking) return;
  let duration = sportfyDurationSeconds(sportfy.current);
  let current = sportfyStartedSeconds();
  if (sportfy.playerReady && sportfy.ytPlayer) {
    duration = sportfy.ytPlayer.getDuration?.() || duration;
    current = sportfy.ytPlayer.getCurrentTime?.() || 0;
  }
  if (sportfy.playing && duration && current >= duration) {
    sportfy.repeat ? sportfyPlay(sportfy.current, sportfy.queue, sportfy.queueIndex) : sportfyNextTrack();
    return;
  }
  progress.value = duration ? String((current / duration) * 100) : "0";
  $("#sportfyCurrentTime").textContent = sportfyFormatClock(current);
  $("#sportfyDurationTime").textContent = sportfyFormatClock(duration);
  const miniProgress = $("#sportfyMiniProgress");
  if (miniProgress) miniProgress.style.width = `${duration ? Math.max(0, Math.min(100, (current / duration) * 100)) : 0}%`;
  const miniArtist = $("#sportfyNowArtist");
  if (miniArtist && sportfy.current) miniArtist.textContent = `${sportfy.bluetooth.connected ? "No veiculo" : sportfy.current.author}${duration ? ` - ${sportfyFormatClock(current)} / ${sportfyFormatClock(duration)}` : ""}`;
}
function sportfySeekToProgress() {
  const duration = sportfy.playerReady && sportfy.ytPlayer ? sportfy.ytPlayer.getDuration?.() || sportfyDurationSeconds(sportfy.current) : sportfyDurationSeconds(sportfy.current);
  const target = (Number($("#sportfyProgress")?.value || 0) / 100) * duration;
  if (sportfy.playerReady && sportfy.ytPlayer) sportfy.ytPlayer.seekTo(target, true);
  sportfy.pausedAt = target;
  if (sportfy.playing) sportfy.startedAt = Date.now() - (target * 1000);
  sportfyPostPlayerCommand({ func: "seekTo", args: [target, true] }, sportfy.localFrame);
  sportfy.seeking = false;
  sportfyUpdateProgress();
}
async function sportfySearch(query) {
  const term = query.trim();
  sportfy.query = term;
  sportfy.searchSerial += 1;
  const serial = sportfy.searchSerial;
  if (!term) {
    sportfy.results = [];
    renderSportfy();
    return;
  }
  const empty = $("#sportfySearchEmpty");
  if (empty) empty.textContent = "Buscando...";
  renderSportfy();
  const searches = [
    sportfyNativeSearch(term, serial),
    ...SPORTFY_SEARCH_ENDPOINTS.map((base) => sportfyFetchSearch(base, term))
  ];
  const found = await new Promise((resolve) => {
    let pending = searches.length;
    searches.forEach((request) => {
      request.then((rows) => {
        if (rows.length) resolve(rows);
        else if (--pending < 1) resolve([]);
      }).catch(() => {
        if (--pending < 1) resolve([]);
      });
    });
  });
  if (serial !== sportfy.searchSerial) return;
  if (found.length) {
    sportfyApplySearchRows(found, serial);
    return;
  }
  sportfy.results = sportfyFallbackSearch(term);
  sportfyPrefetchTracks(sportfy.results);
  if (empty) empty.textContent = sportfy.results.length ? "" : "Nao foi possivel buscar agora.";
  renderSportfy();
}
async function sportfyFetchSearch(base, term) {
  const timeoutMs = 4500;
  const timeout = new Promise((resolve) => window.setTimeout(() => resolve(null), timeoutMs));
  const request = fetch(`${base}/api/v1/search?q=${encodeURIComponent(term)}&type=video&sort_by=relevance&region=BR`, {
    headers: { Accept: "application/json" }
  });
  const response = await Promise.race([request, timeout]);
  if (!response?.ok) {
    console.warn("sportfy search failed", base, response?.status || "timeout");
    return [];
  }
  const contentType = response.headers?.get?.("content-type") || "";
  if (contentType && !contentType.includes("json")) {
    console.warn("sportfy search non-json", base, contentType);
    return [];
  }
  const data = await response.json();
  return data
    .filter((item) => !item.type || item.type === "video")
    .map(sportfyNormalizeTrack)
    .filter((track) => track && !sportfy.unavailable.has(track.id))
    .slice(0, 18);
}
function renderSportfyTrack(track, compact = false) {
  const duration = sportfyFormatDuration(track.duration);
  const favorite = sportfyIsFavorite(track.id);
  return `
    <div class="sportfy-track${sportfy.current?.id === track.id && sportfy.playing ? " is-playing" : ""}" data-sportfy-video="${esc(track.id)}">
      <img src="${esc(track.thumb)}" alt="" loading="lazy" />
      <span><strong>${esc(track.title)}</strong><small>${esc(track.author)}${duration ? ` - ${duration}` : ""}</small></span>
      <button class="${favorite ? "active" : ""}" type="button" data-sportfy-like="${esc(track.id)}" aria-label="Favoritar">${icon("i-heart")}</button>
      ${compact ? "" : `<button class="sportfy-add" type="button" data-sportfy-add="${esc(track.id)}" aria-label="Adicionar">${icon("i-plus")}</button>`}
    </div>
  `;
}
function renderSportfyPlaylistRow(playlist) {
  const cover = playlist.tracks?.[0]?.thumb || "";
  return `
    <div class="sportfy-playlist-row" data-sportfy-playlist-open="${esc(playlist.id)}">
      ${cover ? `<img src="${esc(cover)}" alt="" loading="lazy" />` : `<span><svg><use href="#i-music"/></svg></span>`}
      <strong>${esc(playlist.name)}</strong>
      <small>${playlist.tracks?.length || 0} musica${playlist.tracks?.length === 1 ? "" : "s"}</small>
      <button type="button" data-sportfy-playlist-delete="${esc(playlist.id)}" aria-label="Excluir playlist"><svg><use href="#i-x"/></svg></button>
    </div>
  `;
}
function renderSportfyPlayer() {
  const player = $("#sportfyPlayer");
  if (!player) return;
  player.hidden = !sportfy.current;
  if (!sportfy.current) return;
  const duration = sportfyDurationSeconds(sportfy.current);
  const current = sportfyStartedSeconds();
  const percent = duration ? Math.max(0, Math.min(100, (current / duration) * 100)) : 0;
  $("#sportfyCover").src = sportfy.current.thumb;
  $("#sportfyNowTitle").textContent = sportfy.current.title;
  $("#sportfyNowArtist").textContent = `${sportfy.bluetooth.connected ? "No veiculo" : sportfy.current.author}${duration ? ` - ${sportfyFormatClock(current)} / ${sportfyFormatClock(duration)}` : ""}`;
  const miniProgress = $("#sportfyMiniProgress");
  if (miniProgress) miniProgress.style.width = `${percent}%`;
  $("#sportfyLikeNow").classList.toggle("active", sportfyIsFavorite(sportfy.current.id));
  $("#sportfyPlayPause").innerHTML = icon(sportfy.playing ? "i-pause" : "i-play");
  $("#sportfyNpPlayPause").innerHTML = icon(sportfy.playing ? "i-pause" : "i-play");
}
function renderSportfyNowPlaying() {
  const nowPlaying = $("#sportfyNowPlaying");
  nowPlaying?.classList.toggle("is-open", sportfy.nowOpen);
  nowPlaying?.setAttribute("aria-hidden", sportfy.nowOpen ? "false" : "true");
  if (!sportfy.current) return;
  $("#sportfyNpCover").src = sportfy.current.thumb;
  $("#sportfyNpTitle").textContent = sportfy.current.title;
  $("#sportfyNpArtist").textContent = sportfy.current.author;
  $("#sportfyNpPlayPause").innerHTML = icon(sportfy.playing ? "i-pause" : "i-play");
  $("#sportfyBluetoothLabel").textContent = sportfy.bluetooth.connected ? "Conectado ao veiculo" : "Bluetooth desligado";
  $("#sportfyNpBluetooth")?.classList.toggle("connected", sportfy.bluetooth.connected);
  $("#sportfyShuffle")?.classList.toggle("active", sportfy.shuffle);
  $("#sportfyRepeat")?.classList.toggle("active", sportfy.repeat);
  const volume = $("#sportfyVolume");
  if (volume && Number(volume.value) !== sportfy.volume) volume.value = String(sportfy.volume);
  sportfyUpdateProgress();
}
function renderSportfy() {
  const view = $(".sportfy-view");
  if (!view) return;
  view.dataset.sportfyTab = sportfy.tab;
  $all("[data-sportfy-page]", view).forEach((page) => page.classList.toggle("active", page.dataset.sportfyPage === sportfy.tab));
  $all(".sportfy-tabs [data-sportfy-tab]", view).forEach((button) => button.classList.toggle("active", button.dataset.sportfyTab === sportfy.tab));
  $("#sportfyBtButton")?.classList.toggle("connected", sportfy.bluetooth.connected);
  $("#sportfyGreeting").textContent = sportfy.bluetooth.connected ? "Bluetooth conectado" : "Bem-vindo";
  const searchInput = $("#sportfySearchInput");
  if (searchInput && document.activeElement !== searchInput && searchInput.value !== sportfy.query) searchInput.value = sportfy.query;
  const clearSearch = $("#sportfyClearSearch");
  if (clearSearch) clearSearch.hidden = !String(sportfy.query || searchInput?.value || "").trim();
  $all("[data-sportfy-filter]", view).forEach((button) => button.classList.toggle("active", button.dataset.sportfyFilter === sportfy.filter));
  const recent = sportfy.history.filter((track) => !sportfy.unavailable.has(track.id)).slice(0, 6);
  sportfyPrefetchTracks(recent);
  $("#sportfyQuickPlaylistTitle").textContent = sportfy.playlists[0]?.name || "Criar playlist";
  $("#sportfyHomeEmpty").hidden = recent.length > 0;
  $("#sportfyRecentTitle").hidden = recent.length < 1;
  $("#sportfyHomeList").innerHTML = recent.map((track) => renderSportfyTrack(track)).join("");
  if (!sportfy.query) $("#sportfySearchEmpty").textContent = "Digite uma musica ou cantor para buscar.";
  $("#sportfySearchEmpty").hidden = sportfy.results.length > 0;
  $("#sportfyResults").innerHTML = sportfy.results.map((track) => renderSportfyTrack(track)).join("");
  const selectedPlaylist = sportfy.playlists.find((playlist) => playlist.id === sportfy.libraryPlaylist);
  $("#sportfyPlaylists").innerHTML = selectedPlaylist
    ? `<button class="sportfy-library-back" type="button" data-sportfy-playlist-back><svg><use href="#i-back"/></svg><span>Voltar</span></button><div class="sportfy-list-title">${esc(selectedPlaylist.name)}</div>${selectedPlaylist.tracks.length ? selectedPlaylist.tracks.map((track) => renderSportfyTrack(track, true)).join("") : `<div class="sportfy-empty">Playlist vazia.</div>`}`
    : (sportfy.playlists.length ? sportfy.playlists.map(renderSportfyPlaylistRow).join("") : `<div class="sportfy-empty">Crie sua primeira playlist.</div>`);
  $("#sportfyFavorites").innerHTML = sportfy.favorites.length ? sportfy.favorites.map((track) => renderSportfyTrack(track, true)).join("") : `<div class="sportfy-empty">Nenhuma favorita ainda.</div>`;
  const cleanHistory = sportfy.history.filter((track) => !sportfy.unavailable.has(track.id)).slice(0, 12);
  $("#sportfyHistory").innerHTML = cleanHistory.length ? cleanHistory.map((track) => renderSportfyTrack(track, true)).join("") : `<div class="sportfy-empty">Historico vazio.</div>`;
  renderSportfyPlayer();
  renderSportfyNowPlaying();
}
function sportfyResetTabScroll(tab = sportfy.tab) {
  window.requestAnimationFrame(() => {
    const page = $(`.sportfy-page[data-sportfy-page="${tab}"]`);
    if (page) page.scrollTop = 0;
  });
}
function updateSportfyFromEvent(data) {
  const status = decode(data.status || data.state || "");
  if (data.item === "fav" || data.list === "fav") sportfyUpsert(sportfy.favorites, { id: decode(data.video || data.v), title: decode(data.title || data.t), author: decode(data.author || data.a), duration: decode(data.duration || data.d) }, 80);
  if (data.item === "hist" || data.list === "hist") sportfyUpsert(sportfy.history, { id: decode(data.video || data.v), title: decode(data.title || data.t), author: decode(data.author || data.a), duration: decode(data.duration || data.d) }, 80);
  if (data.type === "sportfy_bt") {
    const wasConnected = sportfy.bluetooth.connected;
    const previousVehicle = sportfy.bluetooth.vehicle;
    const rawVehicle = decode(data.vehicle || "");
    let nextVehicle = rawVehicle === "" ? -1 : Math.trunc(Number(rawVehicle));
    if (!Number.isFinite(nextVehicle)) nextVehicle = -1;
    let nextConnected = status === "connected";
    if (nextConnected) {
      const nativeState = sportfyReadVehicleAudioState(nextVehicle);
      nextConnected = !!nativeState;
      nextVehicle = nativeState ? Number(nativeState.vehicle) : -1;
    }
    const sameConnection = wasConnected && nextConnected && previousVehicle === nextVehicle;
    const sameDisconnection = !wasConnected && !nextConnected;
    sportfy.bluetooth.connected = nextConnected;
    sportfy.bluetooth.vehicle = nextVehicle;
    if (!nextConnected) sportfyStopBluetoothSpatialMonitor();
    if (sameConnection || sameDisconnection) {
      if (sameConnection) sportfyStartConnectedVehicleAudio(false);
      sportfySaveCache();
      renderSportfy();
      return;
    }
    showToast(decode(data.message) || (sportfy.bluetooth.connected ? "Bluetooth conectado." : "Entre em um veiculo para conectar."));
    if (nextConnected && sportfy.current) {
      sportfyStartConnectedVehicleAudio(true);
    } else if (wasConnected && sportfy.current && sportfy.playing) {
      if (previousVehicle >= 0) sportfyStopNativeAmbientSource(previousVehicle);
      const vehiclePos = sportfy.ambient?.pos ? sportfy.ambient.pos + Math.max(0, (Date.now() - (sportfy.ambient.updatedAt || Date.now())) / 1000) : sportfyStartedSeconds();
      sportfy.startedAt = Date.now() - (Math.max(0, Math.floor(vehiclePos)) * 1000);
      sportfy.pausedAt = 0;
      if (sportfy.nativeAudio.active && sportfy.nativeAudio.video === sportfy.current.id) {
        sportfySetAmbientVolumeSmooth(sportfy.volume);
      } else {
        sportfyResumePersonalAfterBluetooth();
      }
    }
  }
  if (data.type === "sportfy_ambient") {
    const video = decode(data.video || data.v || "");
    const rawVehicle = decode(data.vehicle || "");
    let vehicleId = rawVehicle === "" ? -1 : Math.trunc(Number(rawVehicle));
    if (!Number.isFinite(vehicleId)) vehicleId = -1;
    if (status !== "play" || !video) {
      sportfyStopNativeAmbientSource(vehicleId);
      if (vehicleId < 0 || sportfy.ambient.vehicle === vehicleId) {
        sportfy.ambient = { vehicle: 0, video: "", volume: 0, pos: 0, mode: "", updatedAt: 0 };
      }
      sportfy.ambientFrame?.remove();
      sportfy.ambientFrame = null;
      sportfySyncNativeAudioKeepAlive();
    } else {
      const startSeconds = Math.max(0, Math.floor(Number(decode(data.pos || "")) || 0));
      const ambientVolume = sportfyAmbientVolume(
        Math.max(0, Math.min(100, Number(decode(data.volume || "")) || 60)),
        decode(data.mode || "")
      );
      sportfy.ambient = {
        vehicle: vehicleId,
        video,
        volume: ambientVolume,
        pos: startSeconds,
        mode: decode(data.mode || ""),
        updatedAt: Date.now()
      };
      sportfySetNativeAmbientSource(vehicleId, video, ambientVolume, startSeconds, sportfy.ambient.mode);
      sportfySyncNativeAudioKeepAlive();
    }
  }
  sportfySaveCache();
  renderSportfy();
}

function instagramInitials(name = "") {
  return initials(name || instagram.profile.name || profile.name);
}

function instagramOwnId() {
  return String(instagram.profile.id || profile.accountId || "");
}

function instagramOwnUsername() {
  return String(instagram.profile.username || "").toLowerCase();
}

function isInstagramOwnPerson(person = {}) {
  const ownId = instagramOwnId();
  const ownUsername = instagramOwnUsername();
  return Boolean(
    (ownId && String(person.account || person.id || person.actor || person.peer || "") === ownId)
    || (ownUsername && String(person.username || "").toLowerCase() === ownUsername)
  );
}

function instagramPersonMatches(item = {}, person = {}) {
  const id = String(person.id || person.account || person.actor || person.peer || "");
  const username = String(person.username || "").toLowerCase();
  const itemIds = [item.account, item.id, item.actor, item.peer].map((value) => String(value || ""));
  if (id && itemIds.includes(id)) return true;
  return Boolean(username && String(item.username || "").toLowerCase() === username);
}

function instagramHydratePerson(person = {}) {
  const next = { ...person };
  const known = instagram.users.find((user) => instagramPersonMatches(user, next));
  if (known) {
    next.username = next.username || known.username;
    next.name = next.name || known.name;
    next.avatar = next.avatar || known.avatar;
    next.bio = next.bio || known.bio;
  }
  if (instagramPersonMatches(instagram.profile, next)) {
    next.username = next.username || instagram.profile.username;
    next.name = next.name || instagram.profile.name;
    next.avatar = instagram.profile.avatar || next.avatar;
    next.bio = instagram.profile.bio || next.bio;
  }
  return next;
}

function instagramAvatar(person = {}, size = "") {
  const hydrated = instagramHydratePerson(person);
  const name = decode(hydrated.name || hydrated.username || profile.name);
  const avatar = decode(hydrated.avatar || "");
  const resolvedAvatar = resolveMediaUrl(avatar);
  const cls = size ? ` ig-avatar ${size}` : "ig-avatar";
  if (resolvedAvatar) return `<span class="${cls.trim()}"><img src="${esc(resolvedAvatar)}" alt="" /></span>`;
  return `<span class="${cls.trim()}"><b>${esc(instagramInitials(name))}</b></span>`;
}

function resolvedImageHtml(rawUrl, attrs = "") {
  const resolved = resolveMediaUrl(rawUrl);
  if (resolved) return `<img src="${esc(resolved)}" alt="" ${attrs}/>`;
  return imagePlaceholderHtml(rawUrl ? "Carregando foto" : "Sem imagem");
}

function instagramEmpty(text) {
  return `<div class="ig-empty"><span class="ig-mark"></span><strong>${esc(text)}</strong></div>`;
}

function instagramTime(text) {
  return decode(text || "agora");
}

function sendInstagramAction(action, payload = "") {
  bridge?.action?.(action, payload);
}

function refreshInstagramUnread() {
  instagram.unread = instagram.activity.reduce((sum, item) => sum + (item.unread ? 1 : 0), 0)
    + instagram.conversations.reduce((sum, item) => sum + (Number(item.unread) || 0), 0);
  renderInstagramBadges();
}

function markInstagramActivityRead() {
  if (!instagram.activity.some((item) => item.unread)) return;
  instagram.activity.forEach((item) => { item.unread = false; });
  sendInstagramAction("insta_activity_read", "");
  refreshInstagramUnread();
  renderInstagramActivity();
}

function refreshInstagramSuggestions() {
  instagram.suggestions = [...instagram.users]
    .filter((user) => !user.following && !isInstagramOwnPerson(user))
    .sort(() => Math.random() - 0.5)
    .slice(0, 5)
    .map((user) => String(user.id));
}

function loadMoreInstagramFeed() {
  if (bridge === null || instagram.feedLoading || !instagram.feedHasMore || !instagram.feedIds.length) return;
  const before = instagram.feedIds.reduce((lowest, id) => {
    const numeric = Number(id);
    return numeric > 0 && (!lowest || numeric < lowest) ? numeric : lowest;
  }, 0);
  if (!before) return;
  instagram.feedMode = "more";
  instagram.feedLoading = true;
  sendInstagramAction("insta_feed_more", `before=${encodePayload(before)}`);
}

function instagramReset() {
  instagram.posts.length = 0;
  instagram.feedIds.length = 0;
  instagram.feedMode = "initial";
  instagram.feedLoading = true;
  instagram.feedHasMore = true;
  instagram.storySyncing = true;
  instagram.nextStories = [];
  instagram.users.length = 0;
  instagram.suggestions.length = 0;
  instagram.activity.length = 0;
  instagram.conversations.length = 0;
  instagram.messages = {};
  instagram.comments = {};
  instagram.unread = 0;
}

function seedInstagramPreviewData() {
  if (bridge !== null || instagram.demoSeeded) return;
  const demo = {
    id: "preview_david",
    username: "david.morel",
    name: "David Morel",
    bio: "Designer, cafe e bons momentos pela cidade.",
    avatar: "https://images.unsplash.com/photo-1500648767791-00dcc994a43e?auto=format&fit=crop&w=240&q=80",
    followers: 5280,
    followingCount: 130,
    following: false,
    online: true,
    posts: 1
  };
  instagram.demoSeeded = true;
  instagram.users = [demo];
  instagram.posts = [{
    id: "preview_post_1",
    account: demo.id,
    username: demo.username,
    name: demo.name,
    avatar: demo.avatar,
    media: "https://images.unsplash.com/photo-1495474472287-4d71bcdd2085?auto=format&fit=crop&w=700&q=84",
    caption: "Uma pausa tranquila no centro. Cafe, conversa e uma vista bonita.",
    time: "agora",
    likes: 5280,
    comments: 140,
    liked: false
  }];
  instagram.feedIds = ["preview_post_1"];
  instagram.stories = [{
    id: "preview_story_1",
    account: demo.id,
    username: demo.username,
    name: demo.name,
    avatar: demo.avatar,
    media: "https://images.unsplash.com/photo-1519501025264-65ba15a82390?auto=format&fit=crop&w=600&q=84",
    caption: "Passeio pela cidade",
    time: "agora",
    liked: false
  }];
  instagram.activity = [{
    id: "preview_activity_1",
    actor: demo.id,
    actorName: demo.name,
    avatar: demo.avatar,
    eventType: "follow",
    post: "",
    text: "comecou a seguir voce",
    time: "agora",
    unread: true
  }, {
    id: "preview_activity_2",
    actor: demo.id,
    actorName: demo.name,
    avatar: demo.avatar,
    eventType: "comment",
    post: "preview_post_1",
    text: "comentou na sua publicacao",
    time: "agora",
    unread: true
  }];
  instagram.conversations = [{
    id: demo.id,
    username: demo.username,
    name: demo.name,
    avatar: demo.avatar,
    text: "O feed ficou muito bonito!",
    time: "agora",
    unread: 1,
    online: true,
    blocked: false
  }];
  instagram.messages[demo.id] = [
    { id: "preview_dm_1", fromMe: false, text: "E ai, tudo certo?", time: "12:41" },
    { id: "preview_dm_2", fromMe: true, text: "Tudo tranquilo. Curti a foto!", time: "12:42" },
    { id: "preview_dm_3", fromMe: false, text: "O feed ficou muito bonito!", time: "12:43" }
  ];
  instagram.comments.preview_post_1 = [
    { id: "preview_comment_1", account: demo.id, name: demo.name, username: demo.username, avatar: demo.avatar, text: "Lugar bonito demais.", time: "agora", likes: 12, liked: false },
    { id: "preview_comment_2", account: "preview_me", name: "Jogador", username: "trevin.preview", avatar: "", text: "Essa foto ficou muito boa!", time: "agora", likes: 3, liked: false }
  ];
  instagram.unread = 2;
}

function instagramUpsert(list, item, key = "id") {
  const value = String(item[key] || "");
  if (!value) return;
  const index = list.findIndex((entry) => String(entry[key]) === value);
  if (index >= 0) list[index] = { ...list[index], ...item };
  else list.unshift(item);
}

function applyInstagramPersonUpdate(person) {
  if (!person) return;
  const id = String(person.id || person.account || "");
  const username = String(person.username || "").toLowerCase();
  if (!id && !username) return;
  const patch = {
    username: person.username,
    name: person.name,
    avatar: person.avatar,
    bio: person.bio
  };
  const apply = (item) => {
    if (!instagramPersonMatches(item, person)) return;
    Object.entries(patch).forEach(([key, value]) => {
      if (value !== undefined && value !== "") item[key] = value;
    });
    if (patch.name !== undefined && patch.name !== "") item.actorName = patch.name;
  };
  if (instagramPersonMatches(instagram.profile, person)) apply(instagram.profile);
  instagram.posts.forEach(apply);
  instagram.stories.forEach(apply);
  instagram.users.forEach(apply);
  instagram.activity.forEach(apply);
  instagram.conversations.forEach(apply);
  Object.values(instagram.comments).flat().forEach(apply);
}

function updateInstagramFromEvent(data) {
  const type = decode(data.type || "");
  if (type === "insta_onboarding") {
    instagram.ready = true;
    instagram.onboarding = decode(data.required) !== "0";
    return renderInstagram();
  }
  if (type === "insta_register") {
    const ok = decode(data.status) === "ok";
    instagram.ready = true;
    instagram.onboarding = !ok;
    const message = decode(data.message || "");
    if (message) showToast(message);
    const hint = $("#igUsernameHint");
    if (hint) {
      hint.textContent = message || "Use de 3 a 24 letras, numeros, ponto ou underline.";
      hint.classList.toggle("error", !ok);
    }
    return renderInstagram();
  }
  if (type === "insta_reset") {
    instagramReset();
    return renderInstagram();
  }
  if (type === "insta_profile") {
    instagram.ready = true;
    const nextProfile = {
      id: decode(data.id || data.account || instagram.profile.id || profile.accountId),
      username: decode(data.username || instagram.profile.username || profile.name),
      name: decode(data.name || instagram.profile.name || profile.name),
      bio: decode(data.bio || instagram.profile.bio || ""),
      avatar: decode(data.avatar || instagram.profile.avatar || ""),
      posts: Number(decode(data.posts || "")) || 0,
      followers: Number(decode(data.followers || "")) || 0,
      following: Number(decode(data.following || "")) || 0,
    };
    instagram.profile = nextProfile;
    applyInstagramPersonUpdate(nextProfile);
  }
  if (type === "insta_profile_update") {
    const person = {
      id: decode(data.id || data.account),
      username: decode(data.username),
      name: decode(data.name),
      bio: decode(data.bio),
      avatar: decode(data.avatar)
    };
    if (String(person.id) === String(instagram.profile.id || profile.accountId)) {
      instagram.profile = { ...instagram.profile, ...Object.fromEntries(Object.entries(person).filter(([, value]) => value !== "")) };
    }
    instagramUpsert(instagram.users, person);
    applyInstagramPersonUpdate(person);
  }
  if (type === "insta_post") {
    const id = decode(data.id);
    const scope = decode(data.scope || "feed");
    const post = {
      id,
      account: decode(data.account),
      username: decode(data.username),
      name: decode(data.name),
      avatar: decode(data.avatar),
      media: decode(data.media || data.url),
      caption: decode(data.caption),
      time: instagramTime(data.time),
      likes: Number(decode(data.likes || "")) || 0,
      comments: Number(decode(data.comments || "")) || 0,
      liked: decode(data.liked) === "1"
    };
    const localPostIds = instagram.posts
      .filter((item) => String(item.id).startsWith("local_post_") && String(item.media) === String(post.media) && isInstagramOwnPerson(item))
      .map((item) => String(item.id));
    instagram.posts = instagram.posts.filter((item) => !localPostIds.includes(String(item.id)));
    instagram.feedIds = instagram.feedIds.filter((feedId) => !localPostIds.includes(String(feedId)));
    instagramUpsert(instagram.posts, post);
    if (scope === "feed" && id && !instagram.feedIds.includes(id)) {
      if (instagram.feedMode === "live") instagram.feedIds.unshift(id);
      else instagram.feedIds.push(id);
    }
  }
  if (type === "insta_feed_page") {
    instagram.feedLoading = false;
    instagram.feedHasMore = decode(data.hasMore) === "1";
    instagram.feedMode = "live";
  }
  if (type === "insta_stories_begin") {
    instagram.storySyncing = true;
    instagram.nextStories = [];
    return;
  }
  if (type === "insta_story") {
    const story = {
      id: decode(data.id),
      account: decode(data.account),
      username: decode(data.username),
      name: decode(data.name),
      avatar: decode(data.avatar),
      media: decode(data.media || data.url),
      caption: decode(data.caption),
      time: instagramTime(data.time),
      liked: decode(data.liked) === "1",
      views: Number(decode(data.views || "")) || 0
    };
    const targetStories = instagram.storySyncing ? instagram.nextStories : instagram.stories;
    const localStoryIds = targetStories
      .filter((item) => String(item.id).startsWith("local_story_") && String(item.media) === String(story.media) && isInstagramOwnPerson(item))
      .map((item) => String(item.id));
    for (let index = targetStories.length - 1; index >= 0; index -= 1) {
      if (localStoryIds.includes(String(targetStories[index].id))) targetStories.splice(index, 1);
    }
    instagramUpsert(targetStories, story);
    if (instagram.storySyncing) return;
  }
  if (type === "insta_stories_done") {
    const localStories = instagram.stories.filter((story) => String(story.id).startsWith("local_story_"));
    const serverMedia = new Set(instagram.nextStories.map((story) => String(story.media || "")));
    instagram.stories = [
      ...instagram.nextStories,
      ...localStories.filter((story) => !serverMedia.has(String(story.media || "")))
    ];
    instagram.nextStories = [];
    instagram.storySyncing = false;
    if (instagram.story) {
      const current = instagram.stories.find((story) => String(story.id) === String(instagram.story.id));
      if (current) instagram.story = current;
    }
  }
  if (type === "insta_story_delete") {
    const id = decode(data.id || data.story);
    instagram.stories = instagram.stories.filter((story) => String(story.id) !== String(id));
    instagram.storyGroup = instagram.storyGroup.filter((story) => String(story.id) !== String(id));
    if (instagram.story && String(instagram.story.id) === String(id)) {
      if (instagram.storyGroup[instagram.storyIndex]) openInstagramStory(instagram.storyGroup[instagram.storyIndex].id, true);
      else if (instagram.storyGroup[instagram.storyIndex - 1]) openInstagramStory(instagram.storyGroup[instagram.storyIndex - 1].id, true);
      else closeInstagramStory();
    }
  }
  if (type === "insta_story_viewers_clear") {
    const story = decode(data.story);
    if (story) instagram.storyViewers[story] = [];
  }
  if (type === "insta_story_viewer") {
    const story = decode(data.story);
    if (story) {
      if (!instagram.storyViewers[story]) instagram.storyViewers[story] = [];
      instagramUpsert(instagram.storyViewers[story], {
        id: decode(data.id || data.account),
        username: decode(data.username),
        name: decode(data.name),
        avatar: decode(data.avatar),
        time: instagramTime(data.time)
      });
      renderInstagramStoryViewers();
    }
  }
  if (type === "insta_user") {
    const person = {
      id: decode(data.id || data.account),
      username: decode(data.username),
      name: decode(data.name),
      bio: decode(data.bio),
      avatar: decode(data.avatar),
      followers: Number(decode(data.followers || "")) || 0,
      following: decode(data.following) === "1",
      online: decode(data.online) === "1"
    };
    instagramUpsert(instagram.users, person);
    applyInstagramPersonUpdate(person);
  }
  if (type === "insta_activity") {
    instagramUpsert(instagram.activity, {
      id: decode(data.id),
      actor: decode(data.actor),
      actorName: decode(data.actorName || data.name),
      avatar: decode(data.avatar),
      eventType: decode(data.eventType || data.event || data.kind),
      post: decode(data.post),
      text: decode(data.text || data.body),
      time: instagramTime(data.time),
      unread: decode(data.unread) !== "0"
    });
  }
  if (type === "insta_conv") {
    instagramUpsert(instagram.conversations, {
      id: decode(data.peer || data.id),
      name: decode(data.name),
      username: decode(data.username),
      avatar: decode(data.avatar),
      text: decode(data.text),
      time: instagramTime(data.time),
      unread: Number(decode(data.unread || "")) || 0,
      online: decode(data.online) === "1",
      blocked: decode(data.blocked) === "1"
    });
  }
  if (type === "insta_comments_clear") {
    const post = decode(data.post);
    if (post) instagram.comments[post] = [];
  }
  if (type === "insta_comment_row") {
    const post = decode(data.post);
    if (post) {
      if (!instagram.comments[post]) instagram.comments[post] = [];
      instagramUpsert(instagram.comments[post], {
        id: decode(data.id),
        account: decode(data.account),
        username: decode(data.username),
        name: decode(data.name),
        avatar: decode(data.avatar),
        text: decode(data.text),
        time: instagramTime(data.time),
        likes: Number(decode(data.likes || "")) || 0,
        liked: decode(data.liked) === "1"
      });
    }
  }
  if (type === "insta_dm") {
    const peer = decode(data.peer || data.from || data.to);
    if (peer) {
      if (!instagram.messages[peer]) instagram.messages[peer] = [];
      instagram.messages[peer].push({
        id: decode(data.id || `${Date.now()}`),
        fromMe: decode(data.direction) === "out",
        text: decode(data.text),
        time: instagramTime(data.time)
      });
      if (decode(data.direction) === "in" && String(peer) === String(instagram.currentPeer) && !$("#igDirectPanel")?.hidden) {
        const conversation = instagram.conversations.find((item) => String(item.id) === String(peer));
        if (conversation) conversation.unread = 0;
        sendInstagramAction("insta_dm_open", `peer=${encodePayload(peer)}`);
      }
    }
  }
  if (type === "insta_dm_clear") {
    const peer = decode(data.peer);
    if (peer) instagram.messages[peer] = [];
  }
  if (type === "insta_status") {
    const message = decode(data.message || "");
    if (message) showToast(message);
  }
  if (type === "insta_activity_read") instagram.activity.forEach((item) => { item.unread = false; });
  refreshInstagramUnread();
  renderInstagram();
}

function setInstagramTab(tab) {
  const next = tab || "feed";
  const changed = instagram.tab !== next;
  instagram.tab = next;
  $(".instagram-view")?.setAttribute("data-insta-tab", instagram.tab);
  $all(".ig-page").forEach((page) => page.classList.toggle("active", page.dataset.igPage === instagram.tab));
  $all(".ig-tabs [data-ig-tab]").forEach((button) => button.classList.toggle("active", button.dataset.igTab === instagram.tab));
  if (changed) $(".ig-page.active")?.scrollTo?.({ top: 0, behavior: "instant" });
  if (changed && instagram.tab === "explore") refreshInstagramSuggestions();
  if (changed && instagram.tab === "activity") markInstagramActivityRead();
}

function goInstagramOwnProfile() {
  instagram.viewingProfile = "";
  setInstagramTab("profile");
  renderInstagramProfile();
}

function goInstagramBack() {
  if (!$("#igStoryViewer")?.hidden) return closeInstagramStory();
  if (!$("#igCommentsPanel")?.hidden) return closeInstagramComments();
  if (!$("#igPostDetail")?.hidden) return closeInstagramPostDetail();
  if (!$("#igDirectThread")?.hidden) return closeInstagramThread();
  if (!$("#igDirectPanel")?.hidden) return closeInstagramDirect();
  if (!$("#igEditProfilePanel")?.hidden) return closeInstagramEditProfile();
  if (!$("#igCompose")?.hidden) return closeInstagramCompose();
  if (instagram.viewingProfile) {
    instagram.viewingProfile = "";
    renderInstagramProfile();
    return;
  }
  if (instagram.tab !== "feed") return setInstagramTab("feed");
  showView("home");
}

function renderInstagramOnboarding() {
  const panel = $("#igOnboarding");
  if (!panel) return;
  panel.hidden = !(instagram.ready && instagram.onboarding);
}

function submitInstagramUsername() {
  const input = $("#igUsernameInput");
  const hint = $("#igUsernameHint");
  const username = String(input?.value || "").trim().toLowerCase();
  if (!/^[a-z0-9._]{3,24}$/.test(username)) {
    if (hint) {
      hint.textContent = "Use de 3 a 24 letras, numeros, ponto ou underline.";
      hint.classList.add("error");
    }
    return showToast("Escolha um usuario valido.");
  }
  if (bridge === null) {
    seedInstagramPreviewData();
    const exists = instagram.users.some((user) => String(user.username).toLowerCase() === username);
    if (exists) {
      if (hint) {
        hint.textContent = "Esse usuario ja existe. Escolha outro.";
        hint.classList.add("error");
      }
      return showToast("Esse usuario ja existe.");
    }
    instagram.profile = { ...instagram.profile, id: "preview_me", username, name: profile.name || "Jogador" };
    instagram.onboarding = false;
    showToast("Usuario criado para demonstracao.");
    return renderInstagram();
  }
  if (hint) {
    hint.textContent = "Verificando disponibilidade...";
    hint.classList.remove("error");
  }
  sendInstagramAction("insta_register", `username=${encodePayload(username)}`);
}

function instagramStoryGroupKey(story = {}) {
  if (isInstagramOwnPerson(story)) return "self";
  const username = String(story.username || "").toLowerCase();
  return String(story.account || (username ? `u:${username}` : story.id));
}

function instagramStorySortValue(story = {}) {
  const id = String(story.id || "");
  if (id.startsWith("local_story_")) return Number(id.replace("local_story_", "")) || Date.now();
  const numeric = Number(id);
  return Number.isFinite(numeric) ? numeric : 0;
}

function getInstagramStoryGroups() {
  const groups = [];
  const map = new Map();
  instagram.stories.forEach((story) => {
    const key = instagramStoryGroupKey(story);
    if (!map.has(key)) {
      const group = { account: key, story, stories: [] };
      map.set(key, group);
      groups.push(group);
    }
    map.get(key).stories.push(story);
  });
  groups.forEach((group) => {
    group.stories.sort((a, b) => instagramStorySortValue(a) - instagramStorySortValue(b));
    group.story = group.stories[group.stories.length - 1] || group.story;
  });
  return groups;
}

function isInstagramOwnStoryGroup(group = {}) {
  return Boolean(
    group.account === "self"
    || isInstagramOwnPerson(group.story || {})
    || (group.stories || []).some((story) => isInstagramOwnPerson(story))
  );
}

function getInstagramOwnStartStory() {
  const selfGroup = getInstagramStoryGroups().find(isInstagramOwnStoryGroup);
  return selfGroup?.stories?.[0] || selfGroup?.story || null;
}

function renderInstagramStories() {
  const box = $("#igStories");
  if (!box) return;
  const groups = getInstagramStoryGroups();
  const selfGroup = groups.find(isInstagramOwnStoryGroup);
  const selfStartStory = selfGroup?.stories?.[0] || selfGroup?.story;
  const selfStory = `<button type="button" class="ig-story add" ${selfStartStory ? `data-ig-story="${esc(selfStartStory.id)}"` : "data-ig-create-story"}>${instagramAvatar(selfGroup?.story || instagram.profile)}<span>Seu story</span></button>`;
  const rows = groups.filter((group) => !isInstagramOwnStoryGroup(group)).map((group) => `
    <button type="button" class="ig-story" data-ig-story="${esc((group.stories[0] || group.story).id)}">
      ${instagramAvatar(group.story)}
      <span>${esc(group.story.username || group.story.name)}</span>
    </button>
  `).join("");
  box.innerHTML = selfStory + rows;
}

function renderInstagramFeed() {
  const box = $("#igFeed");
  if (!box) return;
  const posts = instagram.feedIds.map((id) => instagram.posts.find((post) => String(post.id) === String(id))).filter(Boolean);
  if (!posts.length) {
    box.innerHTML = instagramEmpty("Nenhuma publicacao ainda.");
    return;
  }
  box.innerHTML = posts.map((post) => `
    <article class="ig-post">
      <header class="ig-post-head" data-ig-view-profile="${esc(post.account)}">${instagramAvatar(post)}<span><strong>${esc(post.name || post.username)}</strong><small>${esc(post.username || "")} · ${esc(post.time)}</small></span></header>
      <div class="ig-media-wrap">
        <button type="button" class="ig-media" data-image-url="${esc(post.media)}">${resolvedImageHtml(post.media, 'loading="lazy"')}</button>
        <div class="ig-reaction-pill">
          <button type="button" class="${post.liked ? "active" : ""}" data-ig-like="${esc(post.id)}">${icon("i-heart")}<b>${post.likes || 0}</b></button>
          <button type="button" data-ig-focus-comment="${esc(post.id)}">${icon("i-message")}<b>${post.comments || 0}</b></button>
        </div>
      </div>
      ${post.caption ? `<p class="ig-caption"><b>${esc(post.name || post.username)}</b> ${esc(post.caption)}</p>` : `<p class="ig-caption muted">Sem legenda.</p>`}
    </article>
  `).join("");
}

function renderInstagramComments() {
  const panel = $("#igCommentsPanel");
  const box = $("#igCommentsList");
  if (!panel || !box) return;
  panel.hidden = !instagram.currentPost;
  if (!instagram.currentPost) return;
  const rows = instagram.comments[instagram.currentPost] || [];
  box.innerHTML = rows.length ? rows.map((comment) => `
    <article class="ig-comment-row ${String(comment.account) === String(instagram.focusCommentActor) ? "focus" : ""}">
      ${instagramAvatar(comment)}
      <span><strong>${esc(comment.name || comment.username)}</strong><p>${esc(comment.text)}</p><small>${esc(comment.time)}</small></span>
      <button type="button" class="${comment.liked ? "active" : ""}" data-ig-comment-like="${esc(comment.id)}">${icon("i-heart")}<b>${comment.likes || 0}</b></button>
    </article>
  `).join("") : instagramEmpty("Nenhum comentario ainda.");
}

function openInstagramComments(postId, focusActor = "") {
  instagram.currentPost = String(postId || "");
  instagram.focusCommentActor = String(focusActor || "");
  if (!instagram.currentPost) return;
  if (bridge !== null) sendInstagramAction("insta_comment_open", `post=${encodePayload(instagram.currentPost)}`);
  renderInstagramComments();
}

function closeInstagramComments() {
  instagram.currentPost = "";
  instagram.focusCommentActor = "";
  $("#igCommentsPanel").hidden = true;
}

function renderInstagramPostDetail() {
  const panel = $("#igPostDetail");
  const box = $("#igPostDetailBody");
  if (!panel || !box) return;
  panel.hidden = !instagram.detailPost;
  if (!instagram.detailPost) return;
  const post = instagram.posts.find((item) => String(item.id) === String(instagram.detailPost));
  if (!post) {
    box.innerHTML = instagramEmpty("Publicacao indisponivel.");
    return;
  }
  const comments = instagram.comments[post.id] || [];
  const preview = comments.slice(0, 3).map((comment) => `<p class="ig-post-comment-preview"><b>${esc(comment.name || comment.username)}</b>${esc(comment.text)}</p>`).join("");
  box.innerHTML = `
    <article class="ig-post">
      <header class="ig-post-head" data-ig-view-profile="${esc(post.account)}">${instagramAvatar(post)}<span><strong>${esc(post.name || post.username)}</strong><small>${esc(post.username || "")} - ${esc(post.time)}</small></span></header>
      <div class="ig-media-wrap">
        <button type="button" class="ig-media" data-image-url="${esc(post.media)}">${resolvedImageHtml(post.media)}</button>
        <div class="ig-reaction-pill">
          <button type="button" class="${post.liked ? "active" : ""}" data-ig-like="${esc(post.id)}">${icon("i-heart")}<b>${post.likes || 0}</b></button>
          <button type="button" data-ig-focus-comment="${esc(post.id)}">${icon("i-message")}<b>${post.comments || 0}</b></button>
        </div>
      </div>
      ${post.caption ? `<p class="ig-caption"><b>${esc(post.name || post.username)}</b> ${esc(post.caption)}</p>` : `<p class="ig-caption muted">Sem legenda.</p>`}
      <div class="ig-post-comments-preview">
        ${preview || '<p class="ig-post-comment-preview">Nenhum comentario ainda.</p>'}
        <button type="button" data-ig-focus-comment="${esc(post.id)}">Ver todos os comentarios</button>
      </div>
    </article>
  `;
}

function openInstagramPostDetail(postId, loadComments = true) {
  instagram.detailPost = String(postId || "");
  if (!instagram.detailPost) return;
  if (loadComments && bridge !== null) sendInstagramAction("insta_comment_open", `post=${encodePayload(instagram.detailPost)}`);
  renderInstagramPostDetail();
}

function closeInstagramPostDetail() {
  instagram.detailPost = "";
  $("#igPostDetail").hidden = true;
}

function openInstagramProfile(account) {
  const target = String(account || "");
  if (!target) return;
  closeInstagramComments();
  closeInstagramPostDetail();
  instagram.viewingProfile = target;
  if (bridge !== null) sendInstagramAction("insta_profile_open", `target=${encodePayload(target)}`);
  setInstagramTab("profile");
  renderInstagramProfile();
}

function renderInstagramExplore() {
  const box = $("#igUsers");
  if (!box) return;
  const query = instagram.query.trim().toLowerCase();
  if (!query && !instagram.suggestions.length) refreshInstagramSuggestions();
  const suggested = new Set(instagram.suggestions);
  const rows = instagram.users.filter((user) => query
    ? `${user.name} ${user.username} ${user.bio}`.toLowerCase().includes(query)
    : suggested.has(String(user.id))).filter((user) => query || (!user.following && !isInstagramOwnPerson(user))).slice(0, query ? 60 : 5);
  if (!rows.length) {
    box.innerHTML = instagramEmpty(query ? "Nenhum jogador encontrado." : "Nenhum perfil encontrado.");
    return;
  }
  box.innerHTML = rows.map((user) => `
    <article class="ig-user-row" data-ig-view-profile="${esc(user.id)}">
      ${instagramAvatar(user)}
      <span><strong>${esc(user.name || user.username)}</strong><small>@${esc(user.username || user.id)} - ${user.followers || 0} seguidores</small></span>
      <button type="button" class="${user.following ? "following" : ""}" data-ig-follow="${esc(user.id)}">${user.following ? "Seguindo" : "Seguir"}</button>
      <button type="button" data-ig-dm-peer="${esc(user.id)}">${icon("i-send")}</button>
    </article>
  `).join("");
}

function renderInstagramProfile() {
  const card = $("#igProfile");
  const grid = $("#igProfileGrid");
  if (!card || !grid) return;
  const visiting = instagram.viewingProfile && instagram.users.find((user) => String(user.id) === String(instagram.viewingProfile));
  const me = visiting || instagram.profile;
  const isMine = !visiting;
  card.innerHTML = `
    <section class="ig-profile-hero">
      <button type="button" class="ig-profile-menu" ${isMine ? 'aria-label="Menu"' : 'data-ig-profile-back aria-label="Voltar"'}><svg><use href="#${isMine ? "i-more" : "i-back"}"/></svg></button>
      ${instagramAvatar(me, "big")}
      <div class="ig-profile-numbers"><span><b>${me.followers || 0}</b><small>Seguidores</small></span><span><b>${isMine ? (me.following || 0) : (me.followingCount || 0)}</b><small>Seguindo</small></span></div>
      <strong>${esc(me.name || profile.name)}</strong>
      <small>@${esc(me.username || me.id || profile.accountId)}</small>
      <p>${esc(me.bio || "Bio vazia.")}</p>
      <div class="ig-profile-actions">${isMine
        ? '<button type="button" id="igEditBio">Editar perfil</button><button type="button" data-ig-tab="create">Postar</button>'
        : `<button type="button" data-ig-follow="${esc(me.id)}">${me.following ? "Seguindo" : "Seguir"}</button><button type="button" data-ig-dm-peer="${esc(me.id)}">Mensagem</button>`
      }</div>
    </section>
  `;
  const myPosts = instagram.posts.filter((post) => String(post.account) === String(me.id || profile.accountId));
  grid.innerHTML = myPosts.length ? myPosts.map((post) => `<button type="button" class="ig-grid-photo" data-ig-open-post="${esc(post.id)}">${resolvedImageHtml(post.media, 'loading="lazy"')}</button>`).join("") : instagramEmpty("Seu grid ainda esta vazio.");
}

function renderInstagramEditProfile() {
  const panel = $("#igEditProfilePanel");
  if (!panel) return;
  const avatar = instagram.edit.avatar || instagram.profile.avatar || "";
  $("#igEditBioInput").value = instagram.profile.bio || "";
  $("#igEditAvatarUrl").value = avatar && /^https?:\/\//i.test(avatar) ? avatar : "";
  const resolvedAvatar = avatar ? (resolveMediaUrl(avatar) || (!needsNativeMediaResolve(avatar) ? avatar : "")) : "";
  $("#igEditAvatarPreview").innerHTML = resolvedAvatar ? `<img src="${esc(resolvedAvatar)}" alt="" />` : `<b>${esc(instagramInitials(instagram.profile.name))}</b>`;
}

function openInstagramEditProfile() {
  instagram.edit = { open: true, avatar: instagram.profile.avatar || "" };
  $("#igEditProfilePanel").hidden = false;
  renderInstagramEditProfile();
}

function closeInstagramEditProfile() {
  instagram.edit.open = false;
  $("#igEditProfilePanel").hidden = true;
}

function saveInstagramEditProfile() {
  const bio = ($("#igEditBioInput")?.value || "").trim().slice(0, 160);
  const link = ($("#igEditAvatarUrl")?.value || "").trim();
  if (link && !/^https?:\/\//i.test(link)) {
    return showToast("Use um link de imagem valido.");
  }
  if (/\.gif(?:[?#].*)?$/i.test(link)) {
    return showToast("GIF animado nao e aceito.");
  }
  const avatar = link || instagram.edit.avatar || instagram.profile.avatar || "";
  persistInstagramProfileUpdate(bio, avatar);
  closeInstagramEditProfile();
}

function persistInstagramProfileUpdate(bio = instagram.profile.bio || "", avatar = instagram.profile.avatar || "") {
  const nextProfile = {
    ...instagram.profile,
    id: instagram.profile.id || profile.accountId,
    username: instagram.profile.username,
    name: instagram.profile.name || profile.name || "Jogador",
    bio,
    avatar
  };
  instagram.profile = nextProfile;
  instagramUpsert(instagram.users, nextProfile);
  applyInstagramPersonUpdate(nextProfile);
  const payload = `bio=${encodePayload(bio)};avatar=${encodePayload(avatar)}`;
  sendInstagramAction("insta_profile_update", payload);
  sendInstagramAction("insta_bio", payload);
  renderInstagram();
}

function renderInstagramActivity() {
  const box = $("#igActivity");
  if (!box) return;
  if (!instagram.activity.length) {
    box.innerHTML = instagramEmpty("Sem notificacoes por enquanto.");
    return;
  }
  box.innerHTML = instagram.activity.map((item) => `
    <button type="button" class="ig-activity-row" data-ig-activity="${esc(item.id)}">
      ${instagramAvatar({ name: item.actorName, avatar: item.avatar })}
      <span><strong>${esc(item.actorName || "Instagram")}</strong><small>${esc(item.text)} · ${esc(item.time)}</small></span>
    </button>
  `).join("");
}

function openInstagramActivity(activityId) {
  const item = instagram.activity.find((activity) => String(activity.id) === String(activityId));
  if (!item) return;
  const eventType = String(item.eventType || "").toLowerCase();
  if (item.post) {
    openInstagramPostDetail(item.post, eventType !== "comment");
    if (eventType === "comment") openInstagramComments(item.post, item.actor);
    return;
  }
  if (item.actor) {
    openInstagramProfile(item.actor);
  }
}

function renderInstagramDirect() {
  const list = $("#igDirectList");
  const thread = $("#igDirectThread");
  const messages = $("#igDirectMessages");
  if (!list || !thread || !messages) return;
  list.innerHTML = instagram.conversations.length ? instagram.conversations.map((conv) => `
    <button type="button" class="ig-direct-row" data-ig-open-dm="${esc(conv.id)}">
      ${instagramAvatar(conv)}
      <span><strong>${esc(conv.name || conv.username || conv.id)}</strong><small>${esc(conv.text || "Toque para conversar")} · ${esc(conv.time || "")}</small></span>
      ${conv.unread ? `<em>${conv.unread}</em>` : ""}
    </button>
  `).join("") : instagramEmpty("Nenhuma conversa ainda.");
  const peer = instagram.currentPeer;
  thread.hidden = !peer;
  if (!peer) return;
  const person = instagram.conversations.find((item) => String(item.id) === String(peer)) || instagram.users.find((item) => String(item.id) === String(peer)) || { id: peer, name: peer };
  $("#igThreadPerson").innerHTML = `<div class="ig-thread-person">${instagramAvatar(person)}<span><strong>${esc(person.name || person.username || person.id)}</strong><small class="${person.online ? "online" : ""}">${person.online ? "Online agora" : "Offline"}</small></span></div>`;
  $("#igBlockPeer").classList.toggle("blocked", Boolean(person.blocked));
  $("#igBlockPeer").setAttribute("aria-label", person.blocked ? "Perfil bloqueado" : "Bloquear mensagens");
  $("#igBlockPeer").setAttribute("title", person.blocked ? "Perfil bloqueado" : "Bloquear mensagens");
  const rows = instagram.messages[peer] || [];
  messages.innerHTML = rows.length ? rows.map((msg) => `<p class="${msg.fromMe ? "me" : "them"}"><span>${esc(msg.text)}</span><small>${esc(msg.time)}</small></p>`).join("") : instagramEmpty("Comece a conversa.");
  messages.scrollTop = messages.scrollHeight;
}

function openInstagramCompose(mode, image, preview = image) {
  instagram.compose = { open: true, mode, image, preview, caption: "" };
  $("#igComposeTitle").textContent = mode === "story" ? "Novo story" : "Nova publicacao";
  $("#igComposeImage").src = resolveMediaUrl(preview || image) || EMPTY_IMAGE_DATA;
  $("#igCaptionInput").value = "";
  $("#igCaptionInput").hidden = mode === "story";
  $("#igCompose").classList.toggle("story", mode === "story");
  $("#igCompose").classList.toggle("post", mode !== "story");
  $("#igCompose").hidden = false;
}

function closeInstagramCompose() {
  instagram.compose.open = false;
  instagram.compose.image = "";
  instagram.compose.preview = "";
  $("#igCompose").hidden = true;
}

function publishInstagramCompose() {
  const image = instagram.compose.image;
  if (!image) return showToast("Tire uma foto primeiro.");
  const mode = instagram.compose.mode;
  const caption = mode === "story" ? "" : ($("#igCaptionInput")?.value.trim() || "");
  const action = mode === "story" ? "insta_story" : "insta_post";
  if (mode === "story") {
    instagramUpsert(instagram.stories, {
      id: `local_story_${Date.now()}`,
      account: instagramOwnId(),
      username: instagram.profile.username,
      name: instagram.profile.name || profile.name,
      avatar: instagram.profile.avatar,
      media: image,
      caption: "",
      time: nowText(),
      liked: false,
      views: 0
    });
    renderInstagramStories();
  } else {
    const localId = `local_post_${Date.now()}`;
    instagramUpsert(instagram.posts, {
      id: localId,
      account: instagramOwnId(),
      username: instagram.profile.username,
      name: instagram.profile.name || profile.name,
      avatar: instagram.profile.avatar,
      media: image,
      caption,
      time: nowText(),
      likes: 0,
      comments: 0,
      liked: false
    });
    instagram.feedMode = "live";
    instagram.feedIds = [localId, ...instagram.feedIds.filter((id) => String(id) !== localId)];
  }
  sendInstagramAction(action, `url=${encodePayload(image)};caption=${encodePayload(caption)}`);
  closeInstagramCompose();
  setInstagramTab("feed");
  renderInstagram();
}

function openInstagramStory(storyId, keepSequence = false) {
  const story = instagram.stories.find((item) => String(item.id) === String(storyId));
  if (!story) return;
  const groupKey = instagramStoryGroupKey(story);
  const group = instagram.stories
    .filter((item) => instagramStoryGroupKey(item) === groupKey)
    .sort((a, b) => instagramStorySortValue(a) - instagramStorySortValue(b));
  const index = Math.max(0, group.findIndex((item) => String(item.id) === String(story.id)));
  if (!keepSequence) {
    instagram.storySeen = new Set();
    instagram.storyGroup = group;
    instagram.storyIndex = index;
  } else if (!instagram.storyGroup.length || !instagram.storyGroup.some((item) => String(item.id) === String(story.id))) {
    instagram.storyGroup = group;
    instagram.storyIndex = index;
  } else {
    instagram.storyIndex = index;
  }
  instagram.storySeen.add(String(story.id));
  window.clearTimeout(instagram.storyTimer);
  instagram.story = story;
  $("#igStoryImage").src = resolveMediaUrl(story.media) || EMPTY_IMAGE_DATA;
  const nextStory = group[index + 1];
  const previousStory = group[index - 1];
  if (nextStory?.media) resolveMediaUrl(nextStory.media);
  if (previousStory?.media) resolveMediaUrl(previousStory.media);
  $("#igStoryName").textContent = story.username || story.name || "Story";
  $("#igStoryTime").textContent = story.time || "";
  const isMine = isInstagramOwnPerson(story);
  $("#igStoryLike").classList.toggle("active", Boolean(story.liked));
  $("#igStoryLike").hidden = isMine;
  $("#igStoryReplyForm").hidden = isMine;
  $("#igStoryViews").hidden = !isMine;
  $("#igStoryViews").textContent = `${Number(story.views || 0)} visualizacao${Number(story.views || 0) === 1 ? "" : "es"}`;
  $("#igStoryMore").hidden = !isMine;
  $("#igStoryMenu").hidden = true;
  $("#igStoryViewersPanel").hidden = true;
  if (!isMine && !String(story.id).startsWith("local_story_")) {
    sendInstagramAction("insta_story_view", `story=${encodePayload(story.id)}`);
  }
  renderInstagramStoryProgress();
  $("#igStoryViewer").hidden = false;
  instagram.storyTimer = window.setTimeout(openNextInstagramStory, INSTAGRAM_STORY_SECONDS * 1000);
}

function renderInstagramStoryProgress() {
  const progress = $("#igStoryProgress");
  if (!progress) return;
  const group = instagram.storyGroup.length ? instagram.storyGroup : (instagram.story ? [instagram.story] : []);
  progress.innerHTML = group.map((story, index) => {
    const cls = index < instagram.storyIndex ? "done" : index === instagram.storyIndex ? "active" : "";
    const width = index < instagram.storyIndex ? "100%" : "0%";
    return `<span class="${cls}" data-story-index="${index}"><i style="width:${width}"></i></span>`;
  }).join("");
  const active = progress.querySelector(`span[data-story-index="${instagram.storyIndex}"] i`);
  if (active) {
    active.style.animation = "none";
    active.style.width = "0";
    active.classList.remove("playing");
    void active.offsetWidth;
    window.requestAnimationFrame(() => {
      active.classList.add("playing");
      active.style.animation = `igStoryProgress ${INSTAGRAM_STORY_SECONDS}s linear forwards`;
    });
  }
}

function closeInstagramStory() {
  window.clearTimeout(instagram.storyTimer);
  instagram.storyTimer = 0;
  instagram.story = null;
  instagram.storyGroup = [];
  instagram.storyIndex = 0;
  instagram.storySeen.clear();
  $("#igStoryMenu").hidden = true;
  $("#igStoryViewersPanel").hidden = true;
  $("#igStoryViewer").hidden = true;
}

function openNextInstagramStory() {
  if (!instagram.story) return;
  window.clearTimeout(instagram.storyTimer);
  const nextInGroup = instagram.storyGroup[instagram.storyIndex + 1];
  if (nextInGroup) return openInstagramStory(nextInGroup.id, true);
  const groups = getInstagramStoryGroups();
  const currentGroupIndex = groups.findIndex((group) => String(group.account) === instagramStoryGroupKey(instagram.story));
  const nextGroup = groups[currentGroupIndex + 1];
  if (nextGroup) return openInstagramStory(nextGroup.stories[0].id, false);
  closeInstagramStory();
}

function openPreviousInstagramStory() {
  if (!instagram.story) return;
  window.clearTimeout(instagram.storyTimer);
  const previousInGroup = instagram.storyGroup[instagram.storyIndex - 1];
  if (previousInGroup) return openInstagramStory(previousInGroup.id, true);
  renderInstagramStoryProgress();
  instagram.storyTimer = window.setTimeout(openNextInstagramStory, INSTAGRAM_STORY_SECONDS * 1000);
}

function handleInstagramStoryTap(event) {
  const viewer = $("#igStoryViewer");
  if (!viewer || viewer.hidden) return false;
  if (event.target.closest("button, form, input, .ig-story-menu")) return false;
  const rect = viewer.getBoundingClientRect();
  const x = event.clientX || rect.left + rect.width / 2;
  if (x < rect.left + rect.width * 0.42) openPreviousInstagramStory();
  else openNextInstagramStory();
  return true;
}

function toggleInstagramStoryLike() {
  const story = instagram.story;
  if (!story || isInstagramOwnPerson(story)) return;
  story.liked = !story.liked;
  $("#igStoryLike").classList.toggle("active", story.liked);
  sendInstagramAction("insta_story_like", `story=${encodePayload(story.id)};on=${story.liked ? 1 : 0}`);
}

function deleteInstagramStory() {
  const story = instagram.story;
  if (!story || !isInstagramOwnPerson(story)) return;
  const storyId = String(story.id);
  instagram.stories = instagram.stories.filter((item) => String(item.id) !== storyId);
  instagram.storyGroup = instagram.storyGroup.filter((item) => String(item.id) !== storyId);
  sendInstagramAction("insta_story_delete", `story=${encodePayload(storyId)}`);
  $("#igStoryMenu").hidden = true;
  showToast("Story excluido.");
  if (instagram.storyGroup[instagram.storyIndex]) return openInstagramStory(instagram.storyGroup[instagram.storyIndex].id, true);
  if (instagram.storyGroup[instagram.storyIndex - 1]) return openInstagramStory(instagram.storyGroup[instagram.storyIndex - 1].id, true);
  closeInstagramStory();
  renderInstagramStories();
}

function renderInstagramStoryViewers() {
  const panel = $("#igStoryViewersPanel");
  const list = $("#igStoryViewersList");
  const story = instagram.story;
  if (!panel || !list || !story) return;
  const rows = instagram.storyViewers[String(story.id)] || [];
  list.innerHTML = rows.length ? rows.map((viewer) => `
    <button type="button" data-ig-view-profile="${esc(viewer.id)}">
      ${instagramAvatar(viewer)}
      <span><b>${esc(viewer.name || viewer.username)}</b><small>@${esc(viewer.username || viewer.id)} - ${esc(viewer.time || "agora")}</small></span>
    </button>
  `).join("") : "<p>Ninguem visualizou ainda.</p>";
}

function openInstagramStoryViewers() {
  const story = instagram.story;
  if (!story || !isInstagramOwnPerson(story)) return;
  const panel = $("#igStoryViewersPanel");
  if (!panel) return;
  panel.hidden = !panel.hidden;
  if (!panel.hidden) {
    window.clearTimeout(instagram.storyTimer);
    sendInstagramAction("insta_story_viewers", `story=${encodePayload(story.id)}`);
    renderInstagramStoryViewers();
  } else {
    instagram.storyTimer = window.setTimeout(openNextInstagramStory, INSTAGRAM_STORY_SECONDS * 1000);
  }
}

function openInstagramDirect(peer = "") {
  if (peer) {
    instagram.currentPeer = String(peer);
    const conversation = instagram.conversations.find((item) => String(item.id) === String(peer));
    if (conversation) conversation.unread = 0;
    refreshInstagramUnread();
    sendInstagramAction("insta_dm_open", `peer=${encodePayload(peer)}`);
  }
  $("#igDirectPanel").hidden = false;
  renderInstagramDirect();
}

function closeInstagramDirect() {
  $("#igDirectPanel").hidden = true;
  instagram.currentPeer = "";
  renderInstagramDirect();
}

function closeInstagramThread() {
  instagram.currentPeer = "";
  $("#igThreadMenu").hidden = true;
  renderInstagramDirect();
}

function toggleInstagramPeerBlock() {
  const peer = instagram.currentPeer;
  if (!peer) return;
  const person = instagram.conversations.find((item) => String(item.id) === String(peer)) || instagram.users.find((item) => String(item.id) === String(peer));
  if (!person) return;
  if (person.blocked) return showToast("Desbloqueie esse perfil pelas configuracoes.");
  instagram.conversations.forEach((item) => {
    if (String(item.id) === String(peer)) item.blocked = true;
  });
  instagram.users.forEach((item) => {
    if (String(item.id) === String(peer)) item.blocked = true;
  });
  sendInstagramAction("insta_block", `target=${encodePayload(peer)};on=1`);
  $("#igThreadMenu").hidden = true;
  renderInstagramDirect();
  renderConfig();
  showToast("Mensagens bloqueadas.");
}

function renderInstagramBadges() {
  setBadge("#homeInstaBadge", instagram.unread);
  setBadge("#igTopBadge", instagram.unread);
}

function renderInstagram() {
  if (!$(".instagram-view")) return;
  if (!instagram.profile.id && profile.accountId) instagram.profile.id = profile.accountId;
  if (!instagram.profile.name || instagram.profile.name === "Jogador") instagram.profile.name = profile.name;
  setInstagramTab(instagram.tab);
  renderInstagramOnboarding();
  renderInstagramStories();
  renderInstagramFeed();
  renderInstagramComments();
  renderInstagramPostDetail();
  renderInstagramExplore();
  renderInstagramProfile();
  renderInstagramActivity();
  renderInstagramDirect();
  renderInstagramBadges();
}
function renderAll() {
  updateProfile();
  renderChats();
  renderContacts();
  if (state.view === "chat") renderMessages();
  renderStatus();
  renderGroups();
  renderCalls();
  renderConfig();
  renderBank();
  renderSportfy();
  renderInstagram();
  renderRadarBot();
  syncBadges();
}
function loadImageCard(card) {
  if (!card || !card.classList.contains("pending")) return;
  const rawUrl = card.dataset.imageUrl || "";
  const url = resolveMediaUrl(rawUrl);
  if (!url) {
    renderImagePlaceholder(card, rawUrl ? "Carregando foto" : "Imagem indisponivel");
    return;
  }
  card.classList.remove("pending");
  card.textContent = "";
  const img = document.createElement("img");
  img.alt = "";
  img.loading = "eager";
  img.decoding = "async";
  img.src = url;
  card.appendChild(img);
}

function imagePlaceholderHtml(text = "Toque para carregar") {
  return `<span class="image-placeholder"><b>Foto</b><small>${esc(text)}</small></span>`;
}

function renderImagePlaceholder(card, text = "Toque para tentar de novo") {
  if (!card) return;
  card.classList.add("pending");
  card.innerHTML = imagePlaceholderHtml(text);
}

function setTab(tab) {
  state.tab = tab;
  const whatsappScreen = $(".whatsapp-screen");
  if (whatsappScreen) whatsappScreen.dataset.activeTab = tab;
  $all("[data-tab]").forEach((button) => button.classList.toggle("active", button.dataset.tab === tab));
  $all("[data-tab-page]").forEach((page) => page.classList.toggle("active", page.dataset.tabPage === tab));
}
function openContactModal(phone = "", locked = false) {
  $("#contactForm").reset();
  $("#contactPhone").value = phone;
  $("#contactPhone").readOnly = locked;
  $("#contactPhoneLabel").hidden = locked;
  state.lockedContactPhone = locked;
  openModal("contact");
}

function callDisplayName(phone, name = "") {
  const normalized = normalizePhone(phone) || phone;
  return name || findContact(normalized)?.name || chats[findChat(normalized)]?.name || normalized || "Contato";
}

function pushCallLog(phone, name, note) {
  const title = callDisplayName(phone, name);
  calls.unshift({ phone, name: title, note: note || nowText() });
  if (calls.length > 30) calls.length = 30;
  renderCalls();
}

function updateCallTimer() {
  if (state.call.status !== "active") return;
  const elapsed = Math.floor((Date.now() - state.call.startedAt) / 1000);
  $("#callTimer").textContent = formatCallDuration(elapsed);
}

function stopCallTimer() {
  window.clearInterval(state.call.timer);
  state.call.timer = 0;
}

function setCallScreen(status, phone, name, direction = "") {
  const normalized = normalizePhone(phone) || phone || state.call.phone;
  const title = callDisplayName(normalized, name || state.call.name);
  state.call.status = status;
  state.call.phone = normalized;
  state.call.name = title;
  state.call.direction = direction || state.call.direction;
  $("#callName").textContent = title;
  $("#callPhone").textContent = normalized;
  $("#callAvatar").textContent = initials(title);
  $("#callStateLabel").textContent = status === "active" ? "Em ligacao" : status === "incoming" ? "Ligacao recebida" : "Ligando";
  $("#callTimer").textContent = status === "active" ? "00:00" : status === "incoming" ? "Recebendo chamada" : "Chamando...";
}

function beginActiveCall(phone, name, direction = "") {
  setCallScreen("active", phone, name, direction);
  bridge?.setCallVoice?.(true);
  state.call.startedAt = Date.now();
  stopCallTimer();
  updateCallTimer();
  state.call.timer = window.setInterval(updateCallTimer, 1000);
  $("#incomingCall").hidden = true;
  showView("call");
}

function clearCallUi(message = "") {
  bridge?.setCallVoice?.(false);
  stopCallTimer();
  $("#incomingCall").hidden = true;
  if (message && state.view === "call") {
    $("#callTimer").textContent = message;
    window.setTimeout(() => {
      if (state.view === "call" && state.call.status !== "active") showView(state.call.returnView || "whatsapp");
    }, 1200);
  }
  state.call = { status: "idle", phone: "", name: "", direction: "", startedAt: 0, timer: 0, returnView: state.call.returnView || "whatsapp" };
}

function showIncomingCall(phone, name) {
  const title = callDisplayName(phone, name);
  state.call.returnView = state.view === "call" ? "whatsapp" : state.view;
  setCallScreen("incoming", phone, title, "in");
  $("#incomingName").textContent = title;
  $("#incomingText").textContent = "Deseja atender?";
  $("#incomingCall").hidden = false;
}

function startCall(phone, name = "") {
  const normalized = normalizePhone(phone);
  if (!normalized) return showToast("Numero invalido.");
  setCallScreen("outgoing", normalized, name, "out");
  showView("call");
  bridge?.action?.("call_start", `to=${encodePayload(normalized)}`);
}

function acceptCall() {
  if (state.call.phone) bridge?.action?.("call_accept", `phone=${encodePayload(state.call.phone)}`);
  beginActiveCall(state.call.phone, state.call.name, "in");
}

function rejectCall() {
  if (state.call.phone) bridge?.action?.("call_reject", `phone=${encodePayload(state.call.phone)}`);
  clearCallUi("Chamada recusada");
}

function endCall() {
  if (state.call.phone) bridge?.action?.("call_end", `phone=${encodePayload(state.call.phone)}`);
  clearCallUi("Chamada encerrada");
}

function openFromNative(payload) {
  fitPhone();
  resetToHomeInstant();
  replayPhoneOpenAnimation();
  const data = payloadToObject(payload);
  if (data.accountId) profile.accountId = decode(data.accountId);
  if (data.name) profile.name = decode(data.name);
  if (data.bio) profile.bio = decode(data.bio);
  if (data.phone) profile.phone = normalizePhone(decode(data.phone));
  const scopedAccount = accountStorageSuffix();
  if (phoneScopedAccount !== scopedAccount) {
    phoneScopedAccount = scopedAccount;
    blockedContacts.splice(0, blockedContacts.length);
    loadPhoneSettings();
    sportfyStopNativeAudio(true);
    sportfyLoadCache();
  } else if (sportfyLoadedAccount !== scopedAccount) {
    sportfyLoadCache();
  }
  applyPhoneSettings(true);
  fitPhone();
  contacts.splice(1);
  chats.length = 0;
  statuses.length = 0;
  groups.length = 0;
  calls.length = 0;
  bank.transactions.length = 0;
  bank.transactionIds.clear();
  bank.receipt = null;
  bank.amountDigits = "";
  bank.target = null;
  state.currentChat = -1;
  if (state.call.status === "idle") clearCallUi();
  renderAll();
  if (state.call.status === "active") showView("call");
}
function receiveFromNative(payload) {
  const data = payloadToObject(payload);
  if (data.type === "contact") {
    saveContactLocal(normalizePhone(decode(data.phone)), decode(data.name));
    scheduleRenderAll();
    return;
  }
  if (data.type === "presence") {
    const phone = normalizePhone(decode(data.phone || data.from));
    const index = findChat(phone);
    if (index >= 0) {
      chats[index].online = data.online === "1";
      if (state.currentChat === index) $("#chatStatus").textContent = chats[index].online ? "online agora" : "offline";
      scheduleRenderChats();
    }
    return;
  }
  if (data.type === "bank") {
    updateBankFromEvent(data);
    return;
  }
  if (data.type === "bank_finance") {
    updateBankFromEvent(data);
    return;
  }
  if (data.type === "bank_txn") {
    addBankTransactionFromEvent(data);
    return;
  }
  if (data.type === "bank_lookup") {
    bank.pending = false;
    if (decode(data.status) !== "ok") {
      showToast(decode(data.message) || "Jogador nao encontrado.");
      return;
    }
    setBankTarget({
      id: decode(data.target || data.id),
      name: decode(data.name || ""),
      bank: decode(data.bank || "Mercantil"),
      agency: decode(data.agency || "0001"),
      account: decode(data.account || bankAccountFromId(data.target || data.id))
    });
    return;
  }
  if (data.type === "bank_transfer") {
    applyBankTransfer(data);
    return;
  }
  if (data.type === "bank_invoice") {
    if (decode(data.status) === "ok") {
      updateBankFromEvent(data);
      showToast(decode(data.message) || "Fatura paga.");
    } else {
      showToast(decode(data.message) || "Nao foi possivel pagar.");
    }
    return;
  }
  if (String(data.type || "").startsWith("radarbot")) {
    updateRadarBotFromEvent(data);
    return;
  }
  if (String(data.type || "").startsWith("sportfy")) {
    updateSportfyFromEvent(data);
    return;
  }
  if (String(data.type || "").startsWith("insta")) {
    updateInstagramFromEvent(data);
    return;
  }
  if (data.type === "call") {
    const status = decode(data.status || data.action || "");
    const phone = normalizePhone(decode(data.from || data.to || data.peer));
    const name = decode(data.name || "");
    if (!phone && status !== "ended") return;
    if (status === "incoming") {
      showIncomingCall(phone, name);
      pushCallLog(phone, name, "Recebida agora");
      return;
    }
    if (status === "outgoing") {
      setCallScreen("outgoing", phone, name, "out");
      showView("call");
      return;
    }
    if (status === "active" || status === "accepted") {
      beginActiveCall(phone, name, decode(data.direction || ""));
      return;
    }
    if (status === "busy") {
      pushCallLog(phone, name, "Ocupado agora");
      setCallScreen("busy", phone, name, "out");
      clearCallUi("Ocupado");
      return;
    }
    if (status === "unavailable") {
      pushCallLog(phone, name, "Indisponivel agora");
      setCallScreen("unavailable", phone, name, "out");
      clearCallUi("Indisponivel");
      return;
    }
    if (status === "rejected") {
      pushCallLog(phone, name, "Recusada agora");
      setCallScreen("rejected", phone, name, state.call.direction);
      clearCallUi("Chamada recusada");
      return;
    }
    if (status === "missed") {
      pushCallLog(phone, name, "Perdida agora");
      clearCallUi();
      return;
    }
    if (status === "ended" || status === "cancelled" || status === "timeout") {
      pushCallLog(phone || state.call.phone, name || state.call.name, status === "timeout" ? "Nao atendida" : "Encerrada agora");
      clearCallUi(status === "timeout" ? "Nao atendida" : "Chamada encerrada");
      return;
    }
    return;
  }
  if (data.type !== "message") return;
  const phone = normalizePhone(decode(data.from));
  const kind = decode(data.kind || "text");
  const audioDuration = Math.max(1, Math.min(60, Number(decode(data.duration || "")) || 0));
  const text = kind === "location" ? "Localizacao compartilhada" : kind === "audio" ? `Audio ${formatVoiceDuration(audioDuration)}` : kind === "image" ? "Foto" : decode(data.text);
  if (!phone || !text) return;
  const fromMe = data.direction === "out";
  if (!fromMe && isBlockedPhone(phone)) return;
  const index = ensureChat(phone, decode(data.name), data.online === "1");
  const chat = chats[index];
  if (data.online !== undefined) chat.online = data.online === "1";
  const contact = findContact(phone);
  if (contact) {
    chat.name = contact.name;
    chat.avatar = contact.avatar;
  }
  const sentAt = eventTime(data);
  chat.messages.push({
    from: fromMe ? "me" : "them",
    kind,
    text,
    time: sentAt,
    x: decode(data.x || ""),
    y: decode(data.y || ""),
    z: decode(data.z || ""),
    url: decode(data.url || ""),
    duration: audioDuration
  });
  chat.preview = text;
  chat.time = sentAt;
  const active = state.view === "chat" && state.currentChat === index;
  if (!fromMe && data.unread !== "0" && !active) chat.unread = (chat.unread || 0) + 1;
  if (active) bridge?.action?.("read", `phone=${encodePayload(phone)}`);
  scheduleRenderChats();
  if (active) {
    scheduleRenderMessages();
  }
}
function closeFromNative() {
  resetToHomeInstant();
}

function refreshFromNative() {
  fitPhone();
  window.setTimeout(fitPhone, 80);
  window.setTimeout(fitPhone, 240);
  renderAll();
}

function updateVoiceTimer() {
  if (!state.voice.recording) return;
  const elapsed = Math.min(60, Math.floor((Date.now() - state.voice.startedAt) / 1000));
  $("#voiceTime").textContent = formatVoiceDuration(elapsed);
}

function startVoiceRecording(event) {
  const composer = $("#composer");
  const input = $("#messageInput");
  const chat = chats[state.currentChat];
  if (!chat || input.value.trim() || state.voice.recording) return;
  event.preventDefault();
  event.stopPropagation();
  state.voice.recording = true;
  state.voice.pointerId = event.pointerId ?? null;
  state.voice.startedAt = Date.now();
  $("#voiceHint").textContent = "Solte para enviar";
  $("#voiceRecording").hidden = false;
  composer.classList.add("recording");
  updateVoiceTimer();
  state.voice.timer = window.setInterval(updateVoiceTimer, 220);
  state.voice.timeout = window.setTimeout(() => finishVoiceRecording(true), 60000);
  if (bridge?.startVoice) bridge.startVoice(chat.phone);
  else {
    $("#voiceHint").textContent = "Gravacao disponivel no app";
    finishVoiceRecording(false);
  }
}

function finishVoiceRecording(send = true) {
  if (!state.voice.recording) return;
  state.voice.recording = false;
  window.clearInterval(state.voice.timer);
  window.clearTimeout(state.voice.timeout);
  $("#composer").classList.remove("recording");
  $("#voiceHint").textContent = send ? "Enviando audio..." : "Audio cancelado";
  if (send) bridge?.stopVoice?.();
  else bridge?.cancelVoice?.();
}

function voiceNativeStatus(status, duration, message) {
  if (status === "recording") return;
  if (status === "uploading") {
    $("#voiceRecording").hidden = false;
    $("#voiceTime").textContent = formatVoiceDuration(duration);
    $("#voiceHint").textContent = "Enviando audio...";
    return;
  }
  if (status === "error") {
    $("#voiceRecording").hidden = false;
    $("#voiceHint").textContent = message || "Nao foi possivel enviar";
    window.setTimeout(() => { $("#voiceRecording").hidden = true; }, 1200);
    return;
  }
  $("#voiceRecording").hidden = true;
}

function stopCurrentAudio() {
  if (currentAudioFrame) {
    cancelAnimationFrame(currentAudioFrame);
    currentAudioFrame = 0;
  }
  if (currentAudioNative) {
    bridge?.stopAudio?.();
    currentAudioNative = false;
  }
  if (currentAudio) {
    currentAudio.pause?.();
    currentAudio = null;
  }
  resetAudioBubble(currentAudioButton);
  currentAudioButton = null;
}

function resetAudioBubble(button) {
  if (!button) return;
  const bubble = button.closest(".bubble.audio");
  const durationLabel = bubble?.querySelector(".audio-duration");
  const original = Number(durationLabel?.dataset.originalDuration || button.dataset.audioDuration || 0);
  button.classList.remove("playing");
  button.innerHTML = icon("i-play");
  button.setAttribute("aria-label", "Reproduzir audio");
  bubble?.classList.remove("playing");
  bubble?.querySelectorAll(".audio-wave span.filled").forEach((bar) => bar.classList.remove("filled"));
  if (durationLabel) durationLabel.textContent = formatVoiceDuration(original);
}

function updateAudioBubble(button, audio) {
  if (!button || !audio) return;
  const bubble = button.closest(".bubble.audio");
  const bars = Array.from(bubble?.querySelectorAll(".audio-wave span") || []);
  const durationLabel = bubble?.querySelector(".audio-duration");
  const fallbackDuration = Number(button.dataset.audioDuration || durationLabel?.dataset.originalDuration || 1);
  const total = Number.isFinite(audio.duration) && audio.duration > 0 ? audio.duration : fallbackDuration;
  const progress = Math.max(0, Math.min(1, audio.currentTime / Math.max(0.1, total)));
  const filled = Math.min(bars.length, Math.floor(progress * bars.length));
  bars.forEach((bar, index) => bar.classList.toggle("filled", index < filled));
  if (durationLabel) durationLabel.textContent = formatVoiceDuration(Math.ceil(Math.max(0, total - audio.currentTime)));
  if (!audio.paused && !audio.ended && !currentAudioFrame) {
    currentAudioFrame = requestAnimationFrame(() => {
      currentAudioFrame = 0;
      updateAudioBubble(button, audio);
    });
  }
}

function updateNativeAudioBubble() {
  if (!currentAudioNative || !currentAudioButton) return;
  const elapsed = Math.max(0, (Date.now() - currentAudioStartedAt) / 1000);
  updateAudioBubble(currentAudioButton, {
    currentTime: Math.min(currentAudioDuration, elapsed),
    duration: currentAudioDuration,
    paused: false,
    ended: elapsed >= currentAudioDuration
  });
  if (elapsed >= currentAudioDuration) {
    audioNativeStatus("ended");
    return;
  }
  currentAudioFrame = requestAnimationFrame(() => {
    currentAudioFrame = 0;
    updateNativeAudioBubble();
  });
}

function playAudio(url, button) {
  if (!url) return;
  if (currentAudioButton === button && (currentAudio || currentAudioNative)) return stopCurrentAudio();
  stopCurrentAudio();
  currentAudioButton = button;
  currentAudioDuration = Math.max(1, Math.min(60, Number(button.dataset.audioDuration || 1)));
  button.classList.add("playing");
  button.innerHTML = icon("i-pause");
  button.setAttribute("aria-label", "Pausar audio");
  button.closest(".bubble.audio")?.classList.add("playing");
  if (bridge?.playAudio && !/^https?:/i.test(String(url))) {
    currentAudioNative = true;
    bridge.playAudio(url);
    return;
  }
  currentAudio = new Audio(url);
  updateAudioBubble(button, currentAudio);
  currentAudio.addEventListener("timeupdate", () => updateAudioBubble(button, currentAudio));
  currentAudio.addEventListener("playing", () => updateAudioBubble(button, currentAudio), { once: true });
  currentAudio.addEventListener("ended", stopCurrentAudio, { once: true });
  currentAudio.addEventListener("error", stopCurrentAudio, { once: true });
  currentAudio.play().catch(stopCurrentAudio);
}

function audioNativeStatus(status) {
  if (status === "playing") {
    currentAudioNative = true;
    currentAudioStartedAt = Date.now();
    updateNativeAudioBubble();
    return;
  }
  const shouldStop = status === "ended" || status === "error" || status === "stopped";
  if (!shouldStop) return;
  currentAudioNative = false;
  if (currentAudioFrame) {
    cancelAnimationFrame(currentAudioFrame);
    currentAudioFrame = 0;
  }
  const button = currentAudioButton;
  currentAudioButton = null;
  resetAudioBubble(button);
  if (status === "error") showToast("Nao foi possivel reproduzir o audio.");
}

function setCameraStatus(text) {
  const status = $("#cameraStatus");
  if (status) status.textContent = text;
}

function cameraFrameVisualCenterX() {
  const frame = $("#cameraFrame")?.getBoundingClientRect();
  const viewportWidth = window.innerWidth || document.documentElement.clientWidth || 1;
  if (!frame || !frame.width || !viewportWidth) return "0.820";
  return Math.max(0.5, Math.min(0.96, (frame.left + frame.width / 2) / viewportWidth)).toFixed(3);
}

function cameraFrameNativeFocusX() {
  const visualCenter = Number(cameraFrameVisualCenterX()) || 0.82;
  if (state.camera.mode !== "selfie") return visualCenter.toFixed(3);
  return Math.max(0.04, Math.min(0.50, 1 - visualCenter)).toFixed(3);
}

function cameraFramePayload() {
  const frame = $("#cameraFrame")?.getBoundingClientRect();
  const viewportWidth = Math.round(window.innerWidth || document.documentElement.clientWidth || 1);
  const viewportHeight = Math.round(window.innerHeight || document.documentElement.clientHeight || 1);
  const left = frame ? Math.round(frame.left) : 0;
  const top = frame ? Math.round(frame.top) : 0;
  const width = frame ? Math.round(frame.width) : 0;
  const height = frame ? Math.round(frame.height) : 0;
  const visualFx = cameraFrameVisualCenterX();
  return [
    `mode=${encodePayload(state.camera.mode)}`,
    `fx=${cameraFrameNativeFocusX()}`,
    `visualFx=${visualFx}`,
    `frameLeft=${left}`,
    `frameTop=${top}`,
    `frameWidth=${width}`,
    `frameHeight=${height}`,
    `viewportWidth=${viewportWidth}`,
    `viewportHeight=${viewportHeight}`
  ].join(";");
}

function clearCameraSyncTimers() {
  (state.camera.syncTimers || []).forEach((timer) => window.clearTimeout(timer));
  state.camera.syncTimers = [];
}

function scheduleCameraSync() {
  clearCameraSyncTimers();
  const steps = [0, 80, 180, 360, 700];
  state.camera.syncTimers = steps.map((delay) => window.setTimeout(() => {
    if (state.view === "camera" && document.body.classList.contains("camera-active")) syncCameraMode();
  }, delay));
}

function updateSelfieMask() {
  const mask = $("#selfieMask");
  const frame = $("#cameraFrame")?.getBoundingClientRect();
  if (!mask || !frame || !document.body.classList.contains("camera-active") || state.view !== "camera") {
    document.body.classList.remove("selfie-camera-active");
    return;
  }
  const width = window.innerWidth || document.documentElement.clientWidth || 1;
  const height = window.innerHeight || document.documentElement.clientHeight || 1;
  const pad = 2;
  const top = Math.max(0, Math.round(frame.top + pad));
  const left = Math.max(0, Math.round(frame.left + pad));
  const right = Math.min(width, Math.round(frame.right - pad));
  const bottom = Math.min(height, Math.round(frame.bottom - pad));
  const pieces = {
    top: { left: 0, top: 0, width, height: top },
    right: { left: right, top, width: Math.max(0, width - right), height: Math.max(0, bottom - top) },
    bottom: { left: 0, top: bottom, width, height: Math.max(0, height - bottom) },
    left: { left: 0, top, width: left, height: Math.max(0, bottom - top) },
  };
  Object.entries(pieces).forEach(([key, rect]) => {
    const node = mask.querySelector(`[data-mask="${key}"]`);
    if (!node) return;
    node.style.left = `${rect.left}px`;
    node.style.top = `${rect.top}px`;
    node.style.width = `${rect.width}px`;
    node.style.height = `${rect.height}px`;
  });
  document.body.classList.add("selfie-camera-active");
}

function syncCameraMode() {
  updateSelfieMask();
  bridge?.action?.("camera_mode", cameraFramePayload());
}

function resetCameraPreview() {
  state.camera.url = "";
  state.camera.previewUrl = "";
  $("#cameraPhoto").hidden = true;
  $("#cameraPhoto").removeAttribute("src");
  $("#cameraLive").hidden = false;
  $("#cameraRetake").hidden = true;
  $("#cameraSend").hidden = true;
  $("#cameraShot").hidden = false;
  $("#cameraFrame").classList.remove("capturing", "has-photo");
  setCameraStatus(state.camera.mode === "selfie" ? "Selfie" : "Ambiente");
  window.requestAnimationFrame(updateSelfieMask);
}

function openCamera(returnView = "chat") {
  state.camera.returnView = returnView;
  resetCameraPreview();
  document.body.classList.add("camera-active");
  showView("camera");
  fitPhone();
  scheduleCameraSync();
}

function stopCameraSession() {
  bridge?.action?.("camera_close", "");
  document.body.classList.remove("camera-active");
  document.body.classList.remove("selfie-camera-active");
  clearCameraSyncTimers();
  state.camera.dragging = false;
  window.clearTimeout(state.camera.moveTimer);
  state.camera.moveTimer = 0;
  state.camera.pendingDx = 0;
  state.camera.pendingDy = 0;
  resetCameraPreview();
}

function closeCamera() {
  state.camera.closing = true;
  stopCameraSession();
  const returnView = String(state.camera.returnView || "chat");
  showView(returnView.startsWith("instagram") ? "instagram" : returnView);
  state.camera.closing = false;
}

function flipCamera() {
  state.camera.mode = state.camera.mode === "selfie" ? "world" : "selfie";
  resetCameraPreview();
  scheduleCameraSync();
}

function adjustCamera(dx, dy) {
  state.camera.pendingDx += dx;
  state.camera.pendingDy += dy;
  const now = Date.now();
  const wait = Math.max(0, 42 - (now - state.camera.lastMove));
  if (wait > 0) {
    if (!state.camera.moveTimer) {
      state.camera.moveTimer = window.setTimeout(flushCameraMove, wait);
    }
    return;
  }
  flushCameraMove();
}

function flushCameraMove() {
  window.clearTimeout(state.camera.moveTimer);
  state.camera.moveTimer = 0;
  const dx = state.camera.pendingDx;
  const dy = state.camera.pendingDy;
  if (Math.abs(dx) < 1 && Math.abs(dy) < 1) return;
  state.camera.pendingDx = 0;
  state.camera.pendingDy = 0;
  state.camera.lastMove = Date.now();
  bridge?.action?.("camera_adjust", `${cameraFramePayload()};dx=${Math.round(dx)};dy=${Math.round(dy)}`);
}

function shootCamera() {
  $("#cameraFrame").classList.add("capturing");
  setCameraStatus("Capturando");
  const frame = $("#cameraFrame").getBoundingClientRect();
  bridge?.capturePhoto?.(
    Math.round(frame.left),
    Math.round(frame.top),
    Math.round(frame.width),
    Math.round(frame.height),
    Math.round(window.innerWidth || document.documentElement.clientWidth || 1),
    Math.round(window.innerHeight || document.documentElement.clientHeight || 1)
  );
}

function cameraNativeStatus(status, url, message) {
  if (status === "capturing") {
    $("#cameraFrame").classList.add("capturing");
    setCameraStatus("Capturando");
    return;
  }
  if (status === "uploading") {
    setCameraStatus("Salvando foto");
    return;
  }
  $("#cameraFrame").classList.remove("capturing");
  if (status === "captured" && url) {
    const previewUrl = message || url;
    state.camera.url = url;
    state.camera.previewUrl = previewUrl;
    $("#cameraPhoto").src = previewUrl;
    $("#cameraPhoto").hidden = false;
    $("#cameraLive").hidden = true;
    $("#cameraFrame").classList.add("has-photo");
    window.requestAnimationFrame(updateSelfieMask);
    $("#cameraRetake").hidden = false;
    $("#cameraSend").hidden = false;
    $("#cameraShot").hidden = true;
    setCameraStatus("Foto pronta");
    return;
  }
  if (status === "error") {
    setCameraStatus(message || "Falha na camera");
    $("#cameraShot").hidden = false;
  }
}

function sendCameraPhoto() {
  if (state.camera.returnView === "instagramAvatar" && state.camera.url) {
    instagram.edit.avatar = state.camera.url;
    document.body.classList.remove("camera-active");
    closeCamera();
    showView("instagram");
    $("#igEditProfilePanel").hidden = false;
    return renderInstagramEditProfile();
  }
  if ((state.camera.returnView === "instagramPost" || state.camera.returnView === "instagramStory") && state.camera.url) {
    const mode = state.camera.returnView === "instagramStory" ? "story" : "post";
    openInstagramCompose(mode, state.camera.url, state.camera.previewUrl || state.camera.url);
    document.body.classList.remove("camera-active");
    closeCamera();
    showView("instagram");
    return;
  }
  const chat = chats[state.currentChat];
  if (!chat || !state.camera.url) return;
  chat.messages.push({ from: "me", kind: "image", url: state.camera.url, time: nowText() });
  chat.preview = "Foto";
  chat.time = nowText();
  bridge?.action?.("image", `to=${encodePayload(chat.phone)};url=${state.camera.url}`);
  renderMessages();
  renderChats();
  document.body.classList.remove("camera-active");
  closeCamera();
}

const GALLERY_STORAGE_KEY = "metropoles_phone_gallery_v1";
let galleryMemory = [];
function galleryLoad() {
  try {
    const parsed = JSON.parse(localStorage.getItem(GALLERY_STORAGE_KEY) || "[]");
    if (Array.isArray(parsed)) galleryMemory = parsed;
  } catch {}
  return galleryMemory;
}
function gallerySave(items) {
  galleryMemory = items.slice(-80);
  try {
    localStorage.setItem(GALLERY_STORAGE_KEY, JSON.stringify(galleryMemory));
  } catch {
    // Fotos grandes podem exceder a cota do localStorage do WebView. Mantém
    // a sessão visível e tenta persistir apenas as mídias mais recentes.
    for (let keep = Math.min(12, galleryMemory.length); keep >= 1; keep--) {
      try {
        const compact = galleryMemory.slice(-keep);
        localStorage.setItem(GALLERY_STORAGE_KEY, JSON.stringify(compact));
        galleryMemory = compact;
        break;
      } catch {}
    }
  }
  renderGallery();
}
function galleryAdd(url, type, name = "Mídia") {
  if (!url) return;
  const items = galleryLoad();
  items.push({ url, type: String(type || "image").startsWith("video") ? "video" : "image", name, createdAt: Date.now() });
  gallerySave(items);
  showToast("Mídia salva na galeria.");
}
function galleryNativeStatus(url, type) { galleryAdd(url, type, type === "video/mp4" ? "Vídeo da câmera" : "Foto da câmera"); }
function renderGallery() {
  const grid = $("#galleryGrid");
  if (!grid) return;
  const items = galleryLoad().reverse();
  grid.innerHTML = items.length ? items.map((item) => item.type === "video"
    ? `<video class="gallery-item" src="${item.url}" controls preload="metadata"></video>`
    : `<img class="gallery-item" src="${item.url}" alt="${item.name || "Foto"}" loading="lazy" />`).join("")
    : '<p class="gallery-empty">Nenhuma foto ou vídeo salvo.</p>';
}
function handleGalleryFiles(files) {
  [...(files || [])].forEach((file) => {
    const reader = new FileReader();
    reader.onload = () => {
      galleryAdd(reader.result, file.type, file.name);
      if (state.gallerySendToChat && String(file.type || "").startsWith("image/") && chats[state.currentChat]) {
        const chat = chats[state.currentChat];
        chat.messages.push({ from: "me", kind: "image", url: reader.result, time: nowText() });
        chat.preview = "Foto";
        chat.time = nowText();
        bridge?.action?.("image", `to=${encodePayload(chat.phone)};url=${encodePayload(reader.result)}`);
        renderMessages();
        renderChats();
      }
      state.gallerySendToChat = false;
    };
    reader.readAsDataURL(file);
  });
}

function bind() {
  $("#galleryPicker")?.addEventListener("change", (event) => {
    handleGalleryFiles(event.target.files);
    event.target.value = "";
  });
  $("#whatsappCamera")?.addEventListener("click", () => openCamera("chat"));
  $("#cameraVideo")?.addEventListener("click", () => bridge?.captureVideo?.());

  document.addEventListener("error", (event) => {
    const img = event.target?.closest?.(".image-card img");
    if (!img) return;
    renderImagePlaceholder(img.closest(".image-card"), "Imagem indisponivel");
  }, true);

  document.addEventListener("click", (event) => {
    const button = event.target.closest("button");
    if (button) {
      beep();
      button.classList.add("tap");
      window.setTimeout(() => button.classList.remove("tap"), 120);
    }
    if (event.target.closest("#phoneHide")) return closePhone();
    const cameraDirection = event.target.closest("[data-camera-dx]");
    if (cameraDirection && state.view === "camera") {
      return adjustCamera(Number(cameraDirection.dataset.cameraDx || 0), Number(cameraDirection.dataset.cameraDy || 0));
    }

    const open = event.target.closest("[data-open]");
    if (open) {
      if (open.dataset.open === "camera") return openCamera("home");
      if (open.dataset.open === "bank") bridge?.action?.("bank_open", "");
      if (open.dataset.open === "sportfy") bridge?.action?.("sportfy_open", "");
      if (open.dataset.open === "bankPix") {
        bank.amountDigits = "";
        bank.target = null;
        renderBank();
      }
      showView(open.dataset.open);
      return;
    }
    const back = event.target.closest("[data-back]");
    if (back) {
      if (state.view === "camera") return closeCamera();
      showView(back.dataset.back);
      return;
    }
    const tab = event.target.closest("[data-tab]");
    if (tab) {
      setTab(tab.dataset.tab);
      return;
    }
    const chatRow = event.target.closest("[data-chat]");
    if (chatRow) {
      setCurrentChat(chatRow.dataset.chat);
      showView("chat");
      return;
    }
    const contactRow = event.target.closest("[data-contact]");
    if (contactRow) {
      const index = ensureChatForContact(contactRow.dataset.contact);
      if (index >= 0) {
        setCurrentChat(index);
        showView("chat");
      }
      return;
    }
    const callRow = event.target.closest("[data-call]");
    if (callRow) {
      startCall(callRow.dataset.call, callRow.dataset.callName || "");
      return;
    }
    const bankPixRow = event.target.closest("[data-bank-pix-id]");
    if (bankPixRow) {
      setBankTarget({
        id: bankPixRow.dataset.bankPixId,
        name: bankPixRow.dataset.bankPixName,
        bank: bankPixRow.dataset.bankPixBank,
        agency: bankPixRow.dataset.bankPixAgency,
        account: bankPixRow.dataset.bankPixAccount
      });
      return;
    }
    const bankShareRow = event.target.closest("[data-bank-share-phone]");
    if (bankShareRow) {
      shareBankReceipt(bankShareRow.dataset.bankSharePhone, bankShareRow.dataset.bankShareName);
      return;
    }
    const bankKey = event.target.closest("[data-key]");
    if (bankKey) return pushBankKey(bankKey.dataset.key);
    const bankExtractTab = event.target.closest("[data-bank-extract]");
    if (bankExtractTab) {
      bank.extractTab = bankExtractTab.dataset.bankExtract === "out" ? "out" : "in";
      return renderBankExtract();
    }
    const financeInstallment = event.target.closest("[data-bank-installment]");
    if (financeInstallment) return toggleFinanceInstallment(financeInstallment.dataset.bankInstallment);
    const financeRow = event.target.closest("[data-bank-finance]");
    if (financeRow) {
      bank.selectedFinanceId = Number(financeRow.dataset.bankFinance) || 0;
      bank.selectedInvoiceMask = 0;
      return renderBank();
    }
    if (event.target.closest("#bankLookupPix") || event.target.closest("#bankLookupManual")) return lookupBankTarget();
    if (event.target.closest("#bankContinueAmount")) {
      if (!bank.target) return showToast("Escolha quem vai receber.");
      if (bankAmountCents() < 1) return showToast("Digite o valor.");
      if (bankAmountCents() > bank.balance) return showToast("Saldo insuficiente.");
      renderBankReview();
      return showView("bankReview");
    }
    if (event.target.closest("#bankConfirmTransfer")) return confirmBankTransfer();
    if (event.target.closest("#bankPayInvoice")) return payBankInvoice("selected");
    if (event.target.closest("#bankSettleFinancing")) return payBankInvoice("all");
    if (event.target.closest("#radarbotToggle")) {
      bridge?.action?.(radarbot.active ? "radarbot_stop" : "radarbot_start", "");
      if (!bridge) {
        radarbot.active = !radarbot.active;
        renderRadarBot();
      }
      return;
    }
    const igTab = event.target.closest(".instagram-view [data-ig-tab]");
    if (igTab) {
      if (igTab.dataset.igTab === "profile") return goInstagramOwnProfile();
      return setInstagramTab(igTab.dataset.igTab);
    }
    if (event.target.closest("#igActivityButton")) {
      setInstagramTab("activity");
      return showView("instagram");
    }
    if (event.target.closest("#igDirectButton")) return openInstagramDirect();
    if (event.target.closest("#igRefreshActivity")) {
      sendInstagramAction("insta_open", "");
      return showToast("Atualizando Instagram.");
    }
    if (event.target.closest("#igCreatePost")) return openCamera("instagramPost");
    if (event.target.closest("#igCreateStory")) return openCamera("instagramStory");
    if (event.target.closest("[data-ig-create-story]")) {
      const ownStory = getInstagramOwnStartStory();
      if (ownStory) return openInstagramStory(ownStory.id);
      return openCamera("instagramStory");
    }
    if (event.target.closest("#igCancelEditProfile")) return closeInstagramEditProfile();
    if (event.target.closest("#igSaveEditProfile")) return saveInstagramEditProfile();
    if (event.target.closest("#igEditAvatarCamera")) return openCamera("instagramAvatar");
    if (event.target.closest("#igCancelCompose")) return closeInstagramCompose();
    if (event.target.closest("#igPublishCompose")) return publishInstagramCompose();
    const igStory = event.target.closest("[data-ig-story]");
    if (igStory) return openInstagramStory(igStory.dataset.igStory);
    if (event.target.closest("#igCloseStory")) return closeInstagramStory();
    if (event.target.closest("#igStoryMore")) {
      window.clearTimeout(instagram.storyTimer);
      const menu = $("#igStoryMenu");
      menu.hidden = !menu.hidden;
      if (menu.hidden && instagram.story) instagram.storyTimer = window.setTimeout(openNextInstagramStory, INSTAGRAM_STORY_SECONDS * 1000);
      return;
    }
    if (event.target.closest("#igDeleteStory")) return deleteInstagramStory();
    if (event.target.closest("#igStoryLike")) return toggleInstagramStoryLike();
    if (event.target.closest("#igStoryViews")) return openInstagramStoryViewers();
    if (event.target.closest("#igStoryViewer") && handleInstagramStoryTap(event)) return;
    if (event.target.closest("#igCloseComments")) return closeInstagramComments();
    if (event.target.closest("#igClosePostDetail")) return closeInstagramPostDetail();
    if (event.target.closest("#igCloseDirect")) return closeInstagramDirect();
    if (event.target.closest("#igCloseThread")) return closeInstagramThread();
    if (event.target.closest("#igThreadMore")) {
      $("#igThreadMenu").hidden = !$("#igThreadMenu").hidden;
      return;
    }
    if (event.target.closest("#igBlockPeer")) return toggleInstagramPeerBlock();
    const igOpenDm = event.target.closest("[data-ig-open-dm]");
    if (igOpenDm) return openInstagramDirect(igOpenDm.dataset.igOpenDm);
    const igDmPeer = event.target.closest("[data-ig-dm-peer]");
    if (igDmPeer) {
      closeInstagramComments();
      closeInstagramPostDetail();
      return openInstagramDirect(igDmPeer.dataset.igDmPeer);
    }
    const igOpenPost = event.target.closest("[data-ig-open-post]");
    if (igOpenPost) return openInstagramPostDetail(igOpenPost.dataset.igOpenPost);
    const igActivity = event.target.closest("[data-ig-activity]");
    if (igActivity) return openInstagramActivity(igActivity.dataset.igActivity);
    const igViewProfile = event.target.closest("[data-ig-view-profile]");
    if (igViewProfile && !event.target.closest("[data-ig-follow]")) {
      return openInstagramProfile(igViewProfile.dataset.igViewProfile);
    }
    if (event.target.closest("[data-ig-profile-back]")) {
      instagram.viewingProfile = "";
      setInstagramTab("explore");
      return renderInstagramProfile();
    }
    const igLike = event.target.closest("[data-ig-like]");
    if (igLike) {
      const post = instagram.posts.find((item) => String(item.id) === String(igLike.dataset.igLike));
      if (!post) return;
      post.liked = !post.liked;
      post.likes = Math.max(0, Number(post.likes || 0) + (post.liked ? 1 : -1));
      sendInstagramAction("insta_like", `post=${encodePayload(post.id)};on=${post.liked ? 1 : 0}`);
      renderInstagramFeed();
      return renderInstagramPostDetail();
    }
    const igCommentLike = event.target.closest("[data-ig-comment-like]");
    if (igCommentLike) {
      const rows = instagram.comments[instagram.currentPost] || [];
      const comment = rows.find((item) => String(item.id) === String(igCommentLike.dataset.igCommentLike));
      if (!comment) return;
      comment.liked = !comment.liked;
      comment.likes = Math.max(0, Number(comment.likes || 0) + (comment.liked ? 1 : -1));
      sendInstagramAction("insta_comment_like", `comment=${encodePayload(comment.id)};on=${comment.liked ? 1 : 0}`);
      return renderInstagramComments();
    }
    const igFollow = event.target.closest("[data-ig-follow]");
    if (igFollow) {
      const user = instagram.users.find((item) => String(item.id) === String(igFollow.dataset.igFollow));
      if (!user) return;
      const nextFollowing = !user.following;
      user.following = nextFollowing;
      user.followers = Math.max(0, Number(user.followers || 0) + (nextFollowing ? 1 : -1));
      if (String(instagram.viewingProfile) === String(user.id)) renderInstagramProfile();
      else renderInstagramExplore();
      sendInstagramAction("insta_follow", `target=${encodePayload(user.id)};on=${user.following ? 1 : 0}`);
      return;
    }
    const igFocusComment = event.target.closest("[data-ig-focus-comment]");
    if (igFocusComment) return openInstagramComments(igFocusComment.dataset.igFocusComment);
    if (event.target.closest("#igEditBio")) {
      return openInstagramEditProfile();
    }
    const sportfyTab = event.target.closest(".sportfy-tabs [data-sportfy-tab]");
    if (sportfyTab) {
      sportfy.tab = sportfyTab.dataset.sportfyTab || "home";
      if (sportfy.tab === "search") {
        $("#sportfySearchInput")?.focus();
      } else {
        const search = $("#sportfySearchInput");
        if (search?.value) search.value = "";
        sportfy.query = "";
        sportfy.results = [];
      }
      renderSportfy();
      sportfyResetTabScroll(sportfy.tab);
      return;
    }
    const sportfyPlayButton = event.target.closest("[data-sportfy-play], [data-sportfy-video]");
    if (sportfyPlayButton && event.target.closest(".sportfy-view")) {
      if (event.target.closest("[data-sportfy-like]")) {
        const video = event.target.closest("[data-sportfy-like]").dataset.sportfyLike;
        return sportfyToggleFavorite(sportfyTrackById(video));
      }
      if (event.target.closest("[data-sportfy-add]")) {
        const video = event.target.closest("[data-sportfy-add]").dataset.sportfyAdd;
        return sportfyAddToPlaylist(sportfyTrackById(video));
      }
      const video = sportfyPlayButton.dataset.sportfyPlay || sportfyPlayButton.dataset.sportfyVideo;
      const rows = $all(".sportfy-track", $(".sportfy-page.active")).map((row) => sportfyTrackById(row.dataset.sportfyVideo)).filter(Boolean);
      const index = Math.max(0, rows.findIndex((track) => track.id === video));
      return sportfyPlay(sportfyTrackById(video), rows, index);
    }
    const sportfyLike = event.target.closest("[data-sportfy-like]");
    if (sportfyLike) return sportfyToggleFavorite(sportfyTrackById(sportfyLike.dataset.sportfyLike));
    const sportfyAdd = event.target.closest("[data-sportfy-add]");
    if (sportfyAdd) return sportfyAddToPlaylist(sportfyTrackById(sportfyAdd.dataset.sportfyAdd));
    const sportfyPlaylistDelete = event.target.closest("[data-sportfy-playlist-delete]");
    if (sportfyPlaylistDelete) return sportfyDeletePlaylist(sportfyPlaylistDelete.dataset.sportfyPlaylistDelete);
    const sportfyPlaylistBack = event.target.closest("[data-sportfy-playlist-back]");
    if (sportfyPlaylistBack) {
      sportfy.libraryPlaylist = "";
      return renderSportfy();
    }
    const sportfyPlaylistOpen = event.target.closest("[data-sportfy-playlist-open]");
    if (sportfyPlaylistOpen) {
      sportfy.libraryPlaylist = sportfyPlaylistOpen.dataset.sportfyPlaylistOpen || "";
      return renderSportfy();
    }
    if (event.target.closest("#sportfyPlayPause")) {
      if (!sportfy.current) return showToast("Escolha uma musica primeiro.");
      return sportfySetLocalPlaying(!sportfy.playing);
    }
    if (event.target.closest("#sportfyPlayer") && !event.target.closest("#sportfyLikeNow, #sportfyPlayPause")) {
      sportfy.nowOpen = true;
      return renderSportfyNowPlaying();
    }
    if (event.target.closest("#sportfyCloseNow")) {
      sportfy.nowOpen = false;
      return renderSportfyNowPlaying();
    }
    if (event.target.closest("#sportfyNpPlayPause")) {
      if (!sportfy.current) return showToast("Escolha uma musica primeiro.");
      return sportfySetLocalPlaying(!sportfy.playing);
    }
    if (event.target.closest("#sportfyNext")) return sportfyNextTrack();
    if (event.target.closest("#sportfyPrevious")) return sportfyPreviousTrack();
    if (event.target.closest("#sportfyShuffle")) {
      sportfy.shuffle = !sportfy.shuffle;
      return renderSportfyNowPlaying();
    }
    if (event.target.closest("#sportfyRepeat")) {
      sportfy.repeat = !sportfy.repeat;
      return renderSportfyNowPlaying();
    }
    if (event.target.closest("#sportfyLikeNow")) return sportfyToggleFavorite(sportfy.current);
    if (event.target.closest("#sportfyAddNow")) return sportfyAddToPlaylist(sportfy.current);
    if (event.target.closest("#sportfyBtButton") || event.target.closest("#sportfyNpBluetooth")) {
      if (sportfy.bluetooth.connected) bridge?.action?.("sportfy_bt_disc", "");
      else bridge?.action?.("sportfy_bt_conn", "");
      return;
    }
    const sportfyFilter = event.target.closest("[data-sportfy-filter]");
    if (sportfyFilter) {
      sportfy.filter = sportfyFilter.dataset.sportfyFilter || "todos";
      sportfy.results = sportfyFilteredSeeds();
      sportfy.tab = "search";
      return renderSportfy();
    }
    const sportfyQuick = event.target.closest("[data-sportfy-quick]");
    if (sportfyQuick) {
      const action = sportfyQuick.dataset.sportfyQuick;
      if (action === "favorites") sportfy.tab = "library";
      if (action === "recent") {
        const rows = sportfy.history.length ? sportfy.history : sportfyFilteredSeeds();
        sportfyPlay(rows[0], rows, 0);
      }
      if (action === "playlist") sportfy.tab = "library";
      if (action === "bluetooth") sportfy.nowOpen = true;
      return renderSportfy();
    }
    const notifyToggle = event.target.closest("[data-notify-toggle]");
    if (notifyToggle) {
      const app = notifyToggle.dataset.notifyToggle;
      if (app && Object.prototype.hasOwnProperty.call(phoneSettings.notifications, app)) {
        phoneSettings.notifications[app] = !phoneSettings.notifications[app];
        applyPhoneSettings(true);
        showToast(phoneSettings.notifications[app] ? "Notificacao ativada." : "Notificacao desativada.");
      }
      return;
    }
    const wallpaperOption = event.target.closest("[data-wallpaper]");
    if (wallpaperOption) {
      const wallpaper = wallpaperOption.dataset.wallpaper;
      if (wallpaperNames[wallpaper]) {
        phoneSettings.wallpaper = wallpaper;
        applyPhoneSettings(false);
        showToast("Fundo alterado.");
      }
      return;
    }
    const unblockButton = event.target.closest("[data-unblock-phone]");
    if (unblockButton) return unblockContact(unblockButton.dataset.unblockPhone);
    const unblockInstagram = event.target.closest("[data-unblock-instagram]");
    if (unblockInstagram) return unblockInstagramProfile(unblockInstagram.dataset.unblockInstagram);
    if (event.target.closest("#systemHome")) return goHome();
    if (event.target.closest("#systemBack")) return goBack();
    if (event.target.closest("#systemRecent")) return $("#recents").classList.contains("open") ? closeRecents() : openRecents();
    if (event.target.closest("#resumeApp")) {
      if (state.runningApp) showView(state.appView || "whatsapp");
      return closeRecents();
    }
    if (event.target.closest("#closeRecentApp")) {
      state.runningApp = false;
      closeRecents();
      return showView("home");
    }
    if (event.target.closest("#chatMenuButton")) {
      $("#chatMenu").classList.toggle("open");
      $("#attachmentMenu")?.classList.remove("open");
      return;
    }
    if (event.target.closest("#startCallButton")) {
      const button = $("#startCallButton");
      const chat = chats[state.currentChat];
      const phone = chat?.phone || button?.dataset.phone || "";
      const name = chat?.name || button?.dataset.name || "";
      if (phone) startCall(phone, name);
      return;
    }
    if (event.target.closest("#acceptCallButton")) return acceptCall();
    if (event.target.closest("#rejectCallButton")) return rejectCall();
    if (event.target.closest("#callEndButton")) return endCall();
    if (event.target.closest("#callBackButton")) {
      if (state.call.status === "active" || state.call.status === "outgoing" || state.call.status === "connecting") return showView(state.call.returnView || "whatsapp");
      return showView(state.call.returnView || "whatsapp");
    }
    if (event.target.closest("#attachButton")) {
      $("#attachmentMenu").classList.toggle("open");
      return;
    }
    if (event.target.closest("#attachCamera") || event.target.closest("#composerCamera")) {
      $("#attachmentMenu")?.classList.remove("open");
      return openCamera("chat");
    }
    if (event.target.closest("#attachPhoto")) {
      $("#attachmentMenu")?.classList.remove("open");
      state.gallerySendToChat = true;
      return $("#galleryPicker")?.click();
    }
    if (event.target.closest("#cameraBack")) {
      return closeCamera();
    }
    if (event.target.closest("#cameraFlip")) {
      return flipCamera();
    }
    if (event.target.closest("#cameraShot")) {
      return shootCamera();
    }
    if (event.target.closest("#cameraRetake")) {
      return resetCameraPreview();
    }
    if (event.target.closest("#cameraSend")) {
      return sendCameraPhoto();
    }
    const imageCard = event.target.closest("[data-image-url]");
    if (imageCard) {
      loadImageCard(imageCard);
      return;
    }
    if (event.target.closest("#attachLocation")) {
      const chat = chats[state.currentChat];
      $("#attachmentMenu")?.classList.remove("open");
      if (chat) bridge?.action?.("location", `to=${encodePayload(chat.phone)}`);
      return;
    }
    const locationCard = event.target.closest("[data-location-x]");
    if (locationCard) {
      bridge?.action?.("mark_location", `x=${encodePayload(locationCard.dataset.locationX)};y=${encodePayload(locationCard.dataset.locationY)};z=${encodePayload(locationCard.dataset.locationZ)}`);
      return;
    }
    const audioButton = event.target.closest("[data-audio-url]");
    if (audioButton) {
      playAudio(audioButton.dataset.audioUrl, audioButton);
      return;
    }
    if (event.target.closest("#saveCurrentContact")) {
      const chat = chats[state.currentChat];
      if (chat) openContactModal(chat.phone, true);
      return $("#chatMenu").classList.remove("open");
    }
    if (event.target.closest("#blockCurrentContact")) {
      const chat = chats[state.currentChat];
      if (chat && !isBlockedPhone(chat.phone)) {
        blockContact(chat);
        chat.messages.push({ from: "them", text: "Contato bloqueado nesta conversa.", time: nowText() });
      } else if (chat) {
        showToast("Contato ja esta bloqueado.");
      }
      $("#chatMenu").classList.remove("open");
      return renderMessages();
    }
    if (event.target.closest("#newContact")) return openContactModal("", false);
    if (event.target.closest("#newStatus") || event.target.closest("#newStatusText") || event.target.closest("#openStatusFile") || event.target.closest("#openStatusCamera")) {
      $("#statusForm").reset();
      $("#statusText").placeholder = event.target.closest("#openStatusCamera")
        ? "Legenda da foto/status"
        : event.target.closest("#openStatusFile")
          ? "Legenda do arquivo/status"
          : "Digite seu status";
      return openModal("status");
    }
    if (event.target.closest("#newGroup")) {
      $("#groupForm").reset();
      return openModal("group");
    }
    if (event.target.closest("#editProfile")) {
      $("#editBio").value = profile.bio;
      return openModal("profile");
    }
    if (event.target.closest("#lastSeen")) {
      state.lastSeen = (state.lastSeen + 1) % lastSeenOptions.length;
      $("#lastSeenValue").textContent = lastSeenOptions[state.lastSeen];
      return;
    }
    if (event.target.closest("#readReceipts")) return $("#readReceipts").classList.toggle("enabled");
    if (event.target.closest("[data-close-modal]") || event.target.id === "modalLayer") return closeMenus();
    if (!event.target.closest("#chatMenu")) $("#chatMenu")?.classList.remove("open");
    if (!event.target.closest("#attachmentMenu") && !event.target.closest("#attachButton")) $("#attachmentMenu")?.classList.remove("open");
  });

  // A área fora do aparelho faz parte da camada do telefone. Não fecha nem
  // esconde o CEF ao tocar nela; o fechamento fica reservado ao botão Voltar
  // do sistema ou ao botão explícito do próprio telefone.
  const radarMap = $("#radarbotMap");
  radarMap?.addEventListener("pointerdown", (event) => {
    radarbot.map.dragging = true;
    radarbot.map.startX = event.clientX;
    radarbot.map.startY = event.clientY;
    radarbot.map.baseX = radarbot.map.x;
    radarbot.map.baseY = radarbot.map.y;
    radarMap.setPointerCapture?.(event.pointerId);
    event.preventDefault();
  });
  window.addEventListener("pointermove", (event) => {
    if (!radarbot.map.dragging) return;
    const nextX = radarbot.map.baseX + (event.clientX - radarbot.map.startX);
    const nextY = radarbot.map.baseY + (event.clientY - radarbot.map.startY);
    radarbot.map.x = Math.max(-180, Math.min(180, nextX));
    radarbot.map.y = Math.max(-180, Math.min(180, nextY));
    renderRadarBotMap();
  });
  window.addEventListener("pointerup", () => {
    radarbot.map.dragging = false;
  });
  window.addEventListener("message", sportfyHandlePlayerMessage);
  $("#chatSearch").addEventListener("input", (event) => {
    state.query = event.target.value;
    renderChats();
  });
  $all("[data-filter]").forEach((button) => button.addEventListener("click", () => {
    state.filter = button.dataset.filter;
    $all("[data-filter]").forEach((item) => item.classList.toggle("active", item === button));
    renderChats();
  }));
  $("#messageInput").addEventListener("input", () => $("#composer").classList.toggle("has-text", $("#messageInput").value.trim().length > 0));
  $("#contactPhone").addEventListener("input", (event) => { event.target.value = normalizePhone(event.target.value); });
  $("#bankPixKey")?.addEventListener("input", (event) => { event.target.value = String(event.target.value || "").replace(/[^\d]/g, "").slice(0, 10); });
  $("#bankPixKey")?.addEventListener("keydown", (event) => {
    if (event.key === "Enter") {
      event.preventDefault();
      lookupBankTarget();
    }
  });
  $("#sportfySearchInput")?.addEventListener("input", (event) => {
    const value = event.target.value;
    sportfy.query = value;
    sportfy.tab = "search";
    window.clearTimeout(sportfy.searchTimer);
    sportfy.searchTimer = window.setTimeout(() => sportfySearch(value), 260);
    renderSportfy();
  });
  $("#sportfyClearSearch")?.addEventListener("click", () => {
    const search = $("#sportfySearchInput");
    if (search) {
      search.value = "";
      search.focus();
    }
    sportfy.query = "";
    sportfy.results = [];
    sportfy.tab = "search";
    renderSportfy();
  });
  $("#sportfySearchInput")?.addEventListener("keydown", (event) => {
    if (event.key === "Enter") {
      event.preventDefault();
      window.clearTimeout(sportfy.searchTimer);
      sportfy.query = event.target.value || "";
      sportfySearch(event.target.value);
    }
  });
  $("#sportfyPlaylistForm")?.addEventListener("submit", (event) => {
    event.preventDefault();
    const input = $("#sportfyPlaylistName");
    sportfyCreatePlaylist(input?.value || "");
    if (input) input.value = "";
  });
  $("#igSearchInput")?.addEventListener("input", (event) => {
    instagram.query = event.target.value || "";
    renderInstagramExplore();
    window.clearTimeout(instagram.searchTimer);
    instagram.searchTimer = window.setTimeout(() => {
      sendInstagramAction("insta_search", `query=${encodePayload(instagram.query.trim())}`);
    }, 240);
  });
  $(".ig-page[data-ig-page=\"feed\"]")?.addEventListener("scroll", (event) => {
    const page = event.currentTarget;
    if (page.scrollHeight - page.scrollTop - page.clientHeight < 320) loadMoreInstagramFeed();
  }, { passive: true });
  $("#igOnboardingForm")?.addEventListener("submit", (event) => {
    event.preventDefault();
    submitInstagramUsername();
  });
  $("#igDirectForm")?.addEventListener("submit", (event) => {
    event.preventDefault();
    const input = $("#igDirectInput");
    const text = input?.value.trim() || "";
    if (!instagram.currentPeer || !text) return;
    const peer = instagram.conversations.find((item) => String(item.id) === String(instagram.currentPeer)) || instagram.users.find((item) => String(item.id) === String(instagram.currentPeer));
    if (peer?.blocked) return showToast("Desbloqueie esse perfil para enviar mensagens.");
    input.value = "";
    if (bridge === null) {
      if (!instagram.messages[instagram.currentPeer]) instagram.messages[instagram.currentPeer] = [];
      instagram.messages[instagram.currentPeer].push({ id: `preview_dm_${Date.now()}`, fromMe: true, text, time: nowText() });
      const conv = instagram.conversations.find((item) => String(item.id) === String(instagram.currentPeer));
      if (conv) {
        conv.text = text;
        conv.time = nowText();
      }
      return renderInstagramDirect();
    }
    sendInstagramAction("insta_dm_send", `to=${encodePayload(instagram.currentPeer)};text=${encodePayload(text)}`);
  });
  $("#igStoryReplyForm")?.addEventListener("submit", (event) => {
    event.preventDefault();
    const input = $("#igStoryReplyInput");
    const text = input?.value.trim() || "";
    const story = instagram.story;
    if (!story || !text) return;
    input.value = "";
    if (bridge === null) {
      showToast("Resposta enviada para demonstracao.");
      return;
    }
    sendInstagramAction("insta_story_reply", `story=${encodePayload(story.id)};text=${encodePayload(text)}`);
    showToast("Resposta enviada pelo direct.");
  });
  $("#igStoryProgress")?.addEventListener("animationend", (event) => {
    if (!instagram.story || event.target !== $("#igStoryProgress")?.querySelector(".active i")) return;
    openNextInstagramStory();
  });
  $("#igCommentForm")?.addEventListener("submit", (event) => {
    event.preventDefault();
    const input = $("#igCommentInput");
    const text = input?.value.trim() || "";
    const postId = instagram.currentPost;
    if (!postId || !text) return;
    const post = instagram.posts.find((item) => String(item.id) === String(postId));
    if (post) post.comments = Number(post.comments || 0) + 1;
    input.value = "";
    if (bridge === null) {
      if (!instagram.comments[postId]) instagram.comments[postId] = [];
      instagram.comments[postId].push({ id: `preview_comment_${Date.now()}`, account: instagram.profile.id, username: instagram.profile.username, name: instagram.profile.name, avatar: instagram.profile.avatar, text, time: "agora", likes: 0, liked: false });
      renderInstagramFeed();
      renderInstagramPostDetail();
      return renderInstagramComments();
    }
    sendInstagramAction("insta_comment", `post=${encodePayload(postId)};text=${encodePayload(text)}`);
    renderInstagramFeed();
    renderInstagramPostDetail();
  });
  $("#sportfyVolume")?.addEventListener("input", (event) => {
    sportfy.volume = Math.max(0, Math.min(100, Number(event.target.value) || 0));
    if (sportfy.playerReady && sportfy.ytPlayer) sportfy.ytPlayer.setVolume(sportfy.volume);
    sportfySetNativeVolume(sportfy.volume);
    sportfyPostPlayerCommand({ func: "setVolume", args: [sportfy.volume] }, sportfy.localFrame);
    if (sportfy.bluetooth.connected && sportfy.current) {
      sportfyUpdateConnectedVehicleSpatialAudio();
      sportfySendBluetoothStateThrottled();
    }
  });
  $("#sportfyProgress")?.addEventListener("pointerdown", () => { sportfy.seeking = true; });
  $("#sportfyProgress")?.addEventListener("pointerup", sportfySeekToProgress);
  $("#sportfyProgress")?.addEventListener("change", sportfySeekToProgress);
  const sendButton = $("#composer .send");
  sendButton.addEventListener("pointerdown", startVoiceRecording);
  window.addEventListener("pointerup", (event) => {
    if (!state.voice.recording) return;
    if (state.voice.pointerId !== null && event.pointerId !== state.voice.pointerId) return;
    finishVoiceRecording(true);
  });
  window.addEventListener("pointercancel", () => finishVoiceRecording(false));
  const cameraFrame = $("#cameraFrame");
  if (cameraFrame) {
    cameraFrame.addEventListener("pointerdown", (event) => {
      if (state.camera.url) return;
      state.camera.dragging = true;
      state.camera.lastX = event.clientX;
      state.camera.lastY = event.clientY;
      state.camera.lastMove = 0;
      state.camera.pendingDx = 0;
      state.camera.pendingDy = 0;
      cameraFrame.setPointerCapture?.(event.pointerId);
    });
    cameraFrame.addEventListener("pointermove", (event) => {
      if (!state.camera.dragging || state.camera.url) return;
      const dx = event.clientX - state.camera.lastX;
      const dy = event.clientY - state.camera.lastY;
      if (Math.abs(dx) < 2 && Math.abs(dy) < 2) return;
      state.camera.lastX = event.clientX;
      state.camera.lastY = event.clientY;
      adjustCamera(dx, dy);
    });
    ["pointerup", "pointercancel", "lostpointercapture"].forEach((type) => {
      cameraFrame.addEventListener(type, () => {
        flushCameraMove();
        state.camera.dragging = false;
      });
    });
  }
  $("#composer").addEventListener("submit", (event) => {
    event.preventDefault();
    if (state.voice.recording) return;
    const chat = chats[state.currentChat];
    const input = $("#messageInput");
    const text = input.value.trim();
    if (!chat || !text) return;
    chat.messages.push({ from: "me", kind: "text", text, time: nowText() });
    chat.preview = text;
    chat.time = nowText();
    input.value = "";
    $("#composer").classList.remove("has-text");
    bridge?.action?.("message", `to=${encodePayload(chat.phone)};text=${encodePayload(text)}`);
    renderMessages();
    renderChats();
  });
  window.addEventListener("resize", updateSelfieMask);
  $("#contactForm").addEventListener("submit", (event) => {
    event.preventDefault();
    const name = $("#contactName").value.trim();
    const phone = normalizePhone($("#contactPhone").value);
    if (!name || !phone) return;
    saveContactLocal(phone, name);
    bridge?.action?.("contact_save", `phone=${encodePayload(phone)};name=${encodePayload(name)}`);
    closeMenus();
    renderAll();
    if (state.view === "chat") setCurrentChat(state.currentChat);
  });
  $("#profileForm").addEventListener("submit", (event) => {
    event.preventDefault();
    profile.bio = $("#editBio").value.trim() || "Disponivel";
    closeMenus();
    updateProfile();
    renderBank();
    renderContacts();
  });
  $("#statusForm").addEventListener("submit", (event) => {
    event.preventDefault();
    const note = $("#statusText").value.trim() || "Status postado agora";
    statuses.unshift({ name: profile.name, note });
    closeMenus();
    renderStatus();
    setTab("status");
  });
  $("#groupForm").addEventListener("submit", (event) => {
    event.preventDefault();
    const name = $("#groupName").value.trim();
    if (!name) return;
    groups.unshift({ name, note: "Grupo criado pelo jogador" });
    closeMenus();
    renderGroups();
    showView("whatsapp");
    setTab("groups");
  });
}

function updateClock() {
  $("#clock").textContent = new Date().toLocaleTimeString("pt-BR", { hour: "2-digit", minute: "2-digit" });
}

fitPhone();
window.addEventListener("resize", fitPhone);
window.addEventListener("orientationchange", () => window.setTimeout(fitPhone, 80));
updateClock();
setInterval(updateClock, 15000);
sportfy.progressTimer = window.setInterval(sportfyUpdateProgress, 700);
loadPhoneSettings();
sportfyLoadCache();
applyPhoneSettings(false);
bind();
renderAll();
setTab("chats");

window.PhoneApp = { openFromNative, receiveFromNative, closeFromNative, refreshFromNative, voiceNativeStatus, cameraNativeStatus, audioNativeStatus, mediaNativeStatus, galleryNativeStatus };
