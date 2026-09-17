const API_BASE = "/api/files";
let isBusy = false;
const MAX_UPLOAD_SIZE = 25 * 1024; // harus sama dengan batas di backend

document.getElementById("upload-input").addEventListener("change", async (event) => {
    const file = event.target.files[0];
    event.target.value = ""; // reset input, biar file yang sama bisa dipilih ulang nanti

    if (!file) return;

    if (file.size > MAX_UPLOAD_SIZE) {
        document.getElementById("status-message").textContent = "file terlalu besar (maks 25 KB)";
        return;
    }

    if (isBusy) return;
    setBusy(true, "uploading...");

    try {
        const currentPath = document.getElementById("current-path").textContent;
        const targetPath = currentPath.endsWith("/")
            ? currentPath + file.name
            : currentPath + "/" + file.name;

        const result = await uploadFile(file, targetPath);

        if (result.cancelled) {
            document.getElementById("status-message").textContent = "upload dibatalkan";
            return;
        }

        if (!result.ok) {
            document.getElementById("status-message").textContent = "upload gagal";
            return;
        }

        await loadFiles(currentPath);
    } finally {
        setBusy(false, "ready");
    }
});

function setBusy(state, message) {
    isBusy = state;
    document.getElementById("status-message").textContent = message;
    document.body.classList.toggle("busy", state);
}

function getParentPath(path) {
    if (path === "/") return "/";

    const trimmed = path.endsWith("/") ? path.slice(0, -1) : path;
    const lastSlash = trimmed.lastIndexOf("/");

    return lastSlash <= 0 ? "/" : trimmed.substring(0, lastSlash);
}

function isImagePath(path) {
    return /\.(png|jpe?g|gif|bmp|ico|svg|webp|avif)$/i.test(path);
}

async function loadFiles(path = "/") {
    if (isBusy) return;
    setBusy(true, "loading direktori...");

    try {
        const res = await fetch(`${API_BASE}?path=${encodeURIComponent(path)}`, {
            cache: "no-store"
        });

        if (!res.ok) {
            document.getElementById("status-message").textContent = "gagal load direktori";
            return;
        }

        const files = await res.json();
        document.getElementById("current-path").textContent = path;
        renderFileTable(files, path);
    } finally {
        setBusy(false, "ready");
    }
}

function renderFileTable(files, path) {
    const tbody = document.querySelector("#file-table tbody");
    tbody.innerHTML = "";

    if (path !== "/") {
        const upRow = document.createElement("tr");
        upRow.textContent = "[..]";
        upRow.dataset.isDir = "true";
        upRow.dataset.path = getParentPath(path);
        upRow.dataset.nav = "true";
        tbody.appendChild(upRow);
    }

    files
        .sort((a, b) => {
            if (a.isDir !== b.isDir) return a.isDir ? -1 : 1;
            return a.name.localeCompare(b.name);
        })
        .forEach(file => {
            const row = document.createElement("tr");
            row.textContent = file.isDir ? `[${file.name}]` : file.name;
            row.dataset.path = file.path;
            row.dataset.isDir = file.isDir;
            tbody.appendChild(row);
        });
}

async function loadPreview(path) {
    if (isBusy) return;
    setBusy(true, "loading preview...");

    const preview = document.getElementById("preview-content");
    const url = `${API_BASE}/view?path=${encodeURIComponent(path)}`;

    try {
        if (isImagePath(path)) {
            preview.innerHTML = "";
            const img = document.createElement("img");
            img.src = url;
            preview.appendChild(img);
            return;
        }

        const res = await fetch(url, {
            cache: "no-store"
        });

        if (!res.ok) {
            preview.textContent = "gagal memuat preview";
            return;
        }

        const text = await res.text();
        preview.innerHTML = "";
        const pre = document.createElement("pre");
        pre.textContent = text;
        preview.appendChild(pre);
    } finally {
        setBusy(false, "ready");
    }
}

document.querySelector("#file-table tbody").addEventListener("click", (event) => {
    if (isBusy) return;

    const row = event.target.closest("tr");
    if (!row) return;

    if (row.dataset.nav === "true") {
        loadFiles(row.dataset.path);
        return;
    }

    document.querySelectorAll("#file-table tr.selected")
        .forEach(r => r.classList.remove("selected"));
    row.classList.add("selected");

    const isDir = row.dataset.isDir === "true";
    const path = row.dataset.path;

    if (isDir) {
        document.getElementById("preview-content").innerHTML =
            `<em>Folder: ${path}<br>Double-click baris ini untuk masuk.</em>`;
        return;
    }

    loadPreview(path);
});

