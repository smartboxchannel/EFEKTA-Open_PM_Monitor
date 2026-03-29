#ifndef UTILS_H
#define UTILS_H

/*********************************************************************
 * FUNCTION PROTOTYPES
 *********************************************************************/

extern double mapRange(double a1, double a2, double b1, double b2, double s);
extern uint16 adcReadSampled(uint8 channel, uint8 resolution, uint8 reference, uint8 samplesCount);

/*********************************************************************
 * I/O MACROS
 *********************************************************************/

#undef P
#undef INP
#define INP INP
#undef DIR
#define DIR DIR
#undef SEL
#define SEL SEL

// General I/O definitions
#define IO_GIO 0 // General purpose I/O
#define IO_PER 1 // Peripheral function
#define IO_IN  0 // Input pin
#define IO_OUT 1 // Output pin
#define IO_PUD 0 // Pull-up/pull-down input
#define IO_TRI 1 // Tri-state input
#define IO_PUP 0 // Pull-up input pin
#define IO_PDN 1 // Pull-down input pin

/* I/O PORT CONFIGURATION MACROS */
#define CAT1(x, y) x##y       // Concatenates 2 strings
#define CAT2(x, y) CAT1(x, y) // Forces evaluation of CAT1

// Builds I/O port name: PNAME(1,INP) ==> P1INP
#define PNAME(y, z) CAT2(P, CAT2(y, z))
// Builds I/O bit name: BNAME(1,2) ==> P1_2
#define BNAME(port, pin) CAT2(CAT2(P, port), CAT2(_, pin))

/*********************************************************************
 * I/O CONFIGURATION MACROS
 *********************************************************************/

// Set I/O port pin direction
#define IO_DIR_PORT_PIN(port, pin, dir)                                                                                                    \
    {                                                                                                                                      \
        if (dir == IO_OUT)                                                                                                                 \
            PNAME(port, DIR) |= (1 << (pin));                                                                                              \
        else                                                                                                                               \
            PNAME(port, DIR) &= ~(1 << (pin));                                                                                             \
    }

// Set I/O port pin function (general purpose or peripheral)
#define IO_FUNC_PORT_PIN(port, pin, func)                                                                                                  \
    {                                                                                                                                      \
        if (port < 2) {                                                                                                                    \
            if (func == IO_PER)                                                                                                            \
                PNAME(port, SEL) |= (1 << (pin));                                                                                          \
            else                                                                                                                           \
                PNAME(port, SEL) &= ~(1 << (pin));                                                                                         \
        } else {                                                                                                                           \
            if (func == IO_PER)                                                                                                            \
                P2SEL |= (1 << (pin >> 1));                                                                                                \
            else                                                                                                                           \
                P2SEL &= ~(1 << (pin >> 1));                                                                                               \
        }                                                                                                                                  \
    }

// Set I/O port pin input mode (pull-up/pull-down or tri-state)
#define IO_IMODE_PORT_PIN(port, pin, mode)                                                                                                 \
    {                                                                                                                                      \
        if (mode == IO_TRI)                                                                                                                \
            PNAME(port, INP) |= (1 << (pin));                                                                                              \
        else                                                                                                                               \
            PNAME(port, INP) &= ~(1 << (pin));                                                                                             \
    }

// Set I/O port pull-up/pull-down direction
#define IO_PUD_PORT(port, dir)                                                                                                             \
    {                                                                                                                                      \
        if (dir == IO_PDN)                                                                                                                 \
            P2INP |= (1 << (port + 5));                                                                                                    \
        else                                                                                                                               \
            P2INP &= ~(1 << (port + 5));                                                                                                   \
    }

#endif /* UTILS_H */