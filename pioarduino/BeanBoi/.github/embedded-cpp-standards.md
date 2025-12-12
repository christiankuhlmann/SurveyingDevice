# Embedded C++ Standards for BeanBoi

## Applicable Standards
- MISRA C++ 2008 (safety-critical embedded systems)
- AUTOSAR C++14 Guidelines (automotive-grade embedded)
- JPL Institutional Coding Standard (resource-constrained systems)
- Google C++ Style Guide (general readability)

## Project-Specific Constraints
- ESP32 platform (Xtensa LX6, 520KB SRAM)
- FreeRTOS RTOS
- Arduino framework compatibility
- Eigen library conventions
- No exceptions (embedded constraint)
- Float precision only (FPU optimization)

## Priority Areas
1. Memory safety (stack overflows, heap fragmentation)
2. Resource management (RAII patterns)
3. Const correctness
4. Naming conventions
5. Error handling patterns