/*
 * gpio.cpp - digital IO handling functions
 * This file is part of the g2core project
 *
 * Copyright (c) 2015 - 2107 Alden S. Hart, Jr.
 * Copyright (c) 2015 - 2017 Robert Giseburt
 *
 * This file ("the software") is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2 as published by the
 * Free Software Foundation. You should have received a copy of the GNU General Public
 * License, version 2 along with the software. If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, you may use this file as part of a software library without
 * restriction. Specifically, if other files instantiate templates or use macros or
 * inline functions from this file, or you compile this file and link it with  other
 * files to produce an executable, this file does not by itself cause the resulting
 * executable to be covered by the GNU General Public License. This exception does not
 * however invalidate any other reasons why the executable file might be covered by the
 * GNU General Public License.
 *
 * THE SOFTWARE IS DISTRIBUTED IN THE HOPE THAT IT WILL BE USEFUL, BUT WITHOUT ANY
 * WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
 * SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
/* Switch Modes
 *
 *  The switches are considered to be homing switches when cycle_state is
 *  CYCLE_HOMING. At all other times they are treated as limit switches:
 *    - Hitting a homing switch puts the current move into feedhold
 *    - Hitting a limit switch causes the machine to shut down and go into lockdown until reset
 *
 *  The normally open switch modes (NO) trigger an interrupt on the falling edge
 *  and lockout subsequent interrupts for the defined lockout period. This approach
 *  beats doing debouncing as an integration as switches fire immediately.
 *
 *  The normally closed switch modes (NC) trigger an interrupt on the rising edge
 *  and lockout subsequent interrupts for the defined lockout period. Ditto on the method.
 */

#include "g2core.h"  // #1
#include "config.h"  // #2
#include "gpio.h"
#include "stepper.h"
#include "encoder.h"
#include "hardware.h"
#include "canonical_machine.h"

#include "text_parser.h"
#include "controller.h"
#include "util.h"
#include "report.h"
#include "xio.h"

#include "MotateTimers.h"
using namespace Motate;

/****
 *
 * Important: Do not directly allocate or access pins here!
 * Do that in board_gpio.cpp!
 *
 ****/


/************************************************************************************
 **** CODE **************************************************************************
 ************************************************************************************/
/*
 * gpio_init() - initialize inputs and outputs
 * gpio_reset() - reset inputs and outputs (no initialization)
 */

void gpio_init(void)
{
    return(gpio_reset());
}

// implement void outputs_reset(void) in board_gpio.cpp
// implement void inputs_reset(void) in board_gpio.cpp

void gpio_reset(void)
{
    inputs_reset();
    outputs_reset();
}

/********************************************
 **** Digital Input Supporting Functions ****
 ********************************************/

/*
 * gpio_set_homing_mode()   - set/clear input to homing mode
 * gpio_set_probing_mode()  - set/clear input to probing mode
 * gpio_get_probing_input() - get probing input
 * gpio_read_input()        - read conditioned input
 *
 * Note: input_num_ext means EXTERNAL input number -- 1-based
 * TODO: lookup the external number to find the internal input, don't assume
 */
int8_t gpio_get_probing_input(void)
{
}

bool gpio_read_input(const uint8_t input_num_ext)
{
    if (input_num_ext == 0) {
        return false;
    }
    return (d_in[input_num_ext-1]->getState());
}


/***********************************************************************************
 * CONFIGURATION AND INTERFACE FUNCTIONS
 * Functions to get and set variables from the cfgArray table
 * These functions are not part of the NIST defined functions
 ***********************************************************************************/

// internal helpers to get input and output object from the cfgArray target
// specified by an nv pointer

template <typename type>
type* _io(const nvObj_t *nv) {
    return reinterpret_cast<type*>(cfgArray[nv->index].target);
};

gpioDigitalInput* _i(const nvObj_t *nv) { return _io<gpioDigitalInput>(nv); }
gpioDigitalInputReader* _ir(const nvObj_t *nv) { return _io<gpioDigitalInputReader>(nv); }
gpioDigitalOutput* _o(const nvObj_t *nv) { return _io<gpioDigitalOutput>(nv); }
gpioDigitalOutputReader* _or(const nvObj_t *nv) { return _io<gpioDigitalOutputReader>(nv); }
gpioAnalogInput* _ai(const nvObj_t *nv) { return _io<gpioAnalogInput>(nv); }

gpioDigitalInputReader in1;
gpioDigitalInputReader in2;
gpioDigitalInputReader in3;
gpioDigitalInputReader in4;
gpioDigitalInputReader in5;
gpioDigitalInputReader in6;
gpioDigitalInputReader in7;
gpioDigitalInputReader in8;
gpioDigitalInputReader in9;
gpioDigitalInputReader in10;
gpioDigitalInputReader in11;
gpioDigitalInputReader in12;
gpioDigitalInputReader in13;
gpioDigitalInputReader in14;

gpioDigitalInputReader* const in_r[14] = {&in1, &in2, &in3, &in4, &in5, &in6, &in7, &in8, &in9, &in10, &in11, &in12, &in13, &in14};

