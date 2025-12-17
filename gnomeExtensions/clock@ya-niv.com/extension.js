// 'use strict';

// import St from 'gi://St';
// import GLib from 'gi://GLib';
// import Clutter from 'gi://Clutter';
// import * as Main from 'resource:///org/gnome/shell/ui/main.js';
// import { Extension } from 'resource:///org/gnome/shell/extensions/extension.js';

// export default class ClockExtension extends Extension {
//     constructor(metadata) {
//         super(metadata);
//         this._label = null;
//         this._timeoutId = 0;
//         this._lastX = null;
//         this._lastY = null;
//         console.log('ClockExtension constructor called');
//     }

//     enable() {
//         console.log('ClockExtension.enable() called');
//         try {
//             this._label = new St.Label({
//                 text: '00:00',
//                 style_class: 'clock-label',
//                 reactive: true,
//                 track_hover: true
//             });
//             console.log('Label created');
            
//             Main.uiGroup.add_child(this._label);
//             console.log('Label added to stage');
            
//             // Load saved position or use default
//             let {x, y} = this._loadPosition();
//             if (x === null || y === null) {
//                 x = 50;
//                 y = 50;
//             }
//             this._label.set_position(x, y);
//             this._lastX = x;
//             this._lastY = y;
//             console.log(`Label positioned at ${x},${y}`);
            
//             this._label.show();
//             console.log('Label shown');
            
//             this._updateClock();
            
//             this._timeoutId = GLib.timeout_add_seconds(
//                 GLib.PRIORITY_DEFAULT,
//                 1,
//                 () => {
//                     this._updateClock();
                    
//                     // Periodically save position
//                     const x = this._label.get_x();
//                     const y = this._label.get_y();
//                     if (x !== this._lastX || y !== this._lastY) {
//                         this._savePosition(x, y);
//                         this._lastX = x;
//                         this._lastY = y;
//                     }
                    
//                     return GLib.SOURCE_CONTINUE;
//                 }
//             );
//             console.log('Clock update timer started');
            
//             // Setup dragging
//             this._setupDragging();
            
//         } catch (e) {
//             console.error('Error in enable():', e);
//             console.error('Stack:', e.stack);
//         }
//     }

//     _setupDragging() {
//         let dragData = { dragging: false, offsetX: 0, offsetY: 0 };

//         this._label.connect('button-press-event', (_, event) => {
//             if (event.get_button() === 1) {
//                 console.log('Button pressed on label');
//                 dragData.dragging = true;
//                 const [stageX, stageY] = event.get_coords();
//                 const actorX = this._label.get_x();
//                 const actorY = this._label.get_y();
//                 dragData.offsetX = stageX - actorX;
//                 dragData.offsetY = stageY - actorY;
//                 console.log(`Drag start: stage(${stageX},${stageY}), offset(${dragData.offsetX},${dragData.offsetY})`);
//                 return Clutter.EVENT_STOP;
//             }
//             return Clutter.EVENT_PROPAGATE;
//         });

//         this._label.connect('button-release-event', (_, event) => {
//             if (event.get_button() === 1) {
//                 console.log('Button released');
//                 dragData.dragging = false;
//                 const x = this._label.get_x();
//                 const y = this._label.get_y();
//                 console.log(`Drag end: position(${x},${y})`);
//                 this._savePosition(x, y);
//                 return Clutter.EVENT_STOP;
//             }
//             return Clutter.EVENT_PROPAGATE;
//         });

//         this._label.connect('motion-event', (_, event) => {
//             if (dragData.dragging) {
//                 const [stageX, stageY] = event.get_coords();
//                 const newX = stageX - dragData.offsetX;
//                 const newY = stageY - dragData.offsetY;
//                 this._label.set_position(newX, newY);
//                 console.log(`Dragging: new position(${newX},${newY})`);
//                 return Clutter.EVENT_STOP;
//             }
//             return Clutter.EVENT_PROPAGATE;
//         });
//     }

//     _loadPosition() {
//         try {
//             const [success, stdout] = GLib.spawn_command_line_sync(`bash /home/yaniv/coding/automateLinux/gnomeExtensions/clock@ya-niv.com/loadClockPosition.sh`);
//             if (success && stdout) {
//                 const output = new TextDecoder().decode(stdout).trim();
//                 const lines = output.split('\n');
//                 let x = null, y = null;
                
//                 if (lines[0]) {
//                     const xVal = parseInt(lines[0]);
//                     if (!isNaN(xVal)) {
//                         x = xVal;
//                     }
//                 }
//                 if (lines[1]) {
//                     const yVal = parseInt(lines[1]);
//                     if (!isNaN(yVal)) {
//                         y = yVal;
//                     }
//                 }
                
//                 console.log(`Loaded position: X=${x}, Y=${y}`);
//                 return {x, y};
//             }
//         } catch (e) {
//             console.log('Failed to load position:', e);
//         }
//         return {x: null, y: null};
//     }

//     _savePosition(x, y) {
//         console.log(`Saving position: X=${Math.round(x)}, Y=${Math.round(y)}`);
        
//         try {
//             const cmd = `bash /home/yaniv/coding/automateLinux/gnomeExtensions/clock@ya-niv.com/saveClockPosition.sh ${Math.round(x)} ${Math.round(y)}`;
//             console.log(`Running: ${cmd}`);
//             GLib.spawn_command_line_async(cmd);
//         } catch (e) {
//             console.warn('Failed to save position:', e);
//         }
//     }

//     disable() {
//         console.log('ClockExtension.disable() called');
//         if (this._timeoutId) {
//             GLib.source_remove(this._timeoutId);
//             this._timeoutId = 0;
//         }

//         if (this._label) {
//             this._label.destroy();
//             this._label = null;
//         }
//     }

//     _updateClock() {
//         if (!this._label) {
//             return;
//         }
        
//         const t = new Date();
//         this._label.text = t.toLocaleTimeString([], {
//             hour: '2-digit',
//             minute: '2-digit',
//             hour12: false
//         });
//     }
// }

'use strict';

import { Extension } from 'resource:///org/gnome/shell/extensions/extension.js';

export default class ClockExtension extends Extension {
    enable() {}
    disable() {}
}
