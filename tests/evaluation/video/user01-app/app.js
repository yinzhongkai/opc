"use strict";

const $ = (selector) => document.querySelector(selector);
const $$ = (selector) => [...document.querySelectorAll(selector)];
const now = () => new Date().toISOString();
const escapeHtml = (value) => String(value ?? "").replace(/[&<>"']/g, character => ({
  "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;"
}[character]));
const DEFAULT_SESSION = "/out/evaluation/T-029/user01/reference-session-v1.json";
const requiredKinds = ["shot", "motion_peak", "action_peak", "negative_span"];
const scoreDimensions = [
  "temporalAlignment", "salienceAccentMatch", "densityAndExtraBeats",
  "continuity", "editReadiness", "overallNaturalness"
];

let definition = null;
let submission = null;
let selectedIndex = 0;
let negativeStart = null;
let activePlayback = null;
let lastCorrectionActivityAt = null;

function storageKey() {
  return `space-rhythm:user01:${definition.sessionDefinitionSha256}`;
}

function newSubmission() {
  return {
    schemaVersion: 1,
    phase: definition.phase,
    sessionId: definition.sessionId,
    sessionDefinitionSha256: definition.sessionDefinitionSha256,
    acceptanceScope: "personal-single-user-acceptance",
    reviewerAnonymousId: "USER-01",
    startedAt: now(),
    updatedAt: now(),
    completedAt: null,
    clips: (definition.clips || definition.trials || []).map(item => ({
      clipId: item.clipId,
      reviewStatus: "pending",
      reviewedKinds: [],
      events: [],
      ratings: null,
      playbackCounts: { A: 0, B: 0 },
      pairPreference: null,
      correctionEvents: structuredClone(item.classicEvents || []),
      correctionLog: [],
      activeEditMs: 0
    })),
    modificationLog: []
  };
}

function save() {
  submission.updatedAt = now();
  localStorage.setItem(storageKey(), JSON.stringify(submission));
  $("#save-state").textContent = `已自动保存 · ${new Date().toLocaleTimeString()}`;
  renderProgress();
}

function loadSaved() {
  const saved = localStorage.getItem(storageKey());
  submission = saved ? JSON.parse(saved) : newSubmission();
}

function currentDefinition() {
  return (definition.clips || definition.trials)[selectedIndex];
}

function currentRecord() {
  return submission.clips.find(item => item.clipId === currentDefinition().clipId);
}

function nearestFrame(seconds, clip = currentDefinition()) {
  const sourceTimes = clip.frameTimesNs || [];
  const displayedTimes = clip.previewFrameTimesNs?.length === sourceTimes.length ? clip.previewFrameTimesNs : sourceTimes;
  if (!sourceTimes.length) return { ordinal: null, timeNs: Math.round(seconds * 1e9) };
  const target = Math.round(seconds * 1e9);
  let low = 0, high = displayedTimes.length - 1;
  while (low < high) {
    const mid = Math.floor((low + high) / 2);
    if (displayedTimes[mid] < target) low = mid + 1;
    else high = mid;
  }
  const right = low;
  const left = Math.max(0, right - 1);
  const ordinal = Math.abs(displayedTimes[left] - target) <= Math.abs(displayedTimes[right] - target) ? left : right;
  return { ordinal, timeNs: sourceTimes[ordinal], previewTimeNs: displayedTimes[ordinal] };
}

function logOperation(operation, clipId, annotationId, before, after) {
  submission.modificationLog.push({
    operation, clipId, annotationId, occurredAt: now(),
    before: before === null ? null : structuredClone(before),
    after: after === null ? null : structuredClone(after)
  });
}

function setPhaseUI() {
  const phaseMap = {
    reference_authoring: ["phase-reference", "人工参考制作"],
    blind_rating: ["phase-blind", "随机化盲评"],
    manual_correction: ["phase-correction", "人工修正"]
  };
  $$(".phase").forEach(node => node.classList.add("locked"));
  const [id, label] = phaseMap[definition.phase] || phaseMap.reference_authoring;
  $(`#${id}`).classList.add("active");
  $(`#${id}`).classList.remove("locked");
  $("#phase-label").textContent = label;
  $("#reference-panel").hidden = definition.phase !== "reference_authoring";
  $("#event-panel").hidden = definition.phase !== "reference_authoring";
  $("#blind-panel").hidden = definition.phase !== "blind_rating";
  $("#correction-panel").hidden = definition.phase !== "manual_correction";
}

