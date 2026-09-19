// 1. Template HTML Sidebar
const sidebarTemplate = `
    <!-- Tombol Toggle Mobile -->
    <button id="sidebar-toggle" aria-label="toggle menu">☰</button>

    <!-- Navigasi Sidebar -->
    <nav id="sidebar">
        <a href="/index.html" class="sidebar-item" data-path="/index.html">
            <span class="icon" style="--icon-url: url('/assets/icons/dashboard.svg')"></span>
            <span class="label">Dashboard</span>
        </a>

        <a href="/wifi-setup.html" class="sidebar-item" data-path="/wifi-setup.html">
            <span class="icon" style="--icon-url: url('/assets/icons/wifi.svg')"></span>
            <span class="label">WiFi Setup</span>
        </a>

        <a href="/file-manager.html" class="sidebar-item" data-path="/file-manager.html">
            <span class="icon" style="--icon-url: url('/assets/icons/file-manager.svg')"></span>
            <span class="label">File Manager</span>
        </a>

        <span class="sidebar-item disabled">
            <span class="icon" style="--icon-url: url('/assets/icons/task-manager.svg')"></span>
            <span class="label">Task Manager</span>
        </span>

        <!-- Menu Dropdown Games -->
        <div class="sidebar-dropdown">
            <button class="sidebar-item dropdown-btn" onclick="this.parentElement.classList.toggle('open')">
                <span class="icon" style="--icon-url: url('/assets/icons/game.svg')"></span>
                <span class="label">Games ▼</span>
            </button>
            <div class="dropdown-content">
                <a href="/game.html" class="sidebar-item submenu-item" data-path="/game.html">- Game Hub</a>
                <a href="/assets/game/snek.html" class="sidebar-item submenu-item" data-path="/assets/game/snek.html">- Snek</a>
                <a href="/assets/game/tetris.html" class="sidebar-item submenu-item" data-path="/assets/game/tetris.html">- Tetris</a>
            </div>
        </div>
    </nav>
`;

// 2. Suntikkan HTML ke bagian paling atas dari <body>
document.body.insertAdjacentHTML('afterbegin', sidebarTemplate);

// 3. Logika Penanda Halaman Aktif (Auto-Highlight)
const currentPath = window.location.pathname;
const activeItem = document.querySelector(`.sidebar-item[data-path="${currentPath}"]`);

if (activeItem) {
    activeItem.classList.add('active');

    // Jika halaman yang aktif ada di dalam dropdown game, otomatis buka dropdown-nya
    const parentDropdown = activeItem.closest('.sidebar-dropdown');
    if (parentDropdown) {
        parentDropdown.classList.add('open');
    }
}

// 4. Logika Toggle Sidebar untuk Mobile (Android)
const sidebarToggle = document.getElementById('sidebar-toggle');
const sidebar = document.getElementById('sidebar');

if (sidebarToggle && sidebar) {
    sidebarToggle.addEventListener('click', () => {
        sidebar.classList.toggle('open');
    });
}