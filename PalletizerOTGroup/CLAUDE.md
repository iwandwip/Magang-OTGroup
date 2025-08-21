# Palletizer Configuration Desktop App

## 📋 Project Overview

**Project Name:** Palletizer Parameter Configuration Tool  
**Type:** Desktop Application (Next.js + Electron)  
**Target Platform:** Windows (Primary), Cross-platform ready  
**Development Stack:** Next.js 15 + JavaScript + TailwindCSS 4 + ShadCN + Electron + Serial Communication

## 🏗️ Technical Architecture

### Frontend Stack
- **Next.js 15**: React framework with App Router
- **JavaScript**: Pure JS implementation (no TypeScript)
- **TailwindCSS 4**: Utility-first CSS framework
- **ShadCN UI**: 31 pre-built components (30 official + 1 custom spinner)
- **Lucide React**: Icon library

### Desktop Integration
- **Electron**: Desktop wrapper for cross-platform deployment
- **Serial Communication**: USB connection to Arduino via SerialPort library
- **Static Export**: Next.js static generation for Electron compatibility

### Arduino Integration
- **3 Firmware Components**:
  - PalletizerCentralStateMachine.ino (Central controller with state machine)
  - PalletizerArmControl.ino (ARM controller)
  - PalletizerArmDriver.ino (Motor driver with AccelStepper)

## 📁 Project Structure

```
PalletizerOTGroup/
├── docs/                        # Documentation
│   └── 01_PROJECT_PLAN.md       # Comprehensive project plan
│
├── app/                         # Next.js App Router
│   ├── layout.js               # Root layout with fonts & metadata
│   ├── page.js                 # Home page
│   ├── dashboard/              # Dashboard routes (planned)
│   ├── parameters/             # Parameter configuration routes (planned)
│   ├── monitoring/             # System monitoring routes (planned)
│   └── settings/               # Application settings routes (planned)
│
├── components/                  # React Components
│   └── ui/                     # ShadCN Components (31 total)
│       ├── form.jsx            # Form handling
│       ├── input.jsx           # Text inputs
│       ├── label.jsx           # Form labels
│       ├── textarea.jsx        # Multi-line text
│       ├── select.jsx          # Dropdown selections
│       ├── checkbox.jsx        # Checkboxes
│       ├── switch.jsx          # Toggle switches
│       ├── slider.jsx          # Range sliders
│       ├── card.jsx            # Content containers
│       ├── separator.jsx       # Visual separators
│       ├── tabs.jsx            # Tab navigation
│       ├── accordion.jsx       # Collapsible sections
│       ├── collapsible.jsx     # Expandable content
│       ├── sheet.jsx           # Side panels
│       ├── dialog.jsx          # Modal dialogs
│       ├── table.jsx           # Data tables
│       ├── badge.jsx           # Status indicators
│       ├── progress.jsx        # Progress bars
│       ├── scroll-area.jsx     # Custom scrollbars
│       ├── tooltip.jsx         # Hover information
│       ├── alert.jsx           # Alert messages
│       ├── sonner.jsx          # Toast notifications
│       ├── alert-dialog.jsx    # Confirmation dialogs
│       ├── popover.jsx         # Floating content
│       ├── navigation-menu.jsx # Complex navigation
│       ├── breadcrumb.jsx      # Navigation breadcrumbs
│       ├── pagination.jsx      # Page navigation
│       ├── button.jsx          # Action buttons
│       ├── dropdown-menu.jsx   # Dropdown menus
│       ├── command.jsx         # Command palette
│       ├── skeleton.jsx        # Loading placeholders
│       └── spinner.jsx         # Custom loading spinner
│
├── lib/                        # Utility Functions
│   └── utils.js               # CN utility for class merging
│
├── firmware/                   # Arduino Firmware
│   ├── PalletizerCentralStateMachine/
│   │   └── PalletizerCentralStateMachine.ino
│   ├── PalletizerArmControl/
│   │   └── PalletizerArmControl.ino
│   ├── PalletizerArmDriver/
│   │   └── PalletizerArmDriver.ino
│   ├── SYSTEM_DATA_FLOW.md
│   ├── SYSTEM_FLOWCHART-ID.md
│   └── SYSTEM_FLOWCHART.md
│
├── public/                     # Static Assets
│   ├── next.svg
│   ├── vercel.svg
│   ├── file.svg
│   ├── globe.svg
│   └── window.svg
│
├── node_modules/               # Dependencies
├── .next/                      # Next.js build output
│
├── package.json               # Project dependencies
├── pnpm-lock.yaml            # Lock file
├── next.config.js            # Next.js configuration
├── tailwind.config.js        # TailwindCSS configuration
├── postcss.config.mjs        # PostCSS configuration
├── eslint.config.mjs         # ESLint configuration
├── jsconfig.json             # JavaScript project configuration
├── components.json           # ShadCN configuration (tsx: false)
├── next-env.d.ts             # Next.js type definitions
└── CLAUDE.md                 # This documentation file
```

