/*
 * main.c
 *
 *  Created on: 23 lis 2022
 *      Author: Piotr
 */


#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include "timers/timers.h"
#include "ifRS485/ifRS485.h"
#include "ds18b20/ds18b20.h"
#include "ds18b20/pindef.h"
#include "ds18b20/onewire.h"