function renderClipList() {
  const items = definition.clips || definition.trials || [];
  $("#clip-list").innerHTML = items.map((clip, index) => {
    const record = submission.clips.find(item => item.clipId === clip.clipId);
    const classes = ["clip-button", index === selectedIndex ? "active" : "", record.reviewStatus === "complete" ? "done" : ""].join(" ");
    return `<button class="${classes}" data-clip-index="${index}"><span>${clip.clipId}</span></button>`;
  }).join("");
  $$('[data-clip-index]').forEach(button => button.addEventListener("click", () => {
    selectedIndex = Number(button.dataset.clipIndex);
    negativeStart = null;
    render();
  }));
}

function renderProgress() {
  if (!definition || !submission) return;
  const complete = submission.clips.filter(item => item.reviewStatus === "complete").length;
  $("#progress").textContent = `${complete} / ${submission.clips.length}`;
  renderClipList();
}

function setVideoSource(video, url, muted) {
  const absolute = new URL(url, window.location.origin).href;
  if (video.src !== absolute) video.src = absolute;
  video.muted = muted;
}

function renderReference() {
  const clip = currentDefinition();
  const record = currentRecord();
  const video = $("#reference-video");
  setVideoSource(video, clip.previewUrl, true);
  requiredKinds.forEach(kind => {
    const checkbox = $(`[data-review-kind="${kind}"]`);
    checkbox.checked = record.reviewedKinds.includes(kind);
  });
  $("#complete-clip").textContent = record.reviewStatus === "complete" ? "已完成（点击重新打开）" : "完成本片段自查";
  renderEvents();
  updateFrameReadout();
}

function options(values, selected) {
  return values.map(value => `<option value="${value}" ${value === selected ? "selected" : ""}>${value}</option>`).join("");
}

function classesForKind(kind) {
  if (kind === "shot") return ["hard_cut", "gradual_transition", "ambiguous"];
  if (kind === "motion_peak") return ["global", "local", "mixed", "ambiguous"];
  if (kind === "action_peak") return ["impact", "stop", "reversal", "ambiguous"];
  return ["all_candidate_kinds", "shot", "motion_peak", "action_peak"];
}

function renderEvents() {
  const record = currentRecord();
  $("#event-count").textContent = `${record.events.length} 项`;
  $("#event-list").innerHTML = record.events.map((event, index) => `
    <div class="event-row" data-event-index="${index}">
      <code>${event.kind}<br>${(event.timeNs / 1e9).toFixed(3)} s</code>
      <select data-field="boundaryOrMotionOrActionClass">${options(classesForKind(event.kind), event.boundaryOrMotionOrActionClass)}</select>
      <select data-field="rhythmRole">${options(["primary_accent", "secondary_accent", "do_not_accent", "uncertain"], event.rhythmRole)}</select>
      <label>时长 ms<input data-field="durationMs" type="number" min="0" value="${event.durationNs / 1e6}"></label>
      <label>前窗 ms<input data-field="beforeMs" type="number" min="0" value="${event.matchWindowBeforeNs / 1e6}"></label>
      <label>后窗 ms<input data-field="afterMs" type="number" min="0" value="${event.matchWindowAfterNs / 1e6}"></label>
      <input data-field="uncertaintyReasonToken" placeholder="uncertain 时必填原因 token" value="${escapeHtml(event.uncertaintyReasonToken)}">
      <button data-delete-event="${index}">删除</button>
    </div>`).join("") || `<p class="muted">尚未标记事件。若确认没有某类事件，也需在上方勾选“已检查”。</p>`;
  $$(".event-row").forEach(row => row.addEventListener("change", event => updateEvent(Number(row.dataset.eventIndex), event.target)));
  $$('[data-delete-event]').forEach(button => button.addEventListener("click", () => deleteEvent(Number(button.dataset.deleteEvent))));
}

function updateEvent(index, target) {
  const record = currentRecord();
  const before = structuredClone(record.events[index]);
  const field = target.dataset.field;
  if (field === "durationMs") record.events[index].durationNs = Math.round(Number(target.value) * 1e6);
  else if (field === "beforeMs") record.events[index].matchWindowBeforeNs = Math.round(Number(target.value) * 1e6);
  else if (field === "afterMs") record.events[index].matchWindowAfterNs = Math.round(Number(target.value) * 1e6);
  else record.events[index][field] = target.value;
  record.events[index].selfCheckDecision = "modify";
  logOperation("modify", record.clipId, record.events[index].annotationId, before, record.events[index]);
  record.reviewStatus = "pending";
  save();
}

