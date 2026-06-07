import React, { useState } from 'react';

export function FolderTree({ folders, selectedPath, onSelectFolder }) {
  const [expanded, setExpanded] = useState({});

  if (!folders || folders.length === 0) {
    return (
      <div style={{
        padding: '20px',
        fontSize: '11px',
        fontFamily: 'var(--font-mono)',
        color: 'var(--text-tertiary)',
        textAlign: 'center',
      }}>
        No folders indexed
      </div>
    );
  }

  // Build tree from flat folders list
  const map = {};
  folders.forEach(f => {
    map[f.path] = { ...f, children: [] };
  });

  const roots = [];
  folders.forEach(f => {
    const item = map[f.path];
    if (f.parentPath && map[f.parentPath]) {
      map[f.parentPath].children.push(item);
    } else {
      roots.push(item);
    }
  });

  // Helper to toggle node expansion
  const toggleExpand = (path, e) => {
    e.stopPropagation();
    setExpanded(prev => ({ ...prev, [path]: !prev[path] }));
  };

  // Recursive render helper
  const renderNode = (node, depth = 0) => {
    const hasChildren = node.children && node.children.length > 0;
    const isExpanded = !!expanded[node.path];
    const isSelected = selectedPath === node.path;

    return (
      <div key={node.path} style={{ display: 'flex', flexDirection: 'column' }}>
        <div
          onClick={() => onSelectFolder(node.path, node.name)}
          style={{
            display: 'flex',
            alignItems: 'center',
            padding: '6px 8px',
            paddingLeft: `${depth * 12 + 8}px`,
            borderRadius: 'var(--r-md)',
            background: isSelected ? 'var(--accent-amber-glow)' : 'transparent',
            border: isSelected ? '1px solid var(--accent-amber-border)' : '1px solid transparent',
            color: isSelected ? 'var(--accent-amber)' : 'var(--text-secondary)',
            fontSize: '12px',
            cursor: 'pointer',
            userSelect: 'none',
            transition: 'all var(--t-base)',
            gap: '6px',
            position: 'relative',
          }}
          onMouseEnter={e => {
            if (!isSelected) {
              e.currentTarget.style.background = 'rgba(255,255,255,0.02)';
              e.currentTarget.style.color = 'var(--text-primary)';
            }
          }}
          onMouseLeave={e => {
            if (!isSelected) {
              e.currentTarget.style.background = 'transparent';
              e.currentTarget.style.color = 'var(--text-secondary)';
            }
          }}
        >
          {/* Collapse/Expand chevron */}
          <div
            onClick={(e) => toggleExpand(node.path, e)}
            style={{
              width: '16px',
              height: '16px',
              display: 'flex',
              alignItems: 'center',
              justifyContent: 'center',
              opacity: hasChildren ? 0.8 : 0,
              pointerEvents: hasChildren ? 'auto' : 'none',
              transform: isExpanded ? 'rotate(90deg)' : 'rotate(0deg)',
              transition: 'transform var(--t-base)',
              color: isSelected ? 'var(--accent-amber)' : 'var(--text-tertiary)',
            }}
          >
            <svg width="8" height="8" viewBox="0 0 8 8" fill="none" stroke="currentColor" strokeWidth="1.5">
              <path d="M2.5 1.5L6 4L2.5 6.5" strokeLinecap="round" strokeLinejoin="round"/>
            </svg>
          </div>

          {/* Folder Icon */}
          <span style={{ opacity: isSelected ? 1 : 0.7, display: 'flex', alignItems: 'center', color: isSelected ? 'var(--accent-amber)' : 'var(--text-secondary)' }}>
            <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
              <path d="M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z" />
            </svg>
          </span>

          {/* Folder Name */}
          <span style={{
            overflow: 'hidden',
            textOverflow: 'ellipsis',
            whiteSpace: 'nowrap',
            fontWeight: isSelected ? 500 : 400,
          }}>
            {node.name}
          </span>
        </div>

        {/* Render children if expanded */}
        {hasChildren && isExpanded && (
          <div style={{ display: 'flex', flexDirection: 'column', marginTop: '2px' }}>
            {node.children.map(child => renderNode(child, depth + 1))}
          </div>
        )}
      </div>
    );
  };

  return (
    <div style={{
      display: 'flex',
      flexDirection: 'column',
      gap: '2px',
      overflowY: 'auto',
      flex: 1,
      padding: '4px',
    }}>
      {roots.map(root => renderNode(root, 0))}
    </div>
  );
}
