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