document.querySelector("#file-table tbody").addEventListener("dblclick", (event) => {
    if (isBusy) return;

    const row = event.target.closest("tr");
    if (!row) return;

    if (row.dataset.isDir === "true") {
        loadFiles(row.dataset.path);
    }
});

async function uploadFile(file, path) {
    const attempt = await sendUpload(file, path, null);

    if (attempt.status !== 409) {
        return attempt;
    }

    const choice = (prompt("File sudah ada. [O]verwrite / [D]uplicate / [C]ancel") || "c")
        .trim()
        .toLowerCase()
        .charAt(0);

    if (choice === "o") return sendUpload(file, path, "overwrite");
    if (choice === "d") return sendUpload(file, path, "duplicate");
    return { ok: false, cancelled: true };
}

async function sendUpload(file, path, onConflict) {
    let url = `${API_BASE}?path=${encodeURIComponent(path)}`;
    if (onConflict) url += `&onConflict=${onConflict}`;

    const res = await fetch(url, {
        method: "POST",
        body: file
    });

    return { ok: res.ok, status: res.status };
}

document.getElementById("btn-delete").addEventListener("click", async () => {
    if (isBusy) return;

    const selectedRow = document.querySelector("#file-table tr.selected");
    if (!selectedRow) {
        document.getElementById("status-message").textContent = "pilih file dulu";
        return;
    }

    const path = selectedRow.dataset.path;

    if (!confirm(`Hapus "${path}"? Aksi ini tidak bisa dibatalkan.`)) return;
    setBusy(true, "menghapus...");

    try {
        const res = await fetch(`${API_BASE}?path=${encodeURIComponent(path)}`, {
            method: "DELETE"
        });

        if (res.status === 409) {
            document.getElementById("status-message").textContent = "folder tidak kosong, hapus isinya dulu";
            return;
        }

        if (!res.ok) {
            document.getElementById("status-message").textContent = "gagal menghapus";
            return;
        }

        document.getElementById("preview-content").innerHTML = "<em>pilih file untuk melihat isi</em>";

        const currentPath = document.getElementById("current-path").textContent;
        await loadFiles(currentPath);
    } finally {
        setBusy(false, "ready");
    }
});

document.getElementById("btn-mkdir").addEventListener("click", async () => {
    if (isBusy) return;

    const name = prompt("Nama folder baru:");
    if (!name) return; // user cancel atau input kosong

    if (name.includes("/") || name.includes("..")) {
        document.getElementById("status-message").textContent = "nama folder tidak valid";
        return;
    }

    const currentPath = document.getElementById("current-path").textContent;
    const targetPath = currentPath.endsWith("/")
        ? currentPath + name
        : currentPath + "/" + name;

    setBusy(true, "membuat folder...");

    try {
        const res = await fetch(`${API_BASE}/mkdir?path=${encodeURIComponent(targetPath)}`, {
            method: "POST"
        });

        if (res.status === 409) {
            document.getElementById("status-message").textContent = "folder sudah ada";
            return;
        }

        if (!res.ok) {
            document.getElementById("status-message").textContent = "gagal membuat folder";
            return;
        }

        await loadFiles(currentPath);
    } finally {
        setBusy(false, "ready");
    }
});

document.getElementById("btn-rename").addEventListener("click", async () => {
    if (isBusy) return;

    const selectedRow = document.querySelector("#file-table tr.selected");
    if (!selectedRow) {
        document.getElementById("status-message").textContent = "pilih file dulu";
        return;
    }

    const oldPath = selectedRow.dataset.path;
    const oldName = oldPath.substring(oldPath.lastIndexOf("/") + 1);

    const newName = prompt("Nama baru:", oldName);
    if (!newName || newName === oldName) return;

    if (newName.includes("/") || newName.includes("..")) {
        document.getElementById("status-message").textContent = "nama tidak valid";
        return;
    }

    const dir = oldPath.substring(0, oldPath.lastIndexOf("/") + 1);
    const newPath = dir + newName;

    setBusy(true, "rename...");

    try {
        const res = await fetch(
            `${API_BASE}/rename?path=${encodeURIComponent(oldPath)}&newPath=${encodeURIComponent(newPath)}`,
            { method: "PATCH" }
        );

        if (res.status === 409) {
            document.getElementById("status-message").textContent = "nama sudah dipakai";
            return;
        }

        if (!res.ok) {
            document.getElementById("status-message").textContent = "gagal rename";
            return;
        }

        document.getElementById("preview-content").innerHTML = "<em>pilih file untuk melihat isi</em>";

        const currentPath = document.getElementById("current-path").textContent;
        await loadFiles(currentPath);
    } finally {
        setBusy(false, "ready");
    }
});

loadFiles();