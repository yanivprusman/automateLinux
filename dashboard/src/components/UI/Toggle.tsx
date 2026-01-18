import React from 'react';

interface ToggleProps {
    checked: boolean;
    onChange: (checked: boolean) => void;
    label?: string;
    size?: 'small' | 'medium';
}

const Toggle: React.FC<ToggleProps> = ({ checked, onChange, label, size = 'medium' }) => {
    const toggleSize = size === 'small' ? {
        width: '36px',
        height: '18px',
        thumbSize: '14px',
        thumbTranslate: '18px'
    } : {
        width: '44px',
        height: '22px',
        thumbSize: '18px',
        thumbTranslate: '22px'
    };

    return (
        <div style={{ display: 'flex', alignItems: 'center', gap: '10px', cursor: 'pointer' }} onClick={() => onChange(!checked)}>
            {label && <span style={{ fontSize: size === 'small' ? '0.75rem' : '0.85rem', color: 'var(--text-secondary)', fontWeight: '600', textTransform: 'uppercase' }}>{label}</span>}
            <div style={{
                width: toggleSize.width,
                height: toggleSize.height,
                background: checked ? 'var(--accent-cyan-dim)' : 'rgba(255, 255, 255, 0.05)',
                border: `1px solid ${checked ? 'var(--accent-cyan)' : 'var(--glass-border)'}`,
                borderRadius: '100px',
                position: 'relative',
                transition: 'var(--transition-fast)',
                display: 'flex',
                alignItems: 'center',
                padding: '1px'
            }}>
                <div style={{
                    width: toggleSize.thumbSize,
                    height: toggleSize.thumbSize,
                    background: checked ? 'var(--accent-cyan)' : 'var(--text-dim)',
                    borderRadius: '50%',
                    position: 'absolute',
                    left: '2px',
                    transform: checked ? `translateX(${toggleSize.thumbTranslate})` : 'translateX(0)',
                    transition: 'var(--transition-fast)',
                    boxShadow: checked ? '0 0 10px var(--accent-cyan)' : 'none'
                }} />
            </div>
        </div>
    );
};

export default Toggle;