## 🔧 Parameter System (98 Total Parameters)

### Global Parameters (2)
- `H`: Product height
- `Ly`: Number of layers

### ARM LEFT Parameters (44)
**Home Parameters (6):**
- `x_L`, `y1_L`, `y2_L`, `z_L`, `t_L`, `g_L`

**GLAD Parameters (6):**
- `gp_L`, `dp_L`, `za_L`, `zb_L`, `Z1_L`, `T90_L`

**Position Arrays (32):**
- Origins: `XO1_L` to `XO8_L`, `YO1_L` to `YO8_L`
- Ends: `XE1_L` to `XE8_L`, `YE1_L` to `YE8_L`

### ARM RIGHT Parameters (44)
**Home Parameters (6):**
- `x_R`, `y1_R`, `y2_R`, `z_R`, `t_R`, `g_R`

**GLAD Parameters (6):**
- `gp_R`, `dp_R`, `za_R`, `zb_R`, `Z1_R`, `T90_R`

**Position Arrays (32):**
- Origins: `XO1_R` to `XO8_R`, `YO1_R` to `YO8_R`
- Ends: `XE1_R` to `XE8_R`, `YE1_R` to `YE8_R`

### Y Pattern Array (8)
- `y_pattern[0]` to `y_pattern[7]`

## 🔄 Arduino Communication Protocol

### Serial Commands
```javascript
// Parameter commands
"GET_PARAMS"                    // Get all parameters
"SET_PARAM:name:value"          // Set single parameter
"SAVE_PARAMS"                   // Save to EEPROM
"LOAD_PARAMS"                   // Load from EEPROM
"RESET_PARAMS"                  // Reset to defaults

// System commands
"GET_STATUS"                    // Get system status
"GET_SENSORS"                   // Get sensor readings
"REBOOT"                        // Restart Arduino
```

### ARM Commands
- **HOME**: `H(x,y,z,t,g)` - Move to home position
- **GLAD**: `G(xn,yn,zn,tn,dp,gp,za,zb,xa,ta)` - Pick and place sequence
- **PARK**: `P` - Park position
- **CALIBRATION**: `C` - Calibration sequence

### Response Format
```javascript
{
  "status": "OK|ERROR",
  "command": "original_command",
  "data": {},
  "message": "description",
  "timestamp": "ISO_timestamp"
}
```

## 🎨 UI Components Usage Plan

### Parameter Configuration Forms
- **Tabs**: Switch between ARM LEFT/RIGHT parameters
- **Card**: Group parameters by category (Home, GLAD, Positions)
- **Form + Input**: Individual parameter inputs with validation
- **Accordion**: Collapse/expand parameter groups
- **Slider**: Numeric parameter adjustment
- **Select**: Dropdown for discrete choices

### Serial Communication Interface
- **Select**: COM port selection
- **Badge**: Connection status indicators
- **Button**: Connect/Disconnect actions
- **Alert**: Connection error messages
- **Progress**: Communication progress

### Monitoring Dashboard
- **Table**: Current parameters, command history
- **Badge**: ARM states (IDLE, PICKING, ERROR)
- **Card**: Sensor readings, system status
- **Sonner**: Real-time toast notifications
- **Spinner**: Loading indicators

### Data Management
- **Dialog**: Parameter save/load confirmations
- **Alert-Dialog**: Reset confirmations
- **Sheet**: Quick parameter overview sidebar
- **Tooltip**: Help text for parameters

## 🚀 Development Commands

```bash
# Development
pnpm dev              # Start Next.js development server

# Build
pnpm build            # Build for production
pnpm start            # Start production server

# Linting
pnpm lint             # Run ESLint

# ShadCN Components
npx shadcn@latest add [component-name]  # Add new components
```

## 📋 Current Status

### ✅ Completed
- Project structure setup with Next.js 15
- JavaScript configuration (no TypeScript)
- ShadCN UI components installed (31 total)
- Arduino firmware analysis complete
- Documentation structure established

### 🔄 In Progress
- Component library setup complete
- Ready for UI development

### 📝 Next Steps
1. Create parameter configuration forms
2. Implement serial communication interface
3. Build monitoring dashboard
4. Add Electron integration
5. Implement Arduino communication protocol

## 🔒 Security & Safety Features

- **Parameter Validation**: Prevent invalid values
- **EEPROM Backup**: Automatic parameter backups
- **Confirmation Dialogs**: Critical operations require confirmation
- **Rollback Feature**: Restore previous parameter sets
- **Checksum Validation**: Data integrity in serial communication
- **Timeout Handling**: Prevent system hanging
- **Error Recovery**: Graceful connection failure handling

## 📖 Documentation References

- **Project Plan**: `docs/01_PROJECT_PLAN.md`
- **Arduino Firmware**: `firmware/` directory
- **ShadCN Documentation**: https://ui.shadcn.com/docs
- **Next.js Documentation**: https://nextjs.org/docs