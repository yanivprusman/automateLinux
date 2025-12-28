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
}

export type MacrosByApp = Record<string, KeyAction[]>;

export const COMMON_KEYS: { [key: number]: string } = {
    28: 'ENTER',
    96: 'KP_ENTER',
    29: 'LEFT_CTRL',
    42: 'LEFT_SHIFT',
    56: 'LEFT_ALT',
    1: 'ESC',
    47: 'V',
    105: 'LEFT',
    106: 'RIGHT',
    103: 'UP',
    108: 'DOWN',
    272: 'BTN_LEFT',
    273: 'BTN_RIGHT',
    277: 'BTN_FORWARD',
    278: 'BTN_BACK',
    1001: 'G1',
    1002: 'G2',
    1003: 'G3',
    1004: 'G4',
    1005: 'G5',
    1006: 'G6'
};
