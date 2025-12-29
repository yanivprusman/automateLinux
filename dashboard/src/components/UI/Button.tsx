import React from 'react';

interface ButtonProps extends React.ButtonHTMLAttributes<HTMLButtonElement> {
    variant?: 'default' | 'primary' | 'secondary';
    size?: 'small' | 'medium' | 'large';
}

const Button: React.FC<ButtonProps> = ({
    children,
    variant = 'default',
    size = 'medium',
    className = '',
    ...props
}) => {
    const variantClass = variant === 'default' ? '' : variant;

    return (
        <button
            className={`glass-button ${variantClass} ${className}`}
            {...props}
        >
            {children}
        </button>
    );
};

export default Button;
