async function loadPartials() {
  const deck = document.getElementById('deck');
  if (!deck) return;

  // 加载导航栏
  const topbar = await fetch('partials/topbar.html').then(r => r.text());
  deck.insertAdjacentHTML('beforeend', topbar);

  // 按顺序加载 5 个幻灯片
  const slideFiles = [
    'slide-hero', 'slide-features', 'slide-highlights', 'slide-team', 'slide-final'
  ];
  for (const file of slideFiles) {
    const html = await fetch(`partials/${file}.html`).then(r => r.text());
    deck.insertAdjacentHTML('beforeend', html);
  }

  // 加载翻页控件
  const pager = await fetch('partials/pager.html').then(r => r.text());
  document.body.insertAdjacentHTML('beforeend', pager);

  // 初始化
  initNavigation();
  initSubNav();
  initHeroMap();
  initFullscreen();
}

/* ── 主幻灯片导航 ── */

function initNavigation() {
  const slides = [...document.querySelectorAll('.slide')];
  const counter = document.getElementById('counter');
  let current = 0;

  // 获取导航按钮（排除全屏按钮）
  const navBtns = [...document.querySelectorAll('#nav button')].filter(
    b => !b.classList.contains('fullscreenBtn')
  );

  function showSlide(index) {
    current = Math.max(0, Math.min(slides.length - 1, index));
    slides.forEach((slide, i) => slide.classList.toggle('active', i === current));
    navBtns.forEach((btn, i) => btn.classList.toggle('active', i === current));
    counter.textContent = `${String(current + 1).padStart(2, '0')} / ${String(slides.length).padStart(2, '0')}`;
  }

  navBtns.forEach(btn => btn.addEventListener('click', () => showSlide(Number(btn.dataset.go))));
  document.getElementById('prevBtn').addEventListener('click', () => showSlide(current - 1));
  document.getElementById('nextBtn').addEventListener('click', () => showSlide(current + 1));
  window.addEventListener('keydown', e => {
    if (e.key === 'Escape') return;
    // 数字键 1-5 跳转主幻灯片
    if (/^[1-5]$/.test(e.key)) { showSlide(Number(e.key) - 1); return; }
    // 左右箭头翻页主幻灯片（仅在非亮点子页面时）
    const subSlides = document.getElementById('subSlides');
    const isHighlight = current === 2;
    if (isHighlight && subSlides) {
      const activeSub = subSlides.querySelector('.subSlide.active');
      const subArr = [...subSlides.querySelectorAll('.subSlide')];
      const subIdx = subArr.indexOf(activeSub);
      if (e.key === 'ArrowRight' || e.key === 'PageDown') {
        if (subIdx < subArr.length - 1) { showSubSlide(subIdx + 1); return; }
      }
      if (e.key === 'ArrowLeft' || e.key === 'PageUp') {
        if (subIdx > 0) { showSubSlide(subIdx - 1); return; }
      }
      if (e.key === ' ' || e.key === 'PageDown') { showSubSlide(subIdx + 1); return; }
      if (e.key === 'PageUp') { showSubSlide(subIdx - 1); return; }
    }
    if (e.key === 'ArrowRight' || e.key === 'PageDown' || e.key === ' ') showSlide(current + 1);
    if (e.key === 'ArrowLeft' || e.key === 'PageUp') showSlide(current - 1);
  });
}

/* ── 子页面导航 ── */

function showSubSlide(index) {
  const subSlides = document.querySelectorAll('#subSlides .subSlide');
  const subBtns = document.querySelectorAll('#subNav .subNavBtn');
  const subCounter = document.getElementById('subCounter');
  const max = subSlides.length - 1;
  const i = Math.max(0, Math.min(max, index));
  subSlides.forEach((s, idx) => s.classList.toggle('active', idx === i));
  subBtns.forEach((b, idx) => b.classList.toggle('active', idx === i));
  if (subCounter) subCounter.textContent = `${i + 1} / ${subSlides.length}`;
}

function initSubNav() {
  const subBtns = document.querySelectorAll('#subNav .subNavBtn');
  const subPrev = document.getElementById('subPrev');
  const subNext = document.getElementById('subNext');

  subBtns.forEach(btn => {
    btn.addEventListener('click', () => showSubSlide(Number(btn.dataset.sub)));
  });
  if (subPrev) subPrev.addEventListener('click', () => {
    const subSlides = document.querySelectorAll('#subSlides .subSlide');
    const active = document.querySelector('#subSlides .subSlide.active');
    const idx = [...subSlides].indexOf(active);
    if (idx > 0) showSubSlide(idx - 1);
  });
  if (subNext) subNext.addEventListener('click', () => {
    const subSlides = document.querySelectorAll('#subSlides .subSlide');
    const active = document.querySelector('#subSlides .subSlide.active');
    const idx = [...subSlides].indexOf(active);
    if (idx < subSlides.length - 1) showSubSlide(idx + 1);
  });
}

/* ── 页面加载完成后执行 ── */

document.addEventListener('DOMContentLoaded', loadPartials);

/* ── 全屏 ── */

function initFullscreen() {
  const btn = document.getElementById('fullscreenBtn');
  if (!btn) return;

  const ICON_ENTER = '⛶'; // ⛶ Squared Cross
  const ICON_EXIT  = '✕'; // ✕ Multiplication X

  function updateBtn() {
    const isFs = !!document.fullscreenElement;
    btn.textContent = isFs ? ICON_EXIT : ICON_ENTER;
    btn.title = isFs ? '退出全屏' : '全屏';
  }

  btn.addEventListener('click', () => {
    if (document.fullscreenElement) {
      document.exitFullscreen();
    } else {
      document.documentElement.requestFullscreen();
    }
  });

  document.addEventListener('fullscreenchange', updateBtn);
  document.addEventListener('keydown', e => {
    if (e.key === 'Escape' && document.fullscreenElement) {
      document.exitFullscreen();
    }
  });
}