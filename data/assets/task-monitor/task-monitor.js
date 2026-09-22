const POLL_MS = 1000;
let prev = null;   // snapshot sebelumnya: { total, rt: Map(n -> rt) }

const $ = (id) => document.getElementById(id);

const IDLE_NAMES = new Set(["IDLE0", "IDLE1"]);

function levelClass(pct) {          // warna bar menurut beban
    if (pct >= 80) return "crit";
    if (pct >= 50) return "warn";
    return "ok";
}

function stackClass(free) {         // sisa stack dalam byte
    if (free < 256) return "crit";
    if (free < 512) return "warn";
    return "";
}

function fmtBytes(n) {
    return n >= 1024 ? (n / 1024).toFixed(1) + " KiB" : n + " B";
}

function showHeap(h) {
    if (!h) return;                    // firmware lama tanpa field heap
    const used = h.size - h.free;      // dipakai sekarang
    const peak = h.size - h.min;       // titik pemakaian tertinggi sejak boot
    const pct = (v) => (100 * v) / h.size;

    setBar($("bar-heap-used"), pct(used));
    setBar($("bar-heap-peak"), pct(peak));
    $("txt-heap-used").textContent = fmtBytes(used) + " " + pct(used).toFixed(1) + "%";
    $("txt-heap-peak").textContent = fmtBytes(peak) + " " + pct(peak).toFixed(1) + "%";
    $("heap-size").textContent = fmtBytes(h.size);
    $("heap-free").textContent = fmtBytes(h.free);
    $("heap-blk").textContent = fmtBytes(h.blk);
}

function setBar(fillEl, pct, forced) {
    const v = pct == null ? 0 : Math.min(100, Math.max(0, pct));
    fillEl.style.width = v + "%";
    fillEl.className = "bar-fill " + (forced || levelClass(v));
}

function barCell(pct, dim) {
    const td = document.createElement("td");
    td.className = "num";
    const wrap = document.createElement("div");
    wrap.className = "cell-bar";
    const bar = document.createElement("div");
    bar.className = "bar bar-sm";
    const fill = document.createElement("div");
    bar.append(fill);
    setBar(fill, pct, dim ? "dim" : null);   // task idle: bar diredupkan, bukan merah
    const txt = document.createElement("span");
    txt.textContent = pct == null ? "--" : pct.toFixed(1) + "%";
    wrap.append(bar, txt);
    td.append(wrap);
    return td;
}

let pollMs = 1000;
let timer = null;

async function poll() {
    try {
        const res = await fetch("/api/tasks", { cache: "no-store" });
        if (!res.ok) throw new Error("HTTP " + res.status);
        render(await res.json());
        $("status").textContent = "live";
    } catch (err) {
        $("status").textContent = "error: " + err.message;
    }
    schedule();
}

function schedule() {
    clearTimeout(timer);
    if (document.hidden) return;          // tab di latar belakang: jangan polling
    timer = setTimeout(poll, pollMs);
}

document.addEventListener("visibilitychange", () => {
    if (document.hidden) {
        clearTimeout(timer);
        $("status").textContent = "paused";
    } else {
        prev = null;                       // selisih sepanjang jeda tidak bermakna
        poll();
    }
});

$("interval").addEventListener("change", (e) => {
    pollMs = Number(e.target.value);
    schedule();
});

poll();

function nameCell(r) {
    const td = document.createElement("td");
    td.textContent = r.name;
    if (r.user && r.tag) {
        const badge = document.createElement("span");
        badge.className = "tag-badge";
        badge.textContent = r.tag;
        td.append(" ", badge);
    }
    return td;
}

function render(data) {
    
    const cur = { total: data.total, up: data.up, rt: new Map(data.tasks.map((t) => [t.n, t.rt])) };
    if (prev && data.up < prev.up) prev = null;   // uptime mundur = reboot
    const dTotal = prev ? (cur.total - prev.total) >>> 0 : 0;

    const rows = data.tasks.map((t) => {
        let pct = null;
        if (dTotal > 0 && prev.rt.has(t.n)) {
            pct = (100 * ((t.rt - prev.rt.get(t.n)) >>> 0)) / dTotal;
        }
        return { ...t, pct };
    });

    const idlePct = (name) => rows.find((r) => r.name === name)?.pct;
    showCore(0, idlePct("IDLE0"));
    showCore(1, idlePct("IDLE1"));

    showHeap(data.heap);

    // Task nyata dulu (CPU terbesar di atas), task idle paling bawah
    rows.sort((a, b) => {
        const ia = IDLE_NAMES.has(a.name), ib = IDLE_NAMES.has(b.name);
        if (ia !== ib) return ia ? 1 : -1;
        return (b.pct ?? -1) - (a.pct ?? -1);
    });

    $("task-count").textContent = rows.length;
    $("task-rows").replaceChildren(...rows.map((r) => {
        const idle = IDLE_NAMES.has(r.name);
        const tr = document.createElement("tr");
        if (idle) tr.className = "idle";
        if (r.user) tr.classList.add("user-task");
        tr.append(
            nameCell(r),
            cell(r.core === -1 ? "any" : (r.core ?? "-"), "num"),
            cell(r.prio, "num"),
            cell(r.state, "st-" + r.state.toLowerCase()),
            cell(r.free, "num " + stackClass(r.free)),
            barCell(r.pct, idle)
        );
        return tr;
    }));

    prev = cur;
}

function showCore(n, idlePct) {

    const load = idlePct == null ? null : Math.max(0, 100 - idlePct);
    setBar($("bar-core" + n), load);
    $("pct-core" + n).textContent = load == null ? "--" : load.toFixed(1) + "%";
}

function cell(text, cls) {
    const td = document.createElement("td");
    td.textContent = text;
    if (cls) td.className = cls;
    return td;
}