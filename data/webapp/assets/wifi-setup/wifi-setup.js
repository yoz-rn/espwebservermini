const form = document.getElementById('wifi-form');
const statusEl = document.getElementById('status');
const submitBtn = form.querySelector('button');

let pollTimer = null;

function stopPolling() {
    if (pollTimer) {
        clearInterval(pollTimer);
        pollTimer = null;
    }
    submitBtn.disabled = false;
}

function pollStatus() {
    fetch('/api/wifi-status')
        .then(response => response.text())
        .then(text => {
            let data;
            try {
                data = JSON.parse(text);
            }
            catch (err) {
                console.error('Raw response gagal di-parse:', text);
                statusEl.textContent = 'Error parsing (cek console): ' + text.substring(0, 100);
                statusEl.style.color = 'var(--text-error, red)'; 
                stopPolling();
                return;
            }

            if (data.status === 'SUCCESS') {
                statusEl.textContent = data.message;
                statusEl.style.color = 'var(--text-success, #00ff00)';
                stopPolling();
                fetchSavedNetwork();
            } else if (data.status === 'FAILED') {
                statusEl.textContent = data.message;
                statusEl.style.color = 'var(--text-error, red)';
                stopPolling();
            } else if (data.status === 'TESTING') {
                statusEl.textContent = data.message;
                statusEl.style.color = 'var(--text-main)';
            }
        })
        .catch(error => {
            statusEl.textContent = 'Error saat cek status: ' + error.message;
            statusEl.style.color = 'var(--text-error, red)';
            stopPolling();
        });
}

const savedNetworkEl = document.getElementById('saved-network');

function renderSavedNetwork(data) {
    const ssid = data.savedSSID;
    if (!ssid) {
        savedNetworkEl.innerHTML = '<div class="network-empty">No Network Saved</div>';
        return;
    }

    const connected = data.staConnected;
    const statusClass = connected ? 'wifi-connected' : 'wifi-ap-only';
    const statusText = connected ? `Connected (${data.staIP})` : 'Saved, Not connected';
    const primaryBtn = connected
        ? '<button id="btn-primary">Disconnect</button>'
        : '<button id="btn-primary">Connect</button>';

    savedNetworkEl.innerHTML = `
        <div class="network-entry">
            <div>
                <div class="ssid"></div>
                <div class="network-status ${statusClass}">
                    <span class="status-dot"></span>
                    <span>${statusText}</span>
                </div>
            </div>
            <div class="network-actions">
                ${primaryBtn}
                <button id="btn-forget" class="btn-forget">Forget</button>
            </div>
        </div>
    `;
    savedNetworkEl.querySelector('.ssid').textContent = ssid;

    document.getElementById('btn-primary').addEventListener('click', connected ? handleDisconnect : handleReconnect);
    document.getElementById('btn-forget').addEventListener('click', handleForget);
}

function handleReconnect(e) {
    const btn = e.target;
    btn.disabled = true;
    btn.textContent = 'Menghubungkan...';

    fetch('/api/wifi-reconnect', { method: 'POST' }).then(() => {
        let tries = 0;
        const poll = setInterval(() => {
            tries++;
            fetch('/api/wifi-status')
                .then(res => res.json())
                .then(data => {
                    if (data.staConnected || tries >= 8) {   // ~12 detik timeout
                        clearInterval(poll);
                        renderSavedNetwork(data);
                    }
                });
        }, 1500);
    });
}

function fetchSavedNetwork() {
    fetch('/api/wifi-status')
        .then(res => res.json())
        .then(renderSavedNetwork)
        .catch(() => {
            savedNetworkEl.innerHTML = '<div class="network-empty">Gagal memuat status</div>';
        });
}

function handleDisconnect() {
    fetch('/api/wifi-disconnect', { method: 'POST' })
        .then(() => setTimeout(fetchSavedNetwork, 600));
}

function handleForget() {
    if (!confirm('Hapus kredensial WiFi ini? Kamu perlu setup ulang untuk connect lagi.')) return;
    fetch('/api/wifi-forget', { method: 'POST' })
        .then(() => setTimeout(fetchSavedNetwork, 600));
}

fetchSavedNetwork();

form.addEventListener('submit', (e) => {
    e.preventDefault();

    const ssid = document.getElementById('ssid').value;
    const password = document.getElementById('password').value;

    submitBtn.disabled = true;
    statusEl.textContent = 'Mengirim...';
    statusEl.style.color = 'var(--text-main)';

    const body = new URLSearchParams();
    body.append('ssid', ssid);
    body.append('password', password);

    fetch('/api/wifi-config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: body
    })
        .then(response => response.json())
        .then(data => {
            if (!data.success) {
                statusEl.textContent = data.message;
                statusEl.style.color = 'var(--text-error, red)';
                submitBtn.disabled = false;
                return;
            }
            statusEl.textContent = data.message;
            pollTimer = setInterval(pollStatus, 1500);
        })
        .catch(error => {
            statusEl.textContent = 'Error: ' + error.message;
            statusEl.style.color = 'var(--text-error, red)';
            submitBtn.disabled = false;
        });
});