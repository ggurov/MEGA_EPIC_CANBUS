# Branch Structure: MEGA_EPIC_CANBUS

**Last Updated:** December 2024

## Branch Overview

This project uses a two-branch strategy to balance focused development with future expansion:

### `main` Branch (Production Focus)
**Purpose:** Complete, production-ready Arduino Mega2560 firmware

**Current Status:**
- ✅ Phase 1 Complete: Analog and digital input transmission operational
- 🚧 Phase 2 In Progress: CAN RX parsing and digital output control
- **Target:** Full-featured firmware with all I/O modules operational

**Contents:**
- Complete Arduino Mega2560 implementation
- MCP_CAN shield integration
- All I/O modules (analog inputs, digital I/O, PWM outputs)
- Production-ready error handling and robustness
- Complete documentation

**Workflow:**
- Main development happens here
- Focus on single platform (Arduino Mega2560)
- Optimizations specific to Mega2560 hardware
- Stable, tested releases

### `expansion` Branch (Multi-Platform Architecture)
**Purpose:** Framework for multi-platform epicEFI system implementations

**Current Status:**
- Planning and architecture phase
- Platform abstraction layer design
- Shared EPIC protocol library structure

**Contents:**
- Platform abstraction layer (CAN, I/O interfaces)
- Shared EPIC protocol library (platform-agnostic)
- Platform-specific implementations:
  - Arduino Mega2560 (reference implementation)
  - ESP32 variant (future)
  - STM32 variant (future)
  - Other platforms (future)

**Workflow:**
- Experimental and architectural work
- Platform abstraction design
- New platform implementations
- Shared library development
- Can be merged back to `main` when stable

## Development Plan Reference

**See:** `.project/developmentPlan.md` for complete roadmap

**Key Phases:**
1. ✅ Phase 1: Core I/O Transmission (Complete)
2. 🚧 Phase 2: CAN RX Processing & Output Control (Current)
3. ⏳ Phase 3: PWM Output Module
4. ⏳ Phase 4: Function Call Support
5. ⏳ Phase 5: Performance Optimization
6. ⏳ Phase 6: Error Handling & Robustness
7. ⏳ Phase 7: Advanced Features
8. ⏳ Phase 8: Testing & Validation
9. ⏳ Phase 9: Documentation & Production Readiness

## Switching Between Branches

```bash
# Work on main branch (current development)
git checkout main

# Work on expansion branch (architecture/multi-platform)
git checkout expansion

# Create feature branch from main
git checkout -b feature/new-feature main

# Create feature branch from expansion
git checkout -b feature/platform-esp32 expansion
```

## Branch Strategy Rationale

### Why Two Branches?

**Separation of Concerns:**
- `main`: Focused, single-platform, production-ready code
- `expansion`: Experimental, multi-platform, architectural exploration

**Risk Management:**
- Experimental changes in `expansion` don't disrupt `main`
- `main` remains stable and deployable
- Can merge stable features from `expansion` to `main`

**Development Velocity:**
- Parallel work: `main` focuses on core features, `expansion` explores new platforms
- No conflicts between production work and experimental architecture
- Shared code can be merged between branches as needed

## Future Branch Structure

As the project grows, we may add:

- `main` - Production Arduino Mega2560 firmware
- `expansion` - Multi-platform architecture
- `release/v1.0` - Tagged releases (when ready)
- `feature/*` - Feature branches (merged when complete)
- `hotfix/*` - Critical bug fixes (merged immediately)

## Contributing

**For Arduino Mega2560 development:**
- Branch from `main`
- Submit PRs to `main`
- Focus on Mega2560-specific features and optimizations

**For multi-platform work:**
- Branch from `expansion`
- Submit PRs to `expansion`
- Focus on platform abstraction and new platform implementations

**For shared protocol/library work:**
- Can be developed in either branch
- May need to merge to both branches if applicable

