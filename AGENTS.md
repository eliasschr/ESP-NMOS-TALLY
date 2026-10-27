# AGENTS.md — ESP-NMOS-TALLY

## 1. Purpose and Philosophy

ESP-NMOS-TALLY is an open-source embedded firmware project providing network-connected tally lights for professional media and broadcast environments.

This file defines the engineering principles and mandatory development rules for all contributors, including AI coding agents.

**Our fundamental principle:**

> We build software for humans, not machines.

Software must be understandable, maintainable, secure, reliable, and accessible to future developers.

We value clarity over cleverness, simplicity over unnecessary abstraction, and long-term maintainability over short-term convenience.

A working solution is not automatically a good solution.

### Core priorities

In order of importance:

1. Security and safety
2. Correctness and reliability
3. Readability and maintainability
4. Testability and observability
5. Performance and resource efficiency
6. Features and convenience

Security must never be sacrificed for performance, convenience, or development speed.

---

## 2. General Development Rules

All contributors and agents MUST follow these rules:

- Write readable, self-explanatory code.
- Follow established C++ and embedded development best practices.
- Prefer simple, proven solutions.
- Keep functions short and focused.
- Apply the Single Responsibility Principle.
- Avoid unnecessary abstraction and complexity.
- Avoid duplicated logic.
- Use meaningful variable, function, class, and file names.
- Prefer explicit behavior over hidden side effects.
- Avoid deeply nested conditionals.
- Avoid unnecessary global mutable state.
- Handle errors explicitly.
- Never silently ignore failures.
- Keep dependencies minimal.
- Do not introduce new frameworks without a clear technical justification.
- Avoid premature optimization.
- Do not rewrite functioning code without a documented reason.

### Complexity

Functions should perform one clearly defined task.

As a guideline:

- Prefer functions shorter than approximately 40 lines.
- Consider refactoring functions exceeding approximately 60 lines.
- Avoid deeply nested logic.
- Prefer early returns where they improve readability.
- Prefer composition over complicated inheritance hierarchies.

These limits are guidelines, not arbitrary enforcement targets. Never split cohesive logic into meaningless helper functions merely to satisfy a line count.

If a solution becomes difficult to explain, reconsider its design.

---

## 3. Documentation Is Mandatory

Documentation is part of the implementation, not optional follow-up work.

Whenever behavior, architecture, configuration, interfaces, or hardware requirements change, update the relevant documentation in the same change.

### Required documentation

Maintain:

- `README.md` — Project overview and getting started
- `docs/architecture.md` — Software architecture and design decisions
- `docs/hardware.md` — Supported hardware, wiring, and electrical constraints
- `docs/getting-started.md` — Build, flash, and initial setup
- `docs/nmos-integration.md` — NMOS interoperability and limitations
- `docs/security.md` — Security architecture, assumptions, and known risks
- `docs/roadmap.md` — Planned functionality and development status

Create additional documents only when they provide meaningful value.

### Documentation rules

- Explain why important design decisions were made.
- Document limitations and trade-offs.
- Include examples where helpful.
- Keep instructions reproducible.
- Document configuration settings and defaults.
- Clearly distinguish implemented from planned functionality.
- Update documentation when behavior changes.
- Remove or correct outdated information.

Do not claim a feature is complete without evidence.

---

## 4. Code Comments

Code comments are important for future developers.

However, comments must add value.

### Required commenting practices

Comment:

- Non-obvious algorithms and calculations
- Important architectural decisions
- Hardware-specific behavior
- Protocol-specific behavior
- Security-sensitive operations
- Workarounds and compatibility requirements
- Unexpected implementation constraints
- Public interfaces where intent, ownership, inputs, outputs, or errors are not obvious

Prefer explaining WHY something is implemented a certain way rather than merely repeating WHAT the code does.

### Good example

```cpp
// Keep the previous valid tally state during a temporary MQTT
// disconnect. After the configured timeout, enter WARNING to
// avoid displaying potentially stale production information.
```

### Bad example

```cpp
// Set brightness to 50
brightness = 50;
```

### Commenting rules

- Write comments in clear English.
- Keep comments accurate and up to date.
- Never use comments to hide poor design.
- Do not add redundant comments to every line.
- Document assumptions and important constraints.
- Use Doxygen-style comments for public APIs when helpful.
- Reference relevant standards or specifications when implementing protocol-specific behavior.

