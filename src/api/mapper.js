// Converts backend SampleDto records into the shape the UI components expect.
//
// The frontend was built against mock data with a few fields the backend
// doesn't ship (pack, model, a precomputed waveform). We derive those here so
// no component needs to change.

const WAVEFORM_BARS = 20;

/**
 * Deterministic pseudo-waveform seeded from the sample id/name. The backend
 * doesn't return amplitude data, but the UI just needs plausible-looking bars,
 * and seeding keeps a given sample's shape stable across renders. Shaped like a
 * one-shot: louder at the front, decaying toward the tail.
 */
function pseudoWaveform(seed, bars = WAVEFORM_BARS) {
  let h = 2166136261;
  const str = String(seed ?? 'sample');
  for (let i = 0; i < str.length; i++) {
    h ^= str.charCodeAt(i);
    h = Math.imul(h, 16777619);
  }

  let x = (h >>> 0) || 1;
  const out = [];
  for (let i = 0; i < bars; i++) {
    x = (Math.imul(x, 1103515245) + 12345) & 0x7fffffff;
    const rand = x / 0x7fffffff;
    const envelope = Math.pow(1 - i / bars, 0.8); // front-loaded decay
    out.push(Math.max(0.02, Math.min(1, rand * 0.55 * envelope + envelope * 0.45)));
  }
  return out;
}

/** Use the parent folder name as a stand-in "pack" label for library samples. */
function packFromPath(filePath) {
  if (!filePath) return 'Library';
  const norm = filePath.replace(/\\/g, '/').replace(/\/+$/, '');
  const parts = norm.split('/').filter(Boolean);
  return parts.length >= 2 ? parts[parts.length - 2] : 'Library';
}

export function mapSample(dto) {
  const type = String(dto.type || '').toLowerCase() === 'generated' ? 'generated' : 'library';
  return {
    id: dto.id,
    name: dto.name,
    type,
    pack: packFromPath(dto.filePath),
    model: dto.provider ? `${dto.provider} model` : 'AI',
    prompt: dto.prompt || '',
    tags: Array.isArray(dto.tags) ? dto.tags : Array.from(dto.tags || []),
    bpm: dto.bpm ?? null,
    key: dto.musicalKey ?? null,
    duration: dto.durationMs ? dto.durationMs / 1000 : 0,
    waveform: pseudoWaveform(dto.id ?? dto.name),
    favorited: !!dto.favorite,
    format: dto.format,
    filePath: dto.filePath,
  };
}

export function mapSamples(list) {
  return (list || []).map(mapSample);
}