function addEvent(kind, startFrame = null, endFrame = null) {
  const record = currentRecord();
  const video = $("#reference-video");
  const frame = startFrame || nearestFrame(video.currentTime);
  const index = record.events.length + submission.modificationLog.length + 1;
  const durationNs = endFrame ? endFrame.timeNs - frame.timeNs : 0;
  const event = {
    annotationId: `${definition.sessionId}:${record.clipId}:${String(index).padStart(5, "0")}`,
    clipId: record.clipId,
    kind,
    timeNs: frame.timeNs,
    durationNs,
    matchWindowBeforeNs: 0,
    matchWindowAfterNs: 0,
    annotationVersion: "initial",
    annotatorAnonymousId: "USER-01",
    boundaryOrMotionOrActionClass: classesForKind(kind)[0],
    rhythmRole: kind === "negative_span" ? "do_not_accent" : "uncertain",
    uncertaintyReasonToken: kind === "negative_span" ? "none" : "pending_user_judgment",
    selfCheckDecision: "accept",
    referenceStatus: "single_user_reference",
    referenceAuthorAnonymousId: "USER-01",
    referenceAuthoringSessionId: definition.sessionId,
    reviewerAnonymousId: "not_applicable(reason=single_user_scope)",
    adjudicatorAnonymousId: "not_applicable(reason=single_user_scope)",
    acceptanceScope: "personal-single-user-acceptance",
    sourceFrameTimeNs: frame.timeNs,
    sourceDecodeOrdinal: frame.ordinal,
    annotationProtocolVersion: "0.2.0"
  };
  record.events.push(event);
  record.reviewStatus = "pending";
  logOperation("add", record.clipId, event.annotationId, null, event);
  save();
  renderEvents();
}

function deleteEvent(index) {
  const record = currentRecord();
  const [removed] = record.events.splice(index, 1);
  logOperation("delete", record.clipId, removed.annotationId, removed, null);
  record.reviewStatus = "pending";
  save();
  renderEvents();
}

function updateFrameReadout() {
  if (!definition || definition.phase !== "reference_authoring") return;
  const video = $("#reference-video");
  const frame = nearestFrame(video.currentTime || 0);
  $("#player-time").textContent = `${(video.currentTime || 0).toFixed(3)} s`;
  $("#frame-time").textContent = `${frame.timeNs} ns`;
  $("#frame-ordinal").textContent = frame.ordinal ?? "unavailable";
}

function renderBlind() {
  const trial = currentDefinition();
  const record = currentRecord();
  setVideoSource($("#blind-a"), trial.variants.A.mediaUrl, false);
  setVideoSource($("#blind-b"), trial.variants.B.mediaUrl, false);
  $("#plays-a").textContent = `${record.playbackCounts.A} / 3`;
  $("#plays-b").textContent = `${record.playbackCounts.B} / 3`;
  const ratings = record.ratings || {};
  $("#rating-form").innerHTML = `<div class="review-box">
    ${scoreDimensions.map(name => `<label>${name}<select data-score="${name}"><option value="">请选择</option>${options(["1", "2", "3", "4", "5", "N/A"], String(ratings[name] || ""))}</select></label>`).join("")}
    <label>直接导出<select id="direct-export"><option value="">请选择</option>${options(["direct_export", "minor_edit", "major_edit", "unusable"], ratings.directExportReadiness || "")}</select></label>
    <label>成对偏好<select id="pair-preference"><option value="">请选择</option>${options(["A", "B", "tie"], record.pairPreference || "")}</select></label>
    <input id="na-reason" placeholder="出现 N/A 时填写稳定原因 token" value="${escapeHtml(ratings.naReasonToken)}">
    <button id="complete-rating" class="primary">提交本片段盲评</button>
  </div>`;
  $("#complete-rating").addEventListener("click", completeBlindRating);
}

function playBlind(label) {
  if (activePlayback) return;
  const record = currentRecord();
  if (record.playbackCounts[label] >= 3) return;
  const video = $(`#blind-${label.toLowerCase()}`);
  activePlayback = label;
  video.currentTime = 0;
  video.onended = () => {
    record.playbackCounts[label] += 1;
    activePlayback = null;
    save();
    renderBlind();
  };
  video.play();
}

