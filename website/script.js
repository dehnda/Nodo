// Nodo Website JavaScript
// Smooth scrolling for navigation links
document.querySelectorAll('a[href^="#"]').forEach(anchor => {
    anchor.addEventListener('click', function (e) {
        e.preventDefault();
        const target = document.querySelector(this.getAttribute('href'));
        if (target) {
            target.scrollIntoView({
                behavior: 'smooth',
                block: 'start'
            });
        }
    });
});

// Mobile menu toggle
const mobileMenuToggle = document.querySelector('.mobile-menu-toggle');
const navLinksContainer = document.querySelector('.nav-links');

if (mobileMenuToggle && navLinksContainer) {
    mobileMenuToggle.addEventListener('click', () => {
        navLinksContainer.classList.toggle('active');
    });

    // Close menu when clicking a link
    navLinksContainer.querySelectorAll('a').forEach(link => {
        link.addEventListener('click', () => {
            navLinksContainer.classList.remove('active');
        });
    });

    // Close menu when clicking outside
    document.addEventListener('click', (e) => {
        if (!e.target.closest('.navbar')) {
            navLinksContainer.classList.remove('active');
        }
    });
}

// Navbar scroll effect
let lastScroll = 0;
const navbar = document.querySelector('.navbar');

window.addEventListener('scroll', () => {
    const currentScroll = window.pageYOffset;

    // Add shadow when scrolled
    if (currentScroll > 0) {
        navbar.style.boxShadow = '0 2px 10px var(--shadow-color)';
    } else {
        navbar.style.boxShadow = 'none';
    }

    lastScroll = currentScroll;
});

// Animate elements on scroll (intersection observer)
const observerOptions = {
    threshold: 0.1,
    rootMargin: '0px 0px -50px 0px'
};

const observer = new IntersectionObserver((entries) => {
    entries.forEach(entry => {
        if (entry.isIntersecting) {
            entry.target.style.opacity = '1';
            entry.target.style.transform = 'translateY(0)';
        }
    });
}, observerOptions);

// Observe all cards and major sections
document.querySelectorAll('.feature-card, .workflow-card, .doc-card, .download-card').forEach(el => {
    el.style.opacity = '0';
    el.style.transform = 'translateY(20px)';
    el.style.transition = 'opacity 0.5s ease, transform 0.5s ease';
    observer.observe(el);
});

// Add active state to navigation based on scroll position
const sections = document.querySelectorAll('section[id]');
const navLinkElements = document.querySelectorAll('.nav-links a[href^="#"]');

window.addEventListener('scroll', () => {
    let current = '';

    sections.forEach(section => {
        const sectionTop = section.offsetTop;
        const sectionHeight = section.clientHeight;
        if (pageYOffset >= sectionTop - 200) {
            current = section.getAttribute('id');
        }
    });

    navLinkElements.forEach(link => {
        link.classList.remove('active');
        if (link.getAttribute('href') === `#${current}`) {
            link.classList.add('active');
        }
    });
});

// Download button analytics (placeholder)
document.querySelectorAll('.btn-download').forEach(button => {
    button.addEventListener('click', (e) => {
        const platform = e.target.closest('.download-card').querySelector('h3').textContent;
        console.log(`Download initiated: ${platform}`);
        // Add analytics tracking here (Google Analytics, Plausible, etc.)
    });
});

// Copy workflow code snippets on click
document.querySelectorAll('.workflow-graph code').forEach(code => {
    code.style.cursor = 'pointer';
    code.title = 'Click to copy';

    code.addEventListener('click', async () => {
        const text = code.textContent;
        try {
            await navigator.clipboard.writeText(text);

            // Visual feedback
            const originalText = code.textContent;
            code.textContent = '✓ Copied!';
            setTimeout(() => {
                code.textContent = originalText;
            }, 1500);
        } catch (err) {
            console.error('Failed to copy:', err);
        }
    });
});

// Preload hero image (when you add a real screenshot)
// const heroImg = new Image();
// heroImg.src = 'path/to/screenshot.png';

// Theme Toggle
const themeToggle = document.querySelector('.theme-toggle');
const html = document.documentElement;

// Load saved theme and accent
const savedTheme = localStorage.getItem('theme') || 'dark';
const savedAccent = localStorage.getItem('accent') || 'coral';

html.setAttribute('data-theme', savedTheme);
html.setAttribute('data-accent', savedAccent);

// Update active color option
const updateActiveColor = () => {
    document.querySelectorAll('.color-option').forEach(opt => {
        opt.classList.toggle('active', opt.dataset.color === savedAccent);
    });
};

// Theme toggle functionality
if (themeToggle) {
    // Set initial icon
    updateThemeIcon(savedTheme);

    themeToggle.addEventListener('click', () => {
        const currentTheme = html.getAttribute('data-theme');
        const newTheme = currentTheme === 'dark' ? 'light' : 'dark';

        html.setAttribute('data-theme', newTheme);
        localStorage.setItem('theme', newTheme);
        updateThemeIcon(newTheme);
    });
}

function updateThemeIcon(theme) {
    const icon = themeToggle.querySelector('svg');
    if (theme === 'dark') {
        // Moon icon
        icon.innerHTML = '<path d="M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z" fill="currentColor"/>';
    } else {
        // Sun icon
        icon.innerHTML = '<circle cx="12" cy="12" r="4" fill="currentColor"/><path d="M12 2v2m0 16v2M4.93 4.93l1.41 1.41m11.32 11.32l1.41 1.41M2 12h2m16 0h2M4.93 19.07l1.41-1.41M16.25 7.75l1.41-1.41" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>';
    }
}

// Color Picker
const colorPickerBtn = document.querySelector('.color-picker-btn');
const colorPickerDropdown = document.querySelector('.color-picker-dropdown');
const colorOptions = document.querySelectorAll('.color-option');

if (colorPickerBtn && colorPickerDropdown) {
    colorPickerBtn.addEventListener('click', (e) => {
        e.stopPropagation();
        colorPickerDropdown.classList.toggle('active');
        updateActiveColor();
    });

    // Close dropdown when clicking outside
    document.addEventListener('click', (e) => {
        if (!e.target.closest('.color-picker-dropdown') && !e.target.closest('.color-picker-btn')) {
            colorPickerDropdown.classList.remove('active');
        }
    });

    // Color selection
    colorOptions.forEach(option => {
        option.addEventListener('click', () => {
            const color = option.dataset.color;
            html.setAttribute('data-accent', color);
            localStorage.setItem('accent', color);

            // Update active state
            colorOptions.forEach(opt => opt.classList.remove('active'));
            option.classList.add('active');
        });
    });
}

console.log('🎨 Nodo website loaded successfully');
