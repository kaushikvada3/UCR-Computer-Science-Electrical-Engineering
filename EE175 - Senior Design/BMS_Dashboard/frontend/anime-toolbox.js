// Select DOM elements
const container = document.querySelector('.home-section-container');
const stickySection = document.querySelector('.fixed-section');
const textSection = document.querySelector('.home-section-text');
const leftLabels = document.querySelectorAll('.toolbox-labels-left .label-item');
const rightLabels = document.querySelectorAll('.toolbox-labels-right .label-item');

// Create the main timeline
// autoplay: false is crucial for scroll-controlled animations
const tl = anime.timeline({
    autoplay: false,
    duration: 1000, // Normalized duration 0-1000 for easier percentage calcs
    easing: 'linear'
});

// 1. Text fades in and moves up
tl.add({
    targets: textSection,
    opacity: [0, 1],
    translateY: [20, 0],
    duration: 100, // First 10% of scroll
    easing: 'easeOutQuad'
}, 0);

// 2. Animate Left Labels (Flying in from left/bottom)
// We'll stagger them slightly
tl.add({
    targets: leftLabels,
    translateX: [-100, 0], // Move from left
    translateY: [100, 0], // Move from bottom
    opacity: [0, 1],
    delay: anime.stagger(50, { start: 100 }), // Start after text
    duration: 400,
    easing: 'easeOutExpo'
}, 0);

// 3. Animate Right Labels (Flying in from right/bottom)
tl.add({
    targets: rightLabels,
    translateX: [100, 0],
    translateY: [100, 0],
    opacity: [0, 1],
    delay: anime.stagger(50, { start: 150 }), // Start slightly after left
    duration: 400,
    easing: 'easeOutExpo'
}, 0);

// 4. "Exit" animation - things fade out or move away as you finish scrolling
// This runs from say 600ms to 900ms
tl.add({
    targets: [textSection, leftLabels, rightLabels],
    opacity: 0,
    scale: 0.9,
    duration: 200,
    easing: 'easeInQuad',
    delay: 700 // Starts at 70% of scroll
}, 0);


// Scroll Listener Logic
function onScroll() {
    const containerRect = container.getBoundingClientRect();
    const windowHeight = window.innerHeight;

    // We want to know how far through the container we have scrolled.
    // When the top of the container hits the top of the viewport -> 0%
    // When the bottom of the container hits the bottom of the viewport -> 100%

    // Actually, sticky behavior is a bit distinctive. 
    // The container is TALL (400vh maybe).
    // The content stays stuck for `containerHeight - viewportHeight`.

    const containerHeight = containerRect.height;
    // Calculate scroll progress (0 to 1)
    // -containerRect.top is how many pixels we've scrolled past the top
    let scrollY = -containerRect.top;

    // The total scrollable distance of the container
    const maxScroll = containerHeight - windowHeight;

    if (maxScroll <= 0) return;

    let progress = scrollY / maxScroll;

    // Clamp between 0 and 1
    progress = Math.max(0, Math.min(1, progress));

    // Seek the timeline
    tl.seek(tl.duration * progress);
}

// Initial positioning styles for labels to make them "fly" nicely
// We set them via JS so if JS fails they might just sit there (or use CSS)
// Ideally CSS handles initial state, but for dynamic 'stagger' randomness we can do it here.
// For this demo, we let anime.js handle the 'from' states defined in the timeline.

// Set specific initial positions for visual variety if needed
anime.set(leftLabels, {
    left: '10%',
    top: (i) => 50 + (i * 50) + 'px' // Stack them vertically
});

anime.set(rightLabels, {
    right: '10%',
    top: (i) => 50 + (i * 50) + 'px' // Stack them vertically
});

window.addEventListener('scroll', onScroll);
window.addEventListener('resize', onScroll);

// Initial trigger
onScroll();