function completeBlindRating() {
  const record = currentRecord();
  if (record.playbackCounts.A < 1 || record.playbackCounts.B < 1) return alert("A/B 必须各完整播放至少一次。 ");
  const ratings = {};
  for (const select of $$('[data-score]')) {
    if (!select.value) return alert("六项评分均需填写。 ");
    ratings[select.dataset.score] = select.value === "N/A" ? "N/A" : Number(select.value);
  }
  ratings.directExportReadiness = $("#direct-export").value;
  ratings.naReasonToken = $("#na-reason").value.trim();
  if (!ratings.directExportReadiness || !$("#pair-preference").value) return alert("直接导出意愿和成对偏好均需填写。 ");
  if (Object.values(ratings).includes("N/A") && !ratings.naReasonToken) return alert("N/A 必须填写原因 token。 ");
  record.ratings = ratings;
  record.pairPreference = $("#pair-preference").value;
  record.reviewStatus = "complete";
  save();
  render();
}

function renderCorrection() {
  const clip = currentDefinition();
  const record = currentRecord();
  setVideoSource($("#correction-video"), clip.mediaUrl, false);
  $("#correction-readiness").value = record.correctionReadiness || "";
  $("#correction-events").innerHTML = record.correctionEvents.map((event, index) => `
    <div class="event-row"><code>${event.kind}<br>${(event.timeNs / 1e9).toFixed(3)} s</code>
      <select data-correction-kind="${index}">${options(["shot", "motion_peak", "action_peak"], event.kind)}</select>
      <button data-move="${index}">移动到当前帧</button><button data-lock="${index}">${event.locked ? "已锁定" : "锁定"}</button>
      <button data-correction-delete="${index}">删除</button></div>`).join("") || `<p class="muted">当前没有 classic 事件。</p>`;
  $$('[data-correction-kind]').forEach(select => select.addEventListener("change", () => correctionOperation("reclassify", Number(select.dataset.correctionKind), select.value)));
  $$('[data-move]').forEach(button => button.addEventListener("click", () => correctionOperation("move", Number(button.dataset.move))));
  $$('[data-lock]').forEach(button => button.addEventListener("click", () => correctionOperation("lock", Number(button.dataset.lock))));
  $$('[data-correction-delete]').forEach(button => button.addEventListener("click", () => correctionOperation("delete", Number(button.dataset.correctionDelete))));
}

function correctionOperation(operation, index, value = null) {
  const record = currentRecord();
  const activityAt = Date.now();
  if (lastCorrectionActivityAt !== null) record.activeEditMs += Math.min(30000, activityAt - lastCorrectionActivityAt);
  lastCorrectionActivityAt = activityAt;
  if (operation === "add") {
    const frame = nearestFrame($("#correction-video").currentTime);
    const created = {
      eventId: `${definition.sessionId}:${record.clipId}:manual:${record.correctionLog.length + 1}`,
      kind: value,
      timeNs: frame.timeNs,
      sourceDecodeOrdinal: frame.ordinal,
      source: "USER-01"
    };
    record.correctionEvents.push(created);
    record.correctionLog.push({ operation, occurredAt: now(), before: null, after: structuredClone(created) });
    record.reviewStatus = "pending";
    save();
    renderCorrection();
    return;
  }
  const before = structuredClone(record.correctionEvents[index]);
  if (operation === "delete") record.correctionEvents.splice(index, 1);
  else if (operation === "move") {
    const frame = nearestFrame($("#correction-video").currentTime);
    record.correctionEvents[index].timeNs = frame.timeNs;
    record.correctionEvents[index].sourceDecodeOrdinal = frame.ordinal;
  }
  else if (operation === "lock") record.correctionEvents[index].locked = true;
  else if (operation === "reclassify") record.correctionEvents[index].kind = value;
  record.correctionLog.push({ operation, occurredAt: now(), before, after: operation === "delete" ? null : structuredClone(record.correctionEvents[index]) });
  record.reviewStatus = "pending";
  save();
  renderCorrection();
}

function completeCorrection() {
  const record = currentRecord();
  const readiness = $("#correction-readiness").value;
  if (!readiness) return alert("请先选择修正后的可用状态。 ");
  record.correctionReadiness = readiness;
  record.reviewStatus = "complete";
  record.completedAt = now();
  save();
  render();
}

