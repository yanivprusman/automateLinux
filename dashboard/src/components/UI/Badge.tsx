import React from 'react';

interface BadgeProps {
    children: React.ReactNode;
    variant?: 'default' | 'primary' | 'success' | 'danger' | 'warning';
    className?: string;
    style?: React.CSSProperties;
}

const Badge: React.FC<BadgeProps> = ({ children, variant = 'default', className = '', style }) => {
    const getVariantClass = () => {
        switch (variant) {
            case 'primary': return 'primary';
            case 'success': return 'success';
            case 'danger': return 'danger';
            case 'warning': return 'warning';
            default: return '';
        }
    };

    return (
        <span
            className={`detail-badge ${getVariantClass()} ${className}`}
            style={style}
        >
            {children}
        </span>
    );
};

export default Badge;