All TODO and FIXME comments must describe an actionable problem. Reference an issue when appropriate.

Never leave misleading or obsolete comments.

---

## 5. Security by Design

**Security is a mandatory project requirement.**

The firmware operates on potentially sensitive production networks and may interact with broadcast infrastructure.

All externally supplied information must be treated as untrusted.

### Network security

- Never assume a local network is trusted.
- Validate all incoming network data.
- Restrict message and payload sizes.
- Use appropriate connection and operation timeouts.
- Do not expose unnecessary network services.
- Minimize open ports and attack surface.
- Avoid insecure fallback behavior.
- Apply least-privilege principles where applicable.
- Handle malformed requests without crashing.
- Apply authentication and authorization to configuration-changing operations.
- Protect against unauthorized configuration changes and CSRF.
- Use secure transport and certificate validation where supported and required.
- Clearly document any intentionally unencrypted communication.

### Credentials and secrets

NEVER:

- Hardcode production credentials.
- Commit passwords, tokens, or private keys.
- Log secrets.
- Expose stored credentials through HTTP endpoints.
- Return secrets in debug or diagnostic output.
- Use shared, universal default administrator credentials.
- Disable certificate verification as a shortcut.

Credentials must be handled carefully during provisioning, storage, and updates.

Do not claim that storage is encrypted unless actual encryption and key protection have been implemented and verified.

### Wi-Fi provisioning

- Provisioning mode must be explicitly defined.
- Avoid universally known default access-point passwords.
- Prefer device-specific credentials or an explicitly authorized provisioning process.
- Ensure credentials are never returned by status endpoints.
- Clearly distinguish provisioning mode from normal operation.
- Avoid exposing unauthenticated configuration endpoints on production networks.
- Provide a documented recovery or factory-reset mechanism.

### Web interface security

- Validate all input on the device, not only in JavaScript.
- Enforce request-size and field-length limits.
- Sanitize or safely encode dynamic output.
- Avoid unsafe HTML injection.
- Avoid storing credentials in browser storage.
- Implement authentication for protected operations.
- Protect state-changing HTTP endpoints against cross-site request forgery.
- Do not make security claims that exceed the implemented protections.

### MQTT and NMOS security

- Validate all incoming MQTT messages.
- Validate NMOS event payload types, structure, and expected fields.
- Define accepted topics and sources explicitly.
- Reject or safely ignore unexpected payloads.
- Handle connection loss and stale events safely.
- Avoid unbounded retries and blocking reconnect loops.
- Support broker authentication when implemented.
- Document transport security limitations.
- Never assume that an MQTT message is trustworthy solely because it arrived from a configured broker.

### Embedded security

- Use bounded buffers.
- Avoid unsafe memory operations.
- Check allocation failures where applicable.
- Avoid unbounded dynamic allocation.
- Avoid uncontrolled recursive processing.
- Handle filesystem and configuration corruption safely.
- Prefer safe defaults.
- Do not enable debugging interfaces in production without justification.
- Never claim firmware signing, secure boot, or encrypted storage without an actual implementation.

### Security reviews

Before completing a change involving networking, configuration, authentication, firmware updates, or external input:

1. Identify relevant threats.
2. Check trust boundaries.
3. Validate input handling.
4. Review access controls.
5. Review failure behavior.
6. Check for credential exposure.
7. Document residual risks.

Security-related changes require explicit validation.

---

## 6. Embedded Development Rules

The initial target is:

- Adafruit Feather HUZZAH ESP8266
- Adafruit NeoPixel FeatherWing 4×8 RGB

ESP32 support may be added later.

The Pycom LoPy is explicitly outside the current project scope.

### Resource management

ESP8266 devices have constrained RAM, flash memory, and processing capacity.

Therefore:

- Avoid large temporary buffers.
- Avoid unnecessary copies of strings and payloads.
- Minimize heap fragmentation.
- Avoid repeated dynamic allocations in hot paths.
- Keep static web assets small.
- Use flash storage appropriately.
- Avoid unnecessary background processing.
- Minimize LED refresh overhead.
- Monitor RAM and flash usage.