gpioDigitalOutputReader out1;
gpioDigitalOutputReader out2;
gpioDigitalOutputReader out3;
gpioDigitalOutputReader out4;
gpioDigitalOutputReader out5;
gpioDigitalOutputReader out6;
gpioDigitalOutputReader out7;
gpioDigitalOutputReader out8;
gpioDigitalOutputReader out9;
gpioDigitalOutputReader out10;
gpioDigitalOutputReader out11;
gpioDigitalOutputReader out12;
gpioDigitalOutputReader out13;
gpioDigitalOutputReader out14;

gpioDigitalOutputReader* const out_r[14] = {&out1 ,&out2 ,&out3 ,&out4 ,&out5 ,&out6 ,&out7 ,&out8 ,&out9 ,&out10 ,&out11 ,&out12 ,&out13 ,&out14};

// lists for the various inputAction events
gpioDigitalInputHandlerList din_handlers[INPUT_ACTION_ACTUAL_MAX+1];


/*
 *  Get/set enabled
 */
stat_t din_get_en(nvObj_t *nv)
{
    return _i(nv)->getEnabled(nv);
}
stat_t din_set_en(nvObj_t *nv)
{
    return _i(nv)->setEnabled(nv);
}

/*
 *  Get/set input polarity
 */
stat_t din_get_po(nvObj_t *nv)
{
    return _i(nv)->getPolarity(nv);
}
stat_t din_set_po(nvObj_t *nv)
{
    return _i(nv)->setPolarity(nv);
}

/*
 *  Get/set input action
 */
stat_t din_get_ac(nvObj_t *nv)
{
    return _i(nv)->getAction(nv);
}
stat_t din_set_ac(nvObj_t *nv)
{
    return _i(nv)->setAction(nv);
}

/*
 *  Get/set input function
 */

/*
 *  Get/set input function
 */
stat_t din_get_in(nvObj_t *nv)
{
    return _i(nv)->getExternalNumber(nv);
}
stat_t din_set_in(nvObj_t *nv)
{
    return _i(nv)->setExternalNumber(nv);
}

/*
 *  Get input state given an nv object
 *  Note: if this is not forwarded to an input or the input is disabled,
 *  it returns NULL.
 */
stat_t din_get_input(nvObj_t *nv)
{
    return _ir(nv)->getState(nv);
}


/*
 *  Get/set enabled
 */
stat_t dout_get_en(nvObj_t *nv)
{
    return _o(nv)->getEnabled(nv);
}
stat_t dout_set_en(nvObj_t *nv)
{
    return _o(nv)->setEnabled(nv);
}

/*
 *  Get/set input polarity
 */
stat_t dout_get_po(nvObj_t *nv)
{
    return _o(nv)->getPolarity(nv);
}
stat_t dout_set_po(nvObj_t *nv)
{
    return _o(nv)->setPolarity(nv);
}

/*
 *  Get/set input polarity
 */
stat_t dout_get_out(nvObj_t *nv)
{
    return _o(nv)->getExternalNumber(nv);
}
stat_t dout_set_out(nvObj_t *nv)
{
    return _o(nv)->setExternalNumber(nv);
}


/*
 *  Gget/set output state given an nv object
 */
stat_t dout_get_output(nvObj_t *nv)
{
    return _or(nv)->getValue(nv);
}
stat_t dout_set_output(nvObj_t *nv)
{
    return _or(nv)->setValue(nv);
}


/*
 *  ain_get_value() - get the measured voltage level of the analog input
 */
stat_t ain_get_value(nvObj_t *nv) {
    return _ai(nv)->getValue(nv);
}
// no ain_set_value

/*
 *  ain_get_resistance() - get the measured resistance of the analog input
 *  NOTE: Requires the circuit type to be configured and the relevant parameters set
 */
stat_t ain_get_resistance(nvObj_t *nv) {
    return _ai(nv)->getResistance(nv);
}
// no ain_set_resistance

/*
 *  ain_get_type() - get the measured voltage level of the analog input
 */
stat_t ain_get_type(nvObj_t *nv) {
    return _ai(nv)->getType(nv);
}
stat_t ain_set_type(nvObj_t *nv) {
    return _ai(nv)->setType(nv);
}

stat_t ain_get_circuit(nvObj_t *nv) {
    return _ai(nv)->getCircuit(nv);
}
stat_t ain_set_circuit(nvObj_t *nv) {
    return _ai(nv)->setCircuit(nv);
}

stat_t ain_get_parameter(nvObj_t *nv, const uint8_t p) {
    return _ai(nv)->getParameter(nv, p);
}
stat_t ain_set_parameter(nvObj_t *nv, const uint8_t p) {
    return _ai(nv)->setParameter(nv, p);
}

