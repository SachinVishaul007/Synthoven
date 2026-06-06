import React from 'react';

/**
 * Compact prev / "page x of y" / next control. Renders nothing when everything
 * fits on a single page.
 */
export function Pagination({ page, pageSize, total, onPage, accent = 'var(--text-secondary)' }) {
  const pageCount = Math.ceil(total / pageSize);
  if (pageCount <= 1) return null;

  const start = (page - 1) * pageSize + 1;
  const end = Math.min(page * pageSize, total);

  const btn = (enabled) => ({
    width: 26, height: 26, borderRadius: 'var(--r-sm)',
    border: '1px solid var(--border-default)',
    background: 'var(--bg-overlay)',
    color: enabled ? accent : 'var(--text-tertiary)',
    display: 'flex', alignItems: 'center', justifyContent: 'center',
    cursor: enabled ? 'pointer' : 'not-allowed',
    opacity: enabled ? 1 : 0.4,
  });

  return (
    <div style={{
      display: 'flex', alignItems: 'center', justifyContent: 'center', gap: '12px',
      padding: '10px 16px', borderTop: '1px solid var(--border-subtle)',
      background: 'var(--bg-surface)',
    }}>
      <button
        style={btn(page > 1)}
        disabled={page <= 1}
        onClick={() => page > 1 && onPage(page - 1)}
        title="Previous page"
      >
        <svg width="11" height="11" viewBox="0 0 11 11" fill="none" stroke="currentColor" strokeWidth="1.6">
          <path d="M7 2L3.5 5.5 7 9" />
        </svg>
      </button>

      <span style={{ fontSize: '10px', fontFamily: 'var(--font-mono)', color: 'var(--text-tertiary)', letterSpacing: '0.06em' }}>
        {start}–{end} of {total} · page {page}/{pageCount}
      </span>

      <button
        style={btn(page < pageCount)}
        disabled={page >= pageCount}
        onClick={() => page < pageCount && onPage(page + 1)}
        title="Next page"
      >
        <svg width="11" height="11" viewBox="0 0 11 11" fill="none" stroke="currentColor" strokeWidth="1.6">
          <path d="M4 2l3.5 3.5L4 9" />
        </svg>
      </button>
    </div>
  );
}
