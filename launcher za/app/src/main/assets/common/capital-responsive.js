(function () {
  function applyResponsiveViewport() {
    document.documentElement.style.setProperty('--viewport-width', `${window.innerWidth}px`);
    document.documentElement.style.setProperty('--viewport-height', `${window.innerHeight}px`);
  }
  window.addEventListener('resize', applyResponsiveViewport, { passive: true });
  window.addEventListener('orientationchange', applyResponsiveViewport, { passive: true });
  applyResponsiveViewport();
})();