function completeReferenceClip() {
  const record = currentRecord();
  if (record.reviewStatus === "complete") {
    record.reviewStatus = "pending";
    save();
    render();
    return;
  }
  const reviewed = $$('[data-review-kind]').filter(node => node.checked).map(node => node.dataset.reviewKind);
  if (reviewed.length !== requiredKinds.length) return alert("请先勾选四项“已检查”。 ");
  const unresolved = record.events.some(event => event.rhythmRole === "uncertain" && (!event.uncertaintyReasonToken || event.uncertaintyReasonToken === "pending_user_judgment"));
  if (unresolved) return alert("uncertain 事件必须填写实际原因 token，不能保留 pending_user_judgment。 ");
  record.reviewedKinds = reviewed;
  record.reviewStatus = "complete";
  save();
  render();
}

function render() {
  renderClipList();
  const clip = currentDefinition();
  $("#clip-title").textContent = clip.clipId;
  $("#clip-meta").textContent = [clip.datasetPurpose, clip.datasetPartition, clip.productCategory].filter(Boolean).join(" · ");
  if (definition.phase === "reference_authoring") renderReference();
  if (definition.phase === "blind_rating") renderBlind();
  if (definition.phase === "manual_correction") renderCorrection();
  renderProgress();
}

function validateComplete() {
  if (submission.clips.some(item => item.reviewStatus !== "complete")) {
    throw new Error("仍有片段未完成，不能导出正式记录。 ");
  }
  if (definition.phase === "reference_authoring") {
    for (const clip of submission.clips) {
      if (new Set(clip.reviewedKinds).size !== requiredKinds.length) throw new Error(`${clip.clipId} 未完成四类检查。`);
    }
  }
}

function exportJson(final) {
  if (final) validateComplete();
  const payload = structuredClone(submission);
  if (final) payload.completedAt = now();
  const blob = new Blob([JSON.stringify(payload, null, 2) + "\n"], { type: "application/json" });
  const link = document.createElement("a");
  link.href = URL.createObjectURL(blob);
  link.download = `${definition.sessionId}-${final ? "submission" : "draft"}.json`;
  link.click();
  URL.revokeObjectURL(link.href);
}

async function loadSession() {
  try {
    $("#load-error").textContent = "";
    const response = await fetch($("#session-url").value, { cache: "no-store" });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    definition = await response.json();
    if (!definition.sessionDefinitionSha256 || !definition.phase) throw new Error("会话定义缺少 hash 或 phase");
    loadSaved();
    selectedIndex = 0;
    $("#workflow").hidden = false;
    setPhaseUI();
    render();
    $("#save-state").textContent = "会话已载入";
  } catch (error) {
    $("#load-error").textContent = `载入失败：${error.message}`;
  }
}

$("#session-url").value = new URLSearchParams(location.search).get("session") || DEFAULT_SESSION;
$("#load-session").addEventListener("click", loadSession);
$("#reference-video").addEventListener("timeupdate", updateFrameReadout);
$("#reference-video").addEventListener("seeked", updateFrameReadout);
$$('[data-kind]').forEach(button => button.addEventListener("click", () => addEvent(button.dataset.kind)));
$$('[data-review-kind]').forEach(checkbox => checkbox.addEventListener("change", () => {
  const record = currentRecord();
  record.reviewedKinds = $$('[data-review-kind]').filter(node => node.checked).map(node => node.dataset.reviewKind);
  record.reviewStatus = "pending";
  save();
}));
$("#negative-start").addEventListener("click", () => {
  negativeStart = nearestFrame($("#reference-video").currentTime);
  $("#negative-end").disabled = false;
  $("#negative-status").textContent = `负例起点：${negativeStart.timeNs} ns；请移动到终点。`;
});
$("#negative-end").addEventListener("click", () => {
  const end = nearestFrame($("#reference-video").currentTime);
  if (end.timeNs <= negativeStart.timeNs) return alert("负例终点必须晚于起点。 ");
  addEvent("negative_span", negativeStart, end);
  negativeStart = null;
  $("#negative-end").disabled = true;
  $("#negative-status").textContent = "";
});
$("#complete-clip").addEventListener("click", completeReferenceClip);
$$('[data-play]').forEach(button => button.addEventListener("click", () => playBlind(button.dataset.play)));
$$('[data-correction-add]').forEach(button => button.addEventListener("click", () => correctionOperation("add", -1, button.dataset.correctionAdd)));
$("#complete-correction").addEventListener("click", completeCorrection);
$("#export-draft").addEventListener("click", () => exportJson(false));
$("#export-final").addEventListener("click", () => {
  try { exportJson(true); } catch (error) { alert(error.message); }
});
loadSession();