### Runtime behavior

- Avoid blocking operations in the main execution path.
- Avoid long `delay()` calls during normal operation.
- Use timeouts for network operations.
- Implement bounded retries.
- Handle Wi-Fi disconnections gracefully.
- Handle MQTT disconnections independently.
- Use watchdog-friendly behavior.
- Keep the local configuration interface recoverable.
- Avoid unnecessary writes to flash memory.

Do not allow network failures to crash or permanently lock up the device.

### Hardware abstraction

Keep hardware-specific logic separate from application logic.

Do not unnecessarily couple NMOS behavior to:

- GPIO pin numbers
- NeoPixel implementations
- ESP8266-specific network classes
- Device-specific filesystem details

Avoid pretending different platforms are interchangeable when hardware-specific behavior requires separate implementations.

---

## 7. Architecture and Modularity

Follow a modular architecture.

Separate responsibilities such as:

- Application lifecycle
- Configuration management
- Network management
- HTTP API
- MQTT transport
- NMOS event processing
- Tally state machine
- LED rendering
- Platform-specific hardware

### Dependency rules

- Business logic should not depend unnecessarily on hardware libraries.
- The tally state machine should be testable without physical hardware.
- Network transport should not directly control LED hardware.
- Protocol handling should remain separate from visual state rendering.
- Prefer explicit interfaces between components.

Avoid overengineering.

Do not introduce factories, service locators, template-heavy abstractions, or complex dependency injection frameworks unless they solve a demonstrated problem.

The simplest maintainable architecture is preferred.

---

## 8. NMOS and Protocol Compliance

NMOS functionality must follow applicable AMWA specifications.

Relevant specifications include:

- IS-04 — Discovery and Registration
- IS-05 — Device Connection Management
- IS-07 — Event and Tally

Do not invent protocol behavior.

Do not confuse application-specific tally conventions with mandatory specification behavior.

### Protocol rules

- Consult the applicable specification before implementing behavior.
- Track supported specification versions.
- Validate required fields and message structures.
- Handle unsupported versions explicitly.
- Document known interoperability limitations.
- Keep protocol parsing separate from application behavior.
- Implement reconnection and stale-state handling where relevant.

A working integration with one test system does not prove complete NMOS conformance.

Never claim AMWA certification or full compliance without appropriate evidence.

---

## 9. Testing and Quality Assurance

Testing is mandatory.

Every meaningful change should include appropriate verification.

### Testing priorities

1. Core logic unit tests
2. Configuration validation tests
3. Protocol parsing tests
4. Error-handling tests
5. Build verification
6. Hardware integration tests

Cover normal behavior, invalid input, boundary conditions, and recovery from errors.

### Required principles

- Tests must be deterministic where practical.
- Tests should not depend on external production services.
- Mock external dependencies where appropriate.
- Avoid tests that merely duplicate implementation logic.
- Add regression tests for confirmed bugs.
- Keep test names descriptive.
- Do not weaken tests merely to make them pass.
- Do not remove failing tests without justification.

### Verification transparency

Always distinguish between:

- Code written
- Code compiled
- Unit tests passed
- Integration tests passed
- Hardware tested
- Specification conformance verified

Never fabricate successful test results.

A successful firmware build does not imply successful hardware behavior.

---

## 10. Error Handling and Logging

Errors must be handled predictably and visibly.

### Error handling

- Return meaningful error information.
- Fail safely whenever possible.
- Avoid silent fallback that conceals dangerous behavior.
- Handle unavailable peripherals.
- Handle invalid configuration.
- Handle network interruptions.
- Define behavior for stale or missing tally information.

For tally indication, consider that displaying stale PROGRAM or PREVIEW information can mislead operators.

Document the selected failure-state policy.

### Logging

Logs should help developers and operators diagnose problems.

Prefer consistent severity levels:

- ERROR
- WARN
- INFO
- DEBUG

Rules:

- Keep messages actionable.
- Avoid excessive logging.
- Never log credentials.
- Avoid unnecessarily exposing identifying information.
- Do not emit unbounded log output.
- Avoid expensive formatting in timing-sensitive paths.

---

## 11. Web Interface and User Experience

The project should be usable by people who are not firmware developers.

The web interface must be:

- Understandable
- Responsive
- Accessible
- Consistent
- Lightweight
- Secure by default

### UX rules

- Use clear language.
- Show meaningful connection status.
- Explain errors rather than only showing error codes.
- Clearly identify manual versus externally controlled tally states.
- Avoid hidden critical settings.
- Confirm destructive operations.
- Provide helpful recovery instructions.
- Do not misrepresent planned or disconnected features as functional.

Prefer vanilla HTML, CSS, and JavaScript.

Avoid unnecessary frontend frameworks and external CDN dependencies.

Keep the interface usable on common desktop and mobile browsers.

---

## 12. Dependencies

Dependencies are part of the security and maintenance surface.

Before introducing a dependency:

1. Explain why it is required.
2. Check whether an existing dependency already solves the problem.
3. Assess maintenance and compatibility.
4. Evaluate licensing requirements.
5. Check known security issues where information is available.
6. Prefer established and maintained libraries.
7. Pin versions or otherwise use a reproducible dependency strategy.

Do not introduce unnecessary dependencies.

Do not update unrelated dependencies as part of a focused change.

---

## 13. Git and Change Management

Keep changes small, focused, and reviewable.

### Rules

- Inspect the repository before editing.
- Respect the current branch and repository state.
- Do not overwrite unrelated user changes.
- Do not modify unrelated repositories.
- Prefer incremental changes.
- Use descriptive commit messages.
- Review diffs before committing.
- Run appropriate tests before declaring completion.
- Never commit generated artifacts, secrets, or local environment files unless explicitly intended.

Do not force-push or rewrite shared history without explicit authorization.

Do not push changes unless explicitly requested.

A task that explicitly requests a commit authorizes a commit only after its stated verification requirements have been met.

### Commit messages

Use Conventional Commits where practical.

Examples:

- `feat: add Wi-Fi provisioning`
- `fix: handle MQTT reconnection timeout`
- `refactor: simplify tally state handling`
- `docs: document hardware pin mapping`
- `test: add configuration validation tests`
- `security: restrict configuration endpoints`

Do not combine unrelated changes into a single commit without a good reason.

---

## 14. Rules for AI Coding Agents

AI agents are contributors, not autonomous project owners.

Agents must:

- Read this file before modifying the repository.
- Understand the affected modules before editing them.
- Respect existing architecture and coding conventions.
- Prefer minimal, targeted changes.
- Avoid speculative refactoring.
- Avoid creating unnecessary files or abstractions.
- Document important design decisions.
- Keep comments meaningful and human-readable.
- Review security implications.
- Add or update relevant tests.
- Update relevant documentation.
- Report limitations and unresolved risks.
- Clearly state which checks actually ran.

Agents must NEVER:

- Fabricate test results.
- Claim hardware verification without physical testing.
- Silently weaken security.
- Bypass authentication to simplify development.
- Hide compiler warnings or test failures.
- Delete existing user work without authorization.
- Introduce unrelated features.
- Commit secrets.
- Claim specification conformance without evidence.

### Decision making

When multiple approaches are possible, prefer the one that is:

1. Secure
2. Correct
3. Easy to understand
4. Easy to maintain
5. Easy to test
6. Efficient enough for the hardware

Explain significant trade-offs.

If security and functionality conflict, preserve security and report the limitation.

---

## 15. Definition of Done

A task is complete only when:

- The requested functionality is implemented.
- The code follows project conventions.
- The implementation is readable and appropriately commented.
- Error paths have been considered.
- Relevant security implications have been reviewed.
- Appropriate tests have been added or updated.
- Available verification checks pass.
- Documentation reflects the actual behavior.
- Unverified behavior is explicitly identified.
- The change contains no unrelated modifications.

If any requirement cannot be fulfilled, report it transparently.

Do not describe incomplete work as complete.

---

## 16. Final Engineering Principle

We are not trying to produce the largest, most abstract, or most sophisticated codebase.

We are building software that future developers can understand, maintain, audit, extend, and trust.

**Readable code is a feature.**

**Documentation is part of the product.**

**Security is a requirement, not an enhancement.**

**Simplicity is a deliberate engineering choice.**

**Correctness must be demonstrated, not assumed.**

When in doubt, choose the solution that will be easiest for another developer to understand five years from now.