stat_t ain_get_p1(nvObj_t *nv) { return ain_get_parameter(nv, 0); };
stat_t ain_set_p1(nvObj_t *nv) { return ain_set_parameter(nv, 0); };
stat_t ain_get_p2(nvObj_t *nv) { return ain_get_parameter(nv, 1); };
stat_t ain_set_p2(nvObj_t *nv) { return ain_set_parameter(nv, 1); };
stat_t ain_get_p3(nvObj_t *nv) { return ain_get_parameter(nv, 2); };
stat_t ain_set_p3(nvObj_t *nv) { return ain_set_parameter(nv, 2); };
stat_t ain_get_p4(nvObj_t *nv) { return ain_get_parameter(nv, 3); };
stat_t ain_set_p4(nvObj_t *nv) { return ain_set_parameter(nv, 3); };
stat_t ain_get_p5(nvObj_t *nv) { return ain_get_parameter(nv, 4); };
stat_t ain_set_p5(nvObj_t *nv) { return ain_set_parameter(nv, 4); };

/***********************************************************************************
 * TEXT MODE SUPPORT
 * Functions to print variables from the cfgArray table
 ***********************************************************************************/

#ifdef __TEXT_MODE

    static const char fmt_gpio_in_en[] = "[%sen] input enabled%13d [-1=unavailable,0=disabled,1=enabled]\n";
    static const char fmt_gpio_in_po[] = "[%spo] input polarity%13d [0=normal/active-high,1=inverted/active-low]\n";
    static const char fmt_gpio_ac[] = "[%sac] input action%15d [0=none,1=stop,2=fast_stop,3=halt,4=alarm,5=shutdown,6=panic,7=reset]\n";
    static const char fmt_gpio_fn[] = "[%sfn] input function%13d [0=none,1=limit,2=interlock,3=shutdown,4=probe]\n";
    static const char fmt_gpio_in[] = "[%sin] input external number%6d [0=none,1-12=inX shows the value of this din]\n";
    static const char fmt_gpio_state[] = "Input %s state: %5d\n";

    static const char fmt_gpio_out_en[] = "[%sen] output enabled%12d [-1=unavailable,0=disabled,1=enabled]\n";
    static const char fmt_gpio_out_po[] = "[%spo] output polarity%12d [0=normal/active-high,1=inverted/active-low]\n";
    static const char fmt_gpio_out_out[] = "[%sout] output external number%5d [0=none,1-14=outX shows the value of this dout]\n";
    static const char fmt_gpio_out_state[] = "Output %s state: %5d\n";

    static const char fmt_ain_value[] = "Analog input %s voltage: %5.2fV\n";
    static const char fmt_ain_resistance[] = "Analog input %s resistance: %5.2fohm\n";
    static const char fmt_ain_type[] = "[%s] input type%17d [0=disabled,1=internal,2=external]\n";
    static const char fmt_ain_circuit[] = "[%s] analog circuit%13d [0=disabled,1=pull-up,2=external,3=inverted op-amp,4=constant current inverted op-amp]\n";
    static const char fmt_ain_parameter[] = "[%s] circuit parameter%6.4f [usage varies by circuit type]\n";

    static void _print_di(nvObj_t *nv, const char *format)
    {
        sprintf(cs.out_buf, format, nv->group, (int)nv->value);
        xio_writeline(cs.out_buf);
    }
    void din_print_en(nvObj_t *nv) {_print_di(nv, fmt_gpio_in_en);}
    void din_print_po(nvObj_t *nv) {_print_di(nv, fmt_gpio_in_po);}
    void din_print_ac(nvObj_t *nv) {_print_di(nv, fmt_gpio_ac);}
    void din_print_fn(nvObj_t *nv) {_print_di(nv, fmt_gpio_fn);}
    void din_print_in(nvObj_t *nv) {_print_di(nv, fmt_gpio_in);}
    void din_print_state(nvObj_t *nv) {
        sprintf(cs.out_buf, fmt_gpio_state, nv->token, (int)nv->value);
        xio_writeline(cs.out_buf);
    }

    void dout_print_en(nvObj_t *nv) {_print_di(nv, fmt_gpio_out_en);}
    void dout_print_po(nvObj_t *nv) {_print_di(nv, fmt_gpio_out_po);}
    void dout_print_out(nvObj_t *nv) {_print_di(nv, fmt_gpio_out_out);}
    void dout_print_out_state(nvObj_t *nv) {
        sprintf(cs.out_buf, fmt_gpio_out_state, nv->token, (int)nv->value);
        xio_writeline(cs.out_buf);
    }

    void ain_print_value(nvObj_t *nv)
    {
       sprintf(cs.out_buf, fmt_ain_value, nv->token, (float)nv->value);
       xio_writeline(cs.out_buf);
    }
    void ain_print_resistance(nvObj_t *nv)
    {
       sprintf(cs.out_buf, fmt_ain_resistance, nv->token, (float)nv->value);
       xio_writeline(cs.out_buf);
    }
    void ain_print_type(nvObj_t *nv)
    {
       sprintf(cs.out_buf, fmt_ain_type, nv->token, (int)nv->value);
       xio_writeline(cs.out_buf);
    }

    void ain_print_circuit(nvObj_t *nv)
    {
       sprintf(cs.out_buf, fmt_ain_circuit, nv->token, (int)nv->value);
       xio_writeline(cs.out_buf);
    }

    void ain_print_p(nvObj_t *nv)
    {
       sprintf(cs.out_buf, fmt_ain_parameter, nv->token, (float)nv->value);
       xio_writeline(cs.out_buf);
    }

#endif
