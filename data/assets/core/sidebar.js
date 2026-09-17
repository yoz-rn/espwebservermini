document.getElementById("sidebar-toggle").addEventListener("click", () => {
    document.getElementById("sidebar").classList.toggle("open");
});

const currentPath = window.location.pathname;
document.querySelectorAll("#sidebar a").forEach(link => {
    if (link.getAttribute("href") === currentPath) {
        link.classList.add("active");
    }
});