export interface Macro {
    message: string;
    trigger: {
        keys: [number, number, boolean][];
    };
    sequence: [number, number][];
    hasHandler: boolean;
}

export type ViewType = 'logs' | 'configs' | 'macros';

export interface KeyTrigger {
    keys: Array<{
        code: number;
        state: number; // 1=press, 0=release
        suppress: boolean;
    }>;
    hasSuppressedKeys: boolean;
}

export interface KeyAction {
    trigger: KeyTrigger;
    sequence: Array<{
        code: number;
        value: number; // 1=press, 0=release
    }>;
    logMessage: string;
    hasHandler: boolean;
    customHandlerName?: string;
}

export type MacrosByApp = Record<string, KeyAction[]>;

export const COMMON_KEYS: { [key: number]: string } = {
    // Keyboard
    1: 'Esc',
    2: '1', 3: '2', 4: '3', 5: '4', 6: '5', 7: '6', 8: '7', 9: '8', 10: '9', 11: '0',
    12: 'Minus', 13: 'Equal', 14: 'Backspace',
    15: 'Tab', 16: 'Q', 17: 'W', 18: 'E', 19: 'R', 20: 'T', 21: 'Y', 22: 'U', 23: 'I', 24: 'O', 25: 'P',
    30: 'A', 31: 'S', 32: 'D', 33: 'F', 34: 'G', 35: 'H', 36: 'J', 37: 'K', 38: 'L',
    44: 'Z', 45: 'X', 46: 'C', 47: 'V', 48: 'B', 49: 'N', 50: 'M',
    28: 'Enter',
    29: 'LeftCtrl',
    42: 'LeftShift',
    56: 'LeftAlt',
    57: 'Space',
    58: 'CapsLock',
    59: 'F1', 60: 'F2', 61: 'F3', 62: 'F4', 63: 'F5', 64: 'F6', 65: 'F7', 66: 'F8', 67: 'F9', 68: 'F10',
    102: 'Home', 103: 'Up', 104: 'PageUp', 105: 'Left', 106: 'Right', 107: 'End', 108: 'Down', 109: 'PageDown',
    110: 'Insert', 111: 'Delete',
    125: 'LeftMeta',

    // Mouse
    272: 'BTN_LEFT',
    273: 'BTN_RIGHT',
    274: 'BTN_MIDDLE',
    275: 'BTN_SIDE',
    276: 'BTN_EXTRA',
    277: 'BTN_FORWARD',
    278: 'BTN_BACK',

    // KP
    96: 'KP_Enter',

    // Virtual G-Keys
    1001: 'G1',
    1002: 'G2',
    1003: 'G3',
    1004: 'G4',
    1005: 'G5',
    1006: 'G6'
};
