function initHeroMap() {
  const mapPattern = [
    '', '', '', '', '', '', '', '', '', '', '', '', '',
    '', '', '', '', '', '', '', '', '', '', '', '', '',
    '', 'base', '', '', 'brick', 'brick', '', '', 'steel', 'steel', '', '', 'brick',
    '', '', '', 'water', '', '', 'brick', '', '', '', '', '', '',
    '', 'brick', 'brick', 'brick', '', '', '', 'grass', 'grass', 'grass', '', '', '',
    '', '', '', '', '', '', 'brick', '', '', '', '', '', '',
    '', '', 'steel', 'steel', '', '', '', '', '', 'steel', 'steel', '', '',
    '', '', '', '', '', '', 'tank', '', '', '', '', '', '',
    '', '', 'steel', 'steel', '', '', '', '', '', 'steel', 'steel', '', '',
    '', '', '', '', '', '', 'brick', '', '', '', '', '', '',
    '', '', '', 'grass', 'grass', 'grass', '', '', '', 'brick', 'brick', 'brick', '',
    '', '', '', '', '', '', 'brick', '', '', 'water', '', '', '',
    'brick', '', '', 'steel', 'steel', '', '', 'brick', 'brick', '', '', 'water', ''
  ];
  const heroMap = document.getElementById('heroMap');
  if (!heroMap) return;

  mapPattern.forEach(cls => {
    const cell = document.createElement('div');
    cell.className = `tile ${cls}`;
    heroMap.appendChild(cell);
  });